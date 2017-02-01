/* @(#)scsi-amigaos.c	1.3 02/10/19 Copyright 1997,2000,2001 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-amigaos.c	1.3 02/10/19 Copyright 1997,2000,2001 J. Schilling";
#endif
/*
 *	Interface for the AmigaOS generic SCSI implementation.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1997, 2000, 2001 J. Schilling
 *	AmigaOS support code written by T. Langer
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

#define	BOOL	int

#include <exec/ports.h>
#include <exec/io.h>
#include <devices/scsidisk.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-amigaos.c-1.3";	/* The version for this transport */
LOCAL	char	_scg_auth[] = "T. Langer";

#define	MAX_SCG		2	/* Max # of SCSI controllers */
#define	MAX_TGT		8
#define	MAX_LUN		8

struct	scg_local {
	struct IOStdReq *scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};

#define	scglocal(p)	((struct scg_local *)((p)->local))

#define MAX_DMA_AMIGAOS (64*1024)

LOCAL struct IOStdReq	*open_amiga_scsi __PR((char *dev, int busno, int tgt, int tlun, char *errstr));

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
			return (_scg_trans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
/*			return (_scg_auth_schily);*/
			return (_scg_auth);
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
	__scg_help(f, "Amiga SCSI", "Generic SCSI",
		"", "bus,target,lun or xxx.device:b,t,l", "1,2,0 or scsi.device:1,2,0", TRUE, FALSE);
	return (0);
}

LOCAL int
scgo_open(scgp, device)
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
	register struct IOStdReq *ioreq;
	char            devname[64] = "scsi.device";

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr) {
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno, target or lun '%d,%d,%d'",
				busno, tgt, tlun);
		}
		return (-1);
	}
	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof(struct scg_local));
		if (scgp->local == NULL)
			return (0);

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++) {
					scglocal(scgp)->scgfiles[b][t][l] = (struct IOStdReq *)-1;
				}
			}
		}
	}
	if (device == NULL || *device == '\0') {
		device = devname;
	}
	if (tlun < 0) {
		tlun = 0;
	}
 /* cdrecord [-scanbus] dev=b,t,l | cdrecord [-scanbus] dev=xxx.device:b,t,l */
	if (busno >= 0 && tgt >= 0 && tlun >= 0) {
		ioreq = open_amiga_scsi(device, busno, tgt, tlun, scgp->errstr);
		if (ioreq != NULL) {
			scglocal(scgp)->scgfiles[busno][tgt][tlun] = ioreq;
			nopen++;;
		}
	}
 /* cdrecord -scanbus  | crederord -scanbus dev=xxxx.device */
	else {
		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++) {
					ioreq = open_amiga_scsi(device, b, t, l, scgp->errstr);

					if (ioreq != NULL) {
						scglocal(scgp)->scgfiles[b][t][l] = ioreq;
						nopen++;
					}
				}
			}
		}
	}
	return (nopen);
}

LOCAL struct IOStdReq *
open_amiga_scsi(dev, busno, tgt, tlun, errstr)
	char	*dev;
	int	busno;
	int	tgt;
	int	tlun;
	char	*errstr;
{
	register struct IOStdReq *ioreq = NULL;
	register int    f;
	ULONG           unit = (100 * busno) + (10 * tlun) + tgt;
	struct MsgPort *Port;

	Port = CreatePort(NULL, 0);
	if (Port != NULL) {
		ioreq = CreateStdIO(Port);
		if (ioreq != NULL) {
			f = OpenDevice(dev, unit, (struct IORequest *) ioreq, 0L);
			if (f != 0) {
				if (errstr) {
					js_snprintf(errstr, SCSI_ERRSTR_SIZE,
						"Cannot open '%s'",
						dev);
				}
				DeleteStdIO(ioreq);
				DeletePort(Port);
				ioreq = NULL;
			}
		} else {
			if (errstr) {
				js_snprintf(errstr, SCSI_ERRSTR_SIZE,
					"Cannot create IOReq");
			}
			DeletePort(Port);
		}
	} else {
		if (errstr) {
			js_snprintf(errstr, SCSI_ERRSTR_SIZE,
				"Cannot open Message Port");
		}
	}
	return (ioreq);
}


