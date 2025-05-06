#include <raylib.h>
#include "spleen-16x32.c"

#define FNT_COUNT 965
#define FNT_WIDTH 16
#define FNT_HEIGHT 32
#define OUT_COLS 255

int main(int argc, char *argv[])
{
    int rows = FNT_COUNT / OUT_COLS;
    Color key = (Color){0xFF, 0x66, 0xFF};
    Image target = GenImageColor(OUT_COLS * FNT_WIDTH, rows * FNT_HEIGHT, key);
}
