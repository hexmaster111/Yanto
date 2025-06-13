#include <raylib.h>
#include <stddef.h>
#include <string.h>

#include "spleen-16x32.c"
#include "spleen-8x16.c"

// #define SPLEEN_8X16_FRAME_COUNT 965
// #define SPLEEN_8X16_FRAME_WIDTH 8
// #define SPLEEN_8X16_FRAME_HEIGHT 16

#define FNT_COUNT 965
#define FNT_WIDTH 8
#define FNT_HEIGHT 16

#define BG_COLOR BLANK
#define FG_COLOR WHITE
#define KY_COLOR MAGENTA

int main(int argc, char *argv[])
{
    int char_cols = 32;
    int char_rows = FNT_COUNT / char_cols;
    int widthPx = (char_cols * FNT_WIDTH) + char_cols + 1;
    int heightPx = (char_rows * FNT_HEIGHT) + char_rows + 1;
    Image target = GenImageColor(widthPx, heightPx, KY_COLOR);
    int ch = 0;

    for (size_t r = 0; r < char_rows; r++)
    {
        for (size_t c = 0; c < char_cols; c++)
        {
            int px = (c * FNT_WIDTH) + c + 1;
            int py = (r * FNT_HEIGHT) + r + 1;

            uint32_t frame[FNT_WIDTH * FNT_HEIGHT];
            memmove(frame, spleen_8x16_data[ch], sizeof(frame));

            // ImageDrawRectangle(&target, px, py, FNT_WIDTH, FNT_HEIGHT, WHITE);
            for (size_t f = 0; f < FNT_WIDTH * FNT_HEIGHT; f++)
            {
                int pxx = px + (f % FNT_WIDTH);
                int pxy = py + (f / FNT_WIDTH);

                ImageDrawPixel(&target, pxx, pxy, frame[f] ? FG_COLOR : BG_COLOR);
            }

            ch += 1;

            if (ch == 255)
                goto DONE; // by default, raylib can only handle about this many
        }
    }

DONE:
    ExportImage(target, "font.png");
    ExportImageAsCode(target, "font_builtin.h");
    return 0;
}
