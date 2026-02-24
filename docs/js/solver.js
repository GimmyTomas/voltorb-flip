// Solver Logic for Voltorb Flip
// Implements iterative deepening with memoization for optimal play

import {
    BOARD_SIZE,
    NUM_TYPES_PER_LEVEL,
    PanelValue,
    getParams,
    getAcceptedCount,
    getN1,
    getTotalSum,
    isMultiplier,
    isKnown,
    isCompatibleWithType
} from './boardTypes.js';

import { Board } from './board.js';

// Check if a board configuration is legal according to type constraints
export function isLegal(board, params) {
    const rowHints = board.rowHints;
    const colHints = board.colHints;

    // Count multipliers in "free" rows/columns (those with 0 voltorbs)
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

    // Check per-row constraints for free rows
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

    // Check per-column constraints for free columns
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

// Check if revealed panels don't exceed hint constraints
export function panelsDontExceedConstraints(board) {
    const rowHints = board.rowHints;
    const colHints = board.colHints;

    // Check row constraints
    for (let i = 0; i < BOARD_SIZE; i++) {
        let sum = 0;
        let voltorbs = 0;
        let unknownCount = 0;

        for (let j = 0; j < BOARD_SIZE; j++) {
            const v = board.get(i, j);
            if (v === PanelValue.Unknown) {
                unknownCount++;
            } else if (v === PanelValue.Voltorb) {
                voltorbs++;
            } else {
                sum += v;
            }
        }

        if (sum > rowHints[i].sum) return false;
        if (voltorbs > rowHints[i].voltorbCount) return false;

        if (unknownCount === 0) {
            if (sum !== rowHints[i].sum) return false;
            if (voltorbs !== rowHints[i].voltorbCount) return false;
        } else {
            const remainingSum = rowHints[i].sum - sum;
            const remainingVoltorbs = rowHints[i].voltorbCount - voltorbs;
            const remainingNonVoltorbs = unknownCount - remainingVoltorbs;

            if (remainingNonVoltorbs < 0) return false;
            if (remainingSum < remainingNonVoltorbs) return false;
            if (remainingSum > remainingNonVoltorbs * 3) return false;
        }
    }

    // Check column constraints
    for (let j = 0; j < BOARD_SIZE; j++) {
        let sum = 0;
        let voltorbs = 0;
        let unknownCount = 0;

        for (let i = 0; i < BOARD_SIZE; i++) {
            const v = board.get(i, j);
            if (v === PanelValue.Unknown) {
                unknownCount++;
            } else if (v === PanelValue.Voltorb) {
                voltorbs++;
            } else {
                sum += v;
            }
        }

        if (sum > colHints[j].sum) return false;
        if (voltorbs > colHints[j].voltorbCount) return false;

        if (unknownCount === 0) {
            if (sum !== colHints[j].sum) return false;
            if (voltorbs !== colHints[j].voltorbCount) return false;
        } else {
            const remainingSum = colHints[j].sum - sum;
            const remainingVoltorbs = colHints[j].voltorbCount - voltorbs;
            const remainingNonVoltorbs = unknownCount - remainingVoltorbs;

            if (remainingNonVoltorbs < 0) return false;
            if (remainingSum < remainingNonVoltorbs) return false;
            if (remainingSum > remainingNonVoltorbs * 3) return false;
        }
    }

    return true;
}

// Generate all combinations of k items from n positions
function* combinations(positions, k) {
    if (k === 0) {
        yield [];
        return;
    }
    if (positions.length < k) return;

    for (let i = 0; i <= positions.length - k; i++) {
        for (const rest of combinations(positions.slice(i + 1), k - 1)) {
            yield [positions[i], ...rest];
        }
    }
}

// Generate all valid voltorb position configurations
function* generateVoltorbPositions(board) {
    const rowHints = board.rowHints;
    const colHints = board.colHints;

    function* generateRow(row, colCounts, current) {
        if (row === BOARD_SIZE) {
            for (let j = 0; j < BOARD_SIZE; j++) {
                if (colCounts[j] !== colHints[j].voltorbCount) return;
            }

            for (let i = 0; i < BOARD_SIZE; i++) {
                for (let j = 0; j < BOARD_SIZE; j++) {
                    const revealed = board.get(i, j);
                    if (isKnown(revealed)) {
                        const shouldBeVoltorb = current[i][j];
                        const isVoltorb = (revealed === PanelValue.Voltorb);
                        if (shouldBeVoltorb !== isVoltorb) return;
                    }
                }
            }

            yield current.map(row => [...row]);
            return;
        }

        const voltorbsNeeded = rowHints[row].voltorbCount;
        const rowPositions = [0, 1, 2, 3, 4];

        for (const combo of combinations(rowPositions, voltorbsNeeded)) {
            const newColCounts = [...colCounts];
            const newCurrent = current.map(r => [...r]);
            newCurrent[row] = [false, false, false, false, false];

            for (const j of combo) {
                newCurrent[row][j] = true;
                newColCounts[j]++;
            }

            let valid = true;
            for (let j = 0; j < BOARD_SIZE; j++) {
                if (newColCounts[j] > colHints[j].voltorbCount) {
                    valid = false;
                    break;
                }
                const remainingRows = BOARD_SIZE - 1 - row;
                if (newColCounts[j] + remainingRows < colHints[j].voltorbCount) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                yield* generateRow(row + 1, newColCounts, newCurrent);
            }
        }
    }

    const initial = [];
    for (let i = 0; i < BOARD_SIZE; i++) {
        initial[i] = [false, false, false, false, false];
    }

    yield* generateRow(0, [0, 0, 0, 0, 0], initial);
}

// Fill non-voltorb positions with 1s, 2s, 3s according to type
function* fillNonVoltorbs(board, voltorbPositions, type, maxBoards = 10000) {
    const level = board.level;
    const params = getParams(level, type);

    const template = new Board(level);
    for (let i = 0; i < BOARD_SIZE; i++) {
        template.setRowHint(i, board.rowHint(i));
        template.setColHint(i, board.colHint(i));
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (voltorbPositions[i][j]) {
                template.set(i, j, PanelValue.Voltorb);
            } else {
                template.set(i, j, PanelValue.Unknown);
            }
        }
    }

    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            const revealed = board.get(i, j);
            if (isKnown(revealed) && revealed !== PanelValue.Voltorb) {
                template.set(i, j, revealed);
            }
        }
    }

    let placed2s = 0, placed3s = 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            const v = template.get(i, j);
            if (v === PanelValue.Two) placed2s++;
            if (v === PanelValue.Three) placed3s++;
        }
    }

    const remaining2s = params.n2 - placed2s;
    const remaining3s = params.n3 - placed3s;

    if (remaining2s < 0 || remaining3s < 0) return;

    const unknownPositions = [];
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (template.get(i, j) === PanelValue.Unknown) {
                unknownPositions.push({ row: i, col: j });
            }
        }
    }

    let count = 0;

    function* fill(posIdx, board, rem2s, rem3s) {
        if (count >= maxBoards) return;

        if (posIdx >= unknownPositions.length) {
            if (panelsDontExceedConstraints(board) && isLegal(board, params)) {
                count++;
                yield board.clone();
            }
            return;
        }

        const pos = unknownPositions[posIdx];
        const remainingPositions = unknownPositions.length - posIdx - 1;

        if (rem2s + rem3s > remainingPositions + 1) return;

        board.set(pos.row, pos.col, PanelValue.One);
        if (panelsDontExceedConstraints(board)) {
            yield* fill(posIdx + 1, board, rem2s, rem3s);
        }

        if (rem2s > 0) {
            board.set(pos.row, pos.col, PanelValue.Two);
            if (panelsDontExceedConstraints(board)) {
                yield* fill(posIdx + 1, board, rem2s - 1, rem3s);
            }
        }

        if (rem3s > 0) {
            board.set(pos.row, pos.col, PanelValue.Three);
            if (panelsDontExceedConstraints(board)) {
                yield* fill(posIdx + 1, board, rem2s, rem3s - 1);
            }
        }

        board.set(pos.row, pos.col, PanelValue.Unknown);
    }

    yield* fill(0, template, remaining2s, remaining3s);
}

