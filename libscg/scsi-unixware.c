/* @(#)scsi-unixware.c	1.27 02/10/19 Copyright 1998 J. Schilling, Santa Cruz Operation */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-unixware.c	1.27 02/10/19 Copyright 1998 J. Schilling, Santa Cruz Operation";
#endif
/*
 *	Interface for the SCO UnixWare SCSI implementation.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1998 J. Schilling, Santa Cruz Operation
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

#undef	sense
#undef	SC_PARITY
#undef	scb

#include <sys/sysmacros.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-unixware.c-1.27";	/* The version for this transport*/

/* Max. number of scg scsibusses.  The real limit would be           */
/* MAX_HBA * MAX_BUS (which would be 32 * 8 on UnixWare 2.1/7.x),    */
/* but given that we will hardly see such a beast, lets take 32      */

#define	MAX_SCG		32

	/* maximum defines for UnixWare 2.x/7.x from <sys/sdi_edt.h> */

#define	MAX_TGT		MAX_EXTCS	/* Max # of target id's      */
#define	MAX_LUN		MAX_EXLUS	/* Max # of lun's            */

#define	MAX_DMA		(32*1024)
#ifdef	__WHAT_TODO__
#define	MAX_DMA		(16*1024)	/* On UnixWare 2.1.x w/ AHA2940 HBA
					 * the max DMA size is 16KB.
					 */
#endif

#define MAXLINE		80
#define	MAXPATH		256

#define DEV_DIR		"/tmp"
#define	DEV_NAME	"scg.s%1dt%1dl%1d"

#define	SCAN_HBA	"%d:%d,%d,%d:%7s : %n"
#define	SCAN_DEV	"%d,%d,%d:%7s : %n"

#define PRIM_HBA	"/dev/hba/hba1"
#define	SCSI_CFG	"LC_ALL=C /etc/scsi/pdiconfig -l"

#define	SCAN_ALL	"LIBSCG_SCAN_ALL"

typedef struct scg2sdi {
	int	valid;
	int	open;
	int	atapi;
	int	initiator;
	int	hba;
	int	bus;
	int	tgt;
	int	lun;

	int	fd;
	dev_t	node;
	dev_t	major;
	dev_t	minor;
	char	type[20];
	char	vend[40];
	char	devn[32];

} scg2sdi_t;

LOCAL	scg2sdi_t	sdidevs [MAX_SCG][MAX_TGT][MAX_LUN];

struct scg_local {
	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};
#define scglocal(p)	((struct scg_local *)((p)->local)) 

LOCAL	int	fdesc[MAX_SCG][MAX_TGT][MAX_LUN];
LOCAL	int	nfopen;

LOCAL	int	unixware_init	__PR((SCSI *scgp));
LOCAL	int	do_scg_cmd	__PR((SCSI *scgp, struct scg_cmd *sp));
LOCAL	int	do_scg_sense	__PR((SCSI *scgp, struct scg_cmd *sp));

/* -------------------------------------------------------------------------
** SCO UnixWare 2.1.x / UnixWare 7 provides a scsi pass-through mechanism, 
** which can be used to access any configured scsi device. 
**
** NOTE: The libscg UnixWare passthrough routines have changed with 
**       cdrecord-1.8 to enable the -scanbus, -load, -eject option 
**	 regardless of the status of media and the addressing
**       scheme is now the same as used on many other platforms like
**       Solaris, Linux etc.
**
**      ===============================================================
**	RUN 'cdrecord -scanbus' TO SEE THE DEVICE ADDRESSES YOU CAN USE
**	===============================================================
*/

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 *
 */
LOCAL char *
scgo_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
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
	__scg_help(f, "SDI_SEND", "Generic SCSI",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
	return (0);
}

/* ---------------------------------------------------------------
** This routine is introduced to create all device nodes necessary
** to access any detected scsi device. It parses the output of
** /etc/scsi/pdiconfig -l and creates passthru device node for each
** found scsi device apart from the listed hba's.
**
*/

