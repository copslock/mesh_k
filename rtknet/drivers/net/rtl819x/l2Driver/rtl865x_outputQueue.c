/*
 *
 *  Copyright (c) 2011 Realtek Semiconductor Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#include <linux/kconfig.h>
#else
#include <linux/config.h>
#endif
#include <net/rtl/rtl_types.h>
#include <net/rtl/rtl_glue.h>
#include <common/rtl8651_tblDrvProto.h>
#include <common/rtl865x_eventMgr.h>
#include <common/rtl865x_vlan.h>
#include <net/rtl/rtl865x_netif.h>
#include <common/rtl865x_netif_local.h>
#include <net/rtl/rtl865x_outputQueue.h>
//#include "assert.h"
//#include "rtl_utils.h"
#include <common/rtl_errno.h>
#if defined (CONFIG_RTL_LOCAL_PUBLIC)
#include <l3Driver/rtl865x_localPublic.h>
#endif

#ifdef CONFIG_RTL_LAYERED_ASIC_DRIVER
#include <AsicDriver/asicRegs.h>
#include <AsicDriver/rtl865x_asicCom.h>
#include <AsicDriver/rtl865x_asicL2.h>
#else
#include <AsicDriver/asicRegs.h>
#include <AsicDriver/rtl8651_tblAsicDrv.h>
#endif

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
#include <net/pkt_cls.h>
#endif

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
int bridge_qos_enable = 0; //if bridge qos is handle by 8367
int nat_qos_enable = 0; //if nat qos is handled by 8367
#define RTL83XX_OUTPUTQUEUE_SIZE 8
extern void rtl83xx_flush_dscp_remark_setting(void);
extern void rtl83xx_disable_dscp_remark_setting(void);
extern void rtl83xx_disable_1p_remark_setting(void);
extern void rtl83xx_flush_1p_remark_setting(void);
//extern int rtl83xx_qos_init(void);
extern int rtl865x_enableRtl83xxUCToCpu(unsigned int acl_idx);
extern int rtl865x_disableRtl83xxUCToCpu(unsigned int acl_idx);
#endif
uint8	netIfNameArray[NETIF_NUMBER][IFNAMSIZ] = {{0}};
static int8	(*rtl865x_compFunc)(rtl865x_qos_t	*entry1, rtl865x_qos_t	*entry2);
#if defined (CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F)
/*add 7 queue and 8 queue*/

static uint8	priorityMatrix[RTL8651_OUTPUTQUEUE_SIZE][TOTAL_VLAN_PRIORITY_NUM] = 
								{{0,0,0,0,0,0,0,0},	
								{0,0,0,0,5,5,5,5},	
								{0,0,0,0,1,1,5,5},
								{0,0,0,1,2,2,5,5},
								{0,0,0,1,2,3,5,5},
								{0,0,1,2,3,4,5,5},
								{0,0,1,2,3,4,5,6},
								{0,1,2,3,4,5,6,7}
								};
//priority used for not mapping to the same queue  
#else
static uint8	priorityMatrix[RTL8651_OUTPUTQUEUE_SIZE][TOTAL_VLAN_PRIORITY_NUM] = 
								{{0,0,0,0,0,0,0,0},	
								{0,0,0,0,5,5,5,5},	
								{0,0,0,0,1,1,5,5},
								{0,0,0,1,2,2,5,5},
								{0,0,0,1,2,3,5,5},
								{0,0,1,2,3,4,5,5}};

#endif

#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
static int32	queueMatrix[RTL8651_OUTPUTQUEUE_SIZE][RTL83XX_OUTPUTQUEUE_SIZE] =
								{{0, -1, -1, -1, -1, -1, -1, -1},
								{0, -1, -1, -1, -1, -1, -1, 7},
								{0, 5, -1, -1, -1, -1, -1, 7},
								{0, 3, 5, -1, -1, -1, -1, 7},
								{0, 3, 4, 5, -1, -1, -1, 7},
								{0, 2, 3, 4, 5, -1, -1, 7},
								{0, 2, 3, 4, 5 ,6, -1, 7},
								{0, 1, 2, 3, 4, 5 ,6, 7}
								};
#else
#if defined (CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F)
static int32	queueMatrix[RTL8651_OUTPUTQUEUE_SIZE][RTL8651_OUTPUTQUEUE_SIZE] =
								{{0, -1, -1, -1, -1, -1, -1, -1},
								{0, -1, -1, -1, -1, 5, -1, -1},
								{0, 4, -1, -1, -1, 6, -1, -1},
								{0, 3, 4, -1, -1, 6, -1, -1},
								{0, 3, 4, 5, -1, 6, -1, -1},
								{0, 2, 3, 4, 5, 6, -1, -1},
								{0, 2, 3, 4, 5 ,6, 7, -1},
								{0, 1, 2, 3, 4, 5 ,6, 7}
								};

#else
static int32	queueMatrix[RTL8651_OUTPUTQUEUE_SIZE][RTL8651_OUTPUTQUEUE_SIZE] =
								{{0, -1, -1, -1, -1, -1},
								{0, -1, -1, -1, -1, 5},
								{0, 4, -1, -1, -1, 6},
								{0, 3, 4, -1, -1, 6},
								{0, 3, 4, 5, -1, 6},
								{0, 2, 3, 4, 5 ,6}};

#endif
#endif

static uint8    priorityDecisionArray[] = {	2,		/* port base */
									8,		/*         802.1p base */ 
#if defined (CONFIG_RTK_VOIP_QOS) 
									8,		/*         dscp base */                   
#else
									4,		/*         dscp base */                   
#endif
									8,		/*         acl base */    
									8		/* nat base */
									#if defined(CONFIG_RTL_8197F)
									,2		/* VID base*/
									#endif	
								};

#if defined(CONFIG_RTL_SW_QUEUE_DECISION_PRIORITY)
static uint8    swPriorityDecisionArray[] = {	8,		/* port base */
									8,		/*         802.1p base */ 
									8,		/*         dscp base */                   
									2,		/*         acl base */    
									8		/* nat base */
									#if defined(CONFIG_RTL_8197F)
									,8		/* VID base*/
									#endif
								};
#endif

static uint32	defPriority[NETIF_NUMBER] = {0};
static uint32	queueNumber[NETIF_NUMBER] = {0};
static uint32	priority2HandleMapping[NETIF_NUMBER][TOTAL_VLAN_PRIORITY_NUM] = {{0}};
static rtl_qos_mark_info_t	mark2Priority[NETIF_NUMBER][MAX_MARK_NUM_PER_DEV] = {{{0}}};

rtl865x_qos_rule_t		*rtl865x_qosRuleHead = NULL;
extern rtl865x_netif_local_t *_rtl865x_getNetifByIdx(int idx);
extern int hw_qos_init_netlink(void);
static int32 _rtl865x_qosArrangeRuleByNetif(uint8 *netIfName);

#if defined (CONFIG_RTL_HW_QOS_SUPPORT)
// tc_hw_qos > 0: tc commands will be synced for hw qos.
unsigned char tc_hw_qos = 1;


struct proc_dir_entry *rtl_hw_qos_config_entry;
extern struct proc_dir_entry proc_root;
#if defined(CONFIG_RTL_PROC_NEW)
int rtl_hw_qos_config_single_open(struct inode *inode, struct file *file);
static ssize_t rtl_hw_qos_config_single_write(struct file * file, const char __user * userbuf,
		     size_t count, loff_t * off);

struct file_operations rtl_hw_qos_config_proc_fops= {
        .open           = rtl_hw_qos_config_single_open,
        .write			= rtl_hw_qos_config_single_write,
        .read           = seq_read,
        .llseek         = seq_lseek,
        .release        = single_release,
};
#else
static int32 rtl_hw_qos_config_read( char *page, char **start, off_t off, int count, int *eof, void *data );
static int32 rtl_hw_qos_config_write( struct file *filp, const char *buff,unsigned long len, void *data );
#endif
#endif

#if defined(CONFIG_RTL_HW_QOS_BRIDGE_FWD_SUPPORT)
#define TOTAL_DSCP_NUM 64
extern int rtl865x_isWanNetifByIdx(int netifIdx);
uint32 classId2Port[NETIF_NUMBER][RTL8651_PORT_NUMBER];
uint32 classId2VlanPriority[NETIF_NUMBER][TOTAL_VLAN_PRIORITY_NUM];
uint32 classId2Dscp[NETIF_NUMBER][TOTAL_DSCP_NUM];
static uint8 vlanPri2SystemPri[TOTAL_VLAN_PRIORITY_NUM] = {2, 0, 1, 3, 4, 5, 6, 7};
#endif

#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
#define IPV4_VERSION				0x1

#if defined(CONFIG_IPV6)
#define IPV6_VERSION				0x2
#define IPV4_IPV6_VERSION			0x4
#endif
#endif

extern uint32 rtl865x_getVlanPortMask(uint32 vid);
extern struct net_device *dev_get_by_name(struct net *net, const char *name);
extern struct net init_net;

#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
int rtl83xx_qosSetBandwidth(unsigned int memberPort, unsigned int Kbps);
int32 rtl83xx_qosFlushBandwidth(unsigned int memberPort);
int32 rtl83xx_qosSetIngressBandwidth(unsigned int memberPort, unsigned int Kbps);

int rtl83xx_qosProcessQueue(unsigned int memberPort, unsigned int queueNum, unsigned int *queueFlag, unsigned int *queueId, unsigned int *queueBw, unsigned int *queueWeight, unsigned int wanPortMask);
int rtl83xx_closeQos(unsigned int memberPort, unsigned int wanPortMask);
#endif

uint32 rtl865x_qosGetMemberPortbyNetif(uint8 *netIfName)
{
	rtl865x_netif_local_t	*netIf;
	rtl865x_vlan_entry_t	*vlan;
	uint32 memberPort = 0;

	if(netIfName==NULL)
		return 0;

	#if defined(CONFIG_RTL_ISP_MULTI_WAN_QOS_SUPPORT) && defined(CONFIG_IMQ)
	if(strcmp(netIfName, RTL_DRV_IMQ0_NETIF_NAME)==0)
		memberPort = RTL_LANPORT_MASK;
	else if(strcmp(netIfName, RTL_DRV_IMQ1_NETIF_NAME)==0)
		memberPort = RTL_WANPORT_MASK;
	else
	#endif
	{
		netIf = _rtl865x_getNetifByName(netIfName);
		if(netIf == NULL)
		   	return 0;
		
		vlan = _rtl8651_getVlanTableEntry(netIf->vid);
		if(vlan == NULL)
			return 0;
		
		memberPort = vlan->memberPortMask;
	
	}
	memberPort &= (1<<RTL8651_MAC_NUMBER)-1;
	return memberPort;
}

int32 rtl865x_qosSetBandwidth(uint8 *netIfName, uint32 bps)
{
	uint32	memberPort;
#if !defined (CONFIG_RTL_83XX_QOS_SUPPORT)
	uint32	port;
#endif
	uint32	asicBandwidth;
	
	memberPort = rtl865x_qosGetMemberPortbyNetif(netIfName);
	if(memberPort==0)
		return FAILED;

	///////////////////////////////////////////////
	/*	Egress bandwidth granularity was 64Kbps for !97F, 1kbps for 97F, 8kbps for 8367*/
	asicBandwidth = bps>>EGRESS_BANDWIDTH_GRANULARITY_BITLEN;
	if (asicBandwidth>0 && (bps&(1<<(EGRESS_BANDWIDTH_GRANULARITY_BITLEN-1)))!=0)
	{
		asicBandwidth++;
	}
#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
	/*for 8367 qos api, Egress bandwidth parameter granularity was 1kbps*/
	asicBandwidth = asicBandwidth<<3;
	rtl83xx_qosSetBandwidth(memberPort, asicBandwidth);
#else

#if defined(CONFIG_RTL_PUBLIC_SSID)
	if(strcmp(netIfName,RTL_GW_WAN_DEVICE_NAME) == 0)
#else
	if(strcmp(netIfName,RTL_DRV_WAN0_NETIF_NAME)==0)
#endif
	{
		//Adjust for wan port egress asic bandwidth
		asicBandwidth+=3;
	}
	if(strcmp(netIfName,RTL_DRV_LAN_NETIF_NAME)==0)
	{
		//Adjust for lan port egress asic bandwidth
		asicBandwidth++;
	}
	
	rtl865xC_lockSWCore();
	for(port=0;port<=CPU;port++)
	{
		if(((1<<port)&memberPort)==0)
			continue;
		rtl8651_setAsicPortEgressBandwidth(port, asicBandwidth);
	}
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();
#endif
	return SUCCESS;
}

int32 rtl865x_qosFlushBandwidth(uint8 *netIfName)
{
	uint32	memberPort;
#if !defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	uint32	port;
#endif

	memberPort = rtl865x_qosGetMemberPortbyNetif(netIfName);
	if(memberPort==0)
		return FAILED;

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	rtl83xx_qosFlushBandwidth(memberPort);
	//need to check
	rtl865x_raiseEvent(EVENT_FLUSH_QOSRULE, NULL);
#else
	rtl865xC_lockSWCore();
	for(port=0;port<=CPU;port++)
	{
		if(((1<<port)&memberPort)==0)
			continue;

		rtl8651_setAsicPortEgressBandwidth(port, APR_MASK>>APR_OFFSET);
	}

	rtl865x_raiseEvent(EVENT_FLUSH_QOSRULE, NULL);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();
#endif
	return SUCCESS;
}
#if 0
int32 rtl865x_qosGetPriorityByHandle(uint8 *priority, uint32 handle)
{
	int	j;

	for(j=0; j < TOTAL_VLAN_PRIORITY_NUM; j++)
	{
		if (priority2HandleMapping[j] == handle)
		{
			*priority = j;
			break;
		}
	}
	if (j<TOTAL_VLAN_PRIORITY_NUM)
		return SUCCESS;
	else
		return FAILED;
}
#else
static int32 _rtl865x_qosGetPriorityByHandle(int32 idx, uint32 handle)
{
	int	j;

	for(j=0; j < TOTAL_VLAN_PRIORITY_NUM; j++)
	{
		if (priority2HandleMapping[idx][j] == handle)
		{
			return j;
		}
	}

	return 0;
}
#endif

int32 rtl_qosGetPriorityByMark(uint8 *netIfName, int32 mark)
{
	int	netifIdx, i;

	//Especil for ppp0 which attach at eth1
	netifIdx = _rtl865x_getNetifIdxByNameExt(netIfName);
	if((netifIdx < 0) || (netifIdx >= NETIF_NUMBER))
		return FAILED;

	if (tc_hw_qos && queueNumber[netifIdx] <=1)
		return defPriority[netifIdx];
	
	for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
	{
		#if defined (CONFIG_OPENWRT_SDK)	
		if ((mark2Priority[netifIdx][i].mark&mark2Priority[netifIdx][i].mask)==(mark&mark2Priority[netifIdx][i].mask))
		#else
		if (mark2Priority[netifIdx][i].mark==mark)
		#endif	
			return mark2Priority[netifIdx][i].priority;
	}

	return defPriority[netifIdx];
}