// Generate compatible boards for a given board state
export function generateCompatibleBoards(board, maxBoards = 10000) {
    const level = board.level;
    const compatibleBoards = [];

    if (!panelsDontExceedConstraints(board)) {
        return compatibleBoards;
    }

    const { totalVoltorbs, totalSum } = board.getTotals();

    const compatibleTypes = [];
    for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        if (isCompatibleWithType(totalVoltorbs, totalSum, level, type)) {
            compatibleTypes.push(type);
        }
    }

    if (compatibleTypes.length === 0) return compatibleBoards;

    const voltorbConfigs = [...generateVoltorbPositions(board)];

    for (const type of compatibleTypes) {
        if (compatibleBoards.length >= maxBoards) break;

        for (const voltorbs of voltorbConfigs) {
            if (compatibleBoards.length >= maxBoards) break;

            for (const filled of fillNonVoltorbs(board, voltorbs, type, maxBoards - compatibleBoards.length)) {
                filled.generatedType = type;
                compatibleBoards.push(filled);
                if (compatibleBoards.length >= maxBoards) break;
            }
        }
    }

    return compatibleBoards;
}

// Group boards by type
export function groupBoardsByType(boards, level) {
    const groups = [];
    for (let i = 0; i < NUM_TYPES_PER_LEVEL; i++) {
        groups[i] = [];
    }

    for (const board of boards) {
        if (board.generatedType !== undefined) {
            groups[board.generatedType].push(board);
        }
    }

    return groups;
}

