// Copyright (c) 2017, Herman Bergwerf. All rights reserved.
// Use of this source code is governed by a MIT-style license
// that can be found in the LICENSE file.

#include <curses.h>
#include <stdio.h>
#include <string.h>

/// All editor data.
typedef struct {
  WINDOW *window;

  /// File path specified at startup.
  char *filePath;

  /// Array of NULL-terminated text strings containing each line.
  /// Note: this should probably be a linked list.
  char **textBuffer;

  /// Total number of lines.
  int lineCount;

  /// Current scrolling offset.
  int xOffset;
  int yOffset;
} EditorData;

int getLineLength(EditorData *data, int row) {
  return strlen(data->textBuffer[row]);
}

/// Not the most efficent, but for now the easiest approach.
void redrawEditor(EditorData *data) {
  // Get dimensions and erase.
  int w = 0, h = 0;
  getmaxyx(data->window, h, w);  // Note: this is an ugly dirty macro.
  erase();

  // Draw each line.
  for (int i = 0; i < h && i + data->xOffset < data->lineCount; i++) {
    char *lineText = data->textBuffer[i];
    int lineLength = strlen(lineText);
    mvaddnstr(i, 0, lineText, lineLength > w ? w : lineLength);
  }

  // Refresh screen.
  refresh();
}

/// Insert newline after the given row in the buffer. Returns true on error.
bool insertLine(EditorData *data, int row) {
  if (data->lineCount <= row || row < -1) return true;

  // Reallocate array of line pointers.
  int newLineCount = data->lineCount + 1;
  char **textBuffer = (char **)malloc(newLineCount * sizeof(char *));

  // Move over all old pointers.
  int offset = 0;
  for (int i = 0; i < newLineCount; i++) {
    if (i == row + 1) {
      char *emptyLine = (char *)malloc(1);
      emptyLine[0] = 0;
      textBuffer[i] = emptyLine;
      offset = 1;
    } else {
      textBuffer[i] = data->textBuffer[i - offset];
    }
  }

  // Swap and free.
  free(data->textBuffer);
  data->textBuffer = textBuffer;
  data->lineCount = newLineCount;

  return false;
}

/// Insert the given fragment of length n at the given position in the text
/// buffer in the given editor. Returns true on error.
bool insertText(EditorData *data, int row, int col, char *fragment, int n) {
  if (data->lineCount <= row) return true;

  char *oldLine = data->textBuffer[row];
  int oldLineLength = strlen(oldLine);
  if (oldLineLength < col) return true;

  // Reallocate the given line and copy the old line while inserting fragment.
  char *newLine = (char *)malloc(oldLineLength + n + 1);
  strncpy(newLine, oldLine, col);
  strncpy(newLine + col, fragment, n);
  strncpy(newLine + col + n, oldLine + col, oldLineLength - col);

  // Ensure the string is NULL-terminated.
  newLine[oldLineLength + n] = 0;

  // Swap and free.
  data->textBuffer[row] = newLine;
  free(oldLine);

  return false;
}

/// Delete n characters from the given position in the text buffer going
/// backward. Returns true on error.
bool removeText(EditorData *data, int row, int col, int n) {
  if (data->lineCount <= row) return true;
  if (col < n) return true;

  char *oldLine = data->textBuffer[row];
  int oldLineLength = strlen(oldLine);
  if (oldLineLength < col) return true;

  // Reallocate the given line and copy parts of the old line.
  char *newLine = (char *)malloc(oldLineLength - n + 1);
  strncpy(newLine, oldLine, col - n);
  strncpy(newLine + col - n, oldLine + col, oldLineLength - col);

  // Ensure the string is NULL-terminated.
  newLine[oldLineLength - n] = 0;

  // Swap and free.
  data->textBuffer[row] = newLine;
  free(oldLine);

  return false;
}

/// Save editor text to given file.
bool saveAsFile(EditorData *data) {
  FILE *file;
  if (file = fopen(data->filePath, "w")) {
    for (int i = 0; i < data->lineCount; i++) {
      char *lineText = data->textBuffer[i];
      fwrite(lineText, 1, strlen(lineText), file);
      fputc('\n', file);
    }
    fclose(file);
    return false;
  } else {
    return true;
  }
}

/// Restore editor text from file.
bool restoreFromFile(EditorData *data) {
  FILE *file;
  if (file = fopen(data->filePath, "r")) {
    // Obtain file size.
    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    rewind(file);

    // Allocate memory to contain the whole file.
    char *buffer = (char *)malloc(fileSize);

    // Copy the file into the buffer.
    int result = fread(buffer, 1, fileSize, file);
    fclose(file);

    // Split into lines.
    if (result != fileSize) {
      free(buffer);
      return true;
    } else {
      // Count lines.
      int lineCount = 0;
      for (int i = 0; i < fileSize; i++) {
        // TODO: this is not very nice, now the buffer cannot contain 0
        // characters.
        if (buffer[i] == '\n') {
          lineCount++;
        }
      }

      // Allocate line array.
      data->lineCount = lineCount;
      data->textBuffer = (char **)malloc(lineCount * sizeof(char *));

      // Copy data to lines.
      // It is neccesary to allocate new buffers or else they cannot be freed
      // seperately.
      int lineIndex = 0, lineLength = 0;
      for (int i = 0; i < fileSize; i++) {
        if (buffer[i] == '\n') {
          // Clone string.
          char *line = (char *)malloc(lineLength + 1);
          strncpy(line, (char *)(buffer + i - lineLength), lineLength);
          line[lineLength] = 0;

          // Store in buffer and move on.
          data->textBuffer[lineIndex] = line;
          lineIndex++;
          lineLength = 0;
        } else {
          lineLength++;
        }
      }
      free(buffer);

      return false;
    }
  } else {
    return true;
  }
}
