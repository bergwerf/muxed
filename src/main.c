// Copyright (c) 2017, Herman Bergwerf. All rights reserved.
// Use of this source code is governed by a MIT-style license
// that can be found in the LICENSE file.

#include <curses.h>
#include <stdlib.h>

#include "editor.c"

#ifndef CTRL
#define CTRL(c) ((c)&037)
#endif

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Please specify a file path.\n");
    exit(EXIT_FAILURE);
  }

  bool err = false;
  EditorData *data;
  data = (EditorData *)malloc(sizeof(EditorData));
  data->filePath = argv[1];

  // Initialize editor window.
  if ((data->window = initscr()) == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    exit(EXIT_FAILURE);
  }

  noecho();                    // Turn off key echoing
  keypad(data->window, TRUE);  // Enable the keypad for non-char keys

  // Try to read file.
  err = restoreFromFile(data);
  if (err) {
    // Add initial newline.
    insertLine(data, -1);
  } else {
    redrawEditor(data);
    move(0, 0);
  }

  // Keyboard event loop.
  int cursorRow = 0, cursorCol = 0;
  bool quit = false, redraw = false;
  int exitCode = EXIT_SUCCESS;

  while (!quit) {
    int inputCode = getch();
    if (inputCode >= ' ' && inputCode <= '~') {
      char charCode = (char)inputCode;
      err = insertText(data, cursorRow, cursorCol, &charCode, 1);
      cursorCol++;
      redraw = true;
    } else {
      int lineLength;
      // TODO: Remove redundancy from arrow key navigation and insert/delete.
      switch (inputCode) {
        case KEY_LEFT:
          if (cursorCol == 0) {
            if (cursorRow > 0) {
              cursorRow--;
              cursorCol = getLineLength(data, cursorRow);
            }
          } else {
            cursorCol--;
          }
          break;

        case KEY_RIGHT:
          lineLength = getLineLength(data, cursorRow);
          if (cursorCol == lineLength) {
            if (cursorRow + 1 < data->lineCount) {
              cursorRow++;
              cursorCol = 0;
            }
          } else {
            cursorCol++;
          }
          break;

        case KEY_UP:
          if (cursorRow > 0) {
            cursorRow--;
            lineLength = getLineLength(data, cursorRow);
            if (lineLength < cursorCol) {
              cursorCol = lineLength;
            }
          }
          break;

        case KEY_DOWN:
          if (cursorRow + 1 < data->lineCount) {
            cursorRow++;
            lineLength = getLineLength(data, cursorRow);
            if (lineLength < cursorCol) {
              cursorCol = lineLength;
            }
          }
          break;

        case KEY_BACKSPACE:
          if (cursorCol == 0) {
            if (cursorRow > 0) {
              cursorRow--;
              cursorCol = getLineLength(data, cursorRow);
            }
          } else {
            err = removeText(data, cursorRow, cursorCol, 1);
            redraw = true;
            cursorCol--;
          }
          break;

        case '\n':
          err = insertLine(data, cursorRow);
          cursorRow++;
          cursorCol = 0;
          redraw = true;
          break;

        case CTRL('x'):
          err = saveAsFile(data);
          break;
      }
    }

    int w = 0, h = 0;
    getmaxyx(data->window, h, w);

    // Update scrolling.
    // TODO: Remove redundancy.
    int rCursorRow = cursorRow - data->rowOffset;
    int rCursorCol = cursorCol - data->colOffset;

    if (rCursorRow >= h) {
      data->rowOffset += rCursorRow - h + 1;
      redraw = true;
    } else if (rCursorRow < 0 && data->rowOffset > 0) {
      data->rowOffset += rCursorRow;
      redraw = true;
    }

    if (rCursorCol >= w) {
      data->colOffset += rCursorCol - w + 1;
      redraw = true;
    } else if (rCursorCol < 0 && data->colOffset > 0) {
      data->colOffset += rCursorCol;
      redraw = true;
    }

    // Redraw lines.
    if (redraw) {
      redrawEditor(data);
      redraw = false;
    }

    // Update cursor.
    rCursorRow = cursorRow - data->rowOffset;
    rCursorCol = cursorCol - data->colOffset;
    move(rCursorRow, rCursorCol);

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
