#ident @(#)libunls_p.mk	1.1 00/03/25 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
INSDIR=		lib
TARGETLIB=	ulns_p
#CPPOPTS +=	-Istdio
COPTS +=	$(COPTGPROF)
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
