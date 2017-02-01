/* @(#)auinfo.c	1.17 02/09/20 Copyright 1998-2002 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)auinfo.c	1.17 02/09/20 Copyright 1998-2002 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2002 J. Schilling
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

#include <mconfig.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <stdio.h>
#include <standard.h>
#include <strdefs.h>
#include <deflts.h>
#include <utypes.h>
#include <schily.h>

#include "cdrecord.h"
#include "cdtext.h"

extern	int	debug;

EXPORT	void	auinfo		__PR((char *name, int track, track_t *trackp));
LOCAL	textptr_t *gettextptr	__PR((int track, track_t *trackp));
LOCAL	char 	*savestr	__PR((char *name));
LOCAL	char 	*readtag	__PR((char *name));
LOCAL	char 	*readtstr	__PR((char *name));
EXPORT	void	setmcn		__PR((char *mcn, track_t *trackp));
LOCAL	void	isrc_illchar	__PR((char *isrc, int c));
EXPORT	void	setisrc		__PR((char *isrc, track_t *trackp));
EXPORT	void	setindex	__PR((char *tindex, track_t *trackp));

#ifdef	XXX
main(ac, av)
	int	ac;
	char	*av[];
{
/*	auinfo("/etc/default/cdrecord");*/
/*	auinfo("/mnt2/CD3/audio_01.inf");*/
	auinfo("/mnt2/CD3/audio_01.wav");
}
#endif

EXPORT void
auinfo(name, track, trackp)
	char	*name;
	int	track;
	track_t	*trackp;
{
	char	infname[1024];
	char	*p;
	track_t	*tp = &trackp[track];
	textptr_t *txp;
	long	l;
	long	tno = -1;
	BOOL	isdao = !is_tao(&trackp[0]);

	strncpy(infname, name, sizeof(infname)-1);
	infname[sizeof(infname)-1] = '\0';
	p = strrchr(infname, '.');
	if (p != 0 && &p[4] < &name[sizeof(infname)]) {
		strcpy(&p[1], "inf");
	}

	if (defltopen(infname) == 0) {

		p = readtstr("CDINDEX_DISCID=");
		p = readtag("CDDB_DISKID=");

		p = readtag("MCN=");
		if (p && *p) {
			setmcn(p, &trackp[0]);
			txp = gettextptr(0, trackp); /* MCN is isrc for trk 0*/
			txp->tc_isrc = savestr(p);
		}

		p = readtag("ISRC=");
		if (p && *p) {
			setisrc(p, &trackp[track]);
			txp = gettextptr(track, trackp);
			txp->tc_isrc = savestr(p);
		}

		p = readtstr("Albumperformer=");
		if (p && *p) {
			txp = gettextptr(0, trackp); /* Album perf. in trk 0*/
			txp->tc_performer = savestr(p);
		}
		p = readtstr("Performer=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_performer = savestr(p);
		}
		p = readtstr("Albumtitle=");
		if (p && *p) {
			txp = gettextptr(0, trackp); /* Album title in trk 0*/
			txp->tc_title = savestr(p);
		}
		p = readtstr("Tracktitle=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_title = savestr(p);
		}
		p = readtstr("Songwriter=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_songwriter = savestr(p);
		}
		p = readtstr("Composer=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_composer = savestr(p);
		}
		p = readtstr("Arranger=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_arranger = savestr(p);
		}
		p = readtstr("Message=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_message = savestr(p);
		}
		p = readtstr("Diskid=");
		if (p && *p) {
			txp = gettextptr(0, trackp); /* Disk id is in trk 0*/
			txp->tc_title = savestr(p);
		}
		p = readtstr("Closed_info=");
		if (p && *p) {
			txp = gettextptr(track, trackp);
			txp->tc_closed_info = savestr(p);
		}

		p = readtag("Tracknumber=");
		if (p && isdao)
			astol(p, &tno);

		p = readtag("Trackstart=");
		if (p && isdao) {
			l = -1L;
			astol(p, &l);
			if (track == 1 && tno == 1 && l > 0) {
				trackp[1].pregapsize = 150 + l;
				printf("Track1 Start: '%s' (%ld)\n", p, l);
			}
		}

		p = readtag("Tracklength=");

		p = readtag("Pre-emphasis=");
		if (p && *p) {
			if (strncmp(p, "yes", 3) == 0) {
				tp->flags |= TI_PREEMP;
				if (tp->tracktype == TOC_DA)
					tp->sectype = SECT_AUDIO_PRE;

			} else if (strncmp(p, "no", 2) == 0) {
				tp->flags &= ~TI_PREEMP;
				if (tp->tracktype == TOC_DA)
					tp->sectype = SECT_AUDIO_NOPRE;
			}
		}

		p = readtag("Channels=");
		p = readtag("Copy_permitted=");
		if (p && *p) {
			/*
			 * -useinfo always wins
			 */
			tp->flags &= ~(TI_COPY|TI_SCMS);

			if (strncmp(p, "yes", 3) == 0)
				tp->flags |= TI_COPY;
			else if (strncmp(p, "no", 2) == 0)
				tp->flags |= TI_SCMS;
			else if (strncmp(p, "once", 2) == 0)
				tp->flags &= ~(TI_COPY|TI_SCMS);
		}
		p = readtag("Endianess=");
		p = readtag("Index=");
		if (p && *p && isdao)
			setindex(p, &trackp[track]);

		p = readtag("Index0=");
		if (p && isdao) {
			Llong ts;
			Llong ps;

			l = -2L;
			astol(p, &l);
			if (l == -1) {
				trackp[track+1].pregapsize = 0;
			} else if (l > 0) {
				ts = tp->itracksize / tp->isecsize;
				ps = ts - l;
				if (ps > 0)
					trackp[track+1].pregapsize = ps;
			}
		}
	}

}

