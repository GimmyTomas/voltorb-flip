# Voltorb Flip Web GUI

A web-based GUI for the Voltorb Flip solver with pixel-art styling, live probability display, and both assistant and self-play modes.

## Features

- **Pixel-art styled GUI** matching the original game
- **Assistant mode**: Enter hints, click to reveal, get solver suggestions
- **Self-play demo**: Watch the solver play automatically
- **Live probabilities**: Show P(Voltorb) for each panel with color-coded bars
- **Win probability** estimate display
- **Safe panel detection**: Guaranteed-safe panels highlighted in green
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

### Files

- `index.html` - Main HTML structure
- `css/style.css` - Pixel-art CSS styling
- `js/boardTypes.js` - Board type definitions (80 types, N_accepted values)
- `js/board.js` - Board state management
- `js/solver.js` - Probability calculation and solving logic
- `js/generator.js` - Random board generation
- `js/ui.js` - DOM manipulation and rendering
- `js/app.js` - Main application controller

### Algorithm

The solver uses Bayesian inference to calculate probabilities:

1. **Generate compatible boards**: Enumerate all valid board configurations matching the current state
2. **Type probability**: P(type | evidence) based on N_accepted values
3. **Panel probability**: P(panel = value | evidence) aggregated across all types
4. **Safe panel detection**: Panels that are never Voltorb in any compatible board
5. **Best move selection**: Prefer safe panels, otherwise lowest Voltorb probability

### Browser Compatibility

Requires a modern browser with ES6 module support:
- Chrome 61+
- Firefox 60+
- Safari 11+
- Edge 79+

## Development

No build process required. Edit files and refresh the browser.

To serve locally (optional, for development):
```bash
cd web
python3 -m http.server 8000
# Open http://localhost:8000
```

## License

MIT License - see main project LICENSE file.