int32 rtl_qosGetPriorityByVid(int32 vid, int32 mark)
{
	int	netifIdx, i;

	netifIdx = _rtl865x_getNetifIdxByVid(vid);
	if((netifIdx < 0) || (netifIdx >= NETIF_NUMBER))
		return FAILED;
	
	if (tc_hw_qos && queueNumber[netifIdx] <=1)
		return defPriority[netifIdx];
	for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
	{
		#if defined (CONFIG_OPENWRT_SDK)	
		if ((mark2Priority[netifIdx][i].mark&mark2Priority[netifIdx][i].mask)==(mark&mark2Priority[netifIdx][i].mask))
		#else
		if (mark2Priority[netifIdx][i].mark==mark)
		#endif	
			return mark2Priority[netifIdx][i].priority;
	}

	return defPriority[netifIdx];
}
#if defined (CONFIG_OPENWRT_SDK)	
int32 rtl_qosSetPriorityByMark(uint8 *netIfName, int32 mark, int32 handler, int32 mask,int32 enable)
#else
int32 rtl_qosSetPriorityByMark(uint8 *netIfName, int32 mark, int32 handler, int32 enable)
#endif
{
	int	netifIdx, i;

	//Especil for ppp0 which attach at eth1
	netifIdx = _rtl865x_getNetifIdxByNameExt(netIfName);
	if((netifIdx < 0) || (netifIdx >= NETIF_NUMBER))
		return FAILED;
		
#if defined (CONFIG_OPENWRT_SDK)
	if(enable ==RTL_MARK2PRIO_FLUSH_FLAG)
	{
		
		for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
		{
		#ifdef DEFAULT_PRIORITY_PATCH
			mark2Priority[netifIdx][i].priority = -1;
			mark2Priority[netifIdx][i].mark = -1;
			mark2Priority[netifIdx][i].mask = -1;
		#else
			mark2Priority[netifIdx][i].priority=0;
		#endif
			
		}
		return SUCCESS;
	}
#endif

	for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
	{
		
		
		if ((mark2Priority[netifIdx][i].mark==mark)
		#if defined (CONFIG_OPENWRT_SDK)	
			&&(mark2Priority[netifIdx][i].mask==mask)
		#endif
			)
		{
			if (enable==TRUE)
			{
				mark2Priority[netifIdx][i].priority = _rtl865x_qosGetPriorityByHandle(netifIdx, handler);
			}
			else
			{
			#ifdef DEFAULT_PRIORITY_PATCH
				mark2Priority[netifIdx][i].mark = -1;
				mark2Priority[netifIdx][i].priority = -1;
				#if defined (CONFIG_OPENWRT_SDK)	
				mark2Priority[netifIdx][i].mask = -1;
				#endif
			#else
				mark2Priority[netifIdx][i].mark = 0;
				mark2Priority[netifIdx][i].priority = 0;
				#if defined (CONFIG_OPENWRT_SDK)	
				mark2Priority[netifIdx][i].mask = 0;
				#endif
			#endif
			}
			break;
		}
	}

	if (i==MAX_MARK_NUM_PER_DEV&&enable==TRUE)
	{
		for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
		{
#ifdef DEFAULT_PRIORITY_PATCH
			if (mark2Priority[netifIdx][i].mark==-1)
#else
			if (mark2Priority[netifIdx][i].mark==0)
#endif
			{
				mark2Priority[netifIdx][i].mark = mark;
				#if defined (CONFIG_OPENWRT_SDK)	
				mark2Priority[netifIdx][i].mask = mask;
				#endif
				mark2Priority[netifIdx][i].priority = _rtl865x_qosGetPriorityByHandle(netifIdx, handler);
				break;
			}
		}

		if (i==MAX_MARK_NUM_PER_DEV)
		{
			return FAILED;
		}
	}
	return SUCCESS;
}

int rtk_cmd_config_mark2priority(uint8 *netIfName, uint8 count, rtk_cmd_mark_p p_rtk_mark)
{
	int	netifIdx;
	
	if(p_rtk_mark == NULL || netIfName == NULL || count > MAX_MARK_NUM_PER_DEV)
		return FAILED;

	netifIdx = _rtl865x_getNetifIdxByNameExt(netIfName);
	if((netifIdx < 0) || (netifIdx >= NETIF_NUMBER))
		return FAILED;

	if(p_rtk_mark->delflag){
		mark2Priority[netifIdx][count].mark = 0;
		mark2Priority[netifIdx][count].priority = 0;
	}
	else{
		mark2Priority[netifIdx][count].mark = p_rtk_mark->mark;
		mark2Priority[netifIdx][count].priority = p_rtk_mark->priority;
	}
	
	return SUCCESS;
}

#if defined(CONFIG_RTL_HW_QOS_SUPPORT) && defined(CONFIG_RTL_SW_QUEUE_DECISION_PRIORITY)
/*Make sure priorityDecisionArray parameter seq is the same with enum decision_priority*/
int rtl_getMaxDecisionBitMap(unsigned int* maxDecision)
{
	int i;
	int max = 0;

	#if defined(CONFIG_RTL_8197F)
	for (i=0; i<6; i++) 
	#else
	for (i=0; i<5; i++) 
	#endif
	{
		if (i==3)	//skip acl decision
			continue;

		if (swPriorityDecisionArray[i] > max) {
			max = swPriorityDecisionArray[i];
			*maxDecision = 0;
			*maxDecision |= 1<<i;
		} else if (swPriorityDecisionArray[i] == max) {
			*maxDecision |= 1<<i;
		}
	}

	return SUCCESS;
}
#endif

#if 1
/*
	_rtl865x_compare2Entry Return Value:
		>0		means entry1 > entry2
		<0		means entry1 < entry2
		=0		means entry1 = entry2
*/
static int8 _rtl865x_compare2Entry(rtl865x_qos_t	*entry1, rtl865x_qos_t	*entry2)
{
#if 0
	if (entry1->handle>entry2->handle)
		return 1;
	else if (entry1->handle<entry2->handle)
		return -1;
	else
		return 0;
#else
	if (entry1->bandwidth<entry2->bandwidth)
		return 1;
	else if (entry1->bandwidth>entry2->bandwidth)
		return -1;
	else
		return 0;
#endif
}
#if defined (CONFIG_RTL_HW_QOS_SP_PRIO)	

static int8 rtl865x_compPri(rtl865x_qos_t	*entry1, rtl865x_qos_t	*entry2)
{
	if (entry1->prio<entry2->prio)
		return 1;
	else if (entry1->prio>entry2->prio)
		return -1;
	else
		return 0;
}
#endif
int32	rtl865x_registerQosCompFunc(int8 (*p_cmpFunc)(rtl865x_qos_t	*entry1, rtl865x_qos_t	*entry2))
{
	if (p_cmpFunc==NULL)
		rtl865x_compFunc = _rtl865x_compare2Entry;
	else
		rtl865x_compFunc = p_cmpFunc;

	return SUCCESS;
}

static int my_gcd(int numA, int numB)
{
	int	tmp;
	int	divisor;

	if (numA<numB)
	{
		tmp = numA;
		numA = numB;
		numB = tmp;
	}

	divisor = numA%numB;
	while(divisor)
	{
		numA = numB;
		numB = divisor;
		divisor = numA%numB;
	}

	return numB;
}

static struct net_device *rtl865x_getDevByName(char *devName)
{
	struct net_device * dev=NULL;

	if(devName != NULL)
	{
		dev=dev_get_by_name(&init_net, devName);
	}
	
	return dev;
}

static int32 _rtl865x_qosArrangeQueue(rtl865x_qos_t *qosInfo)
{
	uint32	queueOrder[RTL8651_OUTPUTQUEUE_SIZE] = {0};
	uint32	entry;
	int32	nStart, nEnd;
	int32	mStart, mEnd;
	int32	i=0, j=0, cnt=0;
	int32	queueNum;
#if !defined (CONFIG_OPENWRT_SDK)
	int32	qosMarkNumIdx;
	uint32	tmpHandle;
	struct net_device * tmpDev;
#endif
	struct net_device *qosDev;
	rtl865x_qos_t	*outputQueue;
	rtl865x_qos_t	tmp_qosInfo[RTL8651_OUTPUTQUEUE_SIZE];
	const int32	idx = _rtl865x_getNetifIdxByNameExt(qosInfo->ifname);
#if defined (CONFIG_RTL_HW_QOS_SP_PRIO) 	
	int ret;
#endif	

	/*	Process the queue type & ratio	*/
	{
		int			divisor;
#if defined (CONFIG_OPENWRT_SDK)
		divisor = qosInfo[0].bandwidth;
#else
		if ((qosInfo[0].flags&QOS_DEF_QUEUE)!=0)
			divisor = qosInfo[0].ceil;
		else
			divisor = qosInfo[0].bandwidth;
#endif
		for(queueNum=0; queueNum<RTL8651_OUTPUTQUEUE_SIZE; queueNum++)
		{
			if ((qosInfo[queueNum].flags&QOS_VALID_MASK)==0)
				break;
			/*	Currently, we set all queue as WFQ		*/
			if ((qosInfo[queueNum].flags&QOS_TYPE_MASK)==QOS_TYPE_WFQ)
			{
			#if !defined (CONFIG_OPENWRT_SDK)
				if ((qosInfo[queueNum].flags&QOS_DEF_QUEUE)!=0)
				{
					qosInfo[queueNum].bandwidth=qosInfo[queueNum].ceil;
				}
			#endif	
				divisor = my_gcd(qosInfo[queueNum].bandwidth, divisor);
			}
		}

		/*	process WFQ type ratio	*/
		{
			if (divisor)
			{
				int	maxBandwidth;
				int	queueNumBackup;
				maxBandwidth = 0;
				queueNumBackup = queueNum;

				while(queueNum>0)
				{
					if (!(qosInfo[queueNum-1].flags&QOS_DEF_QUEUE) && ((qosInfo[queueNum-1].flags&QOS_TYPE_MASK)==QOS_TYPE_WFQ))
					{
						qosInfo[queueNum-1].bandwidth = (qosInfo[queueNum-1].bandwidth/divisor);
						if (maxBandwidth<qosInfo[queueNum-1].bandwidth)
							maxBandwidth = qosInfo[queueNum-1].bandwidth;
					}
					queueNum--;
				}

				if (maxBandwidth>EGRESS_WFQ_MAX_RATIO)
				{
					queueNum = queueNumBackup;
					divisor = (maxBandwidth/EGRESS_WFQ_MAX_RATIO)
						+ ((maxBandwidth%EGRESS_WFQ_MAX_RATIO)>(EGRESS_WFQ_MAX_RATIO>>1)?1:0);
					while(queueNum>0)
					{
						if ((qosInfo[queueNum-1].flags&QOS_TYPE_MASK) 
							==QOS_TYPE_WFQ)
						{
							qosInfo[queueNum-1].bandwidth = 
								(qosInfo[queueNum-1].bandwidth/divisor);
							
							if (qosInfo[queueNum-1].bandwidth==0)
								qosInfo[queueNum-1].bandwidth = 1;
							else if (qosInfo[queueNum-1].bandwidth>EGRESS_WFQ_MAX_RATIO)
								qosInfo[queueNum-1].bandwidth = EGRESS_WFQ_MAX_RATIO;
						}
						queueNum--;
					}
				}
			}
		}

		divisor = 0;
		for(queueNum=0; queueNum<RTL8651_OUTPUTQUEUE_SIZE; queueNum++)
		{
			
			#if !defined (CONFIG_OPENWRT_SDK)
			if ((qosInfo[queueNum].flags&QOS_DEF_QUEUE)!=0)
			{
				qosInfo[queueNum].bandwidth = 1;
			}
			else
			#endif	
			if ((qosInfo[queueNum].flags&QOS_VALID_MASK)!=0)
			{
				continue;
			}
			break;
		}
	}
	
	nStart = nEnd = mStart = mEnd = 0;	/*	reserver 0 for default queue	*/
	queueOrder[0] = 0;
	outputQueue = qosInfo;
	
	for(entry=0; entry<RTL8651_OUTPUTQUEUE_SIZE; entry++, outputQueue++)
	{
		if ((outputQueue->flags&QOS_VALID_MASK)==0)
			break;
		
		/*	rtlglue_printf("index %d, queueType %d.\n", entry, outputQueue->queueType); */
		if ((outputQueue->flags&QOS_TYPE_MASK)==QOS_TYPE_WFQ) 
		{
			/*	Do not exceed the max value: 1 ~ 128	*/
			if (outputQueue->bandwidth>((WEIGHT0_MASK>>WEIGHT0_OFFSET)+1))
				outputQueue->bandwidth = (WEIGHT0_MASK>>WEIGHT0_OFFSET)+1;

			/*	this is a NQueue entry	*/
			{
				/*	process m Queue */
				if (mEnd>mStart)
				{
					i = mEnd-1;
					while(i>=mStart)
					{
						queueOrder[i+1] = queueOrder[i];
						i--;
					}
				}
				mEnd++;
				mStart++;	

				/*	process n Queue */
				i = nEnd;
				{
					while(i>nStart)
					{
						if(rtl865x_compFunc(outputQueue, &qosInfo[queueOrder[i-1]])>0)
						{
							queueOrder[i] = queueOrder[i-1];
							i--;
							continue;
						}
						break;
					}
				}
				nEnd++;
				queueOrder[i] = entry;
			}			
		} else if ((outputQueue->flags&QOS_TYPE_MASK)==QOS_TYPE_STR)
		{
			i = mEnd;
			{
				while(i>mStart)
				{
					
					#if defined (CONFIG_RTL_HW_QOS_SP_PRIO) 
					
					ret =rtl865x_compPri(outputQueue, &qosInfo[queueOrder[i-1]]);
					if (ret>0)
					{
						queueOrder[i] = queueOrder[i-1];
						i--;
						continue;
					}
					else if(ret==0)
					#endif
					{
						if (rtl865x_compFunc(outputQueue, &qosInfo[queueOrder[i-1]])>0)
						{
							queueOrder[i] = queueOrder[i-1];
							i--;
							continue;
						}
					}
					break;
				}
			}
			mEnd++;
			queueOrder[i] = entry;
		}
	}

	queueNumber[idx] = mEnd;
	
	queueNum = 1;
	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (queueNum<queueNumber[i])
			queueNum = queueNumber[i];
	}

	memset((void*)tmp_qosInfo, 0, RTL8651_OUTPUTQUEUE_SIZE*sizeof(rtl865x_qos_t));

	/*	Record the priority <-> handle mapping relationship	*/
	for(i=0;i<mEnd;i++)
	{
		cnt = -1;
		for(j=0;j<RTL8651_OUTPUTQUEUE_SIZE;j++)
		{
			if (queueMatrix[queueNum-1][j]>=0)
				cnt++;
			else
				continue;

			if (cnt==i)
			{
				memcpy(&tmp_qosInfo[j], &qosInfo[queueOrder[i]], sizeof(rtl865x_qos_t));
				priority2HandleMapping[idx][queueMatrix[queueNum-1][j]] = tmp_qosInfo[j].handle;
				tmp_qosInfo[j].queueId = j;
			}
		}
	}

	memcpy(qosInfo, tmp_qosInfo, RTL8651_OUTPUTQUEUE_SIZE*sizeof(rtl865x_qos_t));

	/*	Set default priority	*/
	for(i=0;i<RTL8651_OUTPUTQUEUE_SIZE;i++)
	{
		if (!(qosInfo[i].flags&QOS_DEF_QUEUE))
			continue;
		
		for(j=0; j < TOTAL_VLAN_PRIORITY_NUM; j++)
		{
			if (priority2HandleMapping[idx][j] == qosInfo[i].handle)
				break;
		}

		/*	If we do not find the default queue priority
		  *	just keep the default priority 0
		  */
		if (j==TOTAL_VLAN_PRIORITY_NUM)
			j = 0;

		/*	Set default queue priority	*/
		defPriority[idx] = j;
	}

	//To update mark2Priority
	qosDev=rtl865x_getDevByName(qosInfo->ifname);
	
	if(qosDev)
	{
		#if !defined (CONFIG_OPENWRT_SDK)
		for(qosMarkNumIdx=0;qosMarkNumIdx<MAX_MARK_NUM_PER_DEV;qosMarkNumIdx++)
		{
		#ifdef DEFAULT_PRIORITY_PATCH
			if(mark2Priority[idx][qosMarkNumIdx].mark != -1)
			{
				if(tc_getHandleByKey(mark2Priority[idx][qosMarkNumIdx].mark, &tmpHandle, qosDev, &tmpDev) == 0)
				{
					mark2Priority[idx][qosMarkNumIdx].priority=_rtl865x_qosGetPriorityByHandle(idx, tmpHandle);
				}
				else
				{
					mark2Priority[idx][qosMarkNumIdx].mark=-1;
				}
			}
		#else
			if(mark2Priority[idx][qosMarkNumIdx].mark != 0)
			{
				if(tc_getHandleByKey(mark2Priority[idx][qosMarkNumIdx].mark, &tmpHandle, qosDev, &tmpDev) == 0)
				{
					mark2Priority[idx][qosMarkNumIdx].priority=_rtl865x_qosGetPriorityByHandle(idx, tmpHandle);
				}
				else
				{
					mark2Priority[idx][qosMarkNumIdx].mark=0;
				}
			}
		#endif
		}
		#endif
		dev_put(qosDev);
	}
	
	return SUCCESS;
}
#endif

