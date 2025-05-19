#include <raylib.h>
#include <raymath.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <ctype.h>

#include "osfd.c"
#include "interactivity.c"

#define TAB_SIZE 4
#define LINE_NUMBERS_SUPPORTED 3
#define DEFAULT_FONT_SIZE 16
#define INIT_LINE_COUNT 5
#define BACKGROUND_COLOR ((Color){0xfd, 0xf6, 0xe3, 0xff})
#define rgb(COLORR, COLORG, COLORB) ((Color){COLORR, COLORG, COLORB, 0xff})

#define COLOR_COMMENT (C_Gray)
#define COLOR_TYPE (C_Brick)
#define COLOR_PREPROC (C_Purple)
#define COLOR_FLOWCTRL (C_Green)
#define COLOR_STRCHAR (C_Teal)
#define COLOR_MODIFYER (C_Blue)
#define COLOR_COMMENT_NOTICE (C_Red)
#define COLOR_DEFAULT (C_Black)
#define COLOR_KEYWORD (C_Blue)

Vector2 MeasureTextEx2(const char *text, size_t text_len);
void OnCurrsorLineChanged();

void FreeAllLines(char **lines, int count);
char **ReadAllLines(const char *fname, int *out_linecount);

Font g_font;
float g_font_size, g_font_spacing;

enum
{
  C_Black = 0,
  C_Brick = 1,
  C_Blue = 2,
  C_Green = 3,
  C_Purple = 4,
  C_Gray = 5,
  C_Teal = 6,
  C_Red = 7
};

struct Line
{
  uint8_t *style;
  char *base;
  size_t len, cap;
} *g_lines;

size_t g_lines_count;

int g_topline;
int g_screen_line_count;

int g_cursor_line, g_cursor_col;
char g_open_file_path[1024] = {0};

bool g_file_changed = false;

enum Syntax
{
  S_PlainText,
  S_C,
  S_Verilog
} g_open_file_syntax;

bool HasUnsavedChanges() { return g_file_changed; }

Camera2D g_x_scroll_view = {.zoom = 1}; // used to scroll left and right

void GrowString(struct Line *l)
{
  size_t new_cap = (l->cap + 1) * 2;
  l->base = realloc(l->base, new_cap);
  l->style = realloc(l->style, new_cap);
  l->cap = new_cap;
}

void AppendLine()
{
  g_lines = realloc(g_lines, sizeof(struct Line) * (g_lines_count + 1));
  printf("%zu\n", g_lines_count);
  g_lines[g_lines_count].base = 0;
  g_lines[g_lines_count].style = 0;
  g_lines[g_lines_count].cap = 0;
  g_lines[g_lines_count].len = 0;
  g_lines_count += 1;
}

void DoTxtSyntaxHighlighting()
{
  for (size_t i = 0; i < g_lines_count; i++)
  {
    struct Line l = g_lines[i];

    if (l.style)
    {
      memset(l.style, C_Black, l.len);
    }
  }
}

#include "c_syntax_hl.c"
void DoSyntaxHighlighting()
{
  if (!g_lines)
    return;

  switch (g_open_file_syntax)
  {
    // clang-format off
      case S_C: DoCSyntaxHighlighing(); break;
      case S_Verilog: break;
      case S_PlainText: DoTxtSyntaxHighlighting(); break;
    // clang-format on
  }
}

void InsertChar(char *str, size_t str_len, char ch, int pos)
{ // Shift characters to the right
  for (int i = str_len; i >= pos; i--)
  {
    str[i + 1] = str[i];
  }

  // Insert the new character
  str[pos] = ch;
}

