/* @(#)readcd.c	1.53 02/11/30 Copyright 1987, 1995-2002 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)readcd.c	1.53 02/11/30 Copyright 1987, 1995-2002 J. Schilling";
#endif
/*
 *	Skeleton for the use of the scg genearal SCSI - driver
 *
 *	Copyright (c) 1987, 1995-2002 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <mconfig.h>
#include <stdio.h>
#include <standard.h>
#include <unixstd.h>
#include <stdxlib.h>
#include <strdefs.h>
#include <fctldefs.h>
#include <timedefs.h>
#include <signal.h>
#include <schily.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsimmc.h"
#define	qpto96	__nothing__
#include "cdrecord.h"
#undef	qpto96
#include "movesect.h"

char	cdr_version[] = "2.0";

#if     defined(PROTOTYPES)
#define UINT_C(a)	(a##u)
#define ULONG_C(a)	(a##ul)
#define USHORT_C(a)	(a##uh)
#define CONCAT(a,b)	a##b
#else
#define UINT_C(a)	((unsigned)(a))
#define ULONG_C(a)	((unsigned long)(a))
#define USHORT_C(a)	((unsigned short)(a))
#define CONCAT(a,b)	a/**/b
#endif

extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));

typedef struct {
	long	start;
	long	end;
	long	sptr;		/* sectors per transfer */
	BOOL	askrange;
	char	*name;
} parm_t;

typedef struct {
	int	errors;
	int	c2_errors;
	int	c2_maxerrs;
	int	c2_errsecs;
	int	c2_badsecs;
	int	secsize;
	BOOL	ismmc;
} rparm_t;

struct exargs {
	SCSI	*scgp;
	int	old_secsize;
	int	flags;
	int	exflags;
	char	oerr[3];
} exargs;

EXPORT	BOOL	cvt_cyls	__PR((void));
EXPORT	BOOL	cvt_bcyls	__PR((void));
EXPORT	void	print_defect_list __PR((void));
LOCAL	void	usage		__PR((int ret));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	intr		__PR((int sig));
LOCAL	void	exscsi		__PR((int excode, void *arg));
LOCAL	void	excdr		__PR((int excode, void *arg));
LOCAL	int	prstats		__PR((void));
LOCAL	void	dorw		__PR((SCSI *scgp, char *filename, char* sectors));
LOCAL	void	doit		__PR((SCSI *scgp));
LOCAL	void	read_disk	__PR((SCSI *scgp, parm_t *parmp));

LOCAL	void	readc2_disk	__PR((SCSI *scgp, parm_t *parmp));
LOCAL	int	fread_data	__PR((SCSI *scgp, rparm_t *rp, caddr_t bp, long addr, int cnt));
LOCAL	int	bits		__PR((int c));
LOCAL	int	bitidx		__PR((int c));
LOCAL	int	fread_c2	__PR((SCSI *scgp, rparm_t *rp, caddr_t bp, long addr, int cnt));

LOCAL	int	fdata_null	__PR((rparm_t *rp, caddr_t bp, long addr, int cnt));
LOCAL	int	fdata_c2	__PR((rparm_t *rp, caddr_t bp, long addr, int cnt));

#ifdef	used
LOCAL	int read_scsi_g1	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
#endif

EXPORT	int	write_scsi	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_g0	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
EXPORT	int	write_g1	__PR((SCSI *scgp, caddr_t bp, long addr, int cnt));

#ifdef	used
LOCAL	void	Xrequest_sense	__PR((SCSI *scgp));
#endif
LOCAL	int	read_retry	__PR((SCSI *scgp, caddr_t bp, long addr, long cnt,
					int (*rfunc)(SCSI *scgp, rparm_t *rp, caddr_t bp, long addr, int cnt),
					rparm_t *rp
					));
LOCAL	void	read_generic	__PR((SCSI *scgp, parm_t *parmp,
					int (*rfunc)(SCSI *scgp, rparm_t *rp, caddr_t bp, long addr, int cnt),
					rparm_t *rp,
					int (*dfunc)(rparm_t *rp, caddr_t bp, long addr, int cnt)
));
LOCAL	void	write_disk	__PR((SCSI *scgp, parm_t *parmp));
LOCAL	int	choice		__PR((int n));
LOCAL	void	ra		__PR((SCSI *scgp));

EXPORT	int	read_da		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt, int framesize, int subcode));
EXPORT	int	read_cd		__PR((SCSI *scgp, caddr_t bp, long addr, int cnt, int framesize, int data, int subch));

LOCAL	void	oldmode		__PR((SCSI *scgp, int *errp, int *retrp));
LOCAL	void	domode		__PR((SCSI *scgp, int err, int retr));

LOCAL	void	qpto96		__PR((Uchar *sub, Uchar *subq, int dop));
LOCAL	void	ovtime		__PR((SCSI *scgp));

struct timeval	starttime;
struct timeval	stoptime;
int	didintr;
int	exsig;

char	*Sbuf;
long	Sbufsize;

/*#define	MAX_RETRY	32*/
#define	MAX_RETRY	128

int	help;
int	xdebug;
int	lverbose;
int	quiet;
BOOL	is_suid;
BOOL	is_cdrom;
BOOL	do_write;
BOOL	c2scan;
BOOL	fulltoc;
BOOL	clone;
BOOL	noerror;
BOOL	nocorr;
BOOL	notrunc;
int	retries = MAX_RETRY;

struct	scsi_format_data fmt;

