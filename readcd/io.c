/* @(#)io.c	1.19 98/10/10 Copyright 1988 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)io.c	1.19 98/10/10 Copyright 1988 J. Schilling";
#endif
/*
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
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <mconfig.h>
#include <stdio.h>
#include <standard.h>
#include <vadefs.h>
#include <stdxlib.h>
#include <strdefs.h>
#include <utypes.h>
#include <schily.h>
#include <ctype.h>

struct disk {
	int	dummy;
};

LOCAL	char	*skipwhite	__PR((const char *));
LOCAL	void	prt_std		__PR((char *, long, long, long, struct disk *));
EXPORT	BOOL	cvt_std		__PR((char *, long *, long, long, struct disk *));
extern	BOOL	getvalue	__PR((char *, long *, long, long,
				void (*)(char *, long, long, long, struct disk *),
				BOOL (*)(char *, long *, long, long, struct disk *),
				struct disk *));
extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));
extern	BOOL	yes		__PR((char *, ...));

LOCAL
char *skipwhite(s)
		 const char	*s;
{
	register const Uchar	*p = (const Uchar *)s;

	while (*p) {
		if (!isspace(*p))
			break;
		p++;
	}
	return ((char *)p);
}

/* ARGSUSED */
EXPORT
BOOL
cvt_std(linep, lp, mini, maxi, dp)
	char	*linep;
	long	*lp;
	long	mini;
	long	maxi;
	struct disk	*dp;
{
	long	l	= -1L;

/*	printf("cvt_std(\"%s\", %d, %d, %d);\n", linep, *lp, mini, maxi);*/

	if (linep[0] == '?') {
		printf("Enter a number in the range from %ld to %ld\n",
								mini, maxi);
		printf("The default radix is 10\n");
		printf("Precede number with '0x' for hexadecimal or with '0' for octal\n");
		printf("Shorthands are:\n");
		printf("\t'^' for minimum value (%ld)\n", mini);
		printf("\t'$' for maximum value (%ld)\n", maxi);
		printf("\t'+' for incrementing value to %ld\n", *lp + 1);
		printf("\t'-' for decrementing value to %ld\n", *lp - 1);
		return (FALSE);
	}
	if (linep[0] == '^' && *skipwhite(&linep[1]) == '\0') {
		l = mini;
	} else if (linep[0] == '$' && *skipwhite(&linep[1]) == '\0') {
		l = maxi;
	} else if (linep[0] == '+' && *skipwhite(&linep[1]) == '\0') {
		if (*lp < maxi)
			l = *lp + 1;
	} else if (linep[0] == '-' && *skipwhite(&linep[1]) == '\0') {
		if (*lp > mini)
			l = *lp - 1;
	} else if (*astol(linep, &l)) {
		printf("Not a number: '%s'.\n", linep);
		return (FALSE);
	}
	if (l < mini || l > maxi) {
		printf("'%s' is out of range.\n", linep);
		return (FALSE);
	}
	*lp = l;
	return (TRUE);
}

/* ARGSUSED */
LOCAL void
prt_std(s, l, mini, maxi, dp)
	char	*s;
	long	l;
	long	mini;
	long	maxi;
	struct disk *dp;
{
	printf("%s %ld (%ld - %ld)/<cr>:", s, l, mini, maxi);
}

EXPORT
BOOL getvalue(s, lp, mini, maxi, prt, cvt, dp)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
	void	(*prt) __PR((char *, long, long, long, struct disk *));
	BOOL	(*cvt) __PR((char *, long *, long, long, struct disk *));
	struct disk	*dp;
{
	char	line[128];
	char	*linep;

	for(;;) {
		(*prt)(s, *lp, mini, maxi, dp);
		flush();
		line[0] = '\0';
		if (getline(line, 80) == EOF)
			exit(EX_BAD);

		linep = skipwhite(line);
		/*
		 * Nicht initialisierte Variablen
		 * duerfen nicht uebernommen werden
		 */
		if (linep[0] == '\0' && *lp != -1L)
			return (FALSE);

		if (strlen(linep) == 0) {
			/* Leere Eingabe */
			/* EMPTY */
		} else if ((*cvt)(linep, lp, mini, maxi, dp))
			return (TRUE);
	}
	/* NOTREACHED */
}

EXPORT
BOOL getlong(s, lp, mini, maxi)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
{
	return (getvalue(s, lp, mini, maxi, prt_std, cvt_std, (void *)0));
}

EXPORT
BOOL getint(s, ip, mini, maxi)
	char	*s;
	int	*ip;
	int	mini;
	int	maxi;
{
	long	l = *ip;
	BOOL	ret;

	ret = getlong(s, &l, (long)mini, (long)maxi);
	*ip = l;
	return (ret);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT BOOL yes(char *form, ...)
#else
EXPORT
BOOL yes(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	char okbuf[10];

again:
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	printf("%r", form, args);
	va_end(args);
	flush();
	if (getline(okbuf, sizeof(okbuf)) == EOF)
		exit(EX_BAD);
	if (okbuf[0] == '?') {
		printf("Enter 'y', 'Y', 'yes' or 'YES' if you agree with the previous asked question.\n");
		printf("All other input will be handled as if the question has beed answered with 'no'.\n");
		goto again;
	}
	if(streql(okbuf, "y") || streql(okbuf, "yes") ||
	   streql(okbuf, "Y") || streql(okbuf, "YES"))
		return(TRUE);
	else
		return(FALSE);
}
