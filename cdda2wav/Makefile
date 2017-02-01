#ident @(#)Makefile	1.6 02/04/28 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdda2wav
CPPOPTS +=	-DFIFO -I../cdrecord $(CDDA2WAVDEFS)
CFILES=		cdda2wav.c interface.c semshm.c resample.c \
		toc.c wav.c sun.c raw.c md5c.c setuid.c ringbuff.c \
		sndconfig.c scsi_cmds.c aiff.c aifc.c \
		scsi_cdr.c cd_misc.c modes.c ioctl.c sha_func.c base64.c
HFILES=		cdda2wav.h interface.h semshm.h global.h resample.h \
		mytype.h toc.h wav.h sun.h raw.h md5.h setuid.h ringbuff.h \
		config.h saveargs.h sndfile.h aiff.h aifc.h \
		sndconfig.h scsi_cmds.h lowlevel.h byteorder.h mycdrom.h \
		base64.h sha.h mmsystem.h cd_text.c cd_extra.c
LIBS=		-lrscg -lscg -lparanoia -lschily $(SCSILIB) $(EXTRALIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man cdda2ogg.mk1

LCONFIG=	local.cnf
xall:		all
$(ARCHDIR)/local.cnf $(ARCHDIR)/lconfig.h:	../cdrecord/cdrecord.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################

_ARCH_=		$(_UNIQ)$(O_ARCH)
_HAVE_LINUX=	$(_ARCH_:$(_UNIQ)linux=1)
HAVE_LINUX=	$(_HAVE_LINUX:$(_ARCH_)%=%)
_HAVE_SUN_OS=	$(_ARCH_:$(_UNIQ)sunos%=1)
HAVE_SUN_OS=	$(_HAVE_SUN_OS:$(_ARCH_)%=%)
_HAVE_BSD_OS=	$(_ARCH_:$(_UNIQ)%bsd=1)
HAVE_BSD_OS=	$(_HAVE_BSD_OS:$(_ARCH_)%=%)
_HAVE_CYGWIN=	$(_ARCH_:$(_UNIQ)cygwin32_nt=1)
HAVE_CYGWIN=	$(_HAVE_CYGWIN:$(_ARCH_)%=%)
