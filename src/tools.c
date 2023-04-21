#include "tetris.h"

#include "types.h"
#include "tools.h"

#include "timerutils.h" 
#include "celutils.h" 


static unsigned char bitfonts[] = {0,0,0,0,0,0,0,0,4,12,8,24,16,0,32,0,10,18,20,0,0,0,0,0,0,20,126,40,252,80,
0,0,6,25,124,32,248,34,28,0,4,12,72,24,18,48,32,0,14,18,20,8,21,34,29,0,32,32,64,0,0,0,
0,0,16,32,96,64,64,64,32,0,4,2,2,2,6,4,8,0,8,42,28,127,28,42,8,0,0,4,12,62,24,16,
0,0,0,0,0,0,0,0,32,64,0,0,0,60,0,0,0,0,0,0,0,0,0,0,32,0,4,12,8,24,16,48,
32,0,14,17,35,77,113,66,60,0,12,28,12,8,24,16,16,0,30,50,4,24,48,96,124,0,28,50,6,4,2,98,
60,0,2,18,36,100,126,8,8,0,15,16,24,4,2,50,28,0,14,17,32,76,66,98,60,0,126,6,12,24,16,48,
32,0,56,36,24,100,66,98,60,0,14,17,17,9,2,34,28,0,0,0,16,0,0,16,0,0,0,0,16,0,16,32,
0,0,0,0,0,0,0,0,0,0,0,0,30,0,60,0,0,0,0,0,0,0,0,0,0,0,28,50,6,12,8,0,
16,0,0,0,0,0,0,0,0,0,14,27,51,63,99,65,65,0,28,18,57,38,65,65,62,0,14,25,32,96,64,98,
60,0,12,18,49,33,65,66,60,0,30,32,32,120,64,64,60,0,31,48,32,60,96,64,64,0,14,25,32,96,68,98,
60,0,17,17,50,46,100,68,68,0,8,8,24,16,48,32,32,0,2,2,2,6,68,68,56,0,16,17,54,60,120,76,
66,0,16,48,32,96,64,64,60,0,10,21,49,33,99,66,66,0,17,41,37,101,67,66,66,0,28,50,33,97,67,66,
60,0,28,50,34,36,120,64,64,0,28,50,33,97,77,66,61,0,28,50,34,36,124,70,66,0,14,25,16,12,2,70,
60,0,126,24,16,16,48,32,32,0,17,49,35,98,70,68,56,0,66,102,36,44,40,56,48,0,33,97,67,66,86,84,
40,0,67,36,24,28,36,66,66,0,34,18,22,12,12,8,24,0,31,2,4,4,8,24,62,0};

static CCB *textCel[MAX_STRING_LENGTH];

static char sbuffer[MAX_STRING_LENGTH+1];
static uchar fontsBmp[NUM_FONTS * FONT_SIZE];
static uint16 fontsPal[FONTS_PAL_SIZE];
static uchar fontsMap[FONTS_MAP_SIZE];

bool fontsAreReady = false;

// -------------------------------------

static Item timerIOreq;

int lastTicks = 0;

void initTimer()
{
	timerIOreq = GetTimerIOReq();
}

void initFonts()
{
	int i = 0;
	int n, x, y;

	for (n=0; n<FONTS_PAL_SIZE; ++n) {
		fontsPal[n] = MakeRGB15(n, n, n);
	}

	for (n=0; n<NUM_FONTS; n++) {
		for (y=0; y<FONT_HEIGHT; y++) {
			int c = bitfonts[i++];
			for (x=0; x<FONT_WIDTH; x++) {
				fontsBmp[n * FONT_SIZE + x + y * FONT_WIDTH] = ((c >>  (7 - x)) & 1) * (FONTS_PAL_SIZE - 1);
			}
		}
	}

	for (i=0; i<FONTS_MAP_SIZE; ++i) {
		uchar c = i;

		if (c>31 && c<92)
			c-=32;
		else
			if (c>96 && c<123) c-=64;
		else
			c = 255;

		fontsMap[i] = c;
	}

	for (i=0; i<MAX_STRING_LENGTH; ++i) {
		textCel[i] = CreateCel(FONT_WIDTH, FONT_HEIGHT, 8, CREATECEL_CODED, fontsBmp);
		textCel[i]->ccb_PLUTPtr = (PLUTChunk*)fontsPal;

		textCel[i]->ccb_HDX = 1 << 20;
		textCel[i]->ccb_HDY = 0 << 20;
		textCel[i]->ccb_VDX = 0 << 16;
		textCel[i]->ccb_VDY = 1 << 16;

		textCel[i]->ccb_Flags |= (CCB_ACSC | CCB_ALSC);

		if (i > 0) LinkCel(textCel[i-1], textCel[i]);
	}

	fontsAreReady = true;
}

void setTextColor(uint16 color)
{
	fontsPal[31] = color;
}

void initTools()
{
	initTimer();
	initFonts();
	
	setTextColor(MakeRGB15(31, 31, 31));
}

