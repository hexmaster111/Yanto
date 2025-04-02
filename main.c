#include <raylib.h>
#include <raymath.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAB_SIZE 4
#define LINE_NUMBERS_SUPPORTED 3
#define DEFAULT_FONT_SIZE 12

Vector2 MeasureTextEx2(const char *text, size_t text_len);
void OnCurrsorLineChanged();

void FreeAllLines(char **lines, int count)
{
  for (int i = 0; i < count; i++)
  {
    free(lines[i]);
  }

  free(lines);
}

char **ReadAllLines(const char *fname, int *out_linecount)
{
  FILE *f = fopen(fname, "r");

  if (f == NULL)
    return NULL;

  int arraylen = 1024;
  int strlen = 1024;

  int lines_used = 0;
  int chars_read = 0;

  char **ret = malloc(sizeof(char *) * arraylen);
  ret[lines_used] = malloc(strlen);

  while (1)
  {
    if (feof(f))
    {
      // we are all done reading, finlise the last line
      ret[lines_used] =
          realloc(ret[lines_used], chars_read); // shrink string to be size read
      ret[lines_used][chars_read - 1] = 0;      // terminate the string
      lines_used++;
      break;
    }

    char c = fgetc(f);

    if (ferror(f))
    {
      perror("fgetc");
      return NULL;
    }

    if (chars_read > strlen)
    {
      strlen *= 2;
      ret[lines_used] = realloc(ret[lines_used], strlen);
    }

    if (c == '\n')
    {
      // store this line

      ret[lines_used] = realloc(
          ret[lines_used], chars_read + 1); // shrink string to be size read
      ret[lines_used][chars_read] = 0;      // terminate the string

      strlen = 1024; // reset var
      chars_read = 0;

      lines_used += 1;

      if (lines_used >= arraylen) // grow the continaing array
      {
        arraylen *= 2;
        ret = realloc(ret, arraylen * sizeof(char *));
      }
      ret[lines_used] = malloc(strlen); // alloc the next string
      continue;
    }
    else if (c == '\r' || c == '\n' && 0 >= chars_read)
    {
      // we just got a stray \r or \n
      continue;
    }

    ret[lines_used][chars_read] = c;
    chars_read += 1;
  }

  // resize the return array to fit used
  ret = realloc(ret, lines_used * sizeof(char *));

  *out_linecount = lines_used;
  return ret;
}

Font g_font;
float g_font_size, g_font_spacing;

struct Line
{
  char *base;
  size_t len, cap;
} *g_lines;
size_t g_lines_count;

int g_topline;
int g_screen_line_count;

int g_cursor_line, g_cursor_col;
char g_open_file_path[1024] = {0};

Camera2D g_x_scroll_view = {.zoom = 1}; // used to scroll left and right

void GrowString(struct Line *l)
{
  size_t new_cap = (l->cap + 1) * 2;

  char *new = realloc(l->base, new_cap);

  l->base = new;
  l->cap = new_cap;
}

void AppendLine()
{
  g_lines = realloc(g_lines, sizeof(struct Line) * (g_lines_count + 1));
  printf("%d\n", g_lines_count);
  g_lines[g_lines_count].base = 0;
  g_lines[g_lines_count].cap = 0;
  g_lines[g_lines_count].len = 0;
  g_lines_count += 1;
}

void InsertChar(char *str, size_t str_len, char ch, int pos)
{
  // Shift characters to the right
  for (int i = str_len; i >= pos; i--)
  {
    str[i + 1] = str[i];
  }

  // Insert the new character
  str[pos] = ch;
}

struct Line *InsertLine(size_t idx)
{
  // returns new line, reallocs g_lines, scoots over the existing lines, inserts new line
  struct Line new_line = {0};

  // Reallocate g_lines to make space for the new line
  g_lines = realloc(g_lines, sizeof(struct Line) * (g_lines_count + 1));

  // Shift lines after the insertion point
  for (size_t i = g_lines_count; i > idx; i--)
  {
    g_lines[i] = g_lines[i - 1];
  }

  // Insert the new line at the specified index
  g_lines[idx] = new_line;
  g_lines_count++;

  return &g_lines[idx];
}

void RemoveChar(char *str, size_t str_len, int pos)
{
  // Shift characters to the left
  for (int i = pos; i < str_len - 1; i++)
  {
    str[i] = str[i + 1];
  }
}

