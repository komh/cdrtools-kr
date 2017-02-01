#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		readcd
CPPOPTS +=	-I../cdrecord -DUSE_PG
CPPOPTS +=	-DUSE_LARGEFILES
CFILES=		readcd.c cd_misc.c io.c \
		defaults.c \
		scsi_cdr.c \
		modes.c misc.c getnum.c
HFILES=		
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-ldeflt -lrscg -lscg -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
