// Solver Logic for Voltorb Flip
// Ported from C++ src/solver.cpp, src/probability.cpp, src/constraints.cpp

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
            // Check if remaining constraints are achievable
            const remainingSum = rowHints[i].sum - sum;
            const remainingVoltorbs = rowHints[i].voltorbCount - voltorbs;
            const remainingNonVoltorbs = unknownCount - remainingVoltorbs;

            // Each non-voltorb contributes at least 1, at most 3
            if (remainingNonVoltorbs < 0) return false; // More voltorbs needed than unknowns
            if (remainingSum < remainingNonVoltorbs) return false; // Can't achieve sum (need at least 1 per non-voltorb)
            if (remainingSum > remainingNonVoltorbs * 3) return false; // Can't achieve sum (max 3 per non-voltorb)
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
            // Check if remaining constraints are achievable
            const remainingSum = colHints[j].sum - sum;
            const remainingVoltorbs = colHints[j].voltorbCount - voltorbs;
            const remainingNonVoltorbs = unknownCount - remainingVoltorbs;

            // Each non-voltorb contributes at least 1, at most 3
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
function* generateVoltorbPositions(board, callback) {
    const rowHints = board.rowHints;
    const colHints = board.colHints;

    // Recursive generator
    function* generateRow(row, colCounts, current) {
        if (row === BOARD_SIZE) {
            // Verify column counts match exactly
            for (let j = 0; j < BOARD_SIZE; j++) {
                if (colCounts[j] !== colHints[j].voltorbCount) return;
            }

            // Check conflicts with revealed panels
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

        // Generate all combinations of voltorbsNeeded positions in this row
        for (const combo of combinations(rowPositions, voltorbsNeeded)) {
            // Set voltorb positions for this row
            const newColCounts = [...colCounts];
            const newCurrent = current.map(r => [...r]);
            newCurrent[row] = [false, false, false, false, false];

            for (const j of combo) {
                newCurrent[row][j] = true;
                newColCounts[j]++;
            }

            // Check column constraints so far
            let valid = true;
            for (let j = 0; j < BOARD_SIZE; j++) {
                if (newColCounts[j] > colHints[j].voltorbCount) {
                    valid = false;
                    break;
                }
                // Can we still fit remaining voltorbs?
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

    // Create template board with voltorbs
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

    // Copy revealed panels
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            const revealed = board.get(i, j);
            if (isKnown(revealed) && revealed !== PanelValue.Voltorb) {
                template.set(i, j, revealed);
            }
        }
    }

    // Count already placed 2s and 3s
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

    // Find unknown (non-voltorb) positions
    const unknownPositions = [];
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            if (template.get(i, j) === PanelValue.Unknown) {
                unknownPositions.push({ row: i, col: j });
            }
        }
    }

    let count = 0;

    // Recursive filling
    function* fill(posIdx, board, rem2s, rem3s) {
        if (count >= maxBoards) return;

        if (posIdx >= unknownPositions.length) {
            // All filled - check legality
            if (panelsDontExceedConstraints(board) && isLegal(board, params)) {
                count++;
                yield board.clone();
            }
            return;
        }

        const pos = unknownPositions[posIdx];
        const remainingPositions = unknownPositions.length - posIdx - 1;

        // Must be able to place remaining 2s and 3s
        if (rem2s + rem3s > remainingPositions + 1) return;

        // Try placing a 1
        board.set(pos.row, pos.col, PanelValue.One);
        if (panelsDontExceedConstraints(board)) {
            yield* fill(posIdx + 1, board, rem2s, rem3s);
        }

        // Try placing a 2
        if (rem2s > 0) {
            board.set(pos.row, pos.col, PanelValue.Two);
            if (panelsDontExceedConstraints(board)) {
                yield* fill(posIdx + 1, board, rem2s - 1, rem3s);
            }
        }

        // Try placing a 3
        if (rem3s > 0) {
            board.set(pos.row, pos.col, PanelValue.Three);
            if (panelsDontExceedConstraints(board)) {
                yield* fill(posIdx + 1, board, rem2s, rem3s - 1);
            }
        }

        // Reset for backtracking
        board.set(pos.row, pos.col, PanelValue.Unknown);
    }

    yield* fill(0, template, remaining2s, remaining3s);
}

// Check if board is compatible with a specific type
function isBoardCompatibleWithType(board, level, type) {
    const params = getParams(level, type);
    const { totalVoltorbs, totalSum } = board.getTotals();

    if (params.n0 !== totalVoltorbs) return false;
    if (getTotalSum(params) !== totalSum) return false;

    return true;
}

