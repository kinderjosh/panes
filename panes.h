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

static bool _initialized = false;

/*
static void _assert_initialized() {
    if (!_initialized) {
        endwin();
        fprintf(stderr, "panes: panes_init() was never called\n");
        exit(1);
    }
}
*/

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

// ---------- Macros ----------

#define MAX_WIDTH _panes_maxx()
#define MAX_HEIGHT _panes_maxy()

#define CENTER (123456789)

// ---------- Widgets ----------

typedef enum {
    WIDGET_LABEL
} WidgetType;

typedef struct {
    WidgetType type;
    int width;
    int height;
    int x;
    int y;

    union {
        struct {
            char *text;
        } label;
    };
} Widget;

typedef Widget Label;

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
} Pane;

// Pane flags
#define BREAK_WITH_CTRL_C (0x01)
#define SHOW_CURSOR (0x02)
#define NO_BORDER (0x04)
#define NO_TITLE (0x08)
#define SHOW_KEYPRESSES (0x16)

#define BORDER_THICKNESS 1
#define STARTING_WIDGET_CAPACITY 8

// Initializes the library.
void panes_init() {
    _initialized = true;
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
    _initialized = false;
}

// Creates a new label widget.
Label *create_label(int x, int y, char *text) {
    assert(text != NULL);

    Label *label = malloc(sizeof(Label));
    label->type = WIDGET_LABEL;
    label->width = 0;
    label->height = 0;
    label->x = x;
    label->y = y;
    label->label.text = malloc(strlen(text) + 1);
    strcpy(label->label.text, text);
    return label;
}

// Deletes a widget.
void delete_widget(Widget *widget) {
    assert(widget != NULL);

    switch (widget->type) {
        case WIDGET_LABEL:
            free(widget->label.text);
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

        wattron(win, A_STANDOUT);
        mvwaddstr(win, 0, _CENTER_TEXT(width, title), title);
        wattroff(win, A_STANDOUT);
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

// Places a label onto a pane.
static void _place_label(Pane *pane, Label *label) {
    int x = label->x;
    int y = label->y;

    bool x_centered = false;
    bool y_centered = false;

    if (label->x == CENTER) {
        x = _CENTER_TEXT(pane->width, label->label.text);
        x_centered = true;
    }

    if (label->y == CENTER) {
        y = _CENTER_Y(pane->height);
        y_centered = true;
    }

    if (!(pane->flags & NO_BORDER)) {
        if (!x_centered)
            x += BORDER_THICKNESS;

        if (!y_centered)
            y += BORDER_THICKNESS;
    }

    mvwaddstr(pane->win, pane->start_y + y, pane->start_x + x, label->label.text);
}

// Draws all packed widgets onto a pane.
void update_pane(Pane *pane) {
    assert(pane != NULL);

    // Make sure the title and border is always there if added.
    if ((pane->flags & NO_BORDER))
        box(pane->win, 0, 0);

    if (!(pane->flags & NO_TITLE)) {
        wattron(pane->win, A_STANDOUT);
        mvwaddstr(pane->win, 0, _CENTER_TEXT(pane->width, pane->title), pane->title);
        wattroff(pane->win, A_STANDOUT);
    }

    // Show the widgets.
    for (size_t i = 0; i < pane->widget_count; i++) {
        switch (pane->widgets[i]->type) {
            case WIDGET_LABEL:
                _place_label(pane, pane->widgets[i]);
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

#endif