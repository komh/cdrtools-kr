/* @(#)boot.c	1.9 02/05/30 Copyright 1999 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)boot.c	1.9 02/05/30 Copyright 1999 J. Schilling";
#endif
/*
 *	Support for generic boot (sector 0..16)
 *	and to boot Sun sparc systems.
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

#include <mconfig.h>
#include "mkisofs.h"
#include <fctldefs.h>
#include <utypes.h>
#include <intcvt.h>
#include <schily.h>
#include "sunlabel.h"

LOCAL struct sun_label cd_label;
LOCAL char	*boot_files[NDKMAP];

LOCAL	void	init_sparc_label	__PR((void));
EXPORT	void	sparc_boot_label	__PR((char *label));
EXPORT	void	scan_sparc_boot		__PR((char *files));
EXPORT	int	make_sun_label		__PR((void));
LOCAL	void	dup_sun_label		__PR((int part));
LOCAL	int	sunboot_write		__PR((FILE *outfile));
LOCAL	int	sunlabel_size		__PR((int starting_extent));
LOCAL	int	sunlabel_write		__PR((FILE * outfile));
LOCAL	int	genboot_size		__PR((int starting_extent));
LOCAL	int	genboot_write		__PR((FILE * outfile));

/*
 * Set the virtual geometry in the disk label.
 * If we like to make the geometry variable, we may change
 * dkl_ncyl and dkl_pcyl later.
 */
LOCAL void
init_sparc_label()
{
	
	i_to_4_byte(cd_label.dkl_vtoc.v_version, V_VERSION);
	i_to_2_byte(cd_label.dkl_vtoc.v_nparts, NDKMAP);
	i_to_4_byte(cd_label.dkl_vtoc.v_sanity, VTOC_SANE);

	i_to_2_byte(cd_label.dkl_rpm, CD_RPM);
	i_to_2_byte(cd_label.dkl_pcyl, CD_PCYL);
	i_to_2_byte(cd_label.dkl_apc, CD_APC);
	i_to_2_byte(cd_label.dkl_intrlv, CD_INTRLV);
	i_to_2_byte(cd_label.dkl_ncyl, CD_NCYL);
	i_to_2_byte(cd_label.dkl_acyl, CD_ACYL);
	i_to_2_byte(cd_label.dkl_nhead, CD_NHEAD);
	i_to_2_byte(cd_label.dkl_nsect, CD_NSECT);

	cd_label.dkl_magic[0] =	DKL_MAGIC_0;
	cd_label.dkl_magic[1] =	DKL_MAGIC_1;
}

/*
 * For command line parser: set ASCII label.
 */
EXPORT void
sparc_boot_label(label)
	char	*label;
{
	strncpy(cd_label.dkl_ascilabel, label, 127);
}

/*
 * Parse the command line argument for boot images.
 */
EXPORT void
scan_sparc_boot(files)
	char	*files;
{
	char		*p;
	int		i = 1;
	struct stat	statbuf;
	int		status;

	init_sparc_label();

	do {
		if (i >= NDKMAP)
			comerrno(EX_BAD, "Too many boot partitions.\n");
		boot_files[i++] = files;
		if ((p = strchr(files, ',')) != NULL)
			*p++ = '\0';
		files = p;
	} while (p);

	i_to_2_byte(cd_label.dkl_vtoc.v_part[0].p_tag,  V_USR);
	i_to_2_byte(cd_label.dkl_vtoc.v_part[0].p_flag, V_RONLY);
	for (i=0; i < NDKMAP; i++) {
		p = boot_files[i];
		if (p == NULL || *p == '\0')
			continue;
		if (strcmp(p, "...") == '\0')
			break;

		status = stat_filter(p, &statbuf);
		if (status < 0 || access(p, R_OK) < 0)
			comerr("Cannot access '%s'.\n", p);

		i_to_4_byte(cd_label.dkl_map[i].dkl_nblk,
			roundup(statbuf.st_size, CD_CYLSIZE)/512);

		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_tag,  V_ROOT);
		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_flag, V_RONLY);
	}
}

/*
 * Finish the Sun disk label and compute the size of the additional data.
 */
EXPORT int
make_sun_label()
{
	int	last;
	int	cyl = 0;
	int	nblk;
	int	bsize;
	int	i;
	char	*p;

	/*
	 * Compute the size of the padding for the iso9660 image
	 * to allow the next partition to start on a cylinder boundary.
	 */
	last = roundup(last_extent, (CD_CYLSIZE/SECTOR_SIZE));

	i_to_4_byte(cd_label.dkl_map[0].dkl_nblk, last*4);
	bsize = 0;
	for (i=0; i < NDKMAP; i++) {
		p = boot_files[i];
		if (p != NULL && strcmp(p, "...") == '\0') {
			dup_sun_label(i);
			break;
		}
		if ((nblk = a_to_4_byte(cd_label.dkl_map[i].dkl_nblk)) == 0)
			continue;

		i_to_4_byte(cd_label.dkl_map[i].dkl_cylno, cyl);
		cyl += nblk / (CD_CYLSIZE/512);
		if (i > 0)
			bsize += nblk;
	}
	bsize /= 4;
	return (last-last_extent+bsize);
}

/*
 * Duplicate a partition of the Sun disk label until all partitions are filled up.
 */
