/***************************************************************
**
** SoundInterface.c
**
** Written by Peter Commons with a lot of help (and code) from Phil Burk.
** 
** This is a very high level sound library of sorts. It is designed to be very easy to
** use by other programmers to get lots of cool sound stuff for their programs without
** having to learn all about the sound toolbox.
**
** All calls to the sound library are made through the CallSound() function.
** Anyone who uses the sound library must include SoundInterface.h in their code.
** Look in SoundInterface.h for a lot of information on how to call the sound library.
**
** Copyright (c) 1993, 3DO Company.
** This program is proprietary and confidential.
**
Change History:
07/27/93	Initial Version.
08/02/93	Added stuff for variable rate samples and volume control.
08/04/93	Made the stop spool command synchronous (i.e. it waits until the sound has
			really stopped before returning). This is so you can stop a sound and start
			a new one right up without getting a "can't start, still running" error message.
08/10/93	Grab all knobs at the beginning, so as to avoid slowing stuff down when actually
			playing sounds. Fixed bug with variable rate samples (setting the rate).
			Removed instrument file name parameter from load RAM sound; the instrument
			is now determined automagically.
08/11/93	Altered CallSound() prototype for Leo.
08/13/93	Add checks for setting rate (freq) too high or too low.
08/16/93	Removed isStereo flag from LoadRAMSound. Stereo/Mono determination is done 
			automatically. Added kStopFadeSpoolSound command (new data structure, too)
			which will stop the currently spooled sound over a given number of seconds.
08/16/93.2	Call OpenAudioFolio() in all threads that do audio calls.
08/18/93	Add: SoundLib already initialized error. Remove one point from spooler code.
			Add: SoundID of 0 invalid error. Add: DisconnectInstruments() calls when
			unloading a RAM sound.
08/30/93	Add: kIsSoundSpooling function. Returns true if sound currently spooling.
			Modified to work under DragonTail5.
09/01/93	Add instrument cacheing. The first MAX_CACHED_INSTRUMENTS are cached in RAM
			and not reloaded from disk every time they're used. Once the cache is full,
			there is no replacement strategy.
09/08/93	Moved away from a channel-dependent philosophy. You can now load more sounds
			than you have channels for or than you have room for on the dsp. Intruments
			will get loaded into the dsp and unloaded as needed.
09/16/93	Space is now reserved in the dsp for a spooler instrument (currently I use
			halfmono8.dsp to determine the size because it takes up a lot of room and trying
			to use fixedstereosample.dsp didn't save enough). This should prevent the loading of too many
			RAM sounds precluding the use of the dsp for spooling.
***************************************************************/

#include "types.h"
#include "debug.h"
#include "operror.h"
#include "filefunctions.h"
#include "audio.h" 
#include "music.h"
#include "soundfile.h"
#include "operamath.h"
#include "HD3DOAudioSoundInterface.h"  


// PVC This might be a bit big
#define STACKSIZE (10000)

// PVC These might be a bit big
#define NUMBLOCKS (24)
#define BLOCKSIZE (2048)  
#define BUFSIZE (NUMBLOCKS*BLOCKSIZE)   
#define NUMBUFFS  (4)


/*
**	Internal Forward Declarations
*/

static int32 InitializeSoundLibrary( void );
static int32 SetMixerLevels( int32 theLevel );
static int32 CleanupSoundLibrary( void );
static int32 SpoolASound( SpoolSoundPtr spoolSndPtr );
static int32 StopSpoolingSound( void );
static int32 StopSpoolingSoundFade( int32 seconds );
static void  StopSpoolOnFadeThread( void );
static void  SpoolSoundFileThread( void );
static int32 LoadRAMSound( LoadRAMSoundPtr loadSndPtr );
static int32 AssignChannels( SoundDataPtr theSound, int32 mustSucceed );
static int32 UnassignChannels( SoundDataPtr theSound );
static int32 MyUnloadInstrument( SoundDataPtr chInfo );
static int32 FindEmptyChannel( void ); 
static Item	 MyLoadInstrument( char *instrName );
static int32 UnloadRAMSound( int32 soundID );
static int32 StartRAMSound( int32 soundID );  
static int32 StopRAMSound( int32 soundID );  
static int32 SetRAMSoundFreq( SetRAMSoundPtr setSndPtr );
static int32 SetRAMSoundAmpl( SetRAMSoundPtr setSndPtr );
static int32 FlushInstrument( Item SamplerIns ); 

/*
**	Main Internal Variables
*/

static	int32	soundLibraryIntialized = false;			// Is sound library initialized?
static	ulong	spoolerOuttaHereSignal = 0;				// Spooler Is Done Signal
static	ulong	spoolerFaderOuttaHereSignal = 0;		// Spooler Fader Is Done Signal
static	ulong	spoolerHasStoppedSignal = 0;			// Spooler Is Done Stopping a Sound
static	int32	soundLibraryMainLevel = MAXAMPLITUDE;	// Main Volume Level

