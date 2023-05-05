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
//	Main Tetris Game - 
//

*/

#include "tetris.h"
#include "celutils.h"
#include "HD3DO.h"
//#include "HD3DOAudio.h"
#include "HD3DOAudioSFX.h"
#include "HD3DOAudioSoundInterface.h"

#include "tools.h"

void CleanupTempCels();
void Cleanup();
void UpdateOnScreenStats(); 

void InitCCBFlags(CCB *cel);

void GameLoop();
void HandleInput();
void HandleInputOptionsMenu(uint32 joyBits);
void HandleInputStartMenu(uint32 joyBits);
void HandleOptionsMenuLogic();
void HandleStartMenuLogic();
void HandleGameplayLogic();
void HandleSelectedOptions();
bool TryMoveLeft();
bool TryMoveRight();
bool TryMoveUp();
bool TryMoveDown();
bool TryRotate(bool counterClockwise);
void Rotate(bool counterClockwise);
void MoveLeft();
void MoveRight(); 
void MoveUp();
void MoveDown(bool);

void DisplayBackgroundOnly();
void DisplayStartScreen();
void DisplayOptionsScreen();
void DisplayGameplayScreen();
void ToggleStartMenuSelection();
void ToggleOptionsMenuSelection(int);

void ApplySelectedColorPalette();
void DrawGamePlayScreen();
void Explode();
void GameOverKillBlocks();
void ReadyIn321();
void PauseScreen();
void CheckForNextLevel();
void ApplyCurrentThemeBackground();
void ShowStartMenu();
void HideStartMenu();
void ShowPausedMenu();
void HidePausedMenu();
void TogglePaused(bool isPaused);
void ShowOptionsMenu();
void HideOptionsMenu();
void ToggleOptionsMenu(bool optionsMenuSelected);
void SwapBackgroundImage(char *file, int imgIdx);

void QueueNextBlock();
void LoadNextBlockFromQueue();
void HoldBlock();
void SwapActiveBlockWithHeldBlock();

void InitGame();

void PlayBackgroundMusic();
void PlaySFX(int id);

void ShowIntroSplash();

/* ----- GAME VARIABLES -----*/

static ScreenContext screen;

static Item bitmapItems[SCREEN_PAGES];
static Bitmap *bitmaps[SCREEN_PAGES];

static Item VRAMIOReq;
static Item vsyncItem;
static IOInfo ioInfo;

static DebugData dData;

static int visibleScreenPage = 0;

static bool GamePlayBlocks[10][18] =
{
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static int BlockPivotIdx[7] = { 2, -1, 1, 1, 1, 2, 2}; // The pivot / rotation point of each respective Tetrimino block
static int BlockImageIdx[7] =  { 0, 1, 2, 3, 4, 5, 6 }; // Can be customized

static int Palettes[7][7] =
{
	{ 0, 1, 2, 3, 4, 5, 6 }, // Default
	{ 7, 8, 9, 10, 11, 12, 13 }, // Pinks and Purple
	{ 14, 15, 16, 17, 18, 19, 20 }, // Basic
	{ 23, 21, 7, 8, 9, 10, 11 }, // Alice in Wonderland
	{ 1, 2, 3, 4, 3, 2, 1 }, // Candy
	{ 1, 23, 23, 23, 23, 23, 23 }, // Midnight
	{ 25, 26, 27, 28, 29, 30, 31 } // Easter Egg
};

static CCB *cel_AllBlockImages[32]; // TODO - Load all game block data
static CCB *cel_GuideBlock;

static Tetrimino ActiveBlock;

static BlockCoord DefaultBlockCoords[7][4] = // Relative to it's queue position
{
	{ { 21, 3 }, { 22, 3 }, { 23, 3 }, { 24, 3 } }, // 1.) I-Block
	{ { 22, 3 }, { 22, 4 }, { 23, 3 }, { 23, 4 } }, // 2.) O-Block
	{ { 21, 3 }, { 22, 3 }, { 22, 4 }, { 23, 3 } }, // 3.) T-Block
	{ { 21, 4 }, { 22, 3 }, { 22, 4 }, { 23, 3 } }, // 4.) S-Block
	{ { 21, 3 }, { 22, 3 }, { 22, 4 }, { 23, 4 } }, // 5.) Z-Block
	{ { 21, 3 }, { 21, 4 }, { 22, 4 }, { 23, 4 } }, // 6.) J-Block
	{ { 21, 4 }, { 21, 3 }, { 22, 3 }, { 23, 3 } }  // 7.) L-Block
};

static bool GameOver = false;
static bool QuickReset = false;
static bool GameStarted = false;
static bool OptionsMenuSelected = false;
static bool EasterEggEnabled = false;
static bool smStartSelected = true;
static bool ClearingLines = false;
static bool IsPaused = false;
static bool RenderGameBlocks = false;
static bool AcceptGameInput = false;

static bool AllowShiftToRotate = true;

static int debugMode = 0;

static int frames = 0;

static int swGame = 0;
static int lvSpeed = 48;

static bool localShowGuides = true;
static bool localPlayMusic = true;
static bool localPlaySFX = true;
static bool localDefaultTheme = true; // Synth or Sci-Fi

static int localMainPalette = 0;

static bool OptionsPlayMusic = true;
static bool OptionsPlaySFX = true;
static bool OptionsShowGuides = true;
static bool OptionsDefaultTheme = true;

static int OptionsMainPalette = 0;

const int BLOCK_RED = 1;
const int BLOCK_GREY = 22;
const int BLOCK_3DO = 24;

int HighlightedOption = 0;

int QueueSwaps = 3;

static int frameNum = 0;
static int sfxPlay = 0;
static int sfxInit = 0;
static int sfxLoad = 0;

bool kpLeft = false;
bool kpRight = false;
bool kpUp = false;
bool kpDown = false;
bool kpLS = false;
bool kpRS = false;
bool kpA = false;
bool kpB = false;
bool kpC = false;
bool kpStart = false;
bool kpStop = false;

bool movingLeft = false;
bool movingRight = false;

int32 rNum = 0;

int QueuedShapeIdx = -1;
int HeldShapeIdx = -1;

bool CanHold = true;

int swLeft = 0;
int swRight = 0;
int swUp = 0;
int swDown = 0;
int swLS = 0;
int swRS = 0;
int swA = 0;
int swB = 0;
int swC = 0;

int bPresses = 0;

CCB *cel_Options;

CCB *cel_OptionsOverlay;
CCB *cel_OptionsMain;
CCB *cel_OptionsArrow;

CCB *cel_OptionGuides;
CCB *cel_OptionMusic;
CCB *cel_OptionSFX;
CCB *cel_OptionTheme;
CCB *cel_OptionColors;

CCB *cels_GPB[10][18]; 	// Potential screen block CELs
CCB *cels_AB[4]; 		// ActiveBlock
CCB *cels_NB[4]; 		// Next Block
CCB *cels_HB[4]; 		// Hold Block
CCB *cels_GB[4]; 		// Guide Blocks

CCB *cels_SM[4]; 		// Start Menu Blocks

CCB *cels_OM1[4];		// Options Menu Blocks
CCB *cels_OM2[4];		// Options Menu Blocks
CCB *cels_OM3[4];		// Options Menu Blocks
CCB *cels_OM4[4];		// Options Menu Blocks
CCB *cels_OM5[4];		// Options Menu Blocks
CCB *cels_OM6[4];		// Options Menu Blocks
CCB *cels_OM7[4]; 		// Options Menu Blocks

CCB *cel_PausedHdr;
CCB *cel_PausedOptions;

int HighScore = 50000;
int HighLevel = 15;
int TotScore = 0;
int CurrScore = 0;

int CurrLevel = 1;

int CurrLines = 0;
int TotLines = 0;

int TargetLines = 10;

static ubyte *backgroundBufferPtr1 = NULL;

static int isLast = 0;

void loadData()
{
	int x, y;
	
	// Keep in memory for frequent in-game usage	
	cel_GuideBlock = LoadCel("data/t9.cel", MEMTYPE_CEL);
	InitCCBFlags(cel_GuideBlock);
		
	cel_AllBlockImages[0] = LoadCel("data/block_teal.cel", MEMTYPE_CEL);
	cel_AllBlockImages[1] = LoadCel("data/block_red.cel", MEMTYPE_CEL);
	cel_AllBlockImages[2] = LoadCel("data/block_orange.cel", MEMTYPE_CEL);
	cel_AllBlockImages[3] = LoadCel("data/block_yellow.cel", MEMTYPE_CEL);
	cel_AllBlockImages[4] = LoadCel("data/block_green.cel", MEMTYPE_CEL);
	cel_AllBlockImages[5] = LoadCel("data/block_blue.cel", MEMTYPE_CEL);
	cel_AllBlockImages[6] = LoadCel("data/block_purple.cel", MEMTYPE_CEL);
	
	cel_AllBlockImages[7] = LoadCel("data/j1.cel", MEMTYPE_CEL);
	cel_AllBlockImages[8] = LoadCel("data/j2.cel", MEMTYPE_CEL);
	cel_AllBlockImages[9] = LoadCel("data/j3.cel", MEMTYPE_CEL);
	cel_AllBlockImages[10] = LoadCel("data/j4.cel", MEMTYPE_CEL);
	cel_AllBlockImages[11] = LoadCel("data/j5.cel", MEMTYPE_CEL);
	cel_AllBlockImages[12] = LoadCel("data/j6.cel", MEMTYPE_CEL);
	cel_AllBlockImages[13] = LoadCel("data/j7.cel", MEMTYPE_CEL);
	
	cel_AllBlockImages[14] = LoadCel("data/b1.cel", MEMTYPE_CEL);
	cel_AllBlockImages[15] = LoadCel("data/b2.cel", MEMTYPE_CEL);
	cel_AllBlockImages[16] = LoadCel("data/b3.cel", MEMTYPE_CEL);
	cel_AllBlockImages[17] = LoadCel("data/b4.cel", MEMTYPE_CEL);
	cel_AllBlockImages[18] = LoadCel("data/b5.cel", MEMTYPE_CEL);
	cel_AllBlockImages[19] = LoadCel("data/b6.cel", MEMTYPE_CEL);
	cel_AllBlockImages[20] = LoadCel("data/b7.cel", MEMTYPE_CEL);
	
	cel_AllBlockImages[21] = LoadCel("data/block_white.cel", MEMTYPE_CEL);
	cel_AllBlockImages[22] = LoadCel("data/block_grey.cel", MEMTYPE_CEL);
	cel_AllBlockImages[23] = LoadCel("data/block_black.cel", MEMTYPE_CEL);
	cel_AllBlockImages[24] = LoadCel("data/block_disc8.cel", MEMTYPE_CEL);
	
	cel_AllBlockImages[25] = LoadCel("data/block_disc1.cel", MEMTYPE_CEL);
	cel_AllBlockImages[26] = LoadCel("data/block_disc2.cel", MEMTYPE_CEL);
	cel_AllBlockImages[27] = LoadCel("data/block_disc3.cel", MEMTYPE_CEL);
	cel_AllBlockImages[28] = LoadCel("data/block_disc4.cel", MEMTYPE_CEL);
	cel_AllBlockImages[29] = LoadCel("data/block_disc5.cel", MEMTYPE_CEL);
	cel_AllBlockImages[30] = LoadCel("data/block_disc6.cel", MEMTYPE_CEL);
	cel_AllBlockImages[31] = LoadCel("data/block_disc7.cel", MEMTYPE_CEL);
	
	for (x = 0; x < 32; x++)
	{		
		InitCCBFlags(cel_AllBlockImages[x]);		
	}
	
	// Initialize the Gameplay Block CCBs
	for (x = 0; x < 10; x++)
	{
		for (y = 0; y < 18; y++)
		{
			cels_GPB[x][y] = CopyCel(cel_AllBlockImages[0]); // Doesn't matter which
			
			PositionCelColumn(cels_GPB[x][y], x + 8, y + 1, 4, 0); // Offset 8 columns from the screen origin, 1 down
		}
	}
	
	// Now chain them together
	for (x = 0; x < 10; x++)
	{
		for (y = 0; y < 17; y++)  
		{
			cels_GPB[x][y]->ccb_NextPtr = cels_GPB[x][y + 1]; // (CCB *)MakeCCBRelative( &cel-> ccb_NextPtr, &NextCel )		
		}

		if (x < 9)
		{
			cels_GPB[x][17]->ccb_NextPtr = cels_GPB[x + 1][0];
		}
	}
	
	for (x = 0; x < 4; x++)
	{
		cels_AB[x] = CopyCel(cel_AllBlockImages[0]);
		cels_NB[x] = CopyCel(cel_AllBlockImages[0]);
		cels_HB[x] = CopyCel(cel_AllBlockImages[0]);
		cels_GB[x] = CopyCel(cel_GuideBlock); // White - This never changes

		if (x > 0)
		{
			cels_AB[x - 1]->ccb_NextPtr = cels_AB[x];
			cels_NB[x - 1]->ccb_NextPtr = cels_NB[x];
			cels_HB[x - 1]->ccb_NextPtr = cels_HB[x];
			cels_GB[x - 1]->ccb_NextPtr = cels_GB[x];
		}
		
		cels_GB[x]->ccb_PIXC = 0x1f811f81; 
		
		SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_NB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_HB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP);
	}

	cels_GPB[9][17]->ccb_NextPtr = cels_AB[0];
	cels_AB[3]->ccb_NextPtr = cels_NB[0];
	cels_NB[3]->ccb_NextPtr = cels_HB[0];
	cels_HB[3]->ccb_NextPtr = cels_GB[0];
	cels_GB[3]->ccb_NextPtr = TrackedNumbers[0].cel_NumCels[0];
}

