#include "voltorb/solver.hpp"

#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/probability.hpp"

#include <algorithm>
#include <cmath>

namespace voltorb {

Solver::Solver(SolverOptions options)
    : options_(std::move(options)) {
    // Ensure Zobrist tables are initialized
    if (!ZobristHasher::isInitialized()) {
        ZobristHasher::initialize();
    }
}

void Solver::reset() {
    memo_.clear();
    nodesEvaluated_ = 0;
    cacheHits_ = 0;
    cacheMisses_ = 0;
    allBoards_.clear();
}

bool Solver::checkTimeout() {
    if (timedOut_) return true;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);

    if (elapsed >= options_.timeout) {
        timedOut_ = true;
        return true;
    }

    return false;
}

SolverResult Solver::solve(const Board& board, ProgressCallback onProgress) {
    startTime_ = std::chrono::steady_clock::now();
    timedOut_ = false;
    nodesEvaluated_ = 0;
    cacheHits_ = 0;
    cacheMisses_ = 0;

    // Check if game is already over
    GameResult gameState = board.checkGameResult();
    if (gameState == GameResult::Won) {
        return {
            {0, 0}, 1.0, true, 0,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime_),
            0, "Game already won"};
    }
    if (gameState == GameResult::Lost) {
        return {
            {0, 0}, 0.0, true, 0,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime_),
            0, "Game already lost"};
    }

    // Initialize search state
    auto initialState = initializeSearch(board);
    if (!initialState) {
        return {
            {0, 0}, 0.0, true, 0,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime_),
            0, "No compatible boards found"};
    }

    // Run iterative deepening search
    return iterativeDeepening(*initialState, onProgress);
}

std::optional<SearchState> Solver::initializeSearch(const Board& board) {
    // Generate all compatible boards
    allBoards_.clear();
    size_t boardCount = 0;

    CompatibleBoardGenerator::generateAllTypes(
        board, [&](const Board& b) {
            allBoards_.push_back(b);
            boardCount++;
            return boardCount <= options_.maxCompatibleBoards;
        });

    if (allBoards_.empty()) {
        return std::nullopt;
    }

    // Create initial search state
    SearchState state;
    state.board = board;
    state.zobristHash = ZobristHasher::hashPanels(board.panels());

    // Group boards by type
    Level level = board.level();
    for (size_t idx = 0; idx < allBoards_.size(); idx++) {
        const Board& cb = allBoards_[idx];

        int count0 = 0, count2 = 0, count3 = 0;
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                PanelValue v = cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                if (v == PanelValue::Voltorb) count0++;
                else if (v == PanelValue::Two) count2++;
                else if (v == PanelValue::Three) count3++;
            }
        }

        for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
            const auto& params = BoardTypeData::params(level, type);
            if (params.n0 == count0 && params.n2 == count2 && params.n3 == count3) {
                if (LegalityChecker::isLegal(cb, params)) {
                    state.indicesPerType[type].push_back(idx);
                }
            }
        }
    }

    state.pBoardNorm = calculateProbNorm(state);

    return state;
}

SolverResult Solver::iterativeDeepening(const SearchState& initialState,
                                        ProgressCallback onProgress) {
    Position bestPanel{0, 0};
    double bestWinProb = 0.0;
    bool isExact = false;
    int lastDepth = 0;

    // Check for free panel first (guaranteed safe)
    auto freePanel = findFreePanel(initialState);

    for (int depth = 1; depth <= options_.maxDepth && !timedOut_; depth++) {
        memo_.clear();  // Fresh memo for each depth, matching JS solver behavior
        auto result = depthLimitedSearch(initialState, depth);

        bestPanel = result.bestPanel;
        bestWinProb = result.winProbability;
        lastDepth = depth;
        isExact = result.fullyExplored;

        // Report progress
        if (onProgress) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime_);

            SolverProgress progress{
                bestPanel,
                bestWinProb,
                depth,
                isExact,
                nodesEvaluated_,
                elapsed};

            onProgress(progress);
        }

        // If fully explored, we have the exact answer
        if (isExact) {
            break;
        }

        // Check timeout before starting next depth
        checkTimeout();
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime_);

    // If we have a free panel, suggest it (same win probability, zero risk)
    if (freePanel) {
        return {
            *freePanel,
            bestWinProb,
            isExact,
            initialState.totalCompatible(),
            elapsed,
            lastDepth,
            "Free panel (guaranteed safe)"};
    }

    std::optional<std::string> reason;
    if (isExact) {
        reason = "Exact solution";
    } else if (timedOut_) {
        reason = "Timeout at depth " + std::to_string(lastDepth);
    } else {
        reason = "Depth " + std::to_string(lastDepth);
    }

    return {
        bestPanel,
        bestWinProb,
        isExact,
        initialState.totalCompatible(),
        elapsed,
        lastDepth,
        reason};
}