/*
**	RAM-Resident Sound Internal Variables 
*/

static	SoundDataRec	sounds[kMaxRamSounds]; // Info about each RAM resident sound
static	int32			numSoundsAssigned;
static	int32			assignQueue[kMaxRamSounds];	// List of sounds assigned in the order
												// they were assigned. Unassign the 0th element
												// in the queue whenever we need room.

/*
**	Mixer Internal Variables
*/

static	Item 	MixerTmp = -1;
static	Item 	MixerIns = -1;
static	char	mixerFileName[50] = kMixerFileName;
static	char	envelopeFileName[50] = kEnvelopeFileName;
static	char	spoolSaveFileName[50] = kSpoolerRoomSaveName;
static	Item	leftGainKnob[kNumChannels];
static	Item	rightGainKnob[kNumChannels];

/*
**	Sound Spooling Internal Variables
*/

static Item 	spoolerThread = -1;			// Thread that does spooling
static Item 	spoolerFaderThread = -1;	// Thread that does spooling fade
static int32 	spoolerRunning = false;		// Is a sound currently being spooled? If not, the
											// spooler thread is just hanging out
static int32	spoolerNumReps = 0;			// reps left to play
static	char *	spoolerFileName;			// Pointer to string of name of file to play
static int32	spoolerAmplitude;			// Volume to set for spooling
static	ulong	spoolerStartSignal = 0;
static	ulong	spoolerStopSignal = 0;
static	ulong	spoolerQuitSignal = 0;
static	DataTimePair 	spoolerEnvPoints[4];
static	ulong	spoolerStopOnFadeSignal = 0;
static	ulong	spoolerFaderQuitSignal = 0;
static	int32	spoolerFaderTime = 0;
static	int8	spoolerFading = FALSE;

static	SoundFilePlayer *spoolerSFP = NULL;

static	Item	spoolerEnvIns = -1;		// Envelope used on spooler to do fades
static	Item	spoolerRoomIns = -1;	// When a sound isn't being spooled, this instrument
										// is allocated to keep room for a spooled sound in the dsp.

//	DSP Instrument cache

static	int32	numCachedInstrs = 0;
static	char	cachedInstrName[MAX_CACHED_INSTRUMENTS][100];
static	Item	cachedInstrItem[MAX_CACHED_INSTRUMENTS];

// The Big Lollapalooza
// Go wild on error checking! e.g. if they stop a sound that hasn't started...

/*
**	CallSound()
*/
int32 CallSound( union CallSoundRec *soundPtr )
{
	int32 result = 0;

	if ( soundPtr->whatIWant != kInitializeSound && soundLibraryIntialized == false )
	{
		return (-1);
	}
	
	if ( soundPtr->whatIWant == kInitializeSound && soundLibraryIntialized )
	{
		return (-2);
	}

	switch ( soundPtr->whatIWant )
		{
		case kInitializeSound:
			result = InitializeSoundLibrary();
			break;

		case kCleanupSound:
			result = CleanupSoundLibrary();
			break;

		case kSpoolSound:
			result = SpoolASound( &soundPtr->spoolSound );
			break;

		case kStopSpoolingSound:
			result = StopSpoolingSound();
			break;

		case kStopFadeSpoolSound:
			result = StopSpoolingSoundFade( soundPtr->fadeSound.seconds );
			break;

		case kBeQuiet:
			soundLibraryMainLevel = 0;
			result = SetMixerLevels( soundLibraryMainLevel );
			break;

		case kBeNoisy:
			soundLibraryMainLevel = MAXAMPLITUDE;
			result = SetMixerLevels( soundLibraryMainLevel );
			break;

		case kLoadRAMSound:
			result = LoadRAMSound( &soundPtr->loadSound ); 
			break;

		case kUnloadRAMSound:
			result = UnloadRAMSound( soundPtr->ramSound.soundID);
			break;

		case kStartRAMSound:
			result = StartRAMSound( soundPtr->ramSound.soundID );
			break;

		case kStopRAMSound:
			result = StopRAMSound( soundPtr->ramSound.soundID );
			break;

		case kSetRAMSoundFreq:
			result = SetRAMSoundFreq( &soundPtr->setSound );
			break;

		case kSetRAMSoundAmpl:
			result = SetRAMSoundAmpl( &soundPtr->setSound );
			break;

		case kIsSoundSpooling:
			result = spoolerRunning;
			break;

		default:
			result = -1;
			break;
		}

	return ( result );
}