LOCAL textptr_t *
gettextptr(track, trackp)
	int	track;
	track_t	*trackp;
{
	register textptr_t *txp;

	txp = (textptr_t *)trackp[track].text;
	if (txp == NULL) {
		txp = malloc(sizeof(textptr_t));
		if (txp == NULL)
			comerr("Cannot malloc CD-Text structure.\n");
		fillbytes(txp, sizeof(textptr_t), '\0');
		trackp[track].text = txp;
	}
	return (txp);
}

LOCAL char *
savestr(str)
	char	*str;
{
	char	*ret;

	ret = malloc(strlen(str)+1);
	if (ret)
		strcpy(ret, str);
	else
		comerr("Cannot malloc auinfo string.\n");
	return (ret);
}

LOCAL char *
readtag(name)
	char	*name;
{
	register char	*p;

	p = defltread(name);
	if (p) {
		while (*p == ' ' || *p == '\t')
			p++;
		if (debug)
			printf("%s	'%s'\n", name, p);
	}
	return (p);
}

LOCAL char *
readtstr(name)
	char	*name;
{
	register char	*p;
	register char	*p2;

	p = readtag(name);
	if (p && *p == '\'') {
		p2 = ++p;
		while (*p2 != '\0')
			p2++;
		while (p2 > p && *p2 != '\'')
			p2--;
		*p2 = '\0';
		if (debug)
			printf("%s	'%s'\n", name, p);
	}
	return (p);
}

/*
 * Media catalog number is a 13 digit number.
 */
EXPORT void
setmcn(mcn, trackp)
	char	*mcn;
	track_t	*trackp;
{
	register char	*p;

	if (strlen(mcn) != 13)
		comerrno(EX_BAD, "MCN '%s' has illegal length.\n", mcn);

	for (p = mcn; *p; p++) {
		if (*p < '0' || *p > '9')
			comerrno(EX_BAD, "MCN '%s' contains illegal character '%c'.\n", mcn, *p);
	}
	p = malloc(14);
	strcpy (p, mcn);
	trackp->isrc = p;

	if (debug)
		printf("Track %d MCN: '%s'\n", (int)trackp->trackno, trackp->isrc);
}