// Calculate type probabilities using Bayesian inference
export function calculateTypeProbabilities(level, countsPerType) {
    const probs = new Array(NUM_TYPES_PER_LEVEL).fill(0);
    let pBoardSum = 0;

    for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        const nAccepted = Number(getAcceptedCount(level, type));
        if (nAccepted > 0) {
            const pBoardGivenType = countsPerType[type] / nAccepted;
            probs[type] = pBoardGivenType;
            pBoardSum += pBoardGivenType;
        }
    }

    if (pBoardSum > 0) {
        for (let i = 0; i < probs.length; i++) {
            probs[i] /= pBoardSum;
        }
    }

    return probs;
}

// Calculate probabilities for each panel
export function calculateProbabilities(board, compatibleBoards) {
    const level = board.level;

    if (compatibleBoards.length === 0) {
        return {
            panels: [],
            typeProbs: new Array(NUM_TYPES_PER_LEVEL).fill(0),
            totalCompatible: 0
        };
    }

    const boardsByType = groupBoardsByType(compatibleBoards, level);
    const countsPerType = boardsByType.map(group => group.length);
    const totalCompatible = countsPerType.reduce((a, b) => a + b, 0);
    const typeProbs = calculateTypeProbabilities(level, countsPerType);

    const unknownPositions = board.getUnknownPositions();

    const panelProbs = unknownPositions.map(pos => {
        const probs = { pos, pVoltorb: 0, pOne: 0, pTwo: 0, pThree: 0 };

        for (let value = 0; value <= 3; value++) {
            let pValue = 0;

            for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
                const typeBoards = boardsByType[type];
                if (typeBoards.length === 0) continue;

                let countWithValue = 0;
                for (const b of typeBoards) {
                    if (b.get(pos.row, pos.col) === value) {
                        countWithValue++;
                    }
                }

                const pValueGivenType = countWithValue / typeBoards.length;
                pValue += pValueGivenType * typeProbs[type];
            }

            switch (value) {
                case 0: probs.pVoltorb = pValue; break;
                case 1: probs.pOne = pValue; break;
                case 2: probs.pTwo = pValue; break;
                case 3: probs.pThree = pValue; break;
            }
        }

        return probs;
    });

    return {
        panels: panelProbs,
        typeProbs,
        totalCompatible
    };
}

// Find guaranteed safe panels
export function findSafePanels(board, compatibleBoards) {
    const safePanels = [];
    const unknownPositions = board.getUnknownPositions();

    for (const pos of unknownPositions) {
        let isSafe = true;
        for (const cb of compatibleBoards) {
            if (cb.get(pos.row, pos.col) === PanelValue.Voltorb) {
                isSafe = false;
                break;
            }
        }
        if (isSafe) {
            safePanels.push(pos);
        }
    }

    return safePanels;
}

