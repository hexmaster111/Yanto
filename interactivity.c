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
/// @return idx of button pressed + 1 or 0 for escape
int PopUp(const char *title, const char *message, const char *buttons)
{
    
    // 10 buttons max, 255 chars
    char btn_txt[10][255] = {0};
    
    int btnidx = 0;
    int charidx = 0;
    
    const char *cur = buttons;
    
    for (;; cur += 1)
    {
        if (*cur == '|')
        {
            charidx = 0;
            btnidx += 1;
        }
        else if (*cur == '\0')
        {
            btnidx += 1;
            break;
        }
        else
        {
            btn_txt[btnidx][charidx] = *cur;
            charidx += 1;
        }
    }
    charidx = 0;


    Rectangle pos = {100, 100, 200, g_font_size * (3 + (btnidx - 1))};
    

    while (true)
    {
        BeginDrawing();

        if (IsKeyPressed(KEY_ESCAPE))
        {
            EndDrawing();
            return 0;
        }

        //ClearBackground((Color){0xee, 0xee, 0xef});

        DrawRectangleRec(pos, INTER_BG_COLOR);
        DrawTextEx(g_font, title, (Vector2){pos.x, pos.y}, g_font_size, g_font_spacing, INTER_TEXT_COLOR);
        DrawTextEx(g_font, message, (Vector2){pos.x, pos.y + g_font_size}, g_font_size, g_font_spacing, INTER_TEXT_COLOR);

        for (size_t i = 0; i < btnidx; i++)
        {
            int len = strlen(btn_txt[i]);
            Vector2 sz = osfd_MeasureTextEx2(btn_txt[i], len);

            if (osfd_TextButton(
                    btn_txt[i],
                    pos.x + pos.width - sz.x,
                    (pos.y + g_font_size + g_font_size) + (i * g_font_size),
                    sz.x) //
            )
            {
                EndDrawing();
                return i + 1;
            }
        }

        EndDrawing();
    }

    return 0;
}
