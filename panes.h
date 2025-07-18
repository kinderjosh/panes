// Copyright (c) 2025 Joshua Kinder <https://github.com/kinderjosh>

// Functions and variables that begin with '_' are intended for internal use only.

#ifndef PANES_H
#define PANES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <ncurses.h>

// ---------- Internal utilities ----------

// stdscr dimensions initialized in panes_init().
static int _maxy = 0;
static int _maxx = 0;

int _panes_maxy() {
    return _maxy;
}

int _panes_maxx() {
    return _maxx;
}

#define _CENTER_OFFSET 0 // Sometimes text appears off center, might need this later?
#define _CENTER_TEXT(width, text) (floor((width / 2) - ((int)strlen(text) / 2) - _CENTER_OFFSET))
#define _CENTER_POS(pane_size, widget_size) (floor((pane_size / 2) - (widget_size / 2) - _CENTER_OFFSET))
#define _CENTER_X(width) (floor(width / 2 - _CENTER_OFFSET))
#define _CENTER_Y(height) (floor(height / 2 - _CENTER_OFFSET))

#define _TITLE_ATTR A_STANDOUT
#define _ENTRY_ATTR A_UNDERLINE

// ---------- Macros ----------

#define MAX_WIDTH _panes_maxx()
#define MAX_HEIGHT _panes_maxy()

#define CENTER (123456789)

// ---------- Widgets ----------

typedef enum {
    WIDGET_LABEL,
    WIDGET_ENTRY
} WidgetType;

typedef struct {
    WidgetType type;
    int x;
    int y;

    union {
        struct {
            char *text;
        } label;

        struct {
            int width;
            int height;
            char *placeholder_text;
            char *buffer;
            size_t capacity;
        } entry;
    };
} Widget;

typedef Widget Label;
typedef Widget Entry;

// ---------- Panes ----------

typedef struct {
    int width;
    int height;
    int start_y;
    int start_x;
    char *title;
    WINDOW *win;
    unsigned int flags;
    Widget **widgets;
    size_t widget_count;
    size_t widget_capacity;
    size_t cur_widget;
} Pane;

// Pane flags
#define BREAK_WITH_CTRL_C (0x01)
#define SHOW_CURSOR (0x02)
#define NO_BORDER (0x04)
#define NO_TITLE (0x08)
#define SHOW_KEYPRESSES (0x10)

#define BORDER_THICKNESS 1
#define STARTING_WIDGET_CAPACITY 8

// Initializes the library.
void panes_init() {
    initscr();

    if (has_colors()) {
        start_color();
        use_default_colors();
    }

    getmaxyx(stdscr, _maxy, _maxx);
}

// Ends the library.
void panes_end() {
    endwin();
}

// Creates a new label widget.
Label *create_label(int x, int y, char *text) {
    assert(text != NULL);

    Label *label = malloc(sizeof(Label));
    label->type = WIDGET_LABEL;
    label->x = x;
    label->y = y;
    label->label.text = malloc(strlen(text) + 1);
    strcpy(label->label.text, text);
    return label;
}

// Creates a new entry widget.
Entry *create_entry(int width, int height, int x, int y, char *placeholder_text, char *buffer, size_t capacity) {
    assert(buffer != NULL);

    Entry *entry = malloc(sizeof(Entry));
    entry->type = WIDGET_ENTRY;
    entry->x = x;
    entry->y = y;
    entry->entry.width = width;
    entry->entry.height = height;

    if (placeholder_text == NULL)
        entry->entry.placeholder_text = calloc(1, sizeof(char));
    else {
        entry->entry.placeholder_text = malloc(strlen(placeholder_text) + 1);
        strcpy(entry->entry.placeholder_text, placeholder_text);
    }

    entry->entry.buffer = buffer;
    entry->entry.capacity = capacity;
    return entry;
}

// Deletes a widget.
void delete_widget(Widget *widget) {
    assert(widget != NULL);

    switch (widget->type) {
        case WIDGET_LABEL:
            free(widget->label.text);
            break;
        case WIDGET_ENTRY:
            free(widget->entry.placeholder_text);
            break;
        default: assert(false);
    }

    free(widget);
}

// Packs a widget into a pane to be drawn on update_pane().
void pack_widget(Pane *pane, Widget *widget) {
    assert(pane != NULL);
    assert(widget != NULL);

    if (pane->widget_count + 1 >= pane->widget_capacity) {
        pane->widget_capacity *= 2;
        pane->widgets = malloc(pane->widget_capacity * sizeof(Widget *));
    }

    pane->widgets[pane->widget_count++] = widget;
}

