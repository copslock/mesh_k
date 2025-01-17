#ifndef	RTL865X_OUTPUTQUEUE_H
#define	RTL865X_OUTPUTQUEUE_H
#include <linux/if.h>

#if defined(CONFIG_RTL_HW_QOS_SUPPORT)&&defined(CONFIG_OPENWRT_SDK)
#include <linux/netdevice.h>
#endif

#if	!defined(CONFIG_RTL_LAYERED_ASIC_DRIVER)
#define	RTL865XC_MNQUEUE_OUTPUTQUEUE		1
#define	RTL865XC_QOS_OUTPUTQUEUE				1

typedef struct rtl865xC_outputQueuePara_s {

	uint32	ifg;							/* default: Bandwidth Control Include/exclude Preamble & IFG */
	uint32	gap;							/* default: Per Queue Physical Length Gap = 20 */
	uint32	drop;						/* default: Descriptor Run Out Threshold = 500 */

	uint32	systemSBFCOFF;				/*System shared buffer flow control turn off threshold*/
	uint32	systemSBFCON;				/*System shared buffer flow control turn on threshold*/

	uint32	systemFCOFF;				/* system flow control turn off threshold */
	uint32	systemFCON;					/* system flow control turn on threshold */

	uint32	portFCOFF;					/* port base flow control turn off threshold */
	uint32	portFCON;					/* port base flow control turn on threshold */	

	uint32	queueDescFCOFF;				/* Queue-Descriptor=Based Flow Control turn off Threshold  */
	uint32	queueDescFCON;				/* Queue-Descriptor=Based Flow Control turn on Threshold  */

	uint32	queuePktFCOFF;				/* Queue-Packet=Based Flow Control turn off Threshold  */
	uint32	queuePktFCON;				/* Queue-Packet=Based Flow Control turn on Threshold  */
}	rtl865xC_outputQueuePara_t;
#endif
#if defined(CONFIG_RTL_HW_QOS_SUPPORT) && !defined(CONFIG_OPENWRT_SDK)	
#define CONFIG_RTL_HW_QOS_SP_PRIO		1
#endif

#define	MAX_QOS_RULE_NUM		10
#if defined (CONFIG_OPENWRT_SDK)
#define	MAX_MARK_NUM_PER_DEV	10
#else
//#define	MAX_MARK_NUM_PER_DEV	5	// Note: not include the default mark(0) and should be <= (hw queue number - 1(for default))
#if defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F)
#define	MAX_MARK_NUM_PER_DEV	8
#else 
#define	MAX_MARK_NUM_PER_DEV	6
#endif 
#endif
#define	IPPROTO_ANY		256
#define	IPPROTO_BOTH		257

#define	QOS_DEF_QUEUE		0x4
#define	QOS_VALID_MASK	0x2
#define	QOS_TYPE_MASK		0x1
#define	QOS_TYPE_STR		0x0	/*0x0|QOS_VALID_MASK*/
#define	QOS_TYPE_WFQ		0x1	/*0x1|QOS_VALID_MASK*/

#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
#define	EGRESS_BANDWIDTH_GRANULARITY			0xC00	/* 	8*1024	 */
#define	EGRESS_BANDWIDTH_GRANULARITY_API        0x400
#else
#if defined(CONFIG_RTL_8197F)
#define	EGRESS_BANDWIDTH_GRANULARITY			0x400	/* 	1*1024	 */
#define	EGRESS_BANDWIDTH_GRANULARITY_BITMASK	0x3ff
#else
#define	EGRESS_BANDWIDTH_GRANULARITY			0x10000	/* 	64*1024	 */
#define	EGRESS_BANDWIDTH_GRANULARITY_BITMASK	0xffff
#endif
#endif

