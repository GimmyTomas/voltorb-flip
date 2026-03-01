// Web Worker for Voltorb Flip Solver
// Runs the solver off the main thread to keep UI responsive

import { BOARD_SIZE, PanelValue } from './boardTypes.js';
import { Board } from './board.js';
import {
    generateCompatibleBoards,
    calculateProbabilities,
    findSafePanels,
    iterativeDeepening
} from './solver.js';

/**
 * Deserialize plain board data into a Board instance.
 */
function deserializeBoard(boardData) {
    const board = new Board(boardData.level);
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            board.set(i, j, boardData.panels[i][j]);
        }
        board.setRowHint(i, boardData.rowHints[i]);
        board.setColHint(i, boardData.colHints[i]);
    }
    return board;
}

/**
 * Build a result object from iterative deepening progress.
 */
function buildResult(progress, probabilities, safePanels, compatibleCount, capped, startTime) {
    let suggestedPanel = progress.bestPanel;

    if (safePanels.length > 0 && !suggestedPanel) {
        let bestSafe = safePanels[0];
        let bestScore = -1;
        for (const pos of safePanels) {
            const panelProb = probabilities.panels.find(p =>
                p.pos.row === pos.row && p.pos.col === pos.col
            );
            if (panelProb) {
                const score = panelProb.pOne * 1 + panelProb.pTwo * 2 + panelProb.pThree * 3;
                if (score > bestScore) {
                    bestScore = score;
                    bestSafe = pos;
                }
            }
        }
        suggestedPanel = bestSafe;
    }

    return {
        suggestedPanel,
        winProbability: progress.winProbability,
        probabilities,
        safePanels,
        compatibleCount,
        capped,
        computeTime: performance.now() - startTime,
        depth: progress.depth,
        isExact: progress.isExact,
        reason: progress.reason
    };
}

/**
 * Run the JS solver synchronously inside the Worker.
 */
function solveJS(boardData, options) {
    const startTime = performance.now();
    const { timeout = 5000, maxBoards = 500000 } = options;

    const board = deserializeBoard(boardData);

    // Generate compatible boards
    const compatibleBoards = generateCompatibleBoards(board, maxBoards);
    const capped = compatibleBoards.length >= maxBoards;

    if (compatibleBoards.length === 0) {
        const result = {
            suggestedPanel: null,
            winProbability: 0,
            probabilities: { panels: [], typeProbs: [], totalCompatible: 0 },
            safePanels: [],
            compatibleCount: 0,
            capped: false,
            computeTime: performance.now() - startTime,
            depth: 0,
            isExact: true,
            reason: 'No compatible boards'
        };
        self.postMessage({ type: 'complete', result });
        return;
    }

    // Calculate probabilities and safe panels
    const probabilities = calculateProbabilities(board, compatibleBoards);
    const safePanels = findSafePanels(board, compatibleBoards);

    // Run iterative deepening, posting progress after each depth
    let lastResult = null;
    for (const progress of iterativeDeepening(board, compatibleBoards, { timeout })) {
        lastResult = buildResult(progress, probabilities, safePanels,
            compatibleBoards.length, capped, startTime);
        self.postMessage({ type: 'progress', result: lastResult });
    }

    if (lastResult) {
        self.postMessage({ type: 'complete', result: lastResult });
    }
}

// WASM module state
let wasmModule = null;
let wasmLoading = false;

/**
 * Load the WASM solver module lazily.
 */
async function loadWasm() {
    if (wasmModule) return wasmModule;
    if (wasmLoading) {
        // Wait for in-progress load
        while (wasmLoading) {
            await new Promise(r => setTimeout(r, 50));
        }
        return wasmModule;
    }

    wasmLoading = true;
    try {
        const { default: VoltorbSolverModule } = await import('./solver-wasm.js');
        wasmModule = await VoltorbSolverModule();
        wasmLoading = false;
        return wasmModule;
    } catch (err) {
        wasmLoading = false;
        throw err;
    }
}

/**
 * Run the WASM solver.
 */
async function solveWASM(boardData, options) {
    const startTime = performance.now();
    const { timeout = 5000, maxBoards = 500000 } = options;

    try {
        const module = await loadWasm();

        // Convert board data to flat arrays for WASM
        const panels = [];
        for (let i = 0; i < BOARD_SIZE; i++) {
            for (let j = 0; j < BOARD_SIZE; j++) {
                panels.push(boardData.panels[i][j]);
            }
        }

        const rowSums = boardData.rowHints.map(h => h.sum);
        const rowVoltorbs = boardData.rowHints.map(h => h.voltorbCount);
        const colSums = boardData.colHints.map(h => h.sum);
        const colVoltorbs = boardData.colHints.map(h => h.voltorbCount);

        const wasmResult = module.solveBoard(
            boardData.level, panels,
            rowSums, rowVoltorbs,
            colSums, colVoltorbs,
            timeout, maxBoards
        );

        // Convert WASM result to match JS format
        const result = {
            suggestedPanel: wasmResult.suggestedPanel,
            winProbability: wasmResult.winProbability,
            probabilities: wasmResult.probabilities,
            safePanels: wasmResult.safePanels,
            compatibleCount: wasmResult.compatibleCount,
            capped: wasmResult.capped,
            computeTime: performance.now() - startTime,
            depth: wasmResult.depth,
            isExact: wasmResult.isExact,
            reason: wasmResult.reason
        };

        self.postMessage({ type: 'complete', result });
    } catch (err) {
        self.postMessage({ type: 'error', message: err.message || 'WASM solver failed' });
    }
}

// Message handler
self.onmessage = async function(e) {
    const { type } = e.data;

    if (type === 'solve') {
        const { boardData, options = {} } = e.data;

        try {
            if (options.useWasm) {
                await solveWASM(boardData, options);
            } else {
                solveJS(boardData, options);
            }
        } catch (err) {
            self.postMessage({ type: 'error', message: err.message || 'Solver failed' });
        }
    } else if (type === 'preloadWasm') {
        try {
            await loadWasm();
            self.postMessage({ type: 'wasmReady' });
        } catch (err) {
            self.postMessage({ type: 'wasmError', message: err.message || 'Failed to load WASM module' });
        }
    }
};