void drawZoomedText(int xtp, int ytp, char *text, int zoom, Item bitmapItem)
{
	int i = 0;
	char c;

	if (!fontsAreReady) return;

	do {
		c = fontsMap[*text++];

		textCel[i]->ccb_XPos = xtp  << 16;
		textCel[i]->ccb_YPos = ytp  << 16;

		textCel[i]->ccb_HDX = (zoom << 20) >> TEXT_ZOOM_SHR;
		textCel[i]->ccb_VDY = (zoom << 16) >> TEXT_ZOOM_SHR;

		textCel[i]->ccb_SourcePtr = (CelData*)&fontsBmp[c * FONT_SIZE];

		xtp+= ((zoom * FONT_WIDTH) >> TEXT_ZOOM_SHR);
	} while(c!=255 && ++i < MAX_STRING_LENGTH);

	--i;
	textCel[i]->ccb_Flags |= CCB_LAST;
	DrawCels(bitmapItem, textCel[0]);
	textCel[i]->ccb_Flags ^= CCB_LAST;
}

void drawTextX2(int xtp, int ytp, char *text, Item bitmapItem)
{
	drawZoomedText(xtp, ytp, text, 2 << TEXT_ZOOM_SHR, bitmapItem);
}

void drawText(int xtp, int ytp, char *text, Item bitmapItem)
{
	drawZoomedText(xtp, ytp, text, 1 << TEXT_ZOOM_SHR, bitmapItem);
}

void drawNumber(int xtp, int ytp, int num, Item bitmapItem)
{
	sprintf(sbuffer, "%d", num);	// investigate and see if we can replace sprintf for numbers (I have encountered possible performance drawback for many (80) values per frame)
	drawText(xtp, ytp, sbuffer, bitmapItem);
}


int getTicks()
{
	//return GetTime(timerIOreq);
	return GetMSecTime(timerIOreq);
}
static int fps = 0, prevFrameNum = 0, prevTicks = 0, ticks;
void displayFPS(Item bitmapItem)
{
	

	ticks = getTicks() - prevTicks;
	
	if (getTicks() - prevTicks >= 1000)
	{
		const int frameNum = getFrameNum();

		lastTicks = ticks;
		
		prevTicks = getTicks();
		fps = frameNum - prevFrameNum;
		prevFrameNum = frameNum;
	}

	drawNumber(1, 1, fps, bitmapItem);
	drawNumber(1, 11, ticks, bitmapItem);
	drawNumber(1, 21, lastTicks, bitmapItem);
}

void displayMem(Item bitmapItem)
{
	MemInfo memInfoAny;
	MemInfo memInfoDRAM;
	MemInfo memInfoVRAM;
	MemInfo memInfoCEL;

	const int xp = 170;
	int yp = SCREEN_HEIGHT - (3 * 2 * 8) - 24;

	AvailMem(&memInfoAny, MEMTYPE_ANY);
	AvailMem(&memInfoDRAM, MEMTYPE_DRAM);
	AvailMem(&memInfoVRAM, MEMTYPE_VRAM);
	AvailMem(&memInfoVRAM, MEMTYPE_CEL);

	drawText(xp, yp, " ANY FREE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoAny.minfo_SysFree, bitmapItem); yp += 8;
	drawText(xp, yp, " ANY WIDE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoAny.minfo_SysLargest, bitmapItem); yp += 8;

	drawText(xp, yp, "DRAM FREE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoDRAM.minfo_SysFree, bitmapItem); yp += 8;
	drawText(xp, yp, "DRAM WIDE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoDRAM.minfo_SysLargest, bitmapItem); yp += 8;

	drawText(xp, yp, "VRAM FREE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoVRAM.minfo_SysFree, bitmapItem); yp += 8;
	drawText(xp, yp, "VRAM WIDE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoVRAM.minfo_SysLargest, bitmapItem); yp += 8;
	
	drawText(xp, yp, " CEL FREE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoCEL.minfo_SysFree, bitmapItem); yp += 8;
	drawText(xp, yp, " CEL WIDE:", bitmapItem);
	drawNumber(xp + 11*8, yp, memInfoCEL.minfo_SysLargest, bitmapItem);
}

void setPal(int c0, int c1, int r0, int g0, int b0, int r1, int g1, int b1, uint16* pal, int shr)
{
	int i, rr, gg, bb;
	float dc = (float)(c1 - c0);
	float dr = (float)(r1 - r0) / dc;
	float dg = (float)(g1 - g0) / dc;
	float db = (float)(b1 - b0) / dc;
	float r = (float)r0;
	float g = (float)g0;
	float b = (float)b0;

	pal+=c0;
	for (i=c0; i<=c1; i++)
	{
		rr = (int)r >> shr;
		gg = (int)g >> shr;
		bb = (int)b >> shr;
		*pal++ = (rr << 10) | (gg << 5) | bb;
		r += dr;
		g += dg;
		b += db;
	}
}