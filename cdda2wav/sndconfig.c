/* @(#)sndconfig.c	1.13 02/09/13 Copyright 1998,1999,2000 Heiko Eissfeldt */
#ifndef lint
static char     sccsid[] =
"@(#)sndconfig.c	1.13 02/09/13 Copyright 1998,1999,2000 Heiko Eissfeldt";

#endif
/* os dependent functions */
#include "config.h"
#include <stdio.h>
#include <strdefs.h>
#include <fctldefs.h>
#include <unixstd.h>
#include <sys/ioctl.h>
#if	!defined __CYGWIN32__
# include <timedefs.h>
#endif
#include <schily.h>


/* soundcard setup */
#if defined (HAVE_SOUNDCARD_H) || defined (HAVE_LINUX_SOUNDCARD_H) || defined (HAVE_SYS_SOUNDCARD_H) || defined (HAVE_MACHINE_SOUNDCARD_H)
# if defined (HAVE_SOUNDCARD_H)
#  include <soundcard.h>
# else
#  if defined (HAVE_MACHINE_SOUNDCARD_H)
#   include <machine/soundcard.h>
#  else
#   if defined (HAVE_SYS_SOUNDCARD_H)
#    include <sys/soundcard.h>
#   else
#    if defined (HAVE_LINUX_SOUNDCARD_H)
#     include <linux/soundcard.h>
#    endif
#   endif
#  endif
# endif
#endif

#include "mytype.h"
#include "byteorder.h"
#include "lowlevel.h"
#include "global.h"
#include "sndconfig.h"

#ifdef	ECHO_TO_SOUNDCARD
#   if defined(__CYGWIN32__)
#include <windows.h>
#include "mmsystem.h"
#endif

static char snd_device[200] = SOUND_DEV;

int set_snd_device(devicename)
	const char *devicename;
{
	strncpy(snd_device, devicename, sizeof(snd_device));
	return 0;
}

#if	defined __CYGWIN32__
static HWAVEOUT	DeviceID;
#define WAVEHDRS	3
static WAVEHDR	wavehdr[WAVEHDRS];
static unsigned lastwav = 0;

static int check_winsound_caps __PR((int bits, double rate, int channels));

static int check_winsound_caps(bits, rate, channels)
	int bits;
	double rate;
	int channels;
{
  int result = 0;

  WAVEOUTCAPS caps;

  /* get caps */
  if (waveOutGetDevCaps(0, &caps, sizeof(caps))) {
     fprintf(stderr, "cannot get soundcard capabilities!\n");
     return 1;
  }

  /* check caps */
  if ((bits == 8 && !(caps.dwFormats & 0x333)) ||
      (bits == 16 && !(caps.dwFormats & 0xccc))) {
      fprintf(stderr, "%d bits sound are not supported\n", bits);
      result = 2;
  }

  if ((channels == 1 && !(caps.dwFormats & 0x555)) ||
      (channels == 2 && !(caps.dwFormats & 0xaaa))) {
      fprintf(stderr, "%d sound channels are not supported\n", channels);
      result = 3;
  }

  if ((rate == 44100.0 && !(caps.dwFormats & 0xf00)) ||
      (rate == 22050.0 && !(caps.dwFormats & 0xf0)) ||
      (rate == 11025.0 && !(caps.dwFormats & 0xf))) {
      fprintf(stderr, "%d sample rate is not supported\n", (int)rate);
      result = 4;
  }

  return result;
}
#endif
#endif

#ifdef	HAVE_SUN_AUDIOIO_H
# include <sun/audioio.h>
#endif
#ifdef	HAVE_SYS_AUDIOIO_H
# include <sys/audioio.h>
#endif