#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
#define	EGRESS_BANDWIDTH_GRANULARITY_BITLEN	13
#define EGRESS_BANDWIDTH_GRANULARITY_BYTELEN EGRESS_BANDWIDTH_GRANULARITY_BITLEN-3
#define	INGRESS_BANDWIDTH_GRANULARITY_BITLEN	13
#define	INGRESS_BANDWIDTH_GRANULARITY_BYTELEN	INGRESS_BANDWIDTH_GRANULARITY_BITLEN-3
#else
#if defined(CONFIG_RTL_8197F)
#define	EGRESS_BANDWIDTH_GRANULARITY_BITLEN	10
#define EGRESS_BANDWIDTH_GRANULARITY_BYTELEN EGRESS_BANDWIDTH_GRANULARITY_BITLEN-3
#define	INGRESS_BANDWIDTH_GRANULARITY_BITLEN	10
#define	INGRESS_BANDWIDTH_GRANULARITY_BYTELEN	INGRESS_BANDWIDTH_GRANULARITY_BITLEN-3
#else
#define	EGRESS_BANDWIDTH_GRANULARITY_BITLEN	16
#define EGRESS_BANDWIDTH_GRANULARITY_BYTELEN EGRESS_BANDWIDTH_GRANULARITY_BITLEN-3
#define	INGRESS_BANDWIDTH_GRANULARITY_BITLEN	14
#define	INGRESS_BANDWIDTH_GRANULARITY_BYTELEN	INGRESS_BANDWIDTH_GRANULARITY_BITLEN-3
#endif
#endif

#define	EGRESS_WFQ_MAX_RATIO			0x80
#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
extern unsigned char tc_hw_qos;	
#endif

#if 0
#define QOS_DEBUGP		printk
#else
#define QOS_DEBUGP(format, args...)
#endif

#if 0
#define QOS_RULE_DEBUGP		printk
#else
#define QOS_RULE_DEBUGP(format, args...)
#endif

/* priority decision array index */
enum PriDecIdx
{
	PORT_BASE	= 0,
	D1P_BASE, 
	DSCP_BASE, 
	ACL_BASE, 
	NAT_BASE,
	#if defined(CONFIG_RTL_8197F)
	VID_BASE,
	#endif
	PRI_TYPE_NUM,
};

typedef	unsigned int	bwu;

typedef struct {
	/*	classify	*/
	unsigned int protocol;
	ipaddr_t	local_ip_start;
	ipaddr_t 	local_ip_end;
	ipaddr_t 	remote_ip_start;
	ipaddr_t 	remote_ip_end;
	unsigned short lo_port_start;
	unsigned short lo_port_end;
	unsigned short re_port_start;
	unsigned short re_port_end;

	/*	tc	*/
	uint32		mark;
	unsigned char	prio;
	unsigned char	rate;
} rtl865x_qos_entry_t, *rtl865x_qos_entry_p;

typedef struct {
	uint8		dscpRemark;
	uint8		vlanPriRemark;
	uint16		flags;

	char			ifname[IFNAMSIZ];
	uint32		queueId;			/*	identify outputQueue	*/
	uint32		handle;

	bwu			ceil;
	bwu			burst;
	bwu			bandwidth;		/* average bandwidth, unit kbps	*/
#if defined (CONFIG_RTL_HW_QOS_SP_PRIO)	|| defined(CONFIG_OPENWRT_SDK)
	bwu			prio;
#endif
} rtl865x_qos_t, *rtl865x_qos_p;

typedef	rtl865x_qos_entry_t		QOS_T;
typedef	rtl865x_qos_entry_p		QOS_Tp;

typedef struct _rtl865x_qos_rule_t{
	char			inIfname[IFNAMSIZ];
	char			outIfname[IFNAMSIZ];
	rtl865x_AclRule_t *rule;
	uint32			mark;
	uint32			handle;
	struct _rtl865x_qos_rule_t	*next;
} rtl865x_qos_rule_t, *rtl865x_qos_rule_p;

typedef struct {
	uint32		mark;		/*	mark==0 means entry not used	*/
#if defined (CONFIG_OPENWRT_SDK)	
	uint32		mask;
#endif
	uint32		priority;
	uint8 dscpRemark;
	uint8 vlanpriRemark;
	uint8 		version;
#if defined(CONFIG_IPV6)
	uint8 		v6DscpRemark;
#endif
} rtl_qos_mark_info_t, *rtl_qos_mark_info_p;

#if defined (CONFIG_RTL_HW_QOS_SUPPORT)&& defined (CONFIG_OPENWRT_SDK)

