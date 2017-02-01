/* @(#)bootinfo.h	1.2 00/02/14 Copyright 1999 J. Schilling */
/*
 *	Header file bootinfo.h - mkisofs-defined boot information table
 *	useful for an El Torito-loaded disk image.
 *
 *	Copyright (c) 1999 J. Schilling
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

#ifndef _BOOTINFO_H
#define _BOOTINFO_H

struct mkisofs_boot_info {
	char bi_pvd	 [ISODCL (  1,  4)]; /* LBA of PVD */
	char bi_file	 [ISODCL (  5,  8)]; /* LBA of boot image */
	char bi_length	 [ISODCL (  9, 12)]; /* Length of boot image */
	char bi_csum	 [ISODCL ( 13, 16)]; /* Checksum of boot image */
	char bi_reserved [ISODCL ( 17, 56)]; /* Reserved */
};

#endif	/* _BOOTINFO_H */