int32 rtl865x_qosProcessQueue(uint8 *netIfName, rtl865x_qos_t *qosInfo)
{
	uint32	memberPort = 0;
	uint32	i, queue;
	uint32	queueNum;
#if !defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	uint32	port;
	int32	asicBandwidth;
	int32	weight=0;
#else
	uint32 queueFlag[RTL8651_OUTPUTQUEUE_SIZE];
	uint32 queueId[RTL8651_OUTPUTQUEUE_SIZE];
	uint32 queueBw[RTL8651_OUTPUTQUEUE_SIZE];
	uint32 queueWeight[RTL8651_OUTPUTQUEUE_SIZE];
#endif
	if (qosInfo==NULL)
		return FAILED;

	memberPort = rtl865x_qosGetMemberPortbyNetif(netIfName);
	if(memberPort==0)
		return FAILED;

	rtl865x_closeQos(netIfName);
	
	_rtl865x_qosArrangeQueue(qosInfo);
	_rtl865x_qosArrangeRuleByNetif(netIfName);

#if 0
	for(i=0; i<8; i++){
		printk("[%s:%d] i:%d, rate:%d, ceil:%d, flag: %d\n", __FUNCTION__, __LINE__, 
			i, qosInfo[i].bandwidth, qosInfo[i].ceil, qosInfo[i].flags);
	}
#endif

	/*	Since we use napt base priority, 
	*	the queue number of each network interface should be the same.
	*	So we had to select the max queue number to set to each netif.
	*/
	queueNum = 1;
	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (queueNum<queueNumber[i])
			queueNum = queueNumber[i];
	}
#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
	for (queue=0;queue<RTL8651_OUTPUTQUEUE_SIZE;queue++)
	{
		queueFlag[queue] = qosInfo[queue].flags;
		queueId[queue] = qosInfo[queue].queueId;
		queueWeight[queue] = qosInfo[queue].bandwidth;
		/*	Egress bandwidth granularity was 8Kbps, 	*/
		if(qosInfo[queue].ceil != 0)
		{
			queueBw[queue] = ((qosInfo[queue].ceil)>>(EGRESS_BANDWIDTH_GRANULARITY_BITLEN)) - 1;
			if ((qosInfo[queue].ceil)&(1<<(EGRESS_BANDWIDTH_GRANULARITY_BITLEN-1)))
					queueBw[queue] += 1;
			queueBw[queue] = queueBw[queue]<<3;
		}
		else
		{
			queueBw[queue] = 0;
		}
	}
	rtl83xx_qosProcessQueue(memberPort, queueNum, queueFlag, queueId, queueBw, queueWeight, RTL_WANPORT_MASK);
#else
	rtl865xC_lockSWCore();

	for(port=0;port<=CPU;port++)
	{
		if(((1<<port)&memberPort)==0)
			continue;

		rtl8651_setAsicOutputQueueNumber(port, queueNum);

		for (queue=0;queue<RTL8651_OUTPUTQUEUE_SIZE;queue++)
		{
			if ((qosInfo[queue].flags&QOS_VALID_MASK)==0 || qosInfo[queue].ceil==0)	/*	un-used queue	*/
				continue;

			/*	Egress bandwidth granularity was 64Kbps	*/
			asicBandwidth = ((qosInfo[queue].ceil)>>(EGRESS_BANDWIDTH_GRANULARITY_BITLEN)) - 1;
			if ((qosInfo[queue].ceil)&(1<<(EGRESS_BANDWIDTH_GRANULARITY_BITLEN-1)))
				asicBandwidth += 1;

			if ((qosInfo[queue].flags&QOS_TYPE_MASK)==QOS_TYPE_STR)
			{
				rtl8651_setAsicQueueRate(port, qosInfo[queue].queueId, 
					PPR_MASK>>PPR_OFFSET, 
					L1_MASK>>L1_OFFSET, 
					asicBandwidth);
				rtl8651_setAsicQueueWeight(port, qosInfo[queue].queueId, STR_PRIO, 0);
			}
			else
			{
				rtl8651_setAsicQueueRate(port, qosInfo[queue].queueId, 
					1, 
					L1_MASK>>L1_OFFSET, 
					asicBandwidth);
				weight =qosInfo[queue].bandwidth-1;
				#if defined(CONFIG_OPENWRT_SDK)
				if(weight<0)
					rtl8651_setAsicQueueWeight(port, qosInfo[queue].queueId, WFQ_PRIO, 0);
				else
				#endif	
				rtl8651_setAsicQueueWeight(port, qosInfo[queue].queueId, WFQ_PRIO, qosInfo[queue].bandwidth-1);
			}
		}
	}

	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();
#endif
	return SUCCESS;
}

int32 rtl865x_qosArrangeRuleByNetif(void)
{
	int		i;

	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			rtl865x_flush_allAcl_fromChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
			rtl865x_flush_allAcl_fromChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
		}
	}

	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			_rtl865x_qosArrangeRuleByNetif(netIfNameArray[i]);
		}
	}

	return SUCCESS;
}

static int32 _rtl865x_qosArrangeRuleByNetif(uint8 *netIfName)
{
	rtl865x_qos_rule_t	*qosRule;
	int32			priority;
	int32			i;
	const int32	idx = _rtl865x_getNetifIdxByNameExt(netIfName);
	int32			tmp_idx;
	
	if((idx < 0) || (idx >= NETIF_NUMBER))
		return FAILED;

	for(qosRule = rtl865x_qosRuleHead; qosRule; qosRule=qosRule->next)
	{
		if (qosRule->handle==0)
			continue;
		if ((priority=_rtl865x_qosGetPriorityByHandle(idx, qosRule->handle))==0)
			continue;

		if(qosRule->outIfname[0]!='\0')
		{
			/*	assigned egress netif	*/
			tmp_idx = _rtl865x_getNetifIdxByNameExt(qosRule->outIfname);
			if (tmp_idx!=idx)
				continue;
		}

#if	defined(CONFIG_RTL_QOS_8021P_SUPPORT)
		if (qosRule->rule->ruleType_==RTL865X_ACL_802D1P)
		{
			rtl8651_setAsicDot1qAbsolutelyPriority(qosRule->rule->vlanTagPri_, priority);
			continue;
		}
#endif
		{
			qosRule->rule->priority_ = priority;
			if (qosRule->inIfname[0]!='\0')
			{
				/*	assigned ingress netif	*/
				rtl865x_del_acl(qosRule->rule, qosRule->inIfname, RTL865X_ACL_QOS_USED0);
				rtl865x_add_acl(qosRule->rule, qosRule->inIfname, RTL865X_ACL_QOS_USED0);
				#if defined (CONFIG_RTL_LOCAL_PUBLIC)
				#if defined(CONFIG_RTL_PUBLIC_SSID)
				if (memcmp(RTL_GW_WAN_DEVICE_NAME, qosRule->inIfname, 4)==0
					&&qosRule->rule->direction_==RTL_ACL_INGRESS)
				#else
				if (memcmp(RTL_DRV_WAN0_NETIF_NAME, qosRule->inIfname, 4)==0
					&&qosRule->rule->direction_==RTL_ACL_INGRESS)
				#endif
				{
					rtl_checkLocalPublicNetifIngressRule(qosRule->rule);
				}
				#endif
			}
			else
			{
				/*	do not assigned ingress netif	*/
				for(i=0;i<NETIF_NUMBER;i++)
				{
					if (netIfNameArray[i][0]!=0)
					{
						tmp_idx = _rtl865x_getNetifIdxByNameExt(netIfNameArray[i]);
						if (tmp_idx>=0&&tmp_idx<NETIF_NUMBER&&tmp_idx!=idx)
						{
							rtl865x_del_acl(qosRule->rule, netIfNameArray[i], RTL865X_ACL_QOS_USED0);
							rtl865x_add_acl(qosRule->rule, netIfNameArray[i], RTL865X_ACL_QOS_USED0);
						}
					}
				}
				#if defined (CONFIG_RTL_LOCAL_PUBLIC)
				if (qosRule->rule->direction_==RTL_ACL_INGRESS)
				{
					rtl_checkLocalPublicNetifIngressRule(qosRule->rule);
				}
				#endif
			}
		}
	}

	/*	Add default priority	*/
#ifndef DEFAULT_PRIORITY_PATCH
	{
		rtl865x_AclRule_t	aclRule;
		int	i;
		
		memset(&aclRule, 0, sizeof(rtl865x_AclRule_t));
		aclRule.actionType_ = RTL865X_ACL_PRIORITY;
		aclRule.pktOpApp_ = RTL865X_ACL_ALL_LAYER;
		aclRule.priority_ = defPriority[idx];
		i = 0;
		for(i=0;i<NETIF_NUMBER;i++)
		{
			if (netIfNameArray[i][0]!=0)
			{
				tmp_idx = _rtl865x_getNetifIdxByNameExt(netIfNameArray[i]);
				if (tmp_idx>=0&&tmp_idx<NETIF_NUMBER&&tmp_idx!=idx)
				{
					rtl865x_add_acl(&aclRule, netIfNameArray[i], RTL865X_ACL_QOS_USED1);
				}
			}
		}
	}
#endif

	rtl865x_raiseEvent(EVENT_CHANGE_QOSRULE, NULL);
	
	return SUCCESS;
}

int32 rtl865x_qosAddMarkRule(rtl865x_qos_rule_t *rule)
{
	rtl865x_AclRule_t *qosAclRule;
	int				i;

	qosAclRule = kmalloc(sizeof(rtl865x_AclRule_t), GFP_ATOMIC);
	if (qosAclRule==NULL)
	{
		return RTL_ENOFREEBUFFER;
	}
	memcpy(qosAclRule, rule->rule, sizeof(rtl865x_AclRule_t));
	qosAclRule->pktOpApp_ = RTL865X_ACL_ALL_LAYER;
	qosAclRule->actionType_ = RTL865X_ACL_PRIORITY;
	qosAclRule->aclIdx = 0;

	if (rtl865x_qosRuleHead==NULL)
	{
		rtl865x_qosRuleHead = rtl_malloc(sizeof(rtl865x_qos_rule_t));
		if (rtl865x_qosRuleHead==NULL)
		{
			rtl_free(qosAclRule);
			return RTL_ENOFREEBUFFER;
		}
		memcpy(rtl865x_qosRuleHead, rule, sizeof(rtl865x_qos_rule_t));
		rtl865x_qosRuleHead->rule = qosAclRule;
		rtl865x_qosRuleHead->next = NULL;
	}
	else
	{
		rtl865x_qos_rule_t	*qosRule, *lastQosRule;

		qosRule = rtl_malloc(sizeof(rtl865x_qos_rule_t));
		if (qosRule==NULL)
		{
			rtl_free(qosAclRule);
			return RTL_ENOFREEBUFFER;
		}
		lastQosRule = rtl865x_qosRuleHead;
		while(lastQosRule->next)
			lastQosRule = lastQosRule->next;

		lastQosRule->next = qosRule;
		memcpy(qosRule, rule, sizeof(rtl865x_qos_rule_t));
		qosRule->rule = qosAclRule;
		qosRule->next = NULL;
	}

	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			_rtl865x_qosArrangeRuleByNetif(netIfNameArray[i]);
		}
	}
	
	return SUCCESS;
}

int32 rtl865x_qosCheckNaptPriority(rtl865x_AclRule_t *qosRule)
{
	rtl865x_AclRule_t		*rule_p;

	rule_p = rtl865x_matched_layer4_aclChain(netIfNameArray[qosRule->netifIdx_], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS, qosRule);

	if (!rule_p)
		rule_p = rtl865x_matched_layer2_aclChain(netIfNameArray[qosRule->netifIdx_], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS, qosRule);

	if (rule_p)
	{
		qosRule->priority_ = rule_p->priority_;
		qosRule->aclIdx = rule_p->aclIdx;
		qosRule->upDown_=rule_p->upDown_;
		return SUCCESS;
	}
	else
	{
		qosRule->priority_ = defPriority[qosRule->netifIdx_];
		return FAILED;
	}
}

int32 rtl865x_qosFlushMarkRule(void)
{
	rtl865x_qos_rule_t	*qosRule;
	int	i;

	while(rtl865x_qosRuleHead)
	{
		qosRule = rtl865x_qosRuleHead->next;
		rtl_free(rtl865x_qosRuleHead->rule);
		rtl_free(rtl865x_qosRuleHead);
		rtl865x_qosRuleHead = qosRule;
	}

	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			rtl865x_flush_allAcl_fromChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
			rtl865x_flush_allAcl_fromChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
			_rtl865x_qosArrangeRuleByNetif(netIfNameArray[i]);
		}
	}

	rtl865x_raiseEvent(EVENT_FLUSH_QOSRULE, NULL);
	
	return SUCCESS;
}