typedef struct {
	uint32		mark;		/*	mark==0 means entry not used	*/
	uint32		handle;
} rtl_qos_mark2handl_info_t, *rtl_qos_mark2handle_info_p;
#define RTL_MARK2HANDL_ADD_FLAG 	0x1
#define RTL_MARK2HANDL_DEL_FLAG 	0x2
#define RTL_MARK2HANDL_FLUSH_FLAG 	0x4

#define RTL_MARK2PRIO_FLUSH_FLAG 	0x4

#endif

// for rtk cmd
typedef struct {
	int32		mark;		/*	mark == 0 means entry not used	*/
	uint8		priority;
	uint8		delflag;
} rtk_cmd_mark_t, *rtk_cmd_mark_p;
int rtk_cmd_config_mark2priority(uint8 *netIfName, uint8 count, rtk_cmd_mark_p p_rtk_mark);

int32 rtl865x_qosSetBandwidth(uint8 *netIfName, uint32 bps);
int32 rtl865x_qosFlushBandwidth(uint8 *netIfName);
int32 rtl865x_qosGetPriorityByHandle(uint8 *priority, uint32 handle);
int32 rtl865x_qosProcessQueue(uint8 *netIfName, rtl865x_qos_t *qosInfo);
int32 rtl865x_setRule2HandleMapping(uint32 ifIdx, uint32 idx, uint32 handle);
int32 rtl865x_qosAddMarkRule(rtl865x_qos_rule_t *rule);
int32 rtl865x_qosFlushMarkRule(void);
int32 rtl865x_qosCheckNaptPriority(rtl865x_AclRule_t *qosRule);
int32 rtl865x_closeQos(uint8 *netIfName);
int32 rtl865x_registerQosCompFunc(int8 (*p_cmpFunc)(rtl865x_qos_t	*entry1, rtl865x_qos_t	*entry2));	
int __init rtl865x_initOutputQueue(uint8 **netIfName);
void __exit rtl865x_exitOutputQueue(void);
#if defined(CONFIG_RTL_HW_QOS_SUPPORT)&&defined(CONFIG_OPENWRT_SDK)
void rtl865x_HwQosProcess(struct net_device *dev, int flag);
#endif
int32 rtl865x_qosArrangeRuleByNetif(void);
#if defined(CONFIG_RTL_PROC_DEBUG)
int32 rtl865x_show_allQosAcl(struct seq_file *s);
#endif
extern int32 rtl865x_qosArrangeRuleByNetif(void);
int32 rtl_qosGetPriorityByMark(uint8 *netIfName, int32 mark);

int32 rtl_qosGetPriorityByVid(int32 vid, int32 mark);
#if defined (CONFIG_OPENWRT_SDK)	
int32 rtl_qosSetPriorityByMark(uint8 *netIfName, int32 mark, int32 handler, int32 mask,int32 enable);
#else
int32 rtl_qosSetPriorityByMark(uint8 *netIfName, int32 mark, int32 handler, int32 enable);

#endif
extern u8 netIfNameArray[NETIF_NUMBER][IFNAMSIZ];
extern rtl865x_qos_rule_t		*rtl865x_qosRuleHead;

#if defined(CONFIG_RTK_QOS_FOR_CABLE_MODEM)
void rtl_initQosForCableModem(void);
#endif

#if defined (CONFIG_RTL_HW_QOS_SUPPORT) && defined(CONFIG_RTL_QOS_PATCH)
#define	DEF_QUEUE_NUM					1
#define	MAX_QOS_PATCH_QUEUE_NUM	2
void rtl865x_reinitOutputQueuePatchForQoS(uint32 qosEnabled);
#endif

#if defined (CONFIG_RTL_HW_QOS_SUPPORT) 	// sync from voip customer for multiple ppp
int32 rtl865x_qosFlushMarkRuleByDev(uint8 *netIfName);
int32 rtl865x_qosRearrangeRule(void);
#endif

#if defined(CONFIG_OPENWRT_SDK)
#define DEFAULT_PRIORITY_PATCH 1
#endif
#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
extern int rtl83xx_qosFlushPortBasePri(void);
extern int rtl83xx_qosFlushDscpBasePri(void);
extern int rtl83xx_qosFlush1pBasePri(void);
extern int rtl83xx_qosFlushAclBasePri(unsigned int memberPort);
extern int rtl83xx_qosRestorePriDecision(unsigned int flag);

