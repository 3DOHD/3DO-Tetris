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
//	Main Tetris Game
//

*/

#ifndef TETRIS_H
#define TETRIS_H
#include "displayutils.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "operamath.h"
#include "math.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "event.h"

#include "controlpad.h"

#include "stdio.h"
#include "graphics.h"
#include "audio.h"

#include "soundplayer.h"
#include "effectshandler.h"


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_SIZE_IN_BYTES (SCREEN_WIDTH * SCREEN_HEIGHT * 2)
#define SCREEN_PAGES 2

#define START 0x0000; // For Don's Konami code thing
#define UP 0x0001
#define DN 0x0002
#define LEFT 0x0003
#define RIGHT 0x0004
#define A 0x0005
#define B 0x0006
#define C 0x0007
#define LT 0x0008
#define RT 0x0009
#define SEL 0x000A

#endif

// Don's Konami Stuff
uint32 eeHigh = 0;
uint32 eeLow = 0;

uint32 eeHighMatch = 0x1122;
uint32 eeLowMatch = 0x34346565;

uint32 button = 0x0;
//

typedef struct BlockCoord
{
	int X;
	int Y;
} BlockCoord;

typedef struct Tetrimino
{
	int PivotIdx;
	int ShapeType;
	BlockCoord Blocks[4];
} Tetrimino;

typedef struct GameplayState
{
	bool IsGameOver;
	bool IsGamePaused;
	bool IsLineClearing;
	bool IsGameStarted;
	int ScoreCurrent;
	int ScoreTotal;
	int ScoreHigh;
	int LinesCurrent;
	int LinesTotal;
	int LinesHigh;
	int LevelCurrent;
	int LevelHigh;
	int NextLevelGoal;
	int CurrentLevel;
	int LevelSpeed;
} GameplayState;