/*
**	InitializeSoundLibrary()
*/
static int32 InitializeSoundLibrary( void )
{
	int32 	result = noErr;
	int32 	Priority, i;

	if ( soundLibraryIntialized )
	{
		result = -1;
	}

	if ( result == noErr )
	{
		result = OpenAudioFolio();
		
		if ( result )
		{
			// ERROR
		}
	}

	if ( result == noErr )
	{
		Priority = 180;
		spoolerThread = CreateThread("SoundSpooler", Priority, SpoolSoundFileThread, STACKSIZE);
		spoolerOuttaHereSignal = AllocSignal( 0 );

		spoolerFaderThread = CreateThread("SoundSpoolerFader", Priority, StopSpoolOnFadeThread, STACKSIZE);
		spoolerFaderOuttaHereSignal = AllocSignal( 0 );

		spoolerHasStoppedSignal = AllocSignal( 0 );

		spoolerEnvIns = LoadInstrument( envelopeFileName, 0, 100 );

	}

	if ( result == noErr )
	{
		for ( i = 0; i < kMaxRamSounds; i++ )
		{
			sounds[i].soundID = 0;	// No Sound Allocated To Start
		}

		MixerTmp = LoadInsTemplate( mixerFileName, 0 );

		MixerIns = AllocInstrument( MixerTmp, 20);

		// Set the input from each channel to each speaker side in the mixer to the maximum.

		result = SetMixerLevels( MAXAMPLITUDE );

		// Set the global output volume from the mixer to maximum

		/* Mixer must be started */

		StartInstrument( MixerIns, NULL );

		/* Grab all mixer knobs */

		for ( i = 0; i < kNumChannels; i++ )
		{
			char aName[50];

			sprintf( aName, "LeftGain%d", i );
			leftGainKnob[i] = GrabKnob( MixerIns, aName );

			sprintf( aName, "RightGain%d", i );
			rightGainKnob[i] = GrabKnob( MixerIns, aName );
		}

		/* No cached instruments to start */

		numCachedInstrs = 0;

	}
	
	soundLibraryIntialized = ( result == noErr);

	return result;
}

/*
**	SetMixerLevels()
*/
static int32 SetMixerLevels( int32 theLevel )
{
	int32 	result = noErr;
	int32 	i, newLevelL, newLevelR;
	int32	channelIsHit = 0;

	// This sets the mixer levels harshly; we probably want to throw Phil's
	// envelope code in here at some point.

	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID )
		{
			if ( sounds[i].channel[0] >= 0 )
			{
				channelIsHit |= (1 << sounds[i].channel[0]);

				newLevelL = ( ((sounds[i].amplitude * sounds[i].balance) / 100) / kNumChannels) * AUDIO_MULTIPLIER;

				newLevelR = ( ((sounds[i].amplitude * (100 - sounds[i].balance)) / 100) / kNumChannels)  * AUDIO_MULTIPLIER;

				if ( sounds[i].channel[1] >= 0 )	// stereo
				{
					channelIsHit |= (1 << sounds[i].channel[1]);

					TweakKnob( leftGainKnob[sounds[i].channel[0]], newLevelL );
					TweakKnob( rightGainKnob[sounds[i].channel[0]], 0 );

					TweakKnob( leftGainKnob[sounds[i].channel[1]], 0 );
					TweakKnob( rightGainKnob[sounds[i].channel[1]], newLevelR );
				}
				else		// mono
				{
					TweakKnob( leftGainKnob[sounds[i].channel[0]], newLevelL );
					TweakKnob( rightGainKnob[sounds[i].channel[0]], newLevelR );
				}
			}
		}
	}

	// All unused channels should have their levels set to zero

	for ( i = 0; i < kNumChannels; i++ )
	{
		if ( (channelIsHit & (1 << i)) == 0 )
		{
			TweakKnob( leftGainKnob[i], 0 );
			TweakKnob( rightGainKnob[i], 0 );
		}
	}

	return result;
}