LOCAL int 
unixware_init(scgp)
	SCSI	*scgp;
{
	FILE		*cmd;
	int		hba = 0, bus = 0, scg = 0, tgt = 0, lun = 0;
	int		nscg = -1, lhba = -1, lbus = 0;
	int		atapi, fd, nopen = 0, pos = 0, len = 0;
	int		s, t, l;
	int		scan_disks;
	char		lines[MAXLINE];
	char		class[MAXLINE];
	char		ident[MAXLINE];
	char		dname[MAXPATH];
	struct stat 	stbuf;
	dev_t		ptdev, major, minor, node;
	char		**evsave;
extern	char		**environ;

	/* Check for validity of primary hostbus adapter node */

	if (stat(PRIM_HBA, &stbuf) < 0) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Can not stat() primary hba (%s)",
				PRIM_HBA);
		return (-1);
	}

	if (!S_ISCHR(stbuf.st_mode)) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Primary hba (%s) not a character device",
				PRIM_HBA);
		return (-1);
	}

	major = getmajor(stbuf.st_rdev);

	/*
	 * Check whether we want to scan all devices
	 */
	if (getenv(SCAN_ALL) != NULL) {
		scan_disks = 1;		
	} else {
		scan_disks = 0;
	}

	/* read pdiconfig output and get all attached scsi devices ! */

	evsave = environ;
	environ = 0;
	if ((cmd = popen(SCSI_CFG,"r")) == NULL) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Error popen() for \"%s\"",
				SCSI_CFG);
		environ = evsave;
		return (-1);
	}
	environ = evsave;


	for (;;) {
		if (fgets(lines, MAXLINE, cmd) == NULL) 
			break;

		memset(class, '\0', sizeof(class));
		memset(ident, '\0', sizeof(ident));

		if (lines[0] == ' ') {
			sscanf(lines, SCAN_DEV, &bus, &tgt, &lun, class, &pos);
			hba = lhba;
		} else {
			sscanf(lines, SCAN_HBA, &hba, &bus, &tgt, &lun, class, &pos);
			nscg++;
			lhba = hba;
			atapi = 0;
		}

		/* We can't sscanf() the ident string of the device     */
		/* as it may contain characters sscanf() will           */
		/* recognize as a delimiter. So do a strcpy() instead ! */

		len = strlen(lines) - pos - 1; /* don't copy the '\n' */

		strncpy(ident, &lines[pos], len);

		if (scgp->debug > 0) {
			js_fprintf((FILE *)scgp->errfile,
				"SDI -> %d:%d,%d,%d: %-7s : %s\n", 
				hba, bus, tgt, lun, class, ident);
		}
		if (bus != lbus) { 
			nscg++;
			lbus = bus;
		}

		scg = nscg;

		/* check whether we have a HBA or a SCSI device, don't  */
		/* let HBA's be valid device for cdrecord, but mark     */
		/* them as a controller (initiator = 1).                */

		/* Don't detect disks, opening a mounted disk can hang  */
		/* the disk subsystem !!! So unless we set an           */
		/* environment variable LIBSCG_SCAN_ALL, we will ignore */
		/* disks                                                */

		if (strstr(class, "HBA") == NULL) {
			if (strstr(class, "DISK") != NULL) {
				sdidevs[scg][tgt][lun].valid = scan_disks;
			} else {
				sdidevs[scg][tgt][lun].valid = 1;
			}
		} else {
			sdidevs[scg][tgt][lun].initiator = 1;
		}


		/* There is no real flag that shows a HBA as an ATAPI   */
		/* controller, so as we know the driver is called 'ide' */
		/* we can check the ident string for the occurence of it*/

		if (strstr(ident, "(ide,") != NULL) {
			atapi = 1;
		}

		/* fill the sdidevs array with all we know now          */

		sdidevs[scg][tgt][lun].open  = 0;
		sdidevs[scg][tgt][lun].atapi = atapi;

		sdidevs[scg][tgt][lun].hba = hba;
		sdidevs[scg][tgt][lun].bus = bus;
		sdidevs[scg][tgt][lun].tgt = tgt;
		sdidevs[scg][tgt][lun].lun = lun;

		strcpy(sdidevs[scg][tgt][lun].type, class);
		strcpy(sdidevs[scg][tgt][lun].vend, ident);

		js_snprintf(sdidevs[scg][tgt][lun].devn,
				sizeof(sdidevs[scg][tgt][lun].devn),
				DEV_NAME, scg, tgt, lun);

		minor = SDI_MINOR(hba, tgt, lun, bus);
		node  = makedevice(major, minor);	

		sdidevs[scg][tgt][lun].major = major;
		sdidevs[scg][tgt][lun].minor = minor;
		sdidevs[scg][tgt][lun].node  = node;

		if (scgp->debug > 0) {

			js_fprintf((FILE *)scgp->errfile,
			"h = %d; b = %d, s = %d, t = %d, l = %d, a = %d, ma = %d, mi = %2d, dev = %s, id = %s\n", 
			hba, bus, scg, tgt, lun, 
			sdidevs[scg][tgt][lun].atapi, 
			sdidevs[scg][tgt][lun].major, 
			sdidevs[scg][tgt][lun].minor, 
			sdidevs[scg][tgt][lun].devn,
			sdidevs[scg][tgt][lun].vend);
		}			


	}

	if (pclose(cmd) == -1) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Error pclose() for \"%s\"",
				SCSI_CFG);
		return (-1);
	}


	/* create all temporary device nodes */

	for (s = 0; s < MAX_SCG; s++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN ; l++) {

			  if (sdidevs[s][t][l].valid == 0) {
			  	sdidevs[s][t][l].fd = -1;
			  	continue;
			  }

			  /* Make pass-through interface device node */

			  js_snprintf(dname, sizeof(dname),
				"%s/%s", DEV_DIR, sdidevs[s][t][l].devn);

			  ptdev = sdidevs[s][t][l].node;

			  if (mknod(dname, S_IFCHR | 0700, ptdev) < 0) {
				if (errno == EEXIST) {
					unlink(dname);

					if (mknod(dname, S_IFCHR | 0700, ptdev) < 0) {
						if (scgp->errstr)
							js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
								"mknod() error for \"%s\"", dname);
						return (-1);
					}
				} else {
					if (scgp->errstr)
						js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"mknod() error for \"%s\"", dname);
					return (-1);
				}
			  }

			  /* Open pass-through device node */

			  if ((fd = open(dname, O_RDONLY)) < 0) {
				if (errno == EBUSY && fdesc[s][t][l] >= 0) {
					/*
					 * Device has already been opened, just
					 * return the saved file desc.
					 */
					fd = fdesc[s][t][l];
				} else {
					if (scgp->errstr)
						js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"can not open pass-through %s", dname);
					return (-1);
				}
			  }

			/*
			 * If for whatever reason we may open a pass through
			 * device more than once, this will waste fs's as we
			 * do not check for fdesc[s][t][l] == -1.
			 */
			  fdesc[s][t][l] = fd;
			  sdidevs[s][t][l].fd   = fd;
			  sdidevs[s][t][l].open = 1;
  			  nopen++;
			  scglocal(scgp)->scgfiles[s][t][l] = (short) fd;

			  if (scgp->debug > 0) {

				js_fprintf((FILE *)scgp->errfile,
					"s = %d, t = %d, l = %d, dev = %s, fd = %d\n", 
		    		    	s, t, l, 
					sdidevs[s][t][l].devn,
					sdidevs[s][t][l].fd);
			  }

			}
		}
	}

	if (nopen > 0)
		nfopen++;
	return (nopen);
}


LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	int	busno	= scg_scsibus(scgp);
	int	tgt	= scg_target(scgp);
	int	tlun	= scg_lun(scgp);
	int	c, b, t;

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
		if (scgp->local == NULL)
			return (0);

		for (c = 0; c < MAX_SCG; c++) {
			for (b = 0; b < MAX_TGT; b++) {
				for (t = 0; t < MAX_LUN ; t++)
					scglocal(scgp)->scgfiles[c][b][t] = (short)-1;
			}
		}
	}

	if (nfopen <= 0) {
		for (c = 0; c < MAX_SCG; c++) {
			for (b = 0; b < MAX_TGT; b++) {
				for (t = 0; t < MAX_LUN ; t++)
					fdesc[c][b][t] = -1;
			}
		}
	}

	memset(sdidevs, 0, sizeof(sdidevs));	/* init tmp_structure */

	if (*device != '\0') {		/* we don't allow old dev usage */
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			"Open by 'devname' no longer supported on this OS");
		return (-1);
	} else {			/* this is the new stuff     	 */
					/* it will do the initialisation */
					/* and return the number of      */
					/* detected devices to be used   */
					/* with the new addressing       */
					/* scheme.                       */

		return (unixware_init(scgp));
	}

}


LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	nfopen--;
	for (b=0; b < MAX_SCG; b++) {
		for (t=0; t < MAX_TGT; t++) {
			for (l=0; l < MAX_LUN ; l++) {

				f = scglocal(scgp)->scgfiles[b][t][l];
				if (f >= 0)
					close(f);

				if (nfopen <= 0)
					fdesc[b][t][l] = -1;
				sdidevs[b][t][l].fd    = -1;
				sdidevs[b][t][l].open  =  0;
				sdidevs[b][t][l].valid =  0;

				scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
			}
		}
	}
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return (MAX_DMA);
}


LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"scgo_getbuf: %ld bytes\n", amt);
	}
	scgp->bufbase = (void *) valloc((size_t)(amt));

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

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	if (scgp->local == NULL)
		return (FALSE);

	for (t=0; t < MAX_TGT; t++) {
		for (l=0; l < MAX_LUN ; l++)
			if (scglocal(scgp)->scgfiles[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (busno < 0 || busno >= MAX_SCG ||
	    tgt   < 0 || tgt   >= MAX_TGT ||
	    tlun  < 0 || tlun  >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);

	return ((int)scglocal(scgp)->scgfiles[busno][tgt][tlun]);
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	register int	t;
	register int	l;
	register int	busno;

	busno = scg_scsibus(scgp);

	if (busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	for (t=0; t < MAX_TGT; t++) {
		for (l=0; l < MAX_LUN ; l++)
			if (sdidevs[busno][t][l].initiator == 1){
				if (scgp->debug > 0) {
					js_fprintf((FILE *)scgp->errfile,
						"scgo_initiator_id: id = %d\n", t);
				}
				return (t);
			}
	}

	return (-1);
}

LOCAL int
scgo_isatapi(scgp)
	SCSI	*scgp;
{
	/* if the new address method is used we know if this is ATAPI*/

	return (sdidevs[scg_scsibus(scgp)][scg_target(scgp)][scg_lun(scgp)].atapi);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	errno = EINVAL;
	return(-1);
}

LOCAL int
do_scg_cmd(scgp, sp)
	SCSI		*scgp;
	struct scg_cmd	*sp;
{
	int			ret;
	int			i;
	struct sb		scsi_cmd;
	struct scb		*scbp;

	memset(&scsi_cmd,  0, sizeof(scsi_cmd));

	scsi_cmd.sb_type = ISCB_TYPE;
	scbp = &scsi_cmd.SCB;

	scbp->sc_cmdpt = (caddr_t) sp->cdb.cmd_cdb;
	scbp->sc_cmdsz = sp->cdb_len;

	scbp->sc_datapt = sp->addr;
	scbp->sc_datasz = sp->size;

	if (!(sp->flags & SCG_RECV_DATA) && (sp->size > 0))
		scbp->sc_mode = SCB_WRITE;
	else
		scbp->sc_mode = SCB_READ;

	scbp->sc_time = sp->timeout;

	errno = 0;
	for (;;) {
		if ((ret = ioctl(scgp->fd, SDI_SEND, &scsi_cmd)) < 0) {
			if (errno == EAGAIN){
				sleep(1);
				continue;
			}
			sp->ux_errno = errno;
			if (errno == 0)
				sp->ux_errno = EIO;
			sp->error = SCG_RETRYABLE;

			return (ret);
		}
		break;
	}

	memset(&sp->u_scb.Scb, 0, sizeof(sp->u_scb.Scb));
	sp->u_scb.cmd_scb[0] = scbp->sc_status;
	sp->resid = scbp->sc_resid;
	sp->ux_errno = errno;

	return (0);
}


LOCAL int
do_scg_sense(scgp, sp)
	SCSI		*scgp;
	struct scg_cmd	*sp;
{
	int		ret;
	struct scg_cmd	s_cmd;

	memset((caddr_t)&s_cmd, 0, sizeof(s_cmd));

	s_cmd.addr      = (caddr_t) sp->u_sense.cmd_sense;
	s_cmd.size      = sp->sense_len;
	s_cmd.flags     = SCG_RECV_DATA|SCG_DISRE_ENA;
	s_cmd.cdb_len   = SC_G0_CDBLEN;
	s_cmd.sense_len = CCS_SENSE_LEN;

	s_cmd.cdb.g0_cdb.cmd   = SC_REQUEST_SENSE;
	s_cmd.cdb.g0_cdb.lun   = sp->cdb.g0_cdb.lun;
	s_cmd.cdb.g0_cdb.count = sp->sense_len;

	ret = do_scg_cmd(scgp, &s_cmd);

	if (ret < 0)
		return (ret);

	sp->sense_count = sp->sense_len - s_cmd.resid;
	return (ret);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int	ret;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	} else {

		ret = do_scg_cmd(scgp, sp);

		if (ret < 0)
			return (ret);

		if (sp->u_scb.cmd_scb[0] & S_CKCON)
			ret = do_scg_sense(scgp, sp);

		return (ret);
	}
}

#define	sense	u_sense.Sense
#undef	SC_PARITY
#define	SC_PARITY	0x09
#define	scb		u_scb.Scb
