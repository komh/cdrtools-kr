/* @(#)sndfile.h	1.3 00/04/10 Copyright 1998,1999 Heiko Eissfeldt */
/* generic soundfile structure */

struct soundfile {
  int (* InitSound) __PR(( int audio, long channels, unsigned long rate, long nBitsPerSample, unsigned long expected_bytes));
  int (* ExitSound) __PR(( int audio, unsigned long nBytesDone ));
  unsigned long (* GetHdrSize) __PR(( void ));
  int (* WriteSound) __PR(( int audio, unsigned char *buf, unsigned long BytesToDo ));
  unsigned long (* InSizeToOutSize) __PR(( unsigned long BytesToDo ));

  int need_big_endian;
};