void BackspaceKeyPressed()
{
  struct Line *l = &g_lines[g_cursor_line];

  if (0 >= l->len)
  { // remove this line with no splicing and line shift lines up
    free(l->base);

    for (size_t i = g_cursor_line; i < g_lines_count - 1; i++)
    {
      g_lines[i] = g_lines[i + 1];
    }

    g_lines_count--;
    g_cursor_line -= 1;
    g_cursor_col = g_lines[g_cursor_line].len;
    OnCurrsorLineChanged();
  }
  else if (0 >= g_cursor_col && g_cursor_line > 0)
  { // Move the line up, and splice with the existing line

    struct Line *prev_line = &g_lines[g_cursor_line - 1];
    size_t currsor_return_pos = prev_line->len;
    size_t new_len = prev_line->len + l->len;
    if (new_len > prev_line->cap)
    {
      prev_line->cap = new_len;
      prev_line->base = realloc(prev_line->base, prev_line->cap);
    }

    memcpy(prev_line->base + prev_line->len, l->base, l->len);
    prev_line->len = new_len;

    free(l->base);

    for (size_t i = g_cursor_line; i < g_lines_count - 1; i++)
    {
      g_lines[i] = g_lines[i + 1];
    }

    g_lines_count--;
    g_cursor_line -= 1;
    g_cursor_col = currsor_return_pos;
    OnCurrsorLineChanged();
  }
  else
  {

    RemoveChar(l->base, l->len, g_cursor_col - 1);

    l->len -= 1;
    g_cursor_col -= 1;
  }
}

void DeleteKeyPressed()
{
  struct Line *l = &g_lines[g_cursor_line];

  if (0 >= l->len)
  { // remove this line with no splicing
    free(l->base);

    for (size_t i = g_cursor_line; i < g_lines_count - 1; i++)
    {
      g_lines[i] = g_lines[i + 1];
    }

    g_lines_count--;
    OnCurrsorLineChanged();
  }
  else if (l->len == g_cursor_col && g_cursor_line + 1 < g_lines_count)
  { // move the line below us up to the current line and splice

    struct Line *next_line = &g_lines[g_cursor_line + 1];
    size_t new_len = l->len + next_line->len;

    if (new_len > l->cap)
    {
      l->cap = new_len;
      l->base = realloc(l->base, l->cap);
    }

    memcpy(l->base + l->len, next_line->base, next_line->len);
    l->len = new_len;

    free(next_line->base);

    for (size_t i = g_cursor_line + 1; i < g_lines_count - 1; i++)
    {
      g_lines[i] = g_lines[i + 1];
    }

    g_lines_count--;
    OnCurrsorLineChanged();
  }
  else
  {
    RemoveChar(l->base, l->len, g_cursor_col);
    l->len -= 1;
  }
}

void EnterKeyPressed()
{
  if (0 >= g_cursor_col)
  { // at the start of the line, so we insert a new one at the pos and shift down
    InsertLine(g_cursor_line);
    g_cursor_line += 1;
  }
  else if (g_cursor_col >= g_lines[g_cursor_line].len)
  { // at the end of the line, so we insert below and move cursor down
    InsertLine(g_cursor_line + 1);
    g_cursor_line += 1;
  }
  else
  { // we are somewhere in the center of the line, so we need to split it

    struct Line *new_line = InsertLine(g_cursor_line + 1);
    struct Line *old_line = &g_lines[g_cursor_line];

    size_t new_line_len = old_line->len - g_cursor_col;

    new_line->base = calloc(new_line_len, sizeof(char));
    new_line->cap = new_line_len;
    new_line->len = new_line_len;

    memcpy(new_line->base, old_line->base + g_cursor_col, new_line_len);

    old_line->len = g_cursor_col;

    g_cursor_line += 1;
    g_cursor_col = 0;
  }

  OnCurrsorLineChanged();
}

void InsertCharAtCurrsor(char c)
{
  if (g_cursor_line >= g_lines_count)
  {
    AppendLine();
  }

  struct Line *l = &g_lines[g_cursor_line];

  if (l->len + 1 > l->cap)
  {
    GrowString(l);
  }

  InsertChar(l->base, l->len - 1, c, g_cursor_col);

  l->len += 1;
  g_cursor_col += 1;
}

void FDrawText(const char *text, float x, float y, Color color)
{
  DrawTextEx(g_font, text, (Vector2){x, y}, g_font_size, 1, color);
}