DepthLimitedResult Solver::depthLimitedSearch(const SearchState& state, int depthLimit) {
    nodesEvaluated_++;

    // Check for win condition
    if (isWon(state)) {
        return {{0, 0}, 1.0, true};
    }

    // At depth 0, use heuristic evaluation
    if (depthLimit <= 0) {
        double h = heuristicEval(state);
        return {{0, 0}, h, false};
    }

    // Check memo table
    auto it = memo_.find(state.zobristHash);
    if (it != memo_.end() && it->second.depth >= depthLimit) {
        cacheHits_++;
        return {it->second.bestPanel, it->second.winProbability,
                it->second.fullyExplored};
    }
    cacheMisses_++;

    // Check timeout
    if (checkTimeout()) {
        return {{0, 0}, heuristicEval(state), false};
    }

    // Check for free panel
    auto freePanel = findFreePanel(state);
    if (freePanel) {
        // Recurse with the free panel revealed
        double winProb = 0.0;
        bool fullyExplored = true;

        for (int value = 1; value <= 3; value++) {
            double pValue = probabilityOf(state, *freePanel, static_cast<PanelValue>(value));
            if (pValue <= 0) continue;

            SearchState nextState = revealPanel(state, *freePanel, static_cast<PanelValue>(value));
            auto childResult = depthLimitedSearch(nextState, depthLimit);

            winProb += pValue * childResult.winProbability;
            if (!childResult.fullyExplored) fullyExplored = false;
        }

        // Store in memo table
        auto [ins_it, inserted] = memo_.try_emplace(state.zobristHash,
            MemoEntry{winProb, *freePanel, depthLimit, fullyExplored});
        if (!inserted && depthLimit > ins_it->second.depth) {
            ins_it->second = {winProb, *freePanel, depthLimit, fullyExplored};
        }

        return {*freePanel, winProb, fullyExplored};
    }

    // Get unknown panels in heuristic order
    std::vector<Position> unknownPanels = getOrderedUnknownPanels(state);

    if (unknownPanels.empty()) {
        // No unknown panels but not won - shouldn't happen, but handle gracefully
        return {{0, 0}, 0.0, true};
    }

    Position bestPanel = unknownPanels[0];
    double bestWinProb = 0.0;
    bool allFullyExplored = true;

    for (const Position& pos : unknownPanels) {
        // Upper-bound pruning
        double upperBound = getUpperBound(state, pos);
        if (upperBound <= bestWinProb) {
            continue;  // Can't improve on current best
        }

        // Calculate expected win probability for this panel
        double panelWinProb = 0.0;
        bool panelFullyExplored = true;

        for (int value = 1; value <= 3; value++) {
            double pValue = probabilityOf(state, pos, static_cast<PanelValue>(value));
            if (pValue <= 0) continue;

            SearchState nextState = revealPanel(state, pos, static_cast<PanelValue>(value));
            auto childResult = depthLimitedSearch(nextState, depthLimit - 1);

            panelWinProb += pValue * childResult.winProbability;
            if (!childResult.fullyExplored) panelFullyExplored = false;
        }

        if (!panelFullyExplored) allFullyExplored = false;

        if (panelWinProb > bestWinProb) {
            bestWinProb = panelWinProb;
            bestPanel = pos;
        }

        // Check timeout periodically
        if (checkTimeout()) {
            allFullyExplored = false;
            break;
        }
    }

    // Store in memo table
    auto [ins_it, inserted] = memo_.try_emplace(state.zobristHash,
        MemoEntry{bestWinProb, bestPanel, depthLimit, allFullyExplored});
    if (!inserted && depthLimit > ins_it->second.depth) {
        ins_it->second = {bestWinProb, bestPanel, depthLimit, allFullyExplored};
    }

    return {bestPanel, bestWinProb, allFullyExplored};
}