LOCAL int
scgo_close(SCSI * scgp)
{
	register struct IOStdReq *ioreq;
	register int    b;
	register int    t;
	register int    l;

	if (scgp->local == NULL)
		return (-1);

	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				ioreq = scglocal(scgp)->scgfiles[b][t][l];
				if (ioreq != (struct IOStdReq *)-1) {
					ULONG           unit = (100 * b) + (10 * l) + t;

					if (scgp->debug)
						printf("closing device unit %d\n", unit);
					CloseDevice((struct IORequest *) ioreq);
					DeleteStdIO(ioreq);
					DeletePort(ioreq->io_Message.mn_ReplyPort);
					scglocal(scgp)->scgfiles[b][t][l] = (struct IOStdReq *)-1;
				}
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
	return (MAX_DMA_AMIGAOS);
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
	scgp->bufbase = valloc((size_t)(amt));
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
			if (scglocal(scgp)->scgfiles[busno][t][l] != (struct IOStdReq *)-1)
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
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);

	return ((int)scglocal(scgp)->scgfiles[busno][tgt][tlun]);
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI           *scgp;
{
	return (-1);
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
	/* XXX synchronous reset command - is this wise? */
	errno = EINVAL;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	char		senseData[SCG_MAX_SENSE];
	register struct IOStdReq *ioreq = (struct IOStdReq *) scgp->fd;
	struct SCSICmd	Cmd;
	int		ret = 0;
	struct scg_cmd	*sp = scgp->scmd;

	sp->error = SCG_NO_ERROR;
	sp->sense_count = 0;
	sp->u_scb.cmd_scb[0] = 0;
	sp->resid = 0;

	if (ioreq == (struct IOStdReq *)-1) {
		sp->error = SCG_FATAL;
		return (0);
	}
	ioreq->io_Length = sizeof(struct SCSICmd);
	ioreq->io_Data = &Cmd;
	ioreq->io_Command = HD_SCSICMD;

	Cmd.scsi_Flags = SCSIF_AUTOSENSE;
	if (sp->flags & SCG_RECV_DATA)
		Cmd.scsi_Flags |= SCSIF_READ;
	else if (sp->size > 0)
		Cmd.scsi_Flags |= SCSIF_WRITE;

	Cmd.scsi_Command = malloc(sp->cdb_len);
	movebytes(sp->cdb.cmd_cdb, Cmd.scsi_Command, sp->cdb_len);
	Cmd.scsi_CmdLength = sp->cdb_len;
	Cmd.scsi_Data = (void *) sp->addr;
	Cmd.scsi_Length = sp->size;
	Cmd.scsi_Actual = 0;

	Cmd.scsi_SenseData = senseData;
	fillbytes(Cmd.scsi_SenseData, sizeof(senseData), '\0');
	if (sp->sense_len > sizeof(senseData))
		Cmd.scsi_SenseLength = sizeof(senseData);
	else if (sp->sense_len < 0)
		Cmd.scsi_SenseLength = 0;
	else
		Cmd.scsi_SenseLength = sp->sense_len;
	Cmd.scsi_SenseActual = 0;
	Cmd.scsi_Status = 0;

	if (DoIO((struct IORequest *) ioreq) != 0) {
		if (Cmd.scsi_Status == 0) {
			ret = -1;
			sp->ux_errno = geterrno();
			if (sp->ux_errno == 0)
				sp->ux_errno = EIO;

			if (sp->ux_errno != ENOTTY)
				ret = 0;
		}
	}

	fillbytes(&sp->scb, sizeof(sp->scb), '\0');
	fillbytes(&sp->u_sense.cmd_sense, sizeof(sp->u_sense.cmd_sense), '\0');
	sp->resid = Cmd.scsi_Length - Cmd.scsi_Actual;
	sp->sense_count = Cmd.scsi_SenseActual;
	if (sp->sense_count > SCG_MAX_SENSE)
		sp->sense_count = SCG_MAX_SENSE;
	movebytes(Cmd.scsi_SenseData, sp->u_sense.cmd_sense, sp->sense_count);
	sp->u_scb.cmd_scb[0] = Cmd.scsi_Status;

	switch (ioreq->io_Error) {

	case 0:
		sp->error = SCG_NO_ERROR;
		break;

	case HFERR_BadStatus:
		/*
		 * status and/or sense error
		 */
		sp->ux_errno = EIO;
		sp->error = SCG_NO_ERROR;
		break;

	case HFERR_DMA:
		/*
		 * DMA error
		 */
		sp->resid = sp->size;
		sp->error = SCG_RETRYABLE;
		break;

	case HFERR_SelTimeout:
		/*
		 * Select timed out
		 */
		sp->error = SCG_FATAL;
		break;

	/* XXX was ist hier zu tun ? */

	case HFERR_SelfUnit:
		/*
		 * cannot issue SCSI command to self
		 */
/* XXX try to select our HBA ??? */
		sp->error = SCG_FATAL;
		break;

	case HFERR_Phase:
		/*
		 * illegal or unexpected SCSI phase
		 */
		sp->error = SCG_RETRYABLE;
		break;

	case HFERR_Parity:
		/*
		 * SCSI parity error
		 */
		sp->error = SCG_RETRYABLE;
		break;

	default:
		/* XXX was kann sonst noch passieren? */
		sp->error = SCG_FATAL;
		break;
	}

	if (Cmd.scsi_Command != NULL) {
		free(Cmd.scsi_Command);
	}
	return (ret);
}