void initSPORTwriteValue(unsigned value)
{
	WaitVBL(vsyncItem, 1); // Prevent screen garbage presumably
	
    memset(&ioInfo,0,sizeof(ioInfo));
    ioInfo.ioi_Command = FLASHWRITE_CMD;
    ioInfo.ioi_CmdOptions = 0xffffffff;
    ioInfo.ioi_Offset = value; // background colour
    ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
    ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;	
}

void initSPORTcopyImage(ubyte *srcImage)
{	
	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = SPORTCMD_COPY;
	ioInfo.ioi_Offset = 0xffffffff; // mask
	ioInfo.ioi_Send.iob_Buffer = srcImage;
	ioInfo.ioi_Send.iob_Len = SCREEN_SIZE_IN_BYTES;
	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	ioInfo.ioi_Recv.iob_Len = SCREEN_SIZE_IN_BYTES;
	


	//WaitVBL(vsyncItem, 1); // Prevent screen garbage presumably
}

void initSPORT()
{
	VRAMIOReq = CreateVRAMIOReq(); // Obtain an IOReq for all SPORT operations
	
	SwapBackgroundImage("data/bgblack.img", -99);

	initSPORTcopyImage(backgroundBufferPtr1);
}

int lastImageIdx = -1;
int lastDefaultTheme = true;

void SwapBackgroundImage(char *file, int imgIdx)
{
	if (lastImageIdx != imgIdx || lastDefaultTheme != localDefaultTheme)
	{		
		lastImageIdx = imgIdx;
		lastDefaultTheme = localDefaultTheme;
		
		if (backgroundBufferPtr1 != NULL)
		{
			UnloadImage(backgroundBufferPtr1);
			backgroundBufferPtr1 = NULL;
		}

		backgroundBufferPtr1 = LoadImage(file, NULL, (VdlChunk **)NULL, &screen);
	}
}

void initGraphics()
{
	int i;

	CreateBasicDisplay(&screen, DI_TYPE_DEFAULT, SCREEN_PAGES);

	for(i = 0; i < SCREEN_PAGES; i++)
	{
		bitmapItems[i] = screen.sc_BitmapItems[i];
		bitmaps[i] = screen.sc_Bitmaps[i];
	}

	DisableVAVG(screen.sc_Screens[0]);
	DisableHAVG(screen.sc_Screens[0]);
	DisableVAVG(screen.sc_Screens[1]);
	DisableHAVG(screen.sc_Screens[1]);

	vsyncItem = GetVBLIOReq();
}

void initSystem()
{
    OpenGraphicsFolio();
	OpenMathFolio();
	OpenAudioFolio();
}

void setBackgroundColor(short color)
{
	ioInfo.ioi_Offset = (color << 16) | color;
}

int getFrameNum()
{
	return frameNum;
}

void DisplayBackgroundOnly()
{
	DisplayScreen(screen.sc_Screens[visibleScreenPage], 0);	
	visibleScreenPage = (1 - visibleScreenPage);

	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	DoIO(VRAMIOReq, &ioInfo);
}

void DisplayStartScreen()
{
	DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cels_SM[0]);

	DisplayScreen(screen.sc_Screens[visibleScreenPage], 0);	

	//WaitVBL(vsyncItem, 0);
	visibleScreenPage = (1 - visibleScreenPage);

	//WaitVBL(vsyncItem, 1);

	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	DoIO(VRAMIOReq, &ioInfo);
}

void DisplayOptionsScreen() 
{
	DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_OptionsOverlay);

	DisplayScreen(screen.sc_Screens[visibleScreenPage], 0);

	//WaitVBL(vsyncItem, 0);
	visibleScreenPage = (1 - visibleScreenPage);

	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	DoIO(VRAMIOReq, &ioInfo);
}

int32 lastSeconds = 0;
int32 lastDrawCels = 0;
int32 lastRoundTrip = 0;
int32 last60Time = 0;
int32 avgFrameMS = 0;
int32 frameCount = 0;
int32 msCount = 0;
int32 avgMS = 0;
int32 totElapsedMS = 0;

void DisplayGameplayScreen()
{
	frameCount++;
	
	if (frameCount >= 30)
	{
		if (debugMode > 0)
		{
			TimeVal tv60Elapsed;
			
			SampleSystemTimeTV(&dData.tvFrames60End);
			
			SubTimes(&dData.tvFrames60Start, &dData.tvFrames60End, &tv60Elapsed);	
			
			last60Time = tv60Elapsed.tv_Microseconds / 1000; // 30 frames should be around 500 I think
			
			SampleSystemTimeTV(&dData.tvFrames60Start);
		}
		
		frameCount = 0;
	}
	
	if (debugMode > 0)
	{		
		TimeVal tvCurrLoopElapsed;
		
		SampleSystemTimeTV(&dData.tvCurrLoopEnd);
		
		SubTimes(&dData.tvCurrLoopStart, &dData.tvCurrLoopEnd, &tvCurrLoopElapsed);
		
		msCount++;
		totElapsedMS += tvCurrLoopElapsed.tv_Microseconds / 1000;
		avgMS = totElapsedMS / msCount;
		
		if (msCount > 120)
		{
			msCount = totElapsedMS = 0;
		}
		
		SampleSystemTimeTV(&dData.tvCurrLoopStart);
	}
	
	SampleSystemTimeTV(&dData.tvRenderStart);	
	
	if (debugMode < 3) DrawGamePlayScreen();
	
	if (debugMode > 0)
	{		
		SetCelNumbers(0, debugMode);
		SetCelNumbers(1, lastSeconds);
		SetCelNumbers(2, lastDrawCels);
		SetCelNumbers(3, lastRoundTrip);
		SetCelNumbers(4, last60Time);
		SetCelNumbers(5, avgMS);
	}
	else
	{
		UpdateOnScreenStats();
	}
	
	SampleSystemTimeTV(&dData.tvDrawCelsStart);	
	
	if (debugMode == 2)
	{
		DrawScreenCels(screen.sc_Screens[visibleScreenPage], cels_GPB[0][0]); 
	}
	else
	{
		DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cels_GPB[0][0]); 
	}
	
	//displayMem(screen.sc_BitmapItems[ visibleScreenPage ]);
	
	SampleSystemTimeTV(&dData.tvDrawCelsEnd);	
	
	if (debugMode > 0)
	{
		TimeVal tvElapsed, tvDrawCels, tvRoundTrip;
		
		SubTimes(&dData.tvInit, &dData.tvDrawCelsEnd, &tvElapsed);	
		SubTimes(&dData.tvDrawCelsStart, &dData.tvDrawCelsEnd, &tvDrawCels);
		SubTimes(&dData.tvRenderEnd, &dData.tvDrawCelsEnd, &tvRoundTrip);
		
		lastSeconds = tvElapsed.tv_Seconds;
		lastDrawCels = tvDrawCels.tv_Microseconds;
		lastRoundTrip = tvRoundTrip.tv_Microseconds;
	}	
	
    DisplayScreen(screen.sc_Screens[visibleScreenPage], 0);
	
	visibleScreenPage = (1 - visibleScreenPage);

	ioInfo.ioi_Recv.iob_Buffer = bitmaps[visibleScreenPage]->bm_Buffer;
	DoIO(VRAMIOReq, &ioInfo);

	SampleSystemTimeTV(&dData.tvRenderEnd);	
}