double Solver::heuristicEval(const SearchState& state) {
    // If won, return 1.0
    if (isWon(state)) {
        return 1.0;
    }

    // Count remaining multipliers needed
    size_t multipliersNeeded = 0;

    // Check each unknown panel to see if it could be a multiplier
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) == PanelValue::Unknown) {
                // Check if any compatible board has a multiplier here
                Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
                for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
                    for (size_t idx : state.indicesPerType[type]) {
                        PanelValue v = allBoards_[idx].get(pos);
                        if (isMultiplier(v)) {
                            multipliersNeeded++;
                            goto next_panel;  // Count each position only once
                        }
                    }
                }
            }
            next_panel:;
        }
    }

    if (multipliersNeeded == 0) {
        return 1.0;  // All multipliers already revealed
    }

    // Collect voltorb probabilities for risky panels
    std::vector<double> voltorbProbs;

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            if (state.board.get(pos) != PanelValue::Unknown) continue;
            if (!hasMultiplierPotential(state, pos)) continue;

            double pVoltorb = probabilityOf(state, pos, PanelValue::Voltorb);
            if (pVoltorb > 0) {
                voltorbProbs.push_back(pVoltorb);
            }
        }
    }

    if (voltorbProbs.empty()) {
        return 1.0;  // No risky panels - guaranteed win
    }

    // Sort by risk (lowest first - these are the panels we'd reveal first)
    std::sort(voltorbProbs.begin(), voltorbProbs.end());

    // Estimate: product of survival probabilities for the safest panels
    // we'd need to reveal to get all multipliers
    size_t panelsToReveal = std::min(voltorbProbs.size(), multipliersNeeded);
    double survivalProd = 1.0;

    for (size_t i = 0; i < panelsToReveal; i++) {
        survivalProd *= (1.0 - voltorbProbs[i]);
    }

    return survivalProd;
}

bool Solver::isWon(const SearchState& state) const {
    // Check if all multipliers have been revealed
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) == PanelValue::Unknown) {
                Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
                // Check if any compatible board has a multiplier here
                for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
                    for (size_t idx : state.indicesPerType[type]) {
                        if (isMultiplier(allBoards_[idx].get(pos))) {
                            return false;  // There's still a multiplier to reveal
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool Solver::hasMultiplierPotential(const SearchState& state, Position pos) const {
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        for (size_t idx : state.indicesPerType[type]) {
            if (isMultiplier(allBoards_[idx].get(pos))) {
                return true;
            }
        }
    }
    return false;
}

std::optional<Position> Solver::findFreePanel(const SearchState& state) const {
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};

            if (state.board.get(pos) != PanelValue::Unknown) {
                continue;
            }

            bool isFree = true;

            for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL && isFree; type++) {
                for (size_t idx : state.indicesPerType[type]) {
                    if (allBoards_[idx].get(pos) == PanelValue::Voltorb) {
                        isFree = false;
                        break;
                    }
                }
            }

            if (isFree && hasMultiplierPotential(state, pos)) {
                return pos;
            }
        }
    }

    return std::nullopt;
}

