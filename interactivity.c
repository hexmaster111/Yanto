#include <raylib.h>
#include <stdio.h>
#include <string.h>

#define INTER_TEXT_COLOR (BLACK)
#define INTER_BG_COLOR (WHITE)

// your program shoud define these as non-static
extern Font g_font;
extern float g_font_size;
extern float g_font_spacing;

// TODO: These methods should be moved to this c file
extern Vector2 osfd_MeasureTextEx2(const char *text, size_t text_len);
extern bool osfd_TextButton(const char *text, int x, int y, int textWidth);

// --- FWD Decls ---
int PopUp(const char *title, const char *message, const char *buttons);

// Shows a popup that only has "OK" Button
void Alert(const char *message) { PopUp("Alert", message, "OK"); }

/// @param buttons buttons seprated by a '|' char
/// @return idx of button pressed
int PopUp(const char *title, const char *message, const char *buttons)
{
    Rectangle pos = {100, 100, 200, g_font_size * 3};

    // 10 buttons max, 255 chars
    char btn_txt[10][255] = {0};

    int btnidx = 0;
    int charidx = 0;

    const char *cur = buttons;



    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
        {
            return 0;
        }

        BeginDrawing();
        ClearBackground((Color){0xee, 0xee, 0xef});

        DrawRectangleRec(pos, INTER_BG_COLOR);
        DrawTextEx(g_font, title, (Vector2){pos.x, pos.y}, g_font_size, g_font_spacing, INTER_TEXT_COLOR);
        DrawTextEx(g_font, message, (Vector2){pos.x, pos.y + g_font_size}, g_font_size, g_font_spacing, INTER_TEXT_COLOR);

        EndDrawing();
    }

    return 0;
}
