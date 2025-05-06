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
#define OSFD_TEXTBOX_ACTIVE_BG (SKYBLUE)
#define OSFD_TEXTBOX_FG (BLACK)

#define OSFD_DIRSEP ('/')

#define CANCEL_TEXT ("Cancel")
#define BACK_TEXT ("BCK")
#define FORWARD_TEXT ("FWD")
#define BROWSEUP_TEXT ("UP")
#define REFRESH_TEXT ("REF")

// your program shoud define these as non-static
extern Font g_font;
extern float g_font_size;
extern float g_font_spacing;

int osfd_MeasureText(const char *text)
{
    return MeasureTextEx(g_font, text, g_font_size, g_font_spacing).x;
}

Vector2 osfd_MeasureTextEx2(const char *text, size_t text_len)
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

    textSize.x = tempTextWidth * scaleFactor + (float)((tempByteCounter - 1) * g_font_spacing);
    textSize.y = textHeight;

    return textSize;
}

// true on click
bool osfd_TextButton(const char *text, int x, int y, int textWidth)
{
    Rectangle me = {x, y, textWidth, g_font_size};

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
    DrawTextEx(g_font, text, (Vector2){x, y}, g_font_size, g_font_spacing, fg);
    return clicked;
}

typedef enum
{
    TBNothing, // should do nothing
    TBSelect,  // should get focus
    TBAccept   // should accept the input
} TBAction;

TBAction osfd_TextBox(char *text, int textcap, int x, int y, int width, bool selected_for_input)
{
    static int currsor_pos = 0;
    static double next_flash = 0;
    static bool currsor_flash = 0;
    int len = strlen(text);

    if (selected_for_input)
    {

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))
        {
            currsor_pos -= 1;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_LEFT))
        {
            currsor_pos += 1;
        }

        if (currsor_pos > len)
            currsor_pos = len;
        if (0 > currsor_pos)
            currsor_pos = 0;

        if (next_flash < GetTime())
        {
            next_flash = GetTime() + 0.5;
            currsor_flash = !currsor_flash;
        }

        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
        {
            if (len > 0)
            {
                len -= 1;
                text[len] = '\0';
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

    DrawRectangle(x, y, width, g_font_size, selected_for_input ? OSFD_TEXTBOX_ACTIVE_BG : OSFD_TEXTBOX_BG);
    DrawTextEx(g_font, text, (Vector2){x, y}, g_font_size, g_font_spacing, OSFD_TEXTBOX_FG);

    if (selected_for_input && currsor_flash)
    {
        Vector2 cpos = osfd_MeasureTextEx2(text, currsor_pos);
        DrawRectangle(cpos.x + x, y, 1, g_font_size, OSFD_TEXTBOX_FG);
    }

    if (selected_for_input && IsKeyPressed(KEY_ENTER))
        return TBAccept;
    return (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){x, y, width, g_font_size})) ? TBSelect : TBNothing;
}

#include <stdio.h>

const char *osfd_DialogCore(
    const char *initalPath,
    const char *extention,
    const char *acceptButtonText);

const char *OpenFileDialog(
    const char *initalPath,
    const char *extention)

{
    return osfd_DialogCore(initalPath, extention, "Open");
}

const char *SaveFileDialog(
    const char *initalPath,
    const char *extention)

{
    return osfd_DialogCore(initalPath, extention, "Save");
}