#if defined (CONFIG_RTL_HW_QOS_SUPPORT)	// sync from voip customer for multiple ppp
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
/*
	8367r dscp remark is enabled for whole system, not per port control.
	when configuring dscp remark rule on 8367 wan port, 
	8367 cpu port dscp remark function will be enabled.
	If hw nat qos on 97F would like to match dscp, it will fail.
	To deal with above case, we will make l2 traffic forward by 97f rather than 8367, 
	and make dscp remark configure on 97F.
*/
int rtl865x_syncRemarkInfoToAsic(uint8 *netIfName)
{

	uint32	memberPort, port;
	int qosMarkNumIdx;
	int netifIdx;
	
	memberPort = rtl865x_qosGetMemberPortbyNetif(netIfName);
	if(memberPort==0)
		return FAILED;

	netifIdx = _rtl865x_getNetifIdxByNameExt(netIfName);
	if((netifIdx < 0) || (netifIdx >= NETIF_NUMBER))
		return FAILED;
	
	for(port=0;port<=CPU;port++)
	{
		if(((1<<port)&memberPort)==0)
			continue;
		
		for(qosMarkNumIdx=0;qosMarkNumIdx<MAX_MARK_NUM_PER_DEV;qosMarkNumIdx++)
		{
			if(mark2Priority[netifIdx][qosMarkNumIdx].mark )
			{
				if(mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark != 0xff)
					rtl83xx_1p_remark_set(port, mark2Priority[netifIdx][qosMarkNumIdx].priority, mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark);

				if(bridge_qos_enable == 1){
					if(mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark!=0xff){
						rtl865x_restoreDscpRemark(netIfName, 0);
						rtl8651_disableDscpRemark();
							
						rtl83xx_dscp_remark_set(port, mark2Priority[netifIdx][qosMarkNumIdx].priority, mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark);
					}
				}
				else if(bridge_qos_enable == 0){
					if( mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark!=0xff){
						rtl83xx_flush_dscp_remark_setting();
						rtl83xx_disable_dscp_remark_setting();
						
						rtl8651_setAsicDscpRemark(PHY0, mark2Priority[netifIdx][qosMarkNumIdx].priority,  mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark);
					}
				}
			}
		}
	}
	return SUCCESS;
}
#else
int rtl865x_syncRemarkInfoToAsic(uint8 *netIfName)
{

	uint32	memberPort, port;
	int qosMarkNumIdx;
	int netifIdx;
	
	memberPort = rtl865x_qosGetMemberPortbyNetif(netIfName);
	if(memberPort==0)
		return FAILED;

	netifIdx = _rtl865x_getNetifIdxByNameExt(netIfName);
	if((netifIdx < 0) || (netifIdx >= NETIF_NUMBER))
		return FAILED;

	for(port=0;port<=CPU;port++)
	{
		if(((1<<port)&memberPort)==0)
			continue;
		
		for(qosMarkNumIdx=0;qosMarkNumIdx<MAX_MARK_NUM_PER_DEV;qosMarkNumIdx++)
		{
			if(mark2Priority[netifIdx][qosMarkNumIdx].mark )
			{
				uint8 version = mark2Priority[netifIdx][qosMarkNumIdx].version;
				if(version == IPV4_VERSION){
					if( mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark!=0xff)
						rtl8651_setAsicDscpRemark(port, mark2Priority[netifIdx][qosMarkNumIdx].priority,  mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark);
				}
				#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
				else if((version == IPV4_IPV6_VERSION) || ((version & IPV4_VERSION) && (version & IPV6_VERSION))){	// ipv4+ipv6 dscp remark

					if(mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark!=0xff)
						rtl8651_setAsicDscpRemark(port, mark2Priority[netifIdx][qosMarkNumIdx].priority,  mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark);

					if(mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark!=0xff)
						rtl8651_setAsicV6DscpRemark(port, mark2Priority[netifIdx][qosMarkNumIdx].priority,  mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark);
				}
				else if(version == IPV6_VERSION){
					if(mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark!=0xff)
						rtl8651_setAsicV6DscpRemark(port, mark2Priority[netifIdx][qosMarkNumIdx].priority,  mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark);
				}
				#endif
				
				if( mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark!=0xff)
					rtl8651_setAsicVlanRemark(port, mark2Priority[netifIdx][qosMarkNumIdx].priority,  mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark);
			}
		}
	}
	return SUCCESS;
}
#endif

int rtl865x_setRemarkByMark(uint8 *netIfName, uint32 version, unsigned int markValue,int remarkDscp,int remark8021p)
{
	rtl865x_netif_local_t	*netIf;
	int qosMarkNumIdx;
	int netifIdx;

	if((remarkDscp==-1)&&(remark8021p==-1))
		return FAILED;
	
	netIf = _rtl865x_getNetifByName(netIfName);
	if(netIf == NULL)
		return FAILED;
	netifIdx=netIf->asicIdx ;
	if(netifIdx>=NETIF_NUMBER)
		return FAILED;

	//panic_printk("netif:%s,%d memberPort:%x,mark:%d, pri:%d,remarkDscp:%d,remark8021p:%d,[%s]:[%d].\n",
	//netIfName,netifIdx,memberPort,markValue,priority,remarkDscp,remark8021p,__FUNCTION__,__LINE__);	
	for(qosMarkNumIdx=0;qosMarkNumIdx<MAX_MARK_NUM_PER_DEV;qosMarkNumIdx++)
	{
		if(mark2Priority[netifIdx][qosMarkNumIdx].mark == markValue)
		{
			if(remark8021p!=-1)
				mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark=remark8021p;

			if(remarkDscp != -1){
				mark2Priority[netifIdx][qosMarkNumIdx].version |= version;

				if(version == IPV4_VERSION){
					mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark =remarkDscp;
				}
				#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
				else if((version == IPV4_IPV6_VERSION) || ((version & IPV4_VERSION) && (version & IPV6_VERSION))){
					mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark =remarkDscp;
					mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark =remarkDscp;
				}
				else if(version == IPV6_VERSION){
					mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark =remarkDscp;
				}
				#endif
			}
		}
	}
	
	rtl865x_syncRemarkInfoToAsic(netIfName);
	return SUCCESS;
}

int32 rtl865x_qosFlushMarkRuleByDev(uint8 *netIfName)
{
	int i;
	rtl865x_qosFlushMarkRule();
	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0 && (strcmp(netIfName,netIfNameArray[i])!=0))
		{
			_rtl865x_qosArrangeRuleByNetif(netIfNameArray[i]);
		}
	}

	return SUCCESS;
}

int32 rtl865x_qosRearrangeRule(void)
{
	int i;
	rtl865x_qosFlushMarkRule();
	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			_rtl865x_qosArrangeRuleByNetif(netIfNameArray[i]);
		}
	}

	return SUCCESS;
}

int rtl865x_restoreDscpRemark(char *dev_name, int flag)
{
	int i, idx;

	if(dev_name == NULL)
		return FAILED;
	
	idx = _rtl865x_getNetifIdxByNameExt(dev_name);
	if((idx < 0) || (idx >= NETIF_NUMBER))
		return FAILED;
	
	WRITE_MEM32(DSCPRM0, (READ_MEM32(DSCPRM0) &~(0x3FFFFFFF)));	 
	WRITE_MEM32(DSCPRM1, (READ_MEM32(DSCPRM1) &~(0x3FFFF)));	

	/* add flag for 8367 qos
	   dynamically config dscp remark on 8367 or 97f, need to reserve mark2Priority setting
	*/
 	if(flag){
		for(i=0;i<MAX_MARK_NUM_PER_DEV;i++){
			mark2Priority[idx][i].dscpRemark=0xFF;
		}
 	}

	return SUCCESS;
}

#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
int rtl865x_restoreV6DscpRemark(char *dev_name)
{
	int i, idx;

	if(dev_name == NULL)
		return FAILED;
	
	idx = _rtl865x_getNetifIdxByNameExt(dev_name);
	if((idx < 0) || (idx >= NETIF_NUMBER))
		return FAILED;

	WRITE_MEM32(V6DSCPRMK0, (READ_MEM32(V6DSCPRMK0) &~(0x3FFFFFFF)));	 
	WRITE_MEM32(V6DSCPRMK1, (READ_MEM32(V6DSCPRMK1) &~(0x3FFFF)));

	for(i=0;i<MAX_MARK_NUM_PER_DEV;i++){
		mark2Priority[idx][i].v6DscpRemark=0xFF;
	}

	return SUCCESS;
}
#endif

int rtl865x_restoreVlanRemark(char *dev_name)
{
	int i, idx;

	if(dev_name == NULL)
		return FAILED;

	idx = _rtl865x_getNetifIdxByNameExt(dev_name);
	if((idx < 0) || (idx >= NETIF_NUMBER))
		return FAILED;

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	rtl83xx_flush_1p_remark_setting();
#else
	// don't modify 97f 1p remark because system priority delivery is fixed in rtl865x_initOutputQueue
	WRITE_MEM32(RMCR1P, (READ_MEM32(RMCR1P) &~(0xFFFFFF)));	  
#endif

	for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
		mark2Priority[idx][i].vlanpriRemark=0xFF;

	return SUCCESS;
}

int rtl_dump_AsicRemark_info(struct seq_file *s)
{
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	int i=0, j;
#else
	int i=4, j;
#endif
	int dscpRemark=0;
	int vlanpriRemark=0;
#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
	int v6DscpRemark = 0;
#endif
	
	seq_printf(s, "Dump Asic remark Info:\n");

	//for(i=0; i<=CPU; i++){
		for(j=0; j<=PRI7; j++)
		{
			rtl8651_getAsicDscpRemark(i, j, &dscpRemark);
			rtl8651_getAsicVlanRemark(i, j, &vlanpriRemark);
#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
			rtl8651_getAsicV6DscpRemark(i, j, &v6DscpRemark);
			seq_printf(s, "    Port[%d]'s sys_pri[%d] is remarked as v4Dscp[%d] v6Dscp[%d] 802.1p priority[%d]\n", i, j, dscpRemark, v6DscpRemark, vlanpriRemark);
#else
			seq_printf(s, "    Port[%d]'s sys_pri[%d] is remarked as v4Dscp[%d] 802.1p priority[%d]\n", i, j, dscpRemark, vlanpriRemark);
#endif
		}
	//}
	return SUCCESS;
}

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
extern int rtl83xx_remark_confg_get(int port, int *dscpRemkStat, int *d1pRemkStas);
extern int rtl83xx_remark_get(int sysPri, int *dscpRemk, int *d1pRemk);
int rtl_dump_Asic83xxRemark_info(struct seq_file *s)
{
	int port, j;
	int dscpRemkStat = -1, d1pRemkStas = -1, dscpRemk = 0, d1pRemk;

	for(port = 0; port <= CPU ; port++)
	{
		if((1 << port) & RTL_WANPORT_MASK){
			seq_printf(s, "\nDump Asic 83xx remark Info:\n");
			rtl83xx_remark_confg_get(port, &dscpRemkStat, &d1pRemkStas);

			if(!dscpRemkStat)
				seq_printf(s, "    dscp remark: %s\n", "disable");
			else
				seq_printf(s, "    dscp remark: %s\n", "enable");

			if(!d1pRemkStas)
				seq_printf(s, "    802.1p remark: %s\n\n", "disable");
			else
				seq_printf(s, "    802.1p remark: %s\n\n", "enable");
			
			for(j=0; j<=PRI7; j++){
				rtl83xx_remark_get(j, &dscpRemk, &d1pRemk);
				seq_printf(s, "    83xx port[%d] sys_pri[%d] is remarked as dscp[%d] 802.1p priority[%d]\n", port, j, dscpRemk, d1pRemk);
			}
		}
	}
	
	return SUCCESS;
}
#endif

int rtl_dump_configQosRemark_info(struct seq_file *s)
{
	int qosMarkNumIdx=0;
	rtl865x_netif_local_t	*netIf;
	int netifIdx=0;
	
	seq_printf(s, "Dump config qosInfo:\n");
	for(netifIdx=0;netifIdx<NETIF_NUMBER;netifIdx++)
	{
		netIf = _rtl865x_getNetifByIdx(netifIdx);
		if(netIf == NULL)
			continue;
		
		seq_printf(s, "\n%s config qosInfo:\n",netIf->name);
		for(qosMarkNumIdx=0;qosMarkNumIdx<MAX_MARK_NUM_PER_DEV;qosMarkNumIdx++)
		{
			if(mark2Priority[netifIdx][qosMarkNumIdx].mark)
			{
				#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
				uint8 version = mark2Priority[netifIdx][qosMarkNumIdx].version;
				
				if((version == IPV4_IPV6_VERSION) || ((version & IPV4_VERSION) && (version & IPV6_VERSION))){
					seq_printf(s, "mark [%d] -> priority[%d]-> v4Dscp[%d]: v6Dscp[%d]: vlanpriority[%d]\n", 
					mark2Priority[netifIdx][qosMarkNumIdx].mark,
					mark2Priority[netifIdx][qosMarkNumIdx].priority,
					mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark, 
					mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark,
					mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark);
				}
				else if(version == IPV6_VERSION){
					seq_printf(s, "mark [%d] -> priority[%d]-> v6Dscp[%d]:vlanpriority[%d]\n", 
					mark2Priority[netifIdx][qosMarkNumIdx].mark,
					mark2Priority[netifIdx][qosMarkNumIdx].priority,
					mark2Priority[netifIdx][qosMarkNumIdx].v6DscpRemark,
					mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark);
				}
				else
				#endif
				{
					seq_printf(s, "mark [%d] -> priority[%d]-> v4Dscp[%d]:vlanpriority[%d]\n", 
					mark2Priority[netifIdx][qosMarkNumIdx].mark,
					mark2Priority[netifIdx][qosMarkNumIdx].priority,
					mark2Priority[netifIdx][qosMarkNumIdx].dscpRemark,
					mark2Priority[netifIdx][qosMarkNumIdx].vlanpriRemark);
				}
			}
		}
		seq_printf(s, "====================\n");
	}
	
	return SUCCESS;
}
#endif

int32 rtl865x_closeQos(uint8 *netIfName)
{
	uint32	memberPort;
	uint32	i;
	const int32	idx = _rtl865x_getNetifIdxByNameExt(netIfName);
	int32		tmp_idx;
#if !defined (CONFIG_RTL_83XX_QOS_SUPPORT)
	uint32 port;
#endif

	memberPort = rtl865x_qosGetMemberPortbyNetif(netIfName);

	if((idx < 0) || (idx >= NETIF_NUMBER))
		return FAILED;
	/* clear all acl rules */
#if	defined(CONFIG_RTL_QOS_8021P_SUPPORT)
	rtl8651_flushAsicDot1qAbsolutelyPriority();
#endif
	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			tmp_idx = _rtl865x_getNetifIdxByNameExt(netIfNameArray[i]);
			if (tmp_idx>=0&&tmp_idx<NETIF_NUMBER&&tmp_idx!=idx)
			{
				rtl865x_flush_allAcl_fromChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
				rtl865x_flush_allAcl_fromChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
			}
		}
	}

	
	defPriority[idx] = 0;
	queueNumber[idx] = 0;
	memset(priority2HandleMapping[idx], 0, TOTAL_VLAN_PRIORITY_NUM*sizeof(int));
	for(i=0;i<MAX_MARK_NUM_PER_DEV;i++)
#ifdef DEFAULT_PRIORITY_PATCH
		mark2Priority[idx][i].priority = -1;	//Only clear priority here, mark need to be stored here.
#else	
		mark2Priority[idx][i].priority=0;	//Only clear priority here, mark need to be stored here.
#endif
	/* keep the rule of RTL865X_ACL_QOS_USED2 untouched */

#if defined (CONFIG_RTL_83XX_QOS_SUPPORT)
	rtl83xx_closeQos(memberPort, RTL_WANPORT_MASK);
#else
	rtl865xC_lockSWCore();
	for(port=0;port<=CPU;port++)
	{
		if(((1<<port)&memberPort)==0)
			continue;
		
		for(i=0;i<RTL8651_OUTPUTQUEUE_SIZE;i++)
		{
			rtl8651_setAsicQueueWeight(port, i, STR_PRIO, 0);
			rtl8651_setAsicQueueRate(port,i, PPR_MASK>>PPR_OFFSET, L1_MASK>>L1_OFFSET,APR_MASK>>APR_OFFSET);
		}

		rtl8651_setAsicOutputQueueNumber(port, 1);
	}

	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();
