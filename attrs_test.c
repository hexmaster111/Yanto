#include <stdio.h>
#include <stdint.h>

/*
    I kinda want like a blinking editor all the time... and i think
    i could hack this into the style u8 as there is only 7 colors,
    so we have 4 free bits to store some style information in

*/


#define HIGH_NIBBLE(BYTE) (((BYTE) >> 4) & 0x0F)
#define LOW_NIBBLE(BYTE) ((BYTE) & 0x0F)

#define SET_HIGH_NIBBLE(BYTE, NIBBLE)  ((BYTE) = ((BYTE) & 0x0F) | ((NIBBLE & 0x0F) << 4))
#define SET_LOW_NIBBLE(BYTE, NIBBLE)  ((BYTE) = ((BYTE) & 0xF0) | ((NIBBLE) & 0x0F))

#define STYLE_COLOR(STYLEBYTE) LOW_NIBBLE((STYLEBYTE))
#define STYLE_ATTRS(STYLEBYTE) HIGH_NIBBLE((STYLEBYTE))



int main() {

    uint8_t byte = 0b00000000;

    SET_HIGH_NIBBLE(byte, 5);
    SET_LOW_NIBBLE(byte, 2);

    uint8_t h = HIGH_NIBBLE(byte);
    uint8_t l = LOW_NIBBLE(byte);
    
    printf("byte: %d high: %d, low: %d\n", byte, h, l);

    return 0;
}


// ----------- UNDO REDO IDEAS ------------------ //

typedef struct
{
  enum
  {
    URa_InsertChar,
  } kind;

  union
  {
    struct
    {
      size_t line, offset;
      char c;
    } insert_char;
  };
} UnReDoAction;

typedef struct
{
  size_t cap, top;
  UnReDoAction *items;
} UnReDoStack;

bool IsUnReDoStackEmpty(UnReDoStack *t) { return t->top <= 0; }

UnReDoAction PopUnReDo(UnReDoStack *t)
{
  UnReDoAction ret = t->items[t->top - 1];
  t->top -= 1;
  return ret;
}

void PushUnReDo(UnReDoStack *t, UnReDoAction val)
{
  if (t->top + 1 > t->cap)
  {
    size_t new_cap = (t->cap + 1) * 2;
    t->items = realloc(t->items, new_cap * sizeof(UnReDoAction));
    t->cap = new_cap;
  }

  t->items[t->top] = val;
  t->top += 1;
}

UnReDoStack g_undo = {0}, g_redo = {0};