/*XXX*/EXPORT	BOOL cvt_cyls(){ return (FALSE);}
/*XXX*/EXPORT	BOOL cvt_bcyls(){ return (FALSE);}
/*XXX*/EXPORT	void print_defect_list(){}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\treadcd [options]\n");
	error("options:\n");
	error("\t-version	print version information and exit\n");
	error("\tdev=target	SCSI target to use\n");
	error("\tf=filename	Name of file to read/write\n");
	error("\tsectors=range	Range of sectors to read/write\n");
	error("\tspeed=#		set speed of drive (MMC only)\n");
	error("\t-w		Switch to write mode\n");
	error("\t-c2scan		Do a C2 error scan\n");
	error("\ttimeout=#	set the default SCSI command timeout to #.\n");
	error("\tdebug=#,-d	Set to # or increment misc debug level\n");
	error("\tkdebug=#,kd=#	do Kernel debugging\n");
	error("\t-quiet,-q	be more quiet in error retry mode\n");
	error("\t-verbose,-v	increment general verbose level by one\n");
	error("\t-Verbose,-V	increment SCSI command transport verbose level by one\n");
	error("\t-silent,-s	do not print status of failed SCSI commands\n");
	error("\t-noerror	do not abort on error\n");
	error("\t-notrunc	do not truncate outputfile in read mode\n");
	error("\tretries=#	set retry count (default is %d)\n", retries);
	error("\t-overhead	meter SCSI command overhead times\n");
	error("\n");
	error("sectors=0-0 will read nothing, sectors=0-1 will read one sector starting from 0\n");
	exit(ret);
}	

