/* @(#)scsi-sun.c	1.76 02/10/19 Copyright 1988,1995,2000 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-sun.c	1.76 02/10/19 Copyright 1988,1995,2000 J. Schilling";
#endif
/*
 *	SCSI user level command transport routines for
 *	the SCSI general driver 'scg'.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1988,1995,2000 J. Schilling
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
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <scg/scgio.h>

#include <libport.h>		/* Needed for gethostid() */
#ifdef	HAVE_SUN_DKIO_H
#	include <sun/dkio.h>

#	define	dk_cinfo	dk_conf
#	define	dki_slave	dkc_slave
#	define	DKIO_GETCINFO	DKIOCGCONF
#endif
#ifdef	HAVE_SYS_DKIO_H
#	include <sys/dkio.h>

#	define	DKIO_GETCINFO	DKIOCINFO
#endif

#define	TARGET(slave)	((slave) >> 3)
#define	LUN(slave)	((slave) & 07)

/*
 * Tht USCSI ioctl() is not usable on SunOS 4.x
 */
#ifdef	__SVR4
#	define	USE_USCSI
#endif

LOCAL	char	_scg_trans_version[] = "scg-1.76";	/* The version for /dev/scg	*/
LOCAL	char	_scg_utrans_version[] = "uscsi-1.76";	/* The version for USCSI	*/

#ifdef	USE_USCSI
LOCAL	int	scgo_uhelp	__PR((SCSI *scgp, FILE *f));
LOCAL	int	scgo_uopen	__PR((SCSI *scgp, char *device));
LOCAL	int	scgo_uclose	__PR((SCSI *scgp));
LOCAL	int	scgo_ucinfo	__PR((int f, struct dk_cinfo *cp, int debug));
LOCAL	int	scgo_ugettlun	__PR((int f, int *tgtp, int *lunp));
LOCAL	long	scgo_umaxdma	__PR((SCSI *scgp, long amt));
LOCAL	BOOL	scgo_uhavebus	__PR((SCSI *scgp, int));
LOCAL	int	scgo_ufileno	__PR((SCSI *scgp, int, int, int));
LOCAL	int	scgo_uinitiator_id __PR((SCSI *scgp));
LOCAL	int	scgo_uisatapi	__PR((SCSI *scgp));
LOCAL	int	scgo_ureset	__PR((SCSI *scgp, int what));
LOCAL	int	scgo_usend	__PR((SCSI *scgp));

LOCAL scg_ops_t sun_uscsi_ops = {
	scgo_usend,
	scgo_version,		/* Shared with SCG driver */
	scgo_uhelp,
	scgo_uopen,
	scgo_uclose,
	scgo_umaxdma,
	scgo_getbuf,		/* Shared with SCG driver */
	scgo_freebuf,		/* Shared with SCG driver */
	scgo_uhavebus,
	scgo_ufileno,
	scgo_uinitiator_id,
	scgo_uisatapi,
	scgo_ureset,
};
#endif

/*
 * Need to move this into an scg driver ioctl.
 */
/*#define	MAX_DMA_SUN4M	(1024*1024)*/
#define	MAX_DMA_SUN4M	(124*1024)	/* Currently max working size */
/*#define	MAX_DMA_SUN4C	(126*1024)*/
#define	MAX_DMA_SUN4C	(124*1024)	/* Currently max working size */
#define	MAX_DMA_SUN3	(63*1024)
#define	MAX_DMA_SUN386	(56*1024)
#define	MAX_DMA_OTHER	(32*1024)

#define	ARCH_MASK	0xF0
#define	ARCH_SUN2	0x00
#define	ARCH_SUN3	0x10
#define	ARCH_SUN4	0x20
#define	ARCH_SUN386	0x30
#define	ARCH_SUN3X	0x40
#define	ARCH_SUN4C	0x50
#define	ARCH_SUN4E	0x60
#define	ARCH_SUN4M	0x70
#define	ARCH_SUNX	0x80

/*
 * We are using a "real" /dev/scg?
 */
