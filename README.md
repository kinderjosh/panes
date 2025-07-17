# Panes

Panes is a (WIP) single-header TUI windowing library for C.

## Example

A simple hello world:

```c
// Compile this example with the following command:
// gcc -o example example.c -lncurses

// Just a single header file.
#include "panes.h"

int main(void) {
    // Initialize panes.
    panes_init();

    // Create a maximum width and height pane with the title "Example" and with a thin border.
    Pane *pane = create_pane(MAX_WIDTH, MAX_HEIGHT, 0, 0, "Example", BREAK_WITH_CTRL_C | THIN_BORDER);

    // Add "Hello, World!" into the center of the pane.
    add_text(pane, CENTER, CENTER, "Hello, World!");
    update_pane(pane);

    // Wait for a keypress then delete the pane.
    await_keypress(pane);
    delete_pane(pane);

    // Stop panes.
    panes_end();
    return 0;
}
```

<img src="./example.png">

## Usage

Firstly, make sure you have ```ncurses``` installed on your system.

Then, download the [header file](./panes.h) and include it into your project.

```c
#include "panes.h"
```

Your programs should always start with ```panes_init()``` to initialize the library, and end with ```panes_end()``` likewise.

Also, don't forget to link with ncurses by passing ```-lncurses``` to the end of your compilation command.

## Documentation

Coming soon.

## License

Panes is distributed under the [MIT](./LICENSE) license.