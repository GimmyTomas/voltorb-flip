// Emscripten bindings for the Voltorb Flip solver
// Exposes a single solveBoard() function that takes JS arrays and returns a JS object

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "voltorb/board.hpp"
#include "voltorb/board_type.hpp"
#include "voltorb/probability.hpp"
#include "voltorb/solver.hpp"

using namespace emscripten;
using namespace voltorb;

/**
 * Main entry point for the WASM solver.
 * Takes raw JS types and returns a plain JS result object matching the JS solver format.
 */
val solveBoard(int level, val panels,
               val rowSums, val rowVoltorbs,
               val colSums, val colVoltorbs,
               int timeout, int maxBoards) {

    // Construct Board from JS arrays
    Board board(static_cast<Level>(level));

    for (int i = 0; i < 5; i++) {
        board.setRowHint(static_cast<uint8_t>(i),
                         {static_cast<uint8_t>(rowSums[i].as<int>()),
                          static_cast<uint8_t>(rowVoltorbs[i].as<int>())});
        board.setColHint(static_cast<uint8_t>(i),
                         {static_cast<uint8_t>(colSums[i].as<int>()),
                          static_cast<uint8_t>(colVoltorbs[i].as<int>())});
        for (int j = 0; j < 5; j++) {
            int v = panels[i * 5 + j].as<int>();
            board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j),
                      static_cast<PanelValue>(v));
        }
    }

    // Configure solver
    SolverOptions opts;
    opts.timeout = std::chrono::milliseconds(timeout);
    opts.maxCompatibleBoards = static_cast<size_t>(maxBoards);
    opts.transpositionTableSize = 1 << 20;  // 1M entries, matching native default

    // Run solver
    Solver solver(opts);
    SolverResult solverResult = solver.solve(board);

    // Get compatible boards for probability calculation
    const auto& compatibleBoards = solver.getCompatibleBoards();
    bool capped = solver.isCapped();

    // Calculate probabilities
    BoardProbabilities probs;
    std::vector<Position> safePanels;

    if (!compatibleBoards.empty()) {
        probs = ProbabilityCalculator::calculate(board, compatibleBoards);
        safePanels = ProbabilityCalculator::findGuaranteedSafePanels(board, compatibleBoards);
    }

    // Build JS result object
    val result = val::object();

    // Suggested panel
    if (solverResult.boardsEvaluated > 0 || !compatibleBoards.empty()) {
        val sp = val::object();
        sp.set("row", static_cast<int>(solverResult.suggestedPosition.row));
        sp.set("col", static_cast<int>(solverResult.suggestedPosition.col));
        result.set("suggestedPanel", sp);
    } else {
        result.set("suggestedPanel", val::null());
    }

    result.set("winProbability", solverResult.winProbability);
    result.set("winProbabilityUpper", solverResult.winProbabilityUpper);
    result.set("compatibleCount", static_cast<int>(compatibleBoards.size()));
    result.set("capped", capped);
    result.set("depth", solverResult.searchDepth);
    result.set("isExact", solverResult.isExact);
    result.set("reason", solverResult.reason ? *solverResult.reason : std::string("Unknown"));

    // Probabilities object
    val probsObj = val::object();
    val panelsArr = val::array();

    for (const auto& pp : probs.panels) {
        val panelObj = val::object();
        val posObj = val::object();
        posObj.set("row", static_cast<int>(pp.pos.row));
        posObj.set("col", static_cast<int>(pp.pos.col));
        panelObj.set("pos", posObj);
        panelObj.set("pVoltorb", pp.pVoltorb);
        panelObj.set("pOne", pp.pOne);
        panelObj.set("pTwo", pp.pTwo);
        panelObj.set("pThree", pp.pThree);
        panelsArr.call<void>("push", panelObj);
    }
    probsObj.set("panels", panelsArr);

    // Type probabilities
    val typeProbsArr = val::array();
    for (int i = 0; i < NUM_TYPES_PER_LEVEL; i++) {
        typeProbsArr.call<void>("push", probs.typeProbs[i]);
    }
    probsObj.set("typeProbs", typeProbsArr);
    probsObj.set("totalCompatible", static_cast<int>(probs.totalCompatible));

    result.set("probabilities", probsObj);

    // Safe panels
    val safePanelsArr = val::array();
    for (const auto& pos : safePanels) {
        val posObj = val::object();
        posObj.set("row", static_cast<int>(pos.row));
        posObj.set("col", static_cast<int>(pos.col));
        safePanelsArr.call<void>("push", posObj);
    }
    result.set("safePanels", safePanelsArr);

    return result;
}

/**
 * Solver entry point with per-depth progress callback.
 * The JS callback receives {bestPanel, winProbability, depth, isExact, nodesSearched}
 * at each completed depth level, enabling progressive UI updates.
 */
