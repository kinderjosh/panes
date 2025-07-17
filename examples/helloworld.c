// Compile this example with the following command:
// gcc helloworld.c -lncurses

// Just a single header file.
#include "../panes.h"

int main(void) {
    // Initialize panes.
    panes_init();

    // Create a maximum width and height pane with the title "Hello" and with a thin border.
    Pane *pane = create_pane(MAX_WIDTH, MAX_HEIGHT, 0, 0, "Example", BREAK_WITH_CTRL_C | THIN_BORDER);

    // Add "Hello, World!" into the pane.
    add_text(pane, CENTER, CENTER, "Hello, World!");
    update_pane(pane);

    // Wait for a keypress then delete the pane.
    await_keypress(pane);
    delete_pane(pane);

    // Stop panes.
    panes_end();
    return 0;
}