LOCAL void
dup_sun_label(part)
	int	part;
{
	int	cyl;
	int	nblk;
	int	i;


	if (part < 1 || part >= NDKMAP)
		part = 1;
	cyl = a_to_4_byte(cd_label.dkl_map[part-1].dkl_cylno);
	nblk = a_to_4_byte(cd_label.dkl_map[part-1].dkl_nblk);

	for (i=part; i < NDKMAP; i++) {
		i_to_4_byte(cd_label.dkl_map[i].dkl_cylno, cyl);
		i_to_4_byte(cd_label.dkl_map[i].dkl_nblk, nblk);

		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_tag,  V_ROOT);
		i_to_2_byte(cd_label.dkl_vtoc.v_part[i].p_flag, V_RONLY);
	}
}

/*
 * Write out Sun boot partitions.
 */
LOCAL int
sunboot_write(outfile)
	FILE	*outfile;
{
	char	buffer[SECTOR_SIZE];
	int	i;
	int	n;
	int	nblk;
	int	amt;
	int	f;
	char	*p;

	memset(buffer, 0, sizeof(buffer));

	/*
	 * Write padding to the iso9660 image to allow the
	 * boot partitions to start on a cylinder boundary.
	 */
	amt = roundup(last_extent_written, (CD_CYLSIZE/SECTOR_SIZE)) - last_extent_written;
	for (n=0; n < amt; n++) {
	 	xfwrite(buffer, 1, SECTOR_SIZE, outfile);
		last_extent_written++;
	}
	for (i=1; i < NDKMAP; i++) {
		p = boot_files[i];
		if (p != NULL && strcmp(p, "...") == '\0')
			break;
		if ((nblk = a_to_4_byte(cd_label.dkl_map[i].dkl_nblk)) == 0)
			continue;
		if ((f = open(boot_files[i], O_RDONLY| O_BINARY)) < 0)
			comerr("Cannot open '%s'.\n", boot_files[i]);
		
		amt = nblk / 4;
		for (n=0; n < amt; n++) {
			memset(buffer, 0, sizeof(buffer));
			if (read(f, buffer, SECTOR_SIZE) < 0)
				comerr("Read error on '%s'.\n", boot_files[i]);
		 	xfwrite(buffer, 1, SECTOR_SIZE, outfile);
			last_extent_written++;
		}
		close(f);
	}
	fprintf(stderr, "Total extents including sparc boot = %d\n", 
				last_extent_written - session_start);
	return (0);
}

/*
 * Do size management for the Sun disk label that is located in the first
 * sector of a disk.
 */
LOCAL int
sunlabel_size(starting_extent)
	int	starting_extent;
{
	if (last_extent != session_start)
		comerrno(EX_BAD, "Cannot create sparc boot on offset != 0.\n");
	last_extent++;
	return 0;
}

/*
 * Cumpute the checksum and write a Sun disk label to the first sector
 * of the disk.
 * If the -generic-boot option has been specified too, overlay the
 * Sun disk label on the firs 512 bytes of the generic boot code.
 */
LOCAL int
sunlabel_write(outfile)
	FILE	*outfile;
{
		 char	buffer[SECTOR_SIZE];
	register char	*p;
	register short	count = (512/2) - 1;
		 int	f;

	memset(buffer, 0, sizeof(buffer));
	if (genboot_image) {
		if ((f = open(genboot_image, O_RDONLY| O_BINARY)) < 0)
			comerr("Cannot open '%s'.\n", genboot_image);

		if (read(f, buffer, SECTOR_SIZE) < 0)
			comerr("Read error on '%s'.\n", genboot_image);
		close(f);
	}

	/*
	 * If we don't already have a Sun disk label text
	 * set up the default.
	 */
	if (cd_label.dkl_ascilabel[0] == '\0')
		strcpy(cd_label.dkl_ascilabel, CD_DEFLABEL);

	p = (char *)&cd_label;
	cd_label.dkl_cksum[0] = 0;
	cd_label.dkl_cksum[1] = 0; 
	while (count--) {
		cd_label.dkl_cksum[0] ^= *p++;
		cd_label.dkl_cksum[1] ^= *p++; 
	}

	memcpy(buffer, &cd_label, 512);
 	xfwrite(buffer, 1, SECTOR_SIZE, outfile);
	last_extent_written++;
	return (0);
}

/*
 * Do size management for the generic boot code on sectors 0..16.
  */
LOCAL int
genboot_size(starting_extent)
	int	starting_extent;
{
	if (last_extent > (session_start + 1))
		comerrno(EX_BAD, "Cannot create generic boot on offset != 0.\n");
	last_extent = session_start + 16;
	return 0;
}

/*
 * Write the generic boot code to sectors 0..16.
 * If there is a Sun disk label, start writing at sector 1.
 */
LOCAL int
genboot_write(outfile)
	FILE	*outfile;
{
	char	buffer[SECTOR_SIZE];
	int	i;
	int	f;

	if ((f = open(genboot_image, O_RDONLY| O_BINARY)) < 0)
		comerr("Cannot open '%s'.\n", genboot_image);

	for(i=0; i < 16; i++) {
		memset(buffer, 0, sizeof(buffer));
		if (read(f, buffer, SECTOR_SIZE) < 0)
			comerr("Read error on '%s'.\n", genboot_image);

		if (i != 0 || last_extent_written == session_start) {
		 	xfwrite(buffer, 1, SECTOR_SIZE, outfile);
			last_extent_written++;
		}
	}
	close(f);
	return (0);
}

struct output_fragment sunboot_desc	= {NULL, NULL,		NULL,	sunboot_write,  "Sun Boot" };
struct output_fragment sunlabel_desc	= {NULL, sunlabel_size,	NULL,	sunlabel_write, "Sun Disk Label" };
struct output_fragment genboot_desc	= {NULL, genboot_size,	NULL,	genboot_write,  "Generic Boot" };