void DrawGamePlayScreen()
{
	int x, y, gbOffset;

	int minY = 99; // TODO SET MIN Y ON DROP
	
	if (IsPaused || RenderGameBlocks == false)
	{
		for (x = 0; x < 4; x++)
		{
			SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP); // Guide blocks off by default
			SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP);
		}
		
		for (x = 0; x < 10; x++)
		{
			for (y = 0; y < 18; y++)
			{
				SetFlag(cels_GPB[x][y]->ccb_Flags, CCB_SKIP);
			}
		}
		
		return;
	}

	if (ClearingLines == false) // TODO OPTIMIZE THIS
	{
		for (x = 0; x < 10; x++)
		{
			for (y = 0; y < 18; y++)
			{
				if (GamePlayBlocks[x][y] == true) // 10x19 Grid Blocks
				{
					if (y < minY) minY = y; // For guide blocks

					ClearFlag(cels_GPB[x][y]->ccb_Flags, CCB_SKIP);
				}
				else
				{
					SetFlag(cels_GPB[x][y]->ccb_Flags, CCB_SKIP); // TODO PUT BACK
				}
			}
		}

		for (x = 0; x < 4; x++) // Activeblock is the Tetrimino / state
		{
			PositionCelColumn(cels_AB[x], ActiveBlock.Blocks[x].X + 8, ActiveBlock.Blocks[x].Y + 1, 4, 0); // +1 for the offset from 20 blocks to 19

			ClearFlag(cels_AB[x]->ccb_Flags, CCB_SKIP);
		}
	}

	if (localShowGuides)
	{
		if (IsPaused == false && ClearingLines == false) // Guide Blocks First 2 Levels TODO Configure
		{
			gbOffset = 0;

			while (TryMoveDown() && gbOffset < 20) // Failsafe
			{
				MoveDown(false);

				gbOffset++;
			}

			if (gbOffset > 0)
			{
				for (x = 0; x < gbOffset; x++)
				{
					MoveUp();
				}
			}

			if (gbOffset > 1)
			{
				for (x = 0; x < 4; x++)
				{
					PositionCelColumn(cels_GB[x], ActiveBlock.Blocks[x].X + 8, ActiveBlock.Blocks[x].Y + gbOffset + 1, 4, 0);

					ClearFlag(cels_GB[x]->ccb_Flags, CCB_SKIP); // Guide blocks off by default
				}
			}
			else
			{
				for (x = 0; x < 4; x++)
				{
					SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP); // Guide blocks off by default
				}
			}
		}
	}
}

void UpdateOnScreenStats()
{
	int rem = TargetLines - CurrLines;

	if (rem < 0) rem = 0;

	SetCelNumbers(0, HighScore);
	SetCelNumbers(1, HighLevel);
	SetCelNumbers(2, TotScore);
	SetCelNumbers(3, CurrLines);
	SetCelNumbers(4, rem);
	SetCelNumbers(5, CurrLevel);

	//RenderCelNumbers(screen.sc_BitmapItems[ visibleScreenPage ]); // TODO - no need for a separate call here, just chain the exposed CCB
}

// For Konami Code
int mapJoyBits(uint32 joyBits)
{
	if (joyBits & ControlUp)			return UP;
	if (joyBits & ControlDown) 			return DN;
	if (joyBits & ControlLeft) 			return LEFT;
	if (joyBits & ControlRight) 		return RIGHT;
	if (joyBits & ControlA) 			return A;
	if (joyBits & ControlB) 			return B;
	if (joyBits & ControlC) 			return C;
	if (joyBits & ControlLeftShift)		return LT;
	if (joyBits & ControlRightShift)	return RT;
	if (joyBits & ControlX) 			return SEL;

	return START;
}

// Handle Konami Code

//uint32 eeHighMatch = 0x1122;
//uint32 eeLowMatch = 0x34346565;

int32 HandleEEInput(uint32 joyBits)
{
	uint32 b;
	button = mapJoyBits(joyBits);

	if (!button) // Start button
	{
		// check the state
		if ((eeLowMatch & eeLow) == eeLowMatch && (eeHighMatch & eeHigh) == eeHighMatch)
		{
			eeLow = eeHigh = 0;

			return 1;
		}

		eeLow = eeHigh = 0;
	}

	// save off the high 4 bits
	b = eeLow >> 28;
	eeHigh = (eeHigh << 4) | b;
	eeLow = (eeLow << 4) | button;

	return 0;
}

static ControlPadEventData cped;

void HandleInput()
{
	int x;
	uint32 joyBits;
	
	GetControlPad(1, 0, &cped); //  

	joyBits = cped.cped_ButtonBits;

	if (OptionsMenuSelected == true) // Options Menu
	{
		HandleInputOptionsMenu(joyBits);
		
		return;
	}
	else if (GameStarted == false) // Start Menu
	{
		HandleInputStartMenu(joyBits);
		
		return;
	}

	if (AcceptGameInput == false)
	{
		cped.cped_ButtonBits = 0;
		
		return;
	}
	
	// In gameplay mode now
	
	if (IsPaused && (joyBits & ControlLeftShift) && (joyBits & ControlRightShift))
	{
		GameOver = true;
		GameStarted = false;

		QuickReset = true;
		
		return;
	}
	
	if (joyBits & ControlX) // Reset
	{
		if (kpStop == false)
		{
			if (IsPaused == true)
			{
				ToggleOptionsMenu(true);
			}
			else
			{
				TogglePaused(true);
			}
		}

		kpStop = true;
	}
	else
	{
		kpStop = false;
	}

	if (joyBits & ControlStart)
	{
		if (kpStart == false)
		{
			TogglePaused(!IsPaused);
		}

		kpStart = true;
	}
	else
	{
		kpStart = false;
	}

	if (IsPaused) return;

	if (joyBits & ControlLeftShift) // Hold block
	{
		if (kpLS == false && CanHold == true)
		{
			SwapActiveBlockWithHeldBlock();
		}

		kpLS = true;
		swLS++;

		bPresses = 0;
	}
	else
	{
		swLS = 0;
		kpLS = false;
	}

	if (joyBits & ControlRightShift) // Queue Swap? Kind of fun
	{
		if (kpRS == false && QueueSwaps > 0)
		{
			QueueNextBlock();

			QueueSwaps--;
		}

		kpRS = true;
		swRS++;

		if (swRS >= 60) swRS = 0;

		bPresses = 0;
	}
	else
	{
		swRS = 0;
		kpRS = false;
	}

	if (joyBits & ControlA)
	{
		if (kpA == false || ++swA >= 15)
		{
			if (TryRotate(true) == true)
			{
				Rotate(true);
			}
			else if (AllowShiftToRotate == true) // Try to move over 1 then rotate if you allowed. Otherwise undo
			{
				bool didRotate = false;
				
				if (TryMoveRight()) // TODO THIS NEEDS SOME WORK
				{
					MoveRight();

					if (TryRotate(true) == true)
					{
						Rotate(true);
						
						didRotate = true;
					}
					else
					{
						MoveLeft(); // Undo
					}
				}
				else if (TryMoveLeft())
				{
					MoveLeft();
					
					if (TryRotate(true) == true)
					{
						Rotate(true);
						
						didRotate = true;
					}
					else
					{
						MoveRight(); // Undo
					}
				}
				
				if (didRotate == false)
				{
					if (TryMoveUp())
					{
						MoveUp();
						
						if (TryRotate(true) == true)
						{
							Rotate(true);
						}
						else
						{
							MoveDown(false); // Undo
						}
					}
				}
			}
		}

		kpA = true;

		if (swA >= 15) swA = 0;

		bPresses = 0;
		
		if (debugMode > 1) debugMode = 0;
	}
	else
	{
		kpA = false;
		swA = 0;
	}

	if (joyBits & ControlB)
	{
		if (kpB == false)
		{
			bPresses++;

			if (bPresses >=10)
			{
				debugMode++;
				
				if (bPresses >= 20)
				{
					debugMode = 0;
				}
			}
		}

		kpB = true;
	}
	else
	{
		kpB = false;
		swB = 0;
	}

	if (joyBits & ControlC)
	{
		if (kpC == false || ++swC >= 15)
		{
			if (TryRotate(false) == true)
			{
				Rotate(false);
			}
			else if (AllowShiftToRotate == true) // Try to move over 1 then rotate if you allowed. Otherwise undo
			{
				bool didRotate = false;
				
				if (TryMoveRight()) // TODO THIS NEEDS SOME WORK
				{
					MoveRight();

					if (TryRotate(false) == true)
					{
						Rotate(false);
						
						didRotate = true;
					}
					else
					{
						MoveLeft(); // Undo
					}
				}
				else if (TryMoveLeft())
				{
					MoveLeft();
					
					if (TryRotate(false) == true)
					{
						Rotate(false);
						
						didRotate = true;
					}
					else
					{
						MoveRight(); // Undo
					}
				}
				
				if (didRotate == false)
				{
					if (TryMoveUp())
					{
						MoveUp();
						
						if (TryRotate(false) == true)
						{
							Rotate(false);
						}
						else
						{
							MoveDown(false); // Undo
						}
					}
				}
			}
		}

		kpC = true;

		if (swC >= 15) swC = 0;

		bPresses = 0;
		
		if (debugMode > 1) debugMode = 0;
	}
	else
	{
		kpC = false;
		swC = 0;
	}

	if (joyBits & ControlUp)
	{
		if (kpUp == false || ++swUp >= 15) // Check if can move down obviously
		{
			if (TryMoveDown() == true)
			{
				PlaySFX(SFX_DROP); 
				
				while (TryMoveDown() == true)
				{
					MoveDown(true);
				}
			}

			swGame = lvSpeed; // Lock in the piece
			
			TotScore = TotScore + 25;
		}

		kpUp = true;

		if (swUp >= 15) swUp = 0;

		bPresses = 0;
	}
	else
	{
		swUp = 0;
		kpUp = false;
	}

	if (joyBits & ControlDown)
	{
		if (kpDown == false || ++swDown >= 3) // Check if can move down obviously
		{
			if (TryMoveDown() == true)
			{
				MoveDown(true);
			}
			else if (kpDown == false)
			{
				swGame = lvSpeed; // Lock in the piece
			}
		}

		kpDown = true;

		if (swDown >= 3) swDown = 0;

		bPresses = 0;
	}
	else
	{
		swDown = 0;
		kpDown = false;
	}

	if (joyBits & ControlLeft)
	{
		if (kpLeft == false || (swLeft >= 8 && movingLeft == false) || (swLeft >= 4 && movingLeft == true)) // Slightly stagger the increase speed
		{
			if (TryMoveLeft() == true)
			{
				MoveLeft();
			}
		}

		kpLeft = true;
		swLeft++;

		if (movingLeft && swLeft > 4) swLeft = 0;

		if (movingLeft == false && swLeft > 8)
		{
			movingLeft = true;
		}

		 bPresses = 0;
	}
	else
	{
		swLeft = 0;
		kpLeft = false;
		movingLeft = false;
	}

	if (joyBits & ControlRight)
	{
		if (kpRight == false || (swRight >= 8 && movingRight == false) || (swRight >= 4 && movingRight == true)) // Slightly stagger the increase speed
		{
			if (TryMoveRight() == true)
			{
				MoveRight();
			}
		}

		kpRight = true;
		swRight++;

		if (movingRight && swRight > 4) swRight = 0;

		if (movingRight == false && swRight > 8)
		{
			movingRight = true;
		}

		bPresses = 0;
	}
	else
	{
		swRight = 0;
		kpRight = false;
		movingRight = false;
	}
}