#endif
	return SUCCESS;
}

int __init rtl865x_initOutputQueue(uint8 **netIfName)
{
	int	i,j;
	
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	int prio;
#endif

	rtl865xC_lockSWCore();
	for(i=0;i<RTL8651_OUTPUTQUEUE_SIZE;i++)
	{
		for(j=0;j<TOTAL_VLAN_PRIORITY_NUM;j++)
			rtl8651_setAsicPriorityToQIDMappingTable(i+1, j, priorityMatrix[i][j]);
	}

	/* default value is 0, so skip the code.
	for(j=0;j<8;j++) {
		//rtl8651_setAsicCPUPriorityToQIDMappingTable(CPU, j, j<4?0:5);
		rtl8651_setAsicCPUPriorityToQIDMappingTable(CPU, j, 0);
	}
	*/
	#if defined(CONFIG_RTL_8197F)
	rtl8651_setAsicPriorityDecision(priorityDecisionArray[PORT_BASE], 
		priorityDecisionArray[D1P_BASE], priorityDecisionArray[DSCP_BASE], 
		priorityDecisionArray[ACL_BASE], priorityDecisionArray[NAT_BASE],
		priorityDecisionArray[VID_BASE]);
	#else
	rtl8651_setAsicPriorityDecision(priorityDecisionArray[PORT_BASE], 
		priorityDecisionArray[D1P_BASE], priorityDecisionArray[DSCP_BASE], 
		priorityDecisionArray[ACL_BASE], priorityDecisionArray[NAT_BASE]);
	#endif

#if	defined(CONFIG_RTL_QOS_8021P_SUPPORT)
	rtl8651_flushAsicDot1qAbsolutelyPriority();
#endif
#if !defined(CONFIG_OPENWRT_SDK)
	hw_qos_init_netlink();
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)	
	hw_83xx_qos_init_netlink();
#endif
#endif
	for(i =0; i < RTL8651_OUTPUTQUEUE_SIZE; i++)
	{
		for(j=PHY0;j<=CPU;j++)
		{
			/* 1. If disable hw queue flow control, issues as follows:
			  * 	1) 1Gbps ether port to 100Mbps ether port chariot traffic not stable.
			  * 	2) hw qos not so precise
			  * 2. If enable hw queue flow control, note as follows:
			  *	1) for hw qos and using 2 chariot tcp downlink traffic,
			  *      the no-matched traffic throughput will be pull-down because of the hw queue flow control.
			  *
			  * So we enable hw queue flow control as default setting at present.
			  * 2011-04-02, zj
			  */
			if (rtl8651_setAsicQueueFlowControlConfigureRegister(j, i, TRUE)!=SUCCESS)
			{
				QOS_DEBUGP("Set Queue Flow Control Para Error.\n");
			}
		}
	}

	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();
	
	memcpy(netIfNameArray, netIfName, NETIF_NUMBER*IFNAMSIZ);

	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			rtl865x_regist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
			rtl865x_regist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
		}
	}

	memset(defPriority, 0, NETIF_NUMBER*sizeof(uint32));
	memset(queueNumber, 0, NETIF_NUMBER*sizeof(uint32));
	memset(priority2HandleMapping, 0, NETIF_NUMBER*TOTAL_VLAN_PRIORITY_NUM*sizeof(int));
#ifdef DEFAULT_PRIORITY_PATCH
	for(i=0; i<NETIF_NUMBER; i++)
	{	
		for(j=0; j<MAX_MARK_NUM_PER_DEV; j++)
		{
			mark2Priority[i][j].priority = -1;
			mark2Priority[i][j].mark = -1;
			mark2Priority[i][j].mask = -1;
		}
	}
#else
	memset(mark2Priority, 0, NETIF_NUMBER*MAX_MARK_NUM_PER_DEV*sizeof(rtl_qos_mark_info_t));
#endif

	rtl865x_qosRuleHead = NULL;
	if (rtl865x_compFunc==NULL)
		rtl865x_compFunc = _rtl865x_compare2Entry;
		
	for(i=0;i<NETIF_NUMBER;i++)	
	{
		for(j=0;j<MAX_MARK_NUM_PER_DEV;j++){
			mark2Priority[i][j].dscpRemark =0xff;
			mark2Priority[i][j].vlanpriRemark=0xff;
		#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
			mark2Priority[i][j].v6DscpRemark =0xff;
		#endif
		}
	
	}

#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
	#ifdef CONFIG_RTL_PROC_NEW
    proc_create_data("rtl_hw_qos_config",0,&proc_root,&rtl_hw_qos_config_proc_fops,NULL);
	#else
	rtl_hw_qos_config_entry = create_proc_entry("rtl_hw_qos_config", 0, NULL);
	if(rtl_hw_qos_config_entry)
	{
		rtl_hw_qos_config_entry->read_proc = rtl_hw_qos_config_read;
		rtl_hw_qos_config_entry->write_proc = rtl_hw_qos_config_write;
	}
	#endif
#endif
	
#if defined(CONFIG_RTL_HW_QOS_BRIDGE_FWD_SUPPORT)
	for (i=0; i<NETIF_NUMBER; i++)
	{
		for (j=0; j<RTL8651_PORT_NUMBER; j++) 
			classId2Port[i][j] = 0;
		for (j=0; j<TOTAL_VLAN_PRIORITY_NUM; j++) 
			classId2VlanPriority[i][j] = 0;
		for (j=0; j<TOTAL_DSCP_NUM; j++) 
			classId2Dscp[i][j] = 0;
	}
#endif

#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	rtl83xx_qos_init();
	for(prio = PRI0; prio <= PRI7; prio++)
			rtl8651_setAsicVlanRemark(PHY0, prio, prio);
#endif

	return SUCCESS;
}

void __exit rtl865x_exitOutputQueue(void)
{
	int	i;

	for(i=0;i<NETIF_NUMBER;i++)
	{
		if (netIfNameArray[i][0]!=0)
		{
			rtl865x_unRegist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
			rtl865x_unRegist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
			rtl865x_qosFlushBandwidth(netIfNameArray[i]);
			rtl865x_closeQos(netIfNameArray[i]);
		}
	}
	
#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
#ifdef CONFIG_RTL_PROC_NEW
	remove_proc_entry("rtl_hw_qos_config", &proc_root);
#else
	if (rtl_hw_qos_config_entry) {
		remove_proc_entry("rtl_hw_qos_config", NULL);
		rtl_hw_qos_config_entry = NULL;
	}	
#endif	
#endif
}
#if defined (CONFIG_OPENWRT_SDK)
void rtl865x_HwQosProcess(struct net_device *dev, int flag)
{
	int32 port, queue;
	int i, netif_index;
	uint32	memberPort;
	rtl865x_netif_local_t	*netIf = NULL;
	rtl865x_vlan_entry_t	*vlan = NULL;
	
	if(flag)
	{
		if (dev) {
		    netif_index = _rtl865x_getNetifIdxByNameExt(dev->name);
		    netIf = _rtl865x_getNetifByName(dev->name);
		    if (netIf){
			vlan = _rtl8651_getVlanTableEntry(netIf->vid);
		    	if (vlan)
			    memberPort = vlan->memberPortMask;
		    }
		}
		
		if ((dev==NULL) || (netif_index<0) || (netIf==NULL) || (vlan==NULL)) {
		    rtl865xC_lockSWCore();
		    /* qos enabled */
		    for(i=0;i<NETIF_NUMBER;i++)
		    {
			queueNumber[i]=1;
		    }
		    for ( port = PHY0 ; port <= CPU ; port ++ )
		    {
			rtl8651_setAsicOutputQueueNumber(port, 1);
			
				for ( queue = QUEUE0 ; queue <= MAX_QUEUE ; queue ++ )
			{				
				rtl8651_setAsicQueueWeight(port, queue, STR_PRIO, 0);
				rtl8651_setAsicQueueRate(port,queue, PPR_MASK>>PPR_OFFSET, L1_MASK>>L1_OFFSET,APR_MASK>>APR_OFFSET);
	  		}
		    }
		} else {
		    rtl865xC_lockSWCore();
		    /*enable dev qos*/
		    queueNumber[netif_index] = 1;

		    for ( port = PHY0 ; port <= CPU ; port ++ )
                    {
			if(((1<<port)& memberPort) == 0)
				continue;
                        rtl8651_setAsicOutputQueueNumber(port, 1);

				for ( queue = QUEUE0 ; queue <= MAX_QUEUE ; queue ++ )
                        {
                                rtl8651_setAsicQueueWeight(port, queue, STR_PRIO, 0);
                                rtl8651_setAsicQueueRate(port,queue, PPR_MASK>>PPR_OFFSET, L1_MASK>>L1_OFFSET,APR_MASK>>APR_OFFSET);
                        }
                    }
		}

		rtl865xC_waitForOutputQueueEmpty();
		rtl8651_resetAsicOutputQueue();
		rtl865xC_unLockSWCore();
#if defined (DEFAULT_PRIORITY_PATCH)		
		for(i=0;i<NETIF_NUMBER;i++)
		{
			if (netIfNameArray[i][0]!=0)
			{
				rtl865x_regist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
				rtl865x_regist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
			}
		}
#endif
	}
	else  if(flag ==0)
	{
		if (dev == NULL) {
		    for(i=0;i<NETIF_NUMBER;i++)
		    {
			if (netIfNameArray[i][0]!=0){
				rtl865x_unRegist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
				rtl865x_unRegist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
				rtl865x_qosFlushBandwidth(netIfNameArray[i]);
				rtl865x_closeQos(netIfNameArray[i]);
			}
		   }
		} else {
		    for(i=0;i<NETIF_NUMBER;i++)
		    {
			if ((netIfNameArray[i][0]!=0) && (strncmp(netIfNameArray[i], dev->name, IFNAMSIZ)==0)) {
				rtl865x_unRegist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED0, RTL865X_ACL_INGRESS);
                                rtl865x_unRegist_aclChain(netIfNameArray[i], RTL865X_ACL_QOS_USED1, RTL865X_ACL_INGRESS);
                                rtl865x_qosFlushBandwidth(netIfNameArray[i]);
                                rtl865x_closeQos(netIfNameArray[i]);
			}
		    }
		}
	}

}
#endif

#if defined (CONFIG_RTL_HW_QOS_SUPPORT) && defined(CONFIG_RTL_QOS_PATCH)
void rtl865x_reinitOutputQueuePatchForQoS(uint32 qosEnabled)
{
	int32 port, queue;
	int32 i;
	int32 tmp_idx;
	rtl865x_AclRule_t	aclRule;

	rtl865xC_lockSWCore();
	
	if(qosEnabled == TRUE)
	{
		/* qos enabled */
		for(i=0;i<NETIF_NUMBER;i++)
		{
			queueNumber[i]=DEF_QUEUE_NUM;
		}
		for ( port = PHY0 ; port <= CPU ; port ++ )
		{
			rtl8651_setAsicOutputQueueNumber(port, DEF_QUEUE_NUM);
			
			for ( queue = QUEUE0 ; queue <= MAX_QUEUE ; queue ++ )
			{				
				rtl8651_setAsicQueueWeight(port, queue, STR_PRIO, 0);
				rtl8651_setAsicQueueRate(port,queue, PPR_MASK>>PPR_OFFSET, L1_MASK>>L1_OFFSET,APR_MASK>>APR_OFFSET);
	  		}
		}

	}
	else
	{
		/* qos disabled */
		for(i=0;i<NETIF_NUMBER;i++)
		{
			queueNumber[i]=MAX_QOS_PATCH_QUEUE_NUM;
		}

		for ( port = PHY0 ; port <= CPU ; port ++ )
		{				
			rtl8651_setAsicOutputQueueNumber(port, MAX_QOS_PATCH_QUEUE_NUM);
			
			for ( queue = QUEUE0 ; queue <= MAX_QUEUE ; queue ++ )
			{
				if((queue == QUEUE0) || (queue == QUEUE5))
					rtl8651_setAsicQueueRate(port, queue,PPR_MASK>>PPR_OFFSET, L1_MASK>>L1_OFFSET, APR_MASK>>APR_OFFSET);	// full speed
			}
		}

		// Add ACL rule for tcp dport(80) highest priority 7 to accelarate webpage access when heavy traffic load
		memset(&aclRule, 0, sizeof(rtl865x_AclRule_t));
		aclRule.ruleType_ = RTL865X_ACL_TCP;	
		aclRule.tcpSrcPortUB_=65535;
		aclRule.tcpSrcPortLB_=0;
		aclRule.tcpDstPortUB_=80;
		aclRule.tcpDstPortLB_=80;				
		aclRule.actionType_ = RTL865X_ACL_PRIORITY;
		aclRule.pktOpApp_ = RTL865X_ACL_ALL_LAYER;
		aclRule.priority_ = 7;
		i = 0;
		for(i=0;i<NETIF_NUMBER;i++)	//For all interfaces
		{
			if (netIfNameArray[i][0]!=0)
			{
				tmp_idx = _rtl865x_getNetifIdxByNameExt(netIfNameArray[i]);
				if (tmp_idx>=0&&tmp_idx<NETIF_NUMBER)
				{
					rtl865x_add_acl(&aclRule, netIfNameArray[i], RTL865X_ACL_QOS_USED1);
				}
			}
		}
	}
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();
}
#endif