/*
**	CleanupSoundLibrary()
*/
static int32 CleanupSoundLibrary( void )
{
	int32 result = noErr;
	int32 i;

	if ( !soundLibraryIntialized )
	{
		result = -1;
	}
	else
	{
		/* Clean up spooler */

		SendSignal( spoolerThread, spoolerQuitSignal );
		WaitSignal( spoolerOuttaHereSignal );
		DeleteThread( spoolerThread );
		spoolerThread = -1;

		FreeSignal( spoolerOuttaHereSignal );
		spoolerOuttaHereSignal = 0;

		SendSignal( spoolerFaderThread, spoolerFaderQuitSignal );
		WaitSignal( spoolerFaderOuttaHereSignal );
		DeleteThread( spoolerFaderThread );
		spoolerFaderThread = -1;

		FreeSignal( spoolerFaderOuttaHereSignal );
		spoolerFaderOuttaHereSignal = 0;

		FreeSignal( spoolerHasStoppedSignal );
		spoolerHasStoppedSignal = 0;

		FreeInstrument( spoolerEnvIns );
		spoolerEnvIns = -1;

		/* Clean up any open RAM resident instruments */
		/* Free all mixer gain knobs */

		for ( i = 0; i < kMaxRamSounds; i++ )
		{
			if ( sounds[i].soundID )
			{
				UnloadRAMSound( sounds[i].soundID );
			}
		}

		for ( i = 0; i < kNumChannels; i++ )
		{
			ReleaseKnob( leftGainKnob[i] );
			ReleaseKnob( rightGainKnob[i] );
		}

		/* Unload all cached instruments */

		for ( i = 0; i < numCachedInstrs; i++ )
		{
			UnloadInsTemplate( cachedInstrItem[i] );
		}

		/* Clean up Mixer */

		FreeInstrument( MixerIns );
		MixerIns = -1;
		UnloadInsTemplate( MixerTmp );
		MixerTmp = -1;

		CloseAudioFolio();
	}

	soundLibraryIntialized = false;

	return result;
}

/*
**	SpoolASound()
*/
static int32 SpoolASound( SpoolSoundPtr spoolSndPtr )
{
 	int32 result = 0;

	if ( spoolerRunning  &&  !spoolerFading )
	{
		result = -1;
	}
	else
	{
		if ( spoolSndPtr->fileToSpool == 0 || spoolSndPtr->numReps == 0 || spoolSndPtr->amplitude == 0 )
		{
			// ERROR
		}

		spoolerFileName = spoolSndPtr->fileToSpool;
		spoolerNumReps = spoolSndPtr->numReps;
		spoolerAmplitude = spoolSndPtr->amplitude;

		if (spoolerFading)
		{
			WaitSignal (spoolerHasStoppedSignal);
			spoolerFading = FALSE;
		}

			SendSignal( spoolerThread, spoolerStartSignal );
		}

	return ( result );
}

/*
**	StopSpoolingSound()
*/
static int32 StopSpoolingSound( void )
{
	int32 result = 0;

	if ( !spoolerRunning )
	{
		result = -1;
	}
	else
	{
		SendSignal( spoolerThread, spoolerStopSignal );
		WaitSignal( spoolerHasStoppedSignal );
		spoolerFading = FALSE;
	}

	return ( result );
}

/*
**	StopSpoolingSoundFade()
*/
static int32 StopSpoolingSoundFade( int32 seconds )
{
	int32 result = 0;

	if ( seconds < 0 )
	{
		result = -1;
	}
	else if ( seconds == 0 )	// just stop normally
	{
		result = StopSpoolingSound();
	}
	else if ( !spoolerRunning )
	{
		result = -1;
	}
	else
	{
		spoolerFaderTime = seconds;
		spoolerFading = TRUE;
		SendSignal( spoolerFaderThread, spoolerStopOnFadeSignal );
	}

	return ( result );
}

/*
**	StopSpoolOnFadeThread()
*/
static void StopSpoolOnFadeThread( void )
{
	int32 			SignalIn;
	Item 			myParent = THREAD_PARENT;

	if (OpenAudioFolio())
	{
		// ERROR
	}

	spoolerStopOnFadeSignal = AllocSignal(0);
	spoolerFaderQuitSignal = AllocSignal(0);

	while ( true )
	{
		SignalIn = WaitSignal( spoolerStopOnFadeSignal | spoolerFaderQuitSignal );

		if ( SignalIn & spoolerFaderQuitSignal )
		{
			break;
		}

		spoolerEnvPoints[2].dtpr_Time = 200 + (1000 * spoolerFaderTime);
		ReleaseInstrument( spoolerEnvIns, 0 );

		SleepAudioTicks( (spoolerFaderTime * 240) + 20 ); // Just hang out!

		SendSignal( spoolerThread, spoolerStopSignal );
	}

	if ( spoolerStopOnFadeSignal ) FreeSignal( spoolerStopOnFadeSignal );
	spoolerStopOnFadeSignal = 0;
	if ( spoolerFaderQuitSignal ) FreeSignal( spoolerFaderQuitSignal );
	spoolerFaderQuitSignal = 0;

	CloseAudioFolio();

	/* Tell parent task that I've cleaned up and can be killed. */

	SendSignal(myParent, spoolerFaderOuttaHereSignal );

	/* Wait for parent to kill me (quick and painlessly, without memory loss, hopefully. */

	WaitSignal(0);
}


/****************************************/
/* Sound Spooling Background Thread		*/
/****************************************/