val solveBoardWithProgress(int level, val panels,
                           val rowSums, val rowVoltorbs,
                           val colSums, val colVoltorbs,
                           int timeout, int maxBoards,
                           val progressCallback) {

    // Construct Board from JS arrays (identical to solveBoard)
    Board board(static_cast<Level>(level));

    for (int i = 0; i < 5; i++) {
        board.setRowHint(static_cast<uint8_t>(i),
                         {static_cast<uint8_t>(rowSums[i].as<int>()),
                          static_cast<uint8_t>(rowVoltorbs[i].as<int>())});
        board.setColHint(static_cast<uint8_t>(i),
                         {static_cast<uint8_t>(colSums[i].as<int>()),
                          static_cast<uint8_t>(colVoltorbs[i].as<int>())});
        for (int j = 0; j < 5; j++) {
            int v = panels[i * 5 + j].as<int>();
            board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j),
                      static_cast<PanelValue>(v));
        }
    }

    // Configure solver
    SolverOptions opts;
    opts.timeout = std::chrono::milliseconds(timeout);
    opts.maxCompatibleBoards = static_cast<size_t>(maxBoards);
    opts.transpositionTableSize = 1 << 20;  // 1M entries, matching native default

    // Build C++ progress callback that calls into JS
    Solver solver(opts);

    ProgressCallback cppCallback = nullptr;
    if (!progressCallback.isUndefined() && !progressCallback.isNull()) {
        cppCallback = [&progressCallback](const SolverProgress& progress) {
            val jsProgress = val::object();
            val bp = val::object();
            bp.set("row", static_cast<int>(progress.bestPanel.row));
            bp.set("col", static_cast<int>(progress.bestPanel.col));
            jsProgress.set("bestPanel", bp);
            jsProgress.set("winProbability", progress.winProbability);
            jsProgress.set("winProbabilityUpper", progress.winProbabilityUpper);
            jsProgress.set("depth", progress.depth);
            jsProgress.set("isExact", progress.isExact);
            jsProgress.set("nodesSearched", static_cast<int>(progress.nodesSearched));
            progressCallback(jsProgress);
        };
    }

    SolverResult solverResult = solver.solve(board, cppCallback);

    // Get compatible boards for probability calculation
    const auto& compatibleBoards = solver.getCompatibleBoards();
    bool capped = solver.isCapped();

    // Calculate probabilities
    BoardProbabilities probs;
    std::vector<Position> safePanels;

    if (!compatibleBoards.empty()) {
        probs = ProbabilityCalculator::calculate(board, compatibleBoards);
        safePanels = ProbabilityCalculator::findGuaranteedSafePanels(board, compatibleBoards);
    }

    // Build JS result object (identical to solveBoard)
    val result = val::object();

    if (solverResult.boardsEvaluated > 0 || !compatibleBoards.empty()) {
        val sp = val::object();
        sp.set("row", static_cast<int>(solverResult.suggestedPosition.row));
        sp.set("col", static_cast<int>(solverResult.suggestedPosition.col));
        result.set("suggestedPanel", sp);
    } else {
        result.set("suggestedPanel", val::null());
    }

    result.set("winProbability", solverResult.winProbability);
    result.set("winProbabilityUpper", solverResult.winProbabilityUpper);
    result.set("compatibleCount", static_cast<int>(compatibleBoards.size()));
    result.set("capped", capped);
    result.set("depth", solverResult.searchDepth);
    result.set("isExact", solverResult.isExact);
    result.set("reason", solverResult.reason ? *solverResult.reason : std::string("Unknown"));

    val probsObj = val::object();
    val panelsArr = val::array();

    for (const auto& pp : probs.panels) {
        val panelObj = val::object();
        val posObj = val::object();
        posObj.set("row", static_cast<int>(pp.pos.row));
        posObj.set("col", static_cast<int>(pp.pos.col));
        panelObj.set("pos", posObj);
        panelObj.set("pVoltorb", pp.pVoltorb);
        panelObj.set("pOne", pp.pOne);
        panelObj.set("pTwo", pp.pTwo);
        panelObj.set("pThree", pp.pThree);
        panelsArr.call<void>("push", panelObj);
    }
    probsObj.set("panels", panelsArr);

    val typeProbsArr = val::array();
    for (int i = 0; i < NUM_TYPES_PER_LEVEL; i++) {
        typeProbsArr.call<void>("push", probs.typeProbs[i]);
    }
    probsObj.set("typeProbs", typeProbsArr);
    probsObj.set("totalCompatible", static_cast<int>(probs.totalCompatible));

    result.set("probabilities", probsObj);

    val safePanelsArr = val::array();
    for (const auto& pos : safePanels) {
        val posObj = val::object();
        posObj.set("row", static_cast<int>(pos.row));
        posObj.set("col", static_cast<int>(pos.col));
        safePanelsArr.call<void>("push", posObj);
    }
    result.set("safePanels", safePanelsArr);

    return result;
}

EMSCRIPTEN_BINDINGS(voltorb_solver) {
    function("solveBoard", &solveBoard);
    function("solveBoardWithProgress", &solveBoardWithProgress);
}