#define	scsi_xsend(scgp)	ioctl((scgp)->fd, SCGIO_CMD, (scgp)->scmd)
#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	union {
		int	SCG_files[MAX_SCG];
#ifdef	USE_USCSI
		short	scg_files[MAX_SCG][MAX_TGT][MAX_LUN];
#endif
	} u;
};
#define scglocal(p)	((struct scg_local *)((p)->local))
#define scgfiles(p)	(scglocal(p)->u.SCG_files)

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
LOCAL char *
scgo_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
#ifdef	USE_USCSI
			if (scgp->ops == &sun_uscsi_ops)
				return (_scg_utrans_version);
#endif
			return (_scg_trans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (__sccsid);
		}
	}
	return ((char *)0);
}

LOCAL int
scgo_help(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "scg", "Generic transport independent SCSI",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
#ifdef	USE_USCSI
	scgo_uhelp(scgp, f);
#endif
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
		 int	busno	= scg_scsibus(scgp);
		 int	tgt	= scg_target(scgp);
/*		 int	tlun	= scg_lun(scgp);*/
	register int	f;
	register int	i;
	register int	nopen = 0;
	char		devname[32];

	if (busno >= MAX_SCG) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno '%d'", busno);
		return (-1);
	}

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2)) {
#ifdef	USE_USCSI
		scgp->ops = &sun_uscsi_ops;
		return (SCGO_OPEN(scgp, device));
#else
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Open by 'devname' not supported on this OS");
		return (-1);
#endif
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof(struct scg_local));
		if (scgp->local == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "No memory for scg_local");
			return (0);
		}

		for (i=0; i < MAX_SCG; i++) {
			scgfiles(scgp)[i] = -1;
		}
	}


	for (i=0; i < MAX_SCG; i++) {
		/*
		 * Skip unneeded devices if not in SCSI Bus scan open mode
		 */
		if (busno >= 0 && busno != i)
			continue;
		js_snprintf(devname, sizeof(devname), "/dev/scg%d", i);
		f = open(devname, O_RDWR);
		if (f < 0) {
			if (errno != ENOENT && errno != ENXIO) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
						"Cannot open '%s'", devname);
				return (-1);
			}
		} else {
			nopen++;
		}
		scgfiles(scgp)[i] = f;
	}
#ifdef	USE_USCSI
	if (nopen <= 0) {
		if (scgp->local != NULL) { 
			free(scgp->local);
			scgp->local = NULL;
		}
		scgp->ops = &sun_uscsi_ops;
		return (SCGO_OPEN(scgp, device));
	}
#endif
	return (nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	register int	i;

	if (scgp->local == NULL)
		return (-1);

	for (i=0; i < MAX_SCG; i++) {
		if (scgfiles(scgp)[i] >= 0)
			close(scgfiles(scgp)[i]);
		scgfiles(scgp)[i] = -1;
	}
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long	maxdma = MAX_DMA_OTHER;
#if	!defined(__i386_) && !defined(i386)
	int	cpu_type;
#endif

#if	defined(__i386_) || defined(i386)
	maxdma = MAX_DMA_SUN386;
#else
	cpu_type = gethostid() >> 24;

	switch (cpu_type & ARCH_MASK) {

	case ARCH_SUN4C:
	case ARCH_SUN4E:
		maxdma = MAX_DMA_SUN4C;
		break;

	case ARCH_SUN4M:
	case ARCH_SUNX:
		maxdma = MAX_DMA_SUN4M;
		break;

	default:
		maxdma = MAX_DMA_SUN3;
	}
#endif

#ifndef	__SVR4
	/*
	 * SunOS 4.x allows esp hardware on VME boards and thus
	 * limits DMA on esp to 64k-1
	 */
	if (maxdma > MAX_DMA_SUN3)
		maxdma = MAX_DMA_SUN3;
#endif
	return (maxdma);
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	if (scgp->local == NULL)
		return (FALSE);

	return (busno < 0 || busno >= MAX_SCG) ? FALSE : (scgfiles(scgp)[busno] >= 0);
}

LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL)
		return (-1);

	return (busno < 0 || busno >= MAX_SCG) ? -1 : scgfiles(scgp)[busno];
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	int		id = -1;
#ifdef	DKIO_GETCINFO
	struct dk_cinfo	conf;
#endif

#ifdef	DKIO_GETCINFO
	if (scgp->fd < 0)
		return (id);
	if (ioctl(scgp->fd, DKIO_GETCINFO, &conf) < 0)
		return (id);
	if (TARGET(conf.dki_slave) != -1)
		id = TARGET(conf.dki_slave);
#endif
	return (id);
}

LOCAL int
scgo_isatapi(scgp)
	SCSI	*scgp;
{
	return (FALSE);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (what == SCG_RESET_NOP)
		return (0);
	if (what != SCG_RESET_BUS) {
		errno = EINVAL;
		return (-1);
	}
	return (ioctl(scgp->fd, SCGIORESET, 0));
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	scgp->bufbase = (void *)valloc((size_t)amt);
	return (scgp->bufbase);
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;
}

LOCAL int
scgo_send(scgp)
	SCSI	*scgp;
{
	scgp->scmd->target = scg_target(scgp);
	return (ioctl(scgp->fd, SCGIO_CMD, scgp->scmd));
}

/*--------------------------------------------------------------------------*/
/*
 *	This is Sun USCSI interface code ...
 */
#ifdef	USE_USCSI
#include <sys/scsi/impl/uscsi.h>

/*
 * Bit Mask definitions, for use accessing the status as a byte.
 */
#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02

#define	STATUS_RESERVATION_CONFLICT	0x18
#define	STATUS_TERMINATED		0x22

#ifdef	nonono
#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02

#define	STATUS_MET			0x04
#define	STATUS_BUSY			0x08
#define	STATUS_INTERMEDIATE		0x10
#define	STATUS_SCSI2			0x20
#define	STATUS_INTERMEDIATE_MET		0x14
#define	STATUS_RESERVATION_CONFLICT	0x18
#define	STATUS_TERMINATED		0x22
#define	STATUS_QFULL			0x28
#define	STATUS_ACA_ACTIVE		0x30
#endif

LOCAL int
scgo_uhelp(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "USCSI", "SCSI transport for targets known by Solaris drivers",
		"USCSI:", "bus,target,lun", "USCSI:1,2,0", TRUE, TRUE);
	return (0);
}

LOCAL int
scgo_uopen(scgp, device)
	SCSI	*scgp;
	char	*device;
{
		 int	busno	= scg_scsibus(scgp);
		 int	tgt	= scg_target(scgp);
		 int	tlun	= scg_lun(scgp);
	register int	f;
	register int	b;
	register int	t;
	register int	l;
	register int	nopen = 0;
	char		devname[32];

	if (scgp->overbose) {
		js_fprintf((FILE *)scgp->errfile,
				"Warning: Using USCSI interface.\n");
	}

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
		    js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
		       "Illegal value for busno, target or lun '%d,%d,%d'",
		       busno, tgt, tlun);

		return (-1);
	}
	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof(struct scg_local));
		if (scgp->local == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "No memory for scg_local");
			return (0);
		}

		for (b=0; b < MAX_SCG; b++) {
			for (t=0; t < MAX_TGT; t++) {
				for (l=0; l < MAX_LUN ; l++)
					scglocal(scgp)->u.scg_files[b][t][l] = (short)-1;
			}
		}
	}

	if (device != NULL && strcmp(device, "USCSI") == 0)
		goto uscsiscan;

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2))
		goto openbydev;