/*
**	SpoolSoundFileThread()
*/
static void SpoolSoundFileThread( void )
{
	int32 			result = 0;
	int32 			SignalIn, spoolServiceSignal;
	Item 			myParent = THREAD_PARENT;
	Item			spoolerEnvAttachment;
	Item			spoolerEnvelope; 

	spoolerRunning = false;

	/* Initialize audio, return if error. */

	if (OpenAudioFolio())
	{
		// ERROR 
	}

	/* Set up Sound File Player Once at Beginning */

	spoolerSFP = CreateSoundFilePlayer ( NUMBUFFS, BUFSIZE, NULL );

	// Set room aside for later use

	spoolerRoomIns = LoadInstrument( spoolSaveFileName, 0, 100 ); 

	/* Set up spooler signals */

	spoolerStartSignal = AllocSignal(0);
	spoolerStopSignal = AllocSignal(0);
	spoolerQuitSignal = AllocSignal(0);
	
	spoolServiceSignal = 0;		// Allocated by Spool File Player

	/* Here's our infinite loop - keep waiting or playing until it is time to quit */

	while ( true )
	{
		SignalIn = WaitSignal( spoolerStartSignal | spoolerStopSignal | spoolerQuitSignal );

		if ( SignalIn & spoolerQuitSignal )
		{
			break;
		}

		if (SignalIn & spoolerStopSignal) SendSignal (myParent, spoolerHasStoppedSignal);
		/*
		 * Badly-synced stop signal arrived.  Tell upstairs
		 * that we're done, goldurnit.   ewhac 9311.19
		 */
			
		if (!(SignalIn & spoolerStartSignal)) continue;
		/*
		 * Badly-synced stop signal arrived, or other
		 * signal we don't know how to deal with at this
		 * time.  Cycle over.  ewhac 9311.03
		 */	

		// Got a start spooling signal - so do it!

		spoolerRunning = true;

		if ( spoolerRoomIns >= 0 )
		{
			FreeInstrument( spoolerRoomIns );
			spoolerRoomIns = -1;
		}

		result = LoadSoundFile( spoolerSFP, spoolerFileName );

		/* Set up our envelope control over it */

		result = ConnectInstruments ( spoolerEnvIns, "Output", spoolerSFP->sfp_SamplerIns, "Amplitude");

		spoolerEnvPoints[0].dtpr_Data = 0;
		spoolerEnvPoints[0].dtpr_Time = 0;

		spoolerEnvPoints[1].dtpr_Data = spoolerAmplitude;
		spoolerEnvPoints[1].dtpr_Time = 200;

		spoolerEnvPoints[2].dtpr_Data = 0;
		spoolerEnvPoints[2].dtpr_Time = 400;

		spoolerEnvelope = CreateEnvelope( spoolerEnvPoints, 3, 1, 1 );

		spoolerEnvAttachment = AttachEnvelope( spoolerEnvIns, spoolerEnvelope, "Env" );

		result = StartInstrument( spoolerEnvIns, NULL );

		SignalIn = 0;
		
		while ( spoolerNumReps > 0 )
		{
			spoolerNumReps--;

			result = RewindSoundFile( spoolerSFP );

			result = StartSoundFile( spoolerSFP, spoolerAmplitude );

			/* Keep playing until no more samples. */
			spoolServiceSignal = 0;
			
			do
			{
				if ( spoolServiceSignal )
				{
					SignalIn = WaitSignal(spoolServiceSignal | spoolerStopSignal | spoolerQuitSignal );
				}
				else
				{
					SignalIn = 0;
				}

				if ( (SignalIn & spoolerStopSignal) || (SignalIn & spoolerQuitSignal))
				{
					spoolerNumReps = 0;
					break;
				}

				// signal must be a service routine

				result = ServiceSoundFile(spoolerSFP, SignalIn, &spoolServiceSignal);

			} while ( spoolServiceSignal );


			result = StopSoundFile (spoolerSFP);
			
#if 0
			if ( spoolerNumReps )
			{
				result = RewindSoundFile( spoolerSFP );
			}
#endif
		}

		StopInstrument( spoolerEnvIns, 0 );

		DetachEnvelope( spoolerEnvAttachment );

		DeleteEnvelope( spoolerEnvelope );

		result = UnloadSoundFile( spoolerSFP );

		if ( spoolerRoomIns < 0 )
		{
			spoolerRoomIns = LoadInstrument( spoolSaveFileName, 0, 100 );
		}

		spoolerRunning = false;

		if ( SignalIn & spoolerStopSignal )
		{
			SendSignal(myParent, spoolerHasStoppedSignal);
		}

		if ( SignalIn & spoolerQuitSignal )
		{
			break;
		}
	}

	/* Done - send signal when I'm finished so process can kill me. */

	if ( spoolerStartSignal ) FreeSignal( spoolerStartSignal );
	spoolerStartSignal = 0;
	if ( spoolerStopSignal ) FreeSignal( spoolerStopSignal );
	spoolerStopSignal = 0;
	if ( spoolerQuitSignal ) FreeSignal( spoolerQuitSignal );
	spoolerQuitSignal = 0;

	if ( spoolerSFP ) DeleteSoundFilePlayer( spoolerSFP );
	spoolerSFP = 0;

	if ( spoolerRoomIns >= 0 )
	{
		FreeInstrument( spoolerRoomIns );
		spoolerRoomIns = -1;
	}

	CloseAudioFolio();

	/* Tell parent task that I've cleaned up and can be killed. */

	SendSignal(myParent, spoolerOuttaHereSignal);

	/* Wait for parent to kill me (quick and painlessly, without memory loss, hopefully. */

	WaitSignal(0);
}

