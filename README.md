Pixelfish is a work-in-progress pixel art editor.

### Keybindings

| Key        | Description         |
|------------|---------------------|
| B          | Square brush        |
| Shift+B    | Round brush         |
| E          | Eraser              |
| [          | Decrease brush size |
| ]          | Increase brush size |
| Alt        | Color picker        |
| G          | Bucket fill         |
| Ctrl+Wheel | Zoom in / out       |
| Ctrl+Left  | Pan                 |

### Configuration

This editor does not have a configuration file. Instead, you can set some of its permanent options with environment variables.

Also see `--help` for a list of supported command-line arguments. Note that program arguments have a higher priority than environment variables.

| Env               | Description                        |
|-------------------|------------------------------------|
| PIXELFISH_PALETTE | Path to a different Lospec palette |
| PIXELFISH_THEME   | Can be "light" or "dark"          |
