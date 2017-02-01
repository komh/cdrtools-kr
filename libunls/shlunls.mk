#ident @(#)shlunls.mk	1.1 00/03/25 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
CCOM=		cc
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
INSDIR=		lib
TARGETLIB=	unls
#CPPOPTS +=	-Istdio
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
