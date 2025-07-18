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

static void _assert_initialized() {
    if (!_initialized) {
        endwin();
        fprintf(stderr, "panes: panes_init() was never called\n");
        exit(1);
    }
}

// stdscr dimensions initialized in panes_init().
static int _maxy = 0;
static int _maxx = 0;

int _panes_maxy() {
    _assert_initialized();
    return _maxy;
}

int _panes_maxx() {
    _assert_initialized();
    return _maxx;
}

#define _CENTER_TEXT(width, text) (floor((width / 2) - ((int)strlen(text) / 2)) - 1)

// ---------- Macros ----------

#define MAX_WIDTH _panes_maxx()
#define MAX_HEIGHT _panes_maxy()

#define CENTER (123456789)

// ---------- Panes ----------

typedef struct {
    int width;
    int height;
    int start_y;
    int start_x;
    char *title;
    WINDOW *win;
    unsigned int flags;
} Pane;

// Pane flags
#define BREAK_WITH_CTRL_C (0x01)
#define SHOW_CURSOR (0x02)
#define NO_BORDER (0x04)
#define NO_TITLE (0x08)

#define BORDER_THICKNESS 1

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
    _assert_initialized();
    endwin();
    _initialized = false;
}

// Creates a new pane.
Pane *create_pane(int width, int height, int start_x, int start_y, char *title, unsigned int flags) {
    _assert_initialized();

    noecho();

    if (flags & BREAK_WITH_CTRL_C)
        cbreak();
    else
        raw();

    curs_set(flags & SHOW_CURSOR);

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
    wrefresh(win);

    p->flags = flags;
    return p;
}

// Deletes a pane.
void delete_pane(Pane *pane) {
    _assert_initialized();
    assert(pane != NULL);

    delwin(pane->win);
    free(pane->title);
    free(pane);
}

// Adds text to a pane.
void add_text(Pane *pane, int x, int y, char *text) {
    _assert_initialized();
    assert(pane != NULL);

    bool x_centered = false;
    bool y_centered = false;

    if (x == CENTER) {
        x = _CENTER_TEXT(pane->width, text);
        x_centered = true;
    }
    
    if (y == CENTER) {
        y = floor(pane->height / 2) - 1;
        y_centered = true;
    }

    if (!(pane->flags & NO_BORDER)) {
        if (!x_centered)
            x += BORDER_THICKNESS;

        if (!y_centered)
            y += BORDER_THICKNESS;
    }

    mvwaddstr(pane->win, y, x, text);
}

// Applies additions to a pane.
void update_pane(Pane *pane) {
    _assert_initialized();
    assert(pane != NULL);

    // Make sure the title and border is always there if added.

    if ((pane->flags & NO_BORDER))
        box(pane->win, 0, 0);

    if (!(pane->flags & NO_TITLE)) {
        wattron(pane->win, A_STANDOUT);
        mvwaddstr(pane->win, 0, _CENTER_TEXT(pane->width, pane->title), pane->title);
        wattroff(pane->win, A_STANDOUT);
    }

    wrefresh(pane->win);
}

// Waits a keypress on a pane.
int await_keypress(Pane *pane) {
    _assert_initialized();
    assert(pane != NULL);
    return wgetch(pane->win);
}

void move_cursor(Pane *pane, int x, int y) {
    _assert_initialized();
    assert(pane != NULL);

    bool x_centered = false;
    bool y_centered = false;

    if (x == CENTER) {
        x = floor(pane->width / 2);
        x_centered = true;
    }

    if (y == CENTER) {
        y = floor(pane->height / 2);
        x_centered = true;
    }

    if (!(pane->flags & NO_BORDER)) {
        if (!x_centered)
            x += BORDER_THICKNESS;

        if (!y_centered)
            y += BORDER_THICKNESS;
    }

    wmove(pane->win, y, x);
}

#endif