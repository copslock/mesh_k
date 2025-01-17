/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

/*============================================================
 include files
============================================================*/

#include "mp_precomp.h"
#include "../phydm_precomp.h"

#if (RTL8710C_SUPPORT == 1)
s8 phydm_cckrssi_8710c(struct dm_struct *dm, u8 lna_idx, u8 vga_idx)
{
	s8 rx_pwr_all = 0x00;

	switch (lna_idx) {
	case 7:
		rx_pwr_all = -40 - (2 * vga_idx);
		break;
	case 5:
		rx_pwr_all = -23 - (2 * vga_idx);
		break;
	case 3:
		rx_pwr_all = -12 - (2 * vga_idx);
		break;
	case 0:
		rx_pwr_all = 13 - (2 * vga_idx);
	default:
		break;
	}

	return rx_pwr_all;
}
#endif
