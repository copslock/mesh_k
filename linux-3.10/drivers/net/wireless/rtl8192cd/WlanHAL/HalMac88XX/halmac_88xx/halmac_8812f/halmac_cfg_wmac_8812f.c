/******************************************************************************
 *
 * Copyright(c) 2018 - 2019 Realtek Corporation. All rights reserved.
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

#include "halmac_cfg_wmac_8812f.h"

#if HALMAC_8812F_SUPPORT

/**
 * cfg_drv_info_8812f() - config driver info
 * @adapter : the adapter of halmac
 * @drv_info : driver information selection
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
cfg_drv_info_8812f(struct halmac_adapter *adapter,
		   enum halmac_drv_info drv_info)
{
	u8 drv_info_size = 0;
	u8 phy_status_en = 0;
	u8 sniffer_en = 0;
	u8 plcp_hdr_en = 0;
	u32 value32;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	struct halmac_rx_ignore_info *info = &adapter->rx_ignore_info;
	struct halmac_mac_rx_ignore_cfg cfg;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);
	PLTFM_MSG_TRACE("[TRACE]drv info = %d\n", drv_info);

	switch (drv_info) {
	case HALMAC_DRV_INFO_NONE:
		drv_info_size = 0;
		phy_status_en = 0;
		sniffer_en = 0;
		plcp_hdr_en = 0;
		info->hdr_chk_mask = 1;
		info->fcs_chk_mask = 1;
		break;
	case HALMAC_DRV_INFO_PHY_STATUS:
		drv_info_size = 4;
		phy_status_en = 1;
		sniffer_en = 0;
		plcp_hdr_en = 0;
		info->hdr_chk_mask = 1;
		info->fcs_chk_mask = 1;
		break;
	case HALMAC_DRV_INFO_PHY_SNIFFER:
		drv_info_size = 5; /* phy status 4byte, sniffer info 1byte */
		phy_status_en = 1;
		sniffer_en = 1;
		plcp_hdr_en = 0;
		info->hdr_chk_mask = 0;
		info->fcs_chk_mask = 0;
		break;
	case HALMAC_DRV_INFO_PHY_PLCP:
		drv_info_size = 6; /* phy status 4byte, plcp header 2byte */
		phy_status_en = 1;
		sniffer_en = 0;
		plcp_hdr_en = 1;
		info->hdr_chk_mask = 0;
		info->fcs_chk_mask = 0;
		break;
	default:
		return HALMAC_RET_SW_CASE_NOT_SUPPORT;
	}

	cfg.hdr_chk_en = info->hdr_chk_en;
	cfg.fcs_chk_en = info->fcs_chk_en;
	cfg.cck_rst_en = info->cck_rst_en;
	cfg.fcs_chk_thr = info->fcs_chk_thr;
	api->halmac_set_hw_value(adapter, HALMAC_HW_RX_IGNORE, &cfg);

	HALMAC_REG_W8(REG_RX_DRVINFO_SZ, drv_info_size);

	adapter->drv_info_size = drv_info_size;

	value32 = HALMAC_REG_R32(REG_RCR);
	value32 = (value32 & (~BIT_APP_PHYSTS));
	if (phy_status_en == 1)
		value32 = value32 | BIT_APP_PHYSTS;
	HALMAC_REG_W32(REG_RCR, value32);

	value32 = HALMAC_REG_R32(REG_WMAC_OPTION_FUNCTION + 4);
	value32 = (value32 & (~(BIT(8) | BIT(9))));
	if (sniffer_en == 1)
		value32 = value32 | BIT(9);
	if (plcp_hdr_en == 1)
		value32 = value32 | BIT(8);
	HALMAC_REG_W32(REG_WMAC_OPTION_FUNCTION + 4, value32);

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * init_low_pwr_8812f() - config WMAC register
 * @adapter
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
init_low_pwr_8812f(struct halmac_adapter *adapter)
{
	u16 value16;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	/*RXGCK FIFO threshold CFG*/
	value16 = (HALMAC_REG_R16(REG_RXPSF_CTRL + 2) & 0xF00F);
	value16 |= BIT(10) | BIT(8) | BIT(6) | BIT(4);
	HALMAC_REG_W16(REG_RXPSF_CTRL + 2, value16);

	/*invalid_pkt CFG*/
	value16 = 0;
	value16 = BIT_SET_RXPSF_PKTLENTHR(value16, 1);
	value16 |= BIT_RXPSF_CTRLEN | BIT_RXPSF_VHTCHKEN | BIT_RXPSF_HTCHKEN
		| BIT_RXPSF_OFDMCHKEN | BIT_RXPSF_CCKCHKEN
		| BIT_RXPSF_OFDMRST;

	HALMAC_REG_W16(REG_RXPSF_CTRL, value16);
	HALMAC_REG_W32(REG_RXPSF_TYPE_CTRL, 0xFFFFFFFF);

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

void
cfg_rxgck_fifo_8812f(struct halmac_adapter *adapter, u8 enable)
{
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	if (enable == 1) {
		if (adapter->hw_cfg_info.trx_mode != HALMAC_TRNSFER_NORMAL)
			PLTFM_MSG_ERR("[ERR]trx_mode != normal\n");
		else
			HALMAC_REG_W8_SET(REG_RXPSF_CTRL + 3, BIT(4));
	} else {
		HALMAC_REG_W8_CLR(REG_RXPSF_CTRL + 3, BIT(4));
	}
}

void
cfg_rx_ignore_8812f(struct halmac_adapter *adapter,
		    struct halmac_mac_rx_ignore_cfg *cfg)
{
	u16 value16;
	struct halmac_rx_ignore_info *info = &adapter->rx_ignore_info;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	value16 = HALMAC_REG_R16(REG_RXPSF_CTRL);

	info->hdr_chk_en = cfg->hdr_chk_en;
	info->fcs_chk_en = cfg->fcs_chk_en;
	info->cck_rst_en = cfg->cck_rst_en;
	info->fcs_chk_thr = cfg->fcs_chk_thr;

	/*mac header check enable*/
	if (cfg->hdr_chk_en == 1 && info->hdr_chk_mask == 1)
		value16 |= BIT_RXPSF_MHCHKEN;
	else
		value16 &= ~(BIT_RXPSF_MHCHKEN);

	/*continuous FCS error counter enable*/
	if (cfg->fcs_chk_en == 1 && info->fcs_chk_mask == 1)
		value16 |= BIT_RXPSF_CONT_ERRCHKEN;
	else
		value16 &= ~(BIT_RXPSF_CONT_ERRCHKEN);

	/*MAC Rx reset when CCK enable*/
	if (cfg->cck_rst_en == 1)
		value16 |= BIT_RXPSF_CCKRST;
	else
		value16 &= ~(BIT_RXPSF_CCKRST);

	/*FCS error counter threshold*/
	value16 = BIT_SET_RXPSF_ERRTHR(value16, cfg->fcs_chk_thr);

	HALMAC_REG_W16(REG_RXPSF_CTRL, value16);

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);
}

void
cfg_ampdu_8812f(struct halmac_adapter *adapter,
		struct halmac_ampdu_config *cfg)
{
	u32 ht_max_len;
	u32 vht_max_len;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	HALMAC_REG_W8(REG_PROT_MODE_CTRL + 2, cfg->max_agg_num);
	HALMAC_REG_W8(REG_PROT_MODE_CTRL + 3, cfg->max_agg_num);

	if (cfg->max_len_en == 1) {
		ht_max_len = cfg->ht_max_len & 0xFFFF;
		vht_max_len = cfg->vht_max_len & 0xFFFFF;
		HALMAC_REG_W32(REG_AMPDU_MAX_LENGTH_HT, ht_max_len);
		HALMAC_REG_W32(REG_AMPDU_MAX_LENGTH_VHT, vht_max_len);
	}
}

#endif /* HALMAC_8812F_SUPPORT */