struct Line *InsertLine(size_t idx)
{ // returns new line, reallocs g_lines, scoots over the existing lines, inserts new line
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
{ // Shift characters to the left
  for (int i = pos; i < str_len - 1; i++)
  {
    str[i] = str[i + 1];
  }
}

void BackspaceKeyPressed()
{
  struct Line *l = &g_lines[g_cursor_line];

  if (0 >= l->len && 0 < g_cursor_line)
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
    g_file_changed = true;
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
  else if (0 < g_cursor_col)
  {
    RemoveChar(l->base, l->len, g_cursor_col - 1);

    l->len -= 1;
    g_cursor_col -= 1;
  }

  DoSyntaxHighlighting();
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
    g_file_changed = true;
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

    g_lines_count -= 1;
    OnCurrsorLineChanged();
  }
  else
  {
    RemoveChar(l->base, l->len, g_cursor_col);
    l->len -= 1;
  }

  DoSyntaxHighlighting();
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
  g_file_changed = true;

  DoSyntaxHighlighting();
}

void InsertCharAtCurrsor(char c)
{
  struct Line *l = &g_lines[g_cursor_line];

  if (l->len + 1 > l->cap)
  {
    GrowString(l);
  }

  InsertChar(l->base, l->len - 1, c, g_cursor_col);

  l->len += 1;
  g_cursor_col += 1;
  g_file_changed = true;
}

void FDrawText(const char *text, float x, float y, Color color)
{
  DrawTextEx(g_font, text, (Vector2){x, y}, g_font_size, 1, color);
}

void OpenTextFile(const char *filepath)
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
    g_lines[i].style = malloc(strlen(line));
  }

  SetWindowTitle(TextFormat("%s - FCON", GetFileName(filepath)));
  memcpy(g_open_file_path, filepath, strlen(filepath));

  const char *ext = GetFileExtension(GetFileName(filepath));

  g_open_file_syntax = S_PlainText;

  // clang-format off
  if (strcmp(ext, ".c") == 0)      g_open_file_syntax = S_C;
  else if (strcmp(ext, ".C") == 0) g_open_file_syntax = S_C;
  else if (strcmp(ext, ".h") == 0) g_open_file_syntax = S_C;
  else if (strcmp(ext, ".H") == 0) g_open_file_syntax = S_C;
  else if (strcmp(ext, ".v") == 0) g_open_file_syntax = S_Verilog;
  else if (strcmp(ext, ".V") == 0) g_open_file_syntax = S_Verilog;
  // clang-format on

  DoSyntaxHighlighting();
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

bool IsFileOpen()
{
  if (g_open_file_path[0] == '\0')
    return false;

  return true;
}

void SaveOpenFile()
{
  if (!IsFileOpen()) // new document
  {

    const char *newPath = SaveFileDialog(GetApplicationDirectory(), "*");
    printf("saving : %s\n", newPath);
    if (newPath)
    {
      SetWindowTitle(TextFormat("%s - FCON", GetFileName(newPath)));
      memset(g_open_file_path, 0, sizeof(g_open_file_path));
      memcpy(g_open_file_path, newPath, strlen(newPath));
    }
  }

  if (!IsFileOpen())
  {
    return;
  }

  FILE *f = fopen(g_open_file_path, "w");
  if (!f)
  {

    Alert(TextFormat("Error Opening %s for writing:", g_open_file_path));

    return;
  }

  for (size_t i = 0; i < g_lines_count; i++)
  {
    fwrite(g_lines[i].base, sizeof(char), g_lines[i].len, f);
    fwrite("\n", sizeof(char), 1, f);
  }

  fclose(f);
  g_file_changed = false;
}

// stop_at must be \0 terminated list or NULL to never stop
void JumpCursor(int dir, const char *stop_at)
{
  if (!g_lines)
    return;
  struct Line l = g_lines[g_cursor_line];
  if (!l.base)
    return;

  // idea: skip any groups of same chars (skip row of spaces)

  g_cursor_col += dir;
  if (g_cursor_col < 0)
    g_cursor_col = 0;
  if (g_cursor_col > l.len)
    g_cursor_col = l.len;

  while (g_cursor_col > 0 &&
         g_cursor_col < l.len)
  {
    g_cursor_col += dir;

    if (!stop_at)
      continue; // only stop at eol or sol

    char ch = stop_at[0];
    for (size_t i = 0; ch != '\0'; i++)
    {
      ch = stop_at[i];
      if (l.base[g_cursor_col] == ch)
      {
        goto DONE;
      }
    }
  }
DONE:;
}

