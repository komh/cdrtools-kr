/* @(#)mmcvendor.h	1.2 02/08/29 Copyright 2002 J. Schilling */
/*
 *	Copyright (c) 2002 J. Schilling
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

#ifndef	_MMCVENDOR_H
#define	_MMCVENDOR_H

#include <utypes.h>
#include <btorder.h>

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct ricoh_mode_page_30 {
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0xE = 14 Bytes */
	Ucbit	BUEFS		:1;	/* Burn-Free supported	*/
	Ucbit	TWBFS		:1;	/* Test Burn-Free sup.	*/
	Ucbit	res_2_23	:2;
	Ucbit	ARSCS		:1;	/* Auto read speed control supp. */
	Ucbit	AWSCS		:1;	/* Auto write speed control supp. */
	Ucbit	res_2_67	:2;
	Ucbit	BUEFE		:1;	/* Burn-Free enabled	*/
	Ucbit	res_2_13	:3;
	Ucbit	ARSCE		:1;	/* Auto read speed control enabled */
	Ucbit	AWSCD		:1;	/* Auto write speed control disabled */
	Ucbit	res_3_67	:2;
	Uchar	link_counter[2];	/* Burn-Free link counter */
	Uchar	res[10];		/* Padding up to 16 bytes */
};

#else				/* Motorola bitorder */

struct ricoh_mode_page_30 {
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0xE = 14 Bytes */
	Ucbit	res_2_67	:2;
	Ucbit	AWSCS		:1;	/* Auto write speed control supp. */
	Ucbit	ARSCS		:1;	/* Auto read speed control supp. */
	Ucbit	res_2_23	:2;
	Ucbit	TWBFS		:1;	/* Test Burn-Free sup.	*/
	Ucbit	BUEFS		:1;	/* Burn-Free supported	*/
	Ucbit	res_3_67	:2;
	Ucbit	AWSCD		:1;	/* Auto write speed control disabled */
	Ucbit	ARSCE		:1;	/* Auto read speed control enabled */
	Ucbit	res_2_13	:3;
	Ucbit	BUEFE		:1;	/* Burn-Free enabled	*/
	Uchar	link_counter[2];	/* Burn-Free link counter */
	Uchar	res[10];		/* Padding up to 16 bytes */
};
#endif

struct cd_mode_vendor {
        struct scsi_mode_header header;
        union cd_v_pagex  {
                struct ricoh_mode_page_30 page30;
        } pagex;
};


#endif	/* _MMCVENDOR_H */