std::vector<Position> Solver::getOrderedUnknownPanels(const SearchState& state) const {
    std::vector<std::pair<Position, double>> panelsWithProb;

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            if (state.board.get(pos) == PanelValue::Unknown) {
                if (!hasMultiplierPotential(state, pos)) continue;
                double pVoltorb = probabilityOf(state, pos, PanelValue::Voltorb);
                panelsWithProb.emplace_back(pos, pVoltorb);
            }
        }
    }

    // Sort by voltorb probability (lowest first - safest panels)
    std::sort(panelsWithProb.begin(), panelsWithProb.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });

    std::vector<Position> result;
    result.reserve(panelsWithProb.size());
    for (const auto& [pos, _] : panelsWithProb) {
        result.push_back(pos);
    }

    return result;
}

double Solver::calculateProbNorm(const SearchState& state) const {
    Level level = state.board.level();
    double pBoard = 0.0;

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        int64_t nAccepted = BoardTypeData::acceptedCount(level, type);
        if (nAccepted > 0) {
            pBoard += static_cast<double>(state.indicesPerType[type].size()) /
                      static_cast<double>(nAccepted);
        }
    }

    return pBoard;
}

SearchState Solver::revealPanel(const SearchState& state, Position pos, PanelValue value) const {
    SearchState newState;
    newState.board = state.board.withPanelRevealed(pos, value);

    // Update Zobrist hash incrementally
    newState.zobristHash = ZobristHasher::updateHash(
        state.zobristHash, pos, PanelValue::Unknown, value);

    // Filter compatible boards
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        for (size_t idx : state.indicesPerType[type]) {
            if (allBoards_[idx].get(pos) == value) {
                newState.indicesPerType[type].push_back(idx);
            }
        }
    }

    newState.pBoardNorm = calculateProbNorm(newState);

    return newState;
}

double Solver::probabilityOf(const SearchState& state, Position pos, PanelValue value) const {
    if (state.pBoardNorm <= 0) return 0.0;

    Level level = state.board.level();
    double pValue = 0.0;

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        if (state.indicesPerType[type].empty()) continue;

        int64_t nAccepted = BoardTypeData::acceptedCount(level, type);
        if (nAccepted <= 0) continue;

        // Count boards of this type where pos has this value
        size_t countWithValue = 0;
        for (size_t idx : state.indicesPerType[type]) {
            if (allBoards_[idx].get(pos) == value) {
                countWithValue++;
            }
        }

        // P(value at pos | type) * P(type | board)
        double pValueGivenType = static_cast<double>(countWithValue) /
                                 static_cast<double>(state.indicesPerType[type].size());

        double pTypeWeight = static_cast<double>(state.indicesPerType[type].size()) /
                             static_cast<double>(nAccepted);

        pValue += pValueGivenType * pTypeWeight / state.pBoardNorm;
    }

    return pValue;
}

double Solver::getUpperBound(const SearchState& state, Position pos) const {
    // Upper bound on win probability = P(not voltorb at this position)
    // This is the maximum possible win probability if we choose this panel
    double pVoltorb = probabilityOf(state, pos, PanelValue::Voltorb);
    return 1.0 - pVoltorb;
}

std::vector<Position> Solver::findSafePanels(const Board& board) {
    std::vector<Board> compatibleBoards;

    CompatibleBoardGenerator::generateAllTypes(board, [&](const Board& b) {
        compatibleBoards.push_back(b);
        return compatibleBoards.size() < options_.maxCompatibleBoards;
    });

    return ProbabilityCalculator::findGuaranteedSafePanels(board, compatibleBoards);
}

std::vector<std::pair<Position, double>> Solver::getPanelWinProbabilities(const Board& board) {
    std::vector<std::pair<Position, double>> result;

    // Initialize search state
    auto state = initializeSearch(board);
    if (!state) return result;

    // For each unknown panel, calculate the expected win probability
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            if (board.get(pos) != PanelValue::Unknown) continue;

            double pSafe = 1.0 - probabilityOf(*state, pos, PanelValue::Voltorb);
            result.emplace_back(pos, pSafe);
        }
    }

    return result;
}

} // namespace voltorb