uscsiscan:
	if (busno >= 0 && tgt >= 0 && tlun >= 0) {

		if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN)
			return (-1);

		js_snprintf(devname, sizeof(devname), "/dev/rdsk/c%dt%dd%ds2",
			busno, tgt, tlun);
		f = open(devname, O_RDONLY | O_NDELAY);
		if (f < 0) {
			js_snprintf(scgp->errstr,
				    SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", devname);
			return (0);
		}
		scglocal(scgp)->u.scg_files[busno][tgt][tlun] = f;
		return 1;
	} else {

		for (b=0; b < MAX_SCG; b++) {
			for (t=0; t < MAX_TGT; t++) {
				for (l=0; l < MAX_LUN ; l++) {
					js_snprintf(devname, sizeof(devname),
						"/dev/rdsk/c%dt%dd%ds2",
						b, t, l);
					f = open(devname, O_RDONLY | O_NDELAY);
					if (f < 0 && errno != ENOENT
							&& errno != ENXIO
							&& errno != ENODEV) {
						if (scgp->errstr)
							js_snprintf(scgp->errstr,
							    SCSI_ERRSTR_SIZE,
							    "Cannot open '%s'", devname);
					}
					if (f < 0 && l == 0)
						break;
					if (f >= 0) {
						nopen ++;
						if (scglocal(scgp)->u.scg_files[b][t][l] == -1)
						scglocal(scgp)->u.scg_files[b][t][l] = f;
					else
						close(f);
					}
				}
			}
		}
	}
openbydev:
	if (nopen == 0) {
		int	target;
		int	lun;

		if (device != NULL && strncmp(device, "USCSI:", 6) == 0)
			device += 6;
		if (device == NULL || device[0] == '\0')
			return (0);

		f = open(device, O_RDONLY | O_NDELAY);
		if (f < 0) {
			js_snprintf(scgp->errstr,
				    SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", device);
			return (0);
		}

		if (busno < 0)
			busno = 0;	/* Use Fake number if not specified */

		if (scgo_ugettlun(f, &target, &lun) >= 0) {
			if (tgt >= 0 && tlun >= 0) {
				if (tgt != target || tlun != lun) {
					close(f);
					return (0);
				}
			}
			tgt = target;
			tlun = lun;
		} else {
			if (tgt < 0 || tlun < 0) {
				close(f);
				return (0);
			}
		}

		if (scglocal(scgp)->u.scg_files[busno][tgt][tlun] == -1)
			scglocal(scgp)->u.scg_files[busno][tgt][tlun] = f;
		scg_settarget(scgp, busno, tgt, tlun);

		return (++nopen);
	}
	return (nopen);
}

LOCAL int
scgo_uclose(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++) {
				f = scglocal(scgp)->u.scg_files[b][t][l];
				if (f >= 0)
					close(f);
				scglocal(scgp)->u.scg_files[b][t][l] = (short)-1;
			}
		}
	}
	return (0);
}

LOCAL int
scgo_ucinfo(f, cp, debug)
	int		f;
	struct dk_cinfo *cp;
	int		debug;
{
	fillbytes(cp, sizeof(*cp), '\0');

	if (ioctl(f, DKIOCINFO, cp) < 0)
		return (-1);

	if (debug <= 0)
		return (0);

	js_fprintf(stderr, "cname:		'%s'\n", cp->dki_cname);
	js_fprintf(stderr, "ctype:		0x%04hX %hd\n", cp->dki_ctype, cp->dki_ctype);
	js_fprintf(stderr, "cflags:		0x%04hX\n", cp->dki_flags);
	js_fprintf(stderr, "cnum:		%hd\n", cp->dki_cnum);
#ifdef	__EVER__
	js_fprintf(stderr, "addr:		%d\n", cp->dki_addr);
	js_fprintf(stderr, "space:		%d\n", cp->dki_space);
	js_fprintf(stderr, "prio:		%d\n", cp->dki_prio);
	js_fprintf(stderr, "vec:		%d\n", cp->dki_vec);
#endif
	js_fprintf(stderr, "dname:		'%s'\n", cp->dki_dname);
	js_fprintf(stderr, "unit:		%d\n", cp->dki_unit);
	js_fprintf(stderr, "slave:		%d %04o Tgt: %d Lun: %d\n",
				cp->dki_slave, cp->dki_slave,
				TARGET(cp->dki_slave), LUN(cp->dki_slave));
	js_fprintf(stderr, "partition:	%hd\n", cp->dki_partition);
	js_fprintf(stderr, "maxtransfer:	%d (%d)\n",
				cp->dki_maxtransfer,
				cp->dki_maxtransfer * DEV_BSIZE);
	return (0);
}