char	opts[]   = "debug#,d+,kdebug#,kd#,timeout#,quiet,q,verbose+,v+,Verbose+,V+,x+,xd#,silent,s,help,h,version,dev*,sectors*,w,c2scan,fulltoc,clone,noerror,nocorr,notrunc,retries#,f*,speed#,overhead";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
	int	fcount;
	int	cac;
	char	* const *cav;
	int	scsibus	= 0;
	int	target	= 0;
	int	lun	= 0;
	int	silent	= 0;
	int	verbose	= 0;
	int	kdebug	= 0;
	int	debug	= 0;
	int	deftimeout= 40;
	int	pversion = 0;
	int	speed	= -1;
	int	dooverhead=0;
	SCSI	*scgp;
	char	*filename= NULL;
	char	*sectors = NULL;

	save_args(ac, av);

	cac = --ac;
	cav = ++av;

	if(getallargs(&cac, &cav, opts,
			&debug, &debug,
			&kdebug, &kdebug,
			&deftimeout,
			&quiet, &quiet,
			&lverbose, &lverbose,
			&verbose, &verbose,
			&xdebug, &xdebug,
			&silent, &silent,
			&help, &help, &pversion,
			&dev, &sectors, &do_write,
			&c2scan,
			&fulltoc, &clone,
			&noerror, &nocorr,
			&notrunc, &retries, &filename,
			&speed, &dooverhead) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf("readcd %s (%s-%s-%s) Copyright (C) 1987, 1995-2002 J�rg Schilling\n",
								cdr_version,
								HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	fcount = 0;
	cac = ac;
	cav = av;

	while(getfiles(&cac, &cav, opts) > 0) {
		fcount++;
		if (fcount == 1) {
			if (*astoi(cav[0], &target) != '\0') {
				errmsgno(EX_BAD,
					"Target '%s' is not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		}
		if (fcount == 2) {
			if (*astoi(cav[0], &lun) != '\0') {
				errmsgno(EX_BAD,
					"Lun is '%s' not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		}
		if (fcount == 3) {
			if (*astoi(cav[0], &scsibus) != '\0') {
				errmsgno(EX_BAD,
					"Scsibus is '%s' not a Number.\n",
								cav[0]);
				usage(EX_BAD);
				/* NOTREACHED */
			}
		} else {
			scsibus = 0;
		}
		cac--;
		cav++;
	}
/*error("dev: '%s'\n", dev);*/

	cdr_defaults(&dev, NULL, NULL, NULL);
	if (debug) {
		printf("dev: '%s'\n", dev);
	}
	if (dev) {
		char	errstr[80];

		/*
		 * Call scg_remote() to force loading the remote SCSI transport
		 * library code that is located in librscg instead of the dummy
		 * remote routines that are located inside libscg.
		 */
		scg_remote();
		if ((strncmp(dev, "HELP", 4) == 0) ||
		    (strncmp(dev, "help", 4) == 0)) {
			scg_help(stderr);
			exit(0);
		}
		if ((scgp = scg_open(dev, errstr, sizeof(errstr), debug, lverbose)) == (SCSI *)0) {
			int	err = geterrno();

			errmsgno(err, "%s%sCannot open SCSI driver.\n", errstr, errstr[0]?". ":"");
			errmsgno(EX_BAD, "For possible transport specifiers try 'readcd dev=help'.\n");
			exit(err);
		}
	} else {
		if (scsibus == -1 && target >= 0 && lun >= 0)
			scsibus = 0;

		scgp = scg_smalloc();
		scgp->debug = debug;
		scgp->kdebug = kdebug;

		scg_settarget(scgp, scsibus, target, lun);
		if (scg__open(scgp, NULL) <= 0)
			comerr("Cannot open SCSI driver.\n");
	}
	scgp->silent = silent;
	scgp->verbose = verbose;
	scgp->debug = debug;
	scgp->kdebug = kdebug;
	scg_settimeout(scgp, deftimeout);

	Sbufsize = scg_bufsize(scgp, 256*1024L);
	if ((Sbuf = scg_getbuf(scgp, Sbufsize)) == NULL)
		comerr("Cannot get SCSI I/O buffer.\n");

	is_suid = geteuid() != getuid();
	/*
	 * We don't need root privilleges anymore.
	 */
#ifdef	HAVE_SETREUID
	if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
	if (seteuid(getuid()) < 0)
#else
	if (setuid(getuid()) < 0)
#endif
#endif
		comerr("Panic cannot set back effective uid.\n");

	/* code to use SCG */

	do_inquiry(scgp, FALSE);
	allow_atapi(scgp, TRUE);    /* Try to switch to 10 byte mode cmds */
	if (is_mmc(scgp, NULL, NULL)) {
		int	rspeed;
		int	wspeed;
		/*
		 * At this point we know that we have a SCSI-3/mmc compliant drive.
		 * Unfortunately ATAPI drives violate the SCSI spec in returning
		 * a response data format of '1' which from the SCSI spec would
		 * tell us not to use the "PF" bit in mode select. As ATAPI drives
		 * require the "PF" bit to be set, we 'correct' the inquiry data.
		 */
		if (scgp->inq->data_format < 2)
			scgp->inq->data_format = 2;

		if (speed > 0)
			speed *= 177;
		if (speed > 0xFFFF || speed < 0)
			speed = 0xFFFF;
		scsi_set_speed(scgp, speed, speed, ROTCTL_CLV);
		if (scsi_get_speed(scgp, &rspeed, &wspeed) >= 0) {
			error("Read  speed: %5d kB/s (CD %3dx, DVD %2dx).\n",
				rspeed, rspeed/176, rspeed/1385);
			error("Write speed: %5d kB/s (CD %3dx, DVD %2dx).\n",
				wspeed, wspeed/176, wspeed/1385);
		}
	}
	exargs.scgp	   = scgp;
	exargs.old_secsize = -1;
/*	exargs.flags	   = flags;*/
	exargs.oerr[2]     = 0;

	/*
	 * Install exit handler before we change the drive status.
	 */
	on_comerr(exscsi, &exargs);
	signal(SIGINT, intr);
	signal(SIGTERM, intr);

	if (dooverhead) {
		ovtime(scgp);
		comexit(0);
	}

	if (is_suid) {
		if (scgp->inq->type != INQ_ROMD)
			comerrno(EX_BAD, "Not root. Will only work on CD-ROM in suid mode\n");
	}

	if (filename || sectors || c2scan || fulltoc || clone) {
		dorw(scgp, filename, sectors);
	} else {
		doit(scgp);
	}
	comexit(0);
	return (0);
}

/*
 * XXX Leider kann man vim Signalhandler keine SCSI Kommandos verschicken
 * XXX da meistens das letzte SCSI Kommando noch laeuft.
 * XXX Eine Loesung waere ein Abort Callback in SCSI *.
 */
LOCAL void
intr(sig)
	int	sig;
{
	didintr++;
	exsig = sig;
/*	comexit(sig);*/
}

/* ARGSUSED */
LOCAL void
exscsi(excode, arg)
	int	excode;
	void	*arg;
{
	struct exargs	*exp = (struct exargs *)arg;
		int	i;

	/*
	 * Try to restore the old sector size.
	 */
	if (exp != NULL && exp->exflags == 0) {
		for (i=0; i < 10*100; i++) {
			if (!exp->scgp->running)
				break;
			if (i == 10) {
				errmsgno(EX_BAD,
					"Waiting for current SCSI command to finish.\n");
			}
			usleep(100000);
		}

		if (!exp->scgp->running) {
			if (exp->oerr[2] != 0) {
				domode(exp->scgp, exp->oerr[0], exp->oerr[1]);
			}
			if (exp->old_secsize > 0 && exp->old_secsize != 2048)
				select_secsize(exp->scgp, exp->old_secsize);
		}
		exp->exflags++;	/* Make sure that it only get called once */
	}
}

LOCAL void
excdr(excode, arg)
	int	excode;
	void	*arg;
{
	exscsi(excode, arg);

#ifdef	needed
	/* Do several other restores/statistics here (see cdrecord.c) */
#endif
}

/*
 * Return milliseconds since start time.
 */
LOCAL int
prstats()
{
	int	sec;
	int	usec;
	int	tmsec;

	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		comerr("Cannot get time\n");

	sec = stoptime.tv_sec - starttime.tv_sec;
	usec = stoptime.tv_usec - starttime.tv_usec;
	tmsec = sec*1000 + usec/1000;
#ifdef	lint
	tmsec = tmsec;	/* Bisz spaeter */
#endif
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}

	error("Time total: %d.%03dsec\n", sec, usec/1000);
	return (1000*sec + (usec / 1000));
}

LOCAL void
dorw(scgp, filename, sectors)
	SCSI	*scgp;
	char	*filename;
	char	*sectors;
{
	parm_t	params;
	char	*p = NULL;

	params.start = 0;
	params.end = -1;
	params.sptr = -1;
	params.askrange = FALSE;
	params.name = NULL;

	if (filename)
		params.name = filename;
	if (sectors)
		p = astol(sectors, &params.start);
	if (p && *p == '-')
		p = astol(++p, &params.end);
	if (p && *p != '\0')
		comerrno(EX_BAD, "Not a valid sector range '%s'\n", sectors);

	if (!wait_unit_ready(scgp, 60))
		comerrno(EX_BAD, "Device not ready.\n");

	if (c2scan) {
		noerror = TRUE;
		if (retries == MAX_RETRY)
			retries = 10;
		if (params.name == NULL)
			params.name = "/dev/null";
		readc2_disk(scgp, &params);
	} else if (do_write)
		write_disk(scgp, &params);
	else
		read_disk(scgp, &params);
}

LOCAL void
doit(scgp)
	SCSI	*scgp;
{
	int	i = 0;
	parm_t	params;

	params.start = 0;
	params.end = -1;
	params.sptr = -1;
	params.askrange = TRUE;
	params.name = "/dev/null";

	for(;;) {
		if (!wait_unit_ready(scgp, 60))
			comerrno(EX_BAD, "Device not ready.\n");

		printf("0:read 1:veri   2:erase   3:read buffer 4:cache 5:ovtime 6:cap\n");
		printf("7:wne  8:floppy 9:verify 10:checkcmds  11:read disk 12:write disk\n");
		printf("13:scsireset 14:seektest 15: readda 16: reada 17: c2err\n");

		getint("Enter selection:", &i, 0, 20);
		if (didintr)
			return;

		switch (i) {

		case 5:		ovtime(scgp);		break;
		case 11:	read_disk(scgp, 0);	break;
		case 12:	write_disk(scgp, 0);	break;
		case 15:	ra(scgp);		break;
/*		case 16:	reada_disk(scgp, 0, 0);	break;*/
		case 17:	readc2_disk(scgp, &params);	break;
		}
	}
}

LOCAL void
read_disk(scgp, parmp)
	SCSI	*scgp;
	parm_t	*parmp;
{
	rparm_t	rp;

	read_capacity(scgp);
	print_capacity(scgp, stderr);

	rp.errors = 0;
	rp.c2_errors = 0;
	rp.c2_maxerrs = 0;
	rp.c2_errsecs = 0;
	rp.c2_badsecs = 0;
	rp.secsize = scgp->cap->c_bsize;

	read_generic(scgp, parmp, fread_data, &rp, fdata_null);
}


char	zeroblk[512];

LOCAL void
readc2_disk(scgp, parmp)
	SCSI	*scgp;
	parm_t	*parmp;
{
	rparm_t	rp;
	int	osecsize = 2048;
	int	oerr = 0;
	int	oretr = 10;

	scgp->silent++;
	if (read_capacity(scgp) >= 0)
		 osecsize = scgp->cap->c_bsize;
	scgp->silent--;
	if (osecsize != 2048)
		select_secsize(scgp, 2048);

	read_capacity(scgp);
	print_capacity(scgp, stderr);

	rp.errors = 0;
	rp.c2_errors = 0;
	rp.c2_maxerrs = 0;
	rp.c2_errsecs = 0;
	rp.c2_badsecs = 0;
	rp.secsize = 2352 + 294;
	rp.ismmc = is_mmc(scgp, NULL, NULL);

	oldmode(scgp, &oerr, &oretr);
	exargs.oerr[0] = oerr;
	exargs.oerr[1] = oretr;
	exargs.oerr[2] = 0xFF;
	domode(scgp, 0x21, 10);


	read_generic(scgp, parmp, fread_c2, &rp, fdata_c2);
	if (osecsize != 2048)
		select_secsize(scgp, osecsize);
	domode(scgp, oerr, oretr);

	printf("Total of %d hard read errors.\n", rp.errors);
	printf("C2 errors total: %d bytes in %d sectors on disk\n", rp.c2_errors, rp.c2_errsecs);
	printf("C2 errors rate: %f%% \n", (100.0*rp.c2_errors)/scgp->cap->c_baddr/2352);
	printf("C2 errors on worst sector: %d, sectors with 100+ C2 errors: %d\n", rp.c2_maxerrs, rp.c2_badsecs);
}

/* ARGSUSED */
LOCAL int
fread_data(scgp, rp, bp, addr, cnt)
	SCSI	*scgp;
	rparm_t	*rp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	return (read_g1(scgp, bp, addr, cnt));
}


LOCAL int
bits(c)
	int	c;
{
	int	n = 0;

	if (c & 0x01)
		n++;
	if (c & 0x02)
		n++;
	if (c & 0x04)
		n++;
	if (c & 0x08)
		n++;
	if (c & 0x10)
		n++;
	if (c & 0x20)
		n++;
	if (c & 0x40)
		n++;
	if (c & 0x80)
		n++;
	return (n);
}

LOCAL int 
bitidx(c)
	int	c;
{
	if (c & 0x80)
		return (0);
	if (c & 0x40)
		return (1);
	if (c & 0x20)
		return (2);
	if (c & 0x10)
		return (3);
	if (c & 0x08)
		return (4);
	if (c & 0x04)
		return (5);
	if (c & 0x02)
		return (6);
	if (c & 0x01)
		return (7);
	return (-1);
}

LOCAL int
fread_c2(scgp, rp, bp, addr, cnt)
	SCSI	*scgp;
	rparm_t	*rp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if (rp->ismmc) {
		return (read_cd(scgp, bp, addr, cnt, rp->secsize,
			/* Sync + all headers + user data + EDC/ECC + C2 */
/*			(1 << 7 | 3 << 5 | 1 << 4 | 1 << 3 | 2 << 1),*/
			(1 << 7 | 3 << 5 | 1 << 4 | 1 << 3 | 1 << 1),
        		/* without subchannels */
			0));
	} else {
		return (read_da(scgp, bp, addr, cnt, rp->secsize,
			/* Sync + all headers + user data + EDC/ECC + C2 */
			0x04));
	}
}

/* ARGSUSED */
LOCAL int
fdata_null(rp, bp, addr, cnt)
	rparm_t	*rp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	return (0);
}

LOCAL int
fdata_c2(rp, bp, addr, cnt)
	rparm_t	*rp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	int	i;
	int	j;
	int	k;
	char	*p;

	p = &bp[2352];

	for (i=0; i < cnt; i++, p += (2352+294)) {
/*		scg_prbytes("XXX ", p, 294);*/
		if ((j = cmpbytes(p, zeroblk, 294)) < 294) {
			printf("C2 in sector: %3ld first at byte: %4d (0x%02X)", addr+i,
				j*8 + bitidx(p[j]), p[j]&0xFF);
			for (j=0,k=0; j < 294; j++)
				k += bits(p[j]);
			printf(" total: %4d errors\n", k);
/*			scg_prbytes("XXX ", p, 294);*/
			rp->c2_errors += k;
			if (k > rp->c2_maxerrs)
				rp->c2_maxerrs = k;
			rp->c2_errsecs++;
			if (k >= 100)
				rp->c2_badsecs += 1;
		}
	}	
	return (0);
}

#ifdef	used
LOCAL int
read_scsi_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
/*	scmd->size = cnt*512;*/
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x28;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);
	
	scgp->cmdname = "read extended";

	return (scg_cmd(scgp));
}
#endif