// ============================================================================
// ITERATIVE DEEPENING SOLVER
// ============================================================================

/**
 * Search state for the minimax algorithm.
 * Tracks compatible boards grouped by type for efficient filtering.
 */
class SearchState {
    constructor(board, boardsByType, pBoardNorm) {
        this.board = board;
        this.boardsByType = boardsByType;
        this.pBoardNorm = pBoardNorm;
    }

    totalCompatible() {
        let total = 0;
        for (const boards of this.boardsByType) {
            total += boards.length;
        }
        return total;
    }
}

/**
 * Check if the game is won (all multipliers revealed).
 */
function isWon(state) {
    const totalBoards = state.boardsByType.reduce((sum, boards) => sum + boards.length, 0);
    const shouldLog = totalBoards <= 30;

    // First, count unknown panels and boards per type
    let unknownCount = 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(i, j) === PanelValue.Unknown) {
                unknownCount++;
            }
        }
    }

    if (totalBoards === 0) {
        if (shouldLog) {
            console.log(`[DEBUG] isWon: WARNING - no compatible boards! unknownCount=${unknownCount}`);
        }
        // No compatible boards is an error state - should not happen
        // Return false to be safe (assume not won)
        return unknownCount === 0; // Only won if no unknowns remain
    }

    const unrevealed = [];
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(i, j) === PanelValue.Unknown) {
                const pos = { row: i, col: j };
                let hasMultiplier = false;
                // Check if any compatible board has a multiplier here
                for (const boards of state.boardsByType) {
                    for (const b of boards) {
                        if (isMultiplier(b.get(pos.row, pos.col))) {
                            hasMultiplier = true;
                            unrevealed.push(`(${i},${j})`);
                            break;
                        }
                    }
                    if (hasMultiplier) break;
                }
            }
        }
    }
    if (unrevealed.length > 0) {
        if (shouldLog) {
            console.log(`[DEBUG] isWon=false, unrevealed multipliers at: ${unrevealed.join(', ')}, totalBoards=${totalBoards}, unknownCount=${unknownCount}`);
        }
        return false;
    }
    if (shouldLog) {
        console.log(`[DEBUG] isWon=true (no unrevealed multipliers), totalBoards=${totalBoards}, unknownCount=${unknownCount}`);
        // Log a sample board to verify multiplier positions
        for (const boards of state.boardsByType) {
            if (boards.length > 0) {
                const b = boards[0];
                let mults = [];
                for (let i = 0; i < BOARD_SIZE; i++) {
                    for (let j = 0; j < BOARD_SIZE; j++) {
                        if (isMultiplier(b.get(i, j))) {
                            mults.push(`(${i},${j})=${b.get(i, j)}`);
                        }
                    }
                }
                console.log(`  Sample board multipliers: ${mults.join(', ')}`);
                break;
            }
        }
    }
    return true;
}

/**
 * Find a free (guaranteed safe) panel.
 */
function findFreePanel(state) {
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            const pos = { row: i, col: j };
            if (state.board.get(i, j) !== PanelValue.Unknown) continue;

            let isFree = true;
            outer: for (const boards of state.boardsByType) {
                for (const b of boards) {
                    if (b.get(pos.row, pos.col) === PanelValue.Voltorb) {
                        isFree = false;
                        break outer;
                    }
                }
            }

            if (isFree) return pos;
        }
    }
    return null;
}

/**
 * Get unknown panels sorted by voltorb probability (safest first).
 */
function getOrderedUnknownPanels(state) {
    const panels = [];

    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(i, j) === PanelValue.Unknown) {
                const pos = { row: i, col: j };
                const pVoltorb = probabilityOf(state, pos, PanelValue.Voltorb);
                panels.push({ pos, pVoltorb });
            }
        }
    }

    // Sort by voltorb probability (lowest first)
    panels.sort((a, b) => a.pVoltorb - b.pVoltorb);

    return panels.map(p => p.pos);
}

/**
 * Calculate P(board) normalization factor.
 */