#if defined(CONFIG_RTL_PROC_DEBUG)
int32 rtl865x_show_allQosAcl(struct seq_file *s)
{
	rtl865x_qos_rule_t	*qosRule;
	rtl865x_AclRule_t *rule;
	int8 *actionT[] = { "permit", "redirect to ether", "drop", "to cpu", "legacy drop", 
					"drop for log", "mirror", "redirect to pppoe", "default redirect", "mirror keep match", 
					"drop rate exceed pps", "log rate exceed pps", "drop rate exceed bps", "log rate exceed bps","priority "
					};

	for(qosRule = rtl865x_qosRuleHead;qosRule;qosRule=qosRule->next)
	{
		rule = qosRule->rule;
		switch(rule->ruleType_)
		{
			case RTL865X_ACL_MAC:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "Ethernet", actionT[rule->actionType_]);
				seq_printf(s,"\tether type: %x   ether type mask: %x\n", rule->typeLen_, rule->typeLenMask_);
				seq_printf(s,"\tDMAC: %x:%x:%x:%x:%x:%x  DMACM: %x:%x:%x:%x:%x:%x\n",
						rule->dstMac_.octet[0], rule->dstMac_.octet[1], rule->dstMac_.octet[2],
						rule->dstMac_.octet[3], rule->dstMac_.octet[4], rule->dstMac_.octet[5],
						rule->dstMacMask_.octet[0], rule->dstMacMask_.octet[1], rule->dstMacMask_.octet[2],
						rule->dstMacMask_.octet[3], rule->dstMacMask_.octet[4], rule->dstMacMask_.octet[5]
						);
				
				seq_printf(s, "\tSMAC: %x:%x:%x:%x:%x:%x  SMACM: %x:%x:%x:%x:%x:%x\n",
						rule->srcMac_.octet[0], rule->srcMac_.octet[1], rule->srcMac_.octet[2],
						rule->srcMac_.octet[3], rule->srcMac_.octet[4], rule->srcMac_.octet[5],
						rule->srcMacMask_.octet[0], rule->srcMacMask_.octet[1], rule->srcMacMask_.octet[2],
						rule->srcMacMask_.octet[3], rule->srcMacMask_.octet[4], rule->srcMacMask_.octet[5]
					);
				break;

			case RTL865X_ACL_IP:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "IP", actionT[rule->actionType_]);
				seq_printf(s, "\tdip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos: %x   TosM: %x   ipProto: %x   ipProtoM: %x   ipFlag: %x   ipFlagM: %x\n",
						rule->tos_, rule->tosMask_, rule->ipProto_, rule->ipProtoMask_, rule->ipFlag_, rule->ipFlagMask_
					);
				
				seq_printf(s,"\t<FOP:%x> <FOM:%x> <http:%x> <httpM:%x> <IdentSdip:%x> <IdentSdipM:%x> \n",
						rule->ipFOP_, rule->ipFOM_, rule->ipHttpFilter_, rule->ipHttpFilterM_, rule->ipIdentSrcDstIp_,
						rule->ipIdentSrcDstIpM_
						);
				seq_printf(s, "\t<DF:%x> <MF:%x>\n", rule->ipDF_, rule->ipMF_); 
					break;
					
			case RTL865X_ACL_IP_RANGE:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "IP Range", actionT[rule->actionType_]);
				seq_printf(s,"\tdipU: %d.%d.%d.%d dipL: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsipU: %d.%d.%d.%d sipL: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos: %x   TosM: %x   ipProto: %x   ipProtoM: %x   ipFlag: %x   ipFlagM: %x\n",
						rule->tos_, rule->tosMask_, rule->ipProto_, rule->ipProtoMask_, rule->ipFlag_, rule->ipFlagMask_
						);
				seq_printf(s,"\t<FOP:%x> <FOM:%x> <http:%x> <httpM:%x> <IdentSdip:%x> <IdentSdipM:%x> \n",
						rule->ipFOP_, rule->ipFOM_, rule->ipHttpFilter_, rule->ipHttpFilterM_, rule->ipIdentSrcDstIp_,
						rule->ipIdentSrcDstIpM_
						);
					seq_printf(s,"\t<DF:%x> <MF:%x>\n", rule->ipDF_, rule->ipMF_); 
					break;			
			case RTL865X_ACL_ICMP:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "ICMP", actionT[rule->actionType_]);
				seq_printf(s,"\tdip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos: %x   TosM: %x   type: %x   typeM: %x   code: %x   codeM: %x\n",
						rule->tos_, rule->tosMask_, rule->icmpType_, rule->icmpTypeMask_, 
						rule->icmpCode_, rule->icmpCodeMask_);
				break;
			case RTL865X_ACL_ICMP_IPRANGE:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "ICMP IP RANGE", actionT[rule->actionType_]);
				seq_printf(s,"\tdipU: %d.%d.%d.%d dipL: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsipU: %d.%d.%d.%d sipL: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos: %x   TosM: %x   type: %x   typeM: %x   code: %x   codeM: %x\n",
						rule->tos_, rule->tosMask_, rule->icmpType_, rule->icmpTypeMask_, 
						rule->icmpCode_, rule->icmpCodeMask_);
				break;
			case RTL865X_ACL_IGMP:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "IGMP", actionT[rule->actionType_]);
				seq_printf(s,"\tdip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos: %x   TosM: %x   type: %x   typeM: %x\n", rule->tos_, rule->tosMask_,
						rule->igmpType_, rule->igmpTypeMask_
						);
				break;


			case RTL865X_ACL_IGMP_IPRANGE:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "IGMP IP RANGE", actionT[rule->actionType_]);
				seq_printf(s,"\tdip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos: %x   TosM: %x   type: %x   typeM: %x\n", rule->tos_, rule->tosMask_,
						rule->igmpType_, rule->igmpTypeMask_
						);
				break;

			case RTL865X_ACL_TCP:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "TCP", actionT[rule->actionType_]);
				seq_printf(s,"\tdip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos:%x  TosM:%x  sportL:%d  sportU:%d  dportL:%d  dportU:%d\n",
						rule->tos_, rule->tosMask_, rule->tcpSrcPortLB_, rule->tcpSrcPortUB_,
						rule->tcpDstPortLB_, rule->tcpDstPortUB_
						);
				seq_printf(s,"\tflag: %x  flagM: %x  <URG:%x> <ACK:%x> <PSH:%x> <RST:%x> <SYN:%x> <FIN:%x>\n",
						rule->tcpFlag_, rule->tcpFlagMask_, rule->tcpURG_, rule->tcpACK_,
						rule->tcpPSH_, rule->tcpRST_, rule->tcpSYN_, rule->tcpFIN_
						);
				break;
			case RTL865X_ACL_TCP_IPRANGE:
					seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "TCP IP RANGE", actionT[rule->actionType_]);
					seq_printf(s,"\tdipU: %d.%d.%d.%d dipL: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
					seq_printf(s,"\tsipU: %d.%d.%d.%d sipL: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
					seq_printf(s,"\tTos:%x  TosM:%x  sportL:%d  sportU:%d  dportL:%d  dportU:%d\n",
						rule->tos_, rule->tosMask_, rule->tcpSrcPortLB_, rule->tcpSrcPortUB_,
						rule->tcpDstPortLB_, rule->tcpDstPortUB_
						);
					seq_printf(s,"\tflag: %x  flagM: %x  <URG:%x> <ACK:%x> <PSH:%x> <RST:%x> <SYN:%x> <FIN:%x>\n",
						rule->tcpFlag_, rule->tcpFlagMask_, rule->tcpURG_, rule->tcpACK_,
						rule->tcpPSH_, rule->tcpRST_, rule->tcpSYN_, rule->tcpFIN_
					);
				break;

			case RTL865X_ACL_UDP:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx,"UDP", actionT[rule->actionType_]);
				seq_printf(s,"\tdip: %d.%d.%d.%d dipM: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d sipM: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos:%x  TosM:%x  sportL:%d  sportU:%d  dportL:%d  dportU:%d\n",
						rule->tos_, rule->tosMask_, rule->udpSrcPortLB_, rule->udpSrcPortUB_,
						rule->udpDstPortLB_, rule->udpDstPortUB_
						);
				break;				
			case RTL865X_ACL_UDP_IPRANGE:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "UDP IP RANGE", actionT[rule->actionType_]);
				seq_printf(s,"\tdipU: %d.%d.%d.%d dipL: %d.%d.%d.%d\n", (rule->dstIpAddr_>>24),
						((rule->dstIpAddr_&0x00ff0000)>>16), ((rule->dstIpAddr_&0x0000ff00)>>8),
						(rule->dstIpAddr_&0xff), (rule->dstIpAddrMask_>>24), ((rule->dstIpAddrMask_&0x00ff0000)>>16),
						((rule->dstIpAddrMask_&0x0000ff00)>>8), (rule->dstIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsipU: %d.%d.%d.%d sipL: %d.%d.%d.%d\n", (rule->srcIpAddr_>>24),
						((rule->srcIpAddr_&0x00ff0000)>>16), ((rule->srcIpAddr_&0x0000ff00)>>8),
						(rule->srcIpAddr_&0xff), (rule->srcIpAddrMask_>>24), ((rule->srcIpAddrMask_&0x00ff0000)>>16),
						((rule->srcIpAddrMask_&0x0000ff00)>>8), (rule->srcIpAddrMask_&0xff)
						);
				seq_printf(s,"\tTos:%x  TosM:%x  sportL:%d  sportU:%d  dportL:%d  dportU:%d\n",
						rule->tos_, rule->tosMask_, rule->udpSrcPortLB_, rule->udpSrcPortUB_,
						rule->udpDstPortLB_, rule->udpDstPortUB_
					);
				break;				

			
			case RTL865X_ACL_SRCFILTER:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "Source Filter", actionT[rule->actionType_]);
				seq_printf(s,"\tSMAC: %x:%x:%x:%x:%x:%x  SMACM: %x:%x:%x:%x:%x:%x\n", 
						rule->srcFilterMac_.octet[0], rule->srcFilterMac_.octet[1], rule->srcFilterMac_.octet[2], 
						rule->srcFilterMac_.octet[3], rule->srcFilterMac_.octet[4], rule->srcFilterMac_.octet[5],
						rule->srcFilterMacMask_.octet[0], rule->srcFilterMacMask_.octet[1], rule->srcFilterMacMask_.octet[2],
						rule->srcFilterMacMask_.octet[3], rule->srcFilterMacMask_.octet[4], rule->srcFilterMacMask_.octet[5]
						);
				seq_printf(s,"\tsvidx: %d   svidxM: %x   sport: %d   sportM: %x   ProtoType: %x\n",
						rule->srcFilterVlanIdx_, rule->srcFilterVlanIdxMask_, rule->srcFilterPort_, rule->srcFilterPortMask_,
						(rule->srcFilterIgnoreL3L4_==TRUE? 2: (rule->srcFilterIgnoreL4_ == 1? 1: 0))
						);
				seq_printf(s,"\tsip: %d.%d.%d.%d   sipM: %d.%d.%d.%d\n", (rule->srcFilterIpAddr_>>24),
						((rule->srcFilterIpAddr_&0x00ff0000)>>16), ((rule->srcFilterIpAddr_&0x0000ff00)>>8),
						(rule->srcFilterIpAddr_&0xff), (rule->srcFilterIpAddrMask_>>24),
						((rule->srcFilterIpAddrMask_&0x00ff0000)>>16), ((rule->srcFilterIpAddrMask_&0x0000ff00)>>8),
						(rule->srcFilterIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsportL: %d   sportU: %d\n", rule->srcFilterPortLowerBound_, rule->srcFilterPortUpperBound_);
				break;

			case RTL865X_ACL_SRCFILTER_IPRANGE:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "Source Filter(IP RANGE)", actionT[rule->actionType_]);
				seq_printf(s,"\tSMAC: %x:%x:%x:%x:%x:%x  SMACM: %x:%x:%x:%x:%x:%x\n", 
						rule->srcFilterMac_.octet[0], rule->srcFilterMac_.octet[1], rule->srcFilterMac_.octet[2], 
						rule->srcFilterMac_.octet[3], rule->srcFilterMac_.octet[4], rule->srcFilterMac_.octet[5],
						rule->srcFilterMacMask_.octet[0], rule->srcFilterMacMask_.octet[1], rule->srcFilterMacMask_.octet[2],
						rule->srcFilterMacMask_.octet[3], rule->srcFilterMacMask_.octet[4], rule->srcFilterMacMask_.octet[5]
						);
				seq_printf(s,"\tsvidx: %d   svidxM: %x   sport: %d   sportM: %x   ProtoType: %x\n",
						rule->srcFilterVlanIdx_, rule->srcFilterVlanIdxMask_, rule->srcFilterPort_, rule->srcFilterPortMask_,
						(rule->srcFilterIgnoreL3L4_==TRUE? 2: (rule->srcFilterIgnoreL4_ == 1? 1: 0))
						);
				seq_printf(s,"\tsipU: %d.%d.%d.%d   sipL: %d.%d.%d.%d\n", (rule->srcFilterIpAddr_>>24),
						((rule->srcFilterIpAddr_&0x00ff0000)>>16), ((rule->srcFilterIpAddr_&0x0000ff00)>>8),
						(rule->srcFilterIpAddr_&0xff), (rule->srcFilterIpAddrMask_>>24),
						((rule->srcFilterIpAddrMask_&0x00ff0000)>>16), ((rule->srcFilterIpAddrMask_&0x0000ff00)>>8),
						(rule->srcFilterIpAddrMask_&0xff)
						);
				seq_printf(s,"\tsportL: %d   sportU: %d\n", rule->srcFilterPortLowerBound_, rule->srcFilterPortUpperBound_);
				break;

			case RTL865X_ACL_DSTFILTER:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "Deatination Filter", actionT[rule->actionType_]);
				seq_printf(s,"\tDMAC: %x:%x:%x:%x:%x:%x  DMACM: %x:%x:%x:%x:%x:%x\n", 
						rule->dstFilterMac_.octet[0], rule->dstFilterMac_.octet[1], rule->dstFilterMac_.octet[2], 
						rule->dstFilterMac_.octet[3], rule->dstFilterMac_.octet[4], rule->dstFilterMac_.octet[5],
						rule->dstFilterMacMask_.octet[0], rule->dstFilterMacMask_.octet[1], rule->dstFilterMacMask_.octet[2],
						rule->dstFilterMacMask_.octet[3], rule->dstFilterMacMask_.octet[4], rule->dstFilterMacMask_.octet[5]
						);
				seq_printf(s,"\tdvidx: %d   dvidxM: %x  ProtoType: %x   dportL: %d   dportU: %d\n",
						rule->dstFilterVlanIdx_, rule->dstFilterVlanIdxMask_, 
						(rule->dstFilterIgnoreL3L4_==TRUE? 2: (rule->dstFilterIgnoreL4_ == 1? 1: 0)), 
						rule->dstFilterPortLowerBound_, rule->dstFilterPortUpperBound_
						);
				seq_printf(s,"\tdip: %d.%d.%d.%d   dipM: %d.%d.%d.%d\n", (rule->dstFilterIpAddr_>>24),
						((rule->dstFilterIpAddr_&0x00ff0000)>>16), ((rule->dstFilterIpAddr_&0x0000ff00)>>8),
						(rule->dstFilterIpAddr_&0xff), (rule->dstFilterIpAddrMask_>>24),
						((rule->dstFilterIpAddrMask_&0x00ff0000)>>16), ((rule->dstFilterIpAddrMask_&0x0000ff00)>>8),
						(rule->dstFilterIpAddrMask_&0xff)
						);
				break;
			case RTL865X_ACL_DSTFILTER_IPRANGE:
				seq_printf(s," [%d] rule type: %s   rule action: %s\n", rule->aclIdx, "Deatination Filter(IP Range)", actionT[rule->actionType_]);
				seq_printf(s,"\tDMAC: %x:%x:%x:%x:%x:%x  DMACM: %x:%x:%x:%x:%x:%x\n", 
						rule->dstFilterMac_.octet[0], rule->dstFilterMac_.octet[1], rule->dstFilterMac_.octet[2], 
						rule->dstFilterMac_.octet[3], rule->dstFilterMac_.octet[4], rule->dstFilterMac_.octet[5],
						rule->dstFilterMacMask_.octet[0], rule->dstFilterMacMask_.octet[1], rule->dstFilterMacMask_.octet[2],
						rule->dstFilterMacMask_.octet[3], rule->dstFilterMacMask_.octet[4], rule->dstFilterMacMask_.octet[5]
						);
				seq_printf(s,"\tdvidx: %d   dvidxM: %x  ProtoType: %x   dportL: %d   dportU: %d\n",
						rule->dstFilterVlanIdx_, rule->dstFilterVlanIdxMask_, 
						(rule->dstFilterIgnoreL3L4_==TRUE? 2: (rule->dstFilterIgnoreL4_ == 1? 1: 0)), 
						rule->dstFilterPortLowerBound_, rule->dstFilterPortUpperBound_
						);
				seq_printf(s,"\tdipU: %d.%d.%d.%d   dipL: %d.%d.%d.%d\n", (rule->dstFilterIpAddr_>>24),
						((rule->dstFilterIpAddr_&0x00ff0000)>>16), ((rule->dstFilterIpAddr_&0x0000ff00)>>8),
						(rule->dstFilterIpAddr_&0xff), (rule->dstFilterIpAddrMask_>>24),
						((rule->dstFilterIpAddrMask_&0x00ff0000)>>16), ((rule->dstFilterIpAddrMask_&0x0000ff00)>>8),
						(rule->dstFilterIpAddrMask_&0xff)
					);
				break;

				default:
					seq_printf(s,"rule->ruleType_(0x%x)\n", rule->ruleType_);

		}	

		switch (rule->actionType_) 
		{
			case RTL865X_ACL_PERMIT:
			case RTL865X_ACL_REDIRECT_ETHER:
			case RTL865X_ACL_DROP:
			case RTL865X_ACL_TOCPU:
			case RTL865X_ACL_LEGACY_DROP:
			case RTL865X_ACL_DROPCPU_LOG:
			case RTL865X_ACL_MIRROR:
			case RTL865X_ACL_REDIRECT_PPPOE:
			case RTL865X_ACL_MIRROR_KEEP_MATCH:
				seq_printf(s,"\tnetifIdx: %d   pppoeIdx: %d   l2Idx:%d  ", rule->netifIdx_, rule->pppoeIdx_, rule->L2Idx_);
				break;

			case RTL865X_ACL_PRIORITY: 
				seq_printf(s,"\tprioirty: %d   ",  rule->priority_) ;
				break;
				
			case RTL865X_ACL_DEFAULT_REDIRECT:
				seq_printf(s,"\tnextHop:%d  ",  rule->nexthopIdx_);
				break;

			case RTL865X_ACL_DROP_RATE_EXCEED_PPS:
			case RTL865X_ACL_LOG_RATE_EXCEED_PPS:
			case RTL865X_ACL_DROP_RATE_EXCEED_BPS:
			case RTL865X_ACL_LOG_RATE_EXCEED_BPS:
				seq_printf(s,"\tratelimitIdx: %d  ",  rule->ratelimtIdx_);
				break;
			default: 
				;
			
		}
		seq_printf(s,"pktOpApp: %d	handler:0x%x Mark: %d\n",  rule->pktOpApp_, qosRule->handle, qosRule->mark);
		seq_printf(s,"InDev: %s	OutDev: %s\n", qosRule->inIfname[0]==0?"NULL":qosRule->inIfname, qosRule->outIfname[0]==0?"NULL":qosRule->outIfname);
		
		rule = rule->next;
	}
	
	return SUCCESS;

}
#endif

