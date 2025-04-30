#include <raylib.h>
#include <string.h>

#define MAX_PATH (1024)
#define MAX_FILENAME (255)

#define OSFD_BACKGROUND (BLACK)
#define OSFD_FORGROUND (WHITE)

#define OSFD_BUTTON_HOVERED_BG (LIGHTGRAY)
#define OSFD_BUTTON_CLICKED_BG (YELLOW)
#define OSFD_BUTTON_MAYCLICK_BG (SKYBLUE)

#define OSFD_BUTTON_CLICKED_FG (GRAY)
#define OSFD_BUTTON_HOVERED_FG (GRAY)
#define OSFD_BUTTON_MAYCLICK_FG (GRAY)

#define OSFD_BUTTON_IDLE_BG (GRAY)
#define OSFD_BUTTON_IDLE_FG (BLACK)

#define OSFD_TEXTBOX_BG (LIGHTGRAY)
#define OSFD_TEXTBOX_FG (BLACK)

#define OSFD_FONTSIZE (18)
#define OSFD_FONTSPACING (3)

#define SAVE_TEXT ("Save")
#define CANCEL_TEXT ("Cancel")

Font g_font;

int osfd_MeasureText(const char *text) { return MeasureText(text, OSFD_FONTSIZE); }

Vector2 osfd_MeasureTextEx2(const char *text, size_t text_len)
{
    Vector2 textSize = {0};

    if (((g_font.texture.id == 0)) || (text == NULL) || (text[0] == '\0'))
        return textSize; // Security check

    int tempByteCounter = 0; // Used to count longer text line num chars
    int byteCounter = 0;

    float textWidth = 0.0f;
    float tempTextWidth = 0.0f; // Used to count longer text line width

    float textHeight = OSFD_FONTSIZE;
    float scaleFactor = OSFD_FONTSIZE / (float)g_font.baseSize;

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

            textHeight += (OSFD_FONTSIZE + OSFD_FONTSPACING);
        }

        if (tempByteCounter < byteCounter)
            tempByteCounter = byteCounter;
    }

    if (tempTextWidth < textWidth)
        tempTextWidth = textWidth;

    textSize.x = tempTextWidth * scaleFactor +
                 (float)((tempByteCounter - 1) * OSFD_FONTSPACING);
    textSize.y = textHeight;

    return textSize;
}

// true on click
bool osfd_TextButton(const char *text, int x, int y)
{
    Rectangle me = {x, y, osfd_MeasureText(text), OSFD_FONTSIZE};

    bool hovered = CheckCollisionPointRec(GetMousePosition(), me);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    bool mayclic = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    Color fg = OSFD_BUTTON_IDLE_FG, bg = OSFD_BUTTON_IDLE_BG;

    if (hovered)
    {
        fg = OSFD_BUTTON_HOVERED_FG;
        bg = OSFD_BUTTON_HOVERED_BG;
    }

    if (clicked)
    {
        fg = OSFD_BUTTON_CLICKED_FG;
        bg = OSFD_BUTTON_CLICKED_BG;
    }

    if (mayclic)
    {
        fg = OSFD_BUTTON_MAYCLICK_FG;
        bg = OSFD_BUTTON_MAYCLICK_BG;
    }

    DrawRectangleRec(me, bg);
    DrawText(text, x, y, OSFD_FONTSIZE, fg);

    return clicked;
}

bool osfd_TextBox(char *text, int textcap, int x, int y, int width, bool selected_for_input)
{
    static int currsor_pos = 0;
    static double next_flash = 0;
    static bool currsor_flash = 0;
    int len = strlen(text);

    if (selected_for_input)
    {
        if (currsor_pos > len)
            currsor_pos = len;
        if (0 > currsor_pos)
            currsor_pos = 0;

        if (next_flash < GetTime())
        {
            next_flash = GetTime() + 0.5;
            currsor_flash = !currsor_flash;
        }

        if(IsKeyPressed(KEY_BACKSPACE)){
            if(len > 0){
                len -= 1;
                text[lenselected_for_input] = '\0';
            }
        }
    }

    int c;
    while (selected_for_input && (c = GetCharPressed()))
    {
        if (len >= textcap)
            break;
        text[len] = c;
        currsor_pos += 1;
        len += 1;
    }

    DrawRectangle(x, y, width, OSFD_FONTSIZE, OSFD_TEXTBOX_BG);
    DrawText(text, x, y, OSFD_FONTSIZE, OSFD_TEXTBOX_FG);

    if (selected_for_input && currsor_flash)
    {
        Vector2 cpos = osfd_MeasureTextEx2(text, currsor_pos);
        DrawRectangle(cpos.x + x, y, 1, OSFD_FONTSIZE, OSFD_TEXTBOX_FG);
    }

    return IsMouseButtonReleased(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(GetMousePosition(), (Rectangle){x, y, width, OSFD_FONTSIZE});
}

const char *SaveFileDialog(
    const char *initalPath,
    const char *extention)
{
    g_font = GetFontDefault();
    static char return_buffer[MAX_PATH];
    memset(return_buffer, 0, sizeof(return_buffer));

    char fname[MAX_FILENAME];
    char cwd[MAX_PATH];
    memset(cwd, 0, sizeof(cwd));
    memcpy(cwd, initalPath, strlen(initalPath));
    memset(fname, 0, sizeof(fname));

    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE))
    {
        Rectangle outline = {
            100,
            100,
            400,
            300,
        };

        // Rendering

        /*
        ---------------------------------------
        |⏪ ⏩ 🔼 [C:\Some\Path        ] 🔃 |
        |-------------------------------------|
        | 📂 something                       |
        | 📂 some dir                        |
        | 📂 a cool dir                      |
        | 📝 File 1.txt                      |
        | 📝 File 2.txt                      |
        | 📝 File 3.txt                      |
        | 📝 File 4.txt                      |
        | 📝 File 5.txt                      |
        | 📝 File 6.txt                      |
        |-------------------------------------|
        |[ File 1.txt       ] [Save] [Cancel] |
        |-------------------------------------|
        */

        BeginDrawing();
        DrawRectangleRec(outline, OSFD_BACKGROUND);

        int save_text_measure = osfd_MeasureText(SAVE_TEXT);
        int cancel_text_measure = osfd_MeasureText(CANCEL_TEXT);
        int filename_width = outline.width - (save_text_measure + cancel_text_measure);

        osfd_TextBox(fname, sizeof(fname), outline.x, outline.y + outline.height - OSFD_FONTSIZE, filename_width, true);

        bool saveClicked = osfd_TextButton(SAVE_TEXT,
                                           (outline.x + outline.width) - (save_text_measure + cancel_text_measure),
                                           (outline.y + outline.height) - OSFD_FONTSIZE);

        bool cancelClicked = osfd_TextButton(CANCEL_TEXT,
                                             (outline.x + outline.width) - (cancel_text_measure),
                                             (outline.y + outline.height) - OSFD_FONTSIZE);

        EndDrawing();

        if (cancelClicked)
        {
            return NULL;
        }

        if (saveClicked)
        {
            break;
        }
    }

    return return_buffer;
}