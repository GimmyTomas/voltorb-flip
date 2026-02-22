// Board Generator for Voltorb Flip
// Ported from C++ src/generator.cpp

import {
    BOARD_SIZE,
    TOTAL_PANELS,
    NUM_TYPES_PER_LEVEL,
    PanelValue,
    getParams,
    isMultiplier
} from './boardTypes.js';

import { Board, indexToPos } from './board.js';

// Simple seeded random number generator (Mulberry32)
function mulberry32(seed) {
    return function() {
        let t = seed += 0x6D2B79F5;
        t = Math.imul(t ^ t >>> 15, t | 1);
        t ^= t + Math.imul(t ^ t >>> 7, t | 61);
        return ((t ^ t >>> 14) >>> 0) / 4294967296;
    };
}

export class BoardGenerator {
    constructor(seed = null) {
        this.seed_ = seed || Date.now();
        this.random = mulberry32(this.seed_);
        this.lastRejectionCount_ = 0;
    }

    // Seed the RNG
    seed(seed) {
        this.seed_ = seed;
        this.random = mulberry32(seed);
    }

    // Shuffle array using Fisher-Yates
    shuffle(array) {
        const arr = [...array];
        for (let i = arr.length - 1; i > 0; i--) {
            const j = Math.floor(this.random() * (i + 1));
            [arr[i], arr[j]] = [arr[j], arr[i]];
        }
        return arr;
    }

    // Check if a board configuration is legal
    isLegal(board, params) {
        const rowHints = board.rowHints;
        const colHints = board.colHints;

        // Count multipliers in free rows/columns
        let totalFreeMultipliers = 0;

        for (let i = 0; i < BOARD_SIZE; i++) {
            for (let j = 0; j < BOARD_SIZE; j++) {
                const v = board.get(i, j);
                if (isMultiplier(v)) {
                    if (rowHints[i].voltorbCount === 0 || colHints[j].voltorbCount === 0) {
                        totalFreeMultipliers++;
                    }
                }
            }
        }

        if (totalFreeMultipliers >= params.maxTotalFree) {
            return false;
        }

        // Check per-row constraints
        for (let i = 0; i < BOARD_SIZE; i++) {
            if (rowHints[i].voltorbCount === 0) {
                let rowMultipliers = 0;
                for (let j = 0; j < BOARD_SIZE; j++) {
                    if (isMultiplier(board.get(i, j))) {
                        rowMultipliers++;
                    }
                }
                if (rowMultipliers >= params.maxRowFree) {
                    return false;
                }
            }
        }

        // Check per-column constraints
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (colHints[j].voltorbCount === 0) {
                let colMultipliers = 0;
                for (let i = 0; i < BOARD_SIZE; i++) {
                    if (isMultiplier(board.get(i, j))) {
                        colMultipliers++;
                    }
                }
                if (colMultipliers >= params.maxRowFree) {
                    return false;
                }
            }
        }

        return true;
    }

    // Generate a random board for a given level
    generate(level) {
        const type = Math.floor(this.random() * NUM_TYPES_PER_LEVEL);
        return this.generateWithType(level, type);
    }

    // Generate a random board for a given level and type
    generateWithType(level, type) {
        const params = getParams(level, type);
        this.lastRejectionCount_ = 0;

        while (true) {
            const board = new Board(level);

            // Initialize all panels to 1
            for (let i = 0; i < BOARD_SIZE; i++) {
                for (let j = 0; j < BOARD_SIZE; j++) {
                    board.set(i, j, PanelValue.One);
                }
            }

            // Create shuffled position array
            const positions = [];
            for (let i = 0; i < TOTAL_PANELS; i++) {
                positions.push(i);
            }
            const shuffled = this.shuffle(positions);

            let idx = 0;

            // Place voltorbs
            for (let n = 0; n < params.n0; n++) {
                const pos = indexToPos(shuffled[idx++]);
                board.set(pos.row, pos.col, PanelValue.Voltorb);
            }

            // Place 2s
            for (let n = 0; n < params.n2; n++) {
                const pos = indexToPos(shuffled[idx++]);
                board.set(pos.row, pos.col, PanelValue.Two);
            }

            // Place 3s
            for (let n = 0; n < params.n3; n++) {
                const pos = indexToPos(shuffled[idx++]);
                board.set(pos.row, pos.col, PanelValue.Three);
            }

            // Calculate hints
            board.recalculateHints();

            // Check legality
            if (this.isLegal(board, params)) {
                return board;
            }

            this.lastRejectionCount_++;

            // Safety limit to prevent infinite loop
            if (this.lastRejectionCount_ > 10000) {
                console.warn('Board generation: too many rejections, returning invalid board');
                return board;
            }
        }
    }

    // Get the number of rejections in the last generation
    get lastRejectionCount() {
        return this.lastRejectionCount_;
    }
}

// Create a board from explicit panel values (for testing)
export function createBoardFromValues(level, values, rowHints = null, colHints = null) {
    const board = new Board(level);

    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            board.set(i, j, values[i][j]);
        }
    }

    if (rowHints && colHints) {
        for (let i = 0; i < BOARD_SIZE; i++) {
            board.setRowHint(i, rowHints[i]);
            board.setColHint(i, colHints[i]);
        }
    } else {
        board.recalculateHints();
    }

    return board;
}

// Create an empty board with hints (for assistant mode)
export function createEmptyBoardWithHints(level, rowHints, colHints) {
    const board = new Board(level);

    for (let i = 0; i < BOARD_SIZE; i++) {
        board.setRowHint(i, { sum: rowHints[i].sum, voltorbCount: rowHints[i].voltorbCount });
        board.setColHint(i, { sum: colHints[i].sum, voltorbCount: colHints[i].voltorbCount });
    }

    return board;
}
