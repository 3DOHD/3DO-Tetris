#ifndef TOOLS_H
#define TOOLS_H

#define FONT_WIDTH 8
#define FONT_HEIGHT 8
#define FONT_SIZE (FONT_WIDTH * FONT_HEIGHT) 

#define FONTS_PAL_SIZE 32
#define FONTS_MAP_SIZE 256

#define MAX_STRING_LENGTH 64
#define NUM_FONTS 59

#define TEXT_ZOOM_SHR 8
#endif

void initTools(void);

void drawText(int xtp, int ytp, char *text, Item bitmapItem);
void drawTextX2(int xtp, int ytp, char *text, Item bitmapItem);
void drawZoomedText(int xtp, int ytp, char *text, int zoom, Item bitmapItem);
void drawNumber(int xtp, int ytp, int num, Item bitmapItem);
void setTextColor(uint16 color);

void displayFPS(Item bitmapItem);
void displayMem(Item bitmapItem);

int getTicks(void);

void setPal(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16* pal, int shr);