void HandleInputOptionsMenu(uint32 joyBits)
{
	if (joyBits & ControlUp)
	{
		if (kpUp == false)
		{
			ToggleOptionsMenuSelection(1);
		}

		kpUp = true;
	}
	else
	{
		kpUp = false;
	}

	if (joyBits & ControlDown)
	{
		if (kpDown == false)
		{
			ToggleOptionsMenuSelection(2);
		}

		kpDown = true;
	}
	else
	{
		kpDown = false;
	}

	if (joyBits & ControlLeft)
	{
		if (kpLeft == false)
		{
			ToggleOptionsMenuSelection(3);
		}

		kpLeft = true;
	}
	else
	{
		kpLeft = false;
	}

	if (joyBits & ControlRight)
	{
		if (kpRight == false)
		{
			ToggleOptionsMenuSelection(4);
		}

		kpRight = true;
	}
	else
	{
		kpRight = false;
	}

	if (joyBits & ControlStart) // Save and Close
	{
		if (kpStart == false)
		{
			HandleSelectedOptions();

			ToggleOptionsMenu(false);
		}

		kpStart = true;
	}
	else
	{
		kpStart = false;
	}

	if (joyBits & ControlX) // Cancel and Close
	{
		if (kpStop == false)
		{
			ToggleOptionsMenu(false);
		}

		kpStop = true;
	}
	else
	{
		kpStop = false;
	}
}

void HandleInputStartMenu(uint32 joyBits)
{
	if (joyBits & ControlUp)
	{
		if (kpUp == false)
		{
			ToggleStartMenuSelection();

			HandleEEInput(joyBits);
		}

		kpUp = true;
	}
	else
	{
		kpUp = false;
	}

	if (joyBits & ControlDown)
	{
		if (kpDown == false)
		{
			ToggleStartMenuSelection();

			HandleEEInput(joyBits);
		}

		kpDown = true;
	}
	else
	{
		kpDown = false;
	}

	if (joyBits & ControlLeft)
	{
		if (kpLeft == false)
		{
			HandleEEInput(joyBits);
		}

		kpLeft = true;
	}
	else
	{
		kpLeft = false;
	}

	if (joyBits & ControlRight)
	{
		if (kpRight == false)
		{
			HandleEEInput(joyBits);
		}

		kpRight = true;
	}
	else
	{
		kpRight = false;
	}

	if (joyBits & ControlA)
	{
		if (kpA == false)
		{
			HandleEEInput(joyBits);
		}

		kpA = true;
	}
	else
	{
		kpA = false;
	}

	if (joyBits & ControlB)
	{
		if (kpB == false)
		{
			HandleEEInput(joyBits);
		}

		kpB = true;
	}
	else
	{
		kpB = false;
	}

	if (joyBits & ControlStart)
	{
		if (kpStart == false)
		{
			kpStart = true;

			if (HandleEEInput(joyBits)) // Enable Konami code
			{
				EasterEggEnabled = true; 
				
				localMainPalette = 6;
				
				ApplySelectedColorPalette();

				cels_SM[0]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_3DO]->ccb_SourcePtr;
				cels_SM[1]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_3DO]->ccb_SourcePtr;
				cels_SM[2]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_3DO]->ccb_SourcePtr;
				cels_SM[3]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_3DO]->ccb_SourcePtr;

				return;
			}

			if (smStartSelected == true)
			{				
				GameStarted = true;
			}
			else
			{
				ToggleOptionsMenu(true);
			}
		}
	}
	else
	{
		kpStart = false;
	}
}

void QueueNextBlock()
{
	int x;
	int rNum = rand() % 7;
	
	if (rNum == QueuedShapeIdx)
	{
		rNum = rand() % 7; // Roll the dice again. Doesn't guarantee no duplicates but odds are it will reduce them
	}

	for (x = 0; x < 4; x++)
	{
		cels_NB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[rNum]]->ccb_SourcePtr; // cel_BlockYellow->ccb_SourcePtr;

		PositionCelColumn(cels_NB[x], DefaultBlockCoords[rNum][x].X - (rNum <= 1 ? 1 : 0), DefaultBlockCoords[rNum][x].Y, (rNum <= 1 ? 8 : 2), (rNum == 0 ? 11 : 5)); // Ajustments to center the I-Block and O-Block

		ClearFlag(cels_NB[x]->ccb_Flags, CCB_SKIP);
	}

	QueuedShapeIdx = rNum;
}

void LoadNextBlockFromQueue() // Convert from queued block pixel position to column position, X offset 18 / 4
{
	int x;

	for (x = 0; x < 4; x++)
	{
		cels_AB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[QueuedShapeIdx]]->ccb_SourcePtr; // cels_NB[x]->ccb_SourcePtr;

		ActiveBlock.Blocks[x].X = DefaultBlockCoords[QueuedShapeIdx][x].X - 18;
		ActiveBlock.Blocks[x].Y = DefaultBlockCoords[QueuedShapeIdx][x].Y - 5;

		SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP); // Visibility will be set if needed
		SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP);
	}

	ActiveBlock.PivotIdx = BlockPivotIdx[QueuedShapeIdx];

	ActiveBlock.ShapeType = QueuedShapeIdx;

	QueueNextBlock();
}

void HoldBlock() // Save current block to hold block
{
	int x;

	//PlaySample(gSampleHold);

	for (x = 0; x < 4; x++)
	{
		cels_HB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[ActiveBlock.ShapeType]]->ccb_SourcePtr; // cel_BlockYellow->ccb_SourcePtr;

		PositionCelColumn(cels_HB[x], DefaultBlockCoords[ActiveBlock.ShapeType][x].X - 19, DefaultBlockCoords[ActiveBlock.ShapeType][x].Y, (ActiveBlock.ShapeType <= 1 ? 0 : 6), (ActiveBlock.ShapeType == 0 ? 11 : 5)); // Ajustments to center the I-Block and O-Block

		ClearFlag(cels_HB[x]->ccb_Flags, CCB_SKIP);
	}

	HeldShapeIdx = ActiveBlock.ShapeType; 

	CanHold = false; // Can't swap until next turn
}

void SwapActiveBlockWithHeldBlock()
{
	int x;
	int heldShape = HeldShapeIdx;
	int activeShape = ActiveBlock.ShapeType;
	
	PlaySFX(SFX_HOLD); 

	HoldBlock();

	if (heldShape >= 0)
	{
		for (x = 0; x < 4; x++)
		{
			cels_AB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[heldShape]]->ccb_SourcePtr;

			ActiveBlock.Blocks[x].X = DefaultBlockCoords[heldShape][x].X - 18;
			ActiveBlock.Blocks[x].Y = DefaultBlockCoords[heldShape][x].Y - 4;

			SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP); // Visibility will be set if needed
		}

		ActiveBlock.PivotIdx = BlockPivotIdx[heldShape];

		ActiveBlock.ShapeType = heldShape;
	}
	else
	{
		LoadNextBlockFromQueue();
	}
}

