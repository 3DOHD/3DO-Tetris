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

#include "HD3DO.h"


CCB *cel_Numbers[10];
TrackedNumber TrackedNumbers[8]; // MAXNUMCOUNT = 8

int CelNumberCount = -1;

bool initialized = false;

void InitNumberCels(int count)
{
	int x, i;
	
	if (initialized) return; // TODO - Re-initialize?
	
	CelNumberCount = count;
	
	cel_Numbers[0] = LoadCel("data/num0.cel", MEMTYPE_CEL);
	cel_Numbers[1] = LoadCel("data/num1.cel", MEMTYPE_CEL);
	cel_Numbers[2] = LoadCel("data/num2.cel", MEMTYPE_CEL);
	cel_Numbers[3] = LoadCel("data/num3.cel", MEMTYPE_CEL);
	cel_Numbers[4] = LoadCel("data/num4.cel", MEMTYPE_CEL);
	cel_Numbers[5] = LoadCel("data/num5.cel", MEMTYPE_CEL);
	cel_Numbers[6] = LoadCel("data/num6.cel", MEMTYPE_CEL);
	cel_Numbers[7] = LoadCel("data/num7.cel", MEMTYPE_CEL);
	cel_Numbers[8] = LoadCel("data/num8.cel", MEMTYPE_CEL);
	cel_Numbers[9] = LoadCel("data/num9.cel", MEMTYPE_CEL);
	
	if (count > MAXNUMCOUNT) count = MAXNUMCOUNT; // Max allocated

	for (x = 0; x < count; x++) // Build and Chain
	{
		for (i = 0; i < 9; i++)
		{
			TrackedNumbers[x].cel_NumCels[i] = CloneCel(cel_Numbers[0], 0); // Just zero
			
			ClearFlag(TrackedNumbers[x].cel_NumCels[i]->ccb_Flags, CCB_LAST);
			SetFlag(TrackedNumbers[x].cel_NumCels[i]->ccb_Flags, CCB_SKIP);
		}
	}
	
	for (x = 0; x < count; x++) 
	{
		for (i = 0; i < 8; i++) 
		{
			TrackedNumbers[x].cel_NumCels[i]->ccb_NextPtr = TrackedNumbers[x].cel_NumCels[i + 1];
		}
		
		if (x < count - 1) TrackedNumbers[x].cel_NumCels[8]->ccb_NextPtr = TrackedNumbers[x + 1].cel_NumCels[0];
	}
	
	TrackedNumbers[count - 1].cel_NumCels[8]->ccb_NextPtr = NULL;
	SetFlag(TrackedNumbers[count - 1].cel_NumCels[8]->ccb_Flags, CCB_LAST);
	
	initialized = true;
}

void InitNumberCel(int idx, int x, int y, uint32 value, bool rightAlign)
{
	int i;
	
	if (ValidAndReady(idx) == false) return;
	
	TrackedNumbers[idx].X = x;
	TrackedNumbers[idx].Y = y;
	TrackedNumbers[idx].Value = value;
	TrackedNumbers[idx].RightAlign = rightAlign;

	for (i = 0; i < 9; i++)
	{
		PositionCel(TrackedNumbers[idx].cel_NumCels[i], x + (i * 8), y);
	}
	
	SetCelNumbers(idx, value);
}

CCB *InitAndPositionCel(char *path, int x, int y)
{
	CCB *cel = LoadCel(path, MEMTYPE_CEL);
	
	PositionCel(cel, x, y);
	
	InitCCBFlags(cel);
	
	return cel;
}

void PositionLoadedCel(CCB *cel, int x, int y)
{	
	PositionCel(cel, x, y);
	
	InitCCBFlags(cel);
}

void PositionCel(CCB *cel, int x, int y)
{
	cel->ccb_XPos = Convert32_F16(x);
	cel->ccb_YPos = Convert32_F16(y);
}

void PositionCelColumn(CCB *cel, int x, int y, int xOffset, int yOffset)
{
	cel->ccb_XPos = Convert32_F16((x * 12) + xOffset);
	cel->ccb_YPos = Convert32_F16((y * 12) + yOffset);	
}

void InitCCBFlags(CCB *cel)
{
	cel->ccb_PLUTPtr = NULL;
	cel->ccb_NextPtr = NULL;
	
	cel->ccb_Flags &= ~CCB_LDPLUT;
	cel->ccb_Flags &= ~CCB_LAST;
	cel->ccb_Flags &= ~CCB_SKIP;
	
	//cel->ccb_PRE0 = PRE0_LINEAR | PRE0_BPP_16;
}

