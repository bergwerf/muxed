// Copyright (c) 2017, Herman Bergwerf. All rights reserved.
// Use of this source code is governed by a MIT-style license
// that can be found in the LICENSE file.

#include <curses.h>
#include <stdlib.h>

#include "editor.c"

int main(int argc, char *argv[]) {
  EditorData *data;
  data = (EditorData *)malloc(sizeof(EditorData));

  // Initialize editor window.
  if ((data->window = initscr()) == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(EXIT_FAILURE);
  }

  noecho();                    // Turn off key echoing
  keypad(data->window, TRUE);  // Enable the keypad for non-char keys

  // Add initial newline.
  insertLine(data, -1);

  // Keyboard event loop.
  int cursorRow = 0, cursorCol = 0;
  bool quit = false, err = false, redraw = false;
  int exitCode = EXIT_SUCCESS;
  while (!quit) {
    int inputCode = getch();
    if (inputCode >= ' ' && inputCode <= '~') {
      char charCode = (char)inputCode;
      err = insertText(data, cursorRow, cursorCol, &charCode, 1);
      cursorCol++;
      redraw = true;
    } else {
      // TODO: wrap around!
      switch (inputCode) {
        case KEY_LEFT:
          cursorCol--;
          break;
        case KEY_RIGHT:
          cursorCol++;
          break;
        case KEY_BACKSPACE:
          err = removeText(data, cursorRow, cursorCol, 1);
          redraw = true;
          cursorCol--;
          break;
        case '\n':
          err = insertLine(data, cursorRow);
          cursorRow++;
          cursorCol = 0;
          break;
      }
    }

    if (redraw) {
      redrawEditor(data);
      redraw = false;
    }

    int w = 0, h = 0;
    getmaxyx(data->window, h, w);
    move(cursorRow, cursorCol > w - 1 ? w - 1 : cursorCol);

    if (err) {
      fprintf(stderr, "Error occured.\n");
      exitCode = EXIT_FAILURE;
      quit = true;
    }
  }

  // Clean up.
  delwin(data->window);
  endwin();
  refresh();

  return exitCode;
}
