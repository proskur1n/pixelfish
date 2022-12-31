Pixelfish is a small hobby-project pixel art editor written entirely in C. The core of the editor is written using the SDL2 library for window and input management, but it also requires GTK3 to display the file chooser and some of its dialogs. The look and feel of this editor is inspired by [csprite](https://github.com/pegvin/csprite).

![What? I am not an artist :/](https://raw.githubusercontent.com/proskur1n/pixelfish/master/showcase.png)

### Keybindings

| Key            | Description         |
|----------------|---------------------|
| `B`            | Round brush         |
| `Shift+B`      | Square brush        |
| `E`            | Eraser              |
| `Wheel` / `[`  | Decrease brush size |
| `Wheel` / `]`  | Increase brush size |
| `Alt`          | Color picker        |
| `G`            | Bucket fill         |
| `Ctrl+Wheel` / `+` / `-` | Zoom in / out |
| `Ctrl+Left`    | Pan                 |
| `Ctrl+Z`       | Undo                |
| `Ctrl+Y`       | Redo                |

Furthermore, many of the common key combinations such as `Ctrl-S` or `Ctrl-O` are also supported.

<!-- NOT IMPLEMENTED YET
### Configuration

This editor does not have a configuration file. Instead, you can set some of its global options with environment variables.

Also see `--help` for a list of supported command-line arguments. Note that program arguments have a higher priority than environment variables.

| Env               | Description                        |
|-------------------|------------------------------------|
| PIXELFISH_PALETTE | Path to a different Lospec palette |
| PIXELFISH_THEME   | Can be "light" or "dark"           |
-->