function calculateProbNorm(boardsByType, level) {
    let pBoard = 0;
    for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        const nAccepted = Number(getAcceptedCount(level, type));
        if (nAccepted > 0) {
            pBoard += boardsByType[type].length / nAccepted;
        }
    }
    return pBoard;
}

/**
 * Calculate probability of a specific value at a position.
 */
function probabilityOf(state, pos, value) {
    if (state.pBoardNorm <= 0) return 0;

    const level = state.board.level;
    let pValue = 0;

    for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        const typeBoards = state.boardsByType[type];
        if (typeBoards.length === 0) continue;

        const nAccepted = Number(getAcceptedCount(level, type));
        if (nAccepted <= 0) continue;

        let countWithValue = 0;
        for (const b of typeBoards) {
            if (b.get(pos.row, pos.col) === value) {
                countWithValue++;
            }
        }

        const pValueGivenType = countWithValue / typeBoards.length;
        const pTypeWeight = typeBoards.length / nAccepted;
        pValue += pValueGivenType * pTypeWeight / state.pBoardNorm;
    }

    return pValue;
}

/**
 * Create a new search state after revealing a panel.
 */
function revealPanel(state, pos, value) {
    const newBoard = state.board.withPanelRevealed(pos, value);

    // Filter compatible boards
    const newBoardsByType = [];
    for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        newBoardsByType[type] = state.boardsByType[type].filter(
            b => b.get(pos.row, pos.col) === value
        );
    }

    const newPBoardNorm = calculateProbNorm(newBoardsByType, newBoard.level);

    return new SearchState(newBoard, newBoardsByType, newPBoardNorm);
}

/**
 * Heuristic evaluation for leaf nodes.
 */
function heuristicEval(state) {
    if (isWon(state)) return 1.0;

    // Count multipliers still needed
    let multipliersNeeded = 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(i, j) === PanelValue.Unknown) {
                const pos = { row: i, col: j };
                // Check if any board has a multiplier here
                outer: for (const boards of state.boardsByType) {
                    for (const b of boards) {
                        if (isMultiplier(b.get(pos.row, pos.col))) {
                            multipliersNeeded++;
                            break outer;
                        }
                    }
                }
            }
        }
    }

    if (multipliersNeeded === 0) return 1.0;

    // Collect voltorb probabilities for risky panels
    const voltorbProbs = [];
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (state.board.get(i, j) !== PanelValue.Unknown) continue;
            const pos = { row: i, col: j };
            const pVoltorb = probabilityOf(state, pos, PanelValue.Voltorb);
            if (pVoltorb > 0) {
                voltorbProbs.push(pVoltorb);
            }
        }
    }

    if (voltorbProbs.length === 0) return 1.0;

    // Sort and estimate survival probability
    voltorbProbs.sort((a, b) => a - b);
    const panelsToReveal = Math.min(voltorbProbs.length, multipliersNeeded);
    let survivalProd = 1.0;
    for (let i = 0; i < panelsToReveal; i++) {
        survivalProd *= (1 - voltorbProbs[i]);
    }

    return survivalProd;
}

/**
 * Depth-limited search with memoization.
 */