/****************************************/
/* RAM Resident Sound Functions			*/
/****************************************/

/*
**	LoadRAMSound()
*/
static int32 LoadRAMSound( LoadRAMSoundPtr loadSndPtr )
{
 	int32 	result = noErr;
	int32 	i, soundSlot;
	char	*instrName;

	if ( loadSndPtr->soundID == 0 || loadSndPtr->soundFileName == 0 || loadSndPtr->amplitude == 0 )
	{
		// ERROR
	}

	if ( loadSndPtr->soundID == 0 )
	{
		return (-1);
	}

	soundSlot = -1;
	
	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID == 0 )
		{
			soundSlot = i;
			break;
		}
	}

	if ( soundSlot == -1 )
	{
		return (-1);
	}

	sounds[soundSlot].soundID = loadSndPtr->soundID;
	sounds[soundSlot].amplitude = loadSndPtr->amplitude;
	sounds[soundSlot].balance = loadSndPtr->balance;
	sounds[soundSlot].frequency = loadSndPtr->frequency;

	sounds[soundSlot].sample = LoadSample( loadSndPtr->soundFileName );

	instrName = SelectSamplePlayer( sounds[soundSlot].sample, loadSndPtr->frequency );
	
	if (instrName == NULL)
	{
		// ERROR
	}
	
	strcpy( sounds[soundSlot].instrName, instrName );

	sounds[soundSlot].channel[0] = -1;	// The important one; if this is -1, we haven't assigned a channel
	sounds[soundSlot].channel[1] = -1;

	AssignChannels( &sounds[soundSlot], false );	// assign a channel if possible

	return ( result );
}

/*
**	AssignChannels()
*/
static int32 AssignChannels( SoundDataPtr theSound, int32 mustSucceed )
{
	int32 	result = noErr;
	char	aName[50];
	int32	aSlot, anotherSlot, i;

	if ( theSound->channel[0] >= 0 )
	{
		return noErr;	// already assigned
	}

	// Check Our Resources

	aSlot = FindEmptyChannel();
	theSound->instrument = MyLoadInstrument( theSound->instrName );

	if ( theSound->instrument == AF_ERR_NORSRC || ( theSound->instrument >= 0 && aSlot == -1 ) )
	{
		if ( mustSucceed == false )
		{
			return noErr;
		}
		else
		{
			while ( numSoundsAssigned > 0 && (theSound->instrument == AF_ERR_NORSRC || ( theSound->instrument >= 0 && aSlot == -1 ) ) )
			{
				for ( i = 0; i < kMaxRamSounds; i++ )
				{
					if ( sounds[i].soundID == assignQueue[0] )
					{
						UnassignChannels( &sounds[i] );
						break;
					}
				}

				if ( theSound->instrument < 0 )
				{
					theSound->instrument = MyLoadInstrument( theSound->instrName );
				}
				
				aSlot = FindEmptyChannel();
			}
		}
	}
		
	if ( aSlot == -1 )
	{		
		if ( theSound->instrument >= 0 )
		{
			MyUnloadInstrument( theSound );
		}
		
		return (-1);
	}

	FlushInstrument( theSound->instrument );

	theSound->attachment = AttachSample( theSound->instrument, theSound->sample, 0 );

	theSound->freqKnob = -1;

	// Assume we have a mono sound

	sprintf( aName, "Input%d", aSlot );
	result = ConnectInstruments( theSound->instrument, "Output", MixerIns, aName );
	
	if ( result == noErr )
	{
		theSound->channel[0] = aSlot;
	}

	if ( result ) 	// Probably a stereo sound (i.e. can't find "Output")
	{
		// Stero Sound; assume instrument outputs are "LeftOutput" and "RightOutput"

		sprintf( aName, "Input%d", aSlot );
		result = ConnectInstruments( theSound->instrument, "LeftOutput", MixerIns, aName );
		
		if ( result == noErr )
		{
			theSound->channel[0] = aSlot;
		}

		// Allocate a second channel for the sound

		anotherSlot = FindEmptyChannel();
		
		if ( anotherSlot == -1 )
		{
			if ( mustSucceed )
			{
				return (-1);
			}
			else
			{
				result = DisconnectInstruments( theSound->instrument, "LeftOutput", MixerIns, aName );
				DetachSample( theSound->attachment );
				theSound->attachment = -1;
				MyUnloadInstrument( theSound );
				theSound->instrument = -1;
				theSound->channel[0] = -1;
				
				return noErr;
			}
		}

		sprintf( aName, "Input%d", anotherSlot );
		result = ConnectInstruments( theSound->instrument, "RightOutput", MixerIns, aName );
		
		if ( result == noErr )
		{
			theSound->channel[1] = anotherSlot;
		}
	}

	// Make Sure all our mixer levels are correct

	result = SetMixerLevels( soundLibraryMainLevel );

	if ( theSound->channel[0] != -1 && result == noErr )
	{
		assignQueue[numSoundsAssigned++] = theSound->soundID;	// add to end of queue
	}
		
	return ( result );
}

