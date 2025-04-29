#include <raylib.h>
#include <string.h>

#define MAX_PATH (1024)
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

#define OSFD_FONTSIZE (18)

#define SAVE_TEXT ("Save")
#define CANCEL_TEXT ("Cancel")

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

int osfd_MeasureText(const char *text) { return MeasureText(text, OSFD_FONTSIZE); }

bool osfd_TextButton(const char *text, int x, int y)
{
    Rectangle me = {x, y, osfd_MeasureText(text), OSFD_FONTSIZE};

    bool hovered = CheckCollisionPointRec(GetMousePosition(), me);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    bool mayclic = hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT);

    Color fg = OSFD_BUTTON_IDLE_FG, bg = OSFD_BUTTON_IDLE_BG;

    if (hovered){
        fg = OSFD_BUTTON_HOVERED_FG;
        bg = OSFD_BUTTON_HOVERED_BG;
    }

    if (clicked){
        fg = OSFD_BUTTON_CLICKED_FG;
        bg = OSFD_BUTTON_CLICKED_BG;
    }

    if (mayclic){
        fg = OSFD_BUTTON_MAYCLICK_FG;
        bg = OSFD_BUTTON_MAYCLICK_BG;
    }

    DrawRectangleRec(me, bg);
    DrawText(text, x, y, OSFD_FONTSIZE, fg);


    return clicked;
}

const char *SaveFileDialog(
    const char *initalPath,
    const char *extention)
{
    
    static char buf[MAX_PATH];
    memset(buf, 0, sizeof(buf));

    char cwd[MAX_PATH];
    memset(cwd, 0, sizeof(cwd));
    memcpy(cwd, initalPath, strlen(initalPath));

    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE))
    {
        Rectangle outline = {
            100,
            100,
            400,
            300,
        };

        // Rendering

        BeginDrawing();
        DrawRectangleRec(outline, OSFD_BACKGROUND);

        int save_text_measure = osfd_MeasureText(SAVE_TEXT);
        int cancel_text_measure = osfd_MeasureText(CANCEL_TEXT);

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

    return buf;
}