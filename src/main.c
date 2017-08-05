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
  bool quit = false;
  int exitCode = EXIT_SUCCESS;
  while (!quit) {
    int inputCode = getch();
    if (inputCode >= ' ' && inputCode <= '~') {
      char charCode = (char)inputCode;
      bool err = insertText(data, 0, 0, &charCode, 1);

      if (err) {
        fprintf(stderr, "Error handling event.\n");
        exitCode = EXIT_FAILURE;
        quit = true;
      } else {
        redrawEditor(data);
        move(0, 0);
      }
    }
  }

  // Clean up.
  delwin(data->window);
  endwin();
  refresh();

  return exitCode;
}