const char *jump_stops = " {}()<>\"\',;";
void JumpCursorToLeft() { JumpCursor(-1, jump_stops); }
void JumpCursorToRight() { JumpCursor(1, jump_stops); }
void HomeCursor() { JumpCursor(-1, NULL); }
void EndCursor() { JumpCursor(1, NULL); }

void MoveLineUp()
{
  if (!g_lines)
    return;
  if (g_cursor_line - 1 < 0)
    return;

  struct Line *a = &g_lines[g_cursor_line];
  struct Line *b = &g_lines[g_cursor_line - 1];

  struct Line tmp = *a;
  *a = *b;
  *b = tmp;

  g_cursor_line -= 1;
}

void MoveLineDown()
{
  if (!g_lines)
    return;
  if (g_cursor_line + 1 >= g_lines_count)
    return;
  struct Line *a = &g_lines[g_cursor_line];
  struct Line *b = &g_lines[g_cursor_line + 1];

  struct Line tmp = *a;
  *a = *b;
  *b = tmp;

  g_cursor_line += 1;
}

int main(int argc, char *argv[])
{
  SetConfigFlags(FLAG_WINDOW_RESIZABLE); //| FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(800, 600, "FCON");
  SetTargetFPS(60);

  SetExitKey(KEY_NULL);

  Image img = LoadImage("fonts/font_8_16.png");
  g_font = LoadFontFromImage(img, MAGENTA, '!');
  UnloadImage(img);

  TraceLog(LOG_INFO, "Loaded font with %d Glyphs", g_font.glyphCount);

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

  if (argc == 2)
  {
    OpenTextFile(argv[1]);
  }
  else
  {
    g_lines = malloc(sizeof(struct Line) * INIT_LINE_COUNT);
    g_lines_count = INIT_LINE_COUNT;

    for (size_t i = 0; i < INIT_LINE_COUNT; i++)
    {
      g_lines[i].base = NULL;
      g_lines[i].cap = g_lines[i].len = 0;
    }
  }

  OnResize();

  bool run = true;
  bool show_search_ui = false;

  while (run)
  {

    if (WindowShouldClose())
    {
      if (HasUnsavedChanges())
      {

        int resp = PopUp("You have unsaved changes", "",
                         "Save and exit|I dont wanna exit|Throw it away and exit");

        if (resp == 1)
        {
          SaveOpenFile();

          if (HasUnsavedChanges())
          {
            continue;
          }
        }
        else if (resp == 3)
        {
          goto EXIT;
        }
      }
      else
      {
        goto EXIT;
      }
    }

    if (IsWindowResized())
    {
      OnResize();
    }

    if (!show_search_ui)
    {

      if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
      {
        if (IsKeyPressed(KEY_S) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
        {

          const char *saveAsPath = SaveFileDialog(GetApplicationDirectory(), "*");
          if (saveAsPath)
          {
            printf("save as: %s\n", saveAsPath);
            memset(g_open_file_path, 0, sizeof(g_open_file_path));
            memcpy(g_open_file_path, saveAsPath, strlen(saveAsPath));
            SaveOpenFile();
          }
        }
        else if (IsKeyPressed(KEY_F))
        {
          show_search_ui = true;
        }
        else if (IsKeyPressed(KEY_O))
        {

          const char *newPath = OpenFileDialog(GetApplicationDirectory(), "*");
          printf("opening : %s\n", newPath);
          if (newPath)
          {
            OpenTextFile(newPath);
          }
        }
        else if (IsKeyPressed(KEY_S))
        {
          SaveOpenFile();
        }
        else if (IsKeyPressed(KEY_EQUAL) || IsKeyPressedRepeat(KEY_EQUAL))
        {
          g_font_size += 1.0f;
          OnResize();
        }
        else if (IsKeyPressed(KEY_MINUS) || IsKeyPressedRepeat(KEY_MINUS))
        {
          g_font_size -= 1.0f;
          OnResize();
        }
        else if (IsKeyPressed(KEY_ZERO))
        {
          g_font_size = DEFAULT_FONT_SIZE;
          OnResize();
        }
        else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
        {
          g_topline -= 1;
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
        {
          g_topline += 1;
        }
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))
        {
          JumpCursorToLeft();
        }
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))
        {
          JumpCursorToRight();
        }
        // else if (IsKeyPressed(KEY_Z))
        // {
        //   Undo();
        // }
        // else if (IsKeyPressed(KEY_Z))
        // {
        //   Redo();
        // }
      }
      if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
      {
        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
        {
          MoveLineUp();
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
        {
          MoveLineDown();
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
        else if (IsKeyPressed(KEY_HOME))
        {
          HomeCursor();
        }
        else if (IsKeyPressed(KEY_END))
        {
          EndCursor();
        }
        else if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN))
        {
          g_topline += g_screen_line_count - 5;
          g_cursor_line += g_screen_line_count - 5;
          OnCurrsorLineChanged();
        }
        else if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
        {
          g_cursor_line -= 1;
          OnCurrsorLineChanged();
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
        {
          g_cursor_line += 1;
          OnCurrsorLineChanged();
        }
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))
        {
          g_cursor_col -= 1;
        }
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))
        {
          g_cursor_col += 1;
        }
        else if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
        {
          BackspaceKeyPressed();
        }
        else if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE))
        {
          DeleteKeyPressed();
        }
        else if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER))
        {
          EnterKeyPressed();
        }
        else if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB))
        {
          g_file_changed = true;
          for (size_t i = 0; i < TAB_SIZE; i++)
          {
            InsertCharAtCurrsor(' ');
          }
        }
        else
        {
          int c;
          bool anything_entered = false;

          do
          {
            c = GetCharPressed();
            if (!c)
              break;

            InsertCharAtCurrsor(c);
            anything_entered = true;

          } while (c);

          if (anything_entered)
          {
            DoSyntaxHighlighting();
          }
        }
      }

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
      { // TODO: positions off a little in this mehtod....
        Vector2 pt = GetMousePosition();

        for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
        {
          Rectangle r = {g_font_size * LINE_NUMBERS_SUPPORTED - 1,
                         (i - g_topline) * g_font_size, GetScreenWidth(),
                         g_font_size};

          if (CheckCollisionPointRec(pt, r))
          {
            g_cursor_line = i;

            // now that we found the line, lets find the col that we are on
            struct Line *line = &g_lines[i];
            printf("%p\n", line);

            if (g_cursor_line >= g_lines_count || line->len == 0)
            {
              g_cursor_col = 0;
            }
            else
            {
              for (size_t col = 0; col <= line->len; col++)
              {
                Vector2 textSize = MeasureTextEx2(line->base, col);
                if (pt.x < textSize.x + g_font_size * LINE_NUMBERS_SUPPORTED)
                {
                  g_cursor_col = col;
                  break;
                }
              }
            }

            break;
          }
        }
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

    if (0 > g_topline)
      g_topline = 0;

    if (0 > g_cursor_line)
      g_cursor_line = 0;

    if (0 > g_cursor_col)
      g_cursor_col = 0;

    if (g_cursor_line >= g_lines_count - 1)
      g_cursor_line = g_lines_count - 1;

    if (g_cursor_col > g_lines[g_cursor_line].len)
      g_cursor_col = g_lines[g_cursor_line].len;

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);
    BeginMode2D(g_x_scroll_view);

    { // Text Rendering
      for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
      {

        FDrawText(TextFormat("%*d", LINE_NUMBERS_SUPPORTED, i + 1), 3, (i - g_topline) * g_font_size, BLACK);

        if (i == g_cursor_line)
        {
          DrawRectangle(g_font_size * LINE_NUMBERS_SUPPORTED - 1,
                        (i - g_topline) * g_font_size, GetScreenWidth(),
                        g_font_size, (Color){0xee, 0xe8, 0xd5, 0xFF});
        }

        if (i >= g_lines_count)
          break;

        for (size_t c = 0; c < g_lines[i].len; c++)
        {
          Color color = BLACK;

          if (g_lines[i].style)
          {
            switch (g_lines[i].style[c])
            {
              // clang-format off
              case C_Black: color = rgb(0, 0, 0);break;
              case C_Blue:  color = rgb(0, 121, 241);break;
              case C_Brick: color = rgb(203, 75, 22);break;
              case C_Green: color = rgb(133, 153, 0);break;
              case C_Purple:color = rgb(148, 6, 184); break;
              case C_Gray:color   = rgb(156, 156, 156); break;
              case C_Teal:color   = rgb(42, 161, 152); break;
              case C_Red :color   = rgb(221, 15, 15); break;
              // clang-format on
            }
          }

          FDrawText(TextFormat("%c", g_lines[i].base[c]),
                    g_font.recs[0].width * (LINE_NUMBERS_SUPPORTED + 1 + c),
                    (i - g_topline) * g_font_size, color);
        }
      }
    }

    { // currsor rendering
      struct Line *l = &g_lines[g_cursor_line];
      // Vector2 linelen = MeasureTextEx2(l->base, g_cursor_col);
      float llx = g_font.recs[0].width * (LINE_NUMBERS_SUPPORTED + 1 + g_cursor_col);
      float cy = (g_cursor_line - g_topline) * g_font_size;
      DrawLineEx((Vector2){llx, cy}, (Vector2){llx, cy + g_font_size}, 2, BLACK);
    }

    EndMode2D();

    if (show_search_ui)
    {
      static char serach_buffer[32] = {0};
      static char replace_buffer[32] = {0};

      if (IsKeyPressed(KEY_ESCAPE))
      {
        show_search_ui = false;
      }

      Rectangle pos = {
          GetScreenWidth() * 0.75f,
          10,
          GetScreenWidth() - (GetScreenWidth() * 0.75f),
          g_font_size * 3 + 3};

      DrawRectangleRec(pos, rgb(244, 255, 184));

      // search box
      osfd_TextBox(serach_buffer, sizeof(serach_buffer),
                   pos.x + 1,
                   pos.y + 1,
                   pos.width - 1,
                   true);

      // replace box
      osfd_TextBox(replace_buffer, sizeof(replace_buffer),
                   pos.x + 1,
                   pos.y + 1 + g_font_size,
                   pos.width - 1,
                   false);

      int fn_measure = osfd_MeasureText("Next");
      int fp_measure = osfd_MeasureText("Prev");

      osfd_TextButton("Next", pos.x, pos.y + g_font_size + g_font_size + 1, fn_measure);
      osfd_TextButton("Prev",
                      (pos.x + pos.width) - fp_measure,
                      pos.y + g_font_size + g_font_size + 2,
                      fp_measure);
    }

    { // statusbar rendering
      int status_y = GetScreenHeight() - g_font_size;
      DrawRectangle(0, status_y, GetScreenWidth(), g_font_size, (Color){0xd8, 0xd4, 0xc4, 0xff});
      // DrawTextCodepoint(g_font, '~' + 11, (Vector2){10, status_y}, g_font_size, BLACK);
      DrawTextEx(g_font, TextFormat("Font Size %.0f", g_font_size), (Vector2){10, status_y}, g_font_size, g_font_spacing, BLACK);
    }

    // DrawText(TextFormat("TOP IDX: %d\n%u\n%f\n%f", g_topline, redraw_count, cy,
    //                     linelen.x),
    //          40, GetScreenHeight() - g_font_size * 4, g_font_size, PURPLE);

    EndDrawing();
  }

EXIT:

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