CCB * CopyCel(CCB *src)
{
	int32 allocExtra = 0L;
	void *dataBuf;
	CCB *cel = NULL;

	cel = AllocMagicCel_(allocExtra, 0x0de11CCB, NULL, NULL);

	memcpy(cel, src, sizeof(CCB));
	
	if (src->ccb_NextPtr != NULL)
	{
		cel->ccb_Flags |= CCB_LAST;
		cel->ccb_NextPtr = NULL;
	}

	return cel;
}

void ResetCelNumbers()
{
	int x, i;
	
	if (ValidAndReady(0) == false) return;
	
	for (x = 0; x < CelNumberCount; x++) // Build and Chain
	{
		for (i = 0; i < 9; i++)
		{
			SetFlag(TrackedNumbers[x].cel_NumCels[i]->ccb_Flags, CCB_SKIP);
		}
		
		SetCelNumbers(x, 0);
	}
}

void SetCelNumbers(int idx, uint32 value)
{
	char buffer[9]; // 999999999
	int i, sLen, currVal;
	
	if (ValidAndReady(idx) == false) return;
	
	if (value > 999999999) value = 999999999;
	
	TrackedNumbers[idx].Value = value;	
	
	sprintf(buffer, "%d", value);
	
	sLen = strlen(buffer);
	
	for (i = 0; i < sLen; i++) // TODO handle if right aligned - fill idx 9, 8, 7, etc. 
	{
		currVal = ((int)buffer[i] - '0');
		
		if (currVal < 0) currVal = 0;
		if (currVal > 9) currVal = 9;
		
		if (TrackedNumbers[idx].RightAlign == true)
		{		
			TrackedNumbers[idx].cel_NumCels[9 - (sLen - i)]->ccb_SourcePtr = cel_Numbers[currVal]->ccb_SourcePtr;
	   
			ClearFlag(TrackedNumbers[idx].cel_NumCels[9 - (sLen - i)]->ccb_Flags, CCB_SKIP);
		}
		else
		{
			TrackedNumbers[idx].cel_NumCels[i]->ccb_SourcePtr = cel_Numbers[currVal]->ccb_SourcePtr;
	   
			ClearFlag(TrackedNumbers[idx].cel_NumCels[i]->ccb_Flags, CCB_SKIP);
		}
	}
	
	if (TrackedNumbers[idx].RightAlign == true)
	{
		for (i = 0; i < 9 - sLen; i++) 
		{		
			SetFlag(TrackedNumbers[idx].cel_NumCels[i]->ccb_Flags, CCB_SKIP);
		}
	}
	else
	{
		for (i = sLen; i < 9; i++) 
		{   
			SetFlag(TrackedNumbers[idx].cel_NumCels[i]->ccb_Flags, CCB_SKIP);
		}		
	}
}

void RenderCelNumbers(Item bitmapItem) // Assumes the CELs have already been positioned on screen
{	
	if (ValidAndReady(0) == false) return;
	
	DrawCels(bitmapItem, TrackedNumbers[0].cel_NumCels[0]);		
}

bool ValidAndReady(int idx)
{
	if (initialized == false) return false;
	if (CelNumberCount < 0) return false;		
	if (idx >= CelNumberCount) return false;
	
	return true;	
}

void CleanupNumberCels()
{
	int x, i;
	
	if (ValidAndReady(0) == false) return;
	
	for (x = 0; x < 10; x++)
	{
		UnloadCel(cel_Numbers[x]);
	}
	
	for (x = 0; x < CelNumberCount; x++) // All initialized CELs
	{
		for (i = 0; i < 9; i++)
		{
			UnloadCel(TrackedNumbers[x].cel_NumCels[i]);
		}
	}
}

void BUGOsetDefaultCelValues(CCB *cel)
{
	cel->ccb_Flags =	CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP | CCB_LDPLUT |
						CCB_CCBPRE | CCB_YOXY | CCB_ACW | CCB_ACCW | CCB_ACE | CCB_USEAV | CCB_POVER_MASK | CCB_NOBLK;

	// If we want superclipping (should do functions to set extra gimmicks)
	// cel->ccb_Flags |= (CCB_ACSC | CCB_ALSC);


	cel->ccb_NextPtr = NULL;
	//cel->ccb_SourcePtr = NULL;
	cel->ccb_PLUTPtr = NULL;

	cel->ccb_XPos = 0;
	cel->ccb_YPos = 0;
	cel->ccb_HDX = 1 << 20;
	cel->ccb_HDY = 0;
	cel->ccb_VDX = 0;
	cel->ccb_VDY = 1 << 16;
	cel->ccb_HDDX = 0;
	cel->ccb_HDDY = 0;


	//cel->ccb_PIXC = CEL_BLEND_OPAQUE;

	cel->ccb_PRE0 = 0;
	cel->ccb_PRE1 = PRE1_TLLSB_PDC0;	// blue LSB bit is blue
}