extern int rtl83xx_qosSetQueueNum(unsigned int port, unsigned int queue_num);
extern int rtl83xx_qosGetQueueNum(unsigned int port, unsigned int *queue_num);

extern int rtl83xx_qosSetPortBasePri(unsigned int port, unsigned int prio);
extern int rtl83xx_qosGetPortBasePri(unsigned int port, unsigned int *prio);

extern int rtl83xx_qosSetDscpBasePri(unsigned int dscp, unsigned int prio);
extern int rtl83xx_qosGetDscpBasePri(unsigned int dscp, unsigned int *prio);

extern int rtl83xx_qosSet1pBasePri(unsigned int dot1p_pri, unsigned int prio);
extern int rtl83xx_qosGet1pBasePri(unsigned int dot1p_pri, unsigned int *prio);

extern int rtl83xx_qosSetQueueType(unsigned int port, unsigned int queue_id, unsigned int weight);
extern int rtl83xx_qosGetQueueType(unsigned int port, unsigned int queue_id, unsigned int *weight);

extern int rtl83xx_qosSetPriToQid(unsigned int queue_num, unsigned int pri, unsigned int qid);
extern int rtl83xx_qosGetPriToQid(unsigned int queue_num, unsigned int pri, unsigned int *qid);

extern int rtl83xx_qosGetQueueBwCtrl(unsigned int port, unsigned int *enable);
extern int rtl83xx_qosSetQueueRate(unsigned int port, unsigned int portmask, unsigned int queue_id, unsigned int bandwidth);
extern int rtl83xx_qosGetQueueRate(unsigned int port, unsigned int queue_id, unsigned int *bandwidth);

extern int rtl83xx_qosSetPortBandwidth(unsigned int port, unsigned int bandwidth);
extern int rtl83xx_qosGetPortBandwidth(unsigned int port, unsigned int *bandwidth);

extern int rtl83xx_qosSetPriDecision(unsigned int priSrc, unsigned int decisionPri);
extern int rtl83xx_qosGetPriDecisionAll(unsigned int priDec[], unsigned int len);

extern int rtl83xx_dscp_remark_set(unsigned int remarkPort, unsigned int sysPri, unsigned int dscpValue);
extern int rtl83xx_dscp_remark_get(unsigned int int_pri, unsigned int *redscp);
extern int rtl83xx_dscp_remark_status_get(unsigned int *status);

extern int rtl83xx_1p_remark_set(unsigned int remarkPort, unsigned int sysPri, unsigned int dot1pValue);
extern int rtl83xx_1p_remark_status_get(unsigned int port, unsigned int *status);
extern int rtl83xx_1p_remark_get(unsigned int int_pri, unsigned int *re1p);

extern int rtl83xx_remark_confg_get(int port, int *dscpRemkStat, int *d1pRemkStas);
extern int rtl83xx_remark_get(int sysPri, int *dscpRemk, int *d1pRemk);

extern void rtl83xx_flush_dscp_remark_setting(void);
extern void rtl83xx_disable_dscp_remark_setting(void);
extern void rtl83xx_flush_1p_remark_setting(void);
extern void rtl83xx_disable_1p_remark_setting(void);
extern int rtl83xx_qos_init(void);
extern void rtl83xx_setFlowControl(int qosEnable);
extern void rtl83xx_rechange_qos(unsigned int flag);

extern int rtl83xx_qosFlushBandwidth(unsigned int memberPort);
extern int rtl83xx_closeQos(unsigned int memberPort, unsigned int wanPortMask);

extern int hw_83xx_qos_init_netlink(void);

/* enum Priority Selection Types */
enum PRIDECISION
{
	PRIDEC_PORT = 0,
	PRIDEC_ACL,
	PRIDEC_DSCP,
	PRIDEC_1Q,
	PRIDEC_1AD,
	PRIDEC_CVLAN,
	PRIDEC_DA,
	PRIDEC_SA,
	PRIDEC_END,
};
#endif
#endif
