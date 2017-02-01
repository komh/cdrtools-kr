/* @(#)fdup.c	1.12 01/12/09 Copyright 1986, 1995 J. Schilling */
/*
 *	Copyright (c) 1986, 1995 J. Schilling
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

#include "io.h"

/*
 * Note that because of a definition in io.h we are using fseeko()/ftello()
 * instead of fseek()/ftell() if available.
 */

EXPORT FILE *
fdup(f)
	register FILE  *f;
{
	int  newfd;

	down(f);
	if ((newfd = dup(fileno(f))) < 0)
        	return (FILE *) NULL;

	lseek(newfd, ftell(f), SEEK_SET);
	return (_fcons((FILE *)0, newfd, (FI_READ | FI_WRITE | FI_CLOSE)));
} 

