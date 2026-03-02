// UI Rendering for Voltorb Flip
// Handles DOM manipulation, rendering, and event handling

import { BOARD_SIZE, PanelValue } from './boardTypes.js';

export class UI {
    constructor() {
        // DOM elements
        this.tilesArea = document.querySelector('.tiles-area');
        this.colHintsArea = document.querySelector('.col-hints');
        this.rowHintsArea = document.querySelector('.row-hints');

        // Modals
        this.tileModal = document.getElementById('tileModal');
        this.hintModal = document.getElementById('hintModal');
        this.gameOverModal = document.getElementById('gameOverModal');

        // Controls
        this.levelSelect = document.getElementById('levelSelect');
        this.winProbValue = document.getElementById('winProbValue');
        this.winProbFill = document.getElementById('winProbFill');
        this.solverStatus = document.getElementById('solverStatus');
        this.suggestionValue = document.getElementById('suggestionValue');
        this.solveBtn = document.getElementById('solveBtn');
        this.autoSolveBtn = document.getElementById('autoSolveBtn');
        this.pauseBtn = document.getElementById('pauseBtn');
        this.undoBtn = document.getElementById('undoBtn');
        this.resetBtn = document.getElementById('resetBtn');

        // Mode buttons
        this.assistantModeBtn = document.getElementById('assistantModeBtn');
        this.playModeBtn = document.getElementById('playModeBtn');

        // Self-play controls
        this.selfPlayControls = document.querySelector('.selfplay-controls');
        this.selfPlayBtn = document.getElementById('selfPlayBtn');
        this.speedSlider = document.getElementById('speedSlider');

        // New Game button for assistant mode
        this.newGameAssistantBtn = document.getElementById('newGameAssistantBtn');

        // Solver settings
        this.timeoutSelect = document.getElementById('timeoutSelect');

        // Backend toggle
        this.jsBackendBtn = document.getElementById('jsBackendBtn');
        this.wasmBackendBtn = document.getElementById('wasmBackendBtn');
        this.wasmStatus = document.getElementById('wasmStatus');

        // Probability toggle
        this.probDetailedBtn = document.getElementById('probDetailedBtn');
        this.probVoltorbBtn = document.getElementById('probVoltorbBtn');
        this.probNoneBtn = document.getElementById('probNoneBtn');
        this.legendSection = document.getElementById('legendSection');

        // Callbacks
        this.onTileClick = null;
        this.onHintClick = null;
        this.onTileValueSelect = null;
        this.onHintEdit = null;
        this.onLevelChange = null;
        this.onSolve = null;
        this.onAutoSolveToggle = null;
        this.onUndo = null;
        this.onReset = null;
        this.onModeChange = null;
        this.onNewGameAssistant = null;
        this.onPlayPause = null;
        this.onSpeedChange = null;
        this.onProbDisplayModeChange = null;
        this.onTimeoutChange = null;
        this.onSolverBackendChange = null;
        this.onPause = null;

        // State
        this.selectedTile = null;
        this.selectedHint = null;
        this.probDisplayMode = 'voltorb'; // 'detailed', 'voltorb', 'none'
        this.currentMode = 'selfplay'; // track current mode for hint rendering

        this.setupEventListeners();
    }