int init_soundcard(rate, bits)
	double rate;
	int bits;
{
#ifdef	ECHO_TO_SOUNDCARD
  if (global.echo) {
# if	defined(HAVE_OSS) && HAVE_OSS == 1
    if (open_snd_device() != 0) {
        errmsg("Cannot open sound device '%s'\n", snd_device);
        global.echo = 0;
    } else { 
	/* This the sound device initialisation for 4front Open sound drivers */

	int dummy;
	int garbled_rate = rate;
	int stereo = (global.channels == 2);
	int myformat = bits == 8 ? AFMT_U8 :
        	(MY_LITTLE_ENDIAN ? AFMT_S16_LE : AFMT_S16_BE);
	int mask;

	if (ioctl(global.soundcard_fd, (int)SNDCTL_DSP_GETBLKSIZE, &dummy) == -1) {
	    fprintf(stderr, "Cannot get blocksize for %s\n", snd_device);
	    global.echo = 0;
	}
	if (ioctl(global.soundcard_fd, (int)SNDCTL_DSP_SYNC, NULL) == -1) {
	    fprintf(stderr, "Cannot sync for %s\n", snd_device);
	    global.echo = 0;
	}

	/* check, if the sound device can do the requested format */
	if (ioctl(global.soundcard_fd, (int)SNDCTL_DSP_GETFMTS, &mask) == -1) {
		perror("fatal error:");
		return -1;
	}
	if ((mask & myformat) == 0) {
		fprintf(stderr, "sound format (%d bits signed) is not available\n", bits);
		if ((mask & AFMT_U8) != 0) {
			bits = 8;
			myformat = AFMT_U8;
		}
	}
	if (ioctl(global.soundcard_fd, (int)SNDCTL_DSP_SETFMT, &myformat) == -1) {
	    fprintf(stderr, "Cannot set %d bits/sample for %s\n",bits, snd_device);
	    global.echo = 0;
	}

	/* limited sound devices may not support stereo */
	if (stereo
	    && ioctl(global.soundcard_fd, (int)SNDCTL_DSP_STEREO, &stereo) == -1) {
	    fprintf(stderr, "Cannot set stereo mode for %s\n", snd_device);
	    stereo = 0;
	}
	if (!stereo
            && ioctl(global.soundcard_fd, (int)SNDCTL_DSP_STEREO, &stereo) == -1) {
	    fprintf(stderr, "Cannot set mono mode for %s\n", snd_device);
	    global.echo = 0;
	}

	/* set the sample rate */
	if (ioctl(global.soundcard_fd, (int)SNDCTL_DSP_SPEED, &garbled_rate) == -1) {
	    fprintf(stderr, "Cannot set rate %d.%2d Hz for %s\n",
		(int)rate, (int)(rate*100)%100, snd_device);
	    global.echo = 0;
	}
	if ( abs((long)rate - garbled_rate) > rate / 20) {
	    fprintf(stderr, "sound device: next best sample rate is %d\n",garbled_rate);
	}
    }

# else /* HAVE_OSS */

#  if defined	HAVE_SYS_AUDIOIO_H || defined HAVE_SUN_AUDIOIO_H
	/* This is the SunOS / Solaris and compatibles sound initialisation */

    if ((global.soundcard_fd = open(snd_device, O_WRONLY, 0)) == EOF) {
	perror("");
        fprintf(stderr, "Cannot open %s\n",snd_device);
        global.echo = 0;
    } else { 

        audio_info_t            info;

#   if	defined	(AUDIO_INITINFO) && defined (AUDIO_ENCODING_LINEAR)
        AUDIO_INITINFO(&info);
        info.play.sample_rate = rate;
        info.play.channels = global.channels;
        info.play.precision = bits;
        info.play.encoding = AUDIO_ENCODING_LINEAR;
        info.play.pause = 0;
        info.record.pause = 0;
        info.monitor_gain = 0;
        if (ioctl(global.soundcard_fd, AUDIO_SETINFO, &info) < 0) {
	    fprintf(stderr, "Cannot init %s (sun)\n", snd_device);
	    global.echo = 0;
	}
#   else
	fprintf(stderr, "Cannot init sound device with 44.1 KHz sample rate on %s (sun compatible)\n", snd_device);
	global.echo = 0;
#   endif
    }
#  else /* SUN audio */
#   if defined(__CYGWIN32__)
    /* Windows sound info */

    MMRESULT mmres;
    WAVEFORMATEX wavform;

    if (waveOutGetNumDevs() < 1) {
	fprintf( stderr, "no sound devices available!\n");
	global.echo = 0;
	return 1;
    }

    /* check capabilities */
    if (check_winsound_caps(bits, rate, global.channels) != 0) {
	fprintf( stderr, "soundcard capabilities are not sufficient!\n");
	global.echo = 0;
	return 1;
    }

    wavform.wFormatTag = WAVE_FORMAT_PCM;
    wavform.nChannels = global.channels;
    wavform.nSamplesPerSec = (int)rate;
    wavform.wBitsPerSample = bits;
    wavform.cbSize = 0;
    wavform.nAvgBytesPerSec = (int)rate * global.channels *
				(wavform.wBitsPerSample / 8);
    wavform.nBlockAlign = global.channels * (wavform.wBitsPerSample / 8);
  
    DeviceID = 0;
    mmres = waveOutOpen(&DeviceID, 0, &wavform, 0, 0, 0);
    if (mmres) {
	char erstr[329];

	waveOutGetErrorText(mmres, erstr, sizeof(erstr));
	fprintf( stderr, "soundcard open error: %s!\n", erstr);
	global.echo = 0;
	return 1;
    }

    global.soundcard_fd = 0;

    /* init all wavehdrs */
    { int i;
	for (i=0; i < WAVEHDRS; i++) {
	    wavehdr[i].dwBufferLength = (global.channels*(bits/ 8)*(int)rate*
		    		global.nsectors)/75;
	    wavehdr[i].lpData = malloc(wavehdr[i].dwBufferLength);
	    if (wavehdr[i].lpData == NULL) {
		    fprintf(stderr, "no memory for sound buffers available\n");
 		    waveOutReset(0);
		    waveOutClose(DeviceID);
		    return 1;
	    }
	    
	    wavehdr[i].dwLoops = 0;
	    wavehdr[i].dwFlags = WHDR_DONE;
	    wavehdr[i].dwBufferLength = 0;
	}
    }

#   endif /* CYGWIN Windows sound */
#  endif /* else SUN audio */
# endif /* else HAVE_OSS */
  }
#endif /* ifdef ECHO_TO_SOUNDCARD */
  return 0;
}