void LoadTextFile(const char *filepath)
{
  g_cursor_line = 0;
  g_cursor_col = 0;

  int linecount = 0;
  char **lines = ReadAllLines(filepath, &linecount);

  if (!linecount || !lines)
    return;

  g_lines = malloc(sizeof(struct Line) * linecount);
  if (!g_lines)
    return;

  g_lines_count = linecount;

  for (size_t i = 0; i < linecount; i++)
  {
    char *line = lines[i];
    g_lines[i].len = g_lines[i].cap = strlen(line);
    g_lines[i].base = line;
  }

  SetWindowTitle(TextFormat("%s - FCON", GetFileName(filepath)));
  memcpy(g_open_file_path, filepath, strlen(filepath));
}

void OnResize() { g_screen_line_count = (GetScreenHeight() / g_font_size) - 1; /*for status*/ }

void OnCurrsorLineChanged()
{
  if (g_topline + 5 > g_cursor_line)
  {
    g_topline = g_cursor_line - 5;
  }

  if ((g_topline + g_screen_line_count) - 5 < g_cursor_line)
  {
    g_topline = (g_cursor_line - g_screen_line_count) + 5;
  }
}

int main()
{
  InitWindow(800, 600, "FCON");
  SetTargetFPS(60);
  EnableEventWaiting();

  g_font = LoadFont("romulus.png");
  if (!IsFontValid(g_font))
  {
    TraceLog(LOG_WARNING, "Could not load font, using default.");
    g_font = GetFontDefault();
  }
  g_font_spacing = 1;

  g_font_size = DEFAULT_FONT_SIZE;
  g_topline = 0;
  g_lines_count = 0;
  g_lines = NULL;
  g_screen_line_count = 0;
  g_cursor_line = 0;
  g_cursor_col = 0;

  // LoadTextFile("test_small.fc");
  LoadTextFile("test.fc");
  // LoadTextFile("main.c");

  OnResize();

  unsigned redraw_count = 0;

  while (!WindowShouldClose())
  {
    redraw_count++;

    if (IsWindowResized())
    {
      OnResize();
    }

    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    {
      if (IsKeyPressed(KEY_S))
      {
        FILE *f = fopen(g_open_file_path, "w");
        if(!f) {
          perror(TextFormat("Error Opening %s for writing:", g_open_file_path));
          goto DONE_SAVING;
        }

        for (size_t i = 0; i < g_lines_count; i++)
        {
          fwrite(g_lines[i].base, sizeof(char), g_lines[i].len, f);
          fwrite("\n", sizeof(char), 1, f);
        }

        fclose(f);
        DONE_SAVING:
      }

      if (IsKeyPressed(KEY_EQUAL) || IsKeyPressedRepeat(KEY_EQUAL))
      {
        g_font_size += 1.0f;
        OnResize();
      }

      if (IsKeyPressed(KEY_MINUS) || IsKeyPressedRepeat(KEY_MINUS))
      {
        g_font_size -= 1.0f;
        OnResize();
      }

      if (IsKeyPressed(KEY_ZERO))
      {
        g_font_size = DEFAULT_FONT_SIZE;
        OnResize();
      }

      if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
      {
        g_topline -= 1;
      }

      if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
      {
        g_topline += 1;
      }
    }
    else
    {

      if (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP))
      {
        g_topline -= g_screen_line_count - 5;
        g_cursor_line -= g_screen_line_count - 5;
        OnCurrsorLineChanged();
      }

      if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN))
      {
        g_topline += g_screen_line_count - 5;
        g_cursor_line += g_screen_line_count - 5;
        OnCurrsorLineChanged();
      }

      if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
      {
        g_cursor_line -= 1;
        OnCurrsorLineChanged();
      }

      if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
      {
        g_cursor_line += 1;
        OnCurrsorLineChanged();
      }

      if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))
      {
        g_cursor_col -= 1;
      }

      if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))
      {
        g_cursor_col += 1;
      }

      if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
      {
        BackspaceKeyPressed();
      }

      if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE))
      {
        DeleteKeyPressed();
      }

      if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER))
      {
        EnterKeyPressed();
      }

      if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB))
      {
        for (size_t i = 0; i < TAB_SIZE; i++)
        {
          InsertCharAtCurrsor(' ');
        }
      }

      Vector2 scroll = GetMouseWheelMoveV();
      g_topline -= scroll.y * 2;

      if (g_topline + g_screen_line_count > g_lines_count)
      {
        g_topline = g_lines_count - g_screen_line_count;
      }

      if (0 > g_topline)
        g_topline = 0;

      g_x_scroll_view.offset.x += scroll.x * 30;
      if (g_x_scroll_view.offset.x > 0)
        g_x_scroll_view.offset.x = 0;

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
      {
        Vector2 pt = GetMousePosition();

        for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
        {
          Rectangle r = {g_font_size * LINE_NUMBERS_SUPPORTED - 1,
                         (i - g_topline) * g_font_size, GetScreenWidth(),
                         g_font_size};

          if (CheckCollisionPointRec(pt, r))
          {
            g_cursor_line = i;
            break;
          }
        }
      }

      int c;
      do
      {
        c = GetCharPressed();
        if (!c)
          break;

        InsertCharAtCurrsor(c);

      } while (c);
    }

    if (0 > g_topline)
      g_topline = 0;

    if (0 > g_cursor_line)
      g_cursor_line = 0;

    if (0 > g_cursor_col)
      g_cursor_col = 0;

    if (g_cursor_line >= g_lines_count)
      g_cursor_line = g_lines_count - 1;

    if (g_cursor_col > g_lines[g_cursor_line].len)
      g_cursor_col = g_lines[g_cursor_line].len;

    BeginDrawing();
    ClearBackground((Color){0xfd, 0xf6, 0xe3, 0xFF});
    BeginMode2D(g_x_scroll_view);

    { // Text Rendering
      for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
      {
        FDrawText(TextFormat("%*d", LINE_NUMBERS_SUPPORTED, i + 1), 3,
                  (i - g_topline) * g_font_size, BLACK);

        if (i == g_cursor_line)
        {
          DrawRectangle(g_font_size * LINE_NUMBERS_SUPPORTED - 1,
                        (i - g_topline) * g_font_size, GetScreenWidth(),
                        g_font_size, (Color){0xee, 0xe8, 0xd5, 0xFF});
        }

        if (i >= g_lines_count)
          break;
        FDrawText(TextFormat("%.*s", g_lines[i].len, g_lines[i].base),
                  g_font_size * LINE_NUMBERS_SUPPORTED - 1,
                  (i - g_topline) * g_font_size, BLACK);
      }
    }

    { // currsor rendering
      struct Line *l = &g_lines[g_cursor_line];
      Vector2 linelen = MeasureTextEx2(l->base, g_cursor_col);
      linelen.x += g_font_size * LINE_NUMBERS_SUPPORTED - 1;
      float cy = (g_cursor_line - g_topline) * g_font_size;
      DrawLineV((Vector2){linelen.x, cy}, (Vector2){linelen.x, cy + g_font_size},
                BLACK);
    }

    { // statusbar rendering
      int status_y = GetScreenHeight() - g_font_size;
      DrawRectangle(0, status_y, GetScreenWidth(), g_font_size, (Color){0xd8, 0xd4, 0xc4, 0xff});
    }

    // DrawText(TextFormat("TOP IDX: %d\n%u\n%f\n%f", g_topline, redraw_count, cy,
    //                     linelen.x),
    //          40, GetScreenHeight() - g_font_size * 4, g_font_size, PURPLE);

    EndMode2D();
    EndDrawing();
  }

  CloseWindow();

  return 0;
}

