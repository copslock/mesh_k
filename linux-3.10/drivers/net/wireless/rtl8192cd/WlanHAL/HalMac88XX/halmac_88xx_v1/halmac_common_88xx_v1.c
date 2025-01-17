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

#include "halmac_common_88xx_v1.h"
#include "halmac_88xx_v1_cfg.h"
#include "halmac_init_88xx_v1.h"
#include "halmac_cfg_wmac_88xx_v1.h"
#include "halmac_efuse_88xx_v1.h"
#include "halmac_bb_rf_88xx_v1.h"
#if HALMAC_USB_SUPPORT
#include "halmac_usb_88xx_v1.h"
#endif
#if HALMAC_SDIO_SUPPORT
#include "halmac_sdio_88xx_v1.h"
#endif
#if HALMAC_PCIE_SUPPORT
#include "halmac_pcie_88xx_v1.h"
#endif

#if HALMAC_88XX_V1_SUPPORT

#define ORIGINAL_H2C_CMD_SIZE	8

#define WLHDR_PROT_VER	0

#define WLHDR_TYPE_MGMT		0
#define WLHDR_TYPE_CTRL		1
#define WLHDR_TYPE_DATA		2

/* mgmt frame */
#define WLHDR_SUB_TYPE_ASSOC_REQ	0
#define WLHDR_SUB_TYPE_ASSOC_RSPNS	1
#define WLHDR_SUB_TYPE_REASSOC_REQ	2
#define WLHDR_SUB_TYPE_REASSOC_RSPNS	3
#define WLHDR_SUB_TYPE_PROBE_REQ	4
#define WLHDR_SUB_TYPE_PROBE_RSPNS	5
#define WLHDR_SUB_TYPE_BCN		8
#define WLHDR_SUB_TYPE_DISASSOC		10
#define WLHDR_SUB_TYPE_AUTH		11
#define WLHDR_SUB_TYPE_DEAUTH		12
#define WLHDR_SUB_TYPE_ACTION		13
#define WLHDR_SUB_TYPE_ACTION_NOACK	14

/* ctrl frame */
#define WLHDR_SUB_TYPE_BF_RPT_POLL	4
#define WLHDR_SUB_TYPE_NDPA		5

/* data frame */
#define WLHDR_SUB_TYPE_DATA		0
#define WLHDR_SUB_TYPE_NULL		4
#define WLHDR_SUB_TYPE_QOS_DATA		8
#define WLHDR_SUB_TYPE_QOS_NULL		12

struct wlhdr_frame_ctrl {
	u16 protocol:2;
	u16 type:2;
	u16 sub_type:4;
	u16 to_ds:1;
	u16 from_ds:1;
	u16 more_frag:1;
	u16 retry:1;
	u16 pwr_mgmt:1;
	u16 more_data:1;
	u16 protect_frame:1;
	u16 order:1;
};

static enum halmac_ret_status
parse_c2h_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size);

static enum halmac_ret_status
get_c2h_dbg_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size);

static enum halmac_ret_status
get_h2c_ack_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size);

static enum halmac_ret_status
get_scan_ch_notify_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size);

static enum halmac_ret_status
get_scan_rpt_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size);

static enum halmac_ret_status
get_h2c_ack_cfg_param_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			      u32 size);

static enum halmac_ret_status
get_h2c_ack_update_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			       u32 size);

static enum halmac_ret_status
get_h2c_ack_send_scan_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
				  u32 size);

static enum halmac_ret_status
get_h2c_ack_drop_scan_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
				  u32 size);

static enum halmac_ret_status
get_h2c_ack_update_datapkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
				   u32 size);

static enum halmac_ret_status
get_h2c_ack_run_datapkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
				u32 size);

static enum halmac_ret_status
get_h2c_ack_ch_switch_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			      u32 size);

static enum halmac_cmd_construct_state
cfg_param_cmd_cnstr_state_88xx_v1(struct halmac_adapter *adapter);

static enum halmac_ret_status
proc_cfg_param_88xx_v1(struct halmac_adapter *adapter,
		       struct halmac_phy_parameter_info *param, u8 full_fifo);

static enum halmac_ret_status
read_buf_88xx_v1(struct halmac_adapter *adapter, u32 offset, u32 size,
		 enum hal_fifo_sel sel, u8 *data);

static enum halmac_ret_status
get_cfg_param_status_88xx_v1(struct halmac_adapter *adapter,
			     enum halmac_cmd_process_status *proc_status);

static enum halmac_ret_status
get_ch_switch_status_88xx_v1(struct halmac_adapter *adapter,
			     enum halmac_cmd_process_status *proc_status);

static enum halmac_ret_status
get_update_packet_status_88xx_v1(struct halmac_adapter *adapter,
				 enum halmac_cmd_process_status *proc_status);

static enum halmac_ret_status
get_send_scan_packet_status_88xx_v1(struct halmac_adapter *adapter,
				    enum halmac_cmd_process_status *status);

static enum halmac_ret_status
get_drop_scan_packet_status_88xx_v1(struct halmac_adapter *adapter,
				    enum halmac_cmd_process_status *status);

static enum halmac_ret_status
pwr_sub_seq_parser_88xx_v1(struct halmac_adapter *adapter, u8 cut, u8 intf,
			   struct halmac_wlan_pwr_cfg *cmd);

static void
pwr_state_88xx_v1(struct halmac_adapter *adapter, enum halmac_mac_power *state);

static enum halmac_ret_status
pwr_cmd_polling_88xx_v1(struct halmac_adapter *adapter,
			struct halmac_wlan_pwr_cfg *cmd);

static void
get_ch_map_88xx_v1(struct halmac_adapter *adapter,
		   struct halmac_rqpn_ch_map *mapping);

static enum halmac_ret_status
wlhdr_valid_88xx_v1(struct halmac_adapter *adapter, u8 *buf);

static u8
wlhdr_mgmt_valid_88xx_v1(struct halmac_adapter *adapter,
			 struct wlhdr_frame_ctrl *wlhdr);

static u8
wlhdr_ctrl_valid_88xx_v1(struct halmac_adapter *adapter,
			 struct wlhdr_frame_ctrl *wlhdr);

static u8
wlhdr_data_valid_88xx_v1(struct halmac_adapter *adapter,
			 struct wlhdr_frame_ctrl *wlhdr);

