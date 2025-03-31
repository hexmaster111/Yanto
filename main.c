#include <raylib.h>
#include <raymath.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            ret[lines_used] = realloc(ret[lines_used], chars_read); // shrink string to be size read
            ret[lines_used][chars_read - 1] = 0;                    // terminate the string
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

            ret[lines_used] = realloc(ret[lines_used], chars_read + 1); // shrink string to be size read
            ret[lines_used][chars_read] = 0;                            // terminate the string

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
float g_font_size;

struct Line
{
    char *base;
    int len, cap;
} *g_lines;
int g_lines_count;

int g_topline;
int g_screen_line_count;

int g_cursor_line;

#define LINENUMBERSUPPORT 3

void FDrawText(const char *text, int x, int y, Color color) { DrawTextEx(g_font, text, (Vector2){x, y}, g_font_size, 1, color); }

void LoadTextFile(const char *filepath)
{
    g_cursor_line = 0;

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
}

void OnResize() { g_screen_line_count = GetScreenHeight() / g_font_size; }

int main()
{
    InitWindow(800, 600, "FCON");
    SetTargetFPS(60);
    EnableEventWaiting();

    g_font = LoadFont("romulus.png");
    if (!IsFontValid(g_font))
        return 1;

    g_font_size = 12;
    g_topline = 0;
    g_lines_count = 0;
    g_lines = NULL;
    g_screen_line_count = 0;
    g_cursor_line = 0;

    // LoadTextFile("test_small.fc");
    LoadTextFile("test.fc");

    Camera2D view = {.zoom = 1};

    OnResize();

    unsigned redraw_count = 0;

    while (!WindowShouldClose())
    {
        redraw_count++;

        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        {
            if (IsKeyPressed(KEY_EQUAL))
            {
                g_font_size += 1.0f;
                OnResize();
            }

            if (IsKeyPressed(KEY_MINUS))
            {
                g_font_size -= 1.0f;
                OnResize();
            }

            if (IsKeyPressed(KEY_ZERO))
            {
                g_font_size = 12;
                OnResize();
            }
        }

        if (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP))
        {
            g_topline -= g_screen_line_count - 5;
        }

        if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN))
        {
            g_topline += g_screen_line_count - 5;
        }

        if (IsKeyDown(KEY_UP))
        {
            g_cursor_line -= 1;

            if (g_topline + 5 > g_cursor_line)
            {
                g_topline = g_cursor_line - 5; 
            }
        }
        if (IsKeyDown(KEY_DOWN))
        {
            g_cursor_line += 1;

            if ((g_topline + g_screen_line_count) - 5 < g_cursor_line)
            {
                g_topline = (g_cursor_line - g_screen_line_count) + 5;
            }
        }

        if (IsWindowResized())
        {
            OnResize();
        }

        Vector2 scroll = GetMouseWheelMoveV();
        g_topline -= scroll.y;

        if (g_topline + g_screen_line_count > g_lines_count)
        {
            g_topline = g_lines_count - g_screen_line_count;
        }


        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 pt = GetMousePosition();

            for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
            {
                Rectangle r = {
                    g_font_size * LINENUMBERSUPPORT - 1,
                    (i - g_topline) * g_font_size,
                    GetScreenWidth(),
                    g_font_size};

                if (CheckCollisionPointRec(pt, r))
                {
                    g_cursor_line = i;
                    break;
                }
            }
        }

        if (0 > g_topline)
            g_topline = 0;

        if (0 > g_cursor_line)
            g_cursor_line = 0;

        if (g_cursor_line >= g_lines_count)
            g_cursor_line = g_lines_count - 1;

    
        BeginDrawing();
        ClearBackground((Color){0xfd, 0xf6, 0xe3, 0xFF});

        for (size_t i = g_topline; i < (g_screen_line_count + g_topline); i++)
        {
            FDrawText(TextFormat("%*d", LINENUMBERSUPPORT, i + 1), 3, (i - g_topline) * g_font_size, BLACK);
            if (i >= g_lines_count)
                break;

            if (i == g_cursor_line)
            {
                DrawRectangle(g_font_size * LINENUMBERSUPPORT - 1,
                              (i - g_topline) * g_font_size,
                              GetScreenWidth(),
                              g_font_size,
                              WHITE);
            }

            FDrawText(g_lines[i].base, g_font_size * LINENUMBERSUPPORT - 1, (i - g_topline) * g_font_size, BLACK);
        }

        DrawText(TextFormat("TOP IDX: %d\n%u", g_topline, redraw_count),
                 40, GetScreenHeight() - g_font_size * 2, g_font_size, PURPLE);

        EndDrawing();
    }

    return 0;
}