function depthLimitedSearch(state, depthLimit, memo, nodesRef, startTime, timeout) {
    nodesRef.count++;

    // Timeout check
    if (Date.now() - startTime > timeout) {
        return { bestPanel: null, winProb: heuristicEval(state), fullyExplored: false };
    }

    // Win check
    if (isWon(state)) {
        const totalBoards = state.totalCompatible();
        if (totalBoards <= 30) {
            console.log(`[DEBUG] isWon=true at depth=${depthLimit}, totalBoards=${totalBoards}`);
            // Log revealed panels
            let revealed = [];
            for (let i = 0; i < BOARD_SIZE; i++) {
                for (let j = 0; j < BOARD_SIZE; j++) {
                    const v = state.board.get(i, j);
                    if (v !== PanelValue.Unknown) {
                        revealed.push(`(${i},${j})=${v}`);
                    }
                }
            }
            console.log(`  Revealed: ${revealed.join(', ')}`);
        }
        return { bestPanel: null, winProb: 1.0, fullyExplored: true };
    }

    // Depth limit reached
    if (depthLimit <= 0) {
        return { bestPanel: null, winProb: heuristicEval(state), fullyExplored: false };
    }

    // Memoization check
    const hash = state.board.hash();
    const memoKey = `${hash}_${depthLimit}`;
    if (memo.has(memoKey)) {
        const cached = memo.get(memoKey);
        return { bestPanel: cached.bestPanel, winProb: cached.winProb, fullyExplored: cached.fullyExplored };
    }

    // Free panel check
    const freePanel = findFreePanel(state);
    if (freePanel) {
        // Log at initial state (22 boards is the specific case we're debugging)
        const isInitialState = state.totalCompatible() === 22;
        const shouldLog = isInitialState || depthLimit >= 10;
        if (shouldLog) console.log(`[DEBUG] Free panel found: (${freePanel.row},${freePanel.col}) at depth=${depthLimit}, totalBoards=${state.totalCompatible()}`);
        let winProb = 0;
        let fullyExplored = true;
        let pSum = 0;

        for (let value = 1; value <= 3; value++) {
            const pValue = probabilityOf(state, freePanel, value);
            pSum += pValue;
            if (pValue <= 0) continue;

            const nextState = revealPanel(state, freePanel, value);
            const childBoardCount = nextState.totalCompatible();
            const child = depthLimitedSearch(nextState, depthLimit, memo, nodesRef, startTime, timeout);

            if (shouldLog) console.log(`  Free (${freePanel.row},${freePanel.col}) val=${value}: P=${pValue.toFixed(4)}, W=${child.winProb.toFixed(4)}, exact=${child.fullyExplored}, contribution=${(pValue * child.winProb).toFixed(4)}, childBoards=${childBoardCount}`);

            winProb += pValue * child.winProb;
            if (!child.fullyExplored) fullyExplored = false;
        }
        if (shouldLog) console.log(`  Free panel total: pSum=${pSum.toFixed(4)}, winProb=${winProb.toFixed(4)}, fullyExplored=${fullyExplored}`);

        memo.set(memoKey, { bestPanel: freePanel, winProb, fullyExplored });
        return { bestPanel: freePanel, winProb, fullyExplored };
    }

    // Get unknown panels in heuristic order
    const unknownPanels = getOrderedUnknownPanels(state);

    if (unknownPanels.length === 0) {
        return { bestPanel: null, winProb: 0, fullyExplored: true };
    }

    let bestPanel = unknownPanels[0];
    let bestWinProb = 0;
    let allFullyExplored = true;

    for (const pos of unknownPanels) {
        // Upper-bound pruning
        const upperBound = 1 - probabilityOf(state, pos, PanelValue.Voltorb);
        if (upperBound <= bestWinProb) continue;

        let panelWinProb = 0;
        let panelFullyExplored = true;

        console.log(`[DEBUG] Evaluating risky panel (${pos.row},${pos.col}) at depth=${depthLimit}`);
        for (let value = 1; value <= 3; value++) {
            const pValue = probabilityOf(state, pos, value);
            if (pValue <= 0) continue;

            const nextState = revealPanel(state, pos, value);
            const child = depthLimitedSearch(nextState, depthLimit - 1, memo, nodesRef, startTime, timeout);

            console.log(`  Risky (${pos.row},${pos.col}) val=${value}: P=${pValue.toFixed(3)}, W=${child.winProb.toFixed(3)}, exact=${child.fullyExplored}`);

            panelWinProb += pValue * child.winProb;
            if (!child.fullyExplored) panelFullyExplored = false;
        }
        console.log(`  Risky panel total: winProb=${panelWinProb.toFixed(3)}, fullyExplored=${panelFullyExplored}`);

        if (!panelFullyExplored) allFullyExplored = false;

        if (panelWinProb > bestWinProb) {
            bestWinProb = panelWinProb;
            bestPanel = pos;
        }

        // Timeout check
        if (Date.now() - startTime > timeout) {
            allFullyExplored = false;
            break;
        }
    }

    memo.set(memoKey, { bestPanel, winProb: bestWinProb, fullyExplored: allFullyExplored });
    return { bestPanel, winProb: bestWinProb, fullyExplored: allFullyExplored };
}