bool TryRotate(bool counterClockwise) // Build new block in position.. See if any conflicts
{
	int x, pivotX, pivotY, offsetX, offsetY, newX, newY;
	bool collision = false;

	pivotX = ActiveBlock.Blocks[ActiveBlock.PivotIdx].X;
	pivotY = ActiveBlock.Blocks[ActiveBlock.PivotIdx].Y;

	if (ActiveBlock.PivotIdx < 0) return false;

	for (x = 0; x < 4; x++)
	{
		offsetX = ActiveBlock.Blocks[x].X - pivotX; // Current X Position Relative to Pivot Point
		offsetY = ActiveBlock.Blocks[x].Y - pivotY; // Current Y Position Relative to Pivot Point

		newX = pivotX + ( offsetY * ( counterClockwise ? 1 : -1 ) );
		newY = pivotY - ( offsetX * ( counterClockwise ? 1 : -1 ) );

		if (newX < 0 || newX > 9) // TODO MAYBE SHIFT LEFT OR RIGHT IF OPEN
		{
			collision = true;
		}
		else if ( newY > 17)
		{
			collision = true;
		}
		else if (newY >= 0 && GamePlayBlocks[newX][newY] == true)
		{
			collision = true;
		}

		if (collision == true) break;
	}

	return !collision;
}

void Rotate(bool counterClockwise)
{
	int x, pivotX, pivotY, offsetX, offsetY;
	
	pivotX = ActiveBlock.Blocks[ActiveBlock.PivotIdx].X;
	pivotY = ActiveBlock.Blocks[ActiveBlock.PivotIdx].Y;

	if (ActiveBlock.PivotIdx < 0) return;

	for (x = 0; x < 4; x++)
	{
		offsetX = ActiveBlock.Blocks[x].X - pivotX; // Current X Position Relative to Pivot Point
		offsetY = ActiveBlock.Blocks[x].Y - pivotY; // Current Y Position Relative to Pivot Point

		ActiveBlock.Blocks[x].X = pivotX + ( offsetY * ( counterClockwise ? 1 : -1 ) );
		ActiveBlock.Blocks[x].Y = pivotY - ( offsetX * ( counterClockwise ? 1 : -1 ) );
	}
	
	if (swGame + 12 >= lvSpeed) // Provide a little more time for last second adjustments if need be
	{
		swGame = swGame - 12; 
		
		if (swGame < 0) swGame = 0;
	}
}

bool TryMoveLeft()
{
	if (ActiveBlock.Blocks[0].X == 0) return false;
	if (ActiveBlock.Blocks[1].X == 0) return false;
	if (ActiveBlock.Blocks[2].X == 0) return false;
	if (ActiveBlock.Blocks[3].X == 0) return false;
	
	if (ActiveBlock.Blocks[0].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[0].X - 1][ActiveBlock.Blocks[0].Y] == true) return false;
	if (ActiveBlock.Blocks[1].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[1].X - 1][ActiveBlock.Blocks[1].Y] == true) return false;
	if (ActiveBlock.Blocks[2].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[2].X - 1][ActiveBlock.Blocks[2].Y] == true) return false;
	if (ActiveBlock.Blocks[3].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[3].X - 1][ActiveBlock.Blocks[3].Y] == true) return false;
	
	return true;
}

bool TryMoveRight()
{
	if (ActiveBlock.Blocks[0].X == 9) return false;
	if (ActiveBlock.Blocks[1].X == 9) return false;
	if (ActiveBlock.Blocks[2].X == 9) return false;
	if (ActiveBlock.Blocks[3].X == 9) return false;
	
	if (ActiveBlock.Blocks[0].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[0].X + 1][ActiveBlock.Blocks[0].Y] == true) return false;
	if (ActiveBlock.Blocks[1].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[1].X + 1][ActiveBlock.Blocks[1].Y] == true) return false;
	if (ActiveBlock.Blocks[2].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[2].X + 1][ActiveBlock.Blocks[2].Y] == true) return false;
	if (ActiveBlock.Blocks[3].Y >= 0 && GamePlayBlocks[ActiveBlock.Blocks[3].X + 1][ActiveBlock.Blocks[3].Y] == true) return false;
	
	return true;
}

bool TryMoveUp()
{
	if (ActiveBlock.Blocks[0].Y <= 0) return false;
	if (ActiveBlock.Blocks[1].Y <= 0) return false;
	if (ActiveBlock.Blocks[2].Y <= 0) return false;
	if (ActiveBlock.Blocks[3].Y <= 0) return false;
	
	if (GamePlayBlocks[ActiveBlock.Blocks[0].X][ActiveBlock.Blocks[0].Y - 1] == true) return false;
	if (GamePlayBlocks[ActiveBlock.Blocks[1].X][ActiveBlock.Blocks[1].Y - 1] == true) return false;
	if (GamePlayBlocks[ActiveBlock.Blocks[2].X][ActiveBlock.Blocks[2].Y - 1] == true) return false;
	if (GamePlayBlocks[ActiveBlock.Blocks[3].X][ActiveBlock.Blocks[3].Y - 1] == true) return false;
	
	return true;
}

bool TryMoveDown()
{
	if (ActiveBlock.Blocks[0].Y == 17) return false;
	if (ActiveBlock.Blocks[1].Y == 17) return false;
	if (ActiveBlock.Blocks[2].Y == 17) return false;
	if (ActiveBlock.Blocks[3].Y == 17) return false;
	
	if (ActiveBlock.Blocks[0].Y >= -1 && GamePlayBlocks[ActiveBlock.Blocks[0].X][ActiveBlock.Blocks[0].Y + 1] == true) return false;
	if (ActiveBlock.Blocks[1].Y >= -1 && GamePlayBlocks[ActiveBlock.Blocks[1].X][ActiveBlock.Blocks[1].Y + 1] == true) return false;
	if (ActiveBlock.Blocks[2].Y >= -1 && GamePlayBlocks[ActiveBlock.Blocks[2].X][ActiveBlock.Blocks[2].Y + 1] == true) return false;
	if (ActiveBlock.Blocks[3].Y >= -1 && GamePlayBlocks[ActiveBlock.Blocks[3].X][ActiveBlock.Blocks[3].Y + 1] == true) return false;
	
	return true;
}

void MoveDown(bool resetGameSW)
{
	int x;
	
	for (x = 0; x < 4; x++)
	{
		ActiveBlock.Blocks[x].Y++;
	}

	if (resetGameSW) swGame = 0;
}

void MoveUp()
{
	int x;
	
	for (x = 0; x < 4; x++)
	{
		ActiveBlock.Blocks[x].Y--;
	}
}

void MoveLeft()
{
	int x;

	for (x = 0; x < 4; x++)
	{
		ActiveBlock.Blocks[x].X--;
	}
}

void MoveRight()
{
	int x;

	for (x = 0; x < 4; x++)
	{
		ActiveBlock.Blocks[x].X++;
	}
}

void ToggleOptionsMenuSelection(int udlr)
{
	int x, selImgIdx, dir;
	
	PlaySFX(SFX_TICK);

	if (udlr == 1) 		// Up
	{
		HighlightedOption--;

		if (HighlightedOption < 0) HighlightedOption = 4;
	}
	else if (udlr == 2) // Down
	{
		HighlightedOption++;

		if (HighlightedOption > 4) HighlightedOption = 0;
	}
	else if (udlr == 3 || udlr == 4) // Left
	{
		if (HighlightedOption == 0)
		{
			OptionsShowGuides = !OptionsShowGuides;

			cel_OptionGuides->ccb_SourcePtr = OptionsShowGuides ? cel_AllBlockImages[BLOCK_RED]->ccb_SourcePtr : cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr;
		}
		else if (HighlightedOption == 1)
		{
			OptionsPlayMusic = !OptionsPlayMusic;

			cel_OptionMusic->ccb_SourcePtr = OptionsPlayMusic ? cel_AllBlockImages[BLOCK_RED]->ccb_SourcePtr : cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr;
		}
		else if (HighlightedOption == 2)
		{
			OptionsPlaySFX = !OptionsPlaySFX;

			cel_OptionSFX->ccb_SourcePtr = OptionsPlaySFX ? cel_AllBlockImages[BLOCK_RED]->ccb_SourcePtr : cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr;
		}
		else if (HighlightedOption == 3)
		{
			OptionsDefaultTheme = !OptionsDefaultTheme;

			cel_OptionTheme->ccb_SourcePtr = OptionsDefaultTheme ? cel_AllBlockImages[BLOCK_RED]->ccb_SourcePtr : cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr;
		}
		else
		{
			if (EasterEggEnabled == false)
			{
				dir = udlr == 3 ? -1 : 1;

				OptionsMainPalette+= dir;

				if (OptionsMainPalette < 0) OptionsMainPalette = 5;
				if (OptionsMainPalette > 5) OptionsMainPalette = 0;

				for (x = 0; x < 4; x++)
				{
					cels_OM1[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][0]]->ccb_SourcePtr;
					cels_OM2[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][1]]->ccb_SourcePtr;
					cels_OM3[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][2]]->ccb_SourcePtr;
					cels_OM4[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][3]]->ccb_SourcePtr;
					cels_OM5[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][4]]->ccb_SourcePtr;
					cels_OM6[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][5]]->ccb_SourcePtr;
					cels_OM7[x]->ccb_SourcePtr = cel_AllBlockImages[Palettes[OptionsMainPalette][6]]->ccb_SourcePtr;
				}
			}
		}
	}
}

void ToggleStartMenuSelection()
{	
	smStartSelected = !smStartSelected;

	if (smStartSelected)
	{		
		PositionCel(cels_SM[0], 96, 123);
		PositionCel(cels_SM[1], 108, 123);
		PositionCel(cels_SM[2], 96, 135);
		PositionCel(cels_SM[3], 108, 135);
	}
	else
	{		
		PositionCel(cels_SM[0], 96, 153);
		PositionCel(cels_SM[1], 108, 153);
		PositionCel(cels_SM[2], 96, 165); 
		PositionCel(cels_SM[3], 108, 165);
	}
}

