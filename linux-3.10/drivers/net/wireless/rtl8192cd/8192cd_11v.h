
#ifndef	_8192CD_11V_H_
#define _8192CD_11V_H_



#ifdef DOT11V_DEBUG
#define DOT11VDEBUG(fmt, args...) panic_printk("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#else
#define DOT11VDEBUG(fmt, args...)
#endif

//#define DOT11V_TRACE
#ifdef DOT11V_TRACE
#define DOT11VTRACE(fmt, args...) panic_printk("[%s %d]"fmt,__FUNCTION__,__LINE__,## args)
#else
#define DOT11VTRACE(fmt, args...)
#endif

#ifndef _8192CD_11V_C_
#define EXTERN  extern
#else
#define EXTERN
#endif

#define _WNM_TSF_INFO_                         				1
#define _WNM_CONDENSED_COUNTRY_STRING_ 			2
#define _WNM_BSS_TRANS_CANDIDATE_PREFRENCE_   	3	// subelement
#define _WNM_BSS_TERMINATION_DURATION_  		4
#define _WNM_BEARING_                     					5
#define _WNM_MEASUREMENT_PILOT_          			66
#define _WNM_RRM_ENABLED_CAPABILITIES_   			70
#define _WNM_MULTIPLE_BSSID_             				71

#define TRANS_LIST_PROC_LEN	50
#define MAX_PREFERRED_VAL 		255
#define NEIGHBOR_REPORT_SIZE 	13	 // size of neighbor report structure


EXTERN unsigned char getPreferredVal(struct rtl8192cd_priv *priv, unsigned char channel_utilization, unsigned char rcpi, bool excluded);
EXTERN void OnBSSTransQuery(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char *pframe, int frame_len);
EXTERN void OnBSSTransRsp(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char *pframe, int frame_len);
EXTERN int issue_BSS_Trans_Req(struct rtl8192cd_priv *priv, unsigned char *da, unsigned char dialog_token,
		unsigned char *preferred_BSSID, unsigned char preferred_channel);
EXTERN unsigned char * construct_neighbor_report_ie(unsigned char *pbuf, unsigned int *frlen, struct dot11k_neighbor_report * report);
EXTERN void set_staBssTransCap(struct stat_info *pstat, unsigned char *pframe, int frameLen, unsigned short ie_offset);
EXTERN void reset_staBssTransStatus(struct stat_info *pstat);
EXTERN void WNM_ActionHandler(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char *pframe, int frame_len);
#ifdef DOT11K
EXTERN void set_BssTransPara(struct rtl8192cd_priv *priv, unsigned char *tmpbuf);
#endif
#ifdef RTK_MULTI_AP
EXTERN void addEntryTransitionList(struct rtl8192cd_priv *priv, const struct target_transition_list *list);
EXTERN void deleteEntryTransitionList(struct rtl8192cd_priv * priv,const unsigned char * macAddress);
EXTERN void deleteEntryBlockList(struct rtl8192cd_priv *priv, const unsigned char* macAddress);
EXTERN void expireEntryBlockList(struct rtl8192cd_priv *priv);
EXTERN struct assoc_control_block_list *getEntryBlockList(struct rtl8192cd_priv * priv,const unsigned char * macAddress);
EXTERN int operate_BTM_disallowed_sta(struct rtl8192cd_priv *priv, unsigned char * tmpbuf);
EXTERN int update_assoc_control_block_list(struct rtl8192cd_priv *priv, unsigned char *tmpbuf);
EXTERN int update_agent_steering_policy(struct rtl8192cd_priv *priv, unsigned char *tmpbuf);
EXTERN void rtk_multi_ap_agent_steering_trigger(struct rtl8192cd_priv *priv);
#endif
EXTERN void send_bss_trans_event(struct rtl8192cd_priv *priv, struct stat_info *pstat, int i);
EXTERN void process_BssTransReq(struct rtl8192cd_priv *priv);
EXTERN void BssTrans_DiassocTimer(struct rtl8192cd_priv *priv);
EXTERN void BssTrans_ExpiredTimer(struct rtl8192cd_priv *priv);
EXTERN void BssTrans_ValidatePrefListTimer(struct rtl8192cd_priv *priv);
EXTERN void BssTrans_TerminationTimer(struct rtl8192cd_priv *priv);
#ifdef ROAMING_SUPPORT
EXTERN unsigned char issue_ROAMING_BSS_Trans_Req(struct rtl8192cd_priv * priv, unsigned char * data);
#endif
EXTERN unsigned char *construct_btm_req_mbo_ie(struct rtl8192cd_priv *priv, unsigned char *pbuf, unsigned int *frlen, struct target_transition_list *entry);


#ifdef CONFIG_RTL_PROC_NEW
EXTERN int rtl8192cd_proc_transition_list_read(struct seq_file *s, void *data);
#else
EXTERN int rtl8192cd_proc_transition_list_read(char *buf, char **start, off_t offset, int length, int *eof, void *data);
#endif

#ifdef __ECOS
EXTERN int rtl8192cd_proc_transition_list_write(char *tmp, void *data)
#else
EXTERN int rtl8192cd_proc_transition_list_write(struct file *file, const char *buffer, unsigned long count, void *data);
#endif

#ifdef CONFIG_IEEE80211V_CLI
EXTERN void OnBSSTransReq(struct rtl8192cd_priv *priv, struct stat_info *pstat, unsigned char *pframe, int frame_len);
#endif

EXTERN unsigned char isDualBandClient(struct rtl8192cd_priv *priv, unsigned char* addr);

#endif
