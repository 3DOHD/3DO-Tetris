/*  :ts=8 bk=0
 *
 * sound.c:	Sound initialization and access.
 *
 * Leo L. Schwab					9308.04
 */
#include <types.h>
#include <strings.h>

#include "HD3DOAudioSFX.h"
#include "HD3DOAudioSoundInterface.h" 



/***************************************************************************
 * Sound effects filenames.
 */
static char	*ramfx[] = {	   
	"music/clear.aiff",
	"music/clear4.aiff",
	"music/success.aiff",
	"music/tick.aiff",
	"music/hold.aiff",
	"music/drop.aiff",
	"music/gameover.aiff"
};

/***************************************************************************
 * Code.
 */
void playsound (int id)
{
	RAMSoundRec	rs;

	rs.whatIWant	= kStartRAMSound;
	rs.soundID	= id;

	CallSound ((CallSoundRec *) &rs);
}


void initsound ()
{
	int32 whatIWant;

	whatIWant = kInitializeSound;
	CallSound ((CallSoundRec *) &whatIWant);
}

void closesound ()
{
	int32 whatIWant;

	whatIWant = kCleanupSound;
	CallSound ((CallSoundRec *) &whatIWant);
}


void loadsfx ()
{
	register int	i;
	LoadRAMSoundRec	lrs;

	lrs.whatIWant = kLoadRAMSound;

	for (i = 1;  i < MAX_SFX;  i++)
	{
		lrs.soundID = i;
		lrs.soundFileName = ramfx[i - 1];
		lrs.amplitude = 0x3800;
		lrs.balance = 50;
		lrs.frequency = 0;

		CallSound ((CallSoundRec *) &lrs);
	}
}

void freesfx ()
{
	register int	i;
	RAMSoundRec	rs;

	rs.whatIWant = kUnloadRAMSound;

	for (i = 1;  i < 3;  i++)
	{
		rs.soundID = i;

		CallSound ((CallSoundRec *) &rs);
	}
}


int32 loadsound (char *filename, int32 id) 
{
	LoadRAMSoundRec	lrs;

	lrs.whatIWant = kLoadRAMSound;
	lrs.soundID = id;
	lrs.soundFileName = filename;
	lrs.amplitude = MAXAMPLITUDE;
	lrs.balance = 50;
	lrs.frequency = 0;

	return (CallSound ((CallSoundRec *) &lrs));
}

void unloadsound (int32 id)
{
	RAMSoundRec	rs;

	rs.whatIWant	= kUnloadRAMSound;
	rs.soundID	= id;

	CallSound ((CallSoundRec *) &rs);
}

void spoolsound (char *filename, int32 nreps)
{
	SpoolSoundRec	ssr;

	ssr.whatIWant	= kSpoolSound;
	ssr.fileToSpool	= filename;
	ssr.numReps	= nreps;
	ssr.amplitude	= 0x7FFF;//0x3800;
	CallSound ((CallSoundRec *) &ssr);

}

void stopspoolsound (int32 nsecs)
{
	SpoolFadeSoundRec	sr;

	if (nsecs > 0)
	{
		sr.whatIWant = kStopFadeSpoolSound;
		sr.seconds = nsecs;
	}
	else
	{
		sr.whatIWant = kStopSpoolingSound;
	}

	CallSound ((CallSoundRec *) &sr);
}

int issoundspooling ()
{
	int32 whatIWant;

	whatIWant = kIsSoundSpooling;

	return (CallSound ((CallSoundRec *) &whatIWant));
}