void HandleOptionsMenuLogic() // Changes after 4, 6, 8
{
	PositionCel(cel_OptionsArrow, 50, 41 + (HighlightedOption * 13));

	PositionCel(cel_OptionGuides, OptionsShowGuides ? 200 : 232, 38);
	PositionCel(cel_OptionMusic, OptionsPlayMusic ? 200 : 232, 51);
	PositionCel(cel_OptionSFX, OptionsPlaySFX ? 200 : 232, 64);
	PositionCel(cel_OptionTheme, OptionsDefaultTheme ? 200 : 232, 77);
}

void HandleSelectedOptions()
{
	localShowGuides = OptionsShowGuides;
	localPlaySFX = OptionsPlaySFX;
	
	if (localDefaultTheme != OptionsDefaultTheme)
	{
		localDefaultTheme = OptionsDefaultTheme;
		
		if (GameStarted == true)
		{
			ApplyCurrentThemeBackground();
		}
	}		
	
	if (localMainPalette != OptionsMainPalette)
	{
		if (EasterEggEnabled == false)
		{
			localMainPalette = OptionsMainPalette;
			ApplySelectedColorPalette();
		}
	}
	
	if (OptionsPlayMusic == false && localPlayMusic == true)
	{
		localPlayMusic = false;

		stopspoolsound(5); // Fade out
	}
	else if (OptionsPlayMusic == true && localPlayMusic == false)
	{
		localPlayMusic = true;

		PlayBackgroundMusic();
	}	
}

void ApplySelectedColorPalette()
{
	int x, y;
	int cbIdx = 0; // for randomly re-assigning colors to game board
	
	for (x = 0; x < 7; x++)
	{
		BlockImageIdx[x] = Palettes[localMainPalette][x]; // OptionsMainPalette
	}

	for (x = 0; x < 10; x++)
	{
		for (y = 0; y < 18; y++)
		{
			if (GamePlayBlocks[x][y] == true)
			{
				cels_GPB[x][y]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[cbIdx]]->ccb_SourcePtr; // ASSIGN FROM SELECTED IMAGE IDX ARRAY

				if (++cbIdx > 6) cbIdx = 0;
			}
		}
	}

	for (x = 0; x < 4; x++)
	{
		cels_AB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[ActiveBlock.ShapeType]]->ccb_SourcePtr;

		if (HeldShapeIdx >= 0) cels_HB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[HeldShapeIdx]]->ccb_SourcePtr;
		if (QueuedShapeIdx >= 0) cels_NB[x]->ccb_SourcePtr = cel_AllBlockImages[BlockImageIdx[QueuedShapeIdx]]->ccb_SourcePtr;

		if (localShowGuides == true)
		{
			ClearFlag(cels_GB[x]->ccb_Flags, CCB_SKIP);
		}
		else
		{
			SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP);
		}
	}
}

void ShowIntroSplash()
{
	int x;
	
	SwapBackgroundImage("data/bgblack.img", -99);
	
	DrawImage( screen.sc_Screens[ visibleScreenPage ], backgroundBufferPtr1, &screen );
	
	DisplayScreen( screen.sc_Screens[ visibleScreenPage ], 0);
	
	FadeToBlack( &screen, 30 );
	
	SwapBackgroundImage("data/hdsplash.img", -98);
	
	DrawImage( screen.sc_Screens[ visibleScreenPage ], backgroundBufferPtr1, &screen );
	
	DisplayScreen( screen.sc_Screens[ visibleScreenPage ], 0);
	
	FadeFromBlack( &screen, 60 );	
	
	for (x = 0; x < 180; x++) // Display 3DO HD logo for 3 seconds
	{
		WaitVBL(vsyncItem, 1);
	}
	
	FadeToBlack( &screen, 60 );

	SwapBackgroundImage("data/bgmain.img", 0);
	
	DrawImage( screen.sc_Screens[ visibleScreenPage ], backgroundBufferPtr1, &screen );
	
	DisplayScreen( screen.sc_Screens[ visibleScreenPage ], 0);
	
	FadeFromBlack( &screen, 60 );	
}

void HandleStartMenuLogic() // Nothing to do
{

}

void HandleGameplayLogic()
{
	int x, abx, aby;

	bool collision = false;

	int localTimer = lvSpeed;

	if (IsPaused == true)
	{
		PauseScreen();

		return;
	}

	if (debugMode >= 2) return;

	if (TryMoveDown() == false)
	{
		if (localTimer < 10) localTimer = 10; // Allow for bottom last second adjustments
	}

	if (++swGame >= localTimer) // Start at 60 or once per second
	{
		collision = TryMoveDown() == false;

		if (collision == true) // As it needs to check each block, run this loop independently first
		{
			for (x = 0; x < 4; x++) // For the current Tetrimino, check all blocks to see if any above the limit
			{
				if (ActiveBlock.Blocks[x].Y < 0)
				{
					GameOver = true;
					AcceptGameInput = false;

					break;
				}
			}
		}

		if (collision == true && GameOver == false)
		{
			for (x = 0; x < 4; x++) // For the current Tetrimino, check all blocks to see if any above the limit
			{
				abx = ActiveBlock.Blocks[x].X;
				aby = ActiveBlock.Blocks[x].Y;

				SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP); // Immediately hide, no?

				if (aby >= 0 && aby <= 17) // TODO check this range
				{
					GamePlayBlocks[abx][aby] = true; // Confusing by the Active Block Y is offset 2

					cels_GPB[abx][aby]->ccb_SourcePtr = cels_AB[x]->ccb_SourcePtr; // Change board block color to collided piece color
					ClearFlag(cels_GPB[abx][aby]->ccb_Flags, CCB_SKIP); // Make that block visible and prevent flicker
				}
			}
		}

		if (collision == false) // Move Down
		{
			MoveDown(false);
		}

		swGame = 0;
	}

	if (collision == true)
	{
		if (GameOver == true)
		{
			PlaySFX(SFX_GAMEOVER);
			
			GameOverKillBlocks();
		}
		else
		{
			TotScore = TotScore + 25;
			
			Explode(); // Or check if explosion is necessary.. then explode

			CheckForNextLevel();

			LoadNextBlockFromQueue();

			CanHold = true;

			swUp = 0;
			swDown = 0;
		}
	}
}