#if defined(CONFIG_RTL_HW_QOS_BRIDGE_FWD_SUPPORT)
static int rtl865x_setPortPriorityByClassId(int port, int netifIdx, unsigned int classId)
{
	int priority = 0;

	priority = _rtl865x_qosGetPriorityByHandle(netifIdx, classId);
	
	rtl865xC_lockSWCore();
	
	rtl8651_setAsicPortPriority(port, priority);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_setVlanPriorityByClassId(int vlanPri, int netifIdx, unsigned int classId)
{
	int priority = 0;

	priority = _rtl865x_qosGetPriorityByHandle(netifIdx, classId);
	
	rtl865xC_lockSWCore();
	
	rtl8651_setAsicDot1qAbsolutelyPriority(vlanPri, priority);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_setDscpPriorityByClassId(int dscp, int netifIdx, unsigned int classId)
{
	int priority = 0;

	priority = _rtl865x_qosGetPriorityByHandle(netifIdx, classId);
	
	rtl865xC_lockSWCore();
	
	rtl8651_setAsicDscpPriority(dscp, priority);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_resetPortPriority(int port)
{	
	rtl865xC_lockSWCore();
	
	rtl8651_setAsicPortPriority(port, 0);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_resetVlanPriority(int vlanPri)
{
	int priority = 0;

	priority = vlanPri2SystemPri[vlanPri];
	rtl865xC_lockSWCore();
	
	rtl8651_setAsicDot1qAbsolutelyPriority(vlanPri, priority);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_resetDscpPriority(int dscp)
{
	rtl865xC_lockSWCore();
	
	rtl8651_setAsicDscpPriority(dscp, 1);
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_flushPortPriority(void)
{
	int port;

	rtl865xC_lockSWCore();

	for (port=0; port<RTL8651_PORT_NUMBER; port++) 
	{
		rtl8651_setAsicPortPriority(port, 0);
	}
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_flushVlanPriority(void)
{
	int vlanPri, priority;

	rtl865xC_lockSWCore();

	for (vlanPri=0; vlanPri<TOTAL_VLAN_PRIORITY_NUM; vlanPri++) 
	{
		priority = vlanPri2SystemPri[vlanPri];
		rtl8651_setAsicDot1qAbsolutelyPriority(vlanPri, priority);
	}
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}

static int rtl865x_flushDscpPriority(void)
{
	int dscp;

	rtl865xC_lockSWCore();

	for (dscp=0; dscp<TOTAL_DSCP_NUM; dscp++) 
	{
		rtl8651_setAsicDscpPriority(dscp, 1);
	}
	
	rtl865xC_waitForOutputQueueEmpty();
	rtl8651_resetAsicOutputQueue();
	rtl865xC_unLockSWCore();

	return SUCCESS;
}
#endif

#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
#if defined (CONFIG_RTL_PROC_NEW)
int rtl_83xxQosConfigReadProc(struct seq_file *s, void *v)
{
	seq_printf(s, "rtl83xx hw l2 qos:%d\n", bridge_qos_enable);
	seq_printf(s, "rtl83xx hw nat qos:%d\n", nat_qos_enable);
	
	return 0;
}
#else
int rtl_83xxQosConfigReadProc(char *page, char **start, off_t off, int count, int *eof, void *data)	
{
	int len;
	
	len = sprintf(page, "rtl83xx hw l2 qos:%d\n", bridge_qos_enable);
	len += sprintf(page+len, "rtl83xx hw nat qos:%d\n", nat_qos_enable);
	
				
	if (len <= off+count)
		*eof = 1;
			
	*start = page + off;
		len -= off;
			
	if (len>count)
		len = count;
			
	if (len<0) len = 0;
			
	return len;
}
#endif

int rtl_83xxQosConfigWriteProc(struct file *file, const char *buffer,
		      unsigned long count, void *data)
{
	char tmp[32],*strptr,*tokptr;		
	int prio;
	int dot1p_pri; 
	unsigned int g_prioritytRemap[8] = {0,1,2,3,4,5,6,7};

	if (count < 2)
		return -EFAULT;
	
	if (buffer && !copy_from_user(tmp, buffer, count)) {
	
		tmp[count] = '\0';
		strptr=tmp;
		tokptr = strsep(&strptr," ");
		if (tokptr==NULL)
		{
			goto errout;
		}
		
		if (!memcmp(tokptr, "l2", 2))
		{				
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			bridge_qos_enable = simple_strtol(tokptr, NULL, 0);
			if(bridge_qos_enable)
			{
				rtl865x_disableRtl83xxUCToCpu(3);
			}
			else
			{
				rtl865x_enableRtl83xxUCToCpu(3);
			}
		}
		else if(!memcmp(tokptr, "nat",3))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}
			nat_qos_enable = simple_strtol(tokptr, NULL, 0);
			if(nat_qos_enable)
			{
				for(prio = PRI0; prio <= PRI7; prio++)
				{
					rtl8651_setAsicVlanRemark(PHY0, prio, prio);
				}

				for(dot1p_pri=0; dot1p_pri<=7; dot1p_pri++)
				{
					rtl83xx_qosSet1pBasePri(dot1p_pri, g_prioritytRemap[dot1p_pri]);
					rtl83xx_qosSetPriDecision(PRIDEC_1Q, 0x80);
				}
			}
			else
			{
				rtl8651_disableVlanRemark();
				rtl83xx_qosRestorePriDecision(1<<D1P_BASE);
				rtl83xx_qosFlush1pBasePri();
			}
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("error input!\n");
	}

	return count;	
}

#endif

int rtl_dump_tc_status(struct seq_file *s)
{
	if(tc_hw_qos == 0){
		seq_printf(s, "\n %d: TC commands do not work for hw qos mapping.\n", tc_hw_qos);
	}
	else{
		seq_printf(s, "\n %d: TC commands work for hw qos mapping.\n", tc_hw_qos);
	}

	seq_printf(s, "====================\n\n");
	
	return SUCCESS;
}
#ifdef CONFIG_RTL_PROC_NEW
static int rtl_hw_qos_config_read(struct seq_file *s, void *v)
{
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
	rtl_dump_tc_status(s);
	
	rtl_dump_configQosRemark_info(s);
	rtl_dump_AsicRemark_info(s);
	#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	rtl_dump_Asic83xxRemark_info(s);
	#endif
#endif
	return 0;
}
int rtl_hw_qos_config_single_open(struct inode *inode, struct file *file)
{
        return(single_open(file, rtl_hw_qos_config_read,NULL));
}
#else
static int32 rtl_hw_qos_config_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int i, j;
	int len=0;

	for (i=0; i<2; i++)
	{
		if (rtl865x_isWanNetifByIdx(i)==0) 
			len += sprintf(page+len, "%s\n", "br0 Hw Qos Cofig");
		else if (rtl865x_isWanNetifByIdx(i)==1) 
			len += sprintf(page+len, "%s\n", "eth1 Hw Qos Cofig");
		else
			continue;

		#if defined(CONFIG_RTL_HW_QOS_BRIDGE_FWD_SUPPORT)
		len += sprintf(page+len, "\t%s\n", "Port Based Info:");
		for (j=0; j<RTL8651_PORT_NUMBER; j++)
		{
			if (classId2Port[i][j]!=0) {
				len += sprintf(page+len, "\t\tPort[%d] mapping to classId[0x%x]\n", j, classId2Port[i][j]);
			}
		}
		
		len += sprintf(page+len, "\t%s\n", "Vlan Priority Based Info:");
		for (j=0; j<TOTAL_VLAN_PRIORITY_NUM; j++)
		{
			if (classId2VlanPriority[i][j]!=0) {
				len += sprintf(page+len, "\t\tVlan Priority[%d] mapping to classId[0x%x]\n", j, classId2VlanPriority[i][j]);
			}
		}
		
		len += sprintf(page+len, "\t%s\n", "DSCP Based Info:");
		for (j=0; j<TOTAL_DSCP_NUM; j++)
		{
			if (classId2Dscp[i][j]!=0) {
				len += sprintf(page+len, "\t\tDscp[%d] mapping to classId[0x%x]\n", j, classId2Dscp[i][j]);
			}
		}
		#endif
	}

	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count)
		len = count;
	if (len<0)
	  	len = 0;

	return len;
}
#endif

/*cmd example:
echo "add port 1 classId 1000 eth1" > /proc/rtl_hw_qos_config
echo "add vlan 2 classId 2000 br0" > /proc/rtl_hw_qos_config
echo "add dscp 4 classId 3000 eth1" > /proc/rtl_hw_qos_config

echo "del port 1 classId 1000 eth1" > /proc/rtl_hw_qos_config
echo "del vlan 2 classId 2000 br0" > /proc/rtl_hw_qos_config
echo "del dscp 4 classId 3000 eth1" > /proc/rtl_hw_qos_config

echo "flush port" > /proc/rtl_hw_qos_config
echo "flush vlan" > /proc/rtl_hw_qos_config
echo "flush dscp" > /proc/rtl_hw_qos_config
echo "flush all" > /proc/rtl_hw_qos_config

echo "portpri netif eth1 port 1 mark 13 1 2 > /proc/rtl_hw_qos_config"	
echo "dscppri netif eth1 dscp 1 mark 13 > /proc/rtl_hw_qos_config"
echo "vlanpri netif eth1 vlan 1 mark 13 > /proc/rtl_hw_qos_config"
echo "aclpri netif eth1 br0 1 ip src c0a8010a-c0a8010a mark 13" > /proc/rtl_hw_qos_config
echo "aclpri netif eth1 br0 1 ipv6 src 20010000 0 0 2 mark 13" >/proc/rtl_hw_qos_config

echo "aclpri netif eth1 br0 1 mac src 00:16:d3:32:f8:2f mark 15" > /proc/rtl_hw_qos_config
echo "aclpri netif eth1 br0 1 mac dst 00:16:d3:32:f8:2f mark 15" > /proc/rtl_hw_qos_config

echo "aclpri netif eth9 br0 1 protocol udp v4 mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol udp v6 mark 806" > /proc/rtl_hw_qos_config

echo "aclpri netif eth9 br0 1 protocol tcp srcPort 20000-20000 v4 mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol tcp srcPort 20000-20000 v6 mark 806" > /proc/rtl_hw_qos_config

echo "aclpri netif eth9 br0 1 protocol udp sip c0a8010b-c0a8010b mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth1 br0 1 protocol udp sipv6 20010000 0 0 2 mark 13" >/proc/rtl_hw_qos_config 

echo "aclpri netif eth9 br0 1 protocol tcp srcPort 20000-20000 sip c0a8010b-c0a8010b mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol tcp srcPort 20000-20000 sipv6 20010000 0 0 2 mark 806" > /proc/rtl_hw_qos_config

echo "aclpri netif eth1 br0 1 flowLabel 20 mark 13" >/proc/rtl_hw_qos_config 

echo "aclpri netif eth1 br0 1 dscp 1 30 v4 mark 13" >/proc/rtl_hw_qos_config	-> dscp 30
echo "aclpri netif eth1 br0 1 dscp 1 30 v6 mark 13" >/proc/rtl_hw_qos_config	-> dscp 30

echo "aclpri netif eth1 br0 1 dscp 0 30 v4 mark 13" >/proc/rtl_hw_qos_config	-> tos 30
echo "aclpri netif eth1 br0 1 dscp 0 30 v6 mark 13" >/proc/rtl_hw_qos_config	-> traffic class 30

echo "aclpri netif eth1 br0 1 flowLabel 10 dscp 0 40 v6 mark 806" > /proc/rtl_hw_qos_config

echo "aclpri netif eth1 br0 1 srcPhyport 5 mark 13" > /proc/rtl_hw_qos_config

echo "aclpri flush br0" > /proc/rtl_hw_qos_config
echo "aclpri flush eth1" > /proc/rtl_hw_qos_config
echo "restore decision port 0 dscp 0 vlan 0 acl 0" > /proc/rtl_hw_qos_config 

echo "remark netif eth1 mark 13 v4 1 2" > /proc/rtl_hw_qos_config  		// The stream marked 13 set ipv4 dscp remark =1 and 802.1p priority remark =2
echo "remark netif eth1 mark 13 v6 1 2" > /proc/rtl_hw_qos_config  		// The stream marked 13 set ipv6 dscp remark =1 and 802.1p priority remark  =2 when CONFIG_IPV6=y
echo "remark netif eth1 mark 13 both 1 2" > /proc/rtl_hw_qos_config		// The stream marked 13 set dscp remark =1 and 802.1p priority remark =2 when CONFIG_IPV6=y

echo "remark netif eth1 mark 13 dscp v4 1" > /proc/rtl_hw_qos_config  	// The stream marked 13 set ipv4 dscp remark =1
echo "remark netif eth1 mark 13 dscp v6 1" > /proc/rtl_hw_qos_config  	// The stream marked 13 set ipv6 dscp remark =1 when CONFIG_IPV6=y
echo "remark netif eth1 mark 13 dscp both 1" > /proc/rtl_hw_qos_config  // The stream marked 13 set dscp remark =1 when CONFIG_IPV6=y
echo "remark netif eth1 mark 13 vlan 1" > /proc/rtl_hw_qos_config 		//The stream marked 13 set 802.1p remark =1

echo "aclpri netif eth9 br0 1 protocol udp mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol tcp srcPort 20000-20000 mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol tcp dstPort 30000-30000 mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol tcp srcPort 40000-40000 dstPort 50000-50000 mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol udp sip c0a8010b-c0a8010b mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol udp dip c0a8010c-c0a8010c mark 806" > /proc/rtl_hw_qos_config
echo "aclpri netif eth9 br0 1 protocol udp sip c0a8010d-c0a8010d dip c0a8010e-c0a8010e mark 806" > /proc/rtl_hw_qos_config

//only CONFIG_RTL_83XX_QOS_SUPPORT=y
echo "aclpri netif eth1 br0 1 dscp 1 mark 13" >/proc/rtl_hw_qos_config
echo "aclpri netif eth1 br0 1 vlanpri 1 mark 13" > /proc/rtl_hw_qos_config
*/
static int32 rtl_hw_qos_config_write( struct file *filp, const char *buff,unsigned long len, void *data )
{
	
	char tmpbuf[512];
	char *strptr;
	char	*cmdptr;
	int remarkDscp=-1,remark8021p=-1;
	unsigned int markValue=0;
	uint8	name[MAX_IFNAMESIZE];
	uint8 version = 0;

	#if defined(CONFIG_RTL_HW_QOS_BRIDGE_FWD_SUPPORT)
	int i, j;
	int tmp = 0;
	int netifIdx = 0;
	unsigned classId = 0;
	#endif

	if (len>512) 
		goto errout;

	if (buff && !copy_from_user(tmpbuf, buff, len))
	{
		tmpbuf[len-1] = '\0';
		strptr=tmpbuf;
		if (strlen(strptr)==0) 
			goto errout;

		cmdptr = strsep(&strptr," ");
		if (cmdptr==NULL) 
			goto errout;

		if(strncmp(cmdptr, "remark",6) == 0){
			//rtlglue_printf("remark config [%s]:[%d].\n",__FUNCTION__,__LINE__);
			
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL) 
				goto errout;
			if (strncmp(cmdptr, "netif",5) == 0) 
			{
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;
				
				sscanf(cmdptr, "%s", name);
				
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;
				if (strncmp(cmdptr, "mark",4) == 0) 
				{
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;
					markValue=simple_strtol(cmdptr, NULL, 0);
					
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, "dscp",4) == 0){
						cmdptr = strsep(&strptr," ");
						if (cmdptr==NULL) 
							goto errout;

						if(strncmp(cmdptr, "v4",2) == 0)				// ipv4 dscp remark only 
							version = IPV4_VERSION;
					#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
						else if(strncmp(cmdptr, "v6",2) == 0)			// ipv6 dscp remark only 
							version = IPV6_VERSION;
						else if(strncmp(cmdptr, "both",4) == 0)			// ipv4+ipv6 dscp remark
							version = IPV4_IPV6_VERSION;
					#endif

						cmdptr = strsep(&strptr," ");
						if (cmdptr==NULL) 
							goto errout;
						remarkDscp=simple_strtol(cmdptr, NULL, 0);
					}
					else if(strncmp(cmdptr, "vlan",4) == 0){
						cmdptr = strsep(&strptr," ");
						if (cmdptr==NULL) 
							goto errout;

						remark8021p=simple_strtol(cmdptr, NULL, 0);
					}
					else{
						if(strncmp(cmdptr, "v4",2) == 0)				// ipv4 dscp remark only 
							version = IPV4_VERSION;
					#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
						else if(strncmp(cmdptr, "v6",2) == 0)			// ipv6 dscp remark only 
							version = IPV6_VERSION;
						else if(strncmp(cmdptr, "both",4) == 0)			// both ipv4 & ipv6 dscp remark
							version = IPV4_IPV6_VERSION;
					#endif				
					
						cmdptr = strsep(&strptr," ");
						if (cmdptr==NULL) 
							goto errout;
						remarkDscp=simple_strtol(cmdptr, NULL, 0);
					
						cmdptr = strsep(&strptr," ");
						if (cmdptr==NULL) 
							goto errout;
						remark8021p=simple_strtol(cmdptr, NULL, 0);
					}
					
					//sscanf(cmdptr, "%d %d", &remarkDscp, &remark8021p);
					//panic_printk("netif:%s,mark:%d,remarkdscp:%d,remark8021p:%d,[%s]:[%d].\n",
					//name,markValue,remarkDscp,remark8021p,__FUNCTION__,__LINE__);
					if((remarkDscp!= -1)||(remark8021p!=-1))
					{
						rtl865x_setRemarkByMark(name,version,markValue,remarkDscp,remark8021p);
					}
				}
				else 
					goto errout;	
			}
			else
			{
				goto errout;
			}
		}
		else if (strncmp(cmdptr, "restore",7) == 0){
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL) 
				goto errout;
			
			if ((strncmp(cmdptr, "eth", 3) == 0) || (strncmp(cmdptr, "br", 2) == 0)){
				char dev[16] = {0};
				int flag = 0;

				sscanf(cmdptr, "%s", dev);
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "dscp", 4) == 0) {
					flag = 1;
				}
				else if (strncmp(cmdptr, "vlan", 4) == 0){
					flag = 2;
				}
#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
				else if(strncmp(cmdptr, "v6Dscp", 6) == 0) {
					flag = 3;
				}
#endif
				else
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "remark", 6) == 0){
					if(flag == 1){
						rtl865x_restoreDscpRemark(dev, 1);
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
						rtl83xx_flush_dscp_remark_setting();
#endif
					}
					else if(flag == 2)
						rtl865x_restoreVlanRemark(dev);
#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
					else if(flag == 3)
						rtl865x_restoreV6DscpRemark(dev);
#endif
				}
				else
					goto errout;
				
			}
			else
				goto errout;
		}
		else if (strncmp(cmdptr, "disable",7) == 0){

			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL) 
				goto errout;
			
			if (strncmp(cmdptr, "vlan",4) == 0){
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "remark",6) == 0)
				{
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
					rtl83xx_disable_1p_remark_setting();
#else
					rtl8651_disableVlanRemark();
#endif
				}
				else
					goto errout;
			}
			else if(strncmp(cmdptr, "dscp",4) == 0){
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "remark",6) == 0) {
					rtl8651_disableDscpRemark();
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
					rtl83xx_disable_dscp_remark_setting();
#endif
				}
				else
					goto errout;
			}
