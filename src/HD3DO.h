/*
Copyright 2023 Shaun Nicholson - 3DOHD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// 
//	Utility to assist in rendering numbers to the screen without the need for making
//	additional drawcels calls.. as well as some other various cel helper bits
//

*/

#ifndef HD3DO_H
#define HD3DO_H

#include "celutils.h"
#include "deletecelmagic.h"
#include "mem.h"

#define MAXNUMCOUNT 8

#endif

typedef struct DebugData
{
	int Frames60s; // Frame counter
	int f60Idx;
	uint32 Frames60AvgMS;
	TimeVal tvInit;
	TimeVal tvFrames60Start;
	TimeVal tvFrames60End;
	TimeVal tvRenderStart;
	TimeVal tvRenderEnd;
	TimeVal tvDrawCelsStart;
	TimeVal tvDrawCelsEnd;
	TimeVal tvCurrLoopStart;
	TimeVal tvCurrLoopEnd;
} DebugData;

typedef struct TrackedNumber
{
	uint32 Value;
	int X;
	int Y;
	bool RightAlign;
	CCB *cel_NumCels[9]; // Max limit of 999,999,999
} TrackedNumber;

void InitNumberCels(int count); // Call this first
void SetCelNumbers(int idx, uint32 value);
CCB *InitAndPositionCel(char *path, int x, int y);
void PositionLoadedCel(CCB *cel, int x, int y);
void PositionCel(CCB *cel, int x, int y);
void PositionCelColumn(CCB *cel, int x, int y, int xOffset, int yOffset);
void InitCCBFlags(CCB *cel);
CCB * CopyCel(CCB *src);
void RenderCelNumbers(Item bitmapItem);
void ResetCelNumbers();
bool ValidAndReady();
void CleanupNumberCels();

extern TrackedNumber TrackedNumbers[8];