void Explode()
{
	bool solidRow;

	int x, y;

	int fullRows[4] = { -1, -1, -1, -1 };

	int fullRowCount = 0;

	for (y = 0; y < 18; y++)
	{
		solidRow = true;

		for (x = 0; x < 10; x++)
		{
			if (GamePlayBlocks[x][y] == false)
			{
				solidRow = false;

				break;
			}
		}

		if (solidRow == true)
		{
			fullRows[fullRowCount] = y;

			fullRowCount++;
		}
	}

	if (fullRowCount == 0) return;
	
	if (fullRowCount == 4)
	{
		PlaySFX(SFX_CLEAR4);
	}
	else
	{
		PlaySFX(SFX_CLEAR);
	}
	
	// Scope out the necessary variables
	{ 
		int i, f;
		
		TotScore = (TotScore + (fullRowCount * (fullRowCount * 25)));

		CurrLines += fullRowCount;
		TotLines += fullRowCount;

		ClearingLines = true; // Stop the main Gameplay animation

		for (i = 0; i < fullRowCount; i++) // Make White?
		{
			for (x = 0; x < 10; x++)
			{
				GamePlayBlocks[x][fullRows[i]] = false; // Will this make the new blocks go away
			}
		}

		// BEGIN ROW CLEAR FX		

		if (fullRowCount == 1) // Nothing
		{
			for (i = 0; i < fullRowCount; i++) // Blow up any full row
			{
				for (x = 0; x < 10; x++)
				{
					SetFlag(cels_GPB[x][fullRows[i]]->ccb_Flags, CCB_SKIP);
				}
			}

			for (f = 0; f < 15; f++) // TODO BASED ON TIMER
			{
				DisplayGameplayScreen();
			}
		}
		else if (fullRowCount == 2) // Flash grey and disappear
		{
			for (i = 0; i < fullRowCount; i++) // Blow up any full row
			{
				for (x = 0; x < 10; x++)
				{
					cels_GPB[x][fullRows[i]]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr; // TODO Whatever gray is
					ClearFlag(cels_GPB[x][fullRows[i]]->ccb_Flags, CCB_SKIP);
				}
			}

			for (f = 0; f < 15; f++) // Turn grey
			{
				DisplayGameplayScreen();
			}
		}
		else if (fullRowCount == 3) // Remove 1 at a time
		{
			for (i = 0; i < fullRowCount; i++)
			{
				for (x = 0; x < 10; x++)
				{
					SetFlag(cels_GPB[x][fullRows[i]]->ccb_Flags, CCB_SKIP);

					DisplayGameplayScreen();
				}
			}
		}
		else // MARIA
		{		
			SetFlag(cels_GPB[1][fullRows[0]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[3][fullRows[0]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[6][fullRows[0]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[8][fullRows[0]]->ccb_Flags, CCB_SKIP); // Hide certain blocks

			SetFlag(cels_GPB[0][fullRows[1]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[2][fullRows[1]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[4][fullRows[1]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[5][fullRows[1]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[7][fullRows[1]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[9][fullRows[1]]->ccb_Flags, CCB_SKIP); // Hide certain blocks

			SetFlag(cels_GPB[1][fullRows[2]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[3][fullRows[2]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[6][fullRows[2]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[8][fullRows[2]]->ccb_Flags, CCB_SKIP); // Hide certain blocks

			SetFlag(cels_GPB[0][fullRows[3]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[2][fullRows[3]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[4][fullRows[3]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[5][fullRows[3]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[7][fullRows[3]]->ccb_Flags, CCB_SKIP); // Hide certain blocks
			SetFlag(cels_GPB[9][fullRows[3]]->ccb_Flags, CCB_SKIP); // Hide certain blocks

			for (i = 0; i < fullRowCount; i++) // Blow up any full row
			{
				for (x = 0; x < 10; x++)
				{
					SetFlag(cels_GPB[x][fullRows[i]]->ccb_Flags, CCB_MARIA); // Cool explosion effect
				}
			}

			for (f = 0; f < 15; f++) // Fun MARIA Explosion animation
			{
				for (i = 0; i < fullRowCount; i++) // Blow up any full row
				{
					for (x = 0; x < 10; x++)
					{
						cels_GPB[x][fullRows[i]]->ccb_XPos -= DivSF16(Convert32_F16(6 - (1 + x)), Convert32_F16(4)) << 4;
						cels_GPB[x][fullRows[i]]->ccb_YPos -= DivSF16(Convert32_F16(1), Convert32_F16(3)) << 4;

						cels_GPB[x][fullRows[i]]->ccb_HDX = DivSF16(Convert32_F16(12 + (f * 6)), Convert32_F16(12)) << 4;
						cels_GPB[x][fullRows[i]]->ccb_VDY = DivSF16(Convert32_F16(12 + (f * 12)), Convert32_F16(12));
					}
				}

				DisplayGameplayScreen();
			}

			for (i = 0; i < fullRowCount; i++) // That was fun but now reset.. not sure if I need to do this..
			{
				for (x = 0; x < 10; x++)
				{
					ClearFlag(cels_GPB[x][fullRows[i]]->ccb_Flags, CCB_MARIA);

					PositionCelColumn(cels_GPB[x][fullRows[i]], x + 8, fullRows[i] + 1, 4, 0); // sigh.. magic numbers and weird offsets...

					cels_GPB[x][fullRows[i]]->ccb_HDX = DivSF16(Convert32_F16(12), Convert32_F16(12)) << 4;
					cels_GPB[x][fullRows[i]]->ccb_VDY = DivSF16(Convert32_F16(12), Convert32_F16(12));
				}
			}
		}

		// END ROW CLEAR FX

		for (i = 0; i < fullRowCount; i++) // Clear the rows
		{
			f = fullRows[i];

			for (y = f; y >= 0; y--)
			{
				for (x = 0; x < 10; x++)
				{
					if (y == 0)
					{
						GamePlayBlocks[x][y] = false;
					}
					else
					{
						GamePlayBlocks[x][y] = GamePlayBlocks[x][y-1];

						cels_GPB[x][y]->ccb_SourcePtr = cels_GPB[x][y-1]->ccb_SourcePtr; // Don't remove this or else
						cels_GPB[x][y]->ccb_Flags = cels_GPB[x][y-1]->ccb_Flags; // Don't remove this or else
					}
				}
			}
		}

		ClearingLines = false;
	}
}

void GameOverKillBlocks()
{
	int x, y;	
	
	CCB *cel_GameOver = LoadCel("data/gameover.cel", MEMTYPE_CEL);
	CCB *cel_Credits1 = LoadCel("data/credits.cel", MEMTYPE_CEL);
	CCB *cel_Credits2 = LoadCel("data/credits2.cel", MEMTYPE_CEL);
	CCB *cel_Black = CreateBackdropCel(118, 228, MakeRGB15(0, 0, 1), 95);
	
	PositionCel(cel_GameOver, 112, 15);
	PositionCel(cel_Credits1, 99, 15);
	PositionCel(cel_Credits2, 99, 15);
	PositionCel(cel_Black, 101, 0);

	for (x = 0; x < 4; x++)
	{
		cels_AB[x]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr; // Grey Block
	}

	for (y = 17; y >= 0; y--) // Start at the bottom
	{
		for (x = 0; x < 10; x++)
		{
			if (GamePlayBlocks[x][y] == true)
			{
				cels_GPB[x][y]->ccb_SourcePtr = cel_AllBlockImages[BLOCK_GREY]->ccb_SourcePtr;

				DisplayGameplayScreen();
			}
		}
	}

	for (x = 0; x < 30; x++) // Do nothing
	{
		DisplayGameplayScreen();
	}
	
	for (y = 17; y >= 0; y--) // Start at the bottom
	{
		for (x = 0; x < 10; x++)
		{
			SetFlag(cels_GPB[x][y]->ccb_Flags, CCB_SKIP);
		}
	}
	
	for (x = 0; x < 4; x++)
	{
		SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_HB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_NB[x]->ccb_Flags, CCB_SKIP);
	}

	RenderGameBlocks = false;

	if (QuickReset == false)
	{
		for (x = 0; x < 60 * 3; x++) // Do nothing for 5 seconds
		{
			DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_GameOver);

			DisplayGameplayScreen();
		}

		for (x = 0; x < 60 * 3; x++) // Do nothing for 1 second
		{
			DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Black);
			DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Credits1);

			DisplayGameplayScreen();
		}
		
		for (x = 0; x < 60 * 3; x++) // Do nothing for 1 second
		{
			DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Black);
			DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Credits2);

			DisplayGameplayScreen();
		}
	}
	
	UnloadCel(cel_GameOver);
	UnloadCel(cel_Credits1);
	UnloadCel(cel_Credits2);
	UnloadCel(cel_Black);
	
	HideOptionsMenu();
	HidePausedMenu();
}

void CheckForNextLevel()
{
	if (CurrLines >= TargetLines)
	{
		TargetLines += 2;

		CurrLines = 0;

		CurrLevel++;

		TotScore = TotScore + (CurrLevel * 125); // New level bonus hurray

		if (lvSpeed > 3) lvSpeed = lvSpeed - (CurrLevel < 15 ? 2 : 1);
		
		ApplyCurrentThemeBackground();
	}
}

void ApplyCurrentThemeBackground()
{	
	char str[14];
	
	//PlaySFX(SFX_SUCCESS);

	if (localDefaultTheme == true)
	{
		sprintf(str, "data/bg%d.img", ((CurrLevel + 32) % 33) + 1); // Rotate 1 - 33
	}
	else
	{
		sprintf(str, "data/sf%d.img", ((CurrLevel + 4) % 5) + 1); // Rotate 1 - 5
	}

	SwapBackgroundImage(str, CurrLevel);
}

int main()
{
	initSystem();
	initGraphics();
	OpenAudioFolio();
	
	//initSPORTwriteValue(MakeRGB15(1,1,1));
	
	//initTools();

	InitNumberCels(6); // 3DOHD Initialize 3 sets of number cels for chaining

	InitNumberCel(0, 10, 99, 0, true); // High Level
	InitNumberCel(1, 10, 147, 0, true); // High Score
	InitNumberCel(2, 10, 195, 0, true); // Current Score
	InitNumberCel(3, 237, 99, 0, false); // Current Level
	InitNumberCel(4, 237, 147, 0, false); // Lines
	InitNumberCel(5, 237, 195, 0, false); // Remaining
	
	loadData();
	
	ApplySelectedColorPalette();

	initSPORT();

	srand(ReadHardwareRandomNumber());
	
	sfxInit = initsound(); // Initialize the EFMM Sound Library
	sfxLoad = loadsfx(); // In theory I can spool from here also 
	
	PlayBackgroundMusic();
	
	ShowIntroSplash();

	GameOver = false;
	GameStarted = false; // Debug flag
	
	debugMode = 0;
		
	GameLoop(); // The actual game play happens here - Nothing above gets called again

	// Never gets to this point... but...
	
	closesound();
	freesfx();

	Cleanup();
	CleanupNumberCels();
}

void InitGame()
{
	int x, y;

	for (x = 0; x < 10; x++)
	{
		for (y = 0; y < 18; y++) 
		{
			GamePlayBlocks[x][y] = false;
		}
	}
	
	for (x = 0; x < 4; x++)
	{
		SetFlag(cels_AB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_NB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_HB[x]->ccb_Flags, CCB_SKIP);
		SetFlag(cels_GB[x]->ccb_Flags, CCB_SKIP);
	}

	swGame = 0;
	lvSpeed = 48;

	TotScore = 0;
	TotLines = 0;
	CurrLines = 0;

	TotScore = 0;
	CurrScore = 0;

	CurrLevel = 1;

	CurrLines = 0;
	TotLines = 0;
	
	QueuedShapeIdx = -1;
	HeldShapeIdx = -1;
	
	QueueSwaps = 3;

	TargetLines = 10;

	ResetCelNumbers();
	
	CleanupTempCels();

	GameOver = false;
	QuickReset = false;
	GameStarted = false;
	OptionsMenuSelected = false;
	EasterEggEnabled = false;
	smStartSelected = true;
	ClearingLines = false;
	IsPaused = false;
	RenderGameBlocks = true;
	AcceptGameInput = false;
	
	SwapBackgroundImage("data/bgmain.img", 0); 

	ShowStartMenu();	
	
	SampleSystemTimeTV(&dData.tvInit);
	SampleSystemTimeTV(&dData.tvFrames60Start);
	SampleSystemTimeTV(&dData.tvCurrLoopStart);	
	
	srand(ReadHardwareRandomNumber()); // Called again each time - This ensures good random generator as it also is based on user input timing
}

void GameLoop()
{	
	while (true)
	{		
		InitGame();

		InitEventUtility(1, 0, LC_Observer); // Turn on the joypad listener
		
		while (GameStarted == false)
		{
			HandleInput();

			if (OptionsMenuSelected == true)
			{
				HandleOptionsMenuLogic();

				DisplayOptionsScreen();
			}
			else
			{
				HandleStartMenuLogic();
				
				DisplayStartScreen();
			}
		}
		
		KillEventUtility(); // Disable the joypad listener

		HideStartMenu();
		
		ApplyCurrentThemeBackground();

		ReadyIn321();

		GameStarted = true;

		QueueNextBlock();

		LoadNextBlockFromQueue();

		InitEventUtility(1, 0, LC_Observer); // Turn on the joypad listener
		
		while (GameOver == false)
		{
			HandleInput();
			
			if (OptionsMenuSelected == true) // Ideally this same call wouldn't happen twice but NBD
			{
				HandleOptionsMenuLogic();

				DisplayOptionsScreen();
			}
			else
			{
				HandleGameplayLogic();

				DisplayGameplayScreen();
			}
		}
		
		KillEventUtility(); // Disable the joypad listener

		if (TotScore > HighScore) HighScore = TotScore;
		if (CurrLevel > HighLevel) HighLevel = CurrLevel;
	}
}

void PauseScreen()
{
	DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_PausedOptions);
}

void ReadyIn321()
{
	int x;
	
	CCB *cel_Ready3 = InitAndPositionCel("data/ready3.cel", 107, 18); 
	CCB *cel_Ready2 = InitAndPositionCel("data/ready2.cel", 107, 18);
	CCB *cel_Ready1 = InitAndPositionCel("data/ready1.cel", 107, 18);

	IsPaused = true;

	for (x = 0; x < 30; x++) // Do nothing for .5 Seconds
	{
		DisplayGameplayScreen();
	}

	for (x = 0; x < 60; x++) // Do nothing for 1 second
	{
		DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Ready3);

		DisplayGameplayScreen();
	}

	for (x = 0; x < 60; x++) // Do nothing for 1 second
	{
		DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Ready2);

		DisplayGameplayScreen();
	}

	for (x = 0; x < 60; x++) // Do nothing for 1 second
	{
		DrawCels(screen.sc_BitmapItems[ visibleScreenPage ], cel_Ready1);

		DisplayGameplayScreen();
	}
	
	UnloadCel(cel_Ready3);
	UnloadCel(cel_Ready2);
	UnloadCel(cel_Ready1);

	IsPaused = false;
	AcceptGameInput = true;
}

void ShowStartMenu()
{
	int x;
	
	cel_Options = InitAndPositionCel("data/options.cel", 124, 120);

	for (x = 0; x < 4; x++)
	{
		cels_SM[x] = CopyCel(cel_AllBlockImages[5]);
	}
	
	cels_SM[0]->ccb_NextPtr = cels_SM[1];
	cels_SM[1]->ccb_NextPtr = cels_SM[2];
	cels_SM[2]->ccb_NextPtr = cels_SM[3];
	cels_SM[3]->ccb_NextPtr = cel_Options;
	
	cel_Options->ccb_NextPtr = NULL;
	SetFlag(cel_Options->ccb_Flags, CCB_LAST);
	
	smStartSelected = false;
	
	ToggleStartMenuSelection(); // Hack to init and position the block
}

void HideStartMenu()
{
	int x;

	for (x = 0; x < 4; x++)
	{
		UnloadCel(cels_SM[x]);
	}
	
	UnloadCel(cel_Options);
}

void TogglePaused(bool isPaused)
{
	if (IsPaused != isPaused)
	{
		IsPaused = isPaused;
		
		if (IsPaused)
		{
			ShowPausedMenu();
		}
		else
		{
			HidePausedMenu();
		}
	}
}

void ShowPausedMenu()
{
	cel_PausedHdr = InitAndPositionCel("data/hdpaused.cel", 104, 18);
	cel_PausedOptions = InitAndPositionCel("data/subpaused.cel", 112, 80);

	cel_PausedOptions->ccb_NextPtr = cel_PausedHdr;
	cel_PausedHdr->ccb_NextPtr = NULL;
	SetFlag(cel_PausedHdr->ccb_Flags, CCB_LAST);
}

void HidePausedMenu()
{
	UnloadCel(cel_PausedHdr);
	UnloadCel(cel_PausedOptions);
}

void ToggleOptionsMenu(bool optionsMenuSelected)
{
	if (OptionsMenuSelected != optionsMenuSelected)
	{
		OptionsMenuSelected = optionsMenuSelected;
		
		if (OptionsMenuSelected)
		{
			ShowOptionsMenu();
		}
		else
		{
			HideOptionsMenu();
		}
	}
}

void ShowOptionsMenu()
{
	int x;
	
	cel_OptionGuides = CopyCel(cel_AllBlockImages[ localShowGuides ? BLOCK_RED : BLOCK_GREY ]);
	cel_OptionMusic = CopyCel(cel_AllBlockImages[ localPlayMusic ? BLOCK_RED : BLOCK_GREY ]);
	cel_OptionSFX = CopyCel(cel_AllBlockImages[ localPlaySFX ? BLOCK_RED : BLOCK_GREY ]);
	cel_OptionTheme = CopyCel(cel_AllBlockImages[ localDefaultTheme ? BLOCK_RED : BLOCK_GREY ]);
	
	PositionLoadedCel(cel_OptionGuides, 180, 40);
	PositionLoadedCel(cel_OptionMusic, 180, 53);
	PositionLoadedCel(cel_OptionSFX, 180, 66);
	PositionLoadedCel(cel_OptionTheme, 180, 79);

	cel_OptionGuides->ccb_NextPtr = cel_OptionMusic;
	cel_OptionMusic->ccb_NextPtr = cel_OptionSFX;
	cel_OptionSFX->ccb_NextPtr = cel_OptionTheme;

	SetFlag(cel_OptionTheme->ccb_Flags, CCB_LAST);
	cel_OptionTheme->ccb_NextPtr = NULL;

	for (x = 0; x < 4; x++) // Must initialize before assigning next ptr
	{
		cels_OM1[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[0]]); // BlockImageIdx maintains the state
		cels_OM2[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[1]]); // of the custom images
		cels_OM3[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[2]]);
		cels_OM4[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[3]]);
		cels_OM5[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[4]]);
		cels_OM6[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[5]]);
		cels_OM7[x] = CopyCel(cel_AllBlockImages[BlockImageIdx[6]]);
		
		PositionLoadedCel(cels_OM1[x], 12 * (DefaultBlockCoords[0][x].X - 15), 12 * (DefaultBlockCoords[0][x].Y + 7) + 6); // TODO Position them just so
		PositionLoadedCel(cels_OM2[x], 12 * (DefaultBlockCoords[1][x].X - 16), 12 * (DefaultBlockCoords[1][x].Y + 10) - 4); // Relative to Default Coordinates
		PositionLoadedCel(cels_OM3[x], 12 * (DefaultBlockCoords[2][x].X - 9), 12 * (DefaultBlockCoords[2][x].Y + 7) + 3); 
		PositionLoadedCel(cels_OM4[x], 12 * (DefaultBlockCoords[3][x].X - 9), 12 * (DefaultBlockCoords[3][x].Y + 10) - 4); 
		PositionLoadedCel(cels_OM5[x], 12 * (DefaultBlockCoords[4][x].X - 3), 12 * (DefaultBlockCoords[4][x].Y + 7) + 3); 
		PositionLoadedCel(cels_OM6[x], 12 * (DefaultBlockCoords[5][x].X - 3), 12 * (DefaultBlockCoords[5][x].Y + 10) - 4); 
		PositionLoadedCel(cels_OM7[x], 12 * (DefaultBlockCoords[6][x].X - 3), 12 * (DefaultBlockCoords[6][x].Y + 12) + 1);
	}

	for (x = 0; x < 4; x++)
	{
		cels_OM1[x]->ccb_NextPtr = cels_OM2[x];
		cels_OM2[x]->ccb_NextPtr = cels_OM3[x];
		cels_OM3[x]->ccb_NextPtr = cels_OM4[x];
		cels_OM4[x]->ccb_NextPtr = cels_OM5[x];
		cels_OM5[x]->ccb_NextPtr = cels_OM6[x];
		cels_OM6[x]->ccb_NextPtr = cels_OM7[x];
	}
	
	cels_OM7[0]->ccb_NextPtr = cels_OM1[1];
	cels_OM7[1]->ccb_NextPtr = cels_OM1[2];
	cels_OM7[2]->ccb_NextPtr = cels_OM1[3];
	cels_OM7[3]->ccb_NextPtr = cel_OptionGuides;

	cel_OptionsOverlay = CreateBackdropCel(320, 240, MakeRGB15(1, 1, 1), 90);	
	PositionLoadedCel(cel_OptionsOverlay, 0, 0);	

	cel_OptionsMain = LoadCel("data/mainoptions.cel", MEMTYPE_CEL);
	PositionLoadedCel(cel_OptionsMain, 72, 12);

	cel_OptionsArrow = LoadCel("data/arrow.cel", MEMTYPE_CEL);
	PositionLoadedCel(cel_OptionsArrow, 42, 40);
	
	cel_OptionsOverlay->ccb_NextPtr = cel_OptionsMain;
	cel_OptionsMain->ccb_NextPtr = cel_OptionsArrow;
	cel_OptionsArrow->ccb_NextPtr = cels_OM1[0];
}

