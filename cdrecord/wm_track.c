/* @(#)wm_track.c	1.3 01/10/29 Copyright 1995, 1997 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)wm_track.c	1.3 01/10/29 Copyright 1995, 1997 J. Schilling";
#endif
/*
 *	CDR write method abtraction layer
 *	track at once writing intercace routines
 *
 *	Copyright (c) 1995, 1997 J. Schilling
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
#include <stdio.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <standard.h>
#include <utypes.h>

#include "cdrecord.h"

extern	int	debug;
extern	int	verbose;
extern	int	lverbose;

extern	char	*buf;			/* The transfer buffer */

EXPORT	int	write_track_data __PR((cdr_t *dp, int track, track_t *trackp));