/*
**	FindEmptyChannel()
*/
static int32 FindEmptyChannel()
{
	int32	foundChannel = -1;
	int32	i, j;
	int32	channelIsHit = 0;

	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID )
		{
			for ( j = 0; j < 1; j++ )
			{
				if ( sounds[i].channel[j] >= 0 )
				{
					channelIsHit |= (1 << sounds[i].channel[j]);
				}
			}
		}
	}

	for ( i = 0; i < kNumChannels; i++  )
	{
		if ( (channelIsHit & (1 << i)) == 0 )
		{
			foundChannel = i;
			break;
		}
	}

	return foundChannel;
}

/*
**	MyLoadInstrument()
*/
static Item MyLoadInstrument( char *instrName )
{
	int32	 result = noErr;
	int32 	i;

	for ( i = 0; i < numCachedInstrs; i++ )
	{
		if ( strcmp( instrName, cachedInstrName[i] ) == 0 )
		{
			return AllocInstrument( cachedInstrItem[i], 100 );
		}
	}

	if (numCachedInstrs < MAX_CACHED_INSTRUMENTS)
	{
		strcpy( cachedInstrName[numCachedInstrs], instrName );
		cachedInstrItem[numCachedInstrs] = LoadInsTemplate( instrName, 0 );

		return AllocInstrument( cachedInstrItem[numCachedInstrs++], 100 );
	}
	else
	{
		return LoadInstrument( instrName, 0, 100 );
	}
		
	return (-1);
}

/*
**	MyUnloadInstrument()
*/
static int32 MyUnloadInstrument( SoundDataPtr theSound )
{
	int32 	i;

	for ( i = 0; i < numCachedInstrs; i++ )
	{
		if ( strcmp( theSound->instrName, cachedInstrName[i] ) == 0 )
		{
			return FreeInstrument( theSound->instrument );
		}
	}

	return UnloadInstrument( theSound->instrument );
}

/*
**	UnloadRAMSound()
*/
static int32 UnloadRAMSound( int32 soundID )
{
	int32	 result = noErr;
	int32 	i, foundSlot = -1;

	if ( soundID == 0 )
	{
		return (-1);
	}

	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID == soundID )
		{
			foundSlot = i;

			UnassignChannels( &sounds[i] );

			UnloadSample( sounds[i].sample );
			sounds[i].sample = -1;

			sounds[i].soundID = 0;
		}
	}

	if ( foundSlot == -1 )
	{
		return (-1);
	}

	return result;
}

/*
**	UnassignChannels()
*/
static int32 UnassignChannels( SoundDataPtr theSound )
{
	char	aName[50];
	int32	result = noErr;
	int32	i;
	int32	channel0, channel1;

	if ( theSound->channel[0] == -1 )
	{
		return noErr;
	}

	for ( i = 0; i < numSoundsAssigned; i++ )
	{
		if ( assignQueue[i] == theSound->soundID )
		{
			numSoundsAssigned--;
			memcpy( (void *) &assignQueue[i], (void *) &assignQueue[i+1], (numSoundsAssigned - i) * sizeof(int32) );
			break;
		}
	}

	channel0 = theSound->channel[0];
	channel1 = theSound->channel[1];

	theSound->channel[0] = -1;
	theSound->channel[1] = -1;

	// Set Mixer Levels To Zero before we kill things

	result = SetMixerLevels( soundLibraryMainLevel );

	if ( channel1  == -1 )	// mono
	{
		sprintf( aName, "Input%d", channel0 );
		result = DisconnectInstruments( theSound->instrument, "Output", MixerIns, aName );
	}
	else	// stereo
	{
		sprintf( aName, "Input%d", channel0 );
		result = DisconnectInstruments( theSound->instrument, "LeftOutput", MixerIns, aName );

		sprintf( aName, "Input%d", channel1 );
		result = DisconnectInstruments( theSound->instrument, "RightOutput", MixerIns, aName );
	}

	StopInstrument( theSound->instrument, 0 );

	if ( theSound->freqKnob != -1 )
	{
		ReleaseKnob( theSound->freqKnob );
		theSound->freqKnob = -1;
	}

	DetachSample( theSound->attachment );
	theSound->attachment = -1;
	MyUnloadInstrument( theSound );
	theSound->instrument = -1;

	return result;
}


