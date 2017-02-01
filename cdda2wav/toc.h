/* @(#)toc.h	1.7 02/04/08 Copyright 1998,1999 Heiko Eissfeldt */
#define MAXTRK	100	/* maximum of audio tracks (without a hidden track) */

extern unsigned cdtracks;
extern int have_CD_extra;
extern int have_CD_text;
extern int have_CDDB;

#if	!defined(HAVE_NETDB_H)
#undef	USE_REMOTE
#else
#define USE_REMOTE	1
int request_titles __PR((void));
#endif

int ReadToc __PR(( void ));
void Check_Toc __PR(( void ));
int TOC_entries __PR(( unsigned tracks, unsigned char *a, unsigned char *b,
		       int bvalid));
void toc_entry __PR(( unsigned nr, unsigned flag, unsigned tr,
		      unsigned char *ISRC,
		      unsigned long lba, int m, int s, int f ));
int patch_real_end __PR(( unsigned long sector ));
int no_disguised_audiotracks __PR((void));

int Get_Track __PR(( unsigned long sector ));
long FirstTrack  __PR(( void ));
long LastTrack  __PR(( void ));
long FirstAudioTrack  __PR(( void ));
long FirstDataTrack  __PR(( void ));
long LastAudioTrack  __PR(( void ));
long Get_EndSector  __PR(( unsigned long p_track ));
long Get_StartSector  __PR(( unsigned long p_track ));
long Get_AudioStartSector  __PR(( unsigned long p_track ));
long Get_LastSectorOnCd __PR(( unsigned long p_track ));
int CheckTrackrange __PR(( unsigned long from, unsigned long upto ));

int Get_Preemphasis __PR(( unsigned long p_track ));
int Get_Channels __PR(( unsigned long p_track ));
int Get_Copyright __PR(( unsigned long p_track ));
int Get_Datatrack __PR(( unsigned long p_track ));
int Get_Tracknumber __PR(( unsigned long p_track ));
unsigned char *Get_MCN __PR(( void ));
unsigned char *Get_ISRC __PR(( unsigned long p_track ));

unsigned find_an_off_sector __PR((unsigned lSector, unsigned SectorBurstVal));
void DisplayToc  __PR(( void ));
unsigned FixupTOC __PR((unsigned no_tracks));
void calc_cddb_id __PR((void));
void calc_cdindex_id __PR((void));
void Read_MCN_ISRC __PR(( void ));
unsigned ScanIndices __PR(( unsigned trackval, unsigned indexval, int bulk ));
int   handle_cdtext __PR(( void ));
int lba_2_msf __PR(( long lba, int *m, int *s, int *f));
