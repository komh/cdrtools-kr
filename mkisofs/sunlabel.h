/* @(#)sunlabel.h	1.3 00/02/16 Copyright 1999 J. Schilling */
/*
 *	Support for Sun disk label
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

#ifndef howmany
#define howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#ifndef roundup
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#endif

#define	NDKMAP		8
#define DKL_MAGIC	0xDABE		/* magic number */ 
#define DKL_MAGIC_0	0xDA		/* magic number high byte */ 
#define DKL_MAGIC_1	0xBE		/* magic number low byte  */ 

#define	CD_DEFLABEL	"CD-ROM Disc with Sun sparc boot created by mkisofs"

/*
 * Define a virtual geometry for the CD disk label.
 * The current values are stolen from Sun install disks and do not seem to be
 * a good idea as they limit the size of the CD to 327680 sectors which is less
 * than 74 minutes.
 * There are 84 minute CD's with 378000 sectors and there will be DVD's with
 * even more.
 */
#define	CD_RPM		350
#define	CD_PCYL		2048
#define	CD_APC		0
#define	CD_INTRLV	1
#define	CD_NCYL		2048
#define	CD_ACYL		0
#define	CD_NHEAD	1
#define	CD_NSECT	640

/*
 * NOTE: The virtual cylinder size on CD must be a mutiple of 2048.
 *	 This is true if CD_NSECT is a multiple of 4.
 */
#define	CD_CYLSIZE	(CD_NSECT*CD_NHEAD*512)

#define	V_VERSION	1		/* The VTOC version	 */
#define	VTOC_SANE	0x600DDEEE	/* Indicates a sane VTOC */

#define	V_ROOT		0x02		/* Root partiton	 */
#define	V_USR		0x04		/* Usr partiton		 */

#define	V_RONLY		0x10		/* Read only		 */

struct sun_label {
	char		dkl_ascilabel[128];
	struct dk_vtoc {
		Uchar	v_version[4];	/* layout version	 */
		char	v_volume[8];	/* volume name		 */
		Uchar	v_nparts[2];	/* number of partitions	 */
		struct dk_map2 {
			Uchar	p_tag[2]; /* ID tag of partition */
			Uchar	p_flag[2];/* permission flag	 */

		}	v_part[NDKMAP];
		Uchar	v_xxpad[2];	/* To come over Sun's alignement problem */
		Uchar	v_bootinfo[3*4];/* info for mboot	 */
		Uchar	v_sanity[4];	/* tp verify vtoc sanity */
		Uchar	v_reserved[10*4];
		Uchar	v_timestamp[NDKMAP*4];

	}		dkl_vtoc;	/* vtoc inclusions from AT&T SVr4 */
	char		dkl_pad[512-(128+sizeof(struct dk_vtoc)+NDKMAP*8+14*2)];
	Uchar	 	dkl_rpm[2];	/* rotations per minute */
	Uchar		dkl_pcyl[2];	/* # physical cylinders */ 
	Uchar		dkl_apc[2];	/* alternates per cylinder */
	Uchar		dkl_obs1[2];	/* obsolete */
	Uchar		dkl_obs2[2];	/* obsolete */
	Uchar		dkl_intrlv[2];	/* interleave factor */
	Uchar		dkl_ncyl[2];	/* # of data cylinders */
	Uchar		dkl_acyl[2];	/* # of alternate cylinders */
	Uchar		dkl_nhead[2];	/* # of heads in this partition */
	Uchar		dkl_nsect[2];	/* # of 512 byte sectors per track */
	Uchar		dkl_obs3[2];	/* obsolete */
	Uchar		dkl_obs4[2];	/* obsolete */

	struct dk_map {			/* logical partitions */
		Uchar	dkl_cylno[4];	/* starting cylinder */
		Uchar	dkl_nblk[4];	/* number of blocks */
	}		dkl_map[NDKMAP];/* logical partition headers */

	Uchar		dkl_magic[2];	/* identifies this label format */
	Uchar		dkl_cksum[2];	/* xor checksum of sector */
};
