/* @(#)mycdrom.h	1.5 02/09/04 Copyright 1998,1999 Heiko Eissfeldt */
#if defined(__linux__)
# include <linux/cdrom.h>
#else
# if defined HAVE_SYS_CDIO_H
#  include <sys/cdio.h>

#  if (defined (__sun) && defined (SVR4))
#   if 0
/* just for info */
/* Sun has this cdda reading ioctl: CDROMCDDA */
/*
 * Definition of CD-DA structure
 */
struct cdrom_cdda {
 unsigned int cdda_addr;
 unsigned int cdda_length;
 caddr_t  cdda_data;
 unsigned char cdda_subcode;
};
/*
To get the subcode information related to CD-DA data, the following values are
 appropriate for the cdda_subcode field:

CDROM_DA_NO_SUBCODE
CD-DA data with no subcode.

CDROM_DA_SUBQ
CD-DA data with sub Q code.

CDROM_DA_ALL_SUBCODE
CD-DA data with all subcode.

CDROM_DA_SUBCODE_ONLY
All subcode only.

To allocate the memory related to CD-DA and/or subcode data, the following
 values are appropriate for each data block transferred:

CD-DA data with no subcode
2352 bytes

CD-DA data with sub Q code
2368 bytes

CD-DA data with all subcode
2448 bytes

All subcode only
96 bytes
*/

#   endif /* if 0 */
#  else /* not Sun SVR4 */
#   if defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__
#    if !defined CDIOCREADAUDIO
#     undef HAVE_IOCTL_INTERFACE
#    else

#define CDROM_LBA	CD_LBA_FORMAT
#define CDROM_MSF	CD_MSF_FORMAT
#define CDROM_DATA_TRACK	0x04
#define CDROM_LEADOUT	0xAA

#define CDROMSTOP	CDIOCSTOP
#define CDROMSTART	CDIOCSTART
#define CDROMREADTOCHDR	CDIOREADTOCHEADER
#define CDROMREADTOCENTRY	CDIOREADTOCENTRY
#define CDROMPLAYMSF	CDIOCPLAYMSF
#define CDROMREADAUDIO	CDIOCREADAUDIO
#define CDROM_GET_MCN	CDIOCREADSUBCHANNEL
#define CDROMSUBCHNL	CDIOCREADSUBCHANNEL

#ifndef	CDIOREADTOCENTRY
#define CDIOREADTOCENTRY	CDIOREADTOCENTRYS
#endif

#define cdrom_tochdr ioc_toc_header
#define cdth_trk0	starting_track
#define cdth_trk1	ending_track

#define cdrom_tocentry ioc_read_toc_single_entry
#define cdte_track	track
#define cdte_format	address_format
#define cdte_adr	entry.addr_type
#define cdte_ctrl	entry.control
#define cdte_addr	entry.addr

#define cdrom_read_audio ioc_read_audio
#define addr_format	address_format
#define buff		buffer

#define cdrom_msf	ioc_play_msf
#define cdmsf_min0	start_m
#define cdmsf_sec0	start_s
#define cdmsf_frame0	start_f
#define cdmsf_min1	end_m
#define cdmsf_sec1	end_s
#define cdmsf_frame1	end_f

#define cdrom_subchnl	ioc_read_subchannel
#define cdsc_audiostatus data->header.audio_status
#define cdsc_format	data->what.position.data_format
#define cdsc_adr	data->what.position.addr_type
#define cdsc_ctrl	data->what.position.control
#define cdsc_trk	data->what.position.track_number
#define cdsc_ind	data->what.position.index_number
#define cdsc_absaddr	data->what.position.absaddr
#define cdsc_reladdr	data->what.position.reladdr
#    endif /* defined CDIOCREADAUDIO */
#   else /* not *BSD */
#    undef HAVE_IOCTL_INTERFACE
#   endif /* not *BSD */
#  endif /* not SUN SVR4 */
# else /* HAVE_SYS_CDIO_H */
#  if defined HAVE_SUNDEV_SRREG_H
#   include <sundev/srreg.h>
#   if !defined CDROMCDDA
#    undef HAVE_IOCTL_INTERFACE
#   endif
#  else
#    undef HAVE_IOCTL_INTERFACE
#  endif
# endif /* not HAVE_SYS_CDIO_H */
#endif /* not linux */
