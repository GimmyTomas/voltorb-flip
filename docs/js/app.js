// Main Application Controller for Voltorb Flip
// Manages game state, mode switching, and coordinates between modules

import { BOARD_SIZE, PanelValue, GameResult } from './boardTypes.js';
import { Board } from './board.js';
import { BoardGenerator } from './generator.js';
import { solve } from './solver.js';
import { UI } from './ui.js';

class App {
    constructor() {
        this.ui = new UI();
        this.generator = new BoardGenerator();

        // Game state - default to level 1
        this.board = new Board(1);
        this.solutionBoard = null; // Full solution in self-play mode
        this.history = [];
        this.mode = 'assistant'; // 'assistant' or 'selfplay'

        // Solver results
        this.solverResult = null;
        this.autoSolve = false;

        // Solver settings
        this.solverTimeout = 5000;
        this.useWasm = false;
        this.solverWorker = null;

        // Self-play state
        this.isPlaying = false;
        this.playInterval = null;
        this.speed = 5; // 1-10

        // Statistics
        this.stats = {
            gamesPlayed: 0,
            gamesWon: 0
        };

        this.setupCallbacks();
        this.initializeAssistantMode();
    }

    setupCallbacks() {
        // Tile click
        this.ui.onTileClick = (row, col, currentValue) => {
            if (this.mode === 'assistant') {
                if (currentValue === PanelValue.Unknown) {
                    this.ui.showTileModal(row, col);
                }
            } else {
                // Self-play mode - reveal tile using solution
                if (currentValue === PanelValue.Unknown && this.solutionBoard) {
                    this.revealTile(row, col);
                }
            }
        };

        // Tile value selected (assistant mode)
        this.ui.onTileValueSelect = (row, col, value) => {
            this.saveHistory();
            this.board.set(row, col, value);
            if (this.autoSolve) {
                this.deferSolver();
            } else {
                this.updateDisplay();
            }

            // Check game state
            const result = this.board.checkGameResult();
            if (result === GameResult.Won) {
                this.ui.showGameOverModal(true, 'All multipliers revealed!');
            } else if (result === GameResult.Lost) {
                this.ui.showGameOverModal(false, 'You hit a Voltorb!');
            }
        };

        // Hint click (assistant mode)
        this.ui.onHintClick = (type, index, hint) => {
            if (this.mode === 'assistant') {
                this.ui.showHintModal(type, index, hint);
            }
        };

        // Hint edit
        this.ui.onHintEdit = (type, index, sum, voltorbs) => {
            this.saveHistory();
            if (type === 'row') {
                this.board.setRowHint(index, { sum, voltorbCount: voltorbs });
            } else {
                this.board.setColHint(index, { sum, voltorbCount: voltorbs });
            }
            if (this.autoSolve) {
                this.deferSolver();
            } else {
                this.updateDisplay();
            }
        };

        // Level change
        this.ui.onLevelChange = (level) => {
            this.board.level = level;
            if (this.mode === 'selfplay') {
                this.newGame();
            } else {
                if (this.autoSolve) {
                    this.deferSolver();
                } else {
                    this.updateDisplay();
                }
            }
        };

        // Solve button
        this.ui.onSolve = () => {
            this.runSolver();
        };

        // Auto-solve toggle
        this.ui.onAutoSolveToggle = () => {
            this.autoSolve = !this.autoSolve;
            this.ui.setAutoSolve(this.autoSolve);
            if (this.autoSolve) {
                this.deferSolver();
            }
        };

        // Undo
        this.ui.onUndo = () => {
            this.undo();
        };

        // Reset
        this.ui.onReset = () => {
            if (confirm('Are you sure you want to reset the board?')) {
                this.reset();
            }
        };

        // Mode change
        this.ui.onModeChange = (mode) => {
            this.setMode(mode);
        };

        // New game (works in both modes)
        this.ui.onNewGameAssistant = () => {
            if (this.mode === 'selfplay') {
                this.newGame();
            } else {
                // Assistant mode - generate random board with hints
                const level = parseInt(this.ui.levelSelect.value);
                this.solutionBoard = this.generator.generate(level);
                this.board = this.solutionBoard.toCovered();
                this.board.level = level;

                // Copy hints from solution
                for (let i = 0; i < BOARD_SIZE; i++) {
                    this.board.setRowHint(i, this.solutionBoard.rowHint(i));
                    this.board.setColHint(i, this.solutionBoard.colHint(i));
                }

                this.history = [];
                this.solverResult = null;
                if (this.autoSolve) {
                    this.deferSolver();
                } else {
                    this.updateDisplay();
                }
            }
        };

        // Play/Pause (self-play)
        this.ui.onPlayPause = () => {
            if (this.isPlaying) {
                this.pauseAutoPlay();
            } else {
                this.startAutoPlay();
            }
        };

        // Speed change
        this.ui.onSpeedChange = (speed) => {
            this.speed = speed;
            if (this.isPlaying) {
                this.pauseAutoPlay();
                this.startAutoPlay();
            }
        };

        // Probability display mode change
        this.ui.onProbDisplayModeChange = (mode) => {
            this.updateDisplay();
        };

        // Timeout change
        this.ui.onTimeoutChange = (timeout) => {
            this.solverTimeout = timeout;
        };

        // Solver backend change
        this.ui.onSolverBackendChange = (backend) => {
            this.useWasm = (backend === 'wasm');
            if (this.useWasm) {
                // Preload WASM module
                this.ui.setWasmLoading(true);
                if (this.solverWorker) {
                    this.solverWorker.terminate();
                }
                this.solverWorker = this.createWorker();
                this.solverWorker.postMessage({ type: 'preloadWasm' });
            }
        };
    }

