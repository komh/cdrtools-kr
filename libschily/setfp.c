/* @(#)setfp.c	1.7 00/05/07 Copyright 1988 J. Schilling */
/*
 *	Set frame pointer
 *
 *	Copyright (c) 1988 J. Schilling
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
#include <standard.h>
#include <avoffset.h>
#include <schily.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif
#ifdef	NO_SCANSTACK
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

#	ifdef	HAVE_SCANSTACK

#define	MAXWINDOWS	32
#define	NWINDOWS	7

#if defined(sparc) && defined(__GNUC__)
#	define	IDX		3	/* some strange things on sparc gcc */
#else
#	define	IDX		1
#endif

void setfp(fp)
	void	* const *fp;
{
		long	**dummy[1];
	static	int	idx = IDX;	/* fool optimizer in c compiler */

#ifdef	sparc
	flush_reg_windows(MAXWINDOWS-2);
#endif

	*(&dummy[idx]) = (long **)fp;

#ifdef	sparc
	flush_reg_windows(MAXWINDOWS-2);
#endif
}

#else

void setfp(fp)
	void	* const *fp;
{
	raisecond("setfp_not_implemented", 0L);
}

#endif