/**
 * Iterative deepening solver.
 * Yields progress updates at each depth level.
 */
export function* iterativeDeepening(board, compatibleBoards, options = {}) {
    const { maxDepth = 100, timeout = 10000 } = options;

    const level = board.level;
    const boardsByType = groupBoardsByType(compatibleBoards, level);
    const pBoardNorm = calculateProbNorm(boardsByType, level);

    const initialState = new SearchState(board, boardsByType, pBoardNorm);

    const startTime = Date.now();
    let nodesRef = { count: 0 };

    // Check for free panel first
    const freePanel = findFreePanel(initialState);

    for (let depth = 1; depth <= maxDepth; depth++) {
        // CRITICAL: Create fresh memo for each depth iteration.
        // Reusing memo across depths causes child states to return cached results
        // from shallower searches, effectively limiting all searches to depth 1.
        const memo = new Map();

        console.log(`[DEBUG] ===== Starting iterative deepening depth=${depth} =====`);
        const result = depthLimitedSearch(initialState, depth, memo, nodesRef, startTime, timeout);
        console.log(`[DEBUG] ===== Depth ${depth} result: winProb=${result.winProb.toFixed(3)}, exact=${result.fullyExplored}, bestPanel=(${result.bestPanel?.row},${result.bestPanel?.col}) =====`);

        const elapsed = Date.now() - startTime;

        yield {
            bestPanel: freePanel || result.bestPanel,
            winProbability: result.winProb,
            depth,
            isExact: result.fullyExplored,
            nodesSearched: nodesRef.count,
            elapsed,
            reason: freePanel ? 'Free panel (guaranteed safe)' : (result.fullyExplored ? 'Exact solution' : `Depth ${depth}`)
        };

        if (result.fullyExplored || elapsed >= timeout) {
            break;
        }
    }
}

/**
 * Main solver function with iterative deepening.
 */
export function solve(board, maxBoards = 10000, options = {}) {
    const startTime = performance.now();
    const { timeout = 5000 } = options;

    // Generate compatible boards
    const compatibleBoards = generateCompatibleBoards(board, maxBoards);

    if (compatibleBoards.length === 0) {
        return {
            suggestedPanel: null,
            winProbability: 0,
            probabilities: { panels: [], typeProbs: [], totalCompatible: 0 },
            safePanels: [],
            compatibleCount: 0,
            computeTime: performance.now() - startTime,
            depth: 0,
            isExact: true,
            reason: 'No compatible boards'
        };
    }

    // Calculate probabilities
    const probabilities = calculateProbabilities(board, compatibleBoards);

    // Find safe panels
    const safePanels = findSafePanels(board, compatibleBoards);

    // Run iterative deepening
    let finalResult = null;
    for (const progress of iterativeDeepening(board, compatibleBoards, { timeout })) {
        finalResult = progress;
    }

    // Determine suggested panel
    let suggestedPanel = finalResult?.bestPanel;

    // If we have safe panels and no solver result, prefer safe panel with highest expected value
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
        winProbability: finalResult?.winProbability ?? 0,
        probabilities,
        safePanels,
        compatibleCount: compatibleBoards.length,
        computeTime: performance.now() - startTime,
        depth: finalResult?.depth ?? 0,
        isExact: finalResult?.isExact ?? false,
        reason: finalResult?.reason ?? 'Unknown'
    };
}

// For backward compatibility
export function findBestPanel(board, probabilities) {
    if (probabilities.panels.length === 0) {
        return null;
    }

    let bestPanel = null;
    let lowestVoltorbProb = 1.1;

    for (const panelProb of probabilities.panels) {
        const score = panelProb.pVoltorb - (panelProb.pTwo + panelProb.pThree) * 0.01;
        if (score < lowestVoltorbProb) {
            lowestVoltorbProb = score;
            bestPanel = panelProb.pos;
        }
    }

    return bestPanel;
}

export function estimateWinProbability(board, compatibleBoards, probabilities) {
    // This is now a fallback - the main solver uses iterative deepening
    const result = solve(board, compatibleBoards.length);
    return result.winProbability;
}