#define	G0_MAXADDR	0x1FFFFFL

EXPORT int
write_scsi(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if(addr <= G0_MAXADDR)
		return(write_g0(scgp, bp, addr, cnt));
	else
		return(write_g1(scgp, bp, addr, cnt));
}

EXPORT int
write_g0(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_WRITE;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);
	scmd->cdb.g0_cdb.count = (Uchar)cnt;
	
	scgp->cmdname = "write_g0";

	return (scg_cmd(scgp));
}

EXPORT int
write_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = SC_EWRITE;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);
	
	scgp->cmdname = "write_g1";

	return (scg_cmd(scgp));
}

#ifdef	used
LOCAL void
Xrequest_sense(scgp)
	SCSI	*scgp;
{
	char	sense_buf[32];
	struct	scg_cmd ocmd;
	int	sense_count;
	char	*cmdsave;
	register struct	scg_cmd	*scmd = scgp->scmd;

	cmdsave = scgp->cmdname;

	movebytes(scmd, &ocmd, sizeof(*scmd));

	fillbytes((caddr_t)sense_buf, sizeof(sense_buf), '\0');

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = (caddr_t)sense_buf;
	scmd->size = sizeof(sense_buf);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x3;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = sizeof(sense_buf);
	
	scgp->cmdname = "request sense";

	scg_cmd(scgp);

	sense_count = sizeof(sense_buf) - scg_getresid(scgp);
	movebytes(&ocmd, scmd, sizeof(*scmd));
	scmd->sense_count = sense_count;
	movebytes(sense_buf, (Uchar *)&scmd->sense, scmd->sense_count);

	scgp->cmdname = cmdsave;
	scg_printerr(scgp);
	scg_printresult(scgp);	/* XXX restore key/code in future */
}
#endif