/**
 * ofld_func_cfg_88xx_v1() - config offload function
 * @adapter : the adapter of halmac
 * @info : offload function information
 * Author : Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
ofld_func_cfg_88xx_v1(struct halmac_adapter *adapter,
		      struct halmac_ofld_func_info *info)
{
	adapter->pltfm_info.malloc_size = info->halmac_malloc_max_sz;
	adapter->pltfm_info.rsvd_pg_size = info->rsvd_pg_drv_buf_max_sz;

	return HALMAC_RET_SUCCESS;
}

/**
 * dl_drv_rsvd_page_88xx_v1() - download packet to rsvd page
 * @adapter : the adapter of halmac
 * @pg_offset : page offset of driver's rsvd page
 * @halmac_buf : data to be downloaded, tx_desc is not included
 * @halmac_size : data size to be downloaded
 * Author : KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
dl_drv_rsvd_page_88xx_v1(struct halmac_adapter *adapter, u8 pg_offset, u8 *buf,
			 u32 size)
{
	enum halmac_ret_status status;
	u32 pg_size;
	u16 pg_addr = 0;
	u32 pg_num = 0;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	pg_size = adapter->hw_cfg_info.page_size;
	pg_num = size / pg_size + ((size & (pg_size - 1)) ? 1 : 0);
	if (pg_offset + pg_num > adapter->txff_alloc.rsvd_drv_pg_num) {
		PLTFM_MSG_ERR("[ERR] pkt overflow!!\n");
		return HALMAC_RET_DRV_DL_ERR;
	}

	pg_addr = adapter->txff_alloc.rsvd_drv_addr + pg_offset;

	status = dl_rsvd_page_88xx_v1(adapter, pg_addr, buf, size);

	if (status != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]dl rsvd page fail!!\n");
		return status;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
dl_rsvd_page_88xx_v1(struct halmac_adapter *adapter, u16 pg_addr, u8 *buf,
		     u32 size)
{
	u8 restore[3];
	u8 value8;
	u16 rsvd_pg_head;
	u32 cnt;
	enum halmac_rsvd_pg_state *state = &adapter->halmac_state.rsvd_pg_state;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	if (size == 0) {
		PLTFM_MSG_TRACE("[TRACE]Rsvd page packet size is zero!!\n");
		return HALMAC_RET_ZERO_LEN_RSVD_PACKET;
	}

	if (*state == HALMAC_RSVD_PG_STATE_BUSY)
		return HALMAC_RET_BUSY_STATE;

	*state = HALMAC_RSVD_PG_STATE_BUSY;

	value8 = HALMAC_REG_R8(REG_AUTO_LLT_V1 + 2);
	restore[0] = value8;
	value8 &= 0x0F;
	HALMAC_REG_W8(REG_AUTO_LLT_V1 + 2, value8);

	pg_addr &= BIT_MASK_BCN0_HEAD;
	HALMAC_REG_W16(REG_BCN_CTRL_0, (u16)(pg_addr | BIT(15)));

	value8 = HALMAC_REG_R8(REG_CR + 1);
	restore[1] = value8;
	value8 = (u8)(value8 | BIT(0));
	HALMAC_REG_W8(REG_CR + 1, value8);

	value8 = HALMAC_REG_R8(REG_FWHW_TXQ_CTRL + 1);
	restore[2] = value8;
	value8 = (u8)(value8 & ~(BIT(0)));
	HALMAC_REG_W8(REG_FWHW_TXQ_CTRL + 1, value8);

	if (PLTFM_SEND_RSVD_PAGE(buf, size) == 0) {
		PLTFM_MSG_ERR("[ERR]send rvsd pg(pltfm)!!\n");
		status = HALMAC_RET_DL_RSVD_PAGE_FAIL;
		goto DL_RSVD_PG_END;
	}

	cnt = 1000;
	while (!(HALMAC_REG_R8(REG_BCN_CTRL_0 + 1) & BIT(7))) {
		PLTFM_DELAY_US(10);
		cnt--;
		if (cnt == 0) {
			PLTFM_MSG_ERR("[ERR]bcn valid!!\n");
			status = HALMAC_RET_POLLING_BCN_VALID_FAIL;
			break;
		}
	}
DL_RSVD_PG_END:
	rsvd_pg_head = adapter->txff_alloc.rsvd_boundary;
	HALMAC_REG_W16(REG_BCN_CTRL_0, rsvd_pg_head | BIT(15));
	HALMAC_REG_W8(REG_FWHW_TXQ_CTRL + 1, restore[2]);
	HALMAC_REG_W8(REG_CR + 1, restore[1]);
	HALMAC_REG_W8(REG_AUTO_LLT_V1 + 2, restore[0]);

	*state = HALMAC_RSVD_PG_STATE_IDLE;

	return status;
}

/**
 * get_hw_value_88xx_v1() -get hw config value
 * @adapter : the adapter of halmac
 * @hw_id : hw id for driver to query
 * @value : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
get_hw_value_88xx_v1(struct halmac_adapter *adapter, enum halmac_hw_id hw_id,
		     void *value)
{
	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (!value) {
		PLTFM_MSG_ERR("[ERR]null pointer\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (hw_id) {
	case HALMAC_HW_RQPN_CH_MAPPING:
		get_ch_map_88xx_v1(adapter, (struct halmac_rqpn_ch_map *)value);
		break;
	case HALMAC_HW_EFUSE_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.efuse_size;
		break;
	case HALMAC_HW_EEPROM_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.eeprom_size;
		break;
	case HALMAC_HW_BT_BANK_EFUSE_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.bt_efuse_size;
		break;
	case HALMAC_HW_BT_BANK1_EFUSE_SIZE:
	case HALMAC_HW_BT_BANK2_EFUSE_SIZE:
		*(u32 *)value = 0;
		break;
	case HALMAC_HW_TXFIFO_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.tx_fifo_size;
		break;
	case HALMAC_HW_RXFIFO_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.rx_fifo_size;
		break;
	case HALMAC_HW_RSVD_PG_BNDY:
		*(u16 *)value = adapter->txff_alloc.rsvd_drv_addr;
		break;
	case HALMAC_HW_CAM_ENTRY_NUM:
		*(u8 *)value = adapter->hw_cfg_info.cam_entry_num;
		break;
	case HALMAC_HW_WLAN_EFUSE_AVAILABLE_SIZE:
		get_efuse_available_size_88xx_v1(adapter, (u32 *)value);
		break;
	case HALMAC_HW_IC_VERSION:
		*(u8 *)value = adapter->chip_ver;
		break;
	case HALMAC_HW_PAGE_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.page_size;
		break;
	case HALMAC_HW_TX_AGG_ALIGN_SIZE:
		*(u16 *)value = adapter->hw_cfg_info.tx_align_size;
		break;
	case HALMAC_HW_RX_AGG_ALIGN_SIZE:
		*(u8 *)value = 8;
		break;
	case HALMAC_HW_DRV_INFO_SIZE:
		*(u8 *)value = adapter->drv_info_size;
		break;
	case HALMAC_HW_TXFF_ALLOCATION:
		PLTFM_MEMCPY(value, &adapter->txff_alloc,
			     sizeof(struct halmac_txff_allocation));
		break;
	case HALMAC_HW_RSVD_EFUSE_SIZE:
		*(u32 *)value = get_rsvd_efuse_size_88xx_v1(adapter);
		break;
	case HALMAC_HW_FW_HDR_SIZE:
		*(u32 *)value = WLAN_FW_HDR_SIZE;
		break;
	case HALMAC_HW_TX_DESC_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.txdesc_size;
		break;
	case HALMAC_HW_RX_DESC_SIZE:
		*(u32 *)value = adapter->hw_cfg_info.rxdesc_size;
		break;
	case HALMAC_HW_FW_MAX_SIZE:
		*(u32 *)value = WLAN_FW_MAX_SIZE_88XX_V1;
		break;
	case HALMAC_HW_ORI_H2C_SIZE:
		*(u32 *)value = ORIGINAL_H2C_CMD_SIZE;
		break;
	case HALMAC_HW_RSVD_DRV_PGNUM:
		*(u16 *)value = adapter->txff_alloc.rsvd_drv_pg_num;
		break;
	case HALMAC_HW_TX_PAGE_SIZE:
		*(u16 *)value = TX_PAGE_SIZE_88XX_V1;
		break;
	case HALMAC_HW_USB_TXAGG_DESC_NUM:
		*(u8 *)value = adapter->hw_cfg_info.usb_txagg_num;
		break;
	case HALMAC_HW_PWR_STATE:
		pwr_state_88xx_v1(adapter, (enum halmac_mac_power *)value);
		break;
	case HALMAC_HW_SDIO_INT_LAT:
		break;
	case HALMAC_HW_SDIO_CLK_CNT:
		break;
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

static void
get_ch_map_88xx_v1(struct halmac_adapter *adapter,
		   struct halmac_rqpn_ch_map *mapping)
{
	mapping->dma_map_vo = adapter->ch_map[HALMAC_PQ_MAP_VO];
	mapping->dma_map_vi = adapter->ch_map[HALMAC_PQ_MAP_VI];
	mapping->dma_map_be = adapter->ch_map[HALMAC_PQ_MAP_BE];
	mapping->dma_map_bk = adapter->ch_map[HALMAC_PQ_MAP_BK];
	mapping->dma_map_mg = adapter->ch_map[HALMAC_PQ_MAP_MG];
	mapping->dma_map_hi = adapter->ch_map[HALMAC_PQ_MAP_HI];
}

/**
 * set_hw_value_88xx_v1() -set hw config value
 * @adapter : the adapter of halmac
 * @hw_id : hw id for driver to config
 * @value : hw value, reference table to get data type
 * Author : KaiYuan Chang / Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
set_hw_value_88xx_v1(struct halmac_adapter *adapter, enum halmac_hw_id hw_id,
		     void *value)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (!value) {
		PLTFM_MSG_ERR("[ERR]null ptr-set hw value\n");
		return HALMAC_RET_NULL_POINTER;
	}

	switch (hw_id) {
#if HALMAC_USB_SUPPORT
	case HALMAC_HW_USB_MODE:
		status = set_usb_mode_88xx_v1(adapter,
					      *(enum halmac_usb_mode *)value);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		break;
#endif
	case HALMAC_HW_BANDWIDTH:
		cfg_bw_88xx_v1(adapter, *(enum halmac_bw *)value);
		break;
	case HALMAC_HW_CHANNEL:
		cfg_ch_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_PRI_CHANNEL_IDX:
		cfg_pri_ch_idx_88xx_v1(adapter,
				       *(enum halmac_pri_ch_idx *)value);
		break;
	case HALMAC_HW_EN_BB_RF:
		enable_bb_rf_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_AMPDU_CONFIG:
		cfg_ampdu_88xx_v1(adapter, (struct halmac_ampdu_config *)value);
		break;
	case HALMAC_HW_RX_SHIFT:
		rx_shift_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_TXDESC_CHECKSUM:
		tx_desc_chksum_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_RX_CLK_GATE:
		rx_clk_gate_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_RXGCK_FIFO:
		cfg_rxgck_fifo_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_RX_IGNORE:
		cfg_rx_ignore_88xx_v1(adapter,
				      (struct halmac_mac_rx_ignore_cfg *)value);
		break;
	case HALMAC_HW_FAST_EDCA:
		fast_edca_cfg_88xx_v1(adapter,
				      (struct halmac_fast_edca_cfg *)value);
		break;
	case HALMAC_HW_RTS_FULL_BW:
		rts_full_bw_88xx_v1(adapter, *(u8 *)value);
		break;
	case HALMAC_HW_SDIO_WT_EN:
		break;
	case HALMAC_HW_SDIO_CLK_MONITOR:
		break;
	default:
		return HALMAC_RET_PARA_NOT_SUPPORT;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
get_watcher_88xx_v1(struct halmac_adapter *adapter, enum halmac_watcher_sel sel,
		    void *value)
{
	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
set_h2c_pkt_hdr_88xx_v1(struct halmac_adapter *adapter, u8 *hdr,
			struct halmac_h2c_header_info *info, u16 *seq_num)
{
	PLTFM_MSG_TRACE("[TRACE]%s!!\n", __func__);

	FW_OFFLOAD_H2C_SET_TOTAL_LEN(hdr, 8 + info->content_size);
	FW_OFFLOAD_H2C_SET_SUB_CMD_ID(hdr, info->sub_cmd_id);

	FW_OFFLOAD_H2C_SET_CATEGORY(hdr, 0x01);
	FW_OFFLOAD_H2C_SET_CMD_ID(hdr, 0xFF);

	PLTFM_MUTEX_LOCK(&adapter->h2c_seq_mutex);
	FW_OFFLOAD_H2C_SET_SEQ_NUM(hdr, adapter->h2c_info.seq_num);
	*seq_num = adapter->h2c_info.seq_num;
	(adapter->h2c_info.seq_num)++;
	PLTFM_MUTEX_UNLOCK(&adapter->h2c_seq_mutex);

	if (info->ack == 1)
		FW_OFFLOAD_H2C_SET_ACK(hdr, 1);

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
send_h2c_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *pkt)
{
	u32 cnt = 100;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	while (adapter->h2c_info.buf_fs <= H2C_PKT_SIZE_88XX_V1) {
		get_h2c_buf_free_space_88xx_v1(adapter);
		cnt--;
		if (cnt == 0) {
			PLTFM_MSG_ERR("[ERR]h2c free space!!\n");
			return HALMAC_RET_H2C_SPACE_FULL;
		}
	}

	cnt = 100;
	do {
		if (PLTFM_SEND_H2C_PKT(pkt, H2C_PKT_SIZE_88XX_V1) == 1)
			break;

		cnt--;
		if (cnt == 0) {
			PLTFM_MSG_ERR("[ERR]pltfm - sned h2c pkt!!\n");
			return HALMAC_RET_SEND_H2C_FAIL;
		}
		PLTFM_DELAY_US(5);

	} while (1);

	adapter->h2c_info.buf_fs -= H2C_PKT_SIZE_88XX_V1;

	return status;
}

enum halmac_ret_status
get_h2c_buf_free_space_88xx_v1(struct halmac_adapter *adapter)
{
	u32 hw_wptr;
	u32 fw_rptr;
	struct halmac_h2c_info *info = &adapter->h2c_info;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	hw_wptr = HALMAC_REG_R32(REG_H2C_PKT_WRITEADDR) & 0x3FFFF;
	fw_rptr = HALMAC_REG_R32(REG_H2C_PKT_READADDR) & 0x3FFFF;

	if (hw_wptr >= fw_rptr)
		info->buf_fs = info->buf_size - (hw_wptr - fw_rptr);
	else
		info->buf_fs = fw_rptr - hw_wptr;

	return HALMAC_RET_SUCCESS;
}

/**
 * get_c2h_info_88xx_v1() - process halmac C2H packet
 * @adapter : the adapter of halmac
 * @buf : RX Packet pointer
 * @size : RX Packet size
 *
 * Note : Don't use any IO or DELAY in this API
 *
 * Author : KaiYuan Chang/Ivan Lin
 *
 * Used to process c2h packet info from RX path. After receiving the packet,
 * user need to call this api and pass the packet pointer.
 *
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
get_c2h_info_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	if (GET_RX_DESC_C2H(buf) == 1) {
		PLTFM_MSG_TRACE("[TRACE]Parse c2h pkt\n");

		status = parse_c2h_pkt_88xx_v1(adapter, buf, size);

		if (status != HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]Parse c2h pkt\n");
			return status;
		}
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
parse_c2h_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	u8 cmd_id;
	u8 sub_cmd_id;
	u8 *c2h_pkt = buf + adapter->hw_cfg_info.rxdesc_size;
	u32 c2h_size = size - adapter->hw_cfg_info.rxdesc_size;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	cmd_id = (u8)C2H_HDR_GET_CMD_ID(c2h_pkt);

	if (cmd_id != 0xFF) {
		PLTFM_MSG_TRACE("[TRACE]Not 0xFF cmd!!\n");
		return HALMAC_RET_C2H_NOT_HANDLED;
	}

	sub_cmd_id = (u8)C2H_HDR_GET_C2H_SUB_CMD_ID(c2h_pkt);

	switch (sub_cmd_id) {
	case C2H_SUB_CMD_ID_C2H_DBG:
		status = get_c2h_dbg_88xx_v1(adapter, c2h_pkt, c2h_size);
		break;
	case C2H_SUB_CMD_ID_H2C_ACK_HDR:
		status = get_h2c_ack_88xx_v1(adapter, c2h_pkt, c2h_size);
		break;
	case C2H_SUB_CMD_ID_BT_COEX_INFO:
		status = HALMAC_RET_C2H_NOT_HANDLED;
		break;
	case C2H_SUB_CMD_ID_SCAN_STATUS_RPT:
		status = get_scan_rpt_88xx_v1(adapter, c2h_pkt, c2h_size);
		break;
	case C2H_SUB_CMD_ID_PSD_DATA:
		status = get_psd_data_88xx_v1(adapter, c2h_pkt, c2h_size);
		break;

	case C2H_SUB_CMD_ID_EFUSE_DATA:
		status = get_efuse_data_88xx_v1(adapter, c2h_pkt, c2h_size);
		break;
	case C2H_SUB_CMD_ID_SCAN_CH_NOTIFY:
		status = get_scan_ch_notify_88xx_v1(adapter, c2h_pkt, c2h_size);
		break;
	default:
		PLTFM_MSG_WARN("[WARN]Sub cmd id!!\n");
		status = HALMAC_RET_C2H_NOT_HANDLED;
		break;
	}

	return status;
}

static enum halmac_ret_status
get_c2h_dbg_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_h2c_ack_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	u8 cmd_id;
	u8 sub_cmd_id;
	u8 fw_rc;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	PLTFM_MSG_TRACE("[TRACE]Ack for C2H!!\n");

	fw_rc = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(buf);
	if (HALMAC_H2C_RETURN_SUCCESS != (enum halmac_h2c_return_code)fw_rc)
		PLTFM_MSG_TRACE("[TRACE]fw rc = %d\n", fw_rc);

	cmd_id = (u8)H2C_ACK_HDR_GET_H2C_CMD_ID(buf);

	if (cmd_id != 0xFF) {
		PLTFM_MSG_ERR("[ERR]h2c ack cmd id!!\n");
		return HALMAC_RET_C2H_NOT_HANDLED;
	}

	sub_cmd_id = (u8)H2C_ACK_HDR_GET_H2C_SUB_CMD_ID(buf);

	switch (sub_cmd_id) {
	case H2C_SUB_CMD_ID_DUMP_PHYSICAL_EFUSE_ACK:
		status = get_h2c_ack_phy_efuse_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_CFG_PARAM_ACK:
		status = get_h2c_ack_cfg_param_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_UPDATE_PKT_ACK:
		status = get_h2c_ack_update_pkt_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_SEND_SCAN_PKT_ACK:
		status = get_h2c_ack_send_scan_pkt_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_DROP_SCAN_PKT_ACK:
		status = get_h2c_ack_drop_scan_pkt_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_UPDATE_DATAPACK_ACK:
		status = get_h2c_ack_update_datapkt_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_RUN_DATAPACK_ACK:
		status = get_h2c_ack_run_datapkt_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_CH_SWITCH_ACK:
		status = get_h2c_ack_ch_switch_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_IQK_ACK:
		status = get_h2c_ack_iqk_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_PWR_TRK_ACK:
		status = get_h2c_ack_pwr_trk_88xx_v1(adapter, buf, size);
		break;
	case H2C_SUB_CMD_ID_PSD_ACK:
		break;
	default:
		status = HALMAC_RET_C2H_NOT_HANDLED;
		break;
	}

	return status;
}

static enum halmac_ret_status
get_scan_ch_notify_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	struct halmac_scan_rpt_info *scan_rpt_info = &adapter->scan_rpt_info;

	if (adapter->ch_sw_info.scan_mode == 1) {
		if (scan_rpt_info->avl_buf_size < 12) {
			PLTFM_MSG_ERR("[ERR]ch_notify buffer full!!\n");
			return HALMAC_RET_CH_SW_NO_BUF;
		}

		SCAN_CH_NOTIFY_SET_CH_NUM(scan_rpt_info->buf_wptr,
					  (u8)SCAN_CH_NOTIFY_GET_CH_NUM(buf));
		SCAN_CH_NOTIFY_SET_NOTIFY_ID(scan_rpt_info->buf_wptr,
					     SCAN_CH_NOTIFY_GET_NOTIFY_ID(buf));
		SCAN_CH_NOTIFY_SET_STATUS(scan_rpt_info->buf_wptr,
					  (u8)SCAN_CH_NOTIFY_GET_STATUS(buf));
		SCAN_CH_NOTIFY_SET_TSF_0(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_0(buf));
		SCAN_CH_NOTIFY_SET_TSF_1(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_1(buf));
		SCAN_CH_NOTIFY_SET_TSF_2(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_2(buf));
		SCAN_CH_NOTIFY_SET_TSF_3(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_3(buf));
		SCAN_CH_NOTIFY_SET_TSF_4(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_4(buf));
		SCAN_CH_NOTIFY_SET_TSF_5(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_5(buf));
		SCAN_CH_NOTIFY_SET_TSF_6(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_6(buf));
		SCAN_CH_NOTIFY_SET_TSF_7(scan_rpt_info->buf_wptr,
					 (u8)SCAN_CH_NOTIFY_GET_TSF_7(buf));

		scan_rpt_info->avl_buf_size = scan_rpt_info->avl_buf_size - 12;
		scan_rpt_info->total_size = scan_rpt_info->total_size + 12;
		scan_rpt_info->buf_wptr = scan_rpt_info->buf_wptr + 12;
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_scan_rpt_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	u8 fw_rc;
	enum halmac_cmd_process_status proc_status;
	struct halmac_scan_rpt_info *scan_rpt_info = &adapter->scan_rpt_info;

	fw_rc = (u8)SCAN_STATUS_RPT_GET_H2C_RETURN_CODE(buf);
	proc_status = (HALMAC_H2C_RETURN_SUCCESS ==
		(enum halmac_h2c_return_code)fw_rc) ?
		HALMAC_CMD_PROCESS_DONE : HALMAC_CMD_PROCESS_ERROR;

	PLTFM_EVENT_SIG(HALMAC_FEATURE_CHANNEL_SWITCH, proc_status, NULL, 0);

	adapter->halmac_state.scan_state.proc_status = proc_status;

	if (adapter->ch_sw_info.scan_mode == 1) {
		scan_rpt_info->rpt_tsf_low =
			((SCAN_STATUS_RPT_GET_TSF_3(buf) << 24) |
			(SCAN_STATUS_RPT_GET_TSF_2(buf) << 16) |
			(SCAN_STATUS_RPT_GET_TSF_1(buf) << 8) |
			(SCAN_STATUS_RPT_GET_TSF_0(buf)));
		scan_rpt_info->rpt_tsf_high =
			((SCAN_STATUS_RPT_GET_TSF_7(buf) << 24) |
			(SCAN_STATUS_RPT_GET_TSF_6(buf) << 16) |
			(SCAN_STATUS_RPT_GET_TSF_5(buf) << 8) |
			(SCAN_STATUS_RPT_GET_TSF_4(buf)));
	}

	PLTFM_MSG_TRACE("[TRACE]scan : %X\n", proc_status);

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_h2c_ack_cfg_param_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			      u32 size)
{
	u8 seq_num;
	u8 fw_rc;
	u32 offset_accum;
	u32 value_accum;
	struct halmac_cfg_param_state *state =
		&adapter->halmac_state.cfg_param_state;
	enum halmac_cmd_process_status proc_status =
		HALMAC_CMD_PROCESS_UNDEFINE;

	seq_num = (u8)H2C_ACK_HDR_GET_H2C_SEQ(buf);
	PLTFM_MSG_TRACE("[TRACE]Seq num : h2c->%d c2h->%d\n",
			state->seq_num, seq_num);
	if (seq_num != state->seq_num) {
		PLTFM_MSG_ERR("[ERR]Seq num mismatch : h2c->%d c2h->%d\n",
			      state->seq_num, seq_num);
		return HALMAC_RET_SUCCESS;
	}

	if (state->proc_status != HALMAC_CMD_PROCESS_SENDING) {
		PLTFM_MSG_ERR("[ERR]not cmd sending\n");
		return HALMAC_RET_SUCCESS;
	}

	fw_rc = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(buf);
	state->fw_rc = fw_rc;
	offset_accum = CFG_PARAM_ACK_GET_OFFSET_ACCUMULATION(buf);
	value_accum = CFG_PARAM_ACK_GET_VALUE_ACCUMULATION(buf);

	if (offset_accum != adapter->cfg_param_info.offset_accum ||
	    value_accum != adapter->cfg_param_info.value_accum) {
		PLTFM_MSG_ERR("[ERR][C2H]offset_accu : %x, value_accu : %xn",
			      offset_accum, value_accum);
		PLTFM_MSG_ERR("[ERR][Ada]offset_accu : %x, value_accu : %x\n",
			      adapter->cfg_param_info.offset_accum,
			      adapter->cfg_param_info.value_accum);
		proc_status = HALMAC_CMD_PROCESS_ERROR;
	}

	if ((enum halmac_h2c_return_code)fw_rc == HALMAC_H2C_RETURN_SUCCESS &&
	    proc_status != HALMAC_CMD_PROCESS_ERROR) {
		proc_status = HALMAC_CMD_PROCESS_DONE;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_CFG_PARA, proc_status, NULL, 0);
	} else {
		proc_status = HALMAC_CMD_PROCESS_ERROR;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_CFG_PARA, proc_status,
				&fw_rc, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_h2c_ack_update_pkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			       u32 size)
{
	u8 seq_num;
	u8 fw_rc;
	struct halmac_update_pkt_state *state =
		&adapter->halmac_state.update_pkt_state;
	enum halmac_cmd_process_status proc_status;

	seq_num = (u8)H2C_ACK_HDR_GET_H2C_SEQ(buf);
	PLTFM_MSG_TRACE("[TRACE]Seq num : h2c->%d c2h->%d\n",
			state->seq_num, seq_num);
	if (seq_num != state->seq_num) {
		PLTFM_MSG_ERR("[ERR]Seq num mismatch : h2c->%d c2h->%d\n",
			      state->seq_num, seq_num);
		return HALMAC_RET_SUCCESS;
	}

	if (state->proc_status != HALMAC_CMD_PROCESS_SENDING) {
		PLTFM_MSG_ERR("[ERR]not cmd sending\n");
		return HALMAC_RET_SUCCESS;
	}

	fw_rc = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(buf);
	state->fw_rc = fw_rc;

	if (HALMAC_H2C_RETURN_SUCCESS == (enum halmac_h2c_return_code)fw_rc) {
		proc_status = HALMAC_CMD_PROCESS_DONE;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_UPDATE_PACKET, proc_status,
				NULL, 0);
	} else {
		proc_status = HALMAC_CMD_PROCESS_ERROR;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_UPDATE_PACKET, proc_status,
				&state->fw_rc, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_h2c_ack_send_scan_pkt_88xx_v1(struct halmac_adapter *adapter,
				  u8 *buf, u32 size)
{
	u8 seq_num;
	u8 fw_rc;
	struct halmac_scan_pkt_state *state =
		&adapter->halmac_state.scan_pkt_state;
	enum halmac_cmd_process_status proc_status;

	seq_num = (u8)H2C_ACK_HDR_GET_H2C_SEQ(buf);
	PLTFM_MSG_TRACE("[TRACE]Seq num : h2c->%d c2h->%d\n",
			state->seq_num, seq_num);
	if (seq_num != state->seq_num) {
		PLTFM_MSG_ERR("[ERR]Seq num mismatch : h2c->%d c2h->%d\n",
			      state->seq_num, seq_num);
		return HALMAC_RET_SUCCESS;
	}

	if (state->proc_status != HALMAC_CMD_PROCESS_SENDING) {
		PLTFM_MSG_ERR("[ERR]not cmd sending\n");
		return HALMAC_RET_SUCCESS;
	}

	fw_rc = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(buf);
	state->fw_rc = fw_rc;

	if (HALMAC_H2C_RETURN_SUCCESS == (enum halmac_h2c_return_code)fw_rc) {
		proc_status = HALMAC_CMD_PROCESS_DONE;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_SEND_SCAN_PACKET, proc_status,
				NULL, 0);
	} else {
		proc_status = HALMAC_CMD_PROCESS_ERROR;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_SEND_SCAN_PACKET, proc_status,
				&state->fw_rc, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_h2c_ack_drop_scan_pkt_88xx_v1(struct halmac_adapter *adapter,
				  u8 *buf, u32 size)
{
	u8 seq_num;
	u8 fw_rc;
	struct halmac_drop_pkt_state *state =
		&adapter->halmac_state.drop_pkt_state;
	enum halmac_cmd_process_status proc_status;

	seq_num = (u8)H2C_ACK_HDR_GET_H2C_SEQ(buf);
	PLTFM_MSG_TRACE("[TRACE]Seq num : h2c->%d c2h->%d\n",
			state->seq_num, seq_num);
	if (seq_num != state->seq_num) {
		PLTFM_MSG_ERR("[ERR]Seq num mismatch : h2c->%d c2h->%d\n",
			      state->seq_num, seq_num);
		return HALMAC_RET_SUCCESS;
	}

	if (state->proc_status != HALMAC_CMD_PROCESS_SENDING) {
		PLTFM_MSG_ERR("[ERR]not cmd sending\n");
		return HALMAC_RET_SUCCESS;
	}

	fw_rc = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(buf);
	state->fw_rc = fw_rc;

	if (HALMAC_H2C_RETURN_SUCCESS == (enum halmac_h2c_return_code)fw_rc) {
		proc_status = HALMAC_CMD_PROCESS_DONE;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_DROP_SCAN_PACKET, proc_status,
				NULL, 0);
	} else {
		proc_status = HALMAC_CMD_PROCESS_ERROR;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_DROP_SCAN_PACKET, proc_status,
				&state->fw_rc, 1);
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_h2c_ack_update_datapkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
				   u32 size)
{
	return HALMAC_RET_NOT_SUPPORT;
}

static enum halmac_ret_status
get_h2c_ack_run_datapkt_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
				u32 size)
{
	return HALMAC_RET_NOT_SUPPORT;
}

static enum halmac_ret_status
get_h2c_ack_ch_switch_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			      u32 size)
{
	u8 seq_num;
	u8 fw_rc;
	struct halmac_scan_state *state = &adapter->halmac_state.scan_state;
	struct halmac_scan_rpt_info *scan_rpt_info = &adapter->scan_rpt_info;
	enum halmac_cmd_process_status proc_status;

	seq_num = (u8)H2C_ACK_HDR_GET_H2C_SEQ(buf);
	PLTFM_MSG_TRACE("[TRACE]Seq num : h2c->%d c2h->%d\n",
			state->seq_num, seq_num);
	if (seq_num != state->seq_num) {
		PLTFM_MSG_ERR("[ERR]Seq num mismatch : h2c->%d c2h->%d\n",
			      state->seq_num, seq_num);
		return HALMAC_RET_SUCCESS;
	}

	if (state->proc_status != HALMAC_CMD_PROCESS_SENDING) {
		PLTFM_MSG_ERR("[ERR]not cmd sending\n");
		return HALMAC_RET_SUCCESS;
	}

	fw_rc = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(buf);
	state->fw_rc = fw_rc;

	if (adapter->ch_sw_info.scan_mode == 1) {
		scan_rpt_info->ack_tsf_low =
			((CH_SWITCH_ACK_GET_TSF_3(buf) << 24) |
			(CH_SWITCH_ACK_GET_TSF_2(buf) << 16) |
			(CH_SWITCH_ACK_GET_TSF_1(buf) << 8) |
			(CH_SWITCH_ACK_GET_TSF_0(buf)));
		scan_rpt_info->ack_tsf_high =
			((CH_SWITCH_ACK_GET_TSF_7(buf) << 24) |
			(CH_SWITCH_ACK_GET_TSF_6(buf) << 16) |
			(CH_SWITCH_ACK_GET_TSF_5(buf) << 8) |
			(CH_SWITCH_ACK_GET_TSF_4(buf)));
	}

	if ((enum halmac_h2c_return_code)fw_rc == HALMAC_H2C_RETURN_SUCCESS) {
		proc_status = HALMAC_CMD_PROCESS_RCVD;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_CHANNEL_SWITCH, proc_status,
				NULL, 0);
	} else {
		proc_status = HALMAC_CMD_PROCESS_ERROR;
		state->proc_status = proc_status;
		PLTFM_EVENT_SIG(HALMAC_FEATURE_CHANNEL_SWITCH, proc_status,
				&fw_rc, 1);
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * mac_debug_88xx_v1() - dump debug information
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
mac_debug_88xx_v1(struct halmac_adapter *adapter)
{
	u32 tmp32;
	u32 i;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	/*Dump MAC Register*/
	for (i = 0x0000; i < 0x17fc; i += 4) {
		tmp32 = HALMAC_REG_R32(i);
		PLTFM_MSG_TRACE("[TRACE]dbg-mac[%x]=%x\n", i, tmp32);
	}

	/*Check RX Fifo status*/
	i = REG_RXFF_PTR_V1;
	tmp32 = HALMAC_REG_R32(i);
	PLTFM_MSG_TRACE("[TRACE]dbg-mac[%x]=%x\n", i, tmp32);
	i = REG_RXFF_WTR_V1;
	tmp32 = HALMAC_REG_R32(i);
	PLTFM_MSG_TRACE("[TRACE]dbg-mac[%x]=%x\n", i, tmp32);
	i = REG_RXFF_PTR_V1;
	tmp32 = HALMAC_REG_R32(i);
	PLTFM_MSG_TRACE("[TRACE]dbg-mac[%x]=%x\n", i, tmp32);
	i = REG_RXFF_WTR_V1;
	tmp32 = HALMAC_REG_R32(i);
	PLTFM_MSG_TRACE("[TRACE]dbg-mac[%x]=%x\n", i, tmp32);
	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * cfg_parameter_88xx_v1() - config parameter by FW
 * @adapter : the adapter of halmac
 * @info : cmd id, content
 * @full_fifo : parameter information
 *
 * If msk_en = 1, the format of array is {reg_info, mask, value}.
 * If msk_en =_FAUSE, the format of array is {reg_info, value}
 * The format of reg_info is
 * reg_info[31]=rf_reg, 0: MAC_BB reg, 1: RF reg
 * reg_info[27:24]=rf_path, 0: path_A, 1: path_B
 * if rf_reg=0(MAC_BB reg), rf_path is meaningless.
 * ref_info[15:0]=offset
 *
 * Example: msk_en = 0
 * {0x8100000a, 0x00001122}
 * =>Set RF register, path_B, offset 0xA to 0x00001122
 * {0x00000824, 0x11224433}
 * =>Set MAC_BB register, offset 0x800 to 0x11224433
 *
 * Note : full fifo mode only for init flow
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
cfg_parameter_88xx_v1(struct halmac_adapter *adapter,
		      struct halmac_phy_parameter_info *info, u8 full_fifo)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	enum halmac_cmd_process_status *proc_status;
	enum halmac_cmd_construct_state cmd_state;

	proc_status = &adapter->halmac_state.cfg_param_state.proc_status;

	if (halmac_fw_validate(adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (adapter->fw_ver.h2c_version < 4)
		return HALMAC_RET_FW_NO_SUPPORT;

	if (*proc_status == HALMAC_CMD_PROCESS_SENDING) {
		PLTFM_MSG_TRACE("[TRACE]Wait event(para)\n");
		return HALMAC_RET_BUSY_STATE;
	}

	cmd_state = cfg_param_cmd_cnstr_state_88xx_v1(adapter);
	if (cmd_state != HALMAC_CMD_CNSTR_IDLE &&
	    cmd_state != HALMAC_CMD_CNSTR_CNSTR) {
		PLTFM_MSG_TRACE("[TRACE]Not idle(para)\n");
		return HALMAC_RET_BUSY_STATE;
	}

	*proc_status = HALMAC_CMD_PROCESS_IDLE;

	status = proc_cfg_param_88xx_v1(adapter, info, full_fifo);

	if (status != HALMAC_RET_SUCCESS && status != HALMAC_RET_PARA_SENDING) {
		PLTFM_MSG_ERR("[ERR]send param h2c\n");
		return status;
	}

	return status;
}

static enum halmac_cmd_construct_state
cfg_param_cmd_cnstr_state_88xx_v1(struct halmac_adapter *adapter)
{
	return adapter->halmac_state.cfg_param_state.cmd_cnstr_state;
}

static enum halmac_ret_status
proc_cfg_param_88xx_v1(struct halmac_adapter *adapter,
		       struct halmac_phy_parameter_info *param, u8 full_fifo)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * update_packet_88xx_v1() - send specific packet to FW
 * @adapter : the adapter of halmac
 * @pkt_id : packet id, to know the purpose of this packet
 * @pkt : packet
 * @size : packet size
 *
 * Note : TX_DESC is not included in the pkt
 *
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
update_packet_88xx_v1(struct halmac_adapter *adapter,
		      enum halmac_packet_id pkt_id, u8 *pkt, u32 size)
{
	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
send_scan_packet_88xx_v1(struct halmac_adapter *adapter,
			 u8 index, u8 *pkt, u32 size)
{
	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
drop_scan_packet_88xx_v1(struct halmac_adapter *adapter,
			 struct halmac_drop_pkt_option *option)
{
	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
bcn_ie_filter_88xx_v1(struct halmac_adapter *adapter,
		      struct halmac_bcn_ie_info *info)
{
	return HALMAC_RET_NOT_SUPPORT;
}

enum halmac_ret_status
update_datapack_88xx_v1(struct halmac_adapter *adapter,
			enum halmac_data_type data_type,
			struct halmac_phy_parameter_info *info)
{
	return HALMAC_RET_NOT_SUPPORT;
}

enum halmac_ret_status
run_datapack_88xx_v1(struct halmac_adapter *adapter,
		     enum halmac_data_type data_type)
{
	return HALMAC_RET_NOT_SUPPORT;
}

enum halmac_ret_status
send_bt_coex_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size, u8 ack)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * fill_txdesc_check_sum_88xx_v1() -  fill in tx desc check sum
 * @adapter : the adapter of halmac
 * @txdesc : tx desc packet
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
fill_txdesc_check_sum_88xx_v1(struct halmac_adapter *adapter, u8 *txdesc)
{
	__le16 chksum = 0;
	u16 txdesc_size;
	__le16 *data;
	u32 i;

	if (!txdesc) {
		PLTFM_MSG_ERR("[ERR]null pointer\n");
		return HALMAC_RET_NULL_POINTER;
	}

	if (adapter->tx_desc_checksum != 1)
		PLTFM_MSG_TRACE("[TRACE]chksum disable\n");

	data = (__le16 *)(txdesc);

	if (adapter->tx_desc_transfer == 1) {
		SET_TX_DESC_TXDESC_CHECKSUM_V1(txdesc, 0x0000);

		txdesc_size = (u16)((GET_TX_DESC_PKT_OFFSET_V1(txdesc) +
					(TX_DESC_BODY_SIZE_88XX_V1 >> 3)) << 1);
		for (i = 0; i < txdesc_size; i++)
			chksum ^= (*(data + 2 * i) ^ *(data + (2 * i + 1)));

		SET_TX_DESC_TXDESC_CHECKSUM_V1(txdesc, rtk_le16_to_cpu(chksum));
	} else {
		SET_TX_DESC_BUFFER_TXDESC_CHECKSUM(txdesc, 0x0000);

		txdesc_size = (u16)((GET_TX_DESC_BUFFER_PKT_OFFSET(txdesc) +
					(TX_DESC_BODY_SIZE_88XX_V1 >> 3)) << 1);
		for (i = 0; i < txdesc_size; i++)
			chksum ^= (*(data + 2 * i) ^ *(data + (2 * i + 1)));

		SET_TX_DESC_BUFFER_TXDESC_CHECKSUM(txdesc,
						   rtk_le16_to_cpu(chksum));
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * dump_fifo_88xx_v1() - dump fifo data
 * @adapter : the adapter of halmac
 * @sel : FIFO selection
 * @start_addr : start address of selected FIFO
 * @size : dump size of selected FIFO
 * @data : FIFO data
 *
 * Note : before dump fifo, user need to call halmac_get_fifo_size to
 * get fifo size. Then input this size to halmac_dump_fifo.
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
dump_fifo_88xx_v1(struct halmac_adapter *adapter, enum hal_fifo_sel sel,
		  u32 start_addr, u32 size, u8 *data)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	u8 tmp8;
	u8 enable;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (sel == HAL_FIFO_SEL_TX &&
	    (start_addr + size) > adapter->hw_cfg_info.tx_fifo_size) {
		PLTFM_MSG_ERR("[ERR]size overflow!!\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if (sel == HAL_FIFO_SEL_RX &&
	    (start_addr + size) > adapter->hw_cfg_info.rx_fifo_size) {
		PLTFM_MSG_ERR("[ERR]size overflow!!\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if ((size & (4 - 1)) != 0) {
		PLTFM_MSG_ERR("[ERR]not 4byte alignment!!\n");
		return HALMAC_RET_DUMP_FIFOSIZE_INCORRECT;
	}

	if (!data)
		return HALMAC_RET_NULL_POINTER;

	tmp8 = HALMAC_REG_R8(REG_RCR + 2);
	enable = 0;
	status = api->halmac_set_hw_value(adapter, HALMAC_HW_RX_CLK_GATE,
					  &enable);
	if (status != HALMAC_RET_SUCCESS)
		return status;
	status = read_buf_88xx_v1(adapter, start_addr, size, sel, data);

	HALMAC_REG_W8(REG_RCR + 2, tmp8);

	if (status != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]read_buf_88xx error = %x\n", status);
		return status;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
read_buf_88xx_v1(struct halmac_adapter *adapter, u32 offset, u32 size,
		 enum hal_fifo_sel sel, u8 *data)
{
	u8 value8;
	u32 start_pg;
	u32 value32;
	u32 i;
	u32 residue;
	u32 cnt = 0;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	if (sel == HAL_FIFO_SEL_RSVD_PAGE)
		offset += (adapter->txff_alloc.rsvd_boundary <<
			   TX_PAGE_SIZE_SHIFT_88XX_V1);

	if (sel == HAL_FIFO_SEL_RXBUF_PHY)
		offset += RX_BUF_FW_88XX_V1;

	start_pg = offset >> 12;
	residue = offset & (4096 - 1);

	value8 = HALMAC_REG_R8(REG_CAM_ACCESS_CTRL);
	HALMAC_REG_W8(REG_CAM_ACCESS_CTRL, 0x1F);

	if (sel == HAL_FIFO_SEL_TX || sel == HAL_FIFO_SEL_RSVD_PAGE)
		start_pg += 0x780;
	else if (sel == HAL_FIFO_SEL_RX)
		start_pg += 0x700;
	else if (sel == HAL_FIFO_SEL_REPORT)
		start_pg += 0x660;
	else if (sel == HAL_FIFO_SEL_LLT)
		start_pg += 0x650;
	else if (sel == HAL_FIFO_SEL_RXDESC)
		start_pg += 0x770;
	else if (sel == HAL_BUF_SECURITY_CAM)
		start_pg += 0x670;
	else if (sel == HAL_BUF_WOW_CAM)
		start_pg += 0x674;
	else if (sel == HAL_BUF_RX_FILTER_CAM)
		start_pg += 0x676;
	else if (sel == HAL_BUF_BA_CAM)
		start_pg += 0x677;
	else if (sel == HAL_BUF_MBSSID_CAM)
		start_pg += 0x678;
	else if (sel == HAL_FIFO_SEL_RXBUF_FW || sel == HAL_FIFO_SEL_RXBUF_PHY)
		start_pg += 0x680;
	else
		return HALMAC_RET_NOT_SUPPORT;

	value32 = HALMAC_REG_R16(REG_PKTBUF_DBG_CTRL) & 0xF000;

	do {
		HALMAC_REG_W16(REG_PKTBUF_DBG_CTRL, (u16)(start_pg | value32));

		for (i = 0x8000 + residue; i <= 0x8FFF; i += 4) {
			*(u32 *)(data + cnt) = HALMAC_REG_R32(i);
			*(u32 *)(data + cnt) =
				rtk_le32_to_cpu(*(u32 *)(data + cnt));
			cnt += 4;
			if (size == cnt)
				goto HALMAC_BUF_READ_OK;
		}

		residue = 0;
		start_pg++;
	} while (1);

HALMAC_BUF_READ_OK:
	HALMAC_REG_W16(REG_PKTBUF_DBG_CTRL, (u16)value32);
	HALMAC_REG_W8(REG_CAM_ACCESS_CTRL, value8);

	return HALMAC_RET_SUCCESS;
}

/**
 * get_fifo_size_88xx_v1() - get fifo size
 * @adapter : the adapter of halmac
 * @sel : FIFO selection
 * Author : Ivan Lin/KaiYuan Chang
 * Return : u32
 * More details of status code can be found in prototype document
 */
u32
get_fifo_size_88xx_v1(struct halmac_adapter *adapter, enum hal_fifo_sel sel)
{
	u32 fifo_size = 0;

	if (sel == HAL_FIFO_SEL_TX)
		fifo_size = adapter->hw_cfg_info.tx_fifo_size;
	else if (sel == HAL_FIFO_SEL_RX)
		fifo_size = adapter->hw_cfg_info.rx_fifo_size;
	else if (sel == HAL_FIFO_SEL_RSVD_PAGE)
		fifo_size = adapter->hw_cfg_info.tx_fifo_size -
			    (adapter->txff_alloc.rsvd_boundary <<
			     TX_PAGE_SIZE_SHIFT_88XX_V1);
	else if (sel == HAL_FIFO_SEL_REPORT)
		fifo_size = 65536;
	else if (sel == HAL_FIFO_SEL_LLT)
		fifo_size = 65536;
	else if (sel == HAL_FIFO_SEL_RXDESC)
		fifo_size = adapter->hw_cfg_info.rx_desc_fifo_size;
	else if (sel == HAL_BUF_SECURITY_CAM)
		fifo_size = 16384;
	else if (sel == HAL_BUF_WOW_CAM)
		fifo_size = 8192;
	else if (sel == HAL_BUF_RX_FILTER_CAM)
		fifo_size = 4096;
	else if (sel == HAL_BUF_BA_CAM)
		fifo_size = 4096;
	else if (sel == HAL_BUF_MBSSID_CAM)
		fifo_size = 32768;
	else if (sel == HAL_FIFO_SEL_RXBUF_FW)
		fifo_size = RX_BUF_FW_88XX_V1;
	else if (sel == HAL_FIFO_SEL_RXBUF_PHY)
		fifo_size = RX_BUF_PHY_88XX_V1;

	return fifo_size;
}

/**
 * add_ch_info_88xx_v1() -add channel information
 * @adapter : the adapter of halmac
 * @info : channel information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
add_ch_info_88xx_v1(struct halmac_adapter *adapter,
		    struct halmac_ch_info *info)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * add_extra_ch_info_88xx_v1() -add extra channel information
 * @adapter : the adapter of halmac
 * @info : extra channel information
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
add_extra_ch_info_88xx_v1(struct halmac_adapter *adapter,
			  struct halmac_ch_extra_info *info)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * ctrl_ch_switch_88xx_v1() -send channel switch cmd
 * @adapter : the adapter of halmac
 * @opt : channel switch config
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
ctrl_ch_switch_88xx_v1(struct halmac_adapter *adapter,
		       struct halmac_ch_switch_option *info)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * clear_ch_info_88xx_v1() -clear channel information
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
clear_ch_info_88xx_v1(struct halmac_adapter *adapter)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * chk_txdesc_88xx_v1() -check if the tx packet format is incorrect
 * @adapter : the adapter of halmac
 * @buf : tx Packet buffer, tx desc is included
 * @size : tx packet size
 * Author : KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
chk_txdesc_88xx_v1(struct halmac_adapter *adapter, u8 *buf, u32 size)
{
	u32 mac_clk = 0;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	struct halmac_txdesc_ie_param param;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	u8 max_num = adapter->hw_cfg_info.txdesc_ie_max_num;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (adapter->tx_desc_transfer == 0)
		return HALMAC_RET_NOT_SUPPORT;

	param.ie_offset = (u8 *)PLTFM_MALLOC(max_num);
	param.ie_exist = (u8 *)PLTFM_MALLOC(max_num);
	PLTFM_MEMSET(param.ie_offset, 0xFF, max_num);
	PLTFM_MEMSET(param.ie_exist, 0, max_num);
	param.start_offset = (u8 *)PLTFM_MALLOC(1);
	*param.start_offset = adapter->hw_cfg_info.txdesc_body_size;
	param.end_offset = (u8 *)PLTFM_MALLOC(1);
	*param.end_offset = adapter->hw_cfg_info.txdesc_body_size;
	if (parse_txdesc_ie_88xx_v1(adapter, buf, &param) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_FAIL;

	if (*(param.ie_exist + 1) == 1) {
		if (IE1_GET_TX_DESC_BMC(buf + *(param.ie_offset + 1)) == 1)
			if (GET_TX_DESC_AGG_EN_V1(buf) == 1)
				PLTFM_MSG_ERR("[ERR]txdesc - agg + bmc\n");
	}
	if (size < (GET_TX_DESC_TXPKTSIZE(buf) +
		    adapter->hw_cfg_info.txdesc_body_size +
		    (GET_TX_DESC_PKT_OFFSET_V1(buf) << 3))) {
		PLTFM_MSG_ERR("[ERR]txdesc - total size\n");
		status = HALMAC_RET_TXDESC_SET_FAIL;
	}

	if (wlhdr_valid_88xx_v1(adapter, buf) != HALMAC_RET_SUCCESS) {
		PLTFM_MSG_ERR("[ERR]wlhdr\n");
		status = HALMAC_RET_WLHDR_FAIL;
	}

	switch (BIT_GET_MAC_CLK_SEL(HALMAC_REG_R32(REG_AFE_CTRL1))) {
	case 0x0:
		mac_clk = 80;
		break;
	case 0x1:
		mac_clk = 40;
		break;
	case 0x2:
		mac_clk = 20;
		break;
	case 0x3:
		mac_clk = 10;
		break;
	}

	PLTFM_MSG_ALWAYS("MAC clock : 0x%XM\n", mac_clk);

	PLTFM_MSG_ALWAYS("mac agg en : 0x%X\n", GET_TX_DESC_AGG_EN_V1(buf));
	PLTFM_MSG_ALWAYS("mac agg num : 0x%X\n",
			 IE1_GET_TX_DESC_MAX_AGG_NUM(buf +
						*(param.ie_offset + 1)));
	PLTFM_FREE(param.ie_offset, max_num);
	PLTFM_FREE(param.ie_exist, max_num);
	PLTFM_FREE(param.start_offset, max_num);
	PLTFM_FREE(param.end_offset, max_num);

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return status;
}

static enum halmac_ret_status
wlhdr_valid_88xx_v1(struct halmac_adapter *adapter, u8 *buf)
{
	u32 txdesc_size = adapter->hw_cfg_info.txdesc_body_size +
						GET_TX_DESC_PKT_OFFSET_V1(buf);
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	struct wlhdr_frame_ctrl *wlhdr;

	wlhdr = (struct wlhdr_frame_ctrl *)(buf + txdesc_size);

	if (wlhdr->protocol != WLHDR_PROT_VER) {
		PLTFM_MSG_ERR("[ERR]prot ver!!\n");
		return HALMAC_RET_WLHDR_FAIL;
	}

	switch (wlhdr->type) {
	case WLHDR_TYPE_MGMT:
		if (wlhdr_mgmt_valid_88xx_v1(adapter, wlhdr) != 1)
			status = HALMAC_RET_WLHDR_FAIL;
		break;
	case WLHDR_TYPE_CTRL:
		if (wlhdr_ctrl_valid_88xx_v1(adapter, wlhdr) != 1)
			status = HALMAC_RET_WLHDR_FAIL;
		break;
	case WLHDR_TYPE_DATA:
		if (wlhdr_data_valid_88xx_v1(adapter, wlhdr) != 1)
			status = HALMAC_RET_WLHDR_FAIL;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]undefined type!!\n");
		status = HALMAC_RET_WLHDR_FAIL;
		break;
	}

	return status;
}

static u8
wlhdr_mgmt_valid_88xx_v1(struct halmac_adapter *adapter,
			 struct wlhdr_frame_ctrl *wlhdr)
{
	u8 state;

	switch (wlhdr->sub_type) {
	case WLHDR_SUB_TYPE_ASSOC_REQ:
	case WLHDR_SUB_TYPE_ASSOC_RSPNS:
	case WLHDR_SUB_TYPE_REASSOC_REQ:
	case WLHDR_SUB_TYPE_REASSOC_RSPNS:
	case WLHDR_SUB_TYPE_PROBE_REQ:
	case WLHDR_SUB_TYPE_PROBE_RSPNS:
	case WLHDR_SUB_TYPE_BCN:
	case WLHDR_SUB_TYPE_DISASSOC:
	case WLHDR_SUB_TYPE_AUTH:
	case WLHDR_SUB_TYPE_DEAUTH:
	case WLHDR_SUB_TYPE_ACTION:
	case WLHDR_SUB_TYPE_ACTION_NOACK:
		state = 1;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]mgmt invalid!!\n");
		state = 0;
		break;
	}

	return state;
}

static u8
wlhdr_ctrl_valid_88xx_v1(struct halmac_adapter *adapter,
			 struct wlhdr_frame_ctrl *wlhdr)
{
	u8 state;

	switch (wlhdr->sub_type) {
	case WLHDR_SUB_TYPE_BF_RPT_POLL:
	case WLHDR_SUB_TYPE_NDPA:
		state = 1;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]ctrl invalid!!\n");
		state = 0;
		break;
	}

	return state;
}

static u8
wlhdr_data_valid_88xx_v1(struct halmac_adapter *adapter,
			 struct wlhdr_frame_ctrl *wlhdr)
{
	u8 state;

	switch (wlhdr->sub_type) {
	case WLHDR_SUB_TYPE_DATA:
	case WLHDR_SUB_TYPE_NULL:
	case WLHDR_SUB_TYPE_QOS_DATA:
	case WLHDR_SUB_TYPE_QOS_NULL:
		state = 1;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]data invalid!!\n");
		state = 0;
		break;
	}

	return state;
}

/**
 * get_version_88xx_v1() - get HALMAC version
 * @ver : return version of major, prototype and minor information
 * Author : KaiYuan Chang / Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
get_version_88xx_v1(struct halmac_adapter *adapter, struct halmac_ver *ver)
{
	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	ver->major_ver = (u8)HALMAC_MAJOR_VER;
	ver->prototype_ver = (u8)HALMAC_PROTOTYPE_VER;
	ver->minor_ver = (u8)HALMAC_MINOR_VER;

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
p2pps_88xx_v1(struct halmac_adapter *adapter, struct halmac_p2pps *info)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * query_status_88xx_v1() -query the offload feature status
 * @adapter : the adapter of halmac
 * @feature_id : feature_id
 * @proc_status : feature_status
 * @data : data buffer
 * @size : data size
 *
 * Note :
 * If user wants to know the data size, user can allocate zero
 * size buffer first. If this size less than the data size, halmac
 * will return  HALMAC_RET_BUFFER_TOO_SMALL. User need to
 * re-allocate data buffer with correct data size.
 *
 * Author : Ivan Lin/KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
query_status_88xx_v1(struct halmac_adapter *adapter,
		     enum halmac_feature_id feature_id,
		     enum halmac_cmd_process_status *proc_status, u8 *data,
		     u32 *size)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	if (!proc_status)
		return HALMAC_RET_NULL_POINTER;

	switch (feature_id) {
	case HALMAC_FEATURE_CFG_PARA:
		status = get_cfg_param_status_88xx_v1(adapter, proc_status);
		break;
	case HALMAC_FEATURE_DUMP_PHYSICAL_EFUSE:
		status = get_dump_phy_efuse_status_88xx_v1(adapter, proc_status,
							   data, size);
		break;
	case HALMAC_FEATURE_DUMP_LOGICAL_EFUSE:
		status = get_dump_log_efuse_status_88xx_v1(adapter, proc_status,
							   data, size);
		break;
	case HALMAC_FEATURE_DUMP_LOGICAL_EFUSE_MASK:
		status = get_log_efuse_mask_status_88xx_v1(adapter,
							   proc_status,
							   data, size);
		break;
	case HALMAC_FEATURE_CHANNEL_SWITCH:
		status = get_ch_switch_status_88xx_v1(adapter, proc_status);
		break;
	case HALMAC_FEATURE_UPDATE_PACKET:
		status = get_update_packet_status_88xx_v1(adapter, proc_status);
		break;
	case HALMAC_FEATURE_SEND_SCAN_PACKET:
		status = get_send_scan_packet_status_88xx_v1(adapter,
							     proc_status);
		break;
	case HALMAC_FEATURE_DROP_SCAN_PACKET:
		status = get_drop_scan_packet_status_88xx_v1(adapter,
							     proc_status);
		break;
	case HALMAC_FEATURE_IQK:
		status = get_iqk_status_88xx_v1(adapter, proc_status);
		break;
	case HALMAC_FEATURE_POWER_TRACKING:
		status = get_pwr_trk_status_88xx_v1(adapter, proc_status);
		break;
	case HALMAC_FEATURE_PSD:
		status = get_psd_status_88xx_v1(adapter, proc_status, data,
						size);
		break;
	default:
		return HALMAC_RET_INVALID_FEATURE_ID;
	}

	return status;
}

static enum halmac_ret_status
get_cfg_param_status_88xx_v1(struct halmac_adapter *adapter,
			     enum halmac_cmd_process_status *proc_status)
{
	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_ch_switch_status_88xx_v1(struct halmac_adapter *adapter,
			     enum halmac_cmd_process_status *proc_status)
{
	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_update_packet_status_88xx_v1(struct halmac_adapter *adapter,
				 enum halmac_cmd_process_status *proc_status)
{
	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_send_scan_packet_status_88xx_v1(struct halmac_adapter *adapter,
				    enum halmac_cmd_process_status *proc_status)
{
	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
get_drop_scan_packet_status_88xx_v1(struct halmac_adapter *adapter,
				    enum halmac_cmd_process_status *proc_status)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * cfg_drv_rsvd_pg_num_88xx_v1() -config reserved page number for driver
 * @adapter : the adapter of halmac
 * @pg_num : page number
 * Author : KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
cfg_drv_rsvd_pg_num_88xx_v1(struct halmac_adapter *adapter,
			    enum halmac_drv_rsvd_pg_num pg_num)
{
	if (adapter->api_registry.cfg_drv_rsvd_pg_en == 0)
		return HALMAC_RET_NOT_SUPPORT;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);
	PLTFM_MSG_TRACE("[TRACE]pg_num = %d\n", pg_num);

	switch (pg_num) {
	case HALMAC_RSVD_PG_NUM8:
		adapter->txff_alloc.rsvd_drv_pg_num = 8;
		break;
	case HALMAC_RSVD_PG_NUM16:
		adapter->txff_alloc.rsvd_drv_pg_num = 16;
		break;
	case HALMAC_RSVD_PG_NUM24:
		adapter->txff_alloc.rsvd_drv_pg_num = 24;
		break;
	case HALMAC_RSVD_PG_NUM32:
		adapter->txff_alloc.rsvd_drv_pg_num = 32;
		break;
	case HALMAC_RSVD_PG_NUM64:
		adapter->txff_alloc.rsvd_drv_pg_num = 64;
		break;
	case HALMAC_RSVD_PG_NUM128:
		adapter->txff_alloc.rsvd_drv_pg_num = 128;
		break;
	case HALMAC_RSVD_PG_NUM256:
		adapter->txff_alloc.rsvd_drv_pg_num = 256;
		break;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * (debug API)h2c_lb_88xx_v1() - send h2c loopback packet
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang/Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
h2c_lb_88xx_v1(struct halmac_adapter *adapter)
{
	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
pwr_seq_parser_88xx_v1(struct halmac_adapter *adapter,
		       struct halmac_wlan_pwr_cfg **cmd_seq)
{
	u8 cut;
	u8 intf;
	u32 idx = 0;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	struct halmac_wlan_pwr_cfg *cmd;

	switch (adapter->chip_ver) {
	case HALMAC_CHIP_VER_A_CUT:
		cut = HALMAC_PWR_CUT_A_MSK;
		break;
	case HALMAC_CHIP_VER_B_CUT:
		cut = HALMAC_PWR_CUT_B_MSK;
		break;
	case HALMAC_CHIP_VER_C_CUT:
		cut = HALMAC_PWR_CUT_C_MSK;
		break;
	case HALMAC_CHIP_VER_D_CUT:
		cut = HALMAC_PWR_CUT_D_MSK;
		break;
	case HALMAC_CHIP_VER_E_CUT:
		cut = HALMAC_PWR_CUT_E_MSK;
		break;
	case HALMAC_CHIP_VER_F_CUT:
		cut = HALMAC_PWR_CUT_F_MSK;
		break;
	case HALMAC_CHIP_VER_TEST:
		cut = HALMAC_PWR_CUT_TESTCHIP_MSK;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]cut version!!\n");
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	switch (adapter->intf) {
	case HALMAC_INTERFACE_PCIE:
	case HALMAC_INTERFACE_AXI:
		intf = HALMAC_PWR_INTF_PCI_MSK;
		break;
	case HALMAC_INTERFACE_USB:
		intf = HALMAC_PWR_INTF_USB_MSK;
		break;
	case HALMAC_INTERFACE_SDIO:
		intf = HALMAC_PWR_INTF_SDIO_MSK;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]interface!!\n");
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	do {
		cmd = cmd_seq[idx];

		if (!cmd)
			break;

		status = pwr_sub_seq_parser_88xx_v1(adapter, cut, intf, cmd);
		if (status != HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]pwr sub seq!!\n");
			return status;
		}

		idx++;
	} while (1);
	return status;
}

static enum halmac_ret_status
pwr_sub_seq_parser_88xx_v1(struct halmac_adapter *adapter, u8 cut, u8 intf,
			   struct halmac_wlan_pwr_cfg *cmd)
{
	u8 value;
	u32 offset;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	do {
		if ((cmd->interface_msk & intf) && (cmd->cut_msk & cut)) {
			switch (cmd->cmd) {
			case HALMAC_PWR_CMD_WRITE:
				offset = cmd->offset;

				if (cmd->base == HALMAC_PWR_ADDR_SDIO)
					offset |= SDIO_LOCAL_OFFSET;

				value = HALMAC_REG_R8(offset);
				value = (u8)(value & (u8)(~(cmd->msk)));
				value = (u8)(value | (cmd->value & cmd->msk));

				HALMAC_REG_W8(offset, value);
				break;
			case HALMAC_PWR_CMD_POLLING:
				if (pwr_cmd_polling_88xx_v1(adapter, cmd) !=
				    HALMAC_RET_SUCCESS)
					return HALMAC_RET_PWRSEQ_POLLING_FAIL;
				break;
			case HALMAC_PWR_CMD_DELAY:
				if (cmd->value == HALMAC_PWR_DELAY_US)
					PLTFM_DELAY_US(cmd->offset);
				else
					PLTFM_DELAY_US(1000 * cmd->offset);
				break;
			case HALMAC_PWR_CMD_READ:
				break;
			case HALMAC_PWR_CMD_END:
				return HALMAC_RET_SUCCESS;
			default:
				return HALMAC_RET_PWRSEQ_CMD_INCORRECT;
				}
		}
		cmd++;
	} while (1);

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
pwr_cmd_polling_88xx_v1(struct halmac_adapter *adapter,
			struct halmac_wlan_pwr_cfg *cmd)
{
	u8 value;
	u8 poll_bit;
	u32 offset;
	u32 cnt;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	poll_bit = 0;
	cnt = HALMAC_PWR_POLLING_CNT;

	if (cmd->base == HALMAC_PWR_ADDR_SDIO)
		offset = cmd->offset | SDIO_LOCAL_OFFSET;
	else
		offset = cmd->offset;

	do {
		cnt--;
		value = HALMAC_REG_R8(offset);
		value = (u8)(value & cmd->msk);

		if (value == (cmd->value & cmd->msk)) {
			poll_bit = 1;
		} else {
			if (cnt == 0) {
				PLTFM_MSG_ERR("[ERR]polling to!!\n");
				PLTFM_MSG_ERR("[ERR]cmd offset:%X\n",
					      cmd->offset);
				PLTFM_MSG_ERR("[ERR]cmd value:%X\n",
					      cmd->value);
				PLTFM_MSG_ERR("[ERR]cmd msk:%X\n", cmd->msk);
				PLTFM_MSG_ERR("[ERR]offset = %X\n", offset);
				PLTFM_MSG_ERR("[ERR]value = %X\n", value);
				return HALMAC_RET_PWRSEQ_POLLING_FAIL;
			}

			PLTFM_DELAY_US(50);
		}
	} while (!poll_bit);

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
parse_txdesc_ie_88xx_v1(struct halmac_adapter *adapter, u8 *buf,
			struct halmac_txdesc_ie_param *param)
{
	u8 ie_cnt = 0, body_size = adapter->hw_cfg_info.txdesc_body_size;
	u8 i, num;
	u8 start_ie_offset;

	if (!param->ie_offset) {
		PLTFM_MSG_ERR("[ERR] ie_offset NULL!!\n");
		return HALMAC_RET_FAIL;
	}

	if (!param->ie_exist) {
		PLTFM_MSG_ERR("[ERR] ie_exist NULL!!\n");
		return HALMAC_RET_FAIL;
	}

	if (!buf) {
		PLTFM_MSG_ERR("[ERR] buffer NULL!!\n");
		return HALMAC_RET_FAIL;
	}

	if (adapter->tx_desc_transfer == 0)
		return HALMAC_RET_FAIL;

	if (GET_TX_DESC_IE_END_BODY(buf) == 1)
		return HALMAC_RET_SUCCESS;

	ie_cnt = (u8)GET_TX_DESC_PKT_OFFSET_V1(buf);
	start_ie_offset = (*param->start_offset -
			adapter->hw_cfg_info.txdesc_body_size) >> 3;

	for (i = start_ie_offset; i < ie_cnt; i++) {
		num = (u8)IE0_GET_TX_DESC_IE_NUM(buf + body_size + (i << 3));
		if (num > adapter->hw_cfg_info.txdesc_ie_max_num) {
			PLTFM_MSG_ERR("[ERR]tx desc IE num %d!!\n", num);
			return HALMAC_RET_FAIL;
		}

		if (*(param->ie_exist + num) == 1) {
			PLTFM_MSG_ERR("[ERR]tx desc IE overlap!!\n");
			return HALMAC_RET_FAIL;
		}

		*(param->ie_exist + num) = 1;
		*(param->ie_offset + num) = (i << 3) + body_size;
		*param->end_offset = *param->ie_offset + 8;
		if (IE0_GET_TX_DESC_IE_END(buf + body_size + (i << 3)) == 1)
			break;

		if (i == ie_cnt - 1) {
			PLTFM_MSG_ERR("[ERR]tx desc IE end missing!!\n");
			return HALMAC_RET_FAIL;
		}
	}

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
parse_intf_phy_88xx_v1(struct halmac_adapter *adapter,
		       struct halmac_intf_phy_para *param,
		       enum halmac_intf_phy_platform pltfm,
		       enum hal_intf_phy intf_phy)
{
	u16 value;
	u16 cur_cut;
	u16 offset;
	u16 ip_sel;
	struct halmac_intf_phy_para *cur_param;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;
	u8 result = HALMAC_RET_SUCCESS;

	switch (adapter->chip_ver) {
	case HALMAC_CHIP_VER_A_CUT:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_A;
		break;
	case HALMAC_CHIP_VER_B_CUT:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_B;
		break;
	case HALMAC_CHIP_VER_C_CUT:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_C;
		break;
	case HALMAC_CHIP_VER_D_CUT:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_D;
		break;
	case HALMAC_CHIP_VER_E_CUT:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_E;
		break;
	case HALMAC_CHIP_VER_F_CUT:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_F;
		break;
	case HALMAC_CHIP_VER_TEST:
		cur_cut = (u16)HALMAC_INTF_PHY_CUT_TESTCHIP;
		break;
	default:
		return HALMAC_RET_FAIL;
	}

	cur_param = param;

	do {
		if ((cur_param->cut & cur_cut) &&
		    (cur_param->plaform & (u16)pltfm)) {
			offset =  cur_param->offset;
			value = cur_param->value;
			ip_sel = cur_param->ip_sel;

			if (offset == 0xFFFF)
				break;

			if (ip_sel == HALMAC_IP_SEL_MAC) {
				HALMAC_REG_W8((u32)offset, (u8)value);
			} else if (intf_phy == HAL_INTF_PHY_USB2 ||
				   intf_phy == HAL_INTF_PHY_USB3) {
#if HALMAC_USB_SUPPORT
				result = usbphy_write_88xx_v1(adapter,
							      (u8)offset,
							      value,
							      intf_phy);
				if (result != HALMAC_RET_SUCCESS)
					PLTFM_MSG_ERR("[ERR]usb phy!!\n");
#endif
			} else if (intf_phy == HAL_INTF_PHY_PCIE_GEN1 ||
				   intf_phy == HAL_INTF_PHY_PCIE_GEN2) {
#if HALMAC_PCIE_SUPPORT
				if (ip_sel == HALMAC_IP_INTF_PHY)
					result = mdio_write_88xx_v1(adapter,
								    (u8)offset,
								    value,
								    intf_phy);
				else
					result = dbi_w8_88xx_v1(adapter, offset,
								(u8)value);
				if (result != HALMAC_RET_SUCCESS)
					PLTFM_MSG_ERR("[ERR]mdio/dbi!!\n");
#endif
			} else {
				PLTFM_MSG_ERR("[ERR]intf phy sel!!\n");
			}
		}
		cur_param++;
	} while (1);

	return HALMAC_RET_SUCCESS;
}

/**
 * txfifo_is_empty_88xx_v1() -check if txfifo is empty
 * @adapter : the adapter of halmac
 * @chk_num : check number
 * Author : Ivan Lin
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
txfifo_is_empty_88xx_v1(struct halmac_adapter *adapter, u32 chk_num)
{
	return HALMAC_RET_SUCCESS;
}

static void
pwr_state_88xx_v1(struct halmac_adapter *adapter, enum halmac_mac_power *state)
{
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	if ((HALMAC_REG_R8(REG_SYS_FUNC_EN + 1) & BIT(3)) == 0)
		*state = HALMAC_MAC_POWER_OFF;
	else
		*state = HALMAC_MAC_POWER_ON;
}

#endif /* HALMAC_88XX_V1_SUPPORT */