// Creates a new pane.
Pane *create_pane(int width, int height, int start_x, int start_y, char *title, unsigned int flags) {
    if (flags & SHOW_KEYPRESSES)
        echo();
    else
        noecho();

    if (flags & BREAK_WITH_CTRL_C)
        cbreak();
    else
        raw();

    curs_set(flags & SHOW_CURSOR);

    if (start_x == CENTER)
        start_x = _CENTER_POS(_maxx, width);

    if (start_y == CENTER)
        start_y = _CENTER_POS(_maxy, height);

    WINDOW *win = newwin(height, width, start_y, start_x);

    if (win == NULL) {
        endwin();
        fprintf(stderr, "panes: create_pane: failed to create new window\n");
        exit(1);
    }

    if (!(flags & NO_BORDER))
        box(win, 0, 0);

    Pane *p = malloc(sizeof(Pane));
    assert(p != NULL);
    p->width = width;
    p->height = height;
    p->start_x = start_x;
    p->start_y = start_y;

    if (flags & NO_TITLE)
        p->title = calloc(1, sizeof(char));
    else {
        assert(title != NULL);
        p->title = malloc(strlen(title) + 1);
        strcpy(p->title, title);

        wattron(win, _TITLE_ATTR);
        mvwaddstr(win, 0, _CENTER_TEXT(width, title), title);
        wattroff(win, _TITLE_ATTR);
    }

    p->win = win;
    p->flags = flags;
    p->widgets = malloc(STARTING_WIDGET_CAPACITY * sizeof(Widget *));
    p->widget_count = 0;
    p->widget_capacity = STARTING_WIDGET_CAPACITY;
    return p;
}

// Deletes a pane.
void delete_pane(Pane *pane) {
    assert(pane != NULL);

    for (size_t i = 0; i < pane->widget_count; i++)
        delete_widget(pane->widgets[i]);

    free(pane->widgets);
    free(pane->title);
    delwin(pane->win);
    free(pane);
}

// Calculates where on the actual terminal screen to place widgets inside the panes, border included.
// Also calculates the position to place CENTER.
static void _widget_pos_to_screen_pos(Pane *pane, Widget *widget, int *x, int *y) {
    assert(widget != NULL);
    assert(x != NULL);
    assert(y != NULL);
    
    *x = widget->x;
    *y = widget->y;

    bool x_centered = false;
    bool y_centered = false;

    if (widget->x == CENTER) {
        switch (widget->type) {
            case WIDGET_LABEL:
                *x = _CENTER_TEXT(pane->width, widget->label.text);
                break;
            case WIDGET_ENTRY:
                *x = _CENTER_POS(pane->width, widget->entry.width);
                break;
            default: assert(false);
        }

        x_centered = true;
    }

    if (widget->y == CENTER) {
        switch (widget->type) {
            case WIDGET_LABEL:
                *y = _CENTER_Y(pane->height);
                break;
            case WIDGET_ENTRY:
                *y = _CENTER_POS(pane->height, widget->entry.height);
                break;
        }

        y_centered = true;
    }

    if (!(pane->flags & NO_BORDER)) {
        if (!x_centered)
            *x += BORDER_THICKNESS;

        if (!y_centered)
            *y += BORDER_THICKNESS;
    }

    *x += pane->start_x;
    *y += pane->start_y;
}

// Places a label onto a pane.
static void _place_label(Pane *pane, Label *label) {
    assert(pane != NULL);
    assert(label != NULL);

    int x;
    int y;
    _widget_pos_to_screen_pos(pane, label, &x, &y);
    mvwaddstr(pane->win, y, x, label->label.text);
}

// Places an entry onto a pane.
static void _place_entry(Pane *pane, Entry *entry) {
    assert(pane != NULL);
    assert(entry != NULL);

    int x;
    int y;
    _widget_pos_to_screen_pos(pane, entry, &x, &y);

    wattron(pane->win, _ENTRY_ATTR);
    const size_t len = strlen(entry->entry.placeholder_text);

    for (int i = 0; i < entry->entry.width; i++)
        mvwaddch(pane->win, y, x + i, (size_t)i >= len ? ' ' : entry->entry.placeholder_text[i]);

    wattroff(pane->win, _ENTRY_ATTR);
}

// Draws all packed widgets onto a pane.
void update_pane(Pane *pane) {
    assert(pane != NULL);

    // Make sure the title and border is always there if added.
    if ((pane->flags & NO_BORDER))
        box(pane->win, 0, 0);

    if (!(pane->flags & NO_TITLE)) {
        wattron(pane->win, _TITLE_ATTR);
        mvwaddstr(pane->win, 0, _CENTER_TEXT(pane->width, pane->title), pane->title);
        wattroff(pane->win, _TITLE_ATTR);
    }

    // Show the widgets.
    for (size_t i = 0; i < pane->widget_count; i++) {
        switch (pane->widgets[i]->type) {
            case WIDGET_LABEL:
                _place_label(pane, pane->widgets[i]);
                break;
            case WIDGET_ENTRY:
                _place_entry(pane, pane->widgets[i]);
                break;
            default: assert(false);
        }
    }

    wrefresh(pane->win);
}

// Waits for a key to be pressed on a pane.
int await_keypress(Pane *pane) {
    assert(pane != NULL);
    return wgetch(pane->win);
}

// Moves the cursor to the position of a widget on a pane.
void move_cursor_to_widget(Pane *pane, Widget *widget) {
    assert(pane != NULL);
    assert(widget != NULL);

    int x;
    int y;
    _widget_pos_to_screen_pos(pane, widget, &x, &y);

    wmove(pane->win, y, x);
    wrefresh(pane->win);
}

#endif