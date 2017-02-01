#ident @(#)libedc.mk	1.2 02/10/19 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	edc_ecc
CPPOPTS +=	-Iold
#
SUNPROCOPTOPT=	-fast -xarch=generic
GCCOPTOPT=	-O3  -fexpensive-optimizations
#
CFILES=		edc_ecc.c
HFILES=
#include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