LOCAL int
read_retry(scgp, bp, addr, cnt, rfunc, rp)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	long	cnt;
	int	(*rfunc)__PR((SCSI *scgp, rparm_t *rp, caddr_t bp, long addr, int cnt));
	rparm_t	*rp;
{
/*	int	secsize = scgp->cap->c_bsize;*/
	int	secsize = rp->secsize;
	int	try = 0;
	int	err;
	char	dummybuf[8192];

	if (secsize > sizeof(dummybuf)) {
		errmsgno(EX_BAD, "Cannot retry, sector size %d too big.\n", secsize);
		return (-1);
	}

	errmsgno(EX_BAD, "Retrying from sector %ld.\n", addr);
	while (cnt > 0) {
		error(".");

		do {
			if (didintr)
				comexit(exsig);		/* XXX besseres Konzept?!*/
			wait_unit_ready(scgp, 120);
			if (try >= 10) {		/* First 10 retries without seek */
				if ((try % 8) == 0) {
					error("+");	/* Read last sector */
					scgp->silent++;
					(*rfunc)(scgp, rp, dummybuf, scgp->cap->c_baddr, 1);
					scgp->silent--;
				} else if ((try % 4) == 0) {
					error("-");	/* Read first sector */
					scgp->silent++;
					(*rfunc)(scgp, rp, dummybuf, 0, 1);
					scgp->silent--;
				} else {
					error("~");	/* Read random sector */
					scgp->silent++;
					(*rfunc)(scgp, rp, dummybuf, choice(scgp->cap->c_baddr), 1);
					scgp->silent--;
				}
				if (didintr)
					comexit(exsig);		/* XXX besseres Konzept?!*/
				wait_unit_ready(scgp, 120);
			}
			if (didintr)
				comexit(exsig);		/* XXX besseres Konzept?!*/

			fillbytes(bp, secsize, 0);

			scgp->silent++;
			err = (*rfunc)(scgp, rp, bp, addr, 1);
			scgp->silent--;

			if (err < 0) {
				err = scgp->scmd->ux_errno;
/*				error("\n");*/
/*				errmsgno(err, "Cannot read source disk\n");*/
			} else {
				break;
			}
		} while (++try < retries);

		if (try >= retries) {
			error("\n");
			errmsgno(err, "Error on sector %ld not corrected. Total of %d errors.\n",
					addr, ++rp->errors);

			if (scgp->silent <= 1 && lverbose > 0)
				scg_printerr(scgp); 

			if (!noerror)
				return (-1);
			errmsgno(EX_BAD, "-noerror set, continuing ...\n");
		} else {
			if (try > 1) {
				error("\n");
				errmsgno(EX_BAD,
				"Error on sector %ld corrected after %d tries. Total of %d errors.\n",
					addr, try, rp->errors);
			}
		}
		try = 0;
		cnt -= 1;
		addr += 1;
		bp += secsize;
	}
	return (0);
}

