/* @(#)scsi_scan.h	1.3 01/03/12 Copyright 1997 J. Schilling */
/*
 *	Interface to scan SCSI Bus.
 *
 *	Copyright (c) 1997 J. Schilling
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

#ifndef	_SCSI_SCAN_H
#define	_SCSI_SCAN_H

extern	int	select_target		__PR((SCSI *scgp, FILE *f));

#endif	/* _SCSI_SCAN_H */
