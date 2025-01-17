/*
 *  USB cmd handle routines
 *
 *  Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#define _8192CD_USB_CMD_C_

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#endif

#include "8192cd.h"
#include "8192cd_headers.h"
#include "8192cd_debug.h"
#ifdef RTK_NL80211
#include "8192cd_cfg80211.h" 
#endif


#define CHK_H2C_BUF_INTERVAL		msecs_to_jiffies(5)

struct cmd_header {
	_list list;
	struct rtl8192cd_priv *priv;
	struct stat_info *pstat;
	u16 cmd_code;
	u16 cmd_len;
};

struct slot_time_parm {
	struct cmd_header header;
	u16 use_short;
};

struct switch_edca_parm {
	struct cmd_header header;
	u32 be_edca;
	u32 vi_edca;
};

struct check_dig_parm {
	struct cmd_header header;
	u8 rssi;
};

struct set_key_parm {
	struct cmd_header header;
	struct net_device *dev;
	unsigned long KeyIndex;
//	unsigned long KeyLen;
	unsigned char KeyType;
	unsigned char EncType;
        unsigned char MACAddr[MACADDRLEN];
	unsigned char Key[32];
};

struct disconnect_sta_parm {
	struct cmd_header header;
	struct net_device *dev;
	DOT11_DISCONNECT_REQ DisconnectReq;
};

struct indicate_mic_failure_parm {
	struct cmd_header header;
	struct net_device *dev;
	unsigned char MACAddr[MACADDRLEN];
	unsigned char indicate_upper_layer;
};

struct mac_clone_parm {
	struct cmd_header header;
	unsigned char MACAddr[MACADDRLEN];
};

struct recv_mgnt_frame_parm {
	struct cmd_header header;
	struct rx_frinfo *pfrinfo;
};

struct macid_pause_parm {
	struct cmd_header header;
        u16 macid;
	u16 pause;
};

struct macid_nolink_parm {
	struct cmd_header header;
        u16 macid;
	u16 nolink;
};

#if defined(RTK_NL80211)
struct indicate_cfg_evt_parm {
	struct cmd_header header;
	int evt;
	unsigned char mac[6];
	unsigned char *extra;
};
#endif

u8 callback_update_sta_RATid(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
u8 callback_update_sta_rssi(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_update_sta_ps(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif
u8 callback_del_sta(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);

#ifdef CONFIG_RTL_92C_SUPPORT
u8 callback_HIQ_NoLimit_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif
u8 callback_40M_RRSR_SC_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_NAV_prot_len_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_slot_time_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_NBI_filter_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#ifdef SW_ANT_SWITCH
u8 callback_antenna_switch(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif
u8 callback_mp_ctx_background(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_IOT_EDCA_switch(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_check_DIG_by_rssi(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_set_key(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_disconnect_sta(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_indicate_MIC_failure(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_indicate_MIC_failure_clnt(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#ifdef RTK_BR_EXT
u8 callback_mac_clone(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif
u8 callback_recv_mgnt_frame(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#ifdef CONFIG_RTL_WAPI_SUPPORT
u8 callback_recv_wai_frame(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif
u8 callback_tx_report_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_tx_report_interval_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_macid_pause_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
u8 callback_macid_no_link_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);

#if defined(WIFI_WPAS_CLI) || defined(RTK_NL80211)
u8 callback_wpas_join(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif

#ifdef RTK_NL80211
u8 callback_notify_cfg_evt(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);
#endif

typedef u8 (*cmd_callback)(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr);

struct _cmd_callback {
	u32 cmd_code;
	cmd_callback callback;
};

struct _cmd_callback rtw_cmd_callback[] = {
	{ _CMD_UPDATE_STA_RATID, callback_update_sta_RATid },
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	{ _CMD_UPDATE_STA_RSSI, callback_update_sta_rssi},
	{ _CMD_UPDATE_STA_PS, callback_update_sta_ps},
#endif
	{ _CMD_DEL_STA, callback_del_sta },
	
#ifdef CONFIG_RTL_92C_SUPPORT
	{ _CMD_HIQ_NO_LIMIT_CHANGE, callback_HIQ_NoLimit_change },
#endif
	{ _CMD_40M_RRSR_SC_CHANGE, callback_40M_RRSR_SC_change },
	{ _CMD_NAV_PROT_LEN_CHANGE, callback_NAV_prot_len_change },
	{ _CMD_SLOT_TIME_CHANGE, callback_slot_time_change },
	{ _CMD_NBI_FILTER_CHANGE, callback_NBI_filter_change },
#ifdef SW_ANT_SWITCH
	{ _CMD_ANTENNA_SWITCH, callback_antenna_switch },
#endif
	{ _CMD_MP_CTX_BACKGROUND, callback_mp_ctx_background },
	{ _CMD_IOT_EDCA_SWITCH, callback_IOT_EDCA_switch },
	{ _CMD_CHECK_DIG_BY_RSSI, callback_check_DIG_by_rssi },
	{ _CMD_SET_KEY, callback_set_key },
	{ _CMD_DISCONNECT_STA, callback_disconnect_sta },
	{ _CMD_INDICATE_MIC_FAILURE, callback_indicate_MIC_failure },
	{ _CMD_INDICATE_MIC_FAILURE_CLNT, callback_indicate_MIC_failure_clnt },
#ifdef RTK_BR_EXT
	{ _CMD_MAC_CLONE, callback_mac_clone },
#endif
	{ _CMD_RECV_MGNT_FRAME, callback_recv_mgnt_frame },
#ifdef CONFIG_RTL_WAPI_SUPPORT
	{ _CMD_RECV_WAI_FRAME, callback_recv_wai_frame },
#endif
#ifdef CONFIG_RTL_88E_SUPPORT
	{ _CMD_TX_REPORT_CHANGE, callback_tx_report_change },
	{ _CMD_TX_REPORT_INTERVAL_CHANGE, callback_tx_report_interval_change },
	{ _CMD_MACID_PAUSE_CHANGE, callback_macid_pause_change },
	{ _CMD_MACID_NO_LINK_CHANGE, callback_macid_no_link_change },
#endif
#if defined(WIFI_WPAS_CLI) || defined(RTK_NL80211)
	{ _CMD_WPAS_JOIN, callback_wpas_join },
#endif
#ifdef RTK_NL80211
	{ _CMD_CFG_IND, callback_notify_cfg_evt },
#endif
};

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
static void check_h2c_buf_timeout(unsigned long task_priv)
{
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)task_priv;
	
	if (_rtw_queue_empty(&priv->pshare->h2c_cmd_queue) == FALSE) {
		if (test_and_set_bit(WAKE_EVENT_CMD, &priv->pshare->cmd_wake) == 0)
			wake_up_process(priv->pshare->cmd_thread);
		return;
	}
}
#endif // CONFIG_RTL_92D_SUPPORT || CONFIG_RTL_92C_SUPPORT

int _rtw_init_cmd_priv(struct rtl8192cd_priv *priv)
{
	struct priv_shared_info *pshare;
	
	WARN_ON(ARRAY_SIZE(rtw_cmd_callback) != MAX_RTW_CMD_CODE);
	
	pshare = priv->pshare;
	
	init_waitqueue_head(&pshare->waitqueue);
	
	_rtw_init_queue(&pshare->cmd_queue);
	_rtw_init_queue(&pshare->rx_mgt_queue);
	_rtw_init_queue(&pshare->timer_evt_queue);
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	_rtw_init_queue(&pshare->h2c_cmd_queue);
	
	init_timer(&pshare->chk_h2c_buf_timer);
	pshare->chk_h2c_buf_timer.data = (unsigned long) priv;
	pshare->chk_h2c_buf_timer.function = &check_h2c_buf_timeout;
#endif
	
	return SUCCESS;
}

void _rtw_free_cmd_priv(struct rtl8192cd_priv *priv)
{
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	_rtw_spinlock_free(&priv->pshare->h2c_cmd_queue.lock);
#endif
	_rtw_spinlock_free(&priv->pshare->timer_evt_queue.lock);
	_rtw_spinlock_free(&priv->pshare->rx_mgt_queue.lock);
	_rtw_spinlock_free(&priv->pshare->cmd_queue.lock);
}

static int rtw_enqueue_cmd(struct rtl8192cd_priv *priv, _queue *cmd_queue, struct cmd_header *pcmdhdr)
{
	_irqL irqL;
	
	_enter_critical(&cmd_queue->lock, &irqL);
	
	rtw_list_insert_tail(&pcmdhdr->list, &cmd_queue->queue);
	
	++cmd_queue->qlen;
	
	_exit_critical(&cmd_queue->lock, &irqL);
	
	if (test_and_set_bit(WAKE_EVENT_CMD, &priv->pshare->cmd_wake) == 0)
		wake_up_process(priv->pshare->cmd_thread);
	
	return TRUE;
}

static struct cmd_header* rtw_dequeue_cmd(struct rtl8192cd_priv *priv, _queue *cmd_queue)
{
	_list *phead, *plist;
	_irqL irqL;
	
	struct cmd_header *cmd_obj = NULL;
	
	plist = NULL;
	
	phead = get_list_head(cmd_queue);
	
	_enter_critical(&cmd_queue->lock, &irqL);
	
	if (rtw_is_list_empty(phead) == FALSE) {
		plist = get_next(phead);
		rtw_list_delete(plist);
		--cmd_queue->qlen;
	}
	
	_exit_critical(&cmd_queue->lock, &irqL);
	
	if (plist)
		cmd_obj = LIST_CONTAINOR(plist, struct cmd_header, list);
	
	return cmd_obj;
}

int rtw_enqueue_timer_event(struct rtl8192cd_priv *priv, struct timer_event_entry *entry, int insert_tail)
{
	_irqL irqL;
	_queue *pqueue = &priv->pshare->timer_evt_queue;
	int insert = 0;
	
	_enter_critical(&pqueue->lock, &irqL);
	
	if (rtw_is_list_empty(&entry->list) == TRUE) {
		if (ENQUEUE_TO_TAIL == insert_tail)
			rtw_list_insert_tail(&entry->list, &pqueue->queue);
		else
			rtw_list_insert_head(&entry->list, &pqueue->queue);
		++pqueue->qlen;
		insert = 1;
	}
	
	_exit_critical(&pqueue->lock, &irqL);
	
	if (insert) {
		if (test_and_set_bit(WAKE_EVENT_CMD, &priv->pshare->cmd_wake) == 0)
			wake_up_process(priv->pshare->cmd_thread);
	} else {
		++priv->pshare->nr_timer_evt_miss;
		printk("[%s] inertion fail!! (function=%p)\n", __FUNCTION__, entry->function);
	}
	
	return TRUE;
}

struct timer_event_entry* rtw_dequeue_timer_event(struct rtl8192cd_priv *priv)
{
	_list *phead, *plist;
	_irqL irqL;
	struct timer_event_entry *entry = NULL;
	_queue *pqueue = &priv->pshare->timer_evt_queue;
	
	plist = NULL;
	
	phead = get_list_head(pqueue);
	
	_enter_critical(&pqueue->lock, &irqL);
	
	if (rtw_is_list_empty(phead) == FALSE) {
		plist = get_next(phead);
		rtw_list_delete(plist);
		--pqueue->qlen;
	}
	
	_exit_critical(&pqueue->lock, &irqL);
	
	if (plist)
		entry = LIST_CONTAINOR(plist, struct timer_event_entry, list);
	
	return entry;
}

void timer_event_timer_fn(unsigned long __data)
{
	struct timer_event_entry *entry = (struct timer_event_entry *)__data;
	struct rtl8192cd_priv *priv = (struct rtl8192cd_priv *)entry->data;

	if (!(priv->drv_state & DRV_STATE_OPEN))
		return;

	rtw_enqueue_timer_event(priv, entry, ENQUEUE_TO_TAIL);
}

static void rtw_split_cmd_queue(struct rtl8192cd_priv *priv, struct stat_info *pstat,
	_queue *from_queue, _list *to_list)
{
	_list *phead, *plist;
	_irqL irqL;
	
	struct cmd_header *cmd_obj;
	
	phead = get_list_head(from_queue);
	
	_enter_critical(&from_queue->lock, &irqL);
	
	plist = get_next(phead);
	
	while (rtw_end_of_queue_search(phead, plist) == FALSE) {
		cmd_obj = LIST_CONTAINOR(plist, struct cmd_header, list);
		plist = get_next(plist);
		
		if ((cmd_obj->priv == priv) && (cmd_obj->pstat == pstat)) {
			rtw_list_delete(&cmd_obj->list);
			rtw_list_insert_tail(&cmd_obj->list, to_list);
			--from_queue->qlen;
		}
	}
	
	_exit_critical(&from_queue->lock, &irqL);
}

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
void rtw_flush_h2c_cmd_queue(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	_list cmd_list;
	_list *phead, *plist;
	struct cmd_header *cmd_obj;
	
	if ((NULL != pstat) && (0 == pstat->pending_cmd))
		return;
	
	phead = &cmd_list;
	
	_rtw_init_listhead(phead);
	
	rtw_split_cmd_queue(priv, pstat, &priv->pshare->h2c_cmd_queue, phead);
	
	plist = get_next(phead);
	
	while (rtw_end_of_queue_search(phead, plist) == FALSE) {
		cmd_obj = LIST_CONTAINOR(plist, struct cmd_header, list);
		plist = get_next(plist);
		
		if (cmd_obj->pstat)
			clear_bit(cmd_obj->cmd_code, &cmd_obj->pstat->pending_cmd);
		rtw_mfree((u8*)cmd_obj, cmd_obj->cmd_len);
	}
}
#endif

void rtw_flush_cmd_queue(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	_list cmd_list;
	_list *phead, *plist;
	struct cmd_header *cmd_obj;
	
#ifdef CONFIG_RTL_88E_SUPPORT
	if ((NULL != pstat) && (0 == pstat->pending_cmd))
		return;
#endif
	
	phead = &cmd_list;
	
	_rtw_init_listhead(phead);
	
	rtw_split_cmd_queue(priv, pstat, &priv->pshare->cmd_queue, phead);
	
	plist = get_next(phead);
	
	while (rtw_end_of_queue_search(phead, plist) == FALSE) {
		cmd_obj = LIST_CONTAINOR(plist, struct cmd_header, list);
		plist = get_next(plist);
		
#ifdef CONFIG_RTL_88E_SUPPORT
		if (cmd_obj->pstat)
			clear_bit(cmd_obj->cmd_code, &cmd_obj->pstat->pending_cmd);
#endif
		rtw_mfree((u8*)cmd_obj, cmd_obj->cmd_len);
	}
}

void rtw_flush_rx_mgt_queue(struct rtl8192cd_priv *priv)
{
	_list cmd_list;
	_list *phead, *plist;
	struct cmd_header *cmd_obj;
	struct recv_mgnt_frame_parm *param;
	
	phead = &cmd_list;
	
	_rtw_init_listhead(phead);
	
	rtw_split_cmd_queue(priv, NULL, &priv->pshare->rx_mgt_queue, phead);
	
	plist = get_next(phead);
	
	while (rtw_end_of_queue_search(phead, plist) == FALSE) {
		cmd_obj = LIST_CONTAINOR(plist, struct cmd_header, list);
		plist = get_next(plist);
		
		param = (struct recv_mgnt_frame_parm *)cmd_obj;
		rtl_kfree_skb(priv, param->pfrinfo->pskb, _SKB_RX_);
		rtw_mfree((u8*)cmd_obj, cmd_obj->cmd_len);
	}
}

int rtw_cmd_thread(void *context)
{
	struct rtl8192cd_priv *priv;
	struct priv_shared_info *pshare;
	struct cmd_header* pcmd;
	cmd_callback cmd_handler;
	
	priv = (struct rtl8192cd_priv*) context;
	pshare = priv->pshare;
	
//	set_user_nice(current, -10);
	
	while (1)
	{
		wait_event(pshare->waitqueue, test_and_clear_bit(WAKE_EVENT_CMD, &pshare->cmd_wake));
		
		while (1)
		{
			if ((TRUE == pshare->bDriverStopped) ||(TRUE == pshare->bSurpriseRemoved)) {
				// avoid to continue calling wake_up_process() when cmd_thread is NULL
				set_bit(WAKE_EVENT_CMD, &pshare->cmd_wake);
				
				printk("[%s] bDriverStopped(%d) OR bSurpriseRemoved(%d)\n",
					__FUNCTION__, pshare->bDriverStopped, pshare->bSurpriseRemoved);
				goto out;
			}
			
			pcmd = NULL;
			
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
			if (_rtw_queue_empty(&pshare->h2c_cmd_queue) == FALSE) {
				if (!is_h2c_buf_occupy(priv)) {
					pcmd = rtw_dequeue_cmd(priv, &pshare->h2c_cmd_queue);
					if (NULL != pcmd) {
						++pshare->nr_h2c_cmd;
						goto handle_cmd;
					}
				}
			}
#endif
			
			if (_rtw_queue_empty(&pshare->timer_evt_queue) == FALSE) {
				struct timer_event_entry *timer_event;
				
				timer_event = rtw_dequeue_timer_event(priv);
				if (NULL != timer_event) {
					++pshare->nr_timer_evt;
#ifdef CMD_THREAD_FUNC_DEBUG
					pshare->cur_cmd_func = timer_event->function;
					timer_event->function(timer_event->data);
					pshare->cur_cmd_func = NULL;
#else
					timer_event->function(timer_event->data);
#endif
					continue;
				}
			}
			
			if (_rtw_queue_empty(&pshare->rx_mgt_queue) == FALSE) {
				pcmd = rtw_dequeue_cmd(priv, &pshare->rx_mgt_queue);
				if (NULL != pcmd) {
					++pshare->nr_rx_mgt_cmd;
					goto handle_cmd;
				}
			}
			
			pcmd = rtw_dequeue_cmd(priv, &pshare->cmd_queue);
			if (NULL != pcmd) {
				++pshare->nr_cmd;
				goto handle_cmd;
			}
			
			break;
			
handle_cmd:
			if (pcmd->cmd_code < ARRAY_SIZE(rtw_cmd_callback)) {
				cmd_handler = rtw_cmd_callback[pcmd->cmd_code].callback;
#ifdef CMD_THREAD_FUNC_DEBUG
				pshare->cur_cmd_func = cmd_handler;
				cmd_handler(pcmd->priv, pcmd);
				pshare->cur_cmd_func = NULL;
#else
				cmd_handler(pcmd->priv, pcmd);
#endif
			}
			
			rtw_mfree((u8*)pcmd, pcmd->cmd_len);
		}
		
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
		if (_rtw_queue_empty(&pshare->h2c_cmd_queue) == FALSE) {
			if (!timer_pending(&pshare->chk_h2c_buf_timer))
				mod_timer(&pshare->chk_h2c_buf_timer, jiffies + CHK_H2C_BUF_INTERVAL);
		}
#endif
	}
	
out:
	
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	if (timer_pending(&pshare->chk_h2c_buf_timer))
		del_timer_sync(&pshare->chk_h2c_buf_timer);
#endif
	
	complete_and_exit(&pshare->cmd_thread_done, 0);
}

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
void notify_update_sta_rssi(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_UPDATE_STA_RSSI, &pstat->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_h2c_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		clear_bit(_CMD_UPDATE_STA_RSSI, &pstat->pending_cmd);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_UPDATE_STA_RSSI;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	pcmdhdr->pstat = pstat;
	
	rtw_enqueue_cmd(priv, &priv->pshare->h2c_cmd_queue, pcmdhdr);
}

u8 callback_update_sta_rssi(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct stat_info *pstat = pcmdhdr->pstat;
	
	clear_bit(_CMD_UPDATE_STA_RSSI, &pstat->pending_cmd);
	
	set_rssi_cmd(priv, pstat);
	
	return TRUE;
}

void notify_update_sta_ps(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_UPDATE_STA_PS, &pstat->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_h2c_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		clear_bit(_CMD_UPDATE_STA_PS, &pstat->pending_cmd);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_UPDATE_STA_PS;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	pcmdhdr->pstat = pstat;
	
	rtw_enqueue_cmd(priv, &priv->pshare->h2c_cmd_queue, pcmdhdr);
}

u8 callback_update_sta_ps(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct stat_info *pstat = pcmdhdr->pstat;
	
	clear_bit(_CMD_UPDATE_STA_PS, &pstat->pending_cmd);
	
	set_ps_cmd(priv, pstat);
	
	return TRUE;
}
#endif // CONFIG_RTL_92D_SUPPORT || CONFIG_RTL_92C_SUPPORT

void notify_update_sta_RATid(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
#ifdef CONFIG_RTL_88E_SUPPORT
	add_RATid(priv, pstat);
#else
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_UPDATE_STA_RATID, &pstat->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
		++priv->pshare->nr_h2c_cmd_miss;
#else
		++priv->pshare->nr_cmd_miss;
#endif
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		clear_bit(_CMD_UPDATE_STA_RATID, &pstat->pending_cmd);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_UPDATE_STA_RATID;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	pcmdhdr->pstat = pstat;
	
#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_RTL_92C_SUPPORT)
	rtw_enqueue_cmd(priv, &priv->pshare->h2c_cmd_queue, pcmdhdr);
#else
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
#endif
#endif
}

u8 callback_update_sta_RATid(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct stat_info *pstat = pcmdhdr->pstat;
	
	clear_bit(_CMD_UPDATE_STA_RATID, &pstat->pending_cmd);
	
	add_RATid(priv, pstat);
	
	return TRUE;
}

void notify_del_sta(struct rtl8192cd_priv *priv, struct stat_info *pstat)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_DEL_STA, &pstat->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		clear_bit(_CMD_DEL_STA, &pstat->pending_cmd);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_DEL_STA;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	pcmdhdr->pstat = pstat;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_del_sta(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct stat_info *pstat = pcmdhdr->pstat;
	
	if (test_and_clear_bit(_CMD_DEL_STA, &pstat->pending_cmd)) {
		unsigned char tmpbuf[16];
		
		sprintf((char *)tmpbuf, "%02x%02x%02x%02x%02x%02x",
			pstat->cmn_info.mac_addr[0], pstat->cmn_info.mac_addr[1], pstat->cmn_info.mac_addr[2],
			pstat->cmn_info.mac_addr[3], pstat->cmn_info.mac_addr[4], pstat->cmn_info.mac_addr[5]);
		del_sta(pcmdhdr->priv, tmpbuf);
	}
	
	return TRUE;
}

#ifdef CONFIG_RTL_92C_SUPPORT
void notify_HIQ_NoLimit_change(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_HIQ_NO_LIMIT_CHANGE;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_HIQ_NoLimit_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	u16 value;
	
	value = RTL_R16(RD_CTRL);
	
	if (GET_HW(priv)->HIQ_nolimit_en) {
		if (!(value & HIQ_NO_LMT_EN)) {
			RTL_W16(RD_CTRL, value | HIQ_NO_LMT_EN);
		}
	} else {
		if (value & HIQ_NO_LMT_EN) {
			RTL_W16(RD_CTRL, value & (~HIQ_NO_LMT_EN));
		}
	}
	
	return TRUE;
}
#endif // CONFIG_RTL_92C_SUPPORT

void notify_40M_RRSR_SC_change(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_40M_RRSR_SC_CHANGE, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_40M_RRSR_SC_CHANGE;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_40M_RRSR_SC_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	clear_bit(_CMD_40M_RRSR_SC_CHANGE, priv->pshare->pending_cmd);
	
	if (priv->pshare->is_40m_bw) {
		if (orSTABitMap(&priv->pshare->marvellMapBit) == 0) {
			if (0 != priv->pshare->Reg_RRSR_2) {
				RTL_W8(RRSR+2, priv->pshare->Reg_RRSR_2);
				RTL_W8(0x81b, priv->pshare->Reg_81b);
				priv->pshare->Reg_RRSR_2 = 0;
				priv->pshare->Reg_81b = 0;
			}
		} else {
			if (0 == priv->pshare->Reg_RRSR_2) {
				priv->pshare->Reg_RRSR_2 = RTL_R8(RRSR+2);
				priv->pshare->Reg_81b = RTL_R8(0x81b);
				RTL_W8(RRSR+2, priv->pshare->Reg_RRSR_2 | 0x60);
				RTL_W8(0x81b, priv->pshare->Reg_81b | 0x0E); 
			}
		}
	}
	
	return TRUE;
}

void notify_NAV_prot_len_change(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_NAV_PROT_LEN_CHANGE, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_NAV_PROT_LEN_CHANGE;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_NAV_prot_len_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	clear_bit(_CMD_NAV_PROT_LEN_CHANGE, priv->pshare->pending_cmd);
	
	if (orSTABitMap(&priv->pshare->mimo_ps_dynamic_sta)) {
		RTL_W8(NAV_PROT_LEN, 0x40);
	} else {
		RTL_W8(NAV_PROT_LEN, 0x20);
	}
	
	return TRUE;
}

void notify_slot_time_change(struct rtl8192cd_priv *priv, int use_short)
{
	struct slot_time_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct slot_time_parm *)rtw_zmalloc(sizeof(struct slot_time_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_SLOT_TIME_CHANGE;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->use_short = use_short;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_slot_time_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct slot_time_parm *param = (struct slot_time_parm *)pcmdhdr;
	
	if (param->use_short)
		RTL_W8(SLOT_TIME, 0x09);
	else
		RTL_W8(SLOT_TIME, 0x14);
	
	return TRUE;
}

void notify_NBI_filter_change(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_NBI_FILTER_CHANGE, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_NBI_FILTER_CHANGE;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_NBI_filter_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	clear_bit(_CMD_NBI_FILTER_CHANGE, priv->pshare->pending_cmd);
	
	if (0 == priv->pshare->phw->nbi_filter_on) {
		RTL_W16(rOFDM0_RxDSP, RTL_R16(rOFDM0_RxDSP) & ~ BIT(9));	// NBI off
	} else {
		RTL_W16(rOFDM0_RxDSP, RTL_R16(rOFDM0_RxDSP) | BIT(9));		// NBI on
	}
	
	return TRUE;
}

#ifdef SW_ANT_SWITCH
void notify_antenna_switch(struct rtl8192cd_priv *priv, u8 nextAntenna)
{
	struct cmd_header *pcmdhdr;
	
	priv->pshare->DM_SWAT_Table.NextAntenna = nextAntenna;
	
	if (test_and_set_bit(_CMD_ANTENNA_SWITCH, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_ANTENNA_SWITCH;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_antenna_switch(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	u8 nextAntenna;
	
	clear_bit(_CMD_ANTENNA_SWITCH, priv->pshare->pending_cmd);

	nextAntenna = priv->pshare->DM_SWAT_Table.NextAntenna;
	
#ifdef GPIO_ANT_SWITCH
	PHY_SetBBReg(priv, GPIO_PIN_CTRL, 0x3000, nextAntenna);	
#else		
	if (!priv->pshare->rf_ft_var.antSw_select)
		PHY_SetBBReg(priv, rFPGA0_XA_RFInterfaceOE, 0x300, nextAntenna);
	else
		PHY_SetBBReg(priv, rFPGA0_XB_RFInterfaceOE, 0x300, nextAntenna);
#endif
	
	priv->pshare->DM_SWAT_Table.CurAntenna = nextAntenna;
	
	return TRUE;
}
#endif // SW_ANT_SWITCH

void notify_mp_ctx_background(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_MP_CTX_BACKGROUND, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_MP_CTX_BACKGROUND;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_mp_ctx_background(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	clear_bit(_CMD_MP_CTX_BACKGROUND, priv->pshare->pending_cmd);
	
//	if ((OPMODE & (WIFI_MP_STATE | WIFI_MP_CTX_BACKGROUND | WIFI_MP_CTX_BACKGROUND_STOPPING)) ==
//		(WIFI_MP_STATE | WIFI_MP_CTX_BACKGROUND) )
	mp_ctx(priv, (unsigned char *)"tx-isr");
	
	return TRUE;
}

void notify_IOT_EDCA_switch(struct rtl8192cd_priv *priv, u32 be_edca, u32 vi_edca)
{
	struct switch_edca_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct switch_edca_parm *)rtw_zmalloc(sizeof(struct switch_edca_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_IOT_EDCA_SWITCH;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->be_edca = be_edca;
	param->vi_edca = vi_edca;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_IOT_EDCA_switch(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct switch_edca_parm *param = (struct switch_edca_parm *)pcmdhdr;
	
	if ((u32)-1 != param->be_edca)
		RTL_W32(EDCA_BE_PARA, param->be_edca);
	
	if ((u32)-1 != param->vi_edca)
		RTL_W32(EDCA_VI_PARA, param->vi_edca);
	
	return TRUE;
}

void notify_check_DIG_by_rssi(struct rtl8192cd_priv *priv, u8 rssi)
{
	struct check_dig_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct check_dig_parm *)rtw_zmalloc(sizeof(struct check_dig_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_CHECK_DIG_BY_RSSI;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->rssi = rssi;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_check_DIG_by_rssi(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct check_dig_parm *param = (struct check_dig_parm *)pcmdhdr;
	
	check_DIG_by_rssi(priv, param->rssi);
	
	return TRUE;
}

void notify_set_key(struct net_device *dev, DOT11_SET_KEY *pSetKey, unsigned char *pKey)
{
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	struct set_key_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct set_key_parm *)rtw_zmalloc(sizeof(struct set_key_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_SET_KEY;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->dev = dev;
	memcpy(param->MACAddr, pSetKey->MACAddr, MACADDRLEN);
	param->KeyType = pSetKey->KeyType;
	param->EncType = pSetKey->EncType;
	param->KeyIndex = pSetKey->KeyIndex;
	if (pKey)
		memcpy(param->Key, pKey, sizeof(param->Key));
	else
		memcpy(param->Key, pSetKey->KeyMaterial, sizeof(param->Key));
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_set_key(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct set_key_parm *param = (struct set_key_parm *)pcmdhdr;
	DOT11_SET_KEY Set_Key;
	
	memset((char *)&Set_Key, 0, sizeof(Set_Key));
	memcpy(Set_Key.MACAddr, param->MACAddr, MACADDRLEN);
	Set_Key.KeyType = param->KeyType;
	Set_Key.EncType = param->EncType;
	Set_Key.KeyIndex = param->KeyIndex;
	
	DOT11_Process_Set_Key(param->dev, NULL, &Set_Key, param->Key);
	
	return TRUE;
}

void notify_disconnect_sta(struct net_device *dev, DOT11_DISCONNECT_REQ *pReq)
{
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	struct disconnect_sta_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct disconnect_sta_parm *)rtw_zmalloc(sizeof(struct disconnect_sta_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_DISCONNECT_STA;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->dev = dev;
	memcpy(&param->DisconnectReq, pReq, sizeof(DOT11_DISCONNECT_REQ));
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_disconnect_sta(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct disconnect_sta_parm *param = (struct disconnect_sta_parm *)pcmdhdr;
	struct iw_point wrq;
	
	wrq.pointer = (caddr_t)&param->DisconnectReq;
	wrq.length = sizeof(DOT11_DISCONNECT_REQ);
	
	__DOT11_Process_Disconnect_Req(param->dev, &wrq);
	
	return TRUE;
}

void notify_indicate_MIC_failure(struct net_device *dev, struct stat_info *pstat)
{
	struct rtl8192cd_priv *priv = GET_DEV_PRIV(dev);
	struct indicate_mic_failure_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct indicate_mic_failure_parm *)rtw_zmalloc(sizeof(struct indicate_mic_failure_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_INDICATE_MIC_FAILURE;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->dev = dev;
	if (pstat) {
		param->indicate_upper_layer = 1;
		memcpy(param->MACAddr, pstat->cmn_info.mac_addr, MACADDRLEN);
	} else {
		param->indicate_upper_layer = 0;
	}
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_indicate_MIC_failure(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct indicate_mic_failure_parm *param = (struct indicate_mic_failure_parm *)pcmdhdr;
	struct stat_info *pstat = NULL;
	
	if (param->indicate_upper_layer) {
		pstat = get_stainfo(priv, param->MACAddr); // indicate MIC failure whether STA exist.
	}
	
	__DOT11_Indicate_MIC_Failure(param->dev, pstat);
	
	return TRUE;
}

void notify_indicate_MIC_failure_clnt(struct rtl8192cd_priv *priv, unsigned char *sa)
{
	struct indicate_mic_failure_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct indicate_mic_failure_parm *)rtw_zmalloc(sizeof(struct indicate_mic_failure_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_INDICATE_MIC_FAILURE_CLNT;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	memcpy(param->MACAddr, sa, MACADDRLEN);
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_indicate_MIC_failure_clnt(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct indicate_mic_failure_parm *param = (struct indicate_mic_failure_parm *)pcmdhdr;
	
	__DOT11_Indicate_MIC_Failure_Clnt(priv, param->MACAddr);
	
	return TRUE;
}

#ifdef RTK_BR_EXT
void notify_mac_clone(struct rtl8192cd_priv *priv, unsigned char *addr)
{
	struct mac_clone_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct mac_clone_parm *)rtw_zmalloc(sizeof(struct mac_clone_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_MAC_CLONE;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	memcpy(param->MACAddr, addr, MACADDRLEN);
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

extern void mac_clone(struct rtl8192cd_priv *priv, unsigned char *addr);
u8 callback_mac_clone(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct mac_clone_parm *param = (struct mac_clone_parm *)pcmdhdr;
	
	mac_clone(priv, param->MACAddr);
	priv->macclone_completed = 1;
	
	return TRUE;
}
#endif // RTK_BR_EXT

void notify_recv_mgnt_frame(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct recv_mgnt_frame_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct recv_mgnt_frame_parm *)rtw_zmalloc(sizeof(struct recv_mgnt_frame_parm));
	if (NULL == param) {
		++priv->pshare->nr_rx_mgt_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_RECV_MGNT_FRAME;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->pfrinfo = pfrinfo;
	
	rtw_enqueue_cmd(priv, &priv->pshare->rx_mgt_queue, pcmdhdr);
}

u8 callback_recv_mgnt_frame(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct recv_mgnt_frame_parm *param = (struct recv_mgnt_frame_parm *)pcmdhdr;
	
	rtl8192cd_rx_mgntframe(priv, NULL, param->pfrinfo);
	
	return TRUE;
}

#ifdef CONFIG_RTL_WAPI_SUPPORT
void notify_recv_wai_frame(struct rtl8192cd_priv *priv, struct rx_frinfo *pfrinfo)
{
	struct recv_mgnt_frame_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct recv_mgnt_frame_parm *)rtw_zmalloc(sizeof(struct recv_mgnt_frame_parm));
	if (NULL == param) {
		++priv->pshare->nr_rx_mgt_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_RECV_WAI_FRAME;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->pfrinfo = pfrinfo;
	
	rtw_enqueue_cmd(priv, &priv->pshare->rx_mgt_queue, pcmdhdr);
}

u8 callback_recv_wai_frame(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct recv_mgnt_frame_parm *param = (struct recv_mgnt_frame_parm *)pcmdhdr;
	struct rx_frinfo *pfrinfo = param->pfrinfo;
	unsigned char *pframe = get_pframe(pfrinfo);
	struct stat_info *pstat;

#ifdef WDS
	if (pfrinfo->to_fr_ds == 3) {
		pstat = get_stainfo(priv, GetAddr2Ptr(pframe));
	} else
#endif
	{
#ifdef A4_STA
		if (pfrinfo->to_fr_ds == 3 &&  priv->pshare->rf_ft_var.a4_enable)
			pstat = get_stainfo(priv, GetAddr2Ptr(pframe));
		else
#endif
	                pstat = get_stainfo(priv, pfrinfo->sa);
	}
	
	if (!pstat || wapiHandleRecvPacket(pfrinfo, pstat) == FAILED) {
		rtl_kfree_skb(priv, pfrinfo->pskb, _SKB_RX_);
	}
	
	return TRUE;
}
#endif // CONFIG_RTL_WAPI_SUPPORT

#ifdef CONFIG_RTL_88E_SUPPORT
void notify_tx_report_change(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	if (test_and_set_bit(_CMD_TX_REPORT_CHANGE, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_TX_REPORT_CHANGE;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_tx_report_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	clear_bit(_CMD_TX_REPORT_CHANGE, priv->pshare->pending_cmd);
	
	if (0 == priv->pshare->total_assoc_num) {
		priv->pshare->txRptMacid = 0;
		RTL8188E_SuspendTxReport(priv);
	} else
		RTL8188E_AssignTxReportMacId(priv);
	
	return TRUE;
}

void notify_tx_report_interval_change(struct rtl8192cd_priv *priv, u16 interval)
{
	struct cmd_header *pcmdhdr;
	
	priv->pshare->txRptTime = interval;
	
	if (test_and_set_bit(_CMD_TX_REPORT_INTERVAL_CHANGE, priv->pshare->pending_cmd) != 0)
		return;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_TX_REPORT_INTERVAL_CHANGE;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_tx_report_interval_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	clear_bit(_CMD_TX_REPORT_INTERVAL_CHANGE, priv->pshare->pending_cmd);
	
	RTL_W16(REG_88E_TXRPT_TIM, priv->pshare->txRptTime);
	
	return TRUE;
}

void notify_macid_pause_change(struct rtl8192cd_priv *priv, u16 macid, u16 pause)
{
	struct macid_pause_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct macid_pause_parm *)rtw_zmalloc(sizeof(struct macid_pause_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_MACID_PAUSE_CHANGE;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->macid = macid;
	param->pause = pause;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_macid_pause_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct macid_pause_parm *param = (struct macid_pause_parm *)pcmdhdr;
	
	__RTL8188E_MACID_PAUSE(priv, param->pause, param->macid);
	
	return TRUE;
}

void notify_macid_no_link_change(struct rtl8192cd_priv *priv, u16 macid, u16 nolink)
{
	struct macid_nolink_parm *param;
	struct cmd_header *pcmdhdr;
	
	param = (struct macid_nolink_parm *)rtw_zmalloc(sizeof(struct macid_nolink_parm));
	if (NULL == param) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	// Initialize command header
	pcmdhdr = &param->header;
	pcmdhdr->cmd_code = _CMD_MACID_NO_LINK_CHANGE;
	pcmdhdr->cmd_len = sizeof(*param);
	pcmdhdr->priv = priv;
	
	// Initialize command-specific parameter
	param->macid = macid;
	param->nolink = nolink;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_macid_no_link_change(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct macid_nolink_parm *param = (struct macid_nolink_parm *)pcmdhdr;
	
	__RTL8188E_MACID_NOLINK(priv, param->nolink, param->macid);
	
	return TRUE;
}
#endif // CONFIG_RTL_88E_SUPPORT

#if defined(WIFI_WPAS_CLI) || defined(RTK_NL80211)
void notify_wpas_join(struct rtl8192cd_priv *priv)
{
	struct cmd_header *pcmdhdr;
	
	pcmdhdr = (struct cmd_header *)rtw_zmalloc(sizeof(struct cmd_header));
	if (NULL == pcmdhdr) {
		++priv->pshare->nr_cmd_miss;
		DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
		return;
	}
	
	pcmdhdr->cmd_code = _CMD_WPAS_JOIN;
	pcmdhdr->cmd_len = sizeof(struct cmd_header);
	pcmdhdr->priv = priv;
	
	rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
}

u8 callback_wpas_join(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	start_clnt_join(priv);

	return TRUE;
}
#endif // WIFI_WPAS_CLI

#ifdef RTK_NL80211
void notify_cfg_evt(struct rtl8192cd_priv *priv, unsigned char *mac, int event, unsigned char *extra)
{
	struct indicate_cfg_evt_parm *param;
	struct cmd_header *pcmdhdr;

	if ( priv->dev->flags & IFF_UP ) {
		printk("notify_cfg_evt ++\n");

		param = (struct indicate_cfg_evt_parm *)rtw_zmalloc(sizeof(struct indicate_cfg_evt_parm));
		if (NULL == param) {
			++priv->pshare->nr_cmd_miss;
			DEBUG_ERR("[%s] out of memory!!\n", __FUNCTION__);
			return;
		}
		
		// Initialize command header
		pcmdhdr = &param->header;
		pcmdhdr->cmd_code = _CMD_CFG_IND;
		pcmdhdr->cmd_len = sizeof(*param);
		pcmdhdr->priv = priv;
		
		// Initialize command-specific parameter
		param->evt = event;
		param->extra = extra;
		if (mac != NULL)
			memcpy(param->mac, mac, 6);
		
		rtw_enqueue_cmd(priv, &priv->pshare->cmd_queue, pcmdhdr);
		printk("notify_cfg_evt --\n");
	}
}

u8 callback_notify_cfg_evt(struct rtl8192cd_priv *priv, struct cmd_header *pcmdhdr)
{
	struct indicate_cfg_evt_parm *param = (struct indicate_cfg_evt_parm *)pcmdhdr;
	
	printk("callback_notify_cfg_evt ++\n");

	event_indicate_cfg80211(pcmdhdr->priv, param->mac, param->evt, param->extra);

	printk("callback_notify_cfg_evt --\n");

	return TRUE;
}
#endif // RTK_NL80211

