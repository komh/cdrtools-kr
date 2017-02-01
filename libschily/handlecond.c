/* @(#)handlecond.c	1.13 00/05/07 Copyright 1985 J. Schilling */
/*
 *	setup/clear a condition handler for a software signal
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
/*
 *	A procedure frame is marked to have handlers if the 
 *	previous freme pointer for this procedure is odd.
 *	The even base value, in this case actually points to a SIGBLK which
 *	holds the saved "real" frame pointer.
 *	The SIGBLK mentioned above may me the start of a chain of SIGBLK's,
 *	containing different handlers.
 *
 *	This will work on processors which support a frame pointer chain
 *	on the stack.
 *	On a processor which doesn't support this I think of a method
 *	where handlecond() has an own chain of frames, holding chains of
 *	SIGBLK's.
 *	In this case, a parameter has to be added to handlecond() and 
 *	unhandlecond(). This parameter will be an opaque cookie which is zero
 *	on the first call to handlecond() in a procedure. 
 *	A new cookie will be returned by handlecond() which must be used on
 *	each subsequent call to handlecond() and unhandlecond() in the same
 *	procedure.
 *
 *	Copyright (c) 1985 J. Schilling
 */
#include <mconfig.h>
#include <sigblk.h>
#include <standard.h>
#include <stdxlib.h>
#include <strdefs.h>
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

#ifdef	HAVE_SCANSTACK

#include <stkframe.h>

#define	is_even(p)	((((long)(p)) & 1) == 0)
#define	even(p)		(((long)(p)) & ~1L)
#define	odd(p)		(((long)(p)) | 1)
#ifdef	__future__
#define	even(p)		(((long)(p)) - 1)/* will this work with 64 bit ?? */
#endif

#ifdef	PROTOTYPES
void handlecond(const char	*signame,
		register SIGBLK	*sp,
		int		(*func)(const char *, long, long),
		long		arg1)
#else
void handlecond(signame, sp, func, arg1)
		char	*signame;
	register SIGBLK *sp;
		BOOL	(*func)();
		long	arg1;
#endif
{
	register SIGBLK	*this;
	register SIGBLK	*last = (SIGBLK *)NULL;
	struct frame	*fp;
		int	slen;

	if ((slen = strlen(signame)) == 0) {
		raisecond("handle_bad_name", (long)signame);
		abort();
	}

	fp = (struct frame *)getfp();
	fp = (struct frame *)fp->fr_savfp;	/* point to frame of caller */

	if (is_even(fp->fr_savfp)) {
		/*
		 * Easy case: no handlers yet
		 * save real framepointer in sp
		 */
		sp->sb_savfp   = (long **)fp->fr_savfp;

	} else for (this = (SIGBLK *)even(fp->fr_savfp);
						this;
						this = this->sb_signext) {
		if (this == sp) {
			if (!streql(sp->sb_signame, signame)) {
				raisecond("handle_reused_block", (long)signame);
				abort();
			}
			sp->sb_sigfun = func;
			sp->sb_sigarg = arg1;
			return;
		}
		if (streql(this->sb_signame, signame)) {
			if (last == (SIGBLK *)NULL) {
				/*
				 * this is first entry in chain
				 */
				if (this->sb_signext == (SIGBLK *)NULL) {
					/*
					 * only this entry in chain
					 * copy real frame pointer into new sp
					 */
					sp->sb_savfp = this->sb_savfp;
				} else {
					/*
					 * make second entry first link in chain
					 */
					this->sb_signext->sb_savfp =
								this->sb_savfp;
					fp->fr_savfp =
					(struct frame *)odd(this->sb_signext);
				}
				continue;	/* don't trash 'last' */
			} else {
				last->sb_signext = this->sb_signext;
			}
		}
		last = this;
	}
	sp->sb_signext = (SIGBLK *)NULL;
	sp->sb_signame = signame;
	sp->sb_siglen  = slen;
	sp->sb_sigfun  = func;
	sp->sb_sigarg  = arg1;
	if (last)
		last->sb_signext = sp;
	else
		fp->fr_savfp = (struct frame *)odd(sp);
}

#ifdef	PROTOTYPES
void unhandlecond(void)
#else
void unhandlecond()
#endif
{
	register struct frame	*fp;
	register SIGBLK		*sp;

	fp = (struct frame *)getfp();
	fp = (struct frame *)fp->fr_savfp;	/* point to frame of caller */

	if (!is_even(fp->fr_savfp)) {			/* if handlers	     */
		sp = (SIGBLK *)even(fp->fr_savfp);	/* point to SIGBLK   */
							/* real framepointer */
		fp->fr_savfp = (struct frame *)sp->sb_savfp;
	}
}

#else	/* HAVE_SCANSTACK */

#ifdef	PROTOTYPES
void handlecond(const char	*signame,
		register SIGBLK	*sp,
		int		(*func)(const char *, long, long),
		long		arg1)
#else
void handlecond(signame, sp, func, arg1)
		char	*signame;
	register SIGBLK *sp;
		BOOL	(*func)();
		long	arg1;
#endif
{
	/* dummy function */
}

#ifdef	PROTOTYPES
void unhandlecond(void)
#else
void unhandlecond()
#endif
{
	/* dummy function */
}

#endif	/* HAVE_SCANSTACK */