LOCAL void
read_generic(scgp, parmp, rfunc, rp, dfunc)
	SCSI	*scgp;
	parm_t	*parmp;
	int	(*rfunc)__PR((SCSI *scgp, rparm_t *rp, caddr_t bp, long addr, int cnt));
	rparm_t	*rp;
	int	(*dfunc)__PR((rparm_t *rp, caddr_t bp, long addr, int cnt));
{
	char	filename[512];
	char	*defname = NULL;
	FILE	*f;
	long	addr = 0L;
	long	num;
	long	end = 0L;
	long	start = 0L;
	long	cnt = 0L;
	int	msec;
	int	err = 0;
	BOOL	askrange = FALSE;
	BOOL	isrange = FALSE;
	int	secsize = rp->secsize;

	if (is_suid) {
		if (scgp->inq->type != INQ_ROMD)
			comerrno(EX_BAD, "Not root. Will only read from CD in suid mode\n");
	}

	if (parmp == NULL || parmp->askrange)
		askrange = TRUE;
	if (parmp != NULL && !askrange && (parmp->start <= parmp->end))
		isrange = TRUE;

	filename[0] ='\0';
	
	scgp->silent++;
	if (read_capacity(scgp) >= 0)
		end = scgp->cap->c_baddr + 1;
	scgp->silent--;

	if (end <= 0 || isrange || (askrange && scg_yes("Ignore disk size? ")))
		end = 10000000;	/* Hack to read empty (e.g. blank=fast) disks */

	if (parmp) {
		if (parmp->name)
			defname = parmp->name;
		if (defname != NULL) {
			error("Copy from SCSI (%d,%d,%d) disk to file '%s'\n",
					scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp),
					defname);
		}

		addr = start = parmp->start;
		if (parmp->end != -1 && parmp->end < end)
			end = parmp->end;
		cnt = Sbufsize / secsize;
	}

	if (defname == NULL) {
		defname = "disk.out";
		error("Copy from SCSI (%d,%d,%d) disk to file\n",
					scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
		error("Enter filename [%s]: ", defname);flush();
		(void)getline(filename, sizeof(filename));
	}

	if (askrange) {
		addr = start;
		getlong("Enter starting sector for copy:", &addr, start, end-1);
/*		getlong("Enter starting sector for copy:", &addr, -300, end-1);*/
		start = addr;
	}

	if (askrange) {
		num = end - addr;
		getlong("Enter number of sectors to copy:", &num, 1L, num);
		end = addr + num;
	}

	if (askrange) {
/* XXX askcnt */
		cnt = Sbufsize / secsize;
		getlong("Enter number of sectors per copy:", &cnt, 1L, cnt);
	}

	if (filename[0] == '\0')
		strncpy(filename, defname, sizeof(filename));
	filename[sizeof(filename)-1] = '\0';
	if (streql(filename, "-")) {
		f = stdout;
#if	defined(__CYGWIN32__) || defined(__EMX__)
		setmode(STDOUT_FILENO, O_BINARY);
#endif
	} else if ((f = fileopen(filename, notrunc?"wcub":"wctub")) == NULL)
		comerr("Cannot open '%s'.\n", filename);

	error("end:  %8ld\n", end);
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");

	for(;addr < end; addr += cnt) {
		if (didintr)
			comexit(exsig);		/* XXX besseres Konzept?!*/

		if ((addr + cnt) > end)
			cnt = end - addr;

		error("addr: %8ld cnt: %ld\r", addr, cnt);

		scgp->silent++;
		if ((*rfunc)(scgp, rp, Sbuf, addr, cnt) < 0) {
			scgp->silent--;
			err = scgp->scmd->ux_errno;
			if (quiet) {
				error("\n");
			} else if (scgp->silent == 0) {
				scg_printerr(scgp);
			}
			errmsgno(err, "Cannot read source disk\n");

			if (read_retry(scgp, Sbuf, addr, cnt, rfunc, rp) < 0)
				goto out;
		} else {
			scgp->silent--;
		}
		(*dfunc)(rp, Sbuf, addr, cnt);
		if (filewrite(f, Sbuf, cnt * secsize) < 0) {
			err = geterrno();
			error("\n");
			errmsgno(err, "Cannot write '%s'\n", filename);
			break;
		}
	}
	error("addr: %8ld", addr);
out:
	error("\n");
	msec = prstats();
	if (msec == 0)		/* Avoid division by zero */
		msec = 1;
#ifdef	OOO
	error("Read %.2f kB at %.1f kB/sec.\n",
		(double)(addr - start)/(1024.0/scgp->cap->c_bsize),
		(double)((addr - start)/(1024.0/scgp->cap->c_bsize)) / (0.001*msec));
#else
	error("Read %.2f kB at %.1f kB/sec.\n",
		(double)(addr - start)/(1024.0/secsize),
		(double)((addr - start)/(1024.0/secsize)) / (0.001*msec));
#endif
}