    setupEventListeners() {
        // Modal cancel buttons
        document.getElementById('modalCancel').addEventListener('click', () => {
            this.hideTileModal();
        });

        document.getElementById('hintCancel').addEventListener('click', () => {
            this.hideHintModal();
        });

        document.getElementById('hintOk').addEventListener('click', () => {
            if (this.onHintEdit && this.selectedHint) {
                const sum = parseInt(document.getElementById('hintSum').value) || 0;
                const voltorbs = parseInt(document.getElementById('hintVoltorbs').value) || 0;
                this.onHintEdit(this.selectedHint.type, this.selectedHint.index, sum, voltorbs);
            }
            this.hideHintModal();
        });

        document.getElementById('gameOverOk').addEventListener('click', () => {
            this.hideGameOverModal();
        });

        // Tile value buttons
        document.querySelectorAll('.tile-value-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const value = parseInt(btn.dataset.value);
                if (this.onTileValueSelect && this.selectedTile) {
                    this.onTileValueSelect(this.selectedTile.row, this.selectedTile.col, value);
                }
                this.hideTileModal();
            });
        });

        // Level select
        this.levelSelect.addEventListener('change', () => {
            if (this.onLevelChange) {
                this.onLevelChange(parseInt(this.levelSelect.value));
            }
        });

        // Control buttons
        this.solveBtn.addEventListener('click', () => {
            if (this.onSolve) this.onSolve();
        });

        this.autoSolveBtn.addEventListener('click', () => {
            if (this.onAutoSolveToggle) this.onAutoSolveToggle();
        });

        this.pauseBtn.addEventListener('click', () => {
            if (this.onPause) this.onPause();
        });

        this.undoBtn.addEventListener('click', () => {
            if (this.onUndo) this.onUndo();
        });

        this.resetBtn.addEventListener('click', () => {
            if (this.onReset) this.onReset();
        });

        // Mode buttons
        this.assistantModeBtn.addEventListener('click', () => {
            if (this.onModeChange) this.onModeChange('assistant');
        });

        this.playModeBtn.addEventListener('click', () => {
            if (this.onModeChange) this.onModeChange('selfplay');
        });

        // New Game button (works in both modes)
        this.newGameAssistantBtn.addEventListener('click', () => {
            if (this.onNewGameAssistant) this.onNewGameAssistant();
        });

        this.selfPlayBtn.addEventListener('click', () => {
            if (this.onPlayPause) this.onPlayPause();
        });

        this.speedSlider.addEventListener('input', () => {
            if (this.onSpeedChange) this.onSpeedChange(parseInt(this.speedSlider.value));
        });

        // Probability display toggle
        this.probDetailedBtn.addEventListener('click', () => {
            this.setProbDisplayMode('detailed');
        });

        this.probVoltorbBtn.addEventListener('click', () => {
            this.setProbDisplayMode('voltorb');
        });

        this.probNoneBtn.addEventListener('click', () => {
            this.setProbDisplayMode('none');
        });

        // Timeout select
        this.timeoutSelect.addEventListener('change', () => {
            if (this.onTimeoutChange) {
                this.onTimeoutChange(parseInt(this.timeoutSelect.value));
            }
        });

        // Backend toggle
        this.jsBackendBtn.addEventListener('click', () => {
            this.setSolverBackend('js');
            if (this.onSolverBackendChange) this.onSolverBackendChange('js');
        });

        this.wasmBackendBtn.addEventListener('click', () => {
            this.setSolverBackend('wasm');
            if (this.onSolverBackendChange) this.onSolverBackendChange('wasm');
        });

        // Click outside modal to close
        this.tileModal.addEventListener('click', (e) => {
            if (e.target === this.tileModal) {
                this.hideTileModal();
            }
        });

        this.hintModal.addEventListener('click', (e) => {
            if (e.target === this.hintModal) {
                this.hideHintModal();
            }
        });

        this.gameOverModal.addEventListener('click', (e) => {
            if (e.target === this.gameOverModal) {
                this.hideGameOverModal();
            }
        });
    }

    // Save which hint input is currently focused (before re-render)
    _saveFocusedHintInput() {
        const active = document.activeElement;
        if (active && (active.classList.contains('hint-sum-input') || active.classList.contains('hint-voltorb-input'))) {
            return {
                type: active.dataset.hintType,
                index: active.dataset.hintIndex,
                field: active.dataset.hintField,
                selectionStart: active.selectionStart,
                selectionEnd: active.selectionEnd
            };
        }
        return null;
    }

    // Restore focus to the same hint input after re-render
    _restoreFocusedHintInput(saved) {
        if (!saved) return;
        const selector = saved.field === 'sum' ? '.hint-sum-input' : '.hint-voltorb-input';
        const inputs = document.querySelectorAll(selector);
        for (const input of inputs) {
            if (input.dataset.hintType === saved.type && input.dataset.hintIndex === saved.index) {
                input.focus();
                if (saved.selectionStart !== undefined) {
                    input.setSelectionRange(saved.selectionStart, saved.selectionEnd);
                }
                break;
            }
        }
    }

    // Render the complete board
    renderBoard(board, probabilities = null, suggestedPanel = null, safePanels = []) {
        const savedFocus = this._saveFocusedHintInput();
        this.renderTiles(board, probabilities, suggestedPanel, safePanels);
        this.renderRowHints(board);
        this.renderColHints(board);
        this._restoreFocusedHintInput(savedFocus);
    }

    // Render tiles
    renderTiles(board, probabilities = null, suggestedPanel = null, safePanels = []) {
        this.tilesArea.innerHTML = '';

        for (let i = 0; i < BOARD_SIZE; i++) {
            for (let j = 0; j < BOARD_SIZE; j++) {
                const tile = document.createElement('div');
                tile.className = 'tile';
                tile.dataset.row = i;
                tile.dataset.col = j;

                const value = board.get(i, j);

                if (value === PanelValue.Unknown) {
                    tile.classList.add('covered');

                    // Find probability for this tile
                    const prob = probabilities?.panels?.find(p =>
                        p.pos.row === i && p.pos.col === j
                    );

                    if (prob && this.probDisplayMode !== 'none') {
                        if (this.probDisplayMode === 'detailed') {
                            // Detailed mode: show all four percentages
                            const detailed = document.createElement('div');
                            detailed.className = 'prob-detailed';

                            const values = [
                                { label: '0:', value: prob.pVoltorb, isVoltorb: true },
                                { label: '1:', value: prob.pOne, isVoltorb: false },
                                { label: '2:', value: prob.pTwo, isVoltorb: false },
                                { label: '3:', value: prob.pThree, isVoltorb: false }
                            ];

                            for (const v of values) {
                                const row = document.createElement('div');
                                row.className = 'prob-row' + (v.isVoltorb ? ' voltorb' : '');

                                const label = document.createElement('span');
                                label.className = 'prob-label';
                                label.textContent = v.label;

                                const val = document.createElement('span');
                                val.className = 'prob-value';
                                val.textContent = `${Math.round(v.value * 100)}%`;

                                row.appendChild(label);
                                row.appendChild(val);
                                detailed.appendChild(row);
                            }

                            tile.appendChild(detailed);
                        } else if (this.probDisplayMode === 'voltorb') {
                            // Voltorb mode: show colored safety bar
                            const overlay = document.createElement('div');
                            overlay.className = 'prob-overlay';

                            const safePercent = (1 - prob.pVoltorb) * 100;

                            // Skip colored bar entirely when ~100% Voltorb to avoid red sliver
                            if (safePercent >= 0.5) {
                                const bar = document.createElement('div');
                                bar.className = 'prob-bar';
                                bar.style.width = `${safePercent}%`;

                                // Color based on voltorb probability
                                if (prob.pVoltorb === 0) {
                                    bar.classList.add('safe');
                                } else if (prob.pVoltorb < 0.25) {
                                    bar.classList.add('low-risk');
                                } else if (prob.pVoltorb < 0.5) {
                                    bar.classList.add('medium-risk');
                                } else {
                                    bar.classList.add('high-risk');
                                }

                                overlay.appendChild(bar);
                            }

                            tile.appendChild(overlay);

                            // Add probability text on hover
                            const probText = document.createElement('div');
                            probText.className = 'prob-text';
                            const voltorbIcon = document.createElement('span');
                            voltorbIcon.className = 'voltorb-icon';
                            probText.appendChild(voltorbIcon);
                            probText.appendChild(document.createTextNode(`${Math.round(prob.pVoltorb * 100)}%`));
                            tile.appendChild(probText);
                        }
                    }

                    // Add safety bar for safe panels in detailed mode (like voltorb mode)
                    if (this.probDisplayMode === 'detailed' && prob && prob.pVoltorb === 0) {
                        const overlay = document.createElement('div');
                        overlay.className = 'prob-overlay';
                        const bar = document.createElement('div');
                        bar.className = 'prob-bar safe';
                        bar.style.width = '100%';
                        overlay.appendChild(bar);
                        tile.appendChild(overlay);
                    }

                    // Check if this is a safe panel and suggested panel
                    const isSafe = safePanels.some(p => p.row === i && p.col === j);
                    const isSuggested = suggestedPanel && suggestedPanel.row === i && suggestedPanel.col === j;

                    // Apply combined box-shadow based on both states
                    if (isSafe && this.probDisplayMode !== 'none') {
                        if (isSuggested) {
                            // Both safe and suggested: combine shadows
                            tile.style.boxShadow = '0 0 0 4px var(--suggestion-color), 0 0 15px var(--suggestion-color), 0 0 0 3px var(--safe-color) inset';
                        } else {
                            // Only safe
                            tile.style.boxShadow = '0 0 0 3px var(--safe-color) inset';
                        }
                    } else if (isSuggested) {
                        // Only suggested (handled by CSS class)
                        tile.classList.add('suggested');
                    }
                } else if (value === PanelValue.Voltorb) {
                    tile.classList.add('voltorb');
                } else {
                    tile.classList.add('revealed');
                    tile.classList.add(['', 'one', 'two', 'three'][value]);
                    tile.textContent = value;
                }

                // Add click handler
                tile.addEventListener('click', () => {
                    if (this.onTileClick) {
                        this.onTileClick(i, j, value);
                    }
                });

                this.tilesArea.appendChild(tile);
            }
        }
    }

    // Render row hints
    renderRowHints(board) {
        this.rowHintsArea.innerHTML = '';

        for (let i = 0; i < BOARD_SIZE; i++) {
            const hint = board.rowHint(i);
            const panel = this.createHintPanel(hint, 'row', i);
            this.rowHintsArea.appendChild(panel);
        }
    }

    // Render column hints
    renderColHints(board) {
        this.colHintsArea.innerHTML = '';

        for (let j = 0; j < BOARD_SIZE; j++) {
            const hint = board.colHint(j);
            const panel = this.createHintPanel(hint, 'col', j);
            panel.classList.add('col-hint');
            this.colHintsArea.appendChild(panel);
        }
    }

    // Create a hint panel element
    createHintPanel(hint, type, index) {
        const panel = document.createElement('div');
        panel.className = 'hint-panel';
        panel.dataset.type = type;
        panel.dataset.index = index;

        if (this.currentMode === 'assistant') {
            // Inline editing mode: use input fields
            // tabindex: rows 0-4 sum(1,3,5,7,9) voltorb(2,4,6,8,10), cols 0-4 sum(11,13,15,17,19) voltorb(12,14,16,18,20)
            const baseTab = type === 'row' ? index * 2 + 1 : 10 + index * 2 + 1;

            const sumInput = document.createElement('input');
            sumInput.type = 'number';
            sumInput.className = 'hint-sum-input';
            sumInput.min = 0;
            sumInput.max = 15;
            sumInput.value = hint.sum;
            sumInput.tabIndex = baseTab;
            sumInput.dataset.hintType = type;
            sumInput.dataset.hintIndex = index;
            sumInput.dataset.hintField = 'sum';

            const voltorbDiv = document.createElement('div');
            voltorbDiv.className = 'hint-voltorb';

            const voltorbIcon = document.createElement('div');
            voltorbIcon.className = 'hint-voltorb-icon';

            const voltorbInput = document.createElement('input');
            voltorbInput.type = 'number';
            voltorbInput.className = 'hint-voltorb-input';
            voltorbInput.min = 0;
            voltorbInput.max = 5;
            voltorbInput.value = hint.voltorbCount;
            voltorbInput.tabIndex = baseTab + 1;
            voltorbInput.dataset.hintType = type;
            voltorbInput.dataset.hintIndex = index;
            voltorbInput.dataset.hintField = 'voltorb';

            voltorbDiv.appendChild(voltorbIcon);
            voltorbDiv.appendChild(voltorbInput);

            panel.appendChild(sumInput);
            panel.appendChild(voltorbDiv);

            // Prevent panel click from interfering with inputs
            sumInput.addEventListener('click', (e) => e.stopPropagation());
            voltorbInput.addEventListener('click', (e) => e.stopPropagation());

            // On change, call onHintEdit directly
            const handleChange = () => {
                if (this.onHintEdit) {
                    const sum = parseInt(sumInput.value) || 0;
                    const voltorbs = parseInt(voltorbInput.value) || 0;
                    this.onHintEdit(type, index, sum, voltorbs);
                }
            };

            sumInput.addEventListener('change', handleChange);
            voltorbInput.addEventListener('change', handleChange);

            // Select all text on focus for easy editing
            sumInput.addEventListener('focus', () => sumInput.select());
            voltorbInput.addEventListener('focus', () => voltorbInput.select());
        } else {
            // Self-play mode: display-only rendering
            const sumDiv = document.createElement('div');
            sumDiv.className = 'hint-sum';
            sumDiv.textContent = hint.sum.toString().padStart(2, '0');

            const voltorbDiv = document.createElement('div');
            voltorbDiv.className = 'hint-voltorb';

            const voltorbIcon = document.createElement('div');
            voltorbIcon.className = 'hint-voltorb-icon';

            const voltorbCount = document.createElement('span');
            voltorbCount.textContent = hint.voltorbCount;

            voltorbDiv.appendChild(voltorbIcon);
            voltorbDiv.appendChild(voltorbCount);

            panel.appendChild(sumDiv);
            panel.appendChild(voltorbDiv);

            // Add click handler for editing (selfplay mode uses modal)
            panel.addEventListener('click', () => {
                if (this.onHintClick) {
                    this.onHintClick(type, index, hint);
                }
            });
        }

        return panel;
    }

    // Show tile modal
    showTileModal(row, col) {
        this.selectedTile = { row, col };
        this.tileModal.style.display = 'flex';
    }

    // Hide tile modal
    hideTileModal() {
        this.selectedTile = null;
        this.tileModal.style.display = 'none';
    }

    // Show hint modal
    showHintModal(type, index, hint) {
        this.selectedHint = { type, index };
        document.getElementById('hintSum').value = hint.sum;
        document.getElementById('hintVoltorbs').value = hint.voltorbCount;
        this.hintModal.style.display = 'flex';
    }

    // Hide hint modal
    hideHintModal() {
        this.selectedHint = null;
        this.hintModal.style.display = 'none';
    }

    // Show game over modal
    showGameOverModal(won, message) {
        const title = document.getElementById('gameOverTitle');
        const msg = document.getElementById('gameOverMessage');

        title.textContent = won ? 'You Win!' : 'Game Over';
        title.style.color = won ? 'var(--safe-color)' : 'var(--high-risk-color)';
        msg.textContent = message || '';

        this.gameOverModal.style.display = 'flex';
    }

    // Hide game over modal
    hideGameOverModal() {
        this.gameOverModal.style.display = 'none';
    }

    // Update win probability display
    updateWinProbability(prob, solverInfo = null, searching = false) {
        if (prob === null || prob === undefined) {
            this.winProbValue.textContent = '--';
            this.winProbFill.style.width = '0%';
        } else {
            const percent = Math.round(prob * 100);
            this.winProbValue.textContent = `${percent}%`;
            this.winProbFill.style.width = `${percent}%`;
        }

        // Update solver status
        if (this.solverStatus) {
            if (solverInfo) {
                if (solverInfo.isExact) {
                    this.solverStatus.textContent = 'Exact';
                    this.solverStatus.className = 'solver-status exact';
                } else if (searching) {
                    this.solverStatus.textContent = `Depth ${solverInfo.depth}...`;
                    this.solverStatus.className = 'solver-status searching';
                } else {
                    this.solverStatus.textContent = `Depth ${solverInfo.depth}`;
                    this.solverStatus.className = 'solver-status approximate';
                }
            } else {
                this.solverStatus.textContent = '--';
                this.solverStatus.className = 'solver-status';
            }
        }
    }

    // Update suggestion display
    updateSuggestion(panel, isSafe = false) {
        if (!panel) {
            this.suggestionValue.textContent = '--';
        } else {
            const pos = `R${panel.row + 1}C${panel.col + 1}`;
            this.suggestionValue.textContent = isSafe ? `${pos} (Safe!)` : pos;
            this.suggestionValue.style.color = isSafe ? 'var(--safe-color)' : '';
        }
    }

    // Update solver info (compatible boards and type probabilities)
    updateSolverInfo(compatibleCount, typeProbs, capped = false) {
        const countEl = document.getElementById('compatibleCount');
        const listEl = document.getElementById('boardTypesList');

        if (compatibleCount === null || compatibleCount === undefined) {
            countEl.textContent = '--';
            listEl.innerHTML = '<div style="color: rgba(255,255,255,0.5); font-style: italic;">Click Solve to analyze</div>';
            return;
        }

        // Format compatible count with thousands separator
        countEl.textContent = compatibleCount.toLocaleString() + (capped ? '+' : '');

        // Render type probabilities
        listEl.innerHTML = '';

        if (!typeProbs || typeProbs.length === 0) {
            listEl.innerHTML = '<div style="color: rgba(255,255,255,0.5);">No data</div>';
            return;
        }

        // Find max probability for scaling
        const maxProb = Math.max(...typeProbs);

        // Create rows for each type with non-zero probability
        for (let i = 0; i < typeProbs.length; i++) {
            const prob = typeProbs[i];
            if (prob < 0.001) continue; // Skip near-zero probabilities

            const row = document.createElement('div');
            row.className = 'board-type-row';

            const indexEl = document.createElement('span');
            indexEl.className = 'board-type-index';
            indexEl.textContent = `T${i}`;

            const barContainer = document.createElement('div');
            barContainer.className = 'board-type-bar-container';

            const bar = document.createElement('div');
            bar.className = 'board-type-bar';
            bar.style.width = `${(prob / maxProb) * 100}%`;

            barContainer.appendChild(bar);

            const percentEl = document.createElement('span');
            percentEl.className = 'board-type-percent';
            percentEl.textContent = `${(prob * 100).toFixed(1)}%`;

            row.appendChild(indexEl);
            row.appendChild(barContainer);
            row.appendChild(percentEl);

            listEl.appendChild(row);
        }

        // If no types shown (all zero), show message
        if (listEl.children.length === 0) {
            listEl.innerHTML = '<div style="color: rgba(255,255,255,0.5);">No compatible types</div>';
        }
    }

    // Set mode
    setMode(mode) {
        this.currentMode = mode;
        if (mode === 'assistant') {
            this.assistantModeBtn.classList.add('active');
            this.playModeBtn.classList.remove('active');
            this.selfPlayControls.style.display = 'none';
        } else {
            this.assistantModeBtn.classList.remove('active');
            this.playModeBtn.classList.add('active');
            this.selfPlayControls.style.display = 'block';
        }
    }

    // Set play/pause button state (Self-Play button)
    setPlaying(isPlaying) {
        this.selfPlayBtn.textContent = isPlaying ? 'Pause' : 'Self-Play';
    }

    // Set probability display mode
    setProbDisplayMode(mode) {
        this.probDisplayMode = mode;

        // Update button states
        this.probDetailedBtn.classList.toggle('active', mode === 'detailed');
        this.probVoltorbBtn.classList.toggle('active', mode === 'voltorb');
        this.probNoneBtn.classList.toggle('active', mode === 'none');

        // Show/hide legend based on mode
        if (this.legendSection) {
            this.legendSection.classList.toggle('hidden', mode !== 'voltorb');
        }

        // Trigger callback to re-render
        if (this.onProbDisplayModeChange) {
            this.onProbDisplayModeChange(mode);
        }
    }

    // Enable/disable undo button
    setUndoEnabled(enabled) {
        this.undoBtn.disabled = !enabled;
    }

    // Set auto-solve mode
    setAutoSolve(enabled) {
        this.autoSolveBtn.classList.toggle('active', enabled);
        this.solveBtn.disabled = enabled;
    }

    // Enable/disable pause button
    setPauseEnabled(enabled) {
        this.pauseBtn.disabled = !enabled;
    }

    // Animate tile reveal
    animateTileReveal(row, col, callback) {
        const tile = this.tilesArea.querySelector(`[data-row="${row}"][data-col="${col}"]`);
        if (tile) {
            tile.classList.add('revealing');
            setTimeout(() => {
                tile.classList.remove('revealing');
                if (callback) callback();
            }, 300);
        } else {
            if (callback) callback();
        }
    }

    // Set level without triggering callback
    setLevel(level) {
        this.levelSelect.value = level;
    }

    // Set solver backend toggle
    setSolverBackend(backend) {
        this.jsBackendBtn.classList.toggle('active', backend === 'js');
        this.wasmBackendBtn.classList.toggle('active', backend === 'wasm');
    }

    // Set WASM loading state
    setWasmLoading(loading) {
        if (loading) {
            this.wasmStatus.textContent = 'Loading...';
            this.wasmStatus.className = 'wasm-status loading';
        } else {
            this.wasmStatus.textContent = '';
            this.wasmStatus.className = 'wasm-status';
        }
    }

    // Set WASM availability
    setWasmAvailable(available) {
        if (available) {
            this.wasmStatus.textContent = '';
            this.wasmStatus.className = 'wasm-status';
        } else {
            this.wasmStatus.textContent = 'Error';
            this.wasmStatus.className = 'wasm-status error';
        }
    }
}