// Generate compatible boards for a given board state
export function generateCompatibleBoards(board, maxBoards = 10000) {
    const level = board.level;
    const compatibleBoards = [];

    // Early exit: check if the current revealed state is even possible
    if (!panelsDontExceedConstraints(board)) {
        return compatibleBoards; // Empty - impossible state
    }

    const { totalVoltorbs, totalSum } = board.getTotals();

    // Find compatible types
    const compatibleTypes = [];
    for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        if (isCompatibleWithType(totalVoltorbs, totalSum, level, type)) {
            compatibleTypes.push(type);
        }
    }

    if (compatibleTypes.length === 0) return compatibleBoards;

    // Generate voltorb positions once
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

    // Normalize
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

    // Group by type
    const boardsByType = groupBoardsByType(compatibleBoards, level);

    // Count per type
    const countsPerType = boardsByType.map(group => group.length);
    const totalCompatible = countsPerType.reduce((a, b) => a + b, 0);

    // Type probabilities
    const typeProbs = calculateTypeProbabilities(level, countsPerType);

    // Find unknown panels
    const unknownPositions = board.getUnknownPositions();

    // Calculate probabilities for each unknown panel
    const panelProbs = unknownPositions.map(pos => {
        const probs = { pos, pVoltorb: 0, pOne: 0, pTwo: 0, pThree: 0 };

        for (let value = 0; value <= 3; value++) {
            let pValue = 0;

            for (let type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
                const typeBoards = boardsByType[type];
                if (typeBoards.length === 0) continue;

                // Count boards of this type where panel has this value
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

// Find guaranteed safe panels (never voltorb in any compatible board)
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

// Find best panel to flip using simple heuristic (lowest voltorb probability)
export function findBestPanel(board, probabilities) {
    if (probabilities.panels.length === 0) {
        return null;
    }

    let bestPanel = null;
    let lowestVoltorbProb = 1.1;

    for (const panelProb of probabilities.panels) {
        // Prefer panels with lower voltorb probability
        // Among those, prefer panels with higher multiplier probability
        const score = panelProb.pVoltorb - (panelProb.pTwo + panelProb.pThree) * 0.01;

        if (score < lowestVoltorbProb) {
            lowestVoltorbProb = score;
            bestPanel = panelProb.pos;
        }
    }

    return bestPanel;
}

// Estimate win probability (simplified)
export function estimateWinProbability(board, compatibleBoards, probabilities) {
    if (compatibleBoards.length === 0) return 0;

    // Find safe panels
    const safePanels = findSafePanels(board, compatibleBoards);

    // Count how many multipliers we still need
    const { totalSum } = board.getTotals();

    let revealed2s = 0, revealed3s = 0;
    for (let i = 0; i < BOARD_SIZE; i++) {
        for (let j = 0; j < BOARD_SIZE; j++) {
            const v = board.get(i, j);
            if (v === PanelValue.Two) revealed2s++;
            if (v === PanelValue.Three) revealed3s++;
        }
    }

    // If we have safe panels, we can make progress without risk
    if (safePanels.length > 0) {
        // Check if any safe panel could be a multiplier
        let safeMultiplierPossible = false;
        for (const pos of safePanels) {
            for (const cb of compatibleBoards) {
                const v = cb.get(pos.row, pos.col);
                if (isMultiplier(v)) {
                    safeMultiplierPossible = true;
                    break;
                }
            }
            if (safeMultiplierPossible) break;
        }

        if (safeMultiplierPossible) {
            // We can make safe progress - estimate higher probability
            return Math.min(0.95, 0.7 + safePanels.length * 0.05);
        }
    }

    // Estimate based on lowest voltorb probability
    const bestPanel = findBestPanel(board, probabilities);
    if (!bestPanel) return 0;

    const panelProb = probabilities.panels.find(p =>
        p.pos.row === bestPanel.row && p.pos.col === bestPanel.col
    );

    if (!panelProb) return 0;

    // Rough estimate: survival probability for next move
    // This is a simplification - full minimax would be more accurate
    const survivalProb = 1 - panelProb.pVoltorb;

    // Calculate average voltorb probability only for RISKY panels (exclude safe ones)
    // Safe panels don't contribute to risk since we can reveal them freely
    const riskyPanels = probabilities.panels.filter(p => p.pVoltorb > 0);

    if (riskyPanels.length === 0) {
        // All remaining panels are safe - guaranteed win
        return 1.0;
    }

    const avgVoltorbProb = riskyPanels.reduce((sum, p) => sum + p.pVoltorb, 0) / riskyPanels.length;

    // Estimate future success based on risky panels we need to reveal
    // Use risky panel count, not total unknown count
    const estimatedFutureSuccess = Math.pow(1 - avgVoltorbProb, Math.max(1, riskyPanels.length / 3));

    return survivalProb * Math.max(0.3, estimatedFutureSuccess);
}

// Main solver function
export function solve(board, maxBoards = 10000) {
    const startTime = performance.now();

    // Generate compatible boards
    const compatibleBoards = generateCompatibleBoards(board, maxBoards);

    if (compatibleBoards.length === 0) {
        return {
            suggestedPanel: null,
            winProbability: 0,
            probabilities: { panels: [], typeProbs: [], totalCompatible: 0 },
            safePanels: [],
            compatibleCount: 0,
            computeTime: performance.now() - startTime
        };
    }

    // Calculate probabilities
    const probabilities = calculateProbabilities(board, compatibleBoards);

    // Find safe panels
    const safePanels = findSafePanels(board, compatibleBoards);

    // Determine suggested panel
    let suggestedPanel;
    if (safePanels.length > 0) {
        // Prefer safe panel with highest expected value
        // E[V] = P(1)*1 + P(2)*2 + P(3)*3
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
    } else {
        suggestedPanel = findBestPanel(board, probabilities);
    }

    // Estimate win probability
    const winProbability = estimateWinProbability(board, compatibleBoards, probabilities);

    return {
        suggestedPanel,
        winProbability,
        probabilities,
        safePanels,
        compatibleCount: compatibleBoards.length,
        computeTime: performance.now() - startTime
    };
}