LOCAL void
write_disk(scgp, parmp)
	SCSI	*scgp;
	parm_t	*parmp;
{
	char	filename[512];
	char	*defname = "disk.out";
	FILE	*f;
	long	addr = 0L;
	long	cnt;
	long	amt;
	long	end;
	int	msec;
	int	start;

	if (is_suid)
		comerrno(EX_BAD, "Not root. Will not write in suid mode\n");

	filename[0] ='\0';
	if (read_capacity(scgp) >= 0) {
		end = scgp->cap->c_baddr + 1;
		print_capacity(scgp, stderr);
	}

	if (end <= 0)
		end = 10000000;	/* Hack to write empty disks */

	if (parmp) {
		if (parmp->name)
			defname = parmp->name;
		error("Copy from file '%s' to SCSI (%d,%d,%d) disk\n",
					defname,
					scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));

		addr = start = parmp->start;
		if (parmp->end != -1 && parmp->end < end)
			end = parmp->end;
		cnt = Sbufsize / scgp->cap->c_bsize;
	}
else {

	error("Copy from file to SCSI (%d,%d,%d) disk\n",
					scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	error("Enter filename [%s]: ", defname);flush();
	(void)getline(filename, sizeof(filename));
	error("Notice: reading from file always starts at file offset 0.\n");

	getlong("Enter starting sector for copy:", &addr, 0L, end-1);
	start = addr;
	cnt = end - addr;
	getlong("Enter number of sectors to copy:", &end, 1L, end);
	end = addr + cnt;

	cnt = Sbufsize / scgp->cap->c_bsize;
	getlong("Enter number of sectors per copy:", &cnt, 1L, cnt);
/*	error("end:  %8ld\n", end);*/
}

	if (filename[0] == '\0')
		strncpy(filename, defname, sizeof(filename));
	filename[sizeof(filename)-1] = '\0';
	if (streql(filename, "-")) {
		f = stdin;
#if	defined(__CYGWIN32__) || defined(__EMX__)
		setmode(STDIN_FILENO, O_BINARY);
#endif
	} else if ((f = fileopen(filename, "rub")) == NULL)
		comerr("Cannot open '%s'.\n", filename);

	error("end:  %8ld\n", end);
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");

	for(;addr < end; addr += cnt) {
		if (didintr)
			comexit(exsig);		/* XXX besseres Konzept?!*/

		if ((addr + cnt) > end)
			cnt = end - addr;

		error("addr: %8ld cnt: %ld\r", addr, cnt);

		if ((amt = fileread(f, Sbuf, cnt * scgp->cap->c_bsize)) < 0)
			comerr("Cannot read '%s'\n", filename);
		if (amt == 0)
			break;
		if ((amt / scgp->cap->c_bsize) < cnt)
			cnt = amt / scgp->cap->c_bsize;
		if (write_scsi(scgp, Sbuf, addr, cnt) < 0)
			comerrno(scgp->scmd->ux_errno,
					"Cannot write destination disk\n");
	}
	error("addr: %8ld\n", addr);
	msec = prstats();
	if (msec == 0)		/* Avoid division by zero */
		msec = 1;
	error("Wrote %.2f kB at %.1f kB/sec.\n",
		(double)(addr - start)/(1024.0/scgp->cap->c_bsize),
		(double)((addr - start)/(1024.0/scgp->cap->c_bsize)) / (0.001*msec));
}

LOCAL int
choice(n)
	int	n;
{
#if	defined(HAVE_DRAND48)
	extern	double	drand48 __PR((void));

	return drand48() * n;
#else
#	if	defined(HAVE_RAND)
	extern	int	rand __PR((void));

	return rand() % n;
#	else
	return (0);
#	endif
#endif
}

LOCAL void
ra(scgp)
	SCSI	*scgp;
{
/*	char	filename[512];*/
	FILE	*f;
/*	long	addr = 0L;*/
/*	long	cnt;*/
/*	long	end;*/
/*	int	msec;*/
/*	int	start;*/
/*	int	err = 0;*/

	select_secsize(scgp, 2352);
	read_capacity(scgp);
	print_capacity(scgp, stderr);
	fillbytes(Sbuf, 50*2352, 0);
	if (read_g1(scgp, Sbuf, 0, 50) < 0)
		errmsg("read CD\n");
	f = fileopen("DDA", "wctb");
/*	filewrite(f, Sbuf, 50 * 2352 - scg_getresid(scgp));*/
	filewrite(f, Sbuf, 50 * 2352 );
	fclose(f);
}

#define	g5x_cdblen(cdb, len)	((cdb)->count[0] = ((len) >> 16L)& 0xFF,\
				 (cdb)->count[1] = ((len) >> 8L) & 0xFF,\
				 (cdb)->count[2] = (len) & 0xFF)

EXPORT int
read_da(scgp, bp, addr, cnt, framesize, subcode)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
	int	framesize;
	int	subcode;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*framesize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xd8;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	g5_cdbaddr(&scmd->cdb.g5_cdb, addr);
	g5_cdblen(&scmd->cdb.g5_cdb, cnt);
	scmd->cdb.g5_cdb.res10 = subcode;
	
	scgp->cmdname = "read_da";

	return (scg_cmd(scgp));
}

EXPORT int
read_cd(scgp, bp, addr, cnt, framesize, data, subch)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
	int	framesize;
	int	data;
	int	subch;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof(*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*framesize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xBE;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
        scmd->cdb.g5_cdb.res = 0;	/* expected sector type field ALL */
	g5_cdbaddr(&scmd->cdb.g5_cdb, addr);
	g5x_cdblen(&scmd->cdb.g5_cdb, cnt);

	scmd->cdb.g5_cdb.count[3] = data & 0xFF;
        scmd->cdb.g5_cdb.res10 = subch & 0x07;

	scgp->cmdname = "read_cd";

	return (scg_cmd(scgp));
}