#if defined(CONFIG_IPV6) && (defined(CONFIG_RTL_8198C) || defined(CONFIG_RTL_8197F))
			else if(strncmp(cmdptr, "v6Dscp",6) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "remark",6) == 0) 
					rtl8651_disableV6DscpRemark();
				else
					goto errout;
			}
#endif
			else
				goto errout;
		}
		#if defined(CONFIG_RTL_HW_QOS_BRIDGE_FWD_SUPPORT)
		/*echo "add port 1 classId 1000 eth1" > /proc/rtl_hw_qos_config*/
		else if (strncmp(cmdptr, "add",3) == 0) {
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL) 
				goto errout;

			if (strncmp(cmdptr, "port",4) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				sscanf(cmdptr, "%d", &tmp);
				if (tmp<0 ||tmp>=RTL8651_PORT_NUMBER) 
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "classId",7) == 0) {
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					sscanf(cmdptr, "%u", &classId);

					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, RTL_DRV_LAN_NETIF_NAME,strlen(RTL_DRV_LAN_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_LAN_NETIF_NAME);
					else if (strncmp(cmdptr, RTL_DRV_WAN0_NETIF_NAME, strlen(RTL_DRV_WAN0_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_WAN0_NETIF_NAME);
					else 
						goto errout;

					if (netifIdx < 0)
						goto errout;
					classId2Port[netifIdx][tmp] = classId;
					rtl865x_setPortPriorityByClassId(tmp, netifIdx, classId);
				}	
			} else if (strncmp(cmdptr, "vlan",4) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				sscanf(cmdptr, "%d", &tmp);
				if (tmp<0 ||tmp>=TOTAL_VLAN_PRIORITY_NUM) 
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "classId",7) == 0) {
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					sscanf(cmdptr, "%u", &classId);
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, RTL_DRV_LAN_NETIF_NAME,strlen(RTL_DRV_LAN_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_LAN_NETIF_NAME);
					else if (strncmp(cmdptr, RTL_DRV_WAN0_NETIF_NAME, strlen(RTL_DRV_WAN0_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_WAN0_NETIF_NAME);
					else 
						goto errout;

					if (netifIdx < 0)
						goto errout;
					
					classId2VlanPriority[netifIdx][tmp] = classId;
					rtl865x_setVlanPriorityByClassId(tmp, netifIdx, classId);
				}	
			} else if (strncmp(cmdptr, "dscp",4) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				sscanf(cmdptr, "%d", &tmp);
				if (tmp<0 ||tmp>=TOTAL_DSCP_NUM) 
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "classId",7) == 0) {
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					sscanf(cmdptr, "%u", &classId);
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, RTL_DRV_LAN_NETIF_NAME,strlen(RTL_DRV_LAN_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_LAN_NETIF_NAME);
					else if (strncmp(cmdptr, RTL_DRV_WAN0_NETIF_NAME, strlen(RTL_DRV_WAN0_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_WAN0_NETIF_NAME);
					else 
						goto errout;

					if (netifIdx < 0)
						goto errout;
					
					classId2Dscp[netifIdx][tmp] = classId;
					rtl865x_setDscpPriorityByClassId(tmp, netifIdx, classId);
				}	
			} else
				goto errout;
		} else if (strncmp(cmdptr, "del",3) == 0) {
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL) 
				goto errout;

			if (strncmp(cmdptr, "port",4) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				sscanf(cmdptr, "%d", &tmp);
				if (tmp<0 ||tmp>=RTL8651_PORT_NUMBER) 
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "classId",7) == 0) {
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					sscanf(cmdptr, "%u", &classId);

					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, RTL_DRV_LAN_NETIF_NAME,strlen(RTL_DRV_LAN_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_LAN_NETIF_NAME);
					else if (strncmp(cmdptr, RTL_DRV_WAN0_NETIF_NAME, strlen(RTL_DRV_WAN0_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_WAN0_NETIF_NAME);
					else 
						goto errout;

					if (netifIdx < 0)
						goto errout;

					if (classId2Port[netifIdx][tmp]==classId) {
						classId2Port[netifIdx][tmp] = 0;
						rtl865x_resetPortPriority(tmp);
					}
				}	
			} else if (strncmp(cmdptr, "vlan",4) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				sscanf(cmdptr, "%d", &tmp);
				if (tmp<0 ||tmp>=TOTAL_VLAN_PRIORITY_NUM) 
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "classId",7) == 0) {
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					sscanf(cmdptr, "%u", &classId);
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, RTL_DRV_LAN_NETIF_NAME,strlen(RTL_DRV_LAN_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_LAN_NETIF_NAME);
					else if (strncmp(cmdptr, RTL_DRV_WAN0_NETIF_NAME, strlen(RTL_DRV_WAN0_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_WAN0_NETIF_NAME);
					else 
						goto errout;

					if (netifIdx < 0)
						goto errout;
					
					if (classId2VlanPriority[netifIdx][tmp] == classId) {
						classId2VlanPriority[netifIdx][tmp] = 0;
						rtl865x_resetVlanPriority(tmp);
					}
				}	
			} else if (strncmp(cmdptr, "dscp",4) == 0) {
				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				sscanf(cmdptr, "%d", &tmp);
				if (tmp<0 ||tmp>=TOTAL_DSCP_NUM) 
					goto errout;

				cmdptr = strsep(&strptr," ");
				if (cmdptr==NULL) 
					goto errout;

				if (strncmp(cmdptr, "classId",7) == 0) {
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					sscanf(cmdptr, "%u", &classId);
					cmdptr = strsep(&strptr," ");
					if (cmdptr==NULL) 
						goto errout;

					if (strncmp(cmdptr, RTL_DRV_LAN_NETIF_NAME,strlen(RTL_DRV_LAN_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_LAN_NETIF_NAME);
					else if (strncmp(cmdptr, RTL_DRV_WAN0_NETIF_NAME, strlen(RTL_DRV_WAN0_NETIF_NAME))==0)
						netifIdx = _rtl865x_getNetifIdxByNameExt(RTL_DRV_WAN0_NETIF_NAME);
					else 
						goto errout;

					if (netifIdx < 0)
						goto errout;
					
					if (classId2Dscp[netifIdx][tmp] == classId) {
						classId2Dscp[netifIdx][tmp] = 0;
						rtl865x_resetDscpPriority(tmp);
					}
				}	
			} else
				goto errout;
		}
		else if (strncmp(cmdptr, "flush",5) == 0) {
			cmdptr = strsep(&strptr," ");
			if (cmdptr==NULL) 
				goto errout;

			if (strncmp(cmdptr, "port",4) == 0) {
				for (i=0; i<NETIF_NUMBER; i++) {
					for (j=0; j<RTL8651_PORT_NUMBER; j++) {
						classId2Port[i][j] = 0;
					}
				}
				
				rtl865x_flushPortPriority();
			} else if (strncmp(cmdptr, "vlan",4) == 0) {
				for (i=0; i<NETIF_NUMBER; i++) {
					for (j=0; j<TOTAL_VLAN_PRIORITY_NUM; j++) {
						classId2VlanPriority[i][j] = 0;
					}
				}
				
				rtl865x_flushVlanPriority();
			} else if (strncmp(cmdptr, "dscp",4) == 0) {
				for (i=0; i<NETIF_NUMBER; i++) {
					for (j=0; j<TOTAL_DSCP_NUM; j++) {
						classId2Dscp[i][j] = 0;
					}
				}
				
				rtl865x_flushDscpPriority();
			} else if (strncmp(cmdptr, "all",3) == 0) {
				for (i=0; i<NETIF_NUMBER; i++) {
					for (j=0; j<RTL8651_PORT_NUMBER; j++) 
						classId2Port[i][j] = 0;
					for (j=0; j<TOTAL_VLAN_PRIORITY_NUM; j++) 
						classId2VlanPriority[i][j] = 0;
					for (j=0; j<TOTAL_DSCP_NUM; j++) 
						classId2Dscp[i][j] = 0;
				}
				rtl865x_flushPortPriority();
				rtl865x_flushVlanPriority();
				rtl865x_flushDscpPriority();
			} else
				goto errout;
		}
		#endif
		else {
errout:
			printk("error input\n");
			return len;
		}
	}
	
	return len;
}

#if defined(CONFIG_RTL_PROC_NEW)
static ssize_t rtl_hw_qos_config_single_write(struct file * file, const char __user * userbuf,
		     size_t count, loff_t * off)
{
	    return rtl_hw_qos_config_write(file, userbuf,count, off);
}
#endif
#endif

#if defined (CONFIG_RTL_83XX_QOS_TEST)
void rtl865x_qos_set(void)
{
	int prio;
	//	set port priority highest
	rtl8651_setAsicPriorityDecision(0xF, 0, 0, 0, 0, 0);
	/* set port priority*/
	rtl8651_setAsicPortBasedPriority(0, 0);
	rtl8651_setAsicPortBasedPriority(1, 0);
	rtl8651_setAsicPortBasedPriority(2, 7);
	rtl8651_setAsicPortBasedPriority(3, 7);
	/*802.1p remark*/
	for(prio = 0; prio <= 0; prio++)
		rtl8651_setAsicVlanRemark(0, prio, prio);
}
#endif