    // Initialize assistant mode with empty board
    initializeAssistantMode() {
        this.board = new Board(parseInt(this.ui.levelSelect.value));

        // Set default hints (all zeros)
        for (let i = 0; i < BOARD_SIZE; i++) {
            this.board.setRowHint(i, { sum: 5, voltorbCount: 0 });
            this.board.setColHint(i, { sum: 5, voltorbCount: 0 });
        }

        this.history = [];
        this.solverResult = null;
        if (this.autoSolve) {
            this.deferSolver();
        } else {
            this.updateDisplay();
        }
    }

    // Set mode
    setMode(mode) {
        this.mode = mode;
        this.ui.setMode(mode);

        if (mode === 'assistant') {
            this.pauseAutoPlay();
            this.initializeAssistantMode();
        } else {
            this.newGame();
        }
    }

    // Generate new game (self-play mode)
    newGame() {
        this.pauseAutoPlay();

        const level = parseInt(this.ui.levelSelect.value);
        this.solutionBoard = this.generator.generate(level);
        this.board = this.solutionBoard.toCovered();
        this.board.level = level;

        // Copy hints from solution
        for (let i = 0; i < BOARD_SIZE; i++) {
            this.board.setRowHint(i, this.solutionBoard.rowHint(i));
            this.board.setColHint(i, this.solutionBoard.colHint(i));
        }

        this.history = [];
        this.solverResult = null;
        if (this.autoSolve) {
            this.deferSolver();
        } else {
            this.updateDisplay();
        }
    }

    // Reveal a tile (self-play mode)
    revealTile(row, col) {
        if (!this.solutionBoard) return;

        const value = this.solutionBoard.get(row, col);
        this.saveHistory();
        this.board.set(row, col, value);

        this.ui.animateTileReveal(row, col, () => {
            if (this.autoSolve) {
                this.deferSolver();
            } else {
                this.updateDisplay();
            }

            // Check game state
            const result = this.board.checkGameResult();
            if (result === GameResult.Won) {
                this.pauseAutoPlay();
                this.stats.gamesPlayed++;
                this.stats.gamesWon++;
                this.ui.updateStats(this.stats.gamesPlayed, this.stats.gamesWon);
                this.ui.showGameOverModal(true, 'All multipliers revealed!');
            } else if (result === GameResult.Lost) {
                this.pauseAutoPlay();
                this.stats.gamesPlayed++;
                this.ui.updateStats(this.stats.gamesPlayed, this.stats.gamesWon);
                this.ui.showGameOverModal(false, 'You hit a Voltorb!');
            }
        });
    }

    // Defer solver execution to next microtask
    deferSolver() {
        this.updateDisplay();
        setTimeout(() => {
            this.runSolver();
        }, 0);
    }

    // Serialize board state to plain objects for Worker transfer
    serializeBoard(board) {
        const panels = [];
        for (let i = 0; i < BOARD_SIZE; i++) {
            panels[i] = [];
            for (let j = 0; j < BOARD_SIZE; j++) {
                panels[i][j] = board.get(i, j);
            }
        }

        const rowHints = [];
        const colHints = [];
        for (let i = 0; i < BOARD_SIZE; i++) {
            rowHints[i] = { ...board.rowHint(i) };
            colHints[i] = { ...board.colHint(i) };
        }

        return { level: board.level, panels, rowHints, colHints };
    }

    // Create a new solver Worker
    createWorker() {
        const worker = new Worker('./js/solver-worker.js', { type: 'module' });

        worker.onmessage = (e) => {
            const { type } = e.data;

            if (type === 'progress') {
                this.solverResult = e.data.result;
                this.updateDisplay();
            } else if (type === 'complete') {
                this.solverResult = e.data.result;
                const r = this.solverResult;
                console.log(`Solver completed in ${r.computeTime.toFixed(1)}ms`);
                console.log(`Compatible boards: ${r.compatibleCount}`);
                console.log(`Safe panels: ${r.safePanels.length}`);
                console.log(`Win probability: ${(r.winProbability * 100).toFixed(1)}%`);
                this.updateDisplay();
            } else if (type === 'error') {
                console.error('Solver error:', e.data.message);
            } else if (type === 'wasmReady') {
                this.ui.setWasmAvailable(true);
            } else if (type === 'wasmError') {
                console.error('WASM load error:', e.data.message);
                this.ui.setWasmAvailable(false);
                this.useWasm = false;
                this.ui.setSolverBackend('js');
            }
        };

        worker.onerror = (err) => {
            console.error('Worker error:', err);
        };

        return worker;
    }