TagArg FixedRAMSoundTags[] =
{
	{ AF_TAG_AMPLITUDE, 0},
	{ 0, 0 }
};

TagArg VariableRAMSoundTags[] =
{
	{ AF_TAG_AMPLITUDE, 0},
	{ AF_TAG_RATE, 0},
	{ 0, 0 }
};

/*
**	StartRAMSound()
*/
static int32 StartRAMSound( int32 soundID )
{
 	int32 	result = noErr;
	int32 	i, soundSlot;

	soundSlot = -1;
	
	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID == soundID )
		{
			soundSlot = i;
			break;
		}
	}

	if ( soundSlot == -1 )
	{
		return (-1);
	}

	if ( AssignChannels( &sounds[soundSlot], true ) )
	{
		return (-1);
	}

	if ( sounds[soundSlot].frequency ) 
	{
		VariableRAMSoundTags[0].ta_Arg = (int32 *) MAXAMPLITUDE;
		VariableRAMSoundTags[1].ta_Arg = (int32 *) sounds[soundSlot].frequency;
		
		StartInstrument( sounds[soundSlot].instrument, &VariableRAMSoundTags[0] );

		sounds[soundSlot].freqKnob = GrabKnob( sounds[soundSlot].instrument, "Frequency" );
	}
	else
	{
		FixedRAMSoundTags[0].ta_Arg = (int32 *) MAXAMPLITUDE;
		StartInstrument( sounds[soundSlot].instrument, &FixedRAMSoundTags[0] );
	}
	
	return result;
}

/*
**	StopRAMSound()
*/
static int32 StopRAMSound( int32 soundID )
{
 	int32 	result = noErr;
	int32 	i, soundSlot;

	soundSlot = -1;
	
	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID == soundID )
		{
			soundSlot = i;
			break;
		}
	}

	if ( soundSlot == -1 )
	{
		return (-1);
	}

	StopInstrument( sounds[soundSlot].instrument, NULL);

	return result;
}

/*
**	SetRAMSoundFreq()
*/
static int32 SetRAMSoundFreq( SetRAMSoundPtr setSndPtr )
{
 	int32 	result = noErr;
	int32 	i, soundSlot;

	soundSlot = -1;
	
	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID == setSndPtr->soundID )
		{
			soundSlot = i;
			break;
		}
	}

	if ( soundSlot == -1 )
	{
		return (-1);
	}

	if ( sounds[soundSlot].frequency == 0 )
	{
		return (-1);
	}

	if ( sounds[soundSlot].frequency > 65535 )
	{
		sounds[soundSlot].frequency = 65535;
	}

	if ( sounds[soundSlot].frequency < 100 )
	{
		sounds[soundSlot].frequency = 100;
	}

	sounds[soundSlot].frequency = setSndPtr->level;

	if ( sounds[soundSlot].freqKnob!= -1 )
	{
		TweakRawKnob( sounds[soundSlot].freqKnob, sounds[soundSlot].frequency );
	}

	return result;
}

/*
**	SetRAMSoundAmpl()
*/
static int32 SetRAMSoundAmpl( SetRAMSoundPtr setSndPtr )
{
 	int32 	result = noErr;
	int32 	i, soundSlot;

	soundSlot = -1;
	
	for ( i = 0; i < kMaxRamSounds; i++ )
	{
		if ( sounds[i].soundID == setSndPtr->soundID )
		{
			soundSlot = i;
			break;
		}
	}

	if ( soundSlot == -1 )
	{
		return (-1);
	}

	if ( sounds[soundSlot].frequency == 0 )
	{
		return (-1);
	}

	sounds[soundSlot].amplitude = setSndPtr->level;

	result = SetMixerLevels( soundLibraryMainLevel );

	return result;
}

/*
**	FlushInstrument()
*/
static int32 FlushInstrument( Item SamplerIns )
{
	Item SampleItem, Attachment;
	int32 result;

	result=0;

	/* make short sample */
	SampleItem = MakeSample( 32, NULL );
	
	Attachment = AttachSample(SamplerIns, SampleItem, 0);
	

	/* play it and let it go to silence */
	result = StartInstrument( SamplerIns, NULL );
	
	SleepAudioTicks( 6 );  /* give it time to get into silence */
	StopInstrument( SamplerIns, NULL);

	UnloadSample( SampleItem );
	DetachSample( Attachment );
	
	return result;
}