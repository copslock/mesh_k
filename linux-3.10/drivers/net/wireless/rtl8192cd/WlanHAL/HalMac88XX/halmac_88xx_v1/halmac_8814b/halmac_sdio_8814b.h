/******************************************************************************
 *
 * Copyright(c) 2016 - 2019 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/

#ifndef _HALMAC_SDIO_8814B_H_
#define _HALMAC_SDIO_8814B_H_

#include "../../halmac_api.h"

#if (HALMAC_8814B_SUPPORT && HALMAC_SDIO_SUPPORT)

enum halmac_ret_status
mac_pwr_switch_sdio_8814b(struct halmac_adapter *adapter,
			  enum halmac_mac_power pwr);

enum halmac_ret_status
phy_cfg_sdio_8814b(struct halmac_adapter *adapter,
		   enum halmac_intf_phy_platform pltfm);

enum halmac_ret_status
pcie_switch_sdio_8814b(struct halmac_adapter *adapter,
		       enum halmac_pcie_cfg cfg);

enum halmac_ret_status
intf_tun_sdio_8814b(struct halmac_adapter *adapter);

#endif /* HALMAC_8814B_SUPPORT */

#endif/* _HALMAC_SDIO_8814B_H_ */