const char *osfd_DialogCore(
    const char *initalPath,
    const char *extention,
    const char *acceptButtonText)
{
    static char return_buffer[MAX_PATH];
    memset(return_buffer, 0, sizeof(return_buffer));

    bool run = true;
    char fname[MAX_FILENAME];
    char cwd[MAX_PATH];
    memset(cwd, 0, sizeof(cwd));
    memcpy(cwd, initalPath, strlen(initalPath));
    memset(fname, 0, sizeof(fname));

    int scroll_top_line = 0, linesfit = 0;

    int element_focused = 0;

    // FilePathList filesindir = LoadDirectoryFilesEx(cwd, extention, false);
    FilePathList filesindir = LoadDirectoryFiles(cwd);

    while (run && !WindowShouldClose())
    {

        if (IsKeyPressed(KEY_ESCAPE))
        {
            if (filesindir.paths)
                UnloadDirectoryFiles(filesindir);
            return NULL;
        }

        Rectangle outline = {
            100,
            100,
            500,
            300,
        };

        linesfit = outline.height / g_font_size;
        linesfit -= 2; // file name + address bars

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_EQUAL))
        {
            g_font_size += 1;
        }
        else if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_MINUS))
        {
            g_font_size -= 1;
        }

        Vector2 wheel = GetMouseWheelMoveV();
        scroll_top_line += wheel.y * 2;

        if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_UP)))
        {
            scroll_top_line -= 1;
        }

        if (0 > scroll_top_line)
            scroll_top_line = 0;

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_DOWN))
        {
            scroll_top_line += 1;
        }

        // Rendering

        /*
        --------------------------------------
        |⏪ ⏩ 🔼 [C:\Some\Path        ]🔃 |
        |------------------------------------|
        | 📂 something                     [^]|
        | 📂 some dir                        |
        | 📂 a cool dir                      |
        | 📝 File 1.txt                      |
        | 📝 File 2.txt                      |
        | 📝 File 3.txt                      |
        | 📝 File 4.txt                      |
        | 📝 File 5.txt                      |
        | 📝 File 6.txt                     [v]|
        |-------------------------------------|
        |[ File 1.txt       ] [Save] [Cancel] |
        |-------------------------------------|
        */

        BeginDrawing();
        // ClearBackground((Color){0xee, 0xee, 0xef});
        DrawRectangleRec(outline, OSFD_BACKGROUND);

        int accept_text_measure = osfd_MeasureText(acceptButtonText);
        int cancel_text_measure = osfd_MeasureText(CANCEL_TEXT);
        int filename_width = outline.width - (accept_text_measure + cancel_text_measure);
        int back_text_measutre = osfd_MeasureText(BACK_TEXT);
        int forward_text_measure = osfd_MeasureText(FORWARD_TEXT);
        int up_text_measure = osfd_MeasureText(BROWSEUP_TEXT);
        int refresh_text_measure = osfd_MeasureText(REFRESH_TEXT);
        int filepath_width = (outline.width - refresh_text_measure) - (back_text_measutre + forward_text_measure + up_text_measure);

        TBAction tb_fname_action = osfd_TextBox(fname, sizeof(fname),
                                                outline.x,
                                                outline.y + outline.height - g_font_size,
                                                filename_width,
                                                element_focused == 1);

        if (tb_fname_action == TBSelect)
        {
            element_focused = 1;
        }
        else if (tb_fname_action == TBAccept)
        {
            break;
        }

        TBAction tp_path_action = osfd_TextBox(cwd, sizeof(cwd),
                                               outline.x + forward_text_measure + back_text_measutre + up_text_measure,
                                               outline.y, filepath_width,
                                               element_focused == 2);
        if (tp_path_action == TBSelect)
        {
            element_focused = 2;
        }

        bool openClicked = osfd_TextButton(acceptButtonText,
                                           (outline.x + outline.width) - (accept_text_measure + cancel_text_measure),
                                           (outline.y + outline.height) - g_font_size, accept_text_measure);

        bool btn_cancel_clicked = osfd_TextButton(CANCEL_TEXT,
                                                  (outline.x + outline.width) - (cancel_text_measure),
                                                  (outline.y + outline.height) - g_font_size, cancel_text_measure);

        bool backClicked = osfd_TextButton(BACK_TEXT, outline.x, outline.y, back_text_measutre);
        bool forwardClicked = osfd_TextButton(FORWARD_TEXT, outline.x + back_text_measutre, outline.y, forward_text_measure);
        bool upDirClicked = osfd_TextButton(BROWSEUP_TEXT, outline.x + forward_text_measure + back_text_measutre, outline.y, up_text_measure);
        bool btn_refresh_clicked = osfd_TextButton(REFRESH_TEXT,
                                                   outline.x + forward_text_measure + back_text_measutre + up_text_measure + filepath_width,
                                                   outline.y, refresh_text_measure);

        int items_to_render = filesindir.count > linesfit ? linesfit : filesindir.count;
        items_to_render += scroll_top_line;
        if (items_to_render > filesindir.count)
            items_to_render = filesindir.count;

        size_t draw_line = 0;
        for (size_t i = scroll_top_line; i < items_to_render; i++)
        {
            // printf("i: %d count: %d torender: %d topline: %d, linesfit %d \n", i, filesindir.count, items_to_render, scroll_top_line, linesfit);
            const char *fp = filesindir.paths[i];
            const char *cfn = GetFileName(fp);
            int fnlen = osfd_MeasureText(cfn);

            bool isDir = DirectoryExists(fp);

            if (osfd_TextButton(GetFileName(fp),
                                outline.x,
                                outline.y + (g_font_size * (draw_line + 1)),
                                outline.width))
            {
                printf("%s\n", cwd);

                if (isDir)
                {
                    size_t cwdlen = strlen(cwd);
                    memcpy(cwd + cwdlen, cfn, strlen(cfn));

                    cwd[strlen(cwd)] = OSFD_DIRSEP;

                    printf("%s\n", cwd);
                    if (filesindir.paths)
                        UnloadDirectoryFiles(filesindir);
                    filesindir = LoadDirectoryFiles(cwd);
                    scroll_top_line = 0;
                    goto QuitDrawingFileList;
                }
                else
                {
                    memset(fname, 0, sizeof(fname));
                    memcpy(fname, cfn, strlen(cfn));
                }
            }

            draw_line += 1;
        }

    QuitDrawingFileList:

        EndDrawing();

        if (btn_cancel_clicked)
        {

            if (filesindir.paths)
                UnloadDirectoryFiles(filesindir);

            return NULL;
        }

        if (btn_refresh_clicked || tp_path_action == TBAccept || IsKeyDown(KEY_F5))
        {
            if (filesindir.paths)
                UnloadDirectoryFiles(filesindir);

            filesindir = LoadDirectoryFiles(cwd);
        }

        if (openClicked)
        {
            break;
        }

        if (upDirClicked)
        {
            int strle = strlen(cwd);
            if (strle - 1 > 0 &&
                cwd[strle - 1] == OSFD_DIRSEP)
            {
                cwd[strle - 1] = '\0';
            }

            const char *updir = GetPrevDirectoryPath(cwd);
            memset(cwd, 0, sizeof(cwd));
            memcpy(cwd, updir, strle);

            strle = strlen(cwd);
            if (strle - 1 > 0 &&
                cwd[strle - 1] != OSFD_DIRSEP)
            {
                cwd[strle] = OSFD_DIRSEP;
            }

            if (filesindir.paths)
                UnloadDirectoryFiles(filesindir);
            filesindir = LoadDirectoryFiles(cwd);

            scroll_top_line = 0;
        }
    }

    if (filesindir.paths)
    {
        UnloadDirectoryFiles(filesindir);
    }

    int cwdlen = strlen(cwd);
    memcpy(return_buffer, cwd, cwdlen);
    memcpy(return_buffer + cwdlen, fname, strlen(fname));
    return return_buffer;
}