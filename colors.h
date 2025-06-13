#define rgb(COLORR, COLORG, COLORB) ((Color){COLORR, COLORG, COLORB, 0xff})

#ifdef DARKMODE
#define BACKGROUND_COLOR           rgb(40, 44, 52)
#define SELECTED_TEXT_COLOR        rgb(80, 140, 200)
#define EDITOR_CURRENT_LINE_COLOR  rgb(60, 60, 70)
#define EDITOR_BACKGROUND_COLOR    rgb(50, 54, 62)
#define EXPLOERER_BACKGROUND_COLOR rgb(50, 54, 62)
#define TEXT_NORMAL_COLOR          rgb(220, 220, 220)
#define TEXT_BLUE_COLOR            rgb(97, 175, 239)
#define TEXT_BRICK_COLOR           rgb(224, 108, 117)
#define TEXT_GREEN_COLOR           rgb(152, 195, 121)
#define TEXT_PURPLE_COLOR          rgb(198, 120, 221)
#define TEXT_GRAY_COLOR            rgb(130, 130, 130)
#define TEXT_TEAL_COLOR            rgb(86, 182, 194)
#define TEXT_RED_COLOR             rgb(224, 108, 117)
#endif

#ifndef DARKMODE
#define BACKGROUND_COLOR           rgb(253, 246, 227)
#define SELECTED_TEXT_COLOR        rgb(150, 229, 248)
#define EDITOR_CURRENT_LINE_COLOR  rgb(244, 255, 184)
#define EDITOR_BACKGROUND_COLOR    rgb(238, 232, 213)
#define EXPLOERER_BACKGROUND_COLOR rgb(238, 232, 213)
#define TEXT_NORMAL_COLOR          rgb(0, 0, 0)
#define TEXT_BLUE_COLOR            rgb(0, 121, 241)
#define TEXT_BRICK_COLOR           rgb(203, 75, 22)
#define TEXT_GREEN_COLOR           rgb(133, 153, 0)
#define TEXT_PURPLE_COLOR          rgb(148, 6, 184)
#define TEXT_GRAY_COLOR            rgb(156, 156, 156)
#define TEXT_TEAL_COLOR            rgb(42, 161, 152)
#define TEXT_RED_COLOR             rgb(221, 15, 15)
#endif //LIGHTMODE