LOCAL int
scgo_ugettlun(f, tgtp, lunp)
	int	f;
	int	*tgtp;
	int	*lunp;
{
	struct dk_cinfo ci;

	if (scgo_ucinfo(f, &ci, 0) < 0)
		return (-1);
	if (tgtp)
		*tgtp = TARGET(ci.dki_slave);
	if (lunp)
		*lunp = LUN(ci.dki_slave);
	return (0);
}

LOCAL long
scgo_umaxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	register int	b;
	register int	t;
	register int	l;
	long		maxdma = -1L;
	int		f;
	struct dk_cinfo ci;

	if (scgp->local == NULL)
		return (-1L);

	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++) {
				if ((f = scglocal(scgp)->u.scg_files[b][t][l]) < 0)
					continue;
				if (scgo_ucinfo(f, &ci, 0) < 0)
					continue;
				if (maxdma < 0)
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
				if (maxdma > (long)(ci.dki_maxtransfer * DEV_BSIZE))
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
			}
		}
	}
	/*
	 * The Sun tape driver does not support to retrieve the max DMA count.
	 * Use the knwoledge about default DMA sizes in this case.
	 */
	if (maxdma < 0)
		maxdma = scgo_maxdma(scgp, amt);

	return (maxdma);
}

LOCAL BOOL
scgo_uhavebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (scgp->local == NULL || busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	for (t=0; t < MAX_TGT; t++) {
		for (l=0; l < MAX_LUN ; l++)
			if (scglocal(scgp)->u.scg_files[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

LOCAL int
scgo_ufileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL ||
	    busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	return ((int)scglocal(scgp)->u.scg_files[busno][tgt][tlun]);
}

LOCAL int
scgo_uinitiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL int 
scgo_uisatapi(scgp)
	SCSI	*scgp;
{
	char		devname[32];
	char		symlinkname[MAXPATHLEN];
	int		len;
	struct dk_cinfo ci;

	if (ioctl(scgp->fd, DKIOCINFO, &ci) < 0)
		return (-1);

	js_snprintf(devname, sizeof(devname), "/dev/rdsk/c%dt%dd%ds2",
		scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));

	symlinkname[0] = '\0';
	len = readlink(devname, symlinkname, sizeof(symlinkname));
	if (len > 0)
		symlinkname[len] = '\0';

	if (len >= 0 && strstr(symlinkname, "ide") != NULL)
		return (TRUE);
	else
		return (FALSE);
}

LOCAL int
scgo_ureset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	struct uscsi_cmd req;

	if (what == SCG_RESET_NOP)
		return (0);

	fillbytes(&req, sizeof(req), '\0');

	if (what == SCG_RESET_TGT) {
		req.uscsi_flags = USCSI_RESET | USCSI_SILENT;	/* reset target */
	} else if (what != SCG_RESET_BUS) {
		req.uscsi_flags = USCSI_RESET_ALL | USCSI_SILENT;/* reset bus */
	} else {
		errno = EINVAL;
		return (-1);
	}

	return (ioctl(scgp->fd, USCSICMD, &req));
}

LOCAL int
scgo_usend(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	struct uscsi_cmd req;
	int		ret;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	fillbytes(&req, sizeof(req), '\0');

	req.uscsi_flags = USCSI_SILENT | USCSI_DIAGNOSE | USCSI_RQENABLE;

	if (sp->flags & SCG_RECV_DATA) {
		req.uscsi_flags |= USCSI_READ;
	} else if (sp->size > 0) {
		req.uscsi_flags |= USCSI_WRITE;
	}
	req.uscsi_buflen	= sp->size;
	req.uscsi_bufaddr	= sp->addr;
	req.uscsi_timeout	= sp->timeout;
	req.uscsi_cdblen	= sp->cdb_len;
	req.uscsi_rqbuf		= (caddr_t) sp->u_sense.cmd_sense;
	req.uscsi_rqlen		= sp->sense_len;
	req.uscsi_cdb		= (caddr_t) &sp->cdb;

	errno = 0;
	ret = ioctl(scgp->fd, USCSICMD, &req);

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile, "ret: %d errno: %d (%s)\n", ret, errno, errmsgstr(errno));
		js_fprintf((FILE *)scgp->errfile, "uscsi_flags:     0x%x\n", req.uscsi_flags);
		js_fprintf((FILE *)scgp->errfile, "uscsi_status:    0x%x\n", req.uscsi_status);
		js_fprintf((FILE *)scgp->errfile, "uscsi_timeout:   %d\n", req.uscsi_timeout);
		js_fprintf((FILE *)scgp->errfile, "uscsi_bufaddr:   0x%lx\n", (long)req.uscsi_bufaddr);
								/*
								 * Cast auf int OK solange sp->size
								 * auch ein int bleibt.
								 */
		js_fprintf((FILE *)scgp->errfile, "uscsi_buflen:    %d\n", (int)req.uscsi_buflen);
		js_fprintf((FILE *)scgp->errfile, "uscsi_resid:     %d\n", (int)req.uscsi_resid);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqlen:     %d\n", req.uscsi_rqlen);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqstatus:  0x%x\n", req.uscsi_rqstatus);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqresid:   %d\n", req.uscsi_rqresid);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqbuf ptr: 0x%lx\n", (long)req.uscsi_rqbuf);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqbuf:     ");
		if (req.uscsi_rqbuf != NULL && req.uscsi_rqlen > req.uscsi_rqresid) {
			int	i;
			int	len = req.uscsi_rqlen - req.uscsi_rqresid;

			for (i = 0; i < len; i++) {
				js_fprintf((FILE *)scgp->errfile, "0x%02X ", ((char *)req.uscsi_rqbuf)[i]);
			}		
			js_fprintf((FILE *)scgp->errfile, "\n");
		} else {
			js_fprintf((FILE *)scgp->errfile, "<data not available>\n");
		}
	}
	if (ret < 0) {
		sp->ux_errno = geterrno();
		/*
		 * Check if SCSI command cound not be send at all.
		 */
		if (sp->ux_errno == ENOTTY && scgo_uisatapi(scgp) == TRUE) {
			if (scgp->debug > 0) {
				js_fprintf((FILE *)scgp->errfile,
					"ENOTTY atapi: %d\n", scgo_uisatapi(scgp));
			}
			sp->error = SCG_FATAL;
			return (0);
		}
		if (errno == ENXIO) {
			sp->error = SCG_FATAL;
			return (0);
		}
		if (errno == ENOTTY || errno == EINVAL || errno == EACCES) {
			return (-1);
		}
	} else {
		sp->ux_errno = 0;
	}
	ret			= 0;
	sp->sense_count		= req.uscsi_rqlen - req.uscsi_rqresid;
	sp->resid		= req.uscsi_resid;
	sp->u_scb.cmd_scb[0]	= req.uscsi_status;

	if ((req.uscsi_status & STATUS_MASK) == STATUS_GOOD) {
		sp->error = SCG_NO_ERROR;
		return (0);
	}
	if (req.uscsi_rqstatus == 0 &&
	    ((req.uscsi_status & STATUS_MASK) == STATUS_CHECK)) {
		sp->error = SCG_NO_ERROR;
		return (0);
	}
	if (req.uscsi_status & (STATUS_TERMINATED |
				 STATUS_RESERVATION_CONFLICT)) {
		sp->error = SCG_FATAL;
	}
	if (req.uscsi_status != 0) {
		/*
		 * This is most likely wrong. There seems to be no way
		 * to produce SCG_RETRYABLE with USCSI.
		 */
		sp->error = SCG_RETRYABLE;
	}

	return (ret);
}
#endif	/* USE_USCSI */
