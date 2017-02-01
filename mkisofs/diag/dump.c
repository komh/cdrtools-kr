/* @(#)dump.c	1.16 02/11/30 joerg */
#ifndef lint
static	char sccsid[] =
	"@(#)dump.c	1.16 02/11/30 joerg";
#endif
/*
 * File dump.c - dump a file/device both in hex and in ASCII.

   Written by Eric Youngdale (1993).

   Copyright 1993 Yggdrasil Computing, Incorporated

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mconfig.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <strdefs.h>
#include <utypes.h>

#include <stdio.h>
#include <standard.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#include <sys/ioctl.h>
#else
#include <termio.h>
#endif
#include <signal.h>
#include <schily.h>

FILE *	infile;
off_t	file_addr;
#define PAGE 256
unsigned char buffer[PAGE];
unsigned char search[64];

#ifdef HAVE_TERMIOS_H
struct termios savetty;
struct termios newtty;
#else
struct termio savetty;
struct termio newtty;
#endif

void	reset_tty	__PR((void));
void	set_tty		__PR((void));
void	onsusp		__PR((int sig));
void	crsr2		__PR((int row, int col));
void	showblock	__PR((int flag));
int	getbyte		__PR((void));
void	usage		__PR((int excode));
int	main		__PR((int argc, char *argv[]));

void
reset_tty(){
#ifdef HAVE_TERMIOS_H
  if(tcsetattr(0, TCSANOW, &savetty) == -1)
#else
  if(ioctl(0, TCSETAF, &savetty)==-1)
#endif
    {
#ifdef	USE_LIBSCHILY
      comerr("Cannot put tty into normal mode\n");
#else
      printf("Cannot put tty into normal mode\n");
      exit(1);
#endif
    }
}

void
set_tty()
{
#ifdef HAVE_TERMIOS_H
  if(tcsetattr(0, TCSANOW, &newtty) == -1)
#else
  if(ioctl(0, TCSETAF, &newtty)==-1)
#endif
    {
#ifdef	USE_LIBSCHILY
      comerr("Cannot put tty into raw mode\n");
#else
      printf("Cannot put tty into raw mode\n");
      exit(1);
#endif
    }
}


/* Come here when we get a suspend signal from the terminal */

void
onsusp(sig)
	int	sig;
{
#ifdef	SIGTTOU
    /* ignore SIGTTOU so we don't get stopped if csh grabs the tty */
    signal(SIGTTOU, SIG_IGN);
#endif
    reset_tty ();
    fflush (stdout);
#ifdef	SIGTTOU
    signal(SIGTTOU, SIG_DFL);
    /* Send the TSTP signal to suspend our process group */
    signal(SIGTSTP, SIG_DFL);
/*    sigsetmask(0);*/
    kill (0, SIGTSTP);
    /* Pause for station break */

    /* We're back */
    signal (SIGTSTP, onsusp);
#endif
    set_tty ();
}


void
crsr2(row, col)
	int	row;
	int	col;
{
  printf("\033[%d;%dH",row,col);
}

void
showblock(flag)
	int	flag;
{
  unsigned int k;
  int i, j;
  lseek(fileno(infile), file_addr, SEEK_SET);
  read(fileno(infile), buffer, sizeof(buffer));
  if(flag) {
    for(i=0;i<16;i++){
      crsr2(i+3,1);
	if (sizeof(file_addr) > sizeof(long)) {
		printf("%16.16llx ", (Llong)file_addr+(i<<4));
	} else {
		printf("%8.8lx ", (long)file_addr+(i<<4));
	}
      for(j=15;j>=0;j--){
	printf("%2.2x",buffer[(i<<4)+j]);
	if(!(j & 0x3)) printf(" ");
      };
      for(j=0;j< 16;j++){
	k = buffer[(i << 4) + j];
	if(k >= ' ' && k < 0x80) printf("%c",k);
	else printf(".");
      };
    }
  };
  crsr2(20,1);
	if (sizeof(file_addr) > sizeof(long)) {
		printf(" Zone, zone offset: %14llx %12.12llx  ",
			(Llong)file_addr>>11, (Llong)file_addr & 0x7ff);
	} else {
		printf(" Zone, zone offset: %6lx %4.4lx  ",
			(long)(file_addr>>11), (long)(file_addr & 0x7ff));
	}
  fflush(stdout);
}

