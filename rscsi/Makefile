#ident @(#)Makefile	1.2 00/11/19 
###########################################################################
#include		$(MAKE_M_ARCH).def
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		sbin
TARGET=		rscsi
INSMODE=	4711
INSUSR=		root
#CPPOPTS +=	-DUSE_REMOTE
CFILES=		rscsi.c
#HFILES=
LIBS=		-lscg -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