int open_snd_device ()
{
#if	defined ECHO_TO_SOUNDCARD && !defined __CYGWIN32__
	return (global.soundcard_fd = open(snd_device, O_WRONLY
#ifdef	linux
		/* Linux BUG: the sound driver open() blocks, if the device is in use. */
		 | O_NONBLOCK
#endif
		, 0)) < 0;

#else
	return 0;
#endif
}

int close_snd_device ()
{
#if	defined __CYGWIN32__ && defined ECHO_TO_SOUNDCARD
 waveOutReset(0);
 return waveOutClose(DeviceID);
#else
#ifdef	ECHO_TO_SOUNDCARD
 return close(global.soundcard_fd);
#else
 return 0;
#endif /* ifdef ECHO_TO_SOUNDCARD */
#endif
}

int write_snd_device (buffer, todo)
	char *buffer;
	unsigned todo;
{
	int result = 0;
#ifdef	ECHO_TO_SOUNDCARD
#if	defined __CYGWIN32__
	MMRESULT mmres;

	wavehdr[lastwav].dwBufferLength = todo;
	memcpy(wavehdr[lastwav].lpData, buffer, todo);

	mmres = waveOutPrepareHeader(DeviceID, &wavehdr[lastwav], sizeof(WAVEHDR));
	if (mmres) {
		char erstr[129];

		waveOutGetErrorText(mmres, erstr, sizeof(erstr));
		fprintf( stderr, "soundcard prepare error: %s!\n", erstr);
		return 1;
	}

	mmres = waveOutWrite(DeviceID, &wavehdr[lastwav], sizeof(WAVEHDR));
	if (mmres) {
		char erstr[129];

		waveOutGetErrorText(mmres, erstr, sizeof(erstr));
		fprintf( stderr, "soundcard write error: %s!\n", erstr);
		return 1;
	}
	lastwav = (lastwav + 1) % WAVEHDRS;
	result = mmres;
#else
	int retval2;

	do {
		fd_set writefds[1];
		struct timeval timeout2;
		int wrote;

		timeout2.tv_sec = 0;
		timeout2.tv_usec = 4*120000;

		FD_ZERO(writefds);
		FD_SET(global.soundcard_fd, writefds);
		retval2 = select(global.soundcard_fd + 1,
				 NULL, writefds, NULL, &timeout2);
		switch (retval2) {
			default:
			case -1: perror ("select failed");
			/* fall through */
			case 0: /* timeout */
				result = 2;
				goto outside_loop;
			case 1: break;
		}
		wrote = write(global.soundcard_fd, buffer, todo);
		if (wrote <= 0) {
			perror( "cant write audio");
			result = 1;
			goto outside_loop;
		} else {
			todo -= wrote;
			buffer += wrote;
		}
	} while (todo > 0);
outside_loop:
	;
#endif
#endif
	return result;
}

