# Voltorb Flip Web GUI

A web-based GUI for the Voltorb Flip solver with pixel-art styling, live probability display, and both assistant and self-play modes.

**[Play Online](https://gimmytomas.github.io/voltorb-flip/)**

## Features

- **Pixel-art styled GUI** matching the original game
- **Assistant mode**: Enter hints, click to reveal, get solver suggestions
- **Self-play demo**: Watch the solver play automatically
- **Live probabilities**: Show P(Voltorb) for each panel with color-coded bars
- **Win probability** estimate display
- **Safe panel detection**: Guaranteed-safe panels highlighted in green
- **WASM solver backend**: C++ solver compiled to WebAssembly for near-native speed
- **Progressive depth updates**: See solver results improve in real time at each search depth
- **Solver settings**: Configurable timeout (up to 120s) and JS/WASM engine toggle
- **Undo support**: Revert previous moves
- **Responsive design**: Works on desktop and mobile

## How to Use

### Quick Start

1. Open `index.html` in a modern web browser
2. No server required - runs entirely client-side

### Assistant Mode (Default)

Use this mode when playing the actual game and want solver assistance:

1. **Enter hints**: Click on the orange (row) or blue (column) hint panels to edit the sum and Voltorb count
2. **Reveal tiles**: As you flip tiles in the game, click on them here and select the revealed value
3. **Get suggestions**: Click "Solve" to calculate probabilities and get the best move suggestion
4. **View probabilities**: Covered tiles show a safety bar (green = safe, red = risky)
5. **Undo mistakes**: Click "Undo" to revert the last action

### Self-Play Mode

Watch the solver play automatically:

1. Click "Self-Play" to switch modes
2. Select a level (1-8)
3. Click "New Game" to generate a random board
4. Click "Play" to start auto-play, or click individual tiles to reveal them manually
5. Adjust speed with the slider

### Solver Settings

- **Timeout slider**: Adjust the search time limit (default 30s, max 120s)
- **Engine toggle**: Switch between JS (pure JavaScript) and WASM (WebAssembly) solver backends

## Probability Display

Each covered tile shows:
- **Safety bar**: Width indicates survival probability (green = safe, red = risky)
- **Hover text**: Exact Voltorb probability percentage
- **Yellow glow**: Suggested best move
- **Green border**: Guaranteed safe panel (0% Voltorb chance)

## Color Legend

- **Green**: Safe (0% Voltorb probability)
- **Yellow-green**: Low risk (<25% Voltorb)
- **Yellow**: Medium risk (25-50% Voltorb)
- **Red**: High risk (>50% Voltorb)

## Technical Details

### Architecture

The web GUI uses a **Web Worker** to run the solver off the main thread, keeping the UI responsive:

```
app.js (main thread)  <-->  solver-worker.js (Web Worker)
                                  |
                          JS solver (solver.js)
                                  or
                          WASM solver (solver-wasm.js + voltorb_wasm.wasm)
```

The worker posts `'progress'` messages at each search depth and a final `'complete'` message, enabling real-time updates in the UI.

### Files

- `index.html` - Main HTML structure
- `algorithm.html` - Algorithm documentation page
- `css/style.css` - Pixel-art CSS styling
- `css/docs.css` - Documentation page styling
- `js/boardTypes.js` - Board type definitions (80 types, N_accepted values)
- `js/board.js` - Board state management
- `js/solver.js` - JS probability calculation and solving logic
- `js/generator.js` - Random board generation
- `js/ui.js` - DOM manipulation and rendering
- `js/app.js` - Main application controller
- `js/solver-worker.js` - Web Worker dispatching JS and WASM solvers
- `js/solver-wasm.js` - Emscripten-generated WASM loader (ES module)
- `js/voltorb_wasm.wasm` - Compiled C++ solver (WebAssembly binary)
- `wasm/solver_bindings.cpp` - C++ Emscripten bindings (source)

### Algorithm

The solver uses Bayesian inference to calculate probabilities:

1. **Generate compatible boards**: Enumerate all valid board configurations matching the current state
2. **Type probability**: P(type | evidence) based on N_accepted values
3. **Panel probability**: P(panel = value | evidence) aggregated across all types
4. **Safe panel detection**: Panels that are never Voltorb in any compatible board
5. **Iterative deepening minimax**: Compute expected win probability for each panel choice using depth-limited expectimax search with memoization
6. **Result reporting**: Win probability labeled as exact (full tree explored) or approximate (depth N)

### Browser Compatibility

Requires a modern browser with ES6 module support:
- Chrome 61+
- Firefox 60+
- Safari 11+
- Edge 79+

## Development

No build process required for the JS frontend. Edit files and refresh the browser.

To serve locally (optional, for development):
```bash
cd docs
python3 -m http.server 8000
# Open http://localhost:8000
```

To rebuild the WASM module (requires Emscripten SDK):
```bash
source ~/emsdk/emsdk_env.sh
./build_wasm.sh
```

### Deployment

The `docs/` directory is served directly by GitHub Pages. Push to `main` to deploy.

## License

MIT License - see main project LICENSE file.
