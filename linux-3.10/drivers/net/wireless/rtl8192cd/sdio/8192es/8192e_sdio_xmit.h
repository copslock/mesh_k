/*
 *  Header files defines some SDIO TX inline routines
 *
 *  Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef _8192E_SDIO_XMIT_H_
#define _8192E_SDIO_XMIT_H_

#include "./8192e_sdio.h"

#define NR_XMITFRAME		(512)
// stop netdev tx queue if the remaining amount of xmitframe is equal to threshold when allocating xmitframe
#define STOP_NETIF_TX_QUEUE_THRESH		40
// restart netdev tx queue if the available amount of xmitframe is equal to threshold when recycling xmitframe
#define WAKE_NETIF_TX_QUEUE_THRESH		80
#define MAX_TX_AGG_PKT_NUM	(16)

#ifdef CONFIG_SDIO_TX_AGGREGATION
#define MAX_XMITBUF_PKT	16//10
#define MAX_XMITBUF_SZ		(1536*MAX_XMITBUF_PKT) // 15k 1536*10
#define NR_XMITBUFF		8//(16)
#else
#define MAX_XMITBUF_SZ		(2048)
#define NR_XMITBUFF		(16)
#endif

#define MAX_XMIT_EXTBUF_SZ	(1536)
#define NR_XMIT_EXTBUFF		(32)

#define NR_URG_XMITBUFF		(16)

#ifdef SDIO_AP_OFFLOAD
#define MAX_BCN_XMITBUF_SZ	((RTL8192CD_NUM_VWLAN+1)*1024)
#else
#define MAX_BCN_XMITBUF_SZ	(2048)
#endif

#define NR_BCN_XMITBUFF		(2)

#define XMITBUF_ALIGN_SZ	(512)

#define TXDESC_SIZE		(40)

#define PACKET_OFFSET_SZ	(8)

#define DEFAULT_TXPKT_OFFSET	(0)

#define TXAGG_DESC_ALIGN_SZ	(8)

#define HIQ_NOLIMIT_CHECK_INTERVAL	RTL_MILISECONDS_TO_JIFFIES(10)

#define STA_TS_LIMIT	(16*1024) // 16*1024 us == 16 ms

#define MAX_TX_MC_PKT	10

#define MAX_TCP_ACK_AGG		10
#define TCP_ACK_TIMEOUT		msecs_to_jiffies(10)

//#define TXDMA_ERROR_DEBUG
//#define TXDMA_ERROR_PAGE_DEBUG
//#define TXDMA_ERROR_LLT_DEBUG

#ifndef CONFIG_SDIO_TX_INTERRUPT
#undef TXDMA_ERROR_PAGE_DEBUG
#endif

#ifdef CONFIG_IRQ_LEVEL_XMIT_LOCK
#define xmit_preempt_disable(flags)	local_irq_save(flags)
#define xmit_preempt_enable(flags)	local_irq_restore(flags)
#define xmit_lock(lock, irql)			_enter_critical(lock, irql)
#define xmit_unlock(lock, irql)			_exit_critical(lock, irql)
#else
#define xmit_preempt_disable(flags)	do { local_bh_disable(); (void)(flags); } while (0)
#define xmit_preempt_enable(flags)	do { local_bh_enable(); (void)(flags); } while (0)
#define xmit_lock(lock, irql)			_enter_critical_bh(lock, irql)
#define xmit_unlock(lock, irql)			_exit_critical_bh(lock, irql)
#endif

enum {
	TXPATH_HARD_START_XMIT = 0,
	TXPATH_SLOW_PATH,
	TXPATH_FIRETX
};

enum {
	TXOQT_TYPE_AC = 0,
	TXOQT_TYPE_NOAC = 1,
	MAX_TXOQT_TYPE
};

enum {
	SDIO_TX_THREAD = 0,
	SDIO_TX_ISR,
	SDIO_RX_ISR,
};

enum XMIT_BUF_FLAGS {
	XMIT_BUF_FLAG_REUSE = BIT0,
};

enum XMIT_THREAD_STATE {
	XMIT_THREAD_STATE_RUNNING = BIT0,
};

struct tx_desc {
	unsigned int Dword0;
	unsigned int Dword1;
	unsigned int Dword2;
	unsigned int Dword3;
	unsigned int Dword4;
	unsigned int Dword5;
	unsigned int Dword6;
	unsigned int Dword7;
	unsigned int Dword8;
	unsigned int Dword9;
};

struct xmit_buf {
	_list list;
	_list tx_xmitbuf_list;

	u16	ext_tag; // 0: Normal xmitbuf, 1: extension xmitbuf.

	u8	*pallocated_buf;
	u8	*pkt_head;
	u8	*pkt_data;
	u8	*pkt_tail;
	u8	*pkt_end;

	u32	len;
	u32	flags;
	s32	status;
	
	u8	q_num;
	u8	tx_page_idx;
	u8	pg_num;
	u8	agg_num;
	void    *agg_start_with;

	struct tx_desc_info txdesc_info[MAX_TX_AGG_PKT_NUM];
	
	u8	pkt_offset;	// indicate the num of 8-byte dummy data between TX descriptor and TX packet
	u8	use_hw_queue;	// indicate whether got the ownership to use HW queue
};

struct xmit_frame {
	_list list;

	struct tx_insn txinsn;
	struct rtl8192cd_priv *priv;
};


int _rtw_init_xmit_priv(struct rtl8192cd_priv *priv);
void _rtw_free_xmit_priv(struct rtl8192cd_priv *priv);

struct xmit_buf *rtw_alloc_xmitbuf(struct rtl8192cd_priv *priv, u8 q_num);
s32 rtw_free_xmitbuf(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf);

struct xmit_buf *rtw_alloc_xmitbuf_ext(struct rtl8192cd_priv *priv, u8 q_num);
s32 rtw_free_xmitbuf_ext(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf);

struct xmit_buf *rtw_alloc_urg_xmitbuf(struct rtl8192cd_priv *priv, u8 q_num);
s32 rtw_free_urg_xmitbuf(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf);
int rtw_enqueue_urg_xmitframe(struct rtl8192cd_priv *priv, struct xmit_frame *pxmitframe, int insert_tail);

struct xmit_buf *rtw_alloc_beacon_xmitbuf(struct rtl8192cd_priv *priv);
s32 rtw_free_beacon_xmitbuf(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf);

struct xmit_frame *rtw_alloc_xmitframe(struct rtl8192cd_priv *priv);
s32 rtw_free_xmitframe(struct rtl8192cd_priv *priv, struct xmit_frame *pxmitframe);
void rtw_free_xmitframe_queue(struct rtl8192cd_priv *priv, _queue *pframequeue);

#ifdef CONFIG_TCP_ACK_TXAGG
int rtw_xmit_enqueue_tcpack(struct rtl8192cd_priv *priv, struct tx_insn *txcfg);
void rtw_tcpack_servq_flush(struct rtl8192cd_priv *priv, struct tcpack_servq *tcpackq);
#endif

int rtw_send_xmitframe(struct xmit_frame *pxmitframe);

void rtw_txservq_flush(struct rtl8192cd_priv *priv, struct tx_servq *ptxservq);

void rtw_pspoll_sta_enqueue(struct rtl8192cd_priv *priv, struct stat_info *pstat, int insert_tail);
void rtw_pspoll_sta_delete(struct rtl8192cd_priv *priv, struct stat_info *pstat);
int rtw_enqueue_xmitframe(struct rtl8192cd_priv *priv, struct xmit_frame *pxmitframe, int insert_tail);
struct xmit_frame* rtw_txservq_dequeue(struct rtl8192cd_priv *priv, struct tx_servq *ptxservq);
struct xmit_frame* rtw_dequeue_xmitframe(struct rtl8192cd_priv *priv, int q_num);

void rtw_xmitbuf_aggregate(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf, struct stat_info *pstat, int q_num);

int rtl8192cd_usb_tx_recycle(struct rtl8192cd_priv *priv, struct tx_desc_info* pdescinfo);
void rtl8192cd_usb_cal_txdesc_chksum(struct tx_desc *ptxdesc);

void need_sched_xmit_for_enqueue(struct rtl8192cd_priv *priv, int q_num);
void need_sched_xmit_for_dequeue(struct rtl8192cd_priv *priv, int q_num);
#ifdef CONFIG_SDIO_TX_IN_INTERRUPT
int rtw_check_txoqt(struct rtl8192cd_priv *priv, u8 q_num, u8 agg_num);
#endif
#ifdef CONFIG_SDIO_TX_AGGREGATION
struct xmit_buf* get_usable_pending_xmitbuf(struct rtl8192cd_priv *priv, struct tx_insn* txcfg);
#endif
void enqueue_pending_xmitbuf(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf);
#ifdef CONFIG_SDIO_TX_INTERRUPT
s32 rtl8192es_dequeue_writeport(struct rtl8192cd_priv *priv, int from);
#endif
void rtw_flush_xmit_pending_queue(struct rtl8192cd_priv *priv);
u32 sdio_submit_xmitbuf(struct rtl8192cd_priv *priv, struct xmit_buf *pxmitbuf);

int rtw_xmit_thread(void *context);

#endif // _8192E_SDIO_XMIT_H_