int
getbyte()
{
  char c1;
  c1 = buffer[file_addr & (PAGE-1)];
  file_addr++;
  if ((file_addr & (PAGE-1)) == 0) showblock(0);
  return c1;
}

void
usage(excode)
	int	excode;
{
	errmsgno(EX_BAD, "Usage: %s [options] image\n",
                get_progname());

	error("Options:\n");
	error("\t-help,-h	Print this help\n");
	error("\t-version	Print version info and exit\n");
	exit(excode);
}

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	cac;
	char	* const *cav;
	char	*opts = "help,h,version";
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
  char c;
  int i,j;

	save_args(argc, argv);

	cac = argc - 1;
	cav = argv + 1;
	if (getallargs(&cac, &cav, opts, &help, &help, &prvers) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("devdump %s (%s-%s-%s)\n", "2.0",
					HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}
	cac = argc - 1;
	cav = argv + 1;
	if (getfiles(&cac, &cav, opts) == 0) {
		errmsgno(EX_BAD, "Missing Argument\n");
		usage(EX_BAD);
	}
	infile = fopen(cav[0],"rb");
	if (infile == NULL) {
#ifdef	USE_LIBSCHILY
		comerr("Cannot open '%s'.\n", cav[0]);
#else
		printf("Cannot open '%s'.\n", cav[0]);
		exit(1);
#endif
	}
	cac--, cav++;
	if (getfiles(&cac, &cav, opts) != 0) {
		errmsgno(EX_BAD, "Bad Argument: '%s'\n",cav[0]);
		usage(EX_BAD);
	}
	for(i=0;i<30;i++) printf("\n");
	file_addr = (off_t)0;
/* Now setup the keyboard for single character input. */
#ifdef HAVE_TERMIOS_H
	if(tcgetattr(0, &savetty) == -1)
#else
	if(ioctl(0, TCGETA, &savetty) == -1)
#endif
	  {
#ifdef	USE_LIBSCHILY
	    comerr("Stdin must be a tty\n");
#else
	    printf("Stdin must be a tty\n");
	    exit(1);
#endif
	  }
	newtty=savetty;
	newtty.c_lflag&=~ICANON;
	newtty.c_lflag&=~ECHO;
	newtty.c_cc[VMIN]=1;
  	set_tty();
#ifdef	SIGTSTP
	signal(SIGTSTP, onsusp);
#endif

  do{
    if(file_addr < (off_t)0) file_addr = (off_t)0;
    showblock(1);
    read (0, &c, 1);
    if (c == 'a') file_addr -= PAGE;
    if (c == 'b') file_addr += PAGE;
    if (c == 'g') {
      crsr2(20,1);
      printf("Enter new starting block (in hex):");
	if (sizeof(file_addr) > sizeof(long)) {
		Llong	ll;
		scanf("%llx",&ll);
		file_addr = (off_t)ll;
	} else {
		long	l;
		scanf("%lx",&l);
		file_addr = (off_t)l;
	}
      file_addr = file_addr << 11;
      crsr2(20,1);
      printf("                                     ");
    };
    if (c == 'f') {
      crsr2(20,1);
      printf("Enter new search string:");
      fgets((char *)search,sizeof(search),stdin);
      while(search[strlen((char *)search)-1] == '\n') search[strlen((char *)search)-1] = 0;
      crsr2(20,1);
      printf("                                     ");
    };
    if (c == '+') {
      while(1==1){
	while(1==1){
	  c = getbyte();
	  if (c == search[0]) break;
	};
	for (j=1;j < (int)strlen((char *)search);j++) 
	  if(search[j] != getbyte()) break;
	if(j==strlen((char *)search)) break;
      };
      file_addr &= ~(PAGE-1);
      showblock(1);
    };
    if (c == 'q') break;
  } while(1==1);
  reset_tty();
  fclose(infile);
  return (0);
}