LOCAL	char	upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

LOCAL void
isrc_illchar(isrc, c)
	char	*isrc;
	int	c;
{
	errmsgno(EX_BAD, "ISRC '%s' contains illegal character '%c'.\n", isrc, c);
}

/*
 * ISRC is 12 Byte:
 *
 *	Country code   'C' (alpha)	  2 Bytes
 *	Owner code     'O' (alphanumeric) 3 Bytes
 *	Year of record 'Y' (numeric)	  2 Bytes
 *	Serial number  'S' (numeric)	  5 Bytes
 *
 *	CC-OOO-YY-SSSSS
 */
EXPORT void
setisrc(isrc, trackp)
	char	*isrc;
	track_t	*trackp;
{
	char	ibuf[13];
	char	*ip;
	char	*p;
	int	i;
	int	len;

	if ((len = strlen(isrc)) != 12) {
		for (p = isrc, i = 0; *p; p++) {
			if (*p == '-')
				i++;
		}
		if (((len - i) != 12) || i > 3)
			comerrno(EX_BAD, "ISRC '%s' has illegal length.\n", isrc);
	}

	/*
	 * The country code.
	 */
	for (p = isrc, ip = ibuf, i = 0; i < 2; p++, i++) {
		*ip++ = *p;
		if (!strchr(upper, *p)) {
/*			goto illchar;*/
			/*
			 * Flag numbers but accept them.
			 */
			isrc_illchar(isrc, *p);
			if (*p >= '0' && *p <= '9')
				continue;
			exit(EX_BAD);
		}
	}
	if (*p == '-')
		p++;

	/*
	 * The owner code.
	 */
	for (i = 0; i < 3; p++, i++) {
		*ip++ = *p;
		if (strchr(upper, *p))
			continue;
		if (*p >= '0' && *p <= '9')
			continue;
		goto illchar;
	}
	if (*p == '-')
		p++;

	/*
	 * The Year and the recording number (2 + 5 numbers).
	 */
	for (i = 0; i < 7; p++, i++) {
		*ip++ = *p;
		if (*p >= '0' && *p <= '9')
			continue;
		if (*p == '-' && i == 2) {
			ip--;
			i--;
			continue;
		}
		goto illchar;
	}
	*ip = '\0';
	p = malloc(13);
	strcpy (p, ibuf);
	trackp->isrc = p;

	if (debug)
		printf("Track %d ISRC: '%s'\n", (int)trackp->trackno, trackp->isrc);
	return;
illchar:
	isrc_illchar(isrc, *p);
	exit(EX_BAD);
}

EXPORT void
setindex(tindex, trackp)
	char	*tindex;
	track_t	*trackp;
{
	char	*p;
	int	i;
	int	nindex;
	long	idx;
	long	*idxlist;

	idxlist = malloc(100*sizeof(long));
	p = tindex;
	idxlist[0] = 0;
	i = 0;
	while (*p) {
		p = astol(p, &idx);
		if (*p != '\0' && *p != ' ' && *p != '\t' && *p != ',')
			goto illchar;
		i++;
		if (i > 99)
			comerrno(EX_BAD, "Too many indices for track %d\n", (int)trackp->trackno);
		idxlist[i] = idx;
		if (*p == ',')
			p++;
		while (*p == ' ' || *p == '\t')
			p++;
	}
	nindex = i;

	if (debug)
		printf("Track %d %d Index: '%s'\n", (int)trackp->trackno, i, tindex);

	if (debug) for (i=0; i <= nindex; i++)
		printf("%d: %ld\n", i, idxlist[i]);

	trackp->nindex = nindex;
	trackp->tindex = idxlist;
	return;
illchar:
	comerrno(EX_BAD, "Index '%s' contains illegal character '%c'.\n", tindex, *p);
}