// Measure string size for Font
Vector2 MeasureTextEx2(const char *text, size_t text_len)
{
  Vector2 textSize = {0};

  if (((g_font.texture.id == 0)) || (text == NULL) || (text[0] == '\0'))
    return textSize; // Security check

  int tempByteCounter = 0; // Used to count longer text line num chars
  int byteCounter = 0;

  float textWidth = 0.0f;
  float tempTextWidth = 0.0f; // Used to count longer text line width

  float textHeight = g_font_size;
  float scaleFactor = g_font_size / (float)g_font.baseSize;

  int letter = 0; // Current character
  int index = 0;  // Index position in sprite font

  for (int i = 0; i < text_len;)
  {
    byteCounter++;

    int codepointByteCount = 0;
    letter = GetCodepointNext(&text[i], &codepointByteCount);
    index = GetGlyphIndex(g_font, letter);

    i += codepointByteCount;

    if (letter != '\n')
    {
      if (g_font.glyphs[index].advanceX > 0)
        textWidth += g_font.glyphs[index].advanceX;
      else
        textWidth += (g_font.recs[index].width + g_font.glyphs[index].offsetX);
    }
    else
    {
      if (tempTextWidth < textWidth)
        tempTextWidth = textWidth;
      byteCounter = 0;
      textWidth = 0;

      textHeight += (g_font_size + g_font_spacing);
    }

    if (tempByteCounter < byteCounter)
      tempByteCounter = byteCounter;
  }

  if (tempTextWidth < textWidth)
    tempTextWidth = textWidth;

  textSize.x = tempTextWidth * scaleFactor +
               (float)((tempByteCounter - 1) * g_font_spacing);
  textSize.y = textHeight;

  return textSize;
}