void HideOptionsMenu()
{
	UnloadCel(cel_OptionsOverlay);
	UnloadCel(cel_OptionsMain);
	UnloadCel(cel_OptionsArrow);
}

void PlaySFX(int id)
{
	if (OptionsPlaySFX)
	{ 
		playsound(id);
	}
}

void PlayBackgroundMusic() 
{
	if (OptionsPlayMusic)
	{
		spoolsound("music/tetrismono.aiff", 256);
		//startMusic("MainMusicThread", "Music/tetrismono.aiff", 256); 
	}
}

void CleanupTempCels()
{
	HideOptionsMenu();
	HidePausedMenu();
	HideStartMenu();
}

void Cleanup() // ... but there is no cleanup...
{
	// int x, y;

	CloseGraphics ( &screen );
	CloseMathFolio();
	CloseAudioFolio();

	UnloadImage(backgroundBufferPtr1);
	backgroundBufferPtr1 = NULL;
 }
 
 /* TODO
 
 Cleanup
 Perf Tuning
 Go through all levels 2x
 Better SFX
 Test on 3DO
 OK - DefaultColorMode
 OK - Move what can to new methods
 OK - Remove unnecessary audio crap
 OK - Credits - add new names
 OK - Pause L+R to quit
 OK - Remove click from start options
 OK - Splash screen during music fade in (fade in fade out)
 OK - Fade in fade out BG?
 OK - new version banner
 OK - Drawscreencels VS drawcels
 
 */
 