LOCAL void
oldmode(scgp, errp, retrp)
	SCSI	*scgp;
	int	*errp;
	int	*retrp;
{
	Uchar	mode[0x100];
	Uchar	cmode[0x100];
	Uchar	*p;
	int	i;
	int	len;

	fillbytes(mode, sizeof(mode), '\0');
	fillbytes(cmode, sizeof(cmode), '\0');

	if (!get_mode_params(scgp, 0x01, "CD error recovery parameter",
			mode, (Uchar *)0, (Uchar *)cmode, (Uchar *)0, &len)) {
		return;
	}
	if (xdebug)
		scg_prbytes("Mode Sense Data", mode, len);

	mode[0] = 0;
	mode[2] = 0; /* ??? ist manchmal 0x80 */
	p = mode;
	p += mode[3] + 4;
	*p &= 0x3F;

	if (xdebug)
		scg_prbytes("Mode page 1:", p, 0x10);

	i = p[2];
	if (errp != NULL)
		*errp = i;

	i = p[3];
	if (retrp != NULL)
		*retrp = i;
}

LOCAL void
domode(scgp, err, retr)
	SCSI	*scgp;
	int	err;
	int	retr;
{
	Uchar	mode[0x100];
	Uchar	cmode[0x100];
	Uchar	*p;
	int	i;
	int	len;

	fillbytes(mode, sizeof(mode), '\0');
	fillbytes(cmode, sizeof(cmode), '\0');

	if (!get_mode_params(scgp, 0x01, "CD error recovery parameter",
			mode, (Uchar *)0, (Uchar *)cmode, (Uchar *)0, &len)) {
		return;
	}
	if (xdebug || (err == -1 && retr == -1)) {
		scg_prbytes("Mode Sense Data", mode, len);
	}

	mode[0] = 0;
	mode[2] = 0; /* ??? ist manchmal 0x80 */
	p = mode;
	p += mode[3] + 4;
	*p &= 0x3F;

	if (xdebug || (err == -1 && retr == -1))
		scg_prbytes("Mode page 1:", p, 0x10);

	i = p[2];
	if (err == -1) {
		getint("Error handling? ", &i, 0, 255);
		p[2] = i;
	} else {
		if (xdebug)
			error("Error handling set from %02X to %02X\n",
		p[2], err);
		p[2] = err;
	}

	i = p[3];
	if (retr == -1) {
		getint("Retry count? ", &i, 0, 255);
		p[3] = i;
	} else {
		if (xdebug)
			error("Retry count set from %d to %d\n",
		p[3] & 0xFF, retr);
		p[3] = retr;
	}

	if (xdebug || (err == -1 && retr == -1))
		scg_prbytes("Mode Select Data", mode, len);
	mode_select(scgp, mode, len, 0, scgp->inq->data_format >= 2);
}


/*--------------------------------------------------------------------------*/
LOCAL	void	qpto96		__PR((Uchar *sub, Uchar *subq, int dop));
/*EXPORT	void	qpto96		__PR((Uchar *sub, Uchar *subq, int dop));*/
/*
 * Q-Sub auf 96 Bytes bl�hen und P-Sub addieren
 *
 * OUT: sub, IN: subqptr 
 */
LOCAL void
/*EXPORT void*/
qpto96(sub, subqptr, dop)
	Uchar	*sub;
	Uchar	*subqptr;
	int	dop;
{
	Uchar	tmp[16];
	Uchar	*p;
	int	c;
	int	i;

	if (subqptr == sub) {
		movebytes(subqptr, tmp, 12);
		subqptr = tmp;
	}
	fillbytes(sub, 96, '\0');

	if (dop) for (i=0, p = sub; i < 96; i++) {
		*p++ |= 0x80;
	}
	for (i=0, p = sub; i < 12; i++) {
		c = subqptr[i] & 0xFF;
/*printf("%02X\n", c);*/
		if (c & 0x80)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x40)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x20)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x10)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x08)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x04)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x02)
			*p++ |= 0x40;
		else
			p++;
		if (c & 0x01)
			*p++ |= 0x40;
		else
			p++;
	}
}

/*--------------------------------------------------------------------------*/

LOCAL void
ovtime(scgp)
	SCSI	*scgp;
{
	register int	i;

	scgp->silent++;
	(void)test_unit_ready(scgp);
	scgp->silent--;
	if (test_unit_ready(scgp) < 0)
		return;

	printf("Doing 1000 'TEST UNIT READY' operations.\n");

	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");

	for (i = 1000; --i >= 0;) {
		(void)test_unit_ready(scgp);

		if (didintr)
			return;
	}

	prstats();

	/*
	 * ATAPI drives do not like seek_g0()
	 */
	scgp->silent++;
	i = seek_g0(scgp, 0L);
	scgp->silent--;

	if (i >= 0) {
		printf("Doing 1000 'SEEK_G0 (0)' operations.\n");

		if (gettimeofday(&starttime, (struct timezone *)0) < 0)
			comerr("Cannot get start time\n");

		for (i = 1000; --i >= 0;) {
			(void)seek_g0(scgp, 0L);

			if (didintr)
				return;
		}

		prstats();
	}

	scgp->silent++;
	i = seek_g1(scgp, 0L);
	scgp->silent--;
	if (i < 0)
		return;

	printf("Doing 1000 'SEEK_G1 (0)' operations.\n");

	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		comerr("Cannot get start time\n");

	for (i = 1000; --i >= 0;) {
		(void)seek_g1(scgp, 0L);

		if (didintr)
			return;
	}

	prstats();
}