    // Run the solver via Web Worker
    runSolver() {
        // Terminate existing worker (cancels any in-progress solve)
        if (this.solverWorker) {
            this.solverWorker.terminate();
            this.solverWorker = null;
        }

        console.log('Running solver...');

        this.solverWorker = this.createWorker();
        this.solverWorker.postMessage({
            type: 'solve',
            boardData: this.serializeBoard(this.board),
            options: {
                timeout: this.solverTimeout,
                useWasm: this.useWasm
            }
        });
    }

    // Run the solver synchronously (for auto-play)
    runSolverSync() {
        console.log('Running solver (sync)...');
        const startTime = performance.now();

        this.solverResult = solve(this.board);

        console.log(`Solver completed in ${(performance.now() - startTime).toFixed(1)}ms`);
        console.log(`Compatible boards: ${this.solverResult.compatibleCount}`);
        console.log(`Safe panels: ${this.solverResult.safePanels.length}`);
        console.log(`Win probability: ${(this.solverResult.winProbability * 100).toFixed(1)}%`);

        this.updateDisplay();
    }

    // Update the display
    updateDisplay() {
        const probs = this.solverResult?.probabilities;
        const suggested = this.solverResult?.suggestedPanel;
        const safePanels = this.solverResult?.safePanels || [];

        this.ui.renderBoard(this.board, probs, suggested, safePanels);

        // Update win probability and solver info
        if (this.solverResult) {
            this.ui.updateWinProbability(this.solverResult.winProbability, {
                isExact: this.solverResult.isExact,
                depth: this.solverResult.depth
            });

            // Update suggestion
            const isSafe = suggested && safePanels.some(p =>
                p.row === suggested.row && p.col === suggested.col
            );
            this.ui.updateSuggestion(suggested, isSafe);

            // Update solver info (compatible boards and type probabilities)
            this.ui.updateSolverInfo(
                this.solverResult.compatibleCount,
                this.solverResult.probabilities?.typeProbs,
                this.solverResult.capped
            );
        } else {
            this.ui.updateWinProbability(null);
            this.ui.updateSuggestion(null);
            this.ui.updateSolverInfo(null, null);
        }

        // Update undo button
        this.ui.setUndoEnabled(this.history.length > 0);
    }

    // Save current state to history
    saveHistory() {
        this.history.push({
            board: this.board.clone(),
            solverResult: this.solverResult
        });

        // Limit history size
        if (this.history.length > 50) {
            this.history.shift();
        }
    }

    // Undo last action
    undo() {
        if (this.history.length === 0) return;

        const state = this.history.pop();
        this.board = state.board;
        this.solverResult = state.solverResult;
        if (this.autoSolve) {
            this.deferSolver();
        } else {
            this.updateDisplay();
        }
    }

    // Reset board
    reset() {
        this.pauseAutoPlay();

        if (this.mode === 'assistant') {
            this.initializeAssistantMode();
        } else {
            // Reset to covered state but keep same solution
            if (this.solutionBoard) {
                this.board = this.solutionBoard.toCovered();
                this.board.level = this.solutionBoard.level;

                for (let i = 0; i < BOARD_SIZE; i++) {
                    this.board.setRowHint(i, this.solutionBoard.rowHint(i));
                    this.board.setColHint(i, this.solutionBoard.colHint(i));
                }
            }

            this.history = [];
            this.solverResult = null;
            if (this.autoSolve) {
                this.deferSolver();
            } else {
                this.updateDisplay();
            }
        }
    }

    // Start auto-play
    startAutoPlay() {
        if (this.isPlaying) return;
        if (!this.solutionBoard) {
            this.newGame();
        }

        this.isPlaying = true;
        this.ui.setPlaying(true);

        // Calculate interval based on speed (1-10)
        // Speed 1 = 2000ms, Speed 10 = 200ms
        const interval = 2200 - (this.speed * 200);

        this.playInterval = setInterval(() => {
            this.autoPlayStep();
        }, interval);
    }

    // Pause auto-play
    pauseAutoPlay() {
        if (!this.isPlaying) return;

        this.isPlaying = false;
        this.ui.setPlaying(false);

        if (this.playInterval) {
            clearInterval(this.playInterval);
            this.playInterval = null;
        }
    }

    // Single auto-play step
    autoPlayStep() {
        // Check if game is over
        const result = this.board.checkGameResult();
        if (result !== GameResult.InProgress) {
            this.pauseAutoPlay();
            return;
        }

        // Run solver synchronously to find best move
        this.runSolverSync();

        if (this.solverResult?.suggestedPanel) {
            const { row, col } = this.solverResult.suggestedPanel;
            this.revealTile(row, col);
        } else {
            // No suggested panel - pick first unknown
            const unknowns = this.board.getUnknownPositions();
            if (unknowns.length > 0) {
                this.revealTile(unknowns[0].row, unknowns[0].col);
            } else {
                this.pauseAutoPlay();
            }
        }
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.app = new App();
});
