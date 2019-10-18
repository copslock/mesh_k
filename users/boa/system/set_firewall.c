/*
 *      Utiltiy function for setting firewall filter
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <arpa/inet.h>

#include <sys/stat.h>


#include "apmib.h"
#include <time.h> 
#include "sysconf.h"
#include "sys_utility.h"
#ifdef CONFIG_RTK_VOIP
#include "voip_manager.h"
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>

#include "./../src/utility.h"

#define MAX_L2_LIST_NUM    256

int g_parentControlFlag=0;       //parent control action(add,delete,ignore)
int g_parentControlListTableNumberFlag=0;  //meas which table list need be process
//int g_parentControlDeleteListFlag=0;  //meas which table list need be delete
#define  SUNDAY 	 0
#define  MONDAY 	 1
#define  TUESDAY	 2
#define  WEDNESDAY	 3
#define  THURSDAY	 4
#define  FRIDAY 	 5
#define  SATDAY 	 6
#define WEEK_NUM        7
#define WEEK_TIME_DISABLED 7
extern int getWlStaInfo( char *interface,  WLAN_STA_INFO_Tp pInfo );
#define DEFAULT_PARENT_CONTROL_IGNORE_ACT 0
#define SET_PARENT_CONTROL_LIST   	 	  1
#define DELETE_PARENT_CONTROL_LIST 		  2

//#define CONFIG_REFINE_BR_FW_RULE 1	//for smartbit performance
int setFirewallIptablesRules(int argc, char** argv);
char mark[]="mark";
char _mark[]="--mark";
char Iptables[]="iptables";
#if defined(CONFIG_APP_EBTABLES)&&defined(CONFIG_EBTABLES_KERNEL_SUPPORT)
char Ebtables[]="ebtables";
#endif
char Ip6tables[]="ip6tables";
char INPUT[]= "INPUT";
char OUTPUT[]= "OUTPUT";
char FORWARD[]= "FORWARD";
char PREROUTING[]="PREROUTING";
char POSTROUTING[]= "POSTROUTING";
char ACCEPT[]= "ACCEPT";
char DROP[]= "DROP";
char RET[]= "RETURN";
char LOG[]= "LOG";
char MASQUERADE[]="MASQUERADE";
char REDIRECT[]="REDIRECT";
char MARK[]="MARK";
char DSCP[]="DSCP";
#if defined (QOS_OF_TR069)
char VLANPRIORITY[]="VLANPRIORITY";
#endif
// iptables operations, manupilations, matches, options etc.
char ADD[]= "-A";
char DEL[]= "-D";
char FLUSH[]= "-F";
char INSERT[]="-I";
char NEW[]= "-N";
char POLICY[]= "-P";
char X[]= "-X";
char Z[]= "-Z";
char _dest[]= "-d";
char in[]= "-i";
char jump[]= "-j";
char match[]= "-m";
char out[]= "-o";
char _protocol[]= "-p";
char _src[]= "-s";
char _table[]= "-t";
char nat_table[]= "nat";
char mangle_table[]= "mangle";
char NOT[]= "!";
char _mac[]= "mac";
char mac_src[]= "--mac-source";
char mac_dst[]= "--mac-destination";
char dport[]= "--dport";
char sport[]= "--sport";
char layer7[]="layer7 --l7proto";
char syn[]= "--syn";
char ALL[]= "ALL";
char DNAT[]= "DNAT";
char icmp_type[]="--icmp-type";
char echo_request[]="echo-request";
char echo_reply[]="echo-reply";
char mstate[]="state";
char state[]="--state";
char _udp[]="udp";
char _tcp[]="tcp";
char _icmp[]="icmp";
char RELATED_ESTABLISHED[]= "RELATED,ESTABLISHED";
char INVALID[]="INVALID";
char tcp_flags[]="--tcp-flags";
char MSS_FLAG1[]="SYN,RST";
char MSS_FLAG2[]="SYN";
char clamp[]="--clamp-mss-to-pmtu";
char TCPMSS[]="TCPMSS";
char ip_range[]="iprange";
char src_rnage[]="--src-range";
char dst_rnage[]="--dst-range";
char set_mark[]="--set-mark";
char set_dscp[]="--set-dscp";
#if defined (QOS_CLASSIFY_BY_DSCP) ||defined (ADVANCED_IPQOS)
char match_dscp[]="dscp";
char _dscp[]="--dscp";
#endif
#if defined (QOS_OF_TR069)
char set_vlanpri[]="--set-vlanpri";
char _IEEE_802_3[] = "IEEE_802_3";
char _etype[] = "--etype-value";
char _sap[] = "--sap-value";
char _ctrl[] = "--ctrl-value";
char _oui[] = "--oui-value";
#endif

char match_phyport[]="phyport";
char _sphyport[]="--port-source";
char _dphyport[] = "--port-destination";
char match_vlanpriority[]="vlanpriority"; 
char prio_value[]="--prio-value";


char mark_set[]="--mark-set"; 
static const char _tc[] = "tc";
static const char _qdisc[] = "qdisc";
static const char _add[] = "add";
static const char _dev[] = "dev";
static const char _root[] = "root";
static const char _handle[] = "handle";
static const char _htb[] = "htb";
static const char _default[] = "default";
static const char _classid[] = "classid";
static const char _rate[] = "rate";
static const char _ceil[] = "ceil";
static const char _burst[] = "burst";
static const char _sfq[] = "sfq";
static const char _red[] = "red";
static const char _min[] = "min";
static const char _max[] = "max";
static const char _probability[] ="probability" ;
static const char _perturb[] = "perturb";
static const char _class[] = "class";
static const char _filter[] = "filter";
static const char _protocol2[] = "protocol";
static const char _ip[] = "ip";
static const char _prio[] = "prio";
static const char _fw[] = "fw";
static const char _parent[] = "parent";
static const char _quantum[] = "quantum";
static const char _r2q[] = "r2q";

#ifdef MULTI_PPPOE
//#define MULTI_PPP_DEBUG

struct PPP_info
{ 
	char client_ip[20];
	char server_ip[20];
	char ppp_name[5];
	int order;
};
/*
struct subNet
{	
	int SubnetCount;
	unsigned char startip[3][20];
	unsigned char endip[3][20];
};*/


char SubNet[4][30];
char flushCmds[12][80];
int CmdCount = 0 ;


//struct subNet SubNets[4];
//br0_info
int pppNumbers = 0;
int info_setting = 0;
struct PPP_info infos[5];
char Br0NetSectAddr[30];
//lan partition info
char  lan_ip[4][40] ;

int get_info()
{
	int subCount;
	unsigned char buffer[30];
	int connectNumber,index = -1;
	FILE *local,*remote,*order,*number,*br0,*pF,*pdev;
	if((local=fopen("/etc/ppp/ppp_local","r+"))==NULL)
	{
		printf("Cannot open this file\n");
		return 0;
	}
	if((remote=fopen("/etc/ppp/ppp_remote","r+"))==NULL)
	{
		printf("Cannot open this file\n");		
		return 0;
	}
	
	if((order=fopen("/etc/ppp/ppp_order_info","r+"))==NULL)
	{
		printf("Cannot open this file\n");
		return 0;
	}
	
	if((number=fopen("/etc/ppp/lineNumber","r+"))==NULL)
	{
		printf("Cannot open this file\n");
		return 0;
	}
	if((br0=fopen("/etc/ppp/br0_info","r+"))==NULL)
	{
		printf("Cannot open this file\n");
		return 0;
	}	
	if((pdev=fopen("/etc/ppp/ppp_device","r+"))==NULL)
	{
		printf("Cannot open this file\n");
		return 0;
	}		

	close(order);
	fscanf(br0,"%s",Br0NetSectAddr);
	fscanf(number,"%d",&pppNumbers);
	
	for( index = 0 ; index < pppNumbers ; ++index)
	{		
		int num,i,j;
		char name[5];
		char devname[5];
		
		fscanf(local,"%s",infos[index].client_ip);
		fscanf(remote,"%s",infos[index].server_ip);						
		fscanf(pdev,"%s",devname);
		if((order=fopen("/etc/ppp/ppp_order_info","r+"))==NULL)
			return ;
		while(fscanf(order,"%d--%s",&num,name) > 0 )
		{			
#ifdef MULTI_PPP_DEBUG		
			printf("devname value is:%s\n",devname);
			printf("name value is:%s\n",name);	
			printf("num value is:%d\n",num);				
#endif			
			if(!strcmp(devname,name))
			{
				infos[index].order = num;
				strcpy(infos[index].ppp_name,devname);
#ifdef MULTI_PPP_DEBUG					
				printf("infos[index].order value is:%d\n",infos[index].order);
				printf("infos[index].ppp_name value is:%s\n",infos[index].ppp_name);
#endif				
				break;
			}				
		}
		fclose(order);
	}
	fclose(local);
	fclose(remote);	
	fclose(number);	
	fclose(br0);
	fclose(pdev);
	//get the subnet info
	if((pF = fopen("/etc/ppp/ppp_connect_number","r"))==NULL)
	{
		printf("can't open the file\n");
		return 0;
	}	
	fscanf(pF,"%d",&connectNumber);		//max value is 4
	fclose(pF);	

	//apmib_get( MIB_SUBNET1_F1_START,  (void *)buffer);
	//printf("test-------------%s\n",inet_ntoa(*((struct in_addr *)buffer)));
	if(connectNumber >= 1)
	{
		apmib_get(MIB_PPP_SUBNET1,(void *)buffer);
		strcpy(SubNet[0],buffer);
		
/*	
		apmib_get(MIB_SUBNET1_COUNT, (void *)&subCount);
		SubNets[0].SubnetCount = subCount;
		if(subCount >= 1)
		{
			apmib_get(MIB_SUBNET1_F1_START,(void *)buffer);
			strcpy(SubNets[0].startip[0],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET1_F1_END, (void *)buffer);
			strcpy(SubNets[0].endip[0],inet_ntoa(*((struct in_addr *)buffer)));
		}
		if(subCount >= 2)
		{
			apmib_get(MIB_SUBNET1_F2_START,(void *)buffer);
			strcpy(SubNets[0].startip[1],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET1_F2_END, (void *)buffer);
			strcpy(SubNets[0].endip[1],inet_ntoa(*((struct in_addr *)buffer)));
		}
		if(subCount >= 3)
		{
			apmib_get(MIB_SUBNET1_F3_START,(void *)buffer);
			strcpy(SubNets[0].startip[2],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET1_F3_END, (void *)buffer);
			strcpy(SubNets[0].endip[2],inet_ntoa(*((struct in_addr *)buffer)));
		}		
*/
	}
	if(connectNumber >= 2)
	{
		apmib_get(MIB_PPP_SUBNET2,(void *)buffer);
		strcpy(SubNet[1],buffer);
	
/*		
		apmib_get(MIB_SUBNET2_COUNT, (void *)&subCount);
		SubNets[1].SubnetCount = subCount;
		if(subCount >= 1)
		{
			apmib_get(MIB_SUBNET2_F1_START,(void *)buffer);
			strcpy(SubNets[1].startip[0],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET2_F1_END, (void *)buffer);
			strcpy(SubNets[1].endip[0],inet_ntoa(*((struct in_addr *)buffer)));
		}
		if(subCount >= 2)
		{
			apmib_get(MIB_SUBNET2_F2_START,(void *)buffer);
			strcpy(SubNets[1].startip[1],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET2_F2_END, (void *)buffer);
			strcpy(SubNets[1].endip[1],inet_ntoa(*((struct in_addr *)buffer)));	
		}
		if(subCount >= 3)
		{
			apmib_get(MIB_SUBNET2_F3_START,(void *)buffer);
			strcpy(SubNets[1].startip[2],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET2_F3_END, (void *)buffer);
			strcpy(SubNets[1].endip[2],inet_ntoa(*((struct in_addr *)buffer)));	
		}
*/		
	}
	if(connectNumber >= 3)
	{
		apmib_get(MIB_PPP_SUBNET3,(void *)buffer);	
		strcpy(SubNet[2],buffer);		
/*		
		apmib_get(MIB_SUBNET3_COUNT, (void *)&subCount);
		SubNets[2].SubnetCount = subCount;
		if(subCount >= 1)
		{
			apmib_get(MIB_SUBNET3_F1_START,(void *)buffer);
			strcpy(SubNets[2].startip[0],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET3_F1_END, (void *)buffer);
			strcpy(SubNets[2].endip[0],inet_ntoa(*((struct in_addr *)buffer)));	
		}
		if(subCount >= 2)
		{
			apmib_get(MIB_SUBNET3_F2_START,(void *)buffer);
			strcpy(SubNets[2].startip[1],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET3_F2_END, (void *)buffer);
			strcpy(SubNets[2].endip[1],inet_ntoa(*((struct in_addr *)buffer)));	
		}
		if(subCount >= 3)
		{
			apmib_get(MIB_SUBNET3_F3_START,(void *)buffer);
			strcpy(SubNets[2].startip[2],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET3_F3_END, (void *)buffer);
			strcpy(SubNets[2].endip[2],inet_ntoa(*((struct in_addr *)buffer)));	
		}
*/		

	}
	if(connectNumber >= 4)
	{
		apmib_get(MIB_PPP_SUBNET4,(void *)buffer);	
		strcpy(SubNet[3],buffer);		
		
/*		
		apmib_get(MIB_SUBNET4_COUNT, (void *)&subCount);
		SubNets[3].SubnetCount = subCount;
		if(subCount >= 1)
		{
			apmib_get(MIB_SUBNET4_F1_START,(void *)buffer);
			strcpy(SubNets[3].startip[0],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET4_F1_END, (void *)buffer);
			strcpy(SubNets[3].endip[0],inet_ntoa(*((struct in_addr *)buffer)));	
		}
		if(subCount >= 2)
		{
			apmib_get(MIB_SUBNET4_F2_START,(void *)buffer);
			strcpy(SubNets[3].startip[1],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET4_F2_END, (void *)buffer);
			strcpy(SubNets[3].endip[1],inet_ntoa(*((struct in_addr *)buffer)));	
		}
		if(subCount >= 3)
		{
			apmib_get(MIB_SUBNET4_F3_START,(void *)buffer);
			strcpy(SubNets[3].startip[2],inet_ntoa(*((struct in_addr *)buffer)));
			apmib_get(MIB_SUBNET4_F3_END, (void *)buffer);
			strcpy(SubNets[3].endip[2],inet_ntoa(*((struct in_addr *)buffer)));
		}	
*/		
	}
	return 1;
}
void 	print_info()
{
	int index;
	int sub_index;
	int sub_number;
	for(index = 0 ; index < 4 ; ++index)
	{
		/*
		sub_number = SubNets[index].SubnetCount;

		for(sub_index = 0 ;sub_index< sub_number;++sub_index)
		{	
			printf("the %d subnet  is:%s\n",sub_index+1);
			printf("the value of startip is:%s\n",SubNets[index].startip[sub_index]);
			printf("the value of endip is:%s\n",SubNets[index].endip[sub_index]);				
		}
		printf("--------------------------------------------------------\n");
		*/
	}
}
#endif
extern int apmib_initialized;
//extern int getInAddr( char *interface, int type, void *pAddr );
extern int isFileExist(char *file_name);

#ifdef CONFIG_APP_TR069
extern char acsURLStr[];
#endif //#ifdef CONFIG_APP_TR069

#ifdef CONFIG_RTK_VOIP
int set_voip_parameter(char* pInterface){

	#ifdef CONFIG_RTL_HW_NAPT
	unsigned long	dos_enabled = 0;
	int intVal=0;
	int intVal_num=0;
	#endif
#ifdef SLIC_CH_NUM  // old design 
	const int total_voip_ports = SLIC_CH_NUM + DECT_CH_NUM + DAA_CH_NUM;
#else
	const int total_voip_ports = g_VoIP_Ports;
#endif
	char rtp_port[20]={0};
	char sip_port[10]={0};
	int index, len, i;
	#ifdef CONFIG_RTL_HARDWARE_NAT
	int ivalue = 0;	
	#endif
	voipCfgParam_t  voipCfgParam;
	char voip_service_ports[1024];

	//printf("int set_voip_parameter....\n");
	apmib_get(MIB_VOIP_CFG, (void*)&voipCfgParam);

	len = 0;
	for(index = 0; index < total_voip_ports; index++){
		//iptables -A INPUT -i eth1 -p udp --dport 5060 -j ACCEPT
		sprintf(sip_port,"%d", voipCfgParam.ports[index].sip_port);
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, pInterface, _protocol, _udp, dport,sip_port ,jump,ACCEPT, NULL_STR);
		len += sprintf(&voip_service_ports[len], "%d ", voipCfgParam.ports[index].sip_port);
		
		// iptables -I PREROUTING -t nat -i eth1 -p udp --dport 5060 -j ACCEPT
		RunSystemCmd(NULL_FILE, Iptables, INSERT, PREROUTING, _table, nat_table , in, pInterface, _protocol, _udp, dport,sip_port ,jump,ACCEPT, NULL_STR);

		sprintf(rtp_port,"%d:%d",voipCfgParam.ports[index].media_port,voipCfgParam.ports[index].media_port+3);
		//iptables -I PREROUTING -t nat -i eth1 -p udp --dport 9000:9003 -j ACCEPT
		RunSystemCmd(NULL_FILE, Iptables, INSERT, PREROUTING, _table, nat_table , in, pInterface, _protocol, _udp, dport, rtp_port ,jump,ACCEPT, NULL_STR);

		for (i=0; i<4; i++)
			len += sprintf(&voip_service_ports[len], "%d ", voipCfgParam.ports[index].media_port + i);
	}

	RunSystemCmd("/proc/ext_service_ports", "echo", voip_service_ports, NULL_STR);
	
	#if 0
	def CONFIG_RTL_HW_NAPT
	apmib_get(MIB_URLFILTER_ENABLED,  (void *)&intVal);
	apmib_get(MIB_URLFILTER_TBL_NUM,  (void *)&intVal_num);
	apmib_get(MIB_DOS_ENABLED, (void *)&dos_enabled);
		apmib_get(MIB_SUBNET_MASK,(void*)&ivalue);
		
	//when dos or urlfilter is enable, hwnat must be turn off!
	if((intVal !=0 && intVal_num>0)||(dos_enabled > 0)||(!voipCfgParam.hwnat_enable))
			RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
	else if(voipCfgParam.hwnat_enable)
		{
			if((ivalue&HW_NAT_LIMIT_NETMASK)!=HW_NAT_LIMIT_NETMASK)
				RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
			else
				RunSystemCmd("/proc/hw_nat", "echo", "1", NULL_STR);				
		}
	#endif
}
#endif

//#ifdef ADVANCED_IPQOS
#if defined(ADVANCED_IPQOS)
struct rtl_qos_config
{
	char *netif;
	int mark;
	#if 0	
	int bridgemode;
	int port;
	int vlanpri;
	int dscpValue;
	int dscp_remark;
	int vlanpri_remark;
	int ipversion;
	#endif
	int index;
};
#define RTL_MAX_HW_NETIF_NUM		2

#define RTL_QOS_SP		0x0
#define RTL_QOS_WFQ	0x1

int set_QoS(int operation, int wan_type, int wisp_wan_id)
{
#ifdef   HOME_GATEWAY
	char *br_interface="br0";
	char tmp_args[32]={0}, tmp_args1[32]={0}, tmp_args2[32]={0};
	char tmp_args3[64]={0}, tmp_args4[32]={0};
	unsigned char str_src_ip[64] = {0},str_src_ip1[32] = {0}, str_src_ip2[32] = {0};
	unsigned char 	str_dest_ip[64] = {0},str_dest_ip1[32] = {0}, str_dest_ip2[32] = {0};
	unsigned char str_protocol[32],str_dest_port[32], str_dest_port1[32],str_dest_port2[32];
	unsigned char str_src_port[32],str_src_port1[32],str_src_port2[32];
	unsigned char str_src_mac[128], str_dest_mac[128];
	unsigned char str_dscp[32],str_vlanpir[32]={0},str_phyPort[32];

	unsigned char str_iptables[32]={0};
	unsigned char str_rate[32]={0},str_ceil[32]={0};
	unsigned char command[512]={0};
	struct in_addr *ipaddr,*ipaddr2;

	#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
	unsigned char str_prio[32];
	unsigned char str_weight[32];
	int policy;
	int weight;
	unsigned char  priority=7;
	#endif
	
	char tmp_match[512]={0};
	char *tmpStr=NULL;
	int wan_pkt_mark=13, lan_pkt_mark=53;
	char iface[20], *pInterface="eth1", *pInterface2=NULL,*wanInfterface=NULL;
	int i, QoS_Enabled=0;
	int QoS_Auto_Uplink=0, QoS_Manual_Uplink=0;
	int QoS_Auto_Downlink=0, QoS_Manual_Downlink=0;
	int QoS_Rule_EntryNum=0;
	char PROC_QOS[128]={0};
	int uplink_speed=102400, downlink_speed=102400;
	IPQOS_T entry;
	int get_wanip=0;
	struct in_addr wanaddr;
	unsigned char str_l7_filter[128]={0};
	struct rtl_qos_config qosConfigInfo[RTL_MAX_HW_NETIF_NUM][MAX_QOS_RULE_NUM];
	int j=0,k=0;
	int needSetOnce = 1;
	int vlanSupport, staticIpSupport;

#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
		uplink_speed=1024000;
		downlink_speed=1024000;
#endif

	RunSystemCmd(NULL_FILE, Iptables, FLUSH, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, X, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, Z, _table, mangle_table, NULL_STR);
#ifdef CONFIG_IPV6
	RunSystemCmd(NULL_FILE, Ip6tables, FLUSH, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Ip6tables, X, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Ip6tables, Z, _table, mangle_table, NULL_STR);
#endif
	if(operation == WISP_MODE){
		snprintf(iface, sizeof(iface), "wlan%d", wisp_wan_id);
#if defined(CONFIG_SMART_REPEATER)			
		getWispRptIfaceName(iface,wisp_wan_id);
		//strcat(iface, "-vxd");
#endif		
		pInterface = iface;
		wanInfterface = iface;
		if (wan_type == PPPOE || wan_type == PPTP /*|| wan_type == L2TP */)
		{
			wanInfterface="ppp0";
		}	

	}else{
		if(operation == GATEWAY_MODE){
			if (wan_type == PPPOE || wan_type == PPTP || wan_type == USB3G /*|| wan_type == L2TP*/)
				wanInfterface="ppp0";
			else
				wanInfterface=pInterface;
		}
	}

	if(wan_type == L2TP)//wantype is l2tp
		pInterface2="ppp0";

	get_wanip = getInAddr(wanInfterface, IP_ADDR_T, (void *)&wanaddr);
	if( get_wanip ==0){   //get wan ip fail
		printf("No wan ip currently!\n");
		return 0;
	}

	apmib_get( MIB_QOS_ENABLED, (void *)&QoS_Enabled);
	apmib_get( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&QoS_Auto_Uplink);
	apmib_get( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&QoS_Manual_Uplink);
	apmib_get( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&QoS_Manual_Downlink);
	apmib_get( MIB_QOS_AUTO_DOWNLINK_SPEED, (void *)&QoS_Auto_Downlink);
	apmib_get( MIB_QOS_RULE_TBL_NUM, (void *)&QoS_Rule_EntryNum);
	
#if defined(CONFIG_8021Q_VLAN_SUPPORTED)
	if(apmib_get(MIB_VLAN_ENABLED, (void *)&vlanSupport)){
	}
	else{
		printf("Get MIB_VLAN_ENABLED error!\n");
		return -1;
	}
#else
	vlanSupport = 0;
#endif
	if(apmib_get( MIB_WAN_DHCP, (void *)&staticIpSupport)){
		if(staticIpSupport == 0)
			staticIpSupport = 1;
		else
			staticIpSupport = 0;
	}
	else{
		printf("Get MIB_WAN_DHCP error!\n");
		return -1;
	}

	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", br_interface, "root", NULL_STR);

	//To avoid rule left when wan changed
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", pInterface, "root", NULL_STR);
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", "ppp0", "root", NULL_STR);

	if((strcmp(pInterface, "eth1")!=0)&&(strcmp(pInterface, "ppp0")!=0))
		RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", pInterface, "root", NULL_STR);

	memset(&qosConfigInfo[0][0],0,(sizeof(struct rtl_qos_config)*RTL_MAX_HW_NETIF_NUM*MAX_QOS_RULE_NUM));
	
	snprintf(PROC_QOS, sizeof(PROC_QOS), "%s", "0");

	if(QoS_Enabled==1){
		snprintf(PROC_QOS, sizeof(PROC_QOS), "%s", "1");
	}

	RunSystemCmd("/proc/qos", "echo", PROC_QOS, NULL_STR);
	
#if defined(CONFIG_RTL_83XX_QOS_SUPPORT)
	snprintf(command, sizeof(command), "echo \"restore %s dscp remark\" > /proc/rtl_hw_qos_config", pInterface);
	system(command);

	snprintf(command, sizeof(command), "echo \"restore %s vlan remark\" > /proc/rtl_hw_qos_config", pInterface);
	system(command);

	snprintf(command, sizeof(command), "echo \"restore %s dscp remark\" > /proc/rtl_hw_qos_config", br_interface);
	system(command);

	snprintf(command, sizeof(command), "echo \"restore %s vlan remark\" > /proc/rtl_hw_qos_config", br_interface);
	system(command);

	snprintf(command, sizeof(command), "echo \"disable dscp remark\" > /proc/rtl_hw_qos_config");
	system(command);

	snprintf(command, sizeof(command), "echo \"disable vlan remark\" > /proc/rtl_hw_qos_config");
	system(command);

	snprintf(command, sizeof(command), "echo nat %d > /proc/rtl_83xxQos_config", (QoS_Enabled > 0 ? 1 : 0));
	system(command);
#endif

	if(QoS_Enabled==1){
		if(QoS_Auto_Uplink==0){
			uplink_speed=QoS_Manual_Uplink;
			if(uplink_speed < 100)
				uplink_speed=100;
		}

		if(QoS_Auto_Downlink==0){
			downlink_speed=QoS_Manual_Downlink;
			if(downlink_speed < 100)
				downlink_speed=100;
		}

		/* total bandwidth section--uplink*/

		/* 	root qdisc
			tc qdisc add dev $WAN root handle 2:0 htb default 2 r2q 64	*/
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface, _root, _handle, "2:0", _htb, _default, "2", _r2q, "64", NULL_STR);

		snprintf(tmp_args, sizeof(tmp_args), "%dkbit", uplink_speed);

		/*	root class
			tc class add dev $WAN parent 2:0 classid 2:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit	*/
		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:0", _classid, "2:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);
		

		/*	default class
			tc class add dev $WAN parent 2:1 classid 2:2 htb rate 1kbit ceil ${UPLINK_SPEED}kbit prio 256 quantum 30000	*/
		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:1", _classid, "2:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "7",	_quantum, "30000", NULL_STR);
		
		/*	default qdisc
			tc qdisc add dev $WAN parent 2:2 handle 102: sfq perturb 10	*/
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface, _parent, "2:2", _handle, "102:", _sfq, _perturb, "10", NULL_STR);

		#if 1
		if((pInterface2!=NULL)&&strcmp(pInterface2, "ppp0")==0)//wantype is l2tp
		{
			/*	root qdisc
				tc qdisc add dev $WAN2 root handle 3:0 htb default 2 r2q 64	*/
			RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface2, _root, _handle, "3:0", _htb, _default, "2", _r2q, "64", NULL_STR);
			
			snprintf(tmp_args, sizeof(tmp_args), "%dkbit", uplink_speed);
			/*	root class
				tc class add dev $WAN2 parent 3:0 classid 3:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit	*/
			RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:0", _classid, "3:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);

			/*	default class
				tc class add dev $WAN2 parent 3:1 classid 3:2 htb rate 1kbit ceil ${UPLINK_SPEED}kbit prio 256 quantum 30000	*/
			RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:1", _classid, "3:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "7",  _quantum, "30000", NULL_STR);

			/*	default qdisc
				tc qdisc add dev $WAN2 parent 3:2 handle 302: sfq perturb 10	*/
			RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface2, _parent, "3:2", _handle, "302:", _sfq, _perturb, "10", NULL_STR);

			//???
		    //RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, pInterface2, _parent, "4:0", _protocol2, "all",  _prio, "100", _handle, "2", _fw, _classid, "2:2", NULL_STR);
		
		}
		#endif
#ifdef MULTI_PPPOE
			if(needSetOnce){
#endif

		/* total bandwidth section--downlink*/
		/*	root qdisc
			tc qdisc add dev $BRIDGE root handle 5:0 htb default 5 r2q 64	*/
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, br_interface, _root, _handle, "5:0", _htb, _default, "2", _r2q, "64",NULL_STR);
			
	    snprintf(tmp_args, sizeof(tmp_args), "%dkbit", downlink_speed);
		/*	root class
			tc class add dev $BRIDGE parent 5:0 classid 5:1 htb rate ${DOWNLINK_SPEED}kbit ceil ${DOWNLINK_SPEED}kbit	*/
    	RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:0", _classid, "5:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);

		/*	default class
			tc class add dev $BRIDGE parent 5:1 classid 5:5 htb rate 1kbit ceil ${DOWNLINK_SPEED}kbit prio 256 quantum 30000	*/
	    RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:1", _classid, "5:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "7", _quantum, "30000", NULL_STR);
			
		/*	default qdisc
			tc qdisc add dev $BRIDGE parent 5:5 handle 502: sfq perturb 10	*/
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, br_interface, _parent, "5:2", _handle, "502:", _sfq, _perturb, "10", NULL_STR);

		//??
		//RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, br_interface, _parent, "5:0", _protocol2, "all",  _prio, "100", _handle, "5", _fw, _classid, "5:2", NULL_STR);
		
#ifdef MULTI_PPPOE
		}
#endif

		if(QoS_Rule_EntryNum > 0){
			for (i=1; i<=QoS_Rule_EntryNum; i++) {
				int tcpudpFlag=0;
				*((char *)&entry) = (char)i;
				apmib_get(MIB_QOS_RULE_TBL, (void *)&entry);
				if(entry.enabled > 0)
				{
					//classify
					if( (entry.mode & QOS_RESTRICT_IPV6)  == 0)
					{
						snprintf(str_iptables, sizeof(str_iptables), "%s",Iptables);
					}
					#ifdef CONFIG_IPV6	
					else
					{
						snprintf(str_iptables, sizeof(str_iptables), "%s",Ip6tables);
					}
					#endif

					//protocol 
					if(entry.protocol == 0)
					{
						//both
						tcpudpFlag =1;
						snprintf(str_protocol, sizeof(str_protocol), "%s","");
					}
					else if(entry.protocol ==1)
					{
						snprintf(str_protocol, sizeof(str_protocol), "%s %s", _protocol, "tcp");	
					}
					else if(entry.protocol ==2)
					{
						snprintf(str_protocol, sizeof(str_protocol), "%s %s", _protocol, "udp");	
					}
					else
					{
						printf("not support protocol:%d, [%s]:[%d].\n",entry.protocol,__FUNCTION__,__LINE__);
						continue;
					}
						
					 //dscp
					if(entry.dscp!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
					{
						snprintf(str_dscp, sizeof(str_dscp), "%s %s %s %d",match, match_dscp, _dscp, entry.dscp);
					}
					else
					{
						snprintf(str_dscp, sizeof(str_dscp), "%s", "");
					}
					
					 //802.1p vlan pri
					if(entry.vlan_pri!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
					{
						if(vlanSupport)
							snprintf(str_vlanpir, sizeof(str_vlanpir), "%s %s %s %d", match, match_vlanpriority, prio_value, entry.vlan_pri);
						else
							continue;
					}
					else
					{
						snprintf(str_vlanpir, sizeof(str_vlanpir), "%s", "");
					}				
					//printf("dscp:%s,vlanpri:%s,phyport:%s,[%s][%d].\n",str_dscp,str_vlanpir,str_phyPort,__FUNCTION__,__LINE__);
					
				#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
					// htb pri =0 is the highest priority
					if(entry.policy==RTL_QOS_SP)
					{
						if(entry.priority!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
							snprintf(str_prio, sizeof(str_prio), "%u", (priority-entry.priority));
						else
							snprintf(str_prio, sizeof(str_prio), "%u", ADVANCED_IPQOS_DEF_CHAR_VALUE);
					}
					else if (entry.policy==RTL_QOS_WFQ)
					{
						snprintf(str_prio, sizeof(str_prio), "%u", priority);
						
					}
					else
						printf("not support policy:%d ![%s]:[%d].\n",entry.policy,__FUNCTION__,__LINE__);
				#endif
					// htb pri =0 is the highest priority
	
					//printf("prio:%s  policy:%d,priority:%d ![%s]:[%d].\n",str_prio,entry.policy,entry.priority,__FUNCTION__,__LINE__);
					if(entry.bandwidth > 0)
					{
						//sip match
						ipaddr=((struct in_addr *)entry.local_ip_start);
						if(ipaddr->s_addr)//sip is not empty
						{
							/*this qos rule is set by IP address*/
							ipaddr=((struct in_addr *)entry.local_ip_start);
							tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_start));
							snprintf(str_src_ip1, sizeof(str_src_ip1), "%s", tmpStr);
							ipaddr=((struct in_addr *)entry.local_ip_end);
							if(ipaddr->s_addr)
							{
								tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_end));
								snprintf(str_src_ip2, sizeof(str_src_ip2), "%s", tmpStr);
								snprintf(str_src_ip, sizeof(str_src_ip), "%s %s %s %s-%s",match, ip_range, src_rnage,str_src_ip1, str_src_ip2);
							}
							else
							{
								snprintf(str_src_ip, sizeof(str_src_ip), "%s %s",_src,str_src_ip1);
							}
						
						}
						else
						{
							snprintf(str_src_ip, sizeof(str_src_ip), "%s", "");
						}
						
						//dip match
						ipaddr=((struct in_addr *)entry.remote_ip_start);
						if(ipaddr->s_addr)//dip is not empty
						{
							tmpStr = inet_ntoa(*((struct in_addr *)entry.remote_ip_start));
							snprintf(str_dest_ip1, sizeof(str_dest_ip1), "%s", tmpStr);
							ipaddr=((struct in_addr *)entry.remote_ip_end);
							if(ipaddr->s_addr)
							{
								tmpStr = inet_ntoa(*((struct in_addr *)entry.remote_ip_end));
								snprintf(str_dest_ip2, sizeof(str_dest_ip2), "%s", tmpStr);
								snprintf(str_dest_ip, sizeof(str_dest_ip), "%s %s %s %s-%s",match, ip_range, dst_rnage,str_dest_ip1, str_dest_ip2);
							}
							else
							{
								snprintf(str_dest_ip, sizeof(str_dest_ip), "%s %s",_dest,str_dest_ip1);
							}
						
						}
						else
						{
							snprintf(str_dest_ip, sizeof(str_dest_ip), "%s", "");
						}

						//port
						if(entry.remote_port_start){
							if(entry.protocol == 0)
							{
								tcpudpFlag =1;
							}
							if(!entry.remote_port_end)
								snprintf(str_dest_port, sizeof(str_dest_port), "%s %d",dport, entry.remote_port_start);	
							else	
								snprintf(str_dest_port, sizeof(str_dest_port), "%s %d:%d",dport, entry.remote_port_start,entry.remote_port_end);	
						}
						else
						{
							snprintf(str_dest_port, sizeof(str_dest_port), "%s","");
						}
							
						if(entry.local_port_start){
							if(entry.protocol == 0)
							{
								tcpudpFlag =1;
							}
							if(!entry.local_port_end)
								snprintf(str_src_port, sizeof(str_src_port), "%s %d",sport, entry.local_port_start);	
							else	
								snprintf(str_src_port, sizeof(str_src_port), "%s %d:%d",sport, entry.local_port_start,entry.local_port_end);	
						}
						else
						{
							snprintf(str_src_port, sizeof(str_src_port), "%s","");
						}
						//printf("sip:%s,dip:%s,[%s][%d].\n",str_src_ip,str_dest_ip,__FUNCTION__,__LINE__);
						//smac
						if((entry.mac[0]==0)&&(entry.mac[1]==0) && (entry.mac[2]==0)&&(entry.mac[3]==0) 
						 	 && (entry.mac[4]==0)&&(entry.mac[5]==0))
						{
					 		snprintf(str_src_mac, sizeof(str_src_mac), "%s", "");
					 	}
						else
						{
					 		snprintf(str_src_mac, sizeof(str_src_mac), "%s %s %s %02x:%02x:%02x:%02x:%02x:%02x",match, _mac, mac_src,entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
					 	}
						 //dmac
						if(!((entry.dst_mac[0]==0)&&(entry.dst_mac[1]==0) && (entry.dst_mac[2]==0)&&(entry.dst_mac[3]==0)
							&&(entry.dst_mac[4]==0)&&(entry.dst_mac[5]==0)))
					 	{
							if(staticIpSupport)
					 			snprintf(str_dest_mac, sizeof(str_dest_mac), "%s %s %s %02x:%02x:%02x:%02x:%02x:%02x",match, _mac, mac_dst,entry.dst_mac[0], entry.dst_mac[1], entry.dst_mac[2], entry.dst_mac[3], entry.dst_mac[4], entry.dst_mac[5]);
							else
								continue;
						}
						else
					 	{
					 		snprintf(str_dest_mac, sizeof(str_dest_mac), "%s", "");
					 	}
							 
						//printf("smac:%s,dmac:%s,[%s][%d].\n",str_src_mac,str_dest_mac,__FUNCTION__,__LINE__);
						//phy port
						if(entry.phyport!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
						{
							snprintf(str_phyPort, sizeof(str_phyPort), "%s %s %s %d",match, match_phyport, _sphyport, entry.phyport);
						}
						else
						{
							snprintf(str_phyPort, sizeof(str_phyPort), "%s", "");
						}
						
						snprintf(tmp_args, sizeof(tmp_args), "%d", wan_pkt_mark);
						if(tcpudpFlag==0){
							
							#ifdef CONFIG_IPV6
							if(entry.mode & QOS_RESTRICT_IPV6){							
								//ip6tables -A PREROUTING -t mangle -s 2001::1 -j MARK --set-mark 13
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
								Ip6tables, ADD, PREROUTING, _table, mangle_table , in, br_interface, str_protocol, _src, entry.ip6_src, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);
							}
							#endif

							if(str_dest_mac[0] != '\0'){
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									str_iptables, ADD, POSTROUTING, _table, mangle_table,out, pInterface,
									str_protocol,str_dest_mac,jump, MARK, set_mark, tmp_args, NULL_STR);		
								system(command);
							}
							else{
								if((strcmp(entry.ip6_src,"") == 0))
								{
									snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
										str_iptables, ADD, PREROUTING, _table, mangle_table,in, br_interface,
										str_protocol,str_src_port,str_dest_port,str_src_ip,str_dest_ip,
										str_src_mac,str_phyPort,str_dscp,str_vlanpir,
										jump, MARK, set_mark, tmp_args, NULL_STR);
									system(command);
								}
							}
							//printf("cmd:%s , [%s]:[%d].\n",command,__FUNCTION__,__LINE__);
						}
						else
						{
							#ifdef CONFIG_IPV6
							if(entry.mode & QOS_RESTRICT_IPV6){							
								//ip6tables -A PREROUTING -t mangle -s 2001::1 -j MARK --set-mark 13
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s ", 
								Ip6tables, ADD, PREROUTING, _table, mangle_table , in, br_interface, _protocol, _tcp, _src, entry.ip6_src, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);

								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s ", 
								Ip6tables, ADD, PREROUTING, _table, mangle_table , in, br_interface, _protocol, _udp, _src, entry.ip6_src, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);
							}
							#endif
							
							if(str_dest_mac[0] != '\0'){
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									str_iptables, ADD, POSTROUTING, _table, mangle_table,out, pInterface,
									_protocol, _tcp,str_dest_mac, jump, MARK, set_mark, tmp_args, NULL_STR);
									
								system(command);
												
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									str_iptables, ADD, POSTROUTING, _table, mangle_table,out, pInterface,
									_protocol, _udp, str_dest_mac, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);
							}						
							else{
								if((strcmp(entry.ip6_src,"") == 0))
								{
									snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
										str_iptables, ADD, PREROUTING, _table, mangle_table,in, br_interface,
										_protocol, _tcp,str_src_port,str_dest_port,str_src_ip,str_dest_ip,
										str_src_mac,str_phyPort,str_dscp,str_vlanpir,
										jump, MARK, set_mark, tmp_args, NULL_STR);

									system(command);
													
									snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s  %s %s %s %s %s",
										str_iptables, ADD, PREROUTING, _table, mangle_table,in, br_interface,
										_protocol, _udp,str_src_port,str_dest_port,str_src_ip,str_dest_ip,
										str_src_mac,str_phyPort,str_dscp,str_vlanpir,
										jump, MARK, set_mark, tmp_args, NULL_STR);
									system(command);
								}
							}
						}
						
						if(entry.mode & QOS_RESTRICT_MIN)
						{
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.mode & QOS_RESTRICT_IPV6){
								snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth);	
								snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", uplink_speed);
							}
							else
							{
								if(entry.policy==RTL_QOS_SP){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", uplink_speed);
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",uplink_speed);
								}
								else if (entry.policy==RTL_QOS_WFQ){
									//not support
									snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth); //weight rate
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", uplink_speed);
								}
							}
							#else
							snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", uplink_speed);
							snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth);
							#endif
						}
						else if(entry.mode & QOS_RESTRICT_MAX)
						{
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.mode & QOS_RESTRICT_IPV6){
								snprintf(str_rate, sizeof(str_rate), "%s", "1kbit");	
								snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",entry.bandwidth);
							}
							else
							{
								if(entry.policy==RTL_QOS_SP){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", entry.bandwidth);
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", entry.bandwidth);
								}
								else if (entry.policy==RTL_QOS_WFQ){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", (1*entry.weight));
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", entry.bandwidth);
								}
							}
							#else
							snprintf(str_rate, sizeof(str_rate), "%s", "1kbit");
							snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",entry.bandwidth);
							#endif
						}
							
						snprintf(tmp_args1, sizeof(tmp_args1), "2:%d", wan_pkt_mark);
						snprintf(tmp_args4, sizeof(tmp_args4), "1%d:", wan_pkt_mark);
						//printf("mark:%d  rate:%s ceil:%s ![%s]:[%d].\n",wan_pkt_mark,str_rate,str_ceil,__FUNCTION__,__LINE__);

						#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
						//TC_CMD="tc class add dev $WAN parent 2:1 classid 2:$wan_pkt_mark htb rate 1kbit ceil ${bandwidth}kbit prio 2 quantum 30000"
						RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:1", _classid, tmp_args1, _htb, _rate, str_rate, _ceil, str_ceil, _prio, str_prio, _quantum, "30000", NULL_STR);
						#else
						RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:1", _classid, tmp_args1, _htb, _rate, str_rate, _ceil, str_ceil, _prio, "2", _quantum, "30000", NULL_STR);
						#endif
						//TC_CMD="tc qdisc add dev $WAN parent 2:$wan_pkt_mark handle 1$wan_pkt_mark: sfq perturb 10"
						RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface, _parent, tmp_args1, _handle, tmp_args4, _sfq, _perturb, "10", NULL_STR);

						
						RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, pInterface, _parent, "2:0", _protocol2, "all", _prio, "100", _handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
						//TC_CMD="tc filter add dev $WAN parent 2:0 protocol ip prio 100 handle $wan_pkt_mark fw classid 2:$wan_pkt_mark"

						snprintf(tmp_args1, sizeof(tmp_args1), "3:%d", wan_pkt_mark);
						snprintf(tmp_args4, sizeof(tmp_args4), "3%d:", wan_pkt_mark);
						
						if(entry.mode & QOS_RESTRICT_MIN)
						{
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.mode & QOS_RESTRICT_IPV6){
								snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth);	
								snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", uplink_speed);
							}
							else
							{
								if(entry.policy==RTL_QOS_SP){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", uplink_speed);
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",uplink_speed);
								}
								else if (entry.policy==RTL_QOS_WFQ){
									//not support
									snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth); //weight rate
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", uplink_speed);
								}
							}
							#else
							snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth);	
							snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", uplink_speed);
							#endif
						}
						else if(entry.mode & QOS_RESTRICT_MAX)
						{
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.mode & QOS_RESTRICT_IPV6){
								snprintf(str_rate, sizeof(str_rate), "%s", "1kbit");	
								snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",entry.bandwidth);
							}
							else
							{
								if(entry.policy==RTL_QOS_SP){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", entry.bandwidth);
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", entry.bandwidth);
								}
								else if (entry.policy==RTL_QOS_WFQ){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", (1*entry.weight));
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", entry.bandwidth);
								}
							}
							#else
							snprintf(str_rate, sizeof(str_rate), "%s", "1kbit");
							snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",entry.bandwidth);
							#endif
						}
						
						if((pInterface2!=NULL)&&strcmp(pInterface2, "ppp0")==0)//wantype is l2tp
						{
							//TC_CMD="tc class add dev $WAN2 parent 3:1 classid 3:$wan_pkt_mark htb rate 1kbit ceil ${bandwidth}kbit prio 2 quantum 30000"
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:1", _classid, tmp_args1, _htb, _rate, str_rate, _ceil, str_ceil, _prio, str_prio , _quantum, "30000", NULL_STR);
							#else
							RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:1", _classid, tmp_args1, _htb, _rate, str_rate, _ceil, str_ceil, _prio, "2" , _quantum, "30000", NULL_STR);
							#endif
							
							//TC_CMD="tc qdisc add dev $WAN2 parent 3:$wan_pkt_mark handle 3$wan_pkt_mark: sfq perturb 10"
							RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface2, _parent, tmp_args1, _handle, tmp_args4, _sfq, _perturb, "10", NULL_STR);

							//TC_CMD="tc filter add dev $WAN2 parent 3:0 protocol ip prio 100 handle $wan_pkt_mark fw classid 3:$wan_pkt_mark"
							RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, pInterface2, _parent, "3:0", _protocol2, "all",  _prio, "100", _handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
						}

						if(j<MAX_QOS_RULE_NUM)
						{
							qosConfigInfo[0][j].netif=pInterface;
							qosConfigInfo[0][j].mark=wan_pkt_mark;
							qosConfigInfo[0][j].index =i;
							j++;	
						}
							
						//printf("[%d ]remark dscp:%d,vlanpri:%d ![%s]:[%d].\n",j,entry.remark_dscp,entry.remark_vlan_pri,__FUNCTION__,__LINE__);
						wan_pkt_mark = wan_pkt_mark+1;	
					}
						
				#ifdef MULTI_PPPOE
					if(needSetOnce){
				#endif
					if(entry.bandwidth_downlink > 0){/*DOWNlink*/
						//sip match
						ipaddr=((struct in_addr *)entry.remote_ip_start);
						if(ipaddr->s_addr)//sip is not empty
						{
							/*this qos rule is set by IP address*/
							tmpStr = inet_ntoa(*((struct in_addr *)entry.remote_ip_start));
							snprintf(str_src_ip1, sizeof(str_src_ip1), "%s", tmpStr);
							ipaddr=((struct in_addr *)entry.remote_ip_end);
							if(ipaddr->s_addr)
							{
								tmpStr = inet_ntoa(*((struct in_addr *)entry.remote_ip_end));
								snprintf(str_src_ip2, sizeof(str_src_ip2), "%s", tmpStr);
								snprintf(str_src_ip, sizeof(str_src_ip), "%s %s %s %s-%s",match, ip_range, src_rnage,str_src_ip1, str_src_ip2);
							}
							else
							{
								snprintf(str_src_ip, sizeof(str_src_ip), "%s %s", _src, str_src_ip1);
							}
						
						}
						else
						{
							snprintf(str_src_ip, sizeof(str_src_ip), "%s", "");
						}
		//dip match
						ipaddr=((struct in_addr *)entry.local_ip_start);
						if(ipaddr->s_addr)//dip is not empty
						{
							tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_start));
							snprintf(str_dest_ip1, sizeof(str_dest_ip1), "%s", tmpStr);
							ipaddr=((struct in_addr *)entry.local_ip_end);
							if(ipaddr->s_addr)
							{
								tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_end));
								snprintf(str_dest_ip2, sizeof(str_dest_ip2), "%s", tmpStr);
								snprintf(str_dest_ip, sizeof(str_dest_ip), "%s %s %s %s-%s",match, ip_range, dst_rnage,str_dest_ip1, str_dest_ip2);
							}
							else
							{
								snprintf(str_dest_ip, sizeof(str_dest_ip), "%s %s",_dest,str_dest_ip1);
							}
						}
						else
						{
							snprintf(str_dest_ip, sizeof(str_dest_ip), "%s", "");
						}

						//port
						if(entry.remote_port_start){
							if(entry.protocol == 0)
							{
								tcpudpFlag =1;
							}
							if(!entry.remote_port_end)
								snprintf(str_dest_port, sizeof(str_dest_port), "%s %d",sport, entry.remote_port_start);	
							else	
								snprintf(str_dest_port, sizeof(str_dest_port), "%s %d:%d",sport, entry.remote_port_start,entry.remote_port_end);	
						}
						else
						{
							snprintf(str_dest_port, sizeof(str_dest_port), "%s", "");
						}
						
						if(entry.local_port_start){
							if(entry.protocol == 0)
							{
								tcpudpFlag =1;
							}
							if(!entry.local_port_end)
								snprintf(str_src_port, sizeof(str_src_port), "%s %d", dport, entry.local_port_start);	
							else	
								snprintf(str_src_port, sizeof(str_src_port), "%s %d:%d", dport, entry.local_port_start, entry.local_port_end);	
						}
						else
						{
							snprintf(str_src_port, sizeof(str_src_port), "%s","");
						}
						//printf("sip:%s,dip:%s,[%s][%d].\n",str_src_ip,str_dest_ip,__FUNCTION__,__LINE__);
						//smac
						if((entry.mac[0]==0)&&(entry.mac[1]==0) && (entry.mac[2]==0)&&(entry.mac[3]==0) 
						 	 && (entry.mac[4]==0)&&(entry.mac[5]==0))
						{
					 		snprintf(str_dest_mac, sizeof(str_dest_mac), "%s", ""); 
					 	}
						else
						{
					 		snprintf(str_dest_mac, sizeof(str_dest_mac), "%s %s %s %02x:%02x:%02x:%02x:%02x:%02x",match, _mac, mac_dst,entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
					 	}
						 //dmac
						if(!((entry.dst_mac[0]==0)&&(entry.dst_mac[1]==0) && (entry.dst_mac[2]==0)&&(entry.dst_mac[3]==0)
							&&(entry.dst_mac[4]==0)&&(entry.dst_mac[5]==0)))
					 	{
							if(staticIpSupport)
					 			snprintf(str_src_mac, sizeof(str_src_mac), "%s %s %s %02x:%02x:%02x:%02x:%02x:%02x",match, _mac, mac_src,entry.dst_mac[0], entry.dst_mac[1], entry.dst_mac[2], entry.dst_mac[3], entry.dst_mac[4], entry.dst_mac[5]);
							else
								continue;
						}	
						else
					 	{
					 		snprintf(str_src_mac, sizeof(str_src_mac), "%s", ""); 
					 	}
						 
						//printf("smac:%s,dmac:%s,[%s][%d].\n",str_src_mac,str_dest_mac,__FUNCTION__,__LINE__);

						if(entry.phyport!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
						{
							snprintf(str_phyPort, sizeof(str_phyPort), "%s %s %s %d",match, match_phyport, _sphyport, entry.phyport);
						}
						else
						{
							snprintf(str_phyPort, sizeof(str_phyPort), "%s", "");
						}

						snprintf(tmp_args, sizeof(tmp_args), "%d", lan_pkt_mark);
						if(tcpudpFlag==0){

							#ifdef CONFIG_IPV6
							if(entry.mode & QOS_RESTRICT_IPV6){							
								//ip6tables -A POSTROUTING -t mangle -d 2001::1 -j MARK --set-mark 53
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
								Ip6tables, ADD, POSTROUTING, _table, mangle_table , out, br_interface, str_protocol, _dest, entry.ip6_src, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);
							}
							#endif
							
							if(str_src_mac[0] != '\0'){
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									str_iptables, ADD, PREROUTING, _table, mangle_table,in, pInterface,
									str_protocol,str_src_mac,jump, MARK, set_mark, tmp_args, NULL_STR);						
								system(command);
							}
							else{
								if((strcmp(entry.ip6_src,"") == 0))
								{
									snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
										str_iptables, ADD, POSTROUTING, _table, mangle_table,out, br_interface,
										str_protocol,str_src_port,str_dest_port,str_src_ip,str_dest_ip,
										str_dest_mac,str_phyPort,str_dscp,str_vlanpir,
										jump, MARK, set_mark, tmp_args, NULL_STR);	
									system(command);
								}
							}	
						
						//printf("cmd:%s , [%s]:[%d].\n",command,__FUNCTION__,__LINE__);
						}
						else
						{
							
							#ifdef CONFIG_IPV6
							if(entry.mode & QOS_RESTRICT_IPV6){							
								//ip6tables -A POSTROUTING -t mangle -d 2001::1 -j MARK --set-mark 53
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
								Ip6tables, ADD, POSTROUTING, _table, mangle_table , out, br_interface, _protocol, _tcp, _dest, entry.ip6_src, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);

								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
								Ip6tables, ADD, POSTROUTING, _table, mangle_table , out, br_interface, _protocol, _udp, _dest, entry.ip6_src, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);
							}
							#endif

							if(str_src_mac[0] != '\0'){
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									str_iptables, ADD, PREROUTING, _table, mangle_table,in, pInterface,
									_protocol, _tcp,str_src_mac, jump, MARK, set_mark, tmp_args, NULL_STR);
								//printf("cmd:%s , [%s]:[%d].\n",command,__FUNCTION__,__LINE__);
									
								system(command);
												
								snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
									str_iptables, ADD, PREROUTING, _table, mangle_table,in, pInterface,
									_protocol, _udp, str_src_mac, jump, MARK, set_mark, tmp_args, NULL_STR);
								system(command);
								//printf("cmd:%s , [%s]:[%d].\n",command,__FUNCTION__,__LINE__);
							}
							else{
								if((strcmp(entry.ip6_src,"") == 0))
								{
									snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
										str_iptables, ADD, POSTROUTING, _table, mangle_table,out, br_interface,
										_protocol, _tcp,str_src_port,str_dest_port,str_src_ip,str_dest_ip,
										str_dest_mac,str_phyPort,str_dscp,str_vlanpir,
										jump, MARK, set_mark, tmp_args, NULL_STR);
									//printf("cmd:%s , [%s]:[%d].\n",command,__FUNCTION__,__LINE__);
										
									system(command);
													
									snprintf(command, sizeof(command), "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s  %s %s %s %s %s",
										str_iptables, ADD, POSTROUTING, _table, mangle_table,out, br_interface,
										_protocol, _udp,str_src_port,str_dest_port,str_src_ip,str_dest_ip,
										str_dest_mac,str_phyPort,str_dscp,str_vlanpir,
										jump, MARK, set_mark, tmp_args, NULL_STR);
										//printf("cmd:%s , [%s]:[%d].\n",command,__FUNCTION__,__LINE__);
									system(command);
								}		
							}				
						}
							
						snprintf(tmp_args1, sizeof(tmp_args1), "5:%d", lan_pkt_mark);
						snprintf(tmp_args4, sizeof(tmp_args4), "5%d:", lan_pkt_mark);
					
						if(entry.mode & QOS_RESTRICT_MIN)
						{
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.mode & QOS_RESTRICT_IPV6){
								snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth_downlink);	
								snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", downlink_speed);
							}
							else
							{
								if(entry.policy==RTL_QOS_SP){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", downlink_speed);
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",downlink_speed);
								}
								else if(entry.policy==RTL_QOS_WFQ){
									//not support
									snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth_downlink); //weight rate
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", downlink_speed);
								}
							}
							#else
							snprintf(str_rate, sizeof(str_rate), "%ldkbit",entry.bandwidth_downlink);	
							snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", downlink_speed);
							#endif
						}
						else if(entry.mode & QOS_RESTRICT_MAX)
						{
							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.mode & QOS_RESTRICT_IPV6){
								snprintf(str_rate, sizeof(str_rate), "%s", "1kbit");	
								snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",entry.bandwidth_downlink);
							}
							else
							{
								if(entry.policy==RTL_QOS_SP){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", entry.bandwidth_downlink);
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", entry.bandwidth_downlink);
								}
								else if(entry.policy==RTL_QOS_WFQ){
									snprintf(str_rate, sizeof(str_rate), "%ldkbit", (1*entry.weight));
									snprintf(str_ceil, sizeof(str_ceil), "%ldkbit", entry.bandwidth_downlink);
								}
							}
							#else
							snprintf(str_rate, sizeof(str_rate), "%s", "1kbit");	
							snprintf(str_ceil, sizeof(str_ceil), "%ldkbit",entry.bandwidth_downlink);
							#endif
						}
							
						//printf("mark:%d  rate:%s,ceil:%s ![%s]:[%d].\n",lan_pkt_mark,str_rate,str_ceil,__FUNCTION__,__LINE__);
						//TC_CMD="tc class add dev $BRIDGE parent 5:1 classid 5:$lan_pkt_mark htb rate 1kbit ceil ${bandwidth_dl}kbit prio 2 quantum 30000"
						#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
						RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:1", _classid, tmp_args1, _htb, _rate, str_rate, _ceil, str_ceil, _prio, str_prio ,_quantum, "30000", NULL_STR);
						#else
						RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:1", _classid, tmp_args1, _htb, _rate, str_rate, _ceil, str_ceil, _prio, "2" ,_quantum, "30000", NULL_STR);
						#endif
						//TC_CMD="tc qdisc add dev $BRIDGE parent 5:$lan_pkt_mark handle 5$lan_pkt_mark: sfq perturb 10"
						RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, br_interface, _parent, tmp_args1, _handle, tmp_args4, _sfq, _perturb, "10", NULL_STR);

						//TC_CMD="tc filter add dev $BRIDGE parent 5:0 protocol ip prio 100 handle $lan_pkt_mark fw classid 5:$lan_pkt_mark"
						RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, br_interface, _parent, "5:0", _protocol2, "all",  _prio, "100",_handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
						

						if(k<MAX_QOS_RULE_NUM){
							qosConfigInfo[1][k].netif=br_interface;
							qosConfigInfo[1][k].mark=lan_pkt_mark;
							qosConfigInfo[1][k].index =i;
							k++;	
						}
						//printf("[%d]: remark dscp:%d,vlanpri:%d ![%s]:[%d].\n",k,entry.remark_dscp,entry.remark_vlan_pri,__FUNCTION__,__LINE__);
						lan_pkt_mark = lan_pkt_mark+1;
					}
				#ifdef MULTI_PPPOE
					}
				#endif					
				}
			}
			
			//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
			//qos config
			j=0;
			k=0;
			for (j=0;j<RTL_MAX_HW_NETIF_NUM;j++)
			{
				for (k=0;k<MAX_QOS_RULE_NUM;k++)
				{
					if(qosConfigInfo[j][k].netif)
					{
						*((char *)&entry) = (char)qosConfigInfo[j][k].index;
						apmib_get(MIB_QOS_RULE_TBL, (void *)&entry);
						/*printf("[%d][%d]:netif:%s, remark dscp:%d, vlanpri:%d,bridgemode:%d, port:%d,dscp:%d, [%s]:[%d].\n",
							j,k, qosConfigInfo[j][k].netif,entry.remark_dscp,entry.remark_vlan_pri,
							entry.bridgemode,entry.phyport,entry.dscp,__FUNCTION__,__LINE__);*/
						
						if((entry.remark_vlan_pri!=ADVANCED_IPQOS_DEF_CHAR_VALUE)||
							(entry.remark_dscp!=ADVANCED_IPQOS_DEF_CHAR_VALUE))
						{
							//remark
							if( (entry.mode & QOS_RESTRICT_IPV6)  == 0)
							{
								snprintf(str_iptables, sizeof(str_iptables), "%s",Iptables);
							}
							#ifdef CONFIG_IPV6	
							else
							{
								snprintf(str_iptables, sizeof(str_iptables), "%s",Ip6tables);
							}
							#endif
							
							if(entry.remark_dscp!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
							{
								snprintf(command, sizeof(command), "%s -t mangle -A POSTROUTING -o %s -m mark --mark %d -j DSCP --set-dscp %d %s",str_iptables,qosConfigInfo[j][k].netif ,qosConfigInfo[j][k].mark,entry.remark_dscp,NULL_STR);
								system(command);
								if((qosConfigInfo[j][k].netif==pInterface)&&(pInterface!=wanInfterface))
								{
									snprintf(command, sizeof(command), "%s -t mangle -A POSTROUTING -o %s -m mark --mark %d -j DSCP --set-dscp %d %s",str_iptables,wanInfterface ,qosConfigInfo[j][k].mark,entry.remark_dscp,NULL_STR);
									system(command);
								}
							}
							
							if(entry.remark_vlan_pri!=ADVANCED_IPQOS_DEF_CHAR_VALUE)
							{
								if(vlanSupport){
									snprintf(command, sizeof(command), "%s -t mangle -A POSTROUTING -o %s -m mark --mark %d -j VLANPRIORITY --set-vlanpri %d %s",str_iptables,qosConfigInfo[j][k].netif ,qosConfigInfo[j][k].mark,entry.remark_vlan_pri,NULL_STR);
									system(command);
									
									if((qosConfigInfo[j][k].netif==pInterface)&&(pInterface!=wanInfterface))
									{
										snprintf(command, sizeof(command), "%s -t mangle -A POSTROUTING -o %s -m mark --mark %d -j VLANPRIORITY --set-vlanpri %d %s",str_iptables,wanInfterface,qosConfigInfo[j][k].mark,entry.remark_vlan_pri,NULL_STR);
										system(command);
									}
								}
							}

							#if defined(CONFIG_RTL_HW_QOS_SUPPORT)
							if(entry.remark_vlan_pri!=ADVANCED_IPQOS_DEF_CHAR_VALUE){
								snprintf(command, sizeof(command), "echo \"remark netif %s mark %d vlan %d\" > /proc/rtl_hw_qos_config", qosConfigInfo[j][k].netif, qosConfigInfo[j][k].mark, entry.remark_vlan_pri);
								system(command);
							}

							if(entry.remark_dscp!=ADVANCED_IPQOS_DEF_CHAR_VALUE){
								if((entry.mode & QOS_RESTRICT_IPV6)  == 0){
									snprintf(command, sizeof(command), "echo \"remark netif %s mark %d dscp v4 %d\" > /proc/rtl_hw_qos_config", qosConfigInfo[j][k].netif, qosConfigInfo[j][k].mark, entry.remark_dscp);
									system(command);
								}
							#ifdef CONFIG_IPV6
								else{
									snprintf(command, sizeof(command), "echo \"remark netif %s mark %d dscp v6 %d\" > /proc/rtl_hw_qos_config", qosConfigInfo[j][k].netif, qosConfigInfo[j][k].mark, entry.remark_dscp);
									system(command);
								}
							#endif
						}
					#endif
						}
					}
				}
			}
		}
	}
#endif
	return 0;
}
#else

#ifdef MULTI_PPPOE
int set_QoS(int operation, int wan_type, int wisp_wan_id , char* interface)
#else 
int set_QoS(int operation, int wan_type, int wisp_wan_id)
#endif
{
#ifdef   HOME_GATEWAY
	char *br_interface="br0";
	char tmp_args[32]={0}, tmp_args1[32]={0}, tmp_args2[32]={0};
	char tmp_args3[64]={0}, tmp_args4[32]={0};
	char *tmpStr=NULL;
	int wan_pkt_mark=13, lan_pkt_mark=53;
	char iface[20], *pInterface="eth1", *pInterface2=NULL;
	int i, QoS_Enabled=0;
	int QoS_Auto_Uplink=0, QoS_Manual_Uplink=0;
	int QoS_Auto_Downlink=0, QoS_Manual_Downlink=0;
	int QoS_Rule_EntryNum=0;
	char PROC_QOS[128]={0};
	int uplink_speed=102400, downlink_speed=102400;
	IPQOS_T entry;
	int get_wanip=0;
	struct in_addr wanaddr;
	unsigned char str_l7_filter[128]={0};

	int needSetOnce = 1;
//#define QOS_MAC_U32_FILTER 1	
#ifdef QOS_MAC_U32_FILTER	
	unsigned char macAddr[64]={0};
#endif	
#if defined (CONFIG_RTL_8198)|| defined (CONFIG_RTL_8198C)
	uplink_speed=1024000;
	downlink_speed=1024000;
#endif

#ifdef MULTI_PPPOE
	if(!strncmp(interface,"ppp0",3) ||!strncmp(interface,"ppp1",3) || !strncmp(interface,"ppp2",3)
				|| !strncmp(interface,"ppp3",3))
	{
		FILE* fp;
		int pppDeviceNumber;
		if((fp=fopen("/etc/ppp/hasPppoedevice","r+"))==NULL)
		{
#ifdef MULTI_PPP_DEBUG	   
			printf("Cannot open this file\n");
#endif
			return 0;
		}
		fscanf(fp,"%d",&pppDeviceNumber);
		if(pppDeviceNumber == 1)
			needSetOnce = 1;
		else if( pppDeviceNumber >=2)
			needSetOnce = 0;						
	}
#endif
#ifdef MULTI_PPPOE
		if(needSetOnce){
#endif
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, X, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, Z, _table, mangle_table, NULL_STR);

#ifdef CONFIG_IPV6
	RunSystemCmd(NULL_FILE, Ip6tables, FLUSH, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Ip6tables, X, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Ip6tables, Z, _table, mangle_table, NULL_STR);
#endif

#ifdef MULTI_PPPOE
		}
#endif
	if(operation == WISP_MODE){
		sprintf(iface, "wlan%d", wisp_wan_id);
#if defined(CONFIG_SMART_REPEATER)			
		getWispRptIfaceName(iface,wisp_wan_id);
		//strcat(iface, "-vxd");
#endif		
		pInterface = iface;
		if (wan_type == PPPOE || wan_type == PPTP /*|| wan_type == L2TP */)
#ifdef MULTI_PPPOE
			pInterface = interface;
#else
			pInterface="ppp0";
#endif
	}else{
		if(operation == GATEWAY_MODE){
			if (wan_type == PPPOE || wan_type == PPTP || wan_type == USB3G /*|| wan_type == L2TP*/)
#ifdef MULTI_PPPOE
			pInterface = interface;
#else
			pInterface="ppp0";
#endif
		}
	}

	if(wan_type == L2TP)//wantype is l2tp
		pInterface2="ppp0";


	#if defined(CONFIG_4G_LTE_SUPPORT)
	if (lte_wan()) {
		pInterface = "usb0";
	}
	#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */

	get_wanip = getInAddr(pInterface, IP_ADDR_T, (void *)&wanaddr);
	if( get_wanip ==0){   //get wan ip fail
		printf("No wan ip currently!\n");
		return 0;
	}

	apmib_get( MIB_QOS_ENABLED, (void *)&QoS_Enabled);
	apmib_get( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&QoS_Auto_Uplink);
	apmib_get( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&QoS_Manual_Uplink);
	apmib_get( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&QoS_Manual_Downlink);
	apmib_get( MIB_QOS_AUTO_DOWNLINK_SPEED, (void *)&QoS_Auto_Downlink);
	apmib_get( MIB_QOS_RULE_TBL_NUM, (void *)&QoS_Rule_EntryNum);

	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", br_interface, "root", NULL_STR);

	//To avoid rule left when wan changed
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", pInterface, "root", NULL_STR);
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", "ppp0", "root", NULL_STR);

	if((strcmp(pInterface, "eth1")!=0)&&(strcmp(pInterface, "ppp0")!=0))
		RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", pInterface, "root", NULL_STR);

#ifdef MULTI_PPPOE
	if(needSetOnce){
#endif
	sprintf(PROC_QOS, "%s", "0,");

	if(QoS_Enabled==1){
		sprintf(PROC_QOS, "%s", "1,");
	}

	// echo /proc/qos should before tc rules because of qos patch (CONFIG_RTL_QOS_PATCH in kernel)
	RunSystemCmd("/proc/qos", "echo", PROC_QOS, NULL_STR);
#ifdef MULTI_PPPOE
		}
#endif

	if(QoS_Enabled==1){
		if(QoS_Auto_Uplink==0){
			uplink_speed=QoS_Manual_Uplink;
			if(uplink_speed < 100)
				uplink_speed=100;
		}

		// patch for uplink QoS accuracy
#if 0
#ifdef CONFIG_RTL_8198
		if(uplink_speed > 160000)
			uplink_speed=160000;
#else
		if(uplink_speed > 75000)
			uplink_speed=75000;
#endif
#endif

		if(QoS_Auto_Downlink==0){
			downlink_speed=QoS_Manual_Downlink;
			if(downlink_speed < 100)
				downlink_speed=100;
		}
		// patch for downlink QoS accuracy
#if 0
#ifdef CONFIG_RTL_8198
		if(downlink_speed > 130000)
			downlink_speed=130000;
#else
		if(downlink_speed > 70000)
			downlink_speed=70000;
#endif
#endif

		/* total bandwidth section--uplink*/
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface, _root, _handle, "2:0", _htb, _default, "2", _r2q, "64", NULL_STR);
		//tc qdisc add dev $WAN root handle 2:0 htb default 2 r2q 64
		sprintf(tmp_args, "%dkbit", uplink_speed);
		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:0", _classid, "2:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);
		//TC_CMD="tc class add dev $WAN parent 2:0 classid 2:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit"
		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:1", _classid, "2:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "256",  _quantum, "30000", NULL_STR);
    		//TC_CMD="tc class add dev $WAN parent 2:1 classid 2:2 htb rate 1kbit ceil ${UPLINK_SPEED}kbit prio 256 quantum 30000"
    		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface, _parent, "2:2", _handle, "102:", _sfq, _perturb, "10", NULL_STR);
    		//TC_CMD="tc qdisc add dev $WAN parent 2:2 handle 102: sfq perturb 10"

#if 1
		if((pInterface2!=NULL)&&strcmp(pInterface2, "ppp0")==0)//wantype is l2tp
		{
			RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface2, _root, _handle, "3:0", _htb, _default, "2", _r2q, "64", NULL_STR);
			//tc qdisc add dev $WAN2 root handle 3:0 htb default 2 r2q 64
			sprintf(tmp_args, "%dkbit", uplink_speed);
			RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:0", _classid, "3:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);
			//TC_CMD="tc class add dev $WAN2 parent 3:0 classid 3:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit"
			RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:1", _classid, "3:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "256",  _quantum, "30000", NULL_STR);
	    		//TC_CMD="tc class add dev $WAN2 parent 3:1 classid 3:2 htb rate 1kbit ceil ${UPLINK_SPEED}kbit prio 256 quantum 30000"
	    		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface2, _parent, "3:2", _handle, "302:", _sfq, _perturb, "10", NULL_STR);
	    		//TC_CMD="tc qdisc add dev $WAN2 parent 3:2 handle 302: sfq perturb 10"
		}
#endif
#ifdef MULTI_PPPOE
			if(needSetOnce){
#endif

		/* total bandwidth section--downlink*/
    		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, br_interface, _root, _handle, "5:0", _htb, _default, "2", _r2q, "64",NULL_STR);
    		//tc qdisc add dev $BRIDGE root handle 5:0 htb default 5 r2q 64
    		sprintf(tmp_args, "%dkbit", downlink_speed);
    		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:0", _classid, "5:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);
    		//TC_CMD="tc class add dev $BRIDGE parent 5:0 classid 5:1 htb rate ${DOWNLINK_SPEED}kbit ceil ${DOWNLINK_SPEED}kbit"
    		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:1", _classid, "5:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "256", _quantum, "30000", NULL_STR);
		//TC_CMD="tc class add dev $BRIDGE parent 5:1 classid 5:5 htb rate 1kbit ceil ${DOWNLINK_SPEED}kbit prio 256 quantum 30000"
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, br_interface, _parent, "5:2", _handle, "502:", _sfq, _perturb, "10", NULL_STR);
		//TC_CMD="tc qdisc add dev $BRIDGE parent 5:5 handle 502: sfq perturb 10"
//		sprintf(PROC_QOS, "%s", "1,");
#ifdef MULTI_PPPOE
		}
#endif

		if(QoS_Rule_EntryNum > 0){
			for (i=1; i<=QoS_Rule_EntryNum; i++) {
				unsigned char command[200]={0};
				*((char *)&entry) = (char)i;
				apmib_get(MIB_QOS_RULE_TBL, (void *)&entry);
				if(entry.enabled > 0){
					if(entry.bandwidth > 0){/*UPlink*/
						sprintf(tmp_args, "%d", wan_pkt_mark);

						if((strcmp(entry.l7_protocol,"") == 0) || (strcmp(entry.l7_protocol,"Disable") == 0))
						{
							sprintf(str_l7_filter,"%s","");
						}
						else
						{
							sprintf(str_l7_filter,"%s %s","-m layer7 --l7proto ", entry.l7_protocol);							
						}
						
						if(entry.mode & QOS_RESTRICT_IP)//if(entry.mode == 5 || entry.mode == 6){
						{
							/*this qos rule is set by IP address*/
							tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_start));
							sprintf(tmp_args1, "%s", tmpStr);
							tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_end));
							sprintf(tmp_args2, "%s", tmpStr);
							sprintf(tmp_args3, "%s-%s",tmp_args1, tmp_args2);
							//iptables -A PREROUTING -t mangle -m iprange --src-range 192.168.1.11-192.168.1.22 -j MARK --set-mark 13
							//RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, mangle_table , match, ip_range, src_rnage, tmp_args3, str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s", Iptables, ADD, PREROUTING, _table, mangle_table , match, ip_range, src_rnage, tmp_args3, str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
//printf("\r\n command=[%s],__[%s-%u]\r\n",command,__FILE__,__LINE__);							
							system(command);
						}
						else if(entry.mode & QOS_RESTRICT_MAC){
							sprintf(tmp_args3, "%02x:%02x:%02x:%02x:%02x:%02x",entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
							//iptables -A PREROUTING -t mangle -m mac --mac-source 00:11:22:33:44:55 -j MARK --set-mark 13
							//RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, mangle_table , match, _mac, mac_src, tmp_args3, str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s", Iptables, ADD, PREROUTING, _table, mangle_table , match, _mac, mac_src, tmp_args3, str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
//printf("\r\n command=[%s],__[%s-%u]\r\n",command,__FILE__,__LINE__);	
							system(command);						
						}
				#ifdef CONFIG_IPV6
						else if(entry.mode & QOS_RESTRICT_IPV6){							
							//ip6tables -A PREROUTING -t mangle -s 2001::1 -j MARK --set-mark 13
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s ", 
							Ip6tables, ADD, PREROUTING, _table, mangle_table , in, br_interface, _src, entry.ip6_src, jump, MARK, set_mark, tmp_args);
							system(command);
						}
				#endif
						else //any
						{
							//iptables -A PREROUTING -t mangle -j MARK --set-mark 13
							//RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, mangle_table , str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s", Iptables, ADD, PREROUTING, _table, mangle_table , str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
//printf("\r\n command=[%s],__[%s-%u]\r\n",command,__FILE__,__LINE__);							
							system(command);
						}

						sprintf(tmp_args1, "2:%d", wan_pkt_mark);
						sprintf(tmp_args2, "%ldkbit", entry.bandwidth);
						sprintf(tmp_args3, "%dkbit", uplink_speed);
						sprintf(tmp_args4, "1%d:", wan_pkt_mark);
						if(entry.mode & QOS_RESTRICT_MIN){//if(entry.mode == 5 || entry.mode == 9){
							RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:1", _classid, tmp_args1, _htb, _rate, tmp_args2, _ceil, tmp_args3, _prio, "2",  _quantum, "30000",NULL_STR);
							//TC_CMD="tc class add dev $WAN parent 2:1 classid 2:$wan_pkt_mark htb rate ${bandwidth}kbit ceil ${UPLINK_SPEED}kbit prio 2 quantum 30000"
						}else{
							RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface, _parent, "2:1", _classid, tmp_args1, _htb, _rate, "1kbit", _ceil, tmp_args2, _prio, "2" , _quantum, "30000", NULL_STR);
							//TC_CMD="tc class add dev $WAN parent 2:1 classid 2:$wan_pkt_mark htb rate 1kbit ceil ${bandwidth}kbit prio 2 quantum 30000"
						}

						RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface, _parent, tmp_args1, _handle, tmp_args4, _sfq, _perturb, "10", NULL_STR);
						//TC_CMD="tc qdisc add dev $WAN parent 2:$wan_pkt_mark handle 1$wan_pkt_mark: sfq perturb 10"

						RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, pInterface, _parent, "2:0", _protocol2, "all", _prio, "100", _handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
						//TC_CMD="tc filter add dev $WAN parent 2:0 protocol ip prio 100 handle $wan_pkt_mark fw classid 2:$wan_pkt_mark"

#if 1
						sprintf(tmp_args1, "3:%d", wan_pkt_mark);
						sprintf(tmp_args2, "%ldkbit", entry.bandwidth);
						sprintf(tmp_args3, "%dkbit", uplink_speed);
						sprintf(tmp_args4, "3%d:", wan_pkt_mark);
						if((pInterface2!=NULL)&&strcmp(pInterface2, "ppp0")==0)//wantype is l2tp
						{
							if(entry.mode & QOS_RESTRICT_MIN){//if(entry.mode == 5 || entry.mode == 9){
								RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:1", _classid, tmp_args1, _htb, _rate, tmp_args2, _ceil, tmp_args3, _prio, "2",  _quantum, "30000",NULL_STR);
								//TC_CMD="tc class add dev $WAN2 parent 3:1 classid 3:$wan_pkt_mark htb rate ${bandwidth}kbit ceil ${UPLINK_SPEED}kbit prio 2 quantum 30000"
							}else{
								RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, pInterface2, _parent, "3:1", _classid, tmp_args1, _htb, _rate, "1kbit", _ceil, tmp_args2, _prio, "2" , _quantum, "30000", NULL_STR);
								//TC_CMD="tc class add dev $WAN2 parent 3:1 classid 3:$wan_pkt_mark htb rate 1kbit ceil ${bandwidth}kbit prio 2 quantum 30000"
							}

							RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, pInterface2, _parent, tmp_args1, _handle, tmp_args4, _sfq, _perturb, "10", NULL_STR);
							//TC_CMD="tc qdisc add dev $WAN2 parent 3:$wan_pkt_mark handle 3$wan_pkt_mark: sfq perturb 10"

							RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, pInterface2, _parent, "3:0", _protocol2, "all", _prio, "100", _handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
							//TC_CMD="tc filter add dev $WAN2 parent 3:0 protocol ip prio 100 handle $wan_pkt_mark fw classid 3:$wan_pkt_mark"
						}
#endif

						wan_pkt_mark = wan_pkt_mark+1;
					}
#ifdef MULTI_PPPOE
						if(needSetOnce){
#endif

					if(entry.bandwidth_downlink > 0){/*DOWNlink*/
						sprintf(tmp_args, "%d", lan_pkt_mark);
						if(entry.mode & QOS_RESTRICT_IP){//if(entry.mode == 5 || entry.mode == 6){
							/*this qos rule is set by IP address*/
							tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_start));
							sprintf(tmp_args1, "%s", tmpStr);
							tmpStr = inet_ntoa(*((struct in_addr *)entry.local_ip_end));
							sprintf(tmp_args2, "%s", tmpStr);
							sprintf(tmp_args3, "%s-%s",tmp_args1, tmp_args2);

							//RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, mangle_table , match, ip_range, dst_rnage, tmp_args3, jump, MARK,  set_mark, tmp_args, NULL_STR);							
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s", Iptables, ADD, POSTROUTING, _table, mangle_table , match, ip_range, dst_rnage, tmp_args3, str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
//printf("\r\n command=[%s],__[%s-%u]\r\n",command,__FILE__,__LINE__);							
							system(command);
						}
						else if(entry.mode & QOS_RESTRICT_MAC){
						#ifndef QOS_MAC_U32_FILTER	
							sprintf(tmp_args3, "%02x:%02x:%02x:%02x:%02x:%02x",entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
							//RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, mangle_table , match, _mac, mac_dst, tmp_args3, jump, MARK, set_mark, tmp_args, NULL_STR);
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s", Iptables, ADD, POSTROUTING, _table, mangle_table , match, _mac, mac_dst, tmp_args3, str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
//printf("\r\n command=[%s],__[%s-%u]\r\n",command,__FILE__,__LINE__);	
							system(command);
						#else
							sprintf(macAddr, "%02x:%02x:%02x:%02x:%02x:%02x",entry.mac[0], entry.mac[1], entry.mac[2], entry.mac[3], entry.mac[4], entry.mac[5]);
						#endif						
						}						
		#ifdef CONFIG_IPV6
						else if(entry.mode & QOS_RESTRICT_IPV6){							
							//ip6tables -A PREROUTING -t mangle -s 2001::1 -j MARK --set-mark 13
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s", 
							Ip6tables, ADD, POSTROUTING, _table, mangle_table , out, br_interface, _dest, entry.ip6_src, jump, MARK, set_mark, tmp_args);
							system(command);
						}
		#endif
						else
						{
							sprintf(command,"%s %s %s %s %s %s %s %s %s %s", Iptables, ADD, POSTROUTING, _table, mangle_table , str_l7_filter, jump, MARK, set_mark, tmp_args, NULL_STR);
//printf("\r\n command=[%s],__[%s-%u]\r\n",command,__FILE__,__LINE__);							
							system(command);							
						}

						sprintf(tmp_args1, "5:%d", lan_pkt_mark);
						sprintf(tmp_args2, "%ldkbit", entry.bandwidth_downlink);
						sprintf(tmp_args3, "%dkbit", downlink_speed);
						sprintf(tmp_args4, "5%d:", lan_pkt_mark);


						if(entry.mode & QOS_RESTRICT_MIN){//if(entry.mode == 5 || entry.mode == 9){
							RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:1", _classid, tmp_args1, _htb, _rate, tmp_args2, _ceil, tmp_args3, _prio, "2", _quantum, "30000",NULL_STR);
							//TC_CMD="tc class add dev $BRIDGE parent 5:1 classid 5:$lan_pkt_mark htb rate ${bandwidth_dl}kbit ceil ${DOWNLINK_SPEED}kbit prio 2 quantum 30000"
						}else{
							RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, br_interface, _parent, "5:1", _classid, tmp_args1, _htb, _rate, "1kbit", _ceil, tmp_args2, _prio, "2" ,_quantum, "30000", NULL_STR);
							//TC_CMD="tc class add dev $BRIDGE parent 5:1 classid 5:$lan_pkt_mark htb rate 1kbit ceil ${bandwidth_dl}kbit prio 2 quantum 30000"
						}
						RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, br_interface, _parent, tmp_args1, _handle, tmp_args4, _sfq, _perturb, "10", NULL_STR);
						//TC_CMD="tc qdisc add dev $BRIDGE parent 5:$lan_pkt_mark handle 5$lan_pkt_mark: sfq perturb 10"
						#ifndef QOS_MAC_U32_FILTER
						RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, br_interface, _parent, "5:0", _protocol2, "all", _prio, "100", _handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
						//TC_CMD="tc filter add dev $BRIDGE parent 5:0 protocol ip prio 100 handle $lan_pkt_mark fw classid 5:$lan_pkt_mark"
						#else
						if(entry.mode & QOS_RESTRICT_MAC)
							RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, br_interface, _parent, "5:0", _protocol2, _ip, _prio, "100", "u32","match", "ether", "dst", macAddr,_classid, tmp_args1, NULL_STR);
						else	
							RunSystemCmd(NULL_FILE, _tc, _filter, _add, _dev, br_interface, _parent, "5:0", _protocol2, _ip, _prio, "100", _handle, tmp_args, _fw, _classid, tmp_args1, NULL_STR);
						//TC_CMD="tc filter add dev $WAN2 parent 3:0 protocol ip prio 100 handle $wan_pkt_mark fw classid 3:$wan_pkt_mark"
						#endif	
						lan_pkt_mark = lan_pkt_mark+1;
					}
#ifdef MULTI_PPPOE
				  }
#endif					
				}
			}
		}
	}

//	RunSystemCmd("/proc/qos", "echo", PROC_QOS, NULL_STR);
#endif
	return 0;
}
#endif

int setURLFilter(void)
{
	char keywords[500];
	char cmdBuffer[500];
	char macAddr[30];
	char tmp1[64]={0};
	URLFILTER_T entry;
	int entryNum=0, index;
	int mode,i=0;
	char c = 22;	//unseen char to distinguish
	//printf("set urlfilter\n");
	/*add URL filter Mode 0:Black list 1:White list*/
	apmib_get(MIB_URLFILTER_MODE,  (void *)&mode);
	apmib_get(MIB_URLFILTER_TBL_NUM, (void *)&entryNum);
	//sprintf(keywords, "%d ", entryNum);
	bzero(keywords,sizeof(keywords));
	for (index=1; index<=entryNum; index++) {
		memset(&entry, '\0', sizeof(entry));
		bzero(tmp1,sizeof(tmp1));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_URLFILTER_TBL, (void *)&entry);
		if(mode!=entry.ruleMode)
			continue;
		strcpy(tmp1,entry.urlAddr);
		if(!strncmp(tmp1,"http://",7))
			for(i=7;i<sizeof(tmp1);i++)
				tmp1[i-7]=tmp1[i];
	//printf("%s:%d tmp1=%s\n",__FUNCTION__,__LINE__,tmp1);
		if(!strncmp(tmp1,"www.",4))
			for(i=4;i<sizeof(tmp1);i++)
				tmp1[i-4]=tmp1[i];
	
	//printf("%s:%d entryNum=%d\n",__FUNCTION__,__LINE__,entryNum);
		if(changeDividerToESC(tmp1,sizeof(tmp1)," #:\\")<0)
			return -1;
		//printf("%s:%d tmp1=%s\n",__FUNCTION__,__LINE__,tmp1);
#ifdef URL_FILTER_USER_MODE_SUPPORT
		if(entry.usrMode==0)
			sprintf(tmp1,"%s 0;",tmp1);
		else if(entry.usrMode==1)
			sprintf(tmp1, "%s I%x;", tmp1,*((struct in_addr *)entry.ipAddr));
		else if(entry.usrMode==2){
			sprintf(macAddr,"%02X%02X%02X%02X%02X%02X", entry.macAddr[0], entry.macAddr[1], entry.macAddr[2], entry.macAddr[3], entry.macAddr[4], entry.macAddr[5]);
			sprintf(tmp1, "%s M%s;", tmp1,macAddr);
		}
#else
		sprintf(tmp1, "%s ;", tmp1);
#endif
		//printf("%s:%d tmp1=%s\n",__FUNCTION__,__LINE__,tmp1);
#if defined(CONFIG_RTL_FAST_FILTER)
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
		sprintf(cmdBuffer, "rtk_cmd filter add --url-key %s", tmp1);
		system(cmdBuffer);
#else
		strcat(keywords, tmp1);
#endif
	}
		
	if(mode)
		RunSystemCmd("/proc/filter_table", "echo", "white", NULL_STR);
	else
		RunSystemCmd("/proc/filter_table", "echo", "black", NULL_STR);
	//sprintf(cmdBuffer, "%s", keywords);
	//RunSystemCmd("/proc/url_filter", "echo", cmdBuffer, NULL_STR);//disable h/w nat when url filter enabled
#if defined(CONFIG_RTL_FAST_FILTER)
#else

	sprintf(cmdBuffer, "add:0#3 3 %s",keywords);
	//printf("%s:%d cmdBuffer=%s\n",__FUNCTION__,__LINE__,cmdBuffer);
	//sleep(1);

	RunSystemCmd("/proc/filter_table", "echo", cmdBuffer, NULL_STR);
#endif

	return 0;
}


int setDoS(unsigned long enabled, int op)
{
	char cmdBuffer[500];
	unsigned int *dst, *mask;
	unsigned int synsynflood=0;
	unsigned int sysfinflood=0;
	unsigned int sysudpflood=0;
	unsigned int sysicmpflood=0;
	unsigned int pipsynflood=0;
	unsigned int pipfinflood=0;
	unsigned int pipudpflood=0;
	unsigned int pipicmpflood=0;
	unsigned int blockTime=0;
	struct in_addr curIpAddr={0}, curSubnet={0};

	apmib_get(MIB_DOS_SYSSYN_FLOOD, (void *)&synsynflood);
	apmib_get(MIB_DOS_SYSFIN_FLOOD, (void *)&sysfinflood);
	apmib_get(MIB_DOS_SYSUDP_FLOOD, (void *)&sysudpflood);
	apmib_get(MIB_DOS_SYSICMP_FLOOD, (void *)&sysicmpflood);
	apmib_get(MIB_DOS_PIPSYN_FLOOD, (void *)&pipsynflood);
	apmib_get(MIB_DOS_PIPFIN_FLOOD, (void *)&pipfinflood);
	apmib_get(MIB_DOS_PIPUDP_FLOOD, (void *)&pipudpflood);
	apmib_get(MIB_DOS_PIPICMP_FLOOD, (void *)&pipicmpflood);
	apmib_get(MIB_DOS_BLOCK_TIME, (void *)&blockTime);

	getInAddr("br0", IP_ADDR_T, (void *)&curIpAddr);
    getInAddr("br0", NET_MASK_T, (void *)&curSubnet);
  	//apmib_get(MIB_IP_ADDR,  (void *)ipbuf);
  	dst = (unsigned int *)&curIpAddr;
  	//apmib_get( MIB_SUBNET_MASK,  (void *)maskbuf);
  	mask = (unsigned int *)&curSubnet;
  	if(op==2){
  		sprintf(cmdBuffer, "echo \" 2 %X %X %ld %d %d %d %d %d %d %d %d %d\" >  /proc/enable_dos", htonl(*dst), htonl(*mask), enabled, synsynflood, sysfinflood, sysudpflood, sysicmpflood, pipsynflood, pipfinflood, pipudpflood, pipicmpflood, blockTime);
  		  system(cmdBuffer);
  	}else{
  		sprintf(cmdBuffer, "echo \" 0 %X %X %ld %d %d %d %d %d %d %d %d %d\" >  /proc/enable_dos", (htonl(*dst) & 0xFFFFFF00), htonl(*mask), enabled, synsynflood, sysfinflood, sysudpflood, sysicmpflood, pipsynflood, pipfinflood, pipudpflood, pipicmpflood, blockTime);
  		  system(cmdBuffer);
	}
return 0;


}

int setIpFilter(void)
{
	int entryNum=0, index;
	IPFILTER_T entry;
	char ipAddr[30];
	char *tmpStr;
#if defined(CONFIG_RTL_FAST_FILTER)
	char protocol[10];
	char cmdBuffer[120];
#endif

	apmib_get(MIB_IPFILTER_TBL_NUM, (void *)&entryNum);

	for(index=1; index <= entryNum ; index++) {
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_IPFILTER_TBL, (void *)&entry);

		tmpStr = inet_ntoa(*((struct in_addr *)entry.ipAddr));
		sprintf(ipAddr, "%s", tmpStr);
#if defined(CONFIG_RTL_FAST_FILTER)
		memset(protocol, 0, sizeof(protocol));
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
		if(entry.protoType==PROTO_TCP){
			sprintf(protocol, "tcp");
		}
		else if(entry.protoType==PROTO_UDP){
			sprintf(protocol, "udp");
		}
		else if(entry.protoType==PROTO_BOTH)	{
			sprintf(protocol, "tcp_udp");
		}
		sprintf(cmdBuffer, "rtk_cmd filter add --ip-src %s --protocol %s", ipAddr, protocol);
		system(cmdBuffer);
#else

#ifdef CONFIG_IPV6
		if(entry.ipVer==IPv4){
#endif	
		if(entry.protoType==PROTO_TCP){
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _tcp, _src, ipAddr, jump, DROP, NULL_STR);
		}
		if(entry.protoType==PROTO_UDP){
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, _src, ipAddr, jump, DROP, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp, dport, "53:53", _src, ipAddr, jump, DROP, NULL_STR);
		}
		if(entry.protoType==PROTO_BOTH)	{
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _tcp, _src, ipAddr, jump, DROP, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, _src, ipAddr, jump, DROP, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp, dport, "53:53", _src, ipAddr, jump, DROP, NULL_STR);
		}
#ifdef CONFIG_IPV6
		}
		else if(entry.ipVer==IPv6){
			if(entry.protoType==PROTO_TCP){
				RunSystemCmd(NULL_FILE, Ip6tables, ADD, FORWARD, _protocol, _tcp, _src, entry.ip6Addr, jump, DROP, NULL_STR);
			}
			else if(entry.protoType==PROTO_UDP){
				RunSystemCmd(NULL_FILE, Ip6tables, ADD, FORWARD, _protocol, _udp, _src, entry.ip6Addr, jump, DROP, NULL_STR);
				RunSystemCmd(NULL_FILE, Ip6tables, ADD, INPUT, _protocol, _udp, dport, "53", _src, ipAddr, jump, DROP, NULL_STR);
			}
			else if(entry.protoType==PROTO_BOTH){
				RunSystemCmd(NULL_FILE, Ip6tables, ADD, FORWARD, _protocol, _tcp, _src, entry.ip6Addr, jump, DROP, NULL_STR);
				RunSystemCmd(NULL_FILE, Ip6tables, ADD, FORWARD, _protocol, _udp, _src, entry.ip6Addr, jump, DROP, NULL_STR);
				RunSystemCmd(NULL_FILE, Ip6tables, ADD, INPUT, _protocol, _udp, dport, "53", _src, entry.ip6Addr, jump, DROP, NULL_STR);
			}
		}
#endif	

#endif

	}
	return 0;
}

int setMACFilter(void)
{
	char macEntry[30];
	int entryNum=0, index;
	MACFILTER_T entry;
	char cmdBuffer[80];

	apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&entryNum);

	for (index=1; index<=entryNum; index++) {
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_MACFILTER_TBL, (void *)&entry);
		sprintf(macEntry,"%02X:%02X:%02X:%02X:%02X:%02X", entry.macAddr[0], entry.macAddr[1], entry.macAddr[2], entry.macAddr[3], entry.macAddr[4], entry.macAddr[5]);
#if defined(CONFIG_RTL_FAST_FILTER)
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
		sprintf(cmdBuffer, "rtk_cmd filter add --mac-src %s", macEntry);
		system(cmdBuffer);
#else
#if defined(CONFIG_APP_EBTABLES)&&defined(CONFIG_EBTABLES_KERNEL_SUPPORT)
		RunSystemCmd(NULL_FILE, Ebtables, ADD, INPUT,_src, macEntry, jump, DROP,NULL_STR);	
		RunSystemCmd(NULL_FILE, Ebtables, ADD, OUTPUT,_dest, macEntry, jump, DROP,NULL_STR);	
#else
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, match, "mac" ,mac_src, macEntry, jump, DROP, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, match, "mac" ,mac_src, macEntry, jump, DROP, NULL_STR);
#endif		
#endif
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
		sprintf(cmdBuffer, "rtk_cmd igmp_delete %02X:%02X:%02X:%02X:%02X:%02X", entry.macAddr[0], entry.macAddr[1], entry.macAddr[2], entry.macAddr[3], entry.macAddr[4], entry.macAddr[5]);
		system(cmdBuffer);
	}

	return 0;

}
/////////////////////////////////////
#ifdef GET_LAN_DEV_INFO_SUPPORT
static inline int
iw_get_ext(int                  skfd,           /* Socket to the kernel */
           char *               ifname,         /* Device name */
           int                  request,        /* WE ID */
           struct iwreq *       pwrq)           /* Fixed part of the request */
{
  /* Set device name */
  strncpy(pwrq->ifr_name, ifname, IFNAMSIZ);
  /* Do the request */
  return(ioctl(skfd, request, pwrq));
}

int getWlStaInfo( char *interface,  WLAN_STA_INFO_Tp pInfo )
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)pInfo;
    wrq.u.data.length = sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1);
    memset(pInfo, 0, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSTAINFO, &wrq) < 0){
    	close( skfd );
		return -1;
	}
    close( skfd );
#else
    return -1;
#endif
    return 0;
}

int getPid(char *filename)
{
	struct stat status;
	char buff[100];
	FILE *fp;

	if ( stat(filename, &status) < 0)
		return -1;
	fp = fopen(filename, "r");
	if (!fp) {
        	fprintf(stderr, "Read pid file error!\n");
		return -1;
   	}
	fgets(buff, 100, fp);
	fclose(fp);

	return (atoi(buff));
}

static int __inline__ string_to_hex(char *string, unsigned char *key, int len)
{
	char tmpBuf[4];
	int idx, ii=0;
	for (idx=0; idx<len; idx+=2) {
		tmpBuf[0] = string[idx];
		tmpBuf[1] = string[idx+1];
		tmpBuf[2] = 0;
		if ( !_is_hex(tmpBuf[0]) || !_is_hex(tmpBuf[1]))
			return 0;

		key[ii++] = (unsigned char) strtol(tmpBuf, (char**)NULL, 16);
	}
	return 1;
}

/*
  *@name rtk_get_device_brand
  *@ input 
     mac , the pointer of lan device mac address 
     mac_file , contains the prefix mac and brand list, such as "/etc/device_mac_brand.txt"
  *@output
     brand ,  hold the brand of device, such as Apple, Samsung, Xiaomi, Nokia, Huawei, etc.
  *@ return value
  	RTK_SUCCESS
  	RTK_FAILED
  *
  */
int rtk_get_device_brand(unsigned char *mac, char *mac_file, char *brand)
{		
	FILE *fp;
	int index;
	unsigned char prefix_mac[16], mac_brand[64];
	char *pchar;
	int found=0;
	if(mac==NULL || mac_file==NULL || brand==NULL)
		return -1;
	if((fp= fopen(mac_file, "r"))==NULL)
		return -1;

	sprintf(prefix_mac, "%02X-%02X-%02X", mac[0], mac[1], mac[2]);

	for(index = 0 ; index < 8; ++index)
	{
		if((prefix_mac[index]  >= 'a')  && (prefix_mac[index]<='f'))
			prefix_mac[index] -= 32;
	}

	//printf("%s.%d. str(%s)\n",__FUNCTION__,__LINE__,prefix_mac);

	while(fgets(mac_brand, sizeof(mac_brand), fp))
	{			
		mac_brand[strlen(mac_brand)-1]='\0';		
		if((pchar=strstr(mac_brand, prefix_mac))!=NULL)
		{
			pchar+=9;
			strcpy(brand, pchar);
			found=1;
			break;
		}
	}
	fclose(fp);
	
	if(found==1)
		return 0;
	
	return -1;
}

int get_info_from_l2_tab(char *filename, rtk_l2Info l2list[])
{
	FILE *fp;
	char line_buffer[512];	
	char mac_str[13];
	unsigned char mac_addr[6];
	int idx=0, i, j;	
	char *pchar, *pstart;
	
	unsigned char br0_mac[6];
	unsigned char br0_mac_str[32];
	
	if(filename==NULL)
		return -1; 
	if((fp= fopen(filename, "r"))==NULL)
		return -1;
	
	memset(br0_mac,0,6);
	apmib_get(MIB_ELAN_MAC_ADDR,  (void *)br0_mac);
	if(!memcmp(br0_mac, "\x00\x00\x00\x00\x00\x00", 6))
		apmib_get(MIB_HW_NIC0_ADDR,  (void *)br0_mac);
	
	sprintf(br0_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", br0_mac[0], br0_mac[1], br0_mac[2], br0_mac[3], br0_mac[4], br0_mac[5]);
	br0_mac_str[strlen("ff:ff:ff:ff:ff:ff")]='\0';
	
	while(fgets(line_buffer, sizeof(line_buffer), fp))
	{			
		line_buffer[strlen(line_buffer)-1]='\0';

		if(strstr(line_buffer, "ff:ff:ff:ff:ff:ff") || strstr(line_buffer, "CPU") || strstr(line_buffer, "FID:1") || strstr(line_buffer, br0_mac_str))
			continue;	
		
		pchar=strchr(line_buffer, ':');
		pstart=pchar-2;
		for(i=0, j=0; i<17 && j<12; i++)
		{
			if(pstart[i]!=':')
			{
				mac_str[j++]=pstart[i];
			}
		}
		mac_str[j]=0;
		if (strlen(mac_str)==12 && string_to_hex(mac_str, mac_addr, 12)) 
		{
			memcpy(l2list[idx].mac, mac_addr, 6);
			
			pchar=strstr(line_buffer,"mbr");
			sscanf(pchar,"mbr(%d",&(l2list[idx].portNum));
			
			idx++;
            if(idx >= MAX_L2_LIST_NUM)
                break;
		}		
	}
	fclose(fp);
	return idx;		
}


int get_arp_table_list(char *filename, RTK_ARP_ENTRY_Tp parplist)
{
	FILE *fp;
	char line_buffer[512];	
	char mac_str[13], tmp_mac_str[18];
	char ip_str[16], if_name[16];
	unsigned char mac_addr[6];
	int idx=0, i, j;	
	char *pchar, *pstart, *pend;
	struct in_addr ip_addr;

	if(filename==NULL || parplist==NULL)
		return -1; 
	if((fp= fopen(filename, "r"))==NULL)
		return -1;
	
	while(fgets(line_buffer, sizeof(line_buffer), fp))
	{			
		line_buffer[strlen(line_buffer)-1]='\0';		

		sscanf(line_buffer,"%s %*s %*s %s %*s %s",ip_str,tmp_mac_str,if_name);
		if(strcmp(if_name, "br0")!=0)
			continue;

		inet_aton(ip_str, &ip_addr);
		parplist[idx].ip=ip_addr.s_addr;
		
		for(i=0, j=0; i<17 && j<12; i++)
		{
			if(tmp_mac_str[i]!=':')
			{
				mac_str[j++]=tmp_mac_str[i];
			}
		}
		mac_str[12]=0;			
			
		if (strlen(mac_str)==12 && string_to_hex(mac_str, mac_addr, 12)) 
		{
			memcpy(parplist[idx].mac, mac_addr, 6);
			idx++;
		}		
	}
	fclose(fp);
	return idx;		
}

static int getDhcpClient(char **ppStart, unsigned long *size, unsigned char *hname, unsigned int *ip, unsigned char *mac, unsigned int *lease)
{
	struct dhcpOfferedAddr 
	{
		unsigned char chaddr[16];
		unsigned int yiaddr;       /* network order */
		unsigned int expires;      /* host order */
		unsigned int linktime; /* link time */
//#if defined(CONFIG_RTL8186_KB) || defined(CONFIG_RTL8186_TR) || defined(CONFIG_RTL865X_SC) || defined(CONFIG_RTL865X_AC) || defined(CONFIG_RTL865X_KLD)
		char hostname[64]; /* Brad add for get hostname of client */
		u_int32_t isUnAvailableCurr;	/* Brad add for WEB GUI check */
//#endif
	};

	struct dhcpOfferedAddr entry;
	unsigned char empty_haddr[16]; 

	memset(empty_haddr, 0, 16); 
	//printf("%s:%d size=%d\n",__FUNCTION__,__LINE__,*size);
	if ( *size < sizeof(entry) )
		return -1;

	entry = *((struct dhcpOfferedAddr *)*ppStart);
	*ppStart = *ppStart + sizeof(entry);
	*size = *size - sizeof(entry);
	//printf("%s:%d expires=%d\n",__FUNCTION__,__LINE__,entry.expires);

	if (entry.expires == 0)
		return 0;
	//printf("%s:%d\n",__FUNCTION__,__LINE__);

	if(!memcmp(entry.chaddr, empty_haddr, 16))
		return 0;

	//strcpy(ip, inet_ntoa(*((struct in_addr *)&entry.yiaddr)) );
	*ip=entry.yiaddr;
	memcpy(mac, entry.chaddr, 6);
	
	//snprintf(mac, 20, "%02x:%02x:%02x:%02x:%02x:%02x",entry.chaddr[0],
	//	entry.chaddr[1],entry.chaddr[2],entry.chaddr[3],entry.chaddr[4], entry.chaddr[5]);
	//if(entry.expires == 0xffffffff)
	//	sprintf(liveTime,"%s", "Always");
	//else
	//	snprintf(liveTime, 10, "%lu", (unsigned long)ntohl(entry.expires));
	*lease=entry.expires;
	
	if(entry.hostname[0])
		strcpy(hname, entry.hostname);
	
	return 1;
}

/*
  *@name rtk_get_dhcp_client_list
  *@ input 
     rtk_dhcp_client_info *, the pointer of lan dhcp client list which specific every client info, such as host name, ip, mac, lease time 
  *@output
     num , unsigned int *, which hold the num of dhcp client.
  *@ return value
  	RTK_SUCCESS
  	RTK_FAILED
  *
  */
int rtk_get_dhcp_client_list(unsigned int *num, struct rtk_dhcp_client_info *pclient)
{	
	FILE *fp;
	int idx=0, ret;
	char *buf=NULL, *ptr, tmpBuf[100];
	unsigned int ip, lease;
	unsigned char mac[6], hostname[64]={0};

	struct stat status;
	int pid;
	unsigned long fileSize=0;
	// siganl DHCP server to update lease file
	pid = getPid(_PATH_DHCPS_PID);
	snprintf(tmpBuf, 100, "kill -SIGUSR1 %d\n", pid);

	if ( pid > 0)
		system(tmpBuf);

	usleep(1000);

	if ( stat(_PATH_DHCPS_LEASES, &status) < 0 )
		goto err;

	fileSize=status.st_size;
	buf = malloc(fileSize);
	if ( buf == NULL )
		goto err;
	fp = fopen(_PATH_DHCPS_LEASES, "r");
	if ( fp == NULL )
		goto err;

	fread(buf, 1, fileSize, fp);
	fclose(fp);

	ptr = buf;
	while (1) 
	{
		ret = getDhcpClient(&ptr, &fileSize, hostname, &ip, mac, &lease);
//		printf("%s:%d ret=%d\n",__FUNCTION__,__LINE__,ret);

		if (ret < 0)
			break;
		if (ret == 0)
			continue;

		strcpy(pclient[idx].hostname, hostname);
		pclient[idx].ip=ip;
		memcpy(pclient[idx].mac, mac, 6);
		pclient[idx].expires=lease;
		
//		printf("%s:%d pclient[%d].expires=%d\n",__FUNCTION__,__LINE__,idx,pclient[idx].expires);
	//	if(strcmp(pclient[idx].hostname, "null")==0)
		//	strcpy(pclient[idx].hostname, pclient[idx].brand);
		
		idx++;
		if(idx>=MAX_STA_NUM)
			return -1;
	}
	
err:
	*num=idx;
	if (buf)
		free(buf);
	
	return 0;
}

/********************************************************
** get dst mac index, if not exist, add the mac to (the end of) arrary
*********************************************************/
int getDstMacIdx(RTK_LAN_DEVICE_INFO_Tp pdevinfo,unsigned char mac[6],int max_num)
{
	int i=0;
	char mac_null[6]={0};
	for(i=0;i<max_num;i++)
	{
		if(memcmp(pdevinfo[i].mac,mac,6)==0)
		{
			
			//printf("%s:%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,__LINE__,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
			return i;
		}
		if(memcmp(pdevinfo[i].mac,mac_null,6)==0)
		{
			//printf("%s:%d\n",__FUNCTION__,__LINE__);
			memcpy(pdevinfo[i].mac,mac,6);
			return i;
		}
	}
	return max_num;
}
#endif
/*
        get port status info by proc/rtl865x/asicCounter
*/
void GetPortStatus(int port_number,rtk_asicConterInfo *info)
{
        /*fill cur_rx /cur_tx parememter */
        FILE *fp=NULL;
        int  line_cnt =0;
        unsigned char buffer[128];
        //system("cat /proc/rtl865x/asicCounter  > /var/tmpResult");    

        //if((fp = fopen("/var/tmpResult","r+")) != NULL)
        if((fp = fopen("/proc/rtl865x/asicCounter","r+")) != NULL)
        {
                while(fgets(buffer, 128, fp))
                {
                        line_cnt++;
                        if(line_cnt == 12*port_number+3)        //update receive bytes
                        {
                                sscanf(buffer," Rcv %u ",&(info->rxBytes));
                        }

                        if(line_cnt == 12*port_number+10)       //update send bytes
                        {
                                sscanf(buffer," Snd %u ",&(info->txBytes));
                                fclose(fp);
                                return ;
                        }
                }
        }
        fclose(fp);
}


/*
  *@name rtk_get_lan_device_info
  *@ input 
     pdevinfo , the pointer of lan device info
     MAX_NUM, the max number of lan device, should be MAX_STA_NUM+1
  *@output
     num ,  hold the lan device number
  *@ return value
  	-1:fail
  	0:ok
  *
  */
static int rtk_get_lan_device_info(unsigned int *num, RTK_LAN_DEVICE_INFO_Tp pdevinfo,int max_num)
{	
	int l2_tab_num=0,i=0,wifi_sta_num=0,arp_entry_num=0,dhcp_device_num=0,devInfoIdx=0;
	rtk_l2Info l2list[MAX_L2_LIST_NUM]={0};
	rtk_asicConterInfo asicConInfo={0};
	WLAN_STA_INFO_T wlanStaList[MAX_STA_NUM]={0};
	RTK_ARP_ENTRY_T arp_tab[ARP_TABLE_MAX_NUM]={0};
	struct in_addr lan_addr;
    struct sockaddr hwaddr;
	unsigned char lan_mac[6];
	struct rtk_dhcp_client_info dhcp_client_info[MAX_STA_NUM+1]={0};
	char mac_null[6]={0};
	int devNum=0,ret=0;
	
	if(num==NULL || pdevinfo==NULL || max_num<MAX_STA_NUM)
		return -1;
	bzero(pdevinfo,sizeof(RTK_LAN_DEVICE_INFO_T)*max_num);
	
	getInAddr("br0", IP_ADDR_T, (void *)&lan_addr);
    getInAddr("br0", HW_ADDR_T, (void *)&hwaddr);
    memcpy(lan_mac, hwaddr.sa_data, 6);

//l2 table
    bzero(l2list, sizeof(l2list));
	l2_tab_num=get_info_from_l2_tab("/proc/rtl865x/l2", l2list);
	for(i=0;i<l2_tab_num;i++)
	{//assign all mac in pdevinfo, get mac index. if mac not exist, add it to pdevinfo arrary
	     if(l2list[i].portNum == 2 || l2list[i].portNum == 3)
	     {
    		devInfoIdx=getDstMacIdx(pdevinfo,l2list[i].mac,MAX_STA_NUM);	
    		//printf("%s:%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,__LINE__,maclist[i][0],maclist[i][1],maclist[i][2],maclist[i][3],maclist[i][4],maclist[i][5]);
    		if(devInfoIdx < MAX_STA_NUM)
    		{
        		pdevinfo[devInfoIdx].conType=RTK_ETHERNET;

        		GetPortStatus(l2list[i].portNum,&asicConInfo);
        		pdevinfo[devInfoIdx].tx_bytes=asicConInfo.rxBytes;
        		pdevinfo[devInfoIdx].rx_bytes=asicConInfo.txBytes;	
                pdevinfo[devInfoIdx].rssi = 100;
                pdevinfo[devInfoIdx].rx_speed = 0;
                pdevinfo[devInfoIdx].tx_speed = 0;
    		}
	     }
	}
//	printf("%s:%d \n",__FUNCTION__,__LINE__);
	
//wlan0
	//printf("sizeof maclist=%d\n",sizeof(maclist));
	bzero(wlanStaList,sizeof(wlanStaList));
	getWlStaInfo("wlan0", wlanStaList);
	for(i=0;i<MAX_STA_NUM;i++)
	{
		if(wlanStaList[i].aid && (wlanStaList[i].flag & STA_INFO_FLAG_ASOC))
		{
			devInfoIdx=getDstMacIdx(pdevinfo,wlanStaList[i].addr,MAX_STA_NUM);	
            if(devInfoIdx < MAX_STA_NUM)
		    {
#if defined(CONFIG_RTL_92D_SUPPORT)
			    pdevinfo[devInfoIdx].conType=RTK_WIRELESS_5G;
#else
			    pdevinfo[devInfoIdx].conType=RTK_WIRELESS_2G;
#endif
			    pdevinfo[devInfoIdx].on_link=1;
			    pdevinfo[devInfoIdx].tx_bytes=wlanStaList[i].rx_bytes;
			    pdevinfo[devInfoIdx].rx_bytes=wlanStaList[i].tx_bytes;
                pdevinfo[devInfoIdx].rssi = wlanStaList[i].rssi;
                pdevinfo[devInfoIdx].rx_speed = wlanStaList[i].RxOperaRate;
                pdevinfo[devInfoIdx].tx_speed = wlanStaList[i].txOperaRates;
            }
		}
	}

    bzero(wlanStaList,sizeof(wlanStaList));
	getWlStaInfo("wlan0-va1", wlanStaList);
	for(i=0;i<MAX_STA_NUM;i++)
	{
		if(wlanStaList[i].aid && (wlanStaList[i].flag & STA_INFO_FLAG_ASOC))
		{
			devInfoIdx=getDstMacIdx(pdevinfo,wlanStaList[i].addr,MAX_STA_NUM);	
            if(devInfoIdx < MAX_STA_NUM)
		    {
#if defined(CONFIG_RTL_92D_SUPPORT)
			    pdevinfo[devInfoIdx].conType=RTK_WIRELESS_5G;
#else
			    pdevinfo[devInfoIdx].conType=RTK_WIRELESS_2G;
#endif
			    pdevinfo[devInfoIdx].on_link=1;
			    pdevinfo[devInfoIdx].tx_bytes=wlanStaList[i].rx_bytes;
			    pdevinfo[devInfoIdx].rx_bytes=wlanStaList[i].tx_bytes;
                pdevinfo[devInfoIdx].rssi = wlanStaList[i].rssi;
                pdevinfo[devInfoIdx].rx_speed = wlanStaList[i].RxOperaRate;
                pdevinfo[devInfoIdx].tx_speed = wlanStaList[i].txOperaRates;
            }
		}
	}
	
#if defined(CONFIG_RTL_92D_SUPPORT)
//wlan1
	bzero(wlanStaList,sizeof(wlanStaList));
	getWlStaInfo("wlan1", wlanStaList);
	for(i=0;i<MAX_STA_NUM;i++)
	{
		if(wlanStaList[i].aid && (wlanStaList[i].flag & STA_INFO_FLAG_ASOC))
		{
			devInfoIdx=getDstMacIdx(pdevinfo,wlanStaList[i].addr,MAX_STA_NUM);
            if(devInfoIdx < MAX_STA_NUM)
		    {
    			pdevinfo[devInfoIdx].conType=RTK_WIRELESS_2G;
    			pdevinfo[devInfoIdx].on_link=1;
    			pdevinfo[devInfoIdx].tx_bytes=wlanStaList[i].rx_bytes;
    			pdevinfo[devInfoIdx].rx_bytes=wlanStaList[i].tx_bytes;
                pdevinfo[devInfoIdx].rssi = wlanStaList[i].rssi;
                pdevinfo[devInfoIdx].rx_speed = wlanStaList[i].RxOperaRate;
                pdevinfo[devInfoIdx].tx_speed = wlanStaList[i].txOperaRates;
            }
		}
	}

    bzero(wlanStaList,sizeof(wlanStaList));
	getWlStaInfo("wlan1-va1", wlanStaList);
	for(i=0;i<MAX_STA_NUM;i++)
	{
		if(wlanStaList[i].aid && (wlanStaList[i].flag & STA_INFO_FLAG_ASOC))
		{
			devInfoIdx=getDstMacIdx(pdevinfo,wlanStaList[i].addr,MAX_STA_NUM);
            if(devInfoIdx < MAX_STA_NUM)
		    {
    			pdevinfo[devInfoIdx].conType=RTK_WIRELESS_2G;
    			pdevinfo[devInfoIdx].on_link=1;
    			pdevinfo[devInfoIdx].tx_bytes=wlanStaList[i].rx_bytes;
    			pdevinfo[devInfoIdx].rx_bytes=wlanStaList[i].tx_bytes;
                pdevinfo[devInfoIdx].rssi = wlanStaList[i].rssi;
                pdevinfo[devInfoIdx].rx_speed = wlanStaList[i].RxOperaRate;
                pdevinfo[devInfoIdx].tx_speed = wlanStaList[i].txOperaRates;
            }
		}
	}
#endif

//arp table
	arp_entry_num=get_arp_table_list("/proc/net/arp", arp_tab);
	//printf("%s:%d \n",__FUNCTION__,__LINE__);

	for(i=0;i<arp_entry_num;i++)
	{
		devInfoIdx=getDstMacIdx(pdevinfo,arp_tab[i].mac,MAX_STA_NUM);
		//printf("%s:%d devInfoIdx=%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n",__FUNCTION__,__LINE__,devInfoIdx,
		//arp_tab[i].mac[0],arp_tab[i].mac[1],arp_tab[i].mac[2],arp_tab[i].mac[3],arp_tab[i].mac[4],arp_tab[i].mac[5]);
		if(devInfoIdx < MAX_STA_NUM)
		{
    		pdevinfo[devInfoIdx].ip=arp_tab[i].ip;
    		//printf("%s:%d ip=0x%x\n",__FUNCTION__,__LINE__,pdevinfo[devInfoIdx].ip);
    		if(pdevinfo[devInfoIdx].conType==RTK_ETHERNET)
    		//if(sendArpToCheckDevIsAlive(pdevinfo[devInfoIdx].ip,lan_addr.s_addr, lan_mac)==0)
    			pdevinfo[devInfoIdx].on_link=1;
    		//printf("%s:%d \n",__FUNCTION__,__LINE__);
		}
	}

//dhcp list
	rtk_get_dhcp_client_list(&dhcp_device_num, &dhcp_client_info);
	//printf("%s:%d dhcp_device_num=%d\n",__FUNCTION__,__LINE__,dhcp_device_num);

	for(i=0;i<dhcp_device_num;i++)
	{
		devInfoIdx=getDstMacIdx(pdevinfo,dhcp_client_info[i].mac,MAX_STA_NUM);
        if(devInfoIdx < MAX_STA_NUM)
		{
    		strcpy(pdevinfo[devInfoIdx].hostname,dhcp_client_info[i].hostname);
    		pdevinfo[devInfoIdx].ip=dhcp_client_info[i].ip;
    		pdevinfo[devInfoIdx].expires=dhcp_client_info[i].expires;
        }
	}
	
	
	devNum=getDstMacIdx(pdevinfo,mac_null,MAX_STA_NUM);
	for(i=0;i<devNum;i++)
	{
		if(!pdevinfo[i].hostname[0])
			strcpy(pdevinfo[i].hostname,"---");
		ret=rtk_get_device_brand(pdevinfo[i].mac, _PATH_DEVICE_MAC_BRAND, pdevinfo[i].brand);
		if(ret<0)
			strcpy(pdevinfo[i].brand,"---");
	}

	*num=devNum;
	return 0;
	
}

///////////////////////////////
 static int getParentContrlStatus()
 {
	 int nBytesSent=0, parentEntryNum, i,j;
	 PARENT_CONTRL_T entry;
	 int curentTime;
	 char commandBuf[64]={0};
 
	 time_t rawtime;  
	 struct tm* currentTimeInfo;
	 time (&rawtime);  
	 currentTimeInfo = localtime(&rawtime); 
	 //printf("current local time: %s\n", convertTimeToString(currentTimeInfo)); 
 
	 if ( !apmib_get(MIB_PARENT_CONTRL_TBL_NUM, (void *)&parentEntryNum)) {
		 fprintf(stderr, "Get table entry error!\n");
		 return -1;
	 }
	 
	 for (i=1; i<=parentEntryNum; i++) 
	 {
 
		 *((char *)&entry) = (char)i;
 
		 //  memset(&entry, 0x00, sizeof(entry));
		 if ( !apmib_get(MIB_PARENT_CONTRL_TBL, (void *)&entry))
			 return -1;
		 //printf("------>function_%s_line[%d]: terminalNUm=%d\n",__FUNCTION__,__LINE__,parentEntryNum);
 
		 //printf("\n(---+++++table--%d)tmpMon=%d tmpTues=%d  tmpWed=%d  tmpThur=%d  tmpFri=%d tmpSat=%d  tmpSun=%d  tmpstart=%d  tmpend=%d terminal=%s\n", \
		 i,entry.parentContrlWeekMon,entry.parentContrlWeekTues,entry.parentContrlWeekWed,\
		 entry.parentContrlWeekThur,entry.parentContrlWeekFri,entry.parentContrlWeekSat, \
		 entry.parentContrlWeekSun, entry.parentContrlStartTime,entry.parentContrlEndTime,entry.parentContrlTerminal);
		 //getCurrentTime(currentTimeInfo);
		 curentTime=(currentTimeInfo->tm_hour*60+currentTimeInfo->tm_min);
		 if((currentTimeInfo->tm_wday==(entry.parentContrlWeekMon?MONDAY:WEEK_TIME_DISABLED)) \
		 ||(currentTimeInfo->tm_wday==(entry.parentContrlWeekTues?TUESDAY:WEEK_TIME_DISABLED)) \
		 ||(currentTimeInfo->tm_wday==(entry.parentContrlWeekWed?WEDNESDAY:WEEK_TIME_DISABLED)) \
		 ||(currentTimeInfo->tm_wday==(entry.parentContrlWeekThur?THURSDAY:WEEK_TIME_DISABLED)) \
		 ||(currentTimeInfo->tm_wday==(entry.parentContrlWeekFri?FRIDAY:WEEK_TIME_DISABLED)) \
		 ||(currentTimeInfo->tm_wday==(entry.parentContrlWeekSat?SATDAY:WEEK_TIME_DISABLED)) \
		 ||(currentTimeInfo->tm_wday==(entry.parentContrlWeekSun?SUNDAY:WEEK_TIME_DISABLED))) \
		 {
		  // printf("------>function_%s_line[%d]: parent week is ok \n",__FUNCTION__,__LINE__);
		  if((curentTime>=entry.parentContrlStartTime)&&(curentTime<=entry.parentContrlStartTime))
		  {
		  g_parentControlFlag=SET_PARENT_CONTROL_LIST;
		  g_parentControlListTableNumberFlag=i;
		  }
		  else 
		  {   
			g_parentControlFlag=DELETE_PARENT_CONTROL_LIST;
		    g_parentControlListTableNumberFlag=i;
		  }
		 }
	 }
	 return 0;
 }
 int parentControlSetting(int parentContrlAction,const char* hostname)
 {
	 RTK_LAN_DEVICE_INFO_T devinfo[MAX_STA_NUM] = {0};
	 char cmdBuffer[80];
	 char macEntry[30];
	 int nBytesSent=0;
	 int num = 0;
	 int i=0;	 
	 memset(macEntry,0,sizeof(macEntry));
	 rtk_get_lan_device_info(&num, devinfo, MAX_STA_NUM);
	  
	 for (i=1; i<=num; i++) 
	 {
	   sprintf(macEntry,"%02X:%02X:%02X:%02X:%02X:%02X", devinfo[i-1].mac[0], devinfo[i-1].mac[1], devinfo[i-1].mac[2], devinfo[i-1].mac[3],devinfo[i-1].mac[4],devinfo[i-1].mac[5]);
	   if((strcmp(hostname,devinfo[i-1].hostname)==0)||(strncasecmp(hostname,macEntry,17)==0))
	   {
	 // printf("------>function_%s_line[%d]: mac=%02X:%02X:%02X:%02X:%02X:%02X\n",__FUNCTION__,__LINE__,devinfo[i-1].mac[0], devinfo[i-1].mac[1], devinfo[i-1].mac[2], devinfo[i-1].mac[3],devinfo[i-1].mac[4],devinfo[i-1].mac[5]);
	    //sprintf(macEntry,"%02X:%02X:%02X:%02X:%02X:%02X", devinfo[i-1].mac[0], devinfo[i-1].mac[1], devinfo[i-1].mac[2], devinfo[i-1].mac[3],devinfo[i-1].mac[4],devinfo[i-1].mac[5]);
     if(SET_PARENT_CONTROL_LIST==parentContrlAction)
     {
	 //printf("------>function_%s_line[%d] \n",__FUNCTION__,__LINE__);
#if defined(CONFIG_RTL_FAST_FILTER)
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
	    sprintf(cmdBuffer, "rtk_cmd filter add --mac-src %s", macEntry);
	    system(cmdBuffer);
#else
#if defined(CONFIG_APP_EBTABLES)&&defined(CONFIG_EBTABLES_KERNEL_SUPPORT)
		RunSystemCmd(NULL_FILE, Ebtables, ADD, INPUT,_src, macEntry, jump, DROP,NULL_STR);  
		RunSystemCmd(NULL_FILE, Ebtables, ADD, OUTPUT,_dest, macEntry, jump, DROP,NULL_STR);    
#else
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, match, "mac" ,mac_src, macEntry, jump, DROP, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, match, "mac" ,mac_src, macEntry, jump, DROP, NULL_STR);
#endif		
#endif
		memset(cmdBuffer, 0, sizeof(cmdBuffer));
		sprintf(cmdBuffer, "rtk_cmd igmp_delete %02X:%02X:%02X:%02X:%02X:%02X", devinfo[i-1].mac[0], devinfo[i-1].mac[1],devinfo[i-1].mac[2], devinfo[i-1].mac[3], devinfo[i-1].mac[4], devinfo[i-1].mac[5]);
		system(cmdBuffer);
	   }  
	  if(DELETE_PARENT_CONTROL_LIST==parentContrlAction)
	  {

	// printf("------>function_%s_line[%d] \n",__FUNCTION__,__LINE__);
	   	RunSystemCmd(NULL_FILE, Iptables, INSERT, FORWARD, match, "mac" ,mac_src, macEntry, jump, ACCEPT, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, INSERT, INPUT, match, "mac" ,mac_src, macEntry, jump, ACCEPT, NULL_STR);
	  }
	  
	   }  

	 }
	 //printf("------>function_%s_line[%d] \n",__FUNCTION__,__LINE__);
	 return nBytesSent;
 }

 int parseTerminalMac(int parentContrlAction,const char* hostname)
 {
	 char *p;
	 char *buff;
	 int i=0;
	 buff=hostname;
	

	 p = strsep(&buff, ";");
	while(p)
	{
	 //printf("------>function_%s_line[%d]:hostname[%s] \n",__FUNCTION__,__LINE__,p);
	 parentControlSetting(parentContrlAction,p);
	 p = strsep(&buff, ";");
	 i++;
	}
//	printf("i--->%d",i);
	return 0;
}


int setParentContrl(int parentContrlAction,int parentContrlTableNum )
{
	char macEntry[30];
	int entryNum=0, index;
	PARENT_CONTRL_T entry;
	char cmdBuffer[80];

	apmib_get(MIB_PARENT_CONTRL_TBL_NUM, (void *)&entryNum);

	for (index=1; index<=entryNum; index++) {
		if(index!=parentContrlTableNum)
			continue;
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_PARENT_CONTRL_TBL, (void *)&entry);
	//	 printf("------>function_%s_line[%d]:parentContrlTerminal[%s] \n",__FUNCTION__,__LINE__,entry.parentContrlTerminal);
        parseTerminalMac(parentContrlAction,entry.parentContrlTerminal);				
	}
	return 0;
}



int setPortFilter(void)
{
	char PortRange[30];
	//int DNS_Filter=0;
	int entryNum=0,index;
	PORTFILTER_T entry;
#if defined(CONFIG_RTL_FAST_FILTER)
	char protocol[10];
	char cmdBuffer[120];
#endif
	char cmdName[10];

	apmib_get(MIB_PORTFILTER_TBL_NUM, (void *)&entryNum);
	for (index=1; index<=entryNum; index++) {
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_PORTFILTER_TBL, (void *)&entry);
		sprintf(PortRange, "%d:%d", entry.fromPort, entry.toPort);
#if defined(CONFIG_RTL_FAST_FILTER)
		memset(protocol, 0, sizeof(protocol));
		memset(cmdBuffer, 0, sizeof(cmdBuffer));

		if(entry.protoType==PROTO_TCP){
			sprintf(protocol, "tcp");
		}
		else if(entry.protoType==PROTO_UDP){
			sprintf(protocol, "udp");
		}
		else if(entry.protoType==PROTO_BOTH){
			sprintf(protocol, "tcp_udp");
		}
		sprintf(cmdBuffer, "rtk_cmd filter add --port-range-dst %d:%d --protocol %s", entry.fromPort, entry.toPort, protocol);
		system(cmdBuffer);
#else
#ifdef CONFIG_IPV6
		if(entry.ipVer == IPv6)
			sprintf(cmdName,"%s",Ip6tables);
		else			
#endif
			sprintf(cmdName,"%s",Iptables);

		if(entry.protoType==PROTO_TCP){
			RunSystemCmd(NULL_FILE, cmdName, ADD, FORWARD, _protocol, _tcp, dport, PortRange, jump, DROP, NULL_STR);
		}
				
		if(entry.protoType==PROTO_UDP){
			if(entry.fromPort<53 && entry.toPort >= 53){
				RunSystemCmd(NULL_FILE, cmdName, ADD, INPUT, _protocol, _udp, dport, "53:53", jump, DROP, NULL_STR);
			}
			RunSystemCmd(NULL_FILE, cmdName, ADD, FORWARD, _protocol, _udp, dport, PortRange, jump, DROP, NULL_STR);
		}
		
		if(entry.protoType==PROTO_BOTH)	{
			RunSystemCmd(NULL_FILE, cmdName, ADD, FORWARD, _protocol, _tcp, dport, PortRange, jump, DROP, NULL_STR);
			RunSystemCmd(NULL_FILE, cmdName, ADD, FORWARD, _protocol, _udp, dport, PortRange, jump, DROP, NULL_STR);
			if(entry.fromPort<53 && entry.toPort >= 53){
				RunSystemCmd(NULL_FILE, cmdName, ADD, INPUT, _protocol, _udp, dport, "53:53", jump, DROP, NULL_STR);
			}
		}
#endif
		/*
		if(DNS_Filter==0){
			if(entry.fromPort<= 53 &&  entry.toPort >= 53){
				if(entry.protoType==PROTO_BOTH || (entry.protoType==PROTO_UDP)){
					RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp, dport, "53", jump, DROP, NULL_STR);
				}
			}
			DNS_Filter=1;
		}
		*/

	}
	return 0;
}

#if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
int getlansubnet(char *lansubnet)
{
    
	char *bInterface = "br0";
	int get_br0ip =0;
	int get_br0mask =0;
	//char Br0NetSectAddr[30];
	char * strbr0Ip ,* strbr0Mask ;
	struct in_addr br0addr,br0mask;
	unsigned int numofone ;	
	char NumStr[10]; 

    if (lansubnet == NULL)
        return -1;
    
    get_br0ip = getInAddr(bInterface, IP_ADDR_T,(void *)&br0addr);
    if(get_br0ip ==0 ){
        printf("No ip currently!\n");
        return -1;
    }
    get_br0mask = getInAddr(bInterface, NET_MASK_T,(void *)&br0mask);
    if( get_br0mask ==0 ){
        printf("No MASK currently!\n");
        return -1;
    }
    br0addr.s_addr &= br0mask.s_addr ;
    for(numofone =0;br0mask.s_addr;++numofone)
        br0mask.s_addr &= br0mask.s_addr-1;
    sprintf (NumStr, "%d", numofone);
    strcpy(lansubnet,inet_ntoa(br0addr));
    strcat(lansubnet,"/");
    strcat(lansubnet,NumStr);

    return 0;
}
#endif

int setPortForward(char *pIfaceWan, char *pIpaddrWan)
{
	char PortRange[60];
	char ip[30];
	char *tmpStr;
	int entryNum=0, index;
	PORTFW_T entry;
	int l2tp_vpn=0;
	int pptp_vpn=0;
	int ipsec_vpn=0;
#if defined(CONFIG_RTL_FAST_FILTER)
	char protocol[10];
	char cmdBuffer[120];
#endif
#if defined(CONFIG_RTL_PORTFW_EXTEND)
	unsigned char rmtip[30];
	unsigned char extPortRange[60];	
#endif
	unsigned char tmpbuff[120];

#if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
    char Br0NetSectAddr[30]={0};
    getlansubnet(Br0NetSectAddr);
    //printf("%s %d Br0NetSectAddr=%s\n", __FUNCTION__, __LINE__, Br0NetSectAddr);
#endif

	apmib_get(MIB_PORTFW_TBL_NUM, (void *)&entryNum);
	for (index=1; index<=entryNum; index++) {
		memset(&entry, '\0', sizeof(entry));
		memset((void *)&ip, '\0', sizeof(ip));
		memset((void *)&tmpbuff, '\0', sizeof(tmpbuff));
		
		*((char *)&entry) = (char)index;
		apmib_get(MIB_PORTFW_TBL, (void *)&entry);
		#if defined(CONFIG_RTL_PORTFW_EXTEND)
		if (!entry.enabled)
			continue;
		#endif
		tmpStr =	inet_ntoa(*((struct in_addr *)entry.ipAddr));
		sprintf(ip, "%s", tmpStr);

		if(entry.fromPort == entry.toPort){
			snprintf(PortRange, sizeof(PortRange), "%d", entry.fromPort);
			snprintf(tmpbuff, sizeof(tmpbuff), "%s:%d", ip, entry.fromPort);
		}else{
			snprintf(PortRange, sizeof(PortRange), "%d:%d", entry.fromPort, entry.toPort);
			snprintf(tmpbuff, sizeof(tmpbuff), "%s:%d-%d", ip, entry.fromPort, entry.toPort);
		}

		#if defined(CONFIG_RTL_PORTFW_EXTEND)
		memset((void *)&rmtip, '\0', sizeof(rmtip));
		memset((void *)&extPortRange, '\0', sizeof(extPortRange));		
		
		tmpStr =	inet_ntoa(*((struct in_addr *)entry.rmtipAddr));
		sprintf(rmtip, "%s", tmpStr);		

		if(entry.externelFromPort == entry.externelToPort){
			sprintf(extPortRange, "%d", entry.externelFromPort);
		}else{
			sprintf(extPortRange, "%d:%d", entry.externelFromPort, entry.externelToPort);
		}
		#endif

#if 0 //defined(CONFIG_RTL_FAST_FILTER)
		memset(protocol, 0, sizeof(protocol));
		memset(cmdBuffer, 0, sizeof(cmdBuffer));

		if(entry.protoType ==PROTO_TCP){
			sprintf(protocol, "tcp");
		}
		else if(entry.protoType ==PROTO_UDP){
			sprintf(protocol, "udp");
		}
		else if(entry.protoType ==PROTO_BOTH){
			sprintf(protocol, "tcp_udp");
		}
		sprintf(cmdBuffer, "rtk_cmd filter add --ip-dst %s --port-dst-range %d:%d --protocol %s --policy fastpath", ip, entry.fromPort, entry.toPort, protocol);
		system(cmdBuffer);
#else
		if(entry.fromPort<80 && entry.toPort>80)
			RunSystemCmd(NULL_FILE, Iptables, DEL, INPUT, _protocol, _tcp, dport, "80:80", in, pIfaceWan, _dest, pIpaddrWan, jump, DROP, NULL_STR);

		if(entry.protoType ==PROTO_TCP){
            #if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
            //RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, _src, Br0NetSectAddr, _dest, pIpaddrWan, dport, PortRange, jump, DNAT, "--to-destination", ip, NULL_STR);
            RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, _tcp, _src, Br0NetSectAddr, _dest, ip, dport, PortRange, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
            #endif
			
			#if defined(CONFIG_RTL_PORTFW_EXTEND)
			if (( !strcmp(rmtip, "0.0.0.0")))
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, dport, extPortRange, _dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
			else
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, dport, extPortRange,_src, rmtip, _dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
			#else
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, dport, PortRange,_dest, pIpaddrWan, jump, DNAT, "--to", ip, NULL_STR);			
			#endif
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pIfaceWan, _dest, ip, _protocol, _tcp, dport, PortRange, jump , ACCEPT, NULL_STR);
		}
		if(entry.protoType ==PROTO_UDP){
            #if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
            //RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, _src, Br0NetSectAddr, _dest, pIpaddrWan, dport, PortRange, jump, DNAT, "--to-destination", ip, NULL_STR);    
			#if 0//defined(CONFIG_RTL_PORTFW_EXTEND)
            RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, _udp, _src, Br0NetSectAddr, _dest, ip, dport, extPortRange, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
			#else
            RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, _udp, _src, Br0NetSectAddr, _dest, ip, dport, PortRange, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
			#endif
            #endif
			
			#if defined(CONFIG_RTL_PORTFW_EXTEND)			
			if (( !strcmp(rmtip, "0.0.0.0")))
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport, extPortRange,_dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
			else
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport, extPortRange,_src, rmtip,_dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
			#else
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport, PortRange,_dest, pIpaddrWan, jump, DNAT, "--to", ip, NULL_STR);			
			#endif
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pIfaceWan, _dest, ip, _protocol, _udp, dport, PortRange, jump , ACCEPT, NULL_STR);


		}
		if(entry.protoType ==PROTO_BOTH){
            #if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
            //RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, _src, Br0NetSectAddr, _dest, pIpaddrWan, dport, PortRange, jump, DNAT, "--to-destination", ip, NULL_STR);
            RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, _tcp, _src, Br0NetSectAddr, _dest, ip, dport, PortRange, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
            //RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, _src, Br0NetSectAddr, _dest, pIpaddrWan, dport, PortRange, jump, DNAT, "--to-destination", ip, NULL_STR);
            RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, _udp, _src, Br0NetSectAddr, _dest, ip, dport, PortRange, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
            #endif
			#if defined(CONFIG_RTL_PORTFW_EXTEND)			
			if (( !strcmp(rmtip, "0.0.0.0")))
			{
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, dport, extPortRange, _dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport, extPortRange, _dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
			}
			else
			{
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, dport, extPortRange,_src, rmtip, _dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport, extPortRange,_src, rmtip,_dest, pIpaddrWan, jump, DNAT, "--to", tmpbuff, NULL_STR);
			}
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pIfaceWan, _dest, ip, _protocol, _udp, dport, PortRange, jump , ACCEPT, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pIfaceWan, _dest, ip, _protocol, _tcp, dport, PortRange, jump , ACCEPT, NULL_STR);
			#else
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, dport, PortRange,_dest, pIpaddrWan, jump, DNAT, "--to", ip, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pIfaceWan, _dest, ip, _protocol, _tcp, dport, PortRange, jump , ACCEPT, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport, PortRange,_dest, pIpaddrWan, jump, DNAT, "--to", ip, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pIfaceWan, _dest, ip, _protocol, _udp, dport, PortRange, jump , ACCEPT, NULL_STR);
			#endif

		}
#endif
		if(pptp_vpn==0){
			if(entry.fromPort<= 1723 &&  entry.toPort >= 1723){
				if(entry.protoType==PROTO_BOTH || (entry.protoType==PROTO_TCP)){
                    #if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
                    RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, "47", _src, Br0NetSectAddr, _dest, ip, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
                    #endif
					RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, in, pIfaceWan, _protocol, "47", _dest, pIpaddrWan , jump, DNAT, "--to", ip, NULL_STR);
					RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, "47", in, pIfaceWan, jump, ACCEPT, NULL_STR);
				}
			}

			pptp_vpn=1;
		}

		if(l2tp_vpn==0){
			if(entry.fromPort<= 1701 &&  entry.toPort >= 1701){
				if(entry.protoType==PROTO_BOTH || (entry.protoType==PROTO_UDP)){
						RunSystemCmd("/proc/nat_l2tp", "echo", "0", NULL_STR);
				}
			}
			l2tp_vpn=1;
		}
		if(ipsec_vpn==0){
			if(entry.fromPort<= 500 &&  entry.toPort >= 500){
				if(entry.protoType==PROTO_BOTH || (entry.protoType==PROTO_UDP)){
                    #if defined(CONFIG_RTL_SUPPORT_ACCESS_PORT_FORWARD_FROM_LAN)
                    RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, "esp", _src, Br0NetSectAddr, _dest, ip, jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
                    RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table, _protocol, _udp, _src, Br0NetSectAddr, _dest, ip, dport, "4500", jump, "SNAT", "--to", pIpaddrWan, NULL_STR);
                    #endif
					RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, "esp", _dest, pIpaddrWan, jump, DNAT, "--to", ip, NULL_STR);
					RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _udp, dport,"4500", _dest, pIpaddrWan, jump, DNAT, "--to", ip, NULL_STR);
					RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport,"4500", jump, ACCEPT, NULL_STR);
					RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, "esp", in, pIfaceWan, jump, ACCEPT, NULL_STR);
				}
			}
			ipsec_vpn=1;
		}
	}
	return 0;
}

#if defined (QOS_OF_TR069)
#define LAN_FLAG	0x1
#define WAN_FLAG	0x2
#if 1
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif			   /* max */

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif			   /* min */
#endif

typedef struct {
	char	VendorClassID[256];
	char	VendorClassIDMode[16];
	char	ClientID[256];
	char	ClientIDMode[16];
	char	UserClassID[256];
	char	UserClassIDMode[16];
	char	VendorSpecificInfo[256];
	char	VendorSpecificInfoMode[16];
	
}rtl_dhcp_info;

int32_t get_netmask_length(char* mask)
{
    int32_t masklen=0, i=0;
    uint32_t netmask=0;

    if(mask == NULL)
    {
        return 0;
    }

    struct in_addr ip_addr;
    if( inet_aton(mask, &ip_addr) )
    {
        netmask = ntohl(ip_addr.s_addr);
    }else{
        netmask = 0;
        return 0;
    }
    
    while(0 == (netmask & 0x01) && i<32)
    {
        i++;
        netmask = netmask>>1;
    }
    masklen = 32-i;
    return masklen;
}
int rtl_getQueueFlag(int classQueue)
{
	int entryNum;
	QOSQUEUE_T entry;
	int flag=0;
	apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum);
	int i;

	/* addObject will cause index bigger than entryNum*/
	#if 0	
	if(classQueue>entryNum){
		printf("classQueue:(%d) > queuetblNum(%d)find no queue in queue table\n",classQueue,entryNum);
		goto out;
	}
	#endif
	
    if(classQueue == -1)
    {
        printf("classQueue = -1, find no queue in queue table\n");
        goto out;
    }

	for(i=1; i<=entryNum; i++){
		*((char *)&entry) = (char)i;
		apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry);
		
		if(entry.QueueInstanceNum == classQueue){
			if(strlen(entry.QueueInterface))
			{
				if(strstr(entry.QueueInterface, "WAN"))
					flag =WAN_FLAG;
				else if(strstr(entry.QueueInterface, "LAN"))
					flag =LAN_FLAG;
			}
			else
			{
				flag =LAN_FLAG|WAN_FLAG;
			}

			break;
		}
	}
	
out:
//	printf("flag:%d,[%s]:[%d].\n",flag,__FUNCTION__,__LINE__);
	return flag;
	
}

/*when set class table , need to maintian classificastion order*/
int  add_classifyMIB(QOSCLASS_T entry)
{
	int entryNum=0;
	int index;
	QOSCLASS_T class_entry;
	apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	if(entry.ClassificationOrder)
	{
		if(entry.ClassificationOrder<(entryNum+1) )
		{
			for (index=1; index<=entryNum; index++) 
			{
				memset(&class_entry, '\0', sizeof(class_entry));
				*((char *)&class_entry) = (char)index;
				apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
				if(class_entry.ClassificationOrder >=entry.ClassificationOrder){
					class_entry.ClassificationOrder +=1;
					if ( apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_entry) == 0) {
						printf("Add table entry error!");
						goto setErr;
					}
					
				}	
					
					
			}
		}
	}
	else
	{
		entry.ClassificationOrder =entryNum+1;
	}
	if ( apmib_set(MIB_QOS_CLASS_ADD, (void *)&entry) == 0) {
		printf("Add table entry error!");
		goto setErr;
	}
	return 1;
setErr:
	return 0;
}

int  del_classifyMIB(QOSCLASS_T entry)
{
	int entryNum=0;
	int index;
	QOSCLASS_T class_entry;
	apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	if(entry.ClassificationOrder)
	{
		if(entry.ClassificationOrder<(entryNum+1) )
		{
			for (index=1; index<=entryNum; index++) 
			{
				memset(&class_entry, '\0', sizeof(class_entry));
				*((char *)&class_entry) = (char)index;
				apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
				if(class_entry.ClassificationOrder >=entry.ClassificationOrder)
				{
					class_entry.ClassificationOrder -=1;
					if ( apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_entry) == 0) {
						printf("del table entry error!");
						goto setErr;
					}
					
				}	
					
					
			}

			if ( apmib_set(MIB_QOS_CLASS_DEL, (void *)&entry) == 0) {
				printf("del table entry error!");
				goto setErr;
			}
		}
	}
	
	
	return 1;
setErr:
	return 0;
}

int  mod_classifyMIB(QOSCLASS_T entry,int order)
{
	int entryNum=0;
	int index;
	QOSCLASS_T class_entry;
	int min_order;
	int max_order;
	min_order=min(entry.ClassificationOrder,order);
	max_order=max(entry.ClassificationOrder,order);
	
	apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	if(entry.ClassificationOrder)
	{
		if(entry.ClassificationOrder<(entryNum+1) )
		{
			for (index=1; index<=entryNum; index++) 
			{
				memset(&class_entry, '\0', sizeof(class_entry));
				*((char *)&class_entry) = (char)index;
				apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
				if((class_entry.ClassificationOrder >=min_order)
					&&(class_entry.ClassificationOrder <=max_order)
					)
				{
					if(class_entry.ClassificationOrder < order)
						class_entry.ClassificationOrder +=1;
					else	
						class_entry.ClassificationOrder -=1;
					
					if ( apmib_set(MIB_QOS_CLASS_MOD, (void *)&class_entry) == 0) {
						printf("Add table entry error!");
						goto setErr;
					}
					
				}	
					
					
			}
			
			if ( apmib_set(MIB_QOS_CLASS_MOD, (void *)&entry) == 0) {
				printf("Add table entry error!");
				goto setErr;
			}
			
		}
	}
	
	
	return 1;
setErr:
	return 0;
}

int rtl_find_qid_by_trafficClass(int TrafficClass,int *class_qid)
{
	QOSQUEUE_T entry;
	int flag=0;
	int entryNum=0;
	int index=0;
	int i=0;
	char *strptr;
	char *p=NULL;
	char que_TrafficClasse[256]={0};
	int tmp=-1;
	int ret=0;
	
	apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum);
	for(index=1;index<=entryNum;index++)
	{
		//printf("\n[%d],[%s]:[%d]\n\n",index,__FUNCTION__,__LINE__);
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry);
		if(entry.QueueEnable)
		{
			if(strlen(entry.TrafficClasses))
			{
				memset(que_TrafficClasse,'\0',256);
				strptr=p=NULL;
				tmp =-1;
				strcpy(que_TrafficClasse,entry.TrafficClasses);
				strptr=que_TrafficClasse;
				//printf("[%d] que_TrafficClasse:%s,[%s]:[%d].\n",index,que_TrafficClasse,__FUNCTION__,__LINE__);
				
				p = strsep(&strptr,",");
				while(p)
				{	
					tmp= atoi(p);
					
					//printf("%s,%d =?%d[%s]:[%d]\n",p,tmp,TrafficClass,__FUNCTION__,__LINE__);
					if(TrafficClass==tmp)
					{
						
						class_qid[index]=1;
						ret++;
						
						break;	
					}
					p=strsep(&strptr, ",");
					
				}
					
			}
		}
		
	}
	#if 0
	printf("[%s]:[%d]\n",__FUNCTION__,__LINE__);
	for (index=0;index< MAX_QOS_QUEUE_NUM+1;index++)
	{
		if(class_qid[index]>0)
	 		printf("[%d] %d\n ",index,class_qid[index]);
		
	}
	printf("\n");
	#endif
	return ret;
}

/**************************************************************
*
*	policer process
*
***************************************************************/

static int rtl_check_policer(QOSPOLICER_T *policer_p)
{
	//return 0 means invlalid police
	//check enable status
	if(!policer_p->PolicerEnable) 
		return 0;

	//check supported MeterType , current only SimpleTokenBucket
	if(strcmp(policer_p->MeterType, "SimpleTokenBucket") )
		return 0;

	// check supported ConformingAction ,  NonConformingAction , current only support Null / Drop
	if(strcmp(policer_p->ConformingAction, "Null")  && strcmp(policer_p->ConformingAction, "Drop"))
		return 0;

	if(strcmp(policer_p->NonConformingAction, "Null")  && strcmp(policer_p->NonConformingAction, "Drop"))
		return 0;

	return 1;
	
}

static void mib_to_action(char *mib_action , char *tc_action)
{

	if(!strcmp(mib_action, "Null"))
		sprintf(tc_action,"%s", "continue"); 
	else if(!strcmp(mib_action, "Drop"))
		sprintf(tc_action,"%s", "drop"); 
	else
		sprintf(tc_action,"%s", "continue"); 
}

static int rtl_policer_to_tc(QOSPOLICER_T *policer_p , char *policer_cmd)
{

	char *police_str = "police";
	char action_str[64];
	char rate_srt[64];
	char burst_str[32];
	char  comAction_str[32];
	char  NoncomAction_str[32];

	//map rate string 
	sprintf(rate_srt,"rate %dbps",policer_p->CommittedRate); 

	//map burst string 
	sprintf(burst_str,"burst %d",policer_p->CommittedBurstSize); 

	//action
	mib_to_action(policer_p->ConformingAction,comAction_str);
	mib_to_action(policer_p->NonConformingAction,NoncomAction_str);

	sprintf(action_str,"action %s/%s",comAction_str,NoncomAction_str);	
	
	// final police
	sprintf(policer_cmd,"%s %s %s %s",police_str,rate_srt,burst_str,action_str); //clear police

	//printf("policer_cmd =%s \n",policer_cmd); //mark_dbg
	
}

int  rtl_process_ClassPolicer(int class_policer, char* policer_cmd)
{
	QOSPOLICER_T policer_entry;
	int entryNum=0;
	int ret=0,index,find=0;
	
	//printf("[%s]:[%d]  class_policer = %d .\n",__FUNCTION__,__LINE__,class_policer);

	apmib_get(MIB_QOS_POLICER_TBL_NUM, (void *)&entryNum);	
	/*policer process*/

	//find match plicer instance and it is enable
	for (index=1; index<=entryNum; index++) 
	{
		memset(&policer_entry, '\0', sizeof(policer_entry));
		*((char *)&policer_entry) = (char)index;
		apmib_get(MIB_QOS_POLICER_TBL, (void *)&policer_entry);
		
		if(policer_entry.InstanceNum == class_policer )
			find = 1;
	}

	if(!find)
		goto no_police;

	//check Policer setting is valid for currnet implement
	if(!rtl_check_policer(&policer_entry))
		goto no_police;

	// map policer to tc string
	rtl_policer_to_tc(&policer_entry,policer_cmd);

	return 1;

no_police:
	printf("rtl_process_ClassPolicer : invalid police parameter!\n");
 	sprintf(policer_cmd,""); //clear police
	return 0;
	
}

/**************************************************************
*
*	queue process
*
***************************************************************/
#define SCHEDULE_WFQ	0x1
#define SCHEDULE_SP		0x2
#define SCHEDULE_WRR	0x4

#define DROP_SHEDULE_RED	0x1
#define DROP_SHEDULE_DT		0x2
#define DROP_SHEDULE_WRED	0x4
#define DROP_SHEDULE_BLUE		0x8
int _rtl_queue_preprocess(int maxentryNum,int * qpri,int *upweight,int *downweight,int dft_queId)
{
	QOSQUEUE_T entry;
	int index=0;
	int j=0,i=0,k=0,idflag=0;
	int queueueOrder[MAX_QOS_QUEUE_NUM+1]={-1};
	unsigned int after_Order[MAX_QOS_QUEUE_NUM+1]={-1};
	unsigned int tmp,tmpindex;
	int uplink=0,downlink=0;
	int weight=0;
	int ret=0;
	
	for (index=1; index<=maxentryNum; index++) 
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry);
		
		if(entry.QueueEnable){
			queueueOrder[index]=entry.QueuePrecedence;
			
			//printf("[%d]:%d,[%s]:[%d].\n",index,queueueOrder[index],__FUNCTION__,__LINE__);

			uplink=downlink=0;
			weight=0;

			if(index==dft_queId)
			{
				
				uplink=downlink =1;
			}
			else
			{
				if(strstr(entry.QueueInterface, "WAN")){
					uplink=1;
				}
				 else if(strstr(entry.QueueInterface, "LAN")){
					downlink=1;
		    	}
		        else{
					uplink=downlink =1;
		    	}
			}
			/*schedule : wfq/sp*/
			if(strlen(entry.SchedulerAlgorithm))
			{
				if(strcmp(entry.SchedulerAlgorithm,"WFQ")==0)
				{
					if(entry.QueueWeight)
					{
						weight =entry.QueueWeight;
					}
				}
				
			}
			
			if(uplink)
			{
				*upweight +=weight;
			}
			if(downlink)
			{
				*downweight +=weight;
			}	

		}
		
		after_Order[index]=-1;
		qpri[index]=-1;
	}
	// re-order QueuePrecedence  ODN (Lower numbers imply greater precedence)
	for (index=1; index<=maxentryNum; index++) 
	{
		tmp =0;
		if(queueueOrder[index]!=-1)
		{
			i++;
			for (j=1;j<=maxentryNum;j++)
			{
				idflag=0;
				if(queueueOrder[j]!=-1)
				{
					if(queueueOrder[j]>tmp)
					{
						if((queueueOrder[j]<after_Order[i-1])&&(i>0))
						{
							tmp =queueueOrder[j];
							tmpindex =j;
						}
						else if((queueueOrder[j]==after_Order[i-1])&&(i>0))
						{
							for(k=i-1;k>0;k--)
							{
								if(qpri[k]==j)
								{
									idflag=1;
									break;
								}
							}
							if(idflag==0)
							{
								tmp =queueueOrder[j];
								tmpindex =j;
								break;
							}
							
						}
							
					}
				}
			}
			if(tmp!=0)
			{
				qpri[i]=tmpindex;
				after_Order[i]=tmp;
			}
			else
			{
				
			}
			
		}
		else
		{
			//do nothing
		}
			
	}

	#if 0
	printf("[%s]:[%d]\n",__FUNCTION__,__LINE__);
	for (index=1; index<=maxentryNum; index++) 
	{
		printf("[%d]:%d,%d %d\n",index,queueueOrder[index],after_Order[index],qpri[index]);
	}
	#endif
}
int _rtl_queue_process(	int index,int maxentryNum,int defaultFlag, int upweight, int downweight)
{
	QOSQUEUE_T entry;
	char str_shed[32]={0},tmp_args[32]={0};
	char * lan_intf="br0";
	char * wan_intf="eth1";
	char str_classId[32]={0},str_queId[32]={0},str_pare[32];
	char str_ceil[32]={0},str_rate[32]={0},str_burst[32]={0};
	char str_pri[32]={0};
	char str_min_th[32]={0},str_max_p[32]={0};
	char str_probability[16]="0.02";
	int weight =0;
	int pri=0;
	int burst;
	
	unsigned int bandwidth=0, rate, ceil;
	int shed_flag=0,drop_flag=0;
	int uplink=0,downlink=0;
	int link_speed=102400;
	char command[512];
	//printf("queue index:%d,[%s]:[%d].\n",index,__FUNCTION__,__LINE__);
#if defined (CONFIG_RTL_8198)||defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
	link_speed=1024000;
#endif	
	if((index>0)&&(index<=maxentryNum))
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_QOS_QUEUE_TBL, (void *)&entry);
		if(entry.QueueEnable)
		{
			printf("queue index:%d,[%s]:[%d].\n",index,__FUNCTION__,__LINE__);
			uplink=downlink=0;
			rate=ceil=burst=weight=0;
			shed_flag=drop_flag=0;
			
			if(defaultFlag)
			{
				
				uplink=downlink =1;
			}
			else 
			{
				if(strlen(entry.QueueInterface))
				{
					/*interface*/
					if(strstr(entry.QueueInterface, "WAN")){
						uplink=1;
						
					}	
			        else if(strstr(entry.QueueInterface, "LAN")){
						downlink=1;
						
		        	}
			        
				}
				else{
					
					uplink=downlink =1;
	        	}	
			}
			
			
			/*schedule : wfq/sp */
			if(strlen(entry.SchedulerAlgorithm))
			{
				sprintf(str_shed,"%s",entry.SchedulerAlgorithm);
				if(strcmp(entry.SchedulerAlgorithm,"WFQ")==0)
				{
					shed_flag =SCHEDULE_WFQ;
				}
				else if(strcmp(entry.SchedulerAlgorithm,"SP")==0)
				{
					shed_flag =SCHEDULE_SP;
				}
				else if(strcmp(entry.SchedulerAlgorithm,"WRR")==0)
				{
					shed_flag =SCHEDULE_WRR;
				}
				else
				{	
					printf("invalid SchedulerAlgorithm!");
				}
			}
			else
			{
				printf("invalid SchedulerAlgorithm!");
			}
			
			if(entry.QueuePrecedence)
			{
				pri =entry.QueuePrecedence;
				sprintf(str_pri,"%d",pri);
			}
			
			if(entry.ShapingRate!=-1)
			{
				if(entry.ShapingRate<=100)
				{
					bandwidth = link_speed*entry.ShapingRate/100; //percent of the rate of the highest rate-constrained
				}
				else
					bandwidth =(entry.ShapingRate)/1000;	//bit->kbit
				
				sprintf(str_ceil,"%dkbit",bandwidth);
				
				if(entry.ShapingBurstSize)
				{
					burst =entry.ShapingBurstSize;
					sprintf(str_burst,"%s %d",_burst,burst);
					
				}
				//printf("entry.ShapingRate:%d,bandwidth:%d,ceil:%s,burst:%s[%s]:[%d].\n",entry.ShapingRate,bandwidth,str_ceil,str_burst,__FUNCTION__,__LINE__);
			}
			else
			{
				sprintf(str_ceil,"%dkbit",link_speed);
				sprintf(str_burst,"");
				//printf("no need to shape rate!\n");
			}
			
			if(shed_flag==SCHEDULE_WFQ)
			{
				if(entry.QueueWeight)
				{ 
					weight =entry.QueueWeight;	
				}
			}
			
			//to do: drop algorithm 
			if(strlen(entry.DropAlgorithm))
			{
				if(strcmp(entry.DropAlgorithm,"RED")==0)
					drop_flag=DROP_SHEDULE_RED;
				else if(strcmp(entry.DropAlgorithm,"DT")==0)
					drop_flag=DROP_SHEDULE_DT;
				else if(strcmp(entry.DropAlgorithm,"WRED")==0)
					drop_flag=DROP_SHEDULE_WRED;
				else if(strcmp(entry.DropAlgorithm,"BLUE")==0)
					drop_flag=DROP_SHEDULE_BLUE;
			}

			if(drop_flag ==DROP_SHEDULE_RED)
			{
				if(entry.REDPercentage>0)
				{
					sprintf(str_max_p,"%s %d",_max,entry.REDPercentage);
				}
				else
				{
					sprintf(str_max_p,"%s %d",_max,10);
				}
				
				if(entry.REDThreshold>0)
				{
					sprintf(str_min_th,"%s %d",_min,entry.REDThreshold);
				}
				else 
				{
					
					sprintf(str_min_th,"%s %d",_min,5);
				}
			}
			
			if((uplink==0)&&(downlink==0))
			{
				printf("errror queue setting![%s]:[%d].\n",__FUNCTION__,__LINE__);
				
			}
			//uplink queue
			if(uplink)
			{
				sprintf(str_classId,"%d:%d",WAN_FLAG,(index+2));
				sprintf(str_queId,"%d:",(WAN_FLAG*100+(index+2)));
				if(shed_flag==SCHEDULE_WFQ)
				{
					//wfq weight based on rate
					if((weight)&&(upweight))
					{
						rate= (int)(weight*link_speed/upweight);
						sprintf(str_rate,"%dkbit",rate);
					}
					else
						sprintf(str_rate,"1kbit");
				}
				else if(shed_flag==SCHEDULE_SP)
				{
					
					//strict priority : rate =ceil
					strcpy(str_rate,str_ceil);
				}
				else
				{
					//not support , do nothing	
					sprintf(str_rate,"1kbit");
				}
				printf("add uplink queue[%s]:[%d].\n",__FUNCTION__,__LINE__);
				sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",_tc, _class, _add,_dev, wan_intf,
					_parent, "2:1", _classid, str_classId, _htb, _rate, str_rate, _ceil, str_ceil,str_burst, _prio, str_pri,  _quantum, "30000");
				printf("command:%s,[%s]:[%d].\n",command,__FUNCTION__,__LINE__);
				system(command);
				//TC_CMD="tc class add dev $WAN parent 2:1 classid 2:2 htb rate 1kbit ceil ${UPLINK_SPEED}kbit prio 256 quantum 30000"
				if(drop_flag)
				{
					if(drop_flag==DROP_SHEDULE_RED)
					{
						sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s",_tc, _qdisc, _add, _dev, wan_intf,
						_parent, str_classId, _handle, str_queId, _red, str_min_th,str_max_p,_probability,str_probability );
						//printf("command:%s,[%s]:[%d].\n",command,__FUNCTION__,__LINE__);
						system(command);
					}
					else
					{
						sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s",_tc, _qdisc, _add, _dev, wan_intf,
						_parent, str_classId, _handle, str_queId, _sfq, _perturb, "10");
						//printf("command:%s,[%s]:[%d].\n",command,__FUNCTION__,__LINE__);
						system(command);
					}	
				}
				else
				{
					sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s",_tc, _qdisc, _add, _dev, wan_intf,
					_parent, str_classId, _handle, str_queId, _sfq, _perturb, "10");
					printf("command:%s,[%s]:[%d].\n",command,__FUNCTION__,__LINE__);
					system(command);
				}	
				
				
			}
			//downlink queue
			if(downlink)
			{
				sprintf(str_classId,"%d:%d",LAN_FLAG,(index+2));
				sprintf(str_queId,"%d:",(LAN_FLAG*100+(index+2)));
				if(shed_flag==SCHEDULE_WFQ)
				{
					//wfq weight based on rate
					if(weight&&downweight)
					{
						rate= (int)(weight*link_speed/downweight);
						sprintf(str_rate,"%dkbit",rate);
					}
					else
						sprintf(str_rate,"1kbit");
				}
				else if(shed_flag==SCHEDULE_SP)
				{
					//strict priority : rate =ceil
					strcpy(str_rate,str_ceil);
				}
				else
				{
					//not support , do nothing
					sprintf(str_rate,"1kbit");
				}
				
				//printf("add downlink queue[%s]:[%d].\n",__FUNCTION__,__LINE__);
				
				sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",_tc, _class, _add,_dev, lan_intf,
					_parent, "1:1", _classid, str_classId, _htb, _rate, str_rate, _ceil, str_ceil,str_burst, _prio, str_pri,  _quantum, "30000");
				printf("command:%s,[%s]:[%d].\n",command,__FUNCTION__,__LINE__);
				system(command);
				//TC_CMD="tc class add dev $WAN parent 2:1 classid 2:2 htb rate 1kbit ceil ${UPLINK_SPEED}kbit prio 256 quantum 30000"
				
				sprintf(command,"%s %s %s %s %s %s %s %s %s %s %s %s",_tc, _qdisc, _add, _dev, lan_intf,
				_parent, str_classId, _handle, str_queId, _sfq, _perturb, "10");
				printf("command:%s,[%s]:[%d].\n",command,__FUNCTION__,__LINE__);
				system(command);
				
				
			}	
					
		}
	}
}

int rtl_process_ClassQueue(void)
{
	/*queue process*/
	int entryNum=0;
	int DefaultQueue=-1;
	int DefaultTrafficClass=-1;
	int ret=0;
	int j=0,index=0,default_QId=0;
	int qid[MAX_QOS_QUEUE_NUM+1]={0};
	int qpri[MAX_QOS_QUEUE_NUM+1]={-1};
	char tmp_args[32]={0};
	
	char * lan_intf="br0";
	char * wan_intf="eth1";
	int link_speed=102400;
	int upweight=0,downweight=0;
	int dflt_queue=0;
	char dflt_Qid[32]={0};
#if defined (CONFIG_RTL_8198)||defined (CONFIG_RTL_8198C) || defined (CONFIG_RTL_8367R_SUPPORT) || defined(CONFIG_RTL_83XX_SUPPORT)
	link_speed=1024000;
#endif	
	printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	apmib_get(MIB_QOS_QUEUE_TBL_NUM, (void *)&entryNum);
	if(entryNum)
	{
		apmib_get(MIB_QOS_DEF_QUEUE, (void *)&DefaultQueue);

		apmib_get(MIB_QOS_DEF_TRAFFIC_CLASS, (void *)&DefaultTrafficClass);
		
		//printf("DefaultQueue:%d,DefaultTrafficClass:%d,[%s]:[%d].\n",DefaultQueue,DefaultTrafficClass,__FUNCTION__,__LINE__);
		if((DefaultQueue >0 )&&(DefaultQueue<=entryNum))
		{
			default_QId =DefaultQueue;
			//printf("queue_index:%d,[%s]:[%d].\n",DefaultQueue,__FUNCTION__,__LINE__);
			
		}
		else if(DefaultTrafficClass!=-1)
		{
			
			#if 1
			
			ret=rtl_find_qid_by_trafficClass(DefaultTrafficClass,&qid[0]);
			if(ret){
				for (j=0;j< MAX_QOS_QUEUE_NUM+1;j++)
				{
					if(qid[j])
					{
						/*qid: j*/
						default_QId = j;
						//printf("default_QId:%d,[%s]:[%d].\n",default_QId,__FUNCTION__,__LINE__);
						break;
					}
				}
			}
			#endif
		}
		else
		{
			/*null queue, do nothing*/
			
		}
		
		_rtl_queue_preprocess(entryNum,&qpri[0],&upweight,&downweight,default_QId);
		//printf("upweight:%d,downweight:%d,[%s]:[%d].\n",upweight,downweight,__FUNCTION__,__LINE__);
		/* total bandwidth section--uplink*/
		//root queue
		
		
		sprintf(dflt_Qid,"%d",(default_QId+2));
		
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, wan_intf, _root, _handle, "2:0", _htb, _default, dflt_Qid, _r2q, "64", NULL_STR);
		//tc qdisc add dev $WAN root handle 2:0 htb default 2 r2q 64
		sprintf(tmp_args, "%dkbit", link_speed);
		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, wan_intf, _parent, "2:0", _classid, "2:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);
		//TC_CMD="tc class add dev $WAN parent 2:0 classid 2:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit"
	
		/* total bandwidth section--downlink*/
		RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, lan_intf, _root, _handle, "1:0", _htb, _default, dflt_Qid, _r2q, "64", NULL_STR);
		//tc qdisc add dev $WAN root handle 2:0 htb default 2 r2q 64
		sprintf(tmp_args, "%dkbit", link_speed);
		RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, lan_intf, _parent, "1:0", _classid, "1:1", _htb, _rate, tmp_args, _ceil, tmp_args,  _quantum, "30000", NULL_STR);
		//TC_CMD="tc class add dev $WAN parent 2:0 classid 2:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit"
		if(default_QId==0)	
		{
			//tc class add dev eth1 parent 2:1 classid 2:2 htb rate 102400kbit ceil 102400kbit prio 2

			//tc qdisc add dev eth1 parent 2:2 handle 202: sfq perturb 10
			
			sprintf(tmp_args, "%dkbit", link_speed);
			RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, wan_intf, _parent, "2:1", _classid, "2:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "256",  _quantum, "30000", NULL_STR);
			//TC_CMD="tc class add dev $WAN parent 2:0 classid 2:1 htb rate ${UPLINK_SPEED}kbit ceil ${UPLINK_SPEED}kbit"
			
			RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, wan_intf, _parent, "2:2",_handle, "202", _sfq, _perturb,"10", NULL_STR);

			RunSystemCmd(NULL_FILE, _tc, _class, _add, _dev, lan_intf, _parent, "1:1", _classid, "1:2", _htb, _rate, "1kbit", _ceil, tmp_args, _prio, "256",  _quantum, "30000", NULL_STR);
			/* total bandwidth section--downlink*/
			RunSystemCmd(NULL_FILE, _tc, _qdisc, _add, _dev, lan_intf, _parent, "1:2",_handle, "102", _sfq, _perturb,"10", NULL_STR);
			
		}
		
	}
	
	for (j=1; j<=entryNum; j++) 
	{
		index =qpri[j];
		if(index>0)
		{
			if(index!=default_QId)
			{
				_rtl_queue_process( index,entryNum,0,upweight,downweight);
			}
			else
			{
				//default queue
				_rtl_queue_process( index,entryNum,1,upweight,downweight);
			}
		}
	}
	
}

/*
* qos queue stats
*/
#define TR069_QOS_TMP_FILE "/var/tmpqosStats"
#define TR069_QOS_STATSINFO_FILE "/var/qosStatsInfo"

typedef struct {
	unsigned int	outputPkt;
	unsigned int	outputByte;
	unsigned int	dropPkt;
	unsigned int	dropByte;
	unsigned int	OccupancyPkt;
	unsigned int	OccupancyPercent;
	unsigned int 	qId;
	
}rtl_queStats_info;

//int _rtl_preprocess_queueStats(void)


//tc -s class ls dev eth0  > /var/tmpqosStats
#define QUEUE_STATS_START_FLAG	0x1
#define QUEUE_STATS_END_FLAG	0x2

int rtl_process_queueStats(void)
{
	
	char str_intf[32]={0};
	int downlink=0,uplink=0;
	char lan_intf[32]="br0";
	char wan_intf[32]="eth1";
	char queue_intf[32]={0};
	int index;
	int entryNum;
	int queueIndex=0;	
	int qId;
	int i = -1,j=0;
	int flag=0;
	char tmp[64];
	unsigned int tmpValue;
	rtl_queStats_info stats_info[MAX_QOS_QUEUE_NUM+1];
	char *ptr;
	char *strptr;
	FILE * stream;
	char command[64]={0};
	char buffer[512]={0};
	int retval=-1;
	int mode =0;
	char line_buffer[128]={0};
	QOSQUEUESTATS_T entry;
	//tc -s class ls dev
	sprintf(command,"%s %s %s %s %s %s %s",_tc,"-s",_qdisc,"show",">",TR069_QOS_TMP_FILE );
	system(command);
	
	memset(&(stats_info[0]),0,sizeof(rtl_queStats_info)*(MAX_QOS_QUEUE_NUM+1));
	stream = fopen (TR069_QOS_TMP_FILE, "r" );
	if ( stream == NULL ) 
		return retval;
	else
	{ 	
		while(fgets(buffer, sizeof(buffer), stream))
		{
			strptr=buffer;
			//ptr =strstr(strptr,"qdisc");
			ptr= strsep(&strptr," ");
			
			if(ptr) 
			{
				//printf("ptr:%s strptr:%s,[%s]:[%d]\n",ptr,strptr,__FUNCTION__,__LINE__);
				if(strcmp(ptr,"qdisc")==0)
				{	
					i++;
					flag =QUEUE_STATS_START_FLAG;
					ptr= strsep(&strptr," ");
					
					if(ptr)
					{
						ptr= strsep(&strptr,":");
						if(ptr){
						
							sscanf(ptr,"%d",&tmpValue);
							//printf("ptr%s,tmp:%d [%s]:[%d].\n",ptr,tmpValue,__FUNCTION__,__LINE__);
							
							stats_info[i].qId=tmpValue;
						}
						
					}

					
					
				}
				else
				{
					ptr= strsep(&strptr," ");
					if(ptr){
					if(strcmp(ptr,"rate")==0)
					{	
						flag =QUEUE_STATS_END_FLAG;
					}
					
					if(flag ==QUEUE_STATS_START_FLAG)
					{
						if(strcmp(ptr,"Sent")==0)
						{
							
							ptr= strsep(&strptr," ");
							
							//printf("-----[%d],ptr:%s [%s]:[%d].\n",i,ptr,__FUNCTION__,__LINE__);
							if(ptr)
							{
								sscanf(ptr,"%d",&tmpValue);
								stats_info[i].outputByte=tmpValue;
								
								//printf("tmp:%d [%s]:[%d].\n",tmpValue,__FUNCTION__,__LINE__);
							}
							ptr= strstr(strptr,"bytes");
							
							//printf("ptr:%s [%s]:[%d].\n",ptr,__FUNCTION__,__LINE__);
							if(ptr)
							{
								ptr= ptr+sizeof("bytes");
								if(ptr)
								{
									sscanf(ptr,"%d",&tmpValue);
									stats_info[i].outputPkt=tmpValue;
									
									//printf("tmp:%d [%s]:[%d].\n",tmpValue,__FUNCTION__,__LINE__);
								}
							}
							ptr= strstr(strptr,"dropped");
							
							//printf("ptr:%s [%s]:[%d].\n",ptr,__FUNCTION__,__LINE__);
							if(ptr)
							{
								ptr= ptr+sizeof("dropped");
								if(ptr)
								{
									sscanf(ptr,"%d",&tmpValue);
									
									stats_info[i].dropPkt=tmpValue;
									
									//printf("tmp:%d [%s]:[%d].\n",tmpValue,__FUNCTION__,__LINE__);
								}
							}
							ptr= strstr(strptr,"overlimits");
							
							//printf("ptr:%s [%s]:[%d].\n",ptr,__FUNCTION__,__LINE__);
							if(ptr)
							{
								ptr= ptr+sizeof("overlimits");
								if(ptr)
								{
									sscanf(ptr,"%d",&tmpValue);
									
									//printf("tmp:%d [%s]:[%d].\n",tmpValue,__FUNCTION__,__LINE__);
									stats_info[i].OccupancyPkt=tmpValue;
								}
							}
						}
						
					}
					}
				}
				
			}
		}
	}
#if 0
	for (j=0;j<=i;j++)
	{
		printf("%d,[%d]send:%d bytes %d pkts drop:%d Occupancy%d\n",j,stats_info[j].qId,
			stats_info[j].outputByte,stats_info[j].outputPkt,stats_info[j].dropPkt,stats_info[j].OccupancyPkt);
	}
#endif

	flag=0;

	apmib_get(MIB_QOS_QUEUESTATS_TBL_NUM, (void *)&entryNum);
	for (index=1;index<entryNum;index++)
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_QOS_QUEUESTATS_TBL, (void *)&entry);
		retval=-1;
		uplink=downlink=queueIndex=0;
		memset(command, '\0', sizeof(64));
		if(entry.Enable)
		{
			queueIndex =entry.Queue;
			strcpy(str_intf,entry.Interface);
			if(strlen(str_intf))
			{
				if(strstr(str_intf, "WAN")){
					uplink=1;
				
				}	
		        else if(strstr(str_intf, "LAN")){
					downlink=1;
					
	        	}
				else
				{
				
				}
			}
			if(uplink)
			{
				sprintf(queue_intf,"%s",wan_intf);
			}
			else if(downlink)
			{
				sprintf(queue_intf,"%s",lan_intf);
			}

			if(uplink)
			{
				qId = WAN_FLAG *100+queueIndex;
			}
			else if(downlink)
			{
				qId = LAN_FLAG *100+queueIndex;
			}	

			for (j=0;j<=MAX_QOS_QUEUE_NUM;j++)
			{
				if((stats_info[j].qId)&&(stats_info[j].qId==qId))
				{
					retval=j;
					flag++;
					break;
				}
			}
			
			if((flag)&&(retval!=-1))
			{
				
				mode =2;
				if (flag==1)
				{
					sprintf(line_buffer,"queue[%d] start\n",queueIndex);
					write_line_to_file(TR069_QOS_STATSINFO_FILE, 1, line_buffer);	
				}
				else
				{
					sprintf(line_buffer,"queue[%d] start\n",queueIndex);
					write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);	
				}	
				
				sprintf(line_buffer,"OutputPackets:%d\n",stats_info[retval].outputPkt);
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				sprintf(line_buffer,"OutputBytes:%d\n",stats_info[retval].outputByte);
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				sprintf(line_buffer,"DroppedPackets:%d\n",stats_info[retval].dropPkt);
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				sprintf(line_buffer,"DroppedBytes:%d\n",stats_info[retval].dropByte);
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				sprintf(line_buffer,"OccupancyPackets:%d\n",stats_info[retval].OccupancyPkt);
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				sprintf(line_buffer,"OccupancyPercentage:%d\n",stats_info[retval].OccupancyPercent);
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				
				sprintf(line_buffer,"end\n");
				write_line_to_file(TR069_QOS_STATSINFO_FILE, mode, line_buffer);
				
			}
		}
	}

	return retval;
}


/*
* qosflow
*/
//not support yet
int rtl_process_APPflow(int appIndex,char *appProtocol)
{
	int ret=0,index;
	int entryNum=0;
	int found=0;
	char *p,*p1;
	char flowType[256];
	char flowPara[256];
	char protocol[32],mediatype[32],transport[32];
	int ClassQueue;
	int queueFlag;
	char queueId[8];
	TR098_FLOWCONF_T entry;
#if	0
	apmib_get(MIB_TR098_QOS_FLOW_TBL_NUM, (void *)&entryNum);
	for (index=1; index<=entryNum; index++) 
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)index;
		apmib_get(MIB_TR098_QOS_FLOW_TBL, (void *)&entry);
		if(entry.flow_enable)
		{
			if(entry.app_identify ==appIndex)
			{
				if(strlen(entry.flow_type))
				{
					
					p=strstr(entry.flow_type, "org:");
					if(p)
					{
						p=p+4;
						
					}
					p1 =strsep(p,"-");
					if(p1)
						sprintf(protocol,"%s",p1);
					
					p1 =strsep(p,"-");
					if(p1)
						sprintf(mediatype,"%s",p1);

					sprintf(transport,"%s",p);

					if(strcmp(protocol,appProtocol)==0)
					{
						
					}
				}
				else
				{
					sprintf(flowType,"%s","");
				}
				/*not support flow_parameter*/
				if(strlen(entry.flow_type_para))
				{
					
				}
				else
				{
					sprintf(flowPara,"%s","");
				}

				if((entry.flow_queue!=-1)&&(entry.flow_class!=-1))
				{
					printf("FlowTrafficClass and FlowQueue MUST NOT both be specified.\n");
					goto errout;
				}
				else
				{
					if(entry.flow_queue!=-1)
					{
						ClassQueue=entry.ClassQueue;
						queueFlag=rtl_getQueueFlag(ClassQueue);
						if(queueFlag&LAN_FLAG)
						{
							
							sprintf(queueId,"%d:%d",LAN_FLAG,ClassQueue);
							sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s", 
								_tc, _filter, _add, _dev, br_interface, _parent, "1:0", _prio, "100",_handle,mark_value, _fw, _classid,queueId );
							system(command);
						}
						if(queueFlag&WAN_FLAG)
						{
							
							sprintf(queueId,"%d:%d",WAN_FLAG,ClassQueue);
							sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s", 
								_tc, _filter, _add, _dev, wan_interface, _parent, "2:0", _prio, "100",_handle,mark_value, _fw, _classid,queueId );
							system(command);
						}
					}
					else if(entry.flow_class!=-1)
					{
					
					}
				}
					
			}
		}
		
	}
out:
	return ret;
errout: 
	printf("invalid argument"); 
#endif
	return ret;
}
int rtl_process_ClassAPP(int classIndex,int uplink,int downlink,char *l3_match ,char *l3_match2,int class_app, int *dscp_match)
{
	int ret=0,index =classIndex;
	int entryNum=0;
	TR098_APPCONF_T entry;
	char appProtocol[256];
	char *p,*p1,*p2;
	char protocol[32],mediatype[32],transport[32];
	int retval;
	int ClassQueue = -1;
	int queueFlag;
	char queueId[8];
	int j;
	int class_qid[MAX_QOS_QUEUE_NUM+1]={0};
	char command[512]={0};
	char l2_cmd[512]={0};
	char l3_cmd[512]={0};
	char mark_match[32]={0};
	char protocol_match[64]={0};
	char policer_cmd[256]={0};
	char *wan_interface = "eth1";
    char *lan_interface = "br0";
	int upMark;
	int downMark;
	int tmp;
	
	printf("--process app table!\n");
	/*app process*/
	if((class_app>0)&&(class_app<MAX_TR098_APP_TBL_NUM))
	{
		memset(&entry, '\0', sizeof(entry));
		*((char *)&entry) = (char)class_app;
		apmib_get(MIB_TR098_QOS_APP_TBL, (void *)&entry);
		if(entry.app_enable)
		{
			if(strlen(entry.protocol_identify))
			{
				p=strstr(entry.protocol_identify, "org:");
				if(p)
				{
					p=p+4;
				    sprintf(protocol,"%s",p);
				    sprintf(protocol_match,"%s %s %s",match,layer7,protocol);
				
				    retval=rtl_process_APPflow(class_app,protocol);
                }
                else
                {
                    fprintf(stderr, "%s:%d\n", __FUNCTION__, __LINE__);
                    sprintf(protocol_match,"");
                }
			}
			else
			{
				sprintf(protocol_match,"");
			}
            
			if(((entry.default_class!=-1)&&(entry.default_queue!=-1)))
			{
				printf("AppDefaultTrafficClass and AppDefaultQueue MUST NOT both be specified.\n");
				goto errout;
			}
            else if (((entry.default_class ==-1)&&(entry.default_queue ==-1)))
            {
                printf("One of AppDefaultTrafficClass and AppDefaultQueue SHOULD be specified.\n");
				goto errout;
            }
			else
			{
				if(entry.default_queue!=-1)
				{

					//classify to policer
					if(entry.default_policer!=-1)
					{
						rtl_process_ClassPolicer(entry.default_policer,policer_cmd);
					}
					else
					{
						sprintf(policer_cmd,"");
					}
					//classify to queue
					ClassQueue=entry.default_queue;
					goto process;
				}
				else if(entry.default_class!=-1)
				{
					rtl_find_qid_by_trafficClass(entry.default_class,class_qid);
					for (j=1;j<MAX_QOS_QUEUE_NUM+1;j++)
					{
						if(class_qid[j])
						{
							/*qid: j*/
							ClassQueue= j;
							goto process;
						}
					}
				}
process:	
				{
					queueFlag=rtl_getQueueFlag(ClassQueue);
                    *dscp_match = queueFlag;
                    *(dscp_match+3) = queueFlag;
					//uplink engress wan intf
					if((uplink &&(queueFlag&WAN_FLAG))||(downlink &&(queueFlag&LAN_FLAG)))
                    {
                        if(uplink &&(queueFlag&WAN_FLAG)) 
                        {
                            tmp = (index<<2)+4;
                            sprintf(l3_cmd, "%s %s %s", "-i", lan_interface, l3_match);
                            sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
                                Iptables, ADD, PREROUTING, _table, mangle_table, l3_cmd, jump, MARK, set_mark, tmp);
                            system(command);

                            if(strlen(l3_match2))
                            {
                                sprintf(l3_cmd, "%s %s %s", "-i", lan_interface, l3_match2);
                                sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
                                    Iptables, ADD, PREROUTING, _table, mangle_table, l3_cmd, jump, MARK, set_mark, tmp);
                                system(command);
                            }

                            sprintf(mark_match, "%s %s %s %d", match, mark, "--mark", tmp);
                            
                            upMark = (index<<2)+2;
                            fprintf(stderr, "%s:%d, upMark = %d, index = %d\n", __FUNCTION__, __LINE__, upMark, index);
                            sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d", 
                                Iptables, ADD, PREROUTING, _table, mangle_table, mark_match, protocol_match, jump, MARK, set_mark, upMark);
                            fprintf(stderr, "uplink l3 command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
                            system(command);

                            *(dscp_match+1) = tmp;
                            *(dscp_match+4) = upMark;
 
                            sprintf(queueId,"%d:%d",WAN_FLAG,(ClassQueue+2));
                            //to-do: polilcer process
                            sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d %s %s %s", 
                                _tc, _filter, _add, _dev, wan_interface, _parent, "2:0", _protocol2, _ip, _prio, "100",_handle,upMark, _fw, _classid,queueId );
                            
                            fprintf(stderr, "uplink filter command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
                            system(command);
                        }
                        if(downlink &&(queueFlag&LAN_FLAG))
                        {
                            tmp = (index<<2)+3;
                            sprintf(l3_cmd, "%s %s %s", "-o", lan_interface, l3_match);
                            sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
                                Iptables, ADD, POSTROUTING, _table, mangle_table, l3_cmd, jump, MARK, set_mark, tmp);
                            system(command);
                            sprintf(mark_match, "%s %s %s %d", match, mark, "--mark", tmp);

                            if(strlen(l3_match2))
                            {
                                sprintf(l3_cmd, "%s %s %s", "-o", lan_interface, l3_match2);
                                sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
                                    Iptables, ADD, PREROUTING, _table, mangle_table, l3_cmd, jump, MARK, set_mark, tmp);
                                system(command);
                            }
                            
                            downMark= (index<<2)+1;
                            fprintf(stderr, "%s:%d, downMark = %d, index = %d\n", __FUNCTION__, __LINE__, downMark, index);
                            sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d", 
                            Iptables, ADD, POSTROUTING, _table, mangle_table, mark_match, protocol_match, jump, MARK, set_mark, downMark);
                            fprintf(stderr, "down l3 command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
                            system(command);
                            
                            *(dscp_match+2) = tmp;
                            *(dscp_match+5) = downMark;
                            
                            sprintf(queueId,"%d:%d",LAN_FLAG,(ClassQueue+2));
                            //to-do: polilcer process
                            sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d %s %s %s", 
                                    _tc, _filter, _add, _dev, lan_interface, _parent, "1:0", _protocol2, _ip, _prio, "100",_handle,downMark, _fw, _classid,queueId );
                            
                            fprintf(stderr, "downlink filter command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
                            system(command);
                        }
                    }
                    else
				    {
					    printf("\nuplink:%d,downlink:%d,queueFlag:%d,find no queue related to classify!\n",uplink,downlink,queueFlag);
				    }
					//rtl_process_ClassQueue(ClassQueue);	
				}	
			}		
		}
		else
        {
            fprintf(stderr, "App entry %d is disabled!\n", class_app);
        }
	}
    else
    {
        fprintf(stderr, "There is no App entry %d!\n", class_app);
        goto errout;
    }
out:
	return ret;
errout:	
	printf("invalid argument");	
goto out;	
}

#ifndef DHCPD_CONF_FILE
#define DHCPD_CONF_FILE "/var/udhcpd.conf"	
#endif

#define DHCP_VENDORCLASSID	0x1
#define DHCP_CLIENTID		0x2
#define DHCP_USERCLASSID	0x4
#define DHCP_VENDORSPECIFICINfO		0x8	
int rtl_process_classifyByDhcpOption(rtl_dhcp_info * dhcp_info, int dhcpFlag,char * ipStart,char *ipEnd)
{
#define RTL_START_FLAG		0x1
#define RTL_END_FLAG		0x2

	FILE *stream;
	char buffer[512];
	rtl_dhcp_info dhcp_conf;
	char startip[32];
	char endip[32];
	char * ptr;
	int flag;
	int index=0;
	int unmatch=0;
	int ret =0;

	//printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	stream = fopen (DHCPD_CONF_FILE, "r" );
	if ( stream != NULL ) 
	{ 	
		while(fgets(buffer, sizeof(buffer), stream))
		{
			//printf("buffer:%s\n",buffer);
			ptr = strstr(buffer, "poolname");
			
			if (ptr) 
			{
				flag = RTL_START_FLAG;
				
			}
			
			ptr = strstr(buffer, "poolend");
			
			if (ptr)
			{
				flag = RTL_END_FLAG;
			}
			if(flag ==RTL_START_FLAG)
			{
				ptr = strstr(buffer, "vendorclass");
				if (ptr) 
				{
					ptr = ptr + strlen("vendorclass");
					if (ptr)
					{
						
						sscanf(ptr, "%s", dhcp_conf.VendorClassID); 
						
						//printf("VendorClassID:%d,[%s]:[%d].\n",dhcp_conf.VendorClassID,__FUNCTION__,__LINE__);
					}
				}
				
				ptr = strstr(buffer, "vendorclassmode");
				if (ptr) 
				{
					ptr = ptr + strlen("vendorclassmode");
					if (ptr)
					{
						
						sscanf(ptr, "%s",dhcp_conf.VendorClassIDMode); 
						
						//printf("VendorClassIDMode:%s,[%s]:[%d].\n",dhcp_conf.VendorClassIDMode,__FUNCTION__,__LINE__);
					}
				}
				
				ptr = strstr(buffer, "clientid");
				if (ptr) 
				{
					ptr = ptr + strlen("clientid");
					if (ptr)
					{
						
						sscanf(ptr, "%s", dhcp_conf.ClientID); 
						
						//printf("ClientID:%s,[%s]:[%d].\n",dhcp_conf.ClientID,__FUNCTION__,__LINE__);
					}
				}
				
				
				ptr = strstr(buffer, "userclass");
				if (ptr) 
				{
					ptr = ptr + strlen("userclass");
					if (ptr)
					{
						
						sscanf(ptr, "%s", dhcp_conf.ClientID); 
						
						//printf("ClientID:%s,[%s]:[%d].\n",dhcp_conf.ClientID,__FUNCTION__,__LINE__);
					}
				}

				
				//to-do :VendorSpecificInfo

				
				ptr = strstr(buffer, "start");
				if (ptr) 
				{
					ptr = ptr + strlen("start");
					if (ptr)
					{
						
						sscanf(ptr, "%s", startip); 
						
						//printf("startip:%s,[%s]:[%d].\n",startip,__FUNCTION__,__LINE__);
					}
				}

				ptr = strstr(buffer, "end");
				if (ptr) 
				{
					ptr = ptr + strlen("end");
					if (ptr)
					{
						
						sscanf(ptr, "%s", endip); 
						
						//printf("startip:%s,[%s]:[%d].\n",endip,__FUNCTION__,__LINE__);
					}
				}
			}
			else if(flag == RTL_END_FLAG)
			{
				
				for (index=0;index<4;index++)
				{
					if(dhcpFlag&(1<<index))
					{
						switch(1<<index)
						{
						
							case DHCP_VENDORCLASSID:
								if(strcmp(dhcp_conf.VendorClassID,dhcp_info->VendorClassID))
								{
									unmatch |=(1<<index);
									goto nextp;
								}	
								break;

							case DHCP_CLIENTID:
								if(strcmp(dhcp_conf.ClientID,dhcp_info->ClientID))
								{
									unmatch |=(1<<index);
									goto nextp;
								}
								break;	

							case DHCP_USERCLASSID:
								if(strcmp(dhcp_conf.UserClassID,dhcp_info->UserClassID))
								{
									unmatch |=(1<<index);
									goto nextp;
								}
								break;
								
							case DHCP_VENDORSPECIFICINfO:
								if(strcmp(dhcp_conf.VendorSpecificInfo,dhcp_info->VendorSpecificInfo))
								{
									unmatch |=(1<<index);
									goto nextp;
								}
								break;
								
						}		
						
							
					}
				}
nextp:					
				if(unmatch==0)
				{
					strcpy(ipStart,startip);
					strcpy(ipEnd,endip);
					ret =1;
					goto out;
				}
				else{
					memset(&dhcp_conf,0,sizeof(dhcp_conf));
					unmatch=0;
					flag=0;
					ptr=NULL;
					
				}
			}
		}
		
		
	}
	
out:
	return ret;
}

int rtl_processClassifyOrder(int * order)
{
	int index;
	int j=0,i=0;
	int entryNum;
	int ClassificationOrder[MAX_QOS_CLASS_NUM+1]={0};
	int tmp;
	int ret=0;
    int valid_num = 0;
	QOSCLASS_T class_entry;
	printf("[%s]:[%d].\n",__FUNCTION__,__LINE__);
	apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	for (index=1; index<=entryNum; index++) 
	{
		memset(&class_entry, '\0', sizeof(class_entry));
		*((char *)&class_entry) = (char)index;
		apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);

        order[index] = index;
		
		if(class_entry.ClassificationEnable){
			ClassificationOrder[index]=class_entry.ClassificationOrder;
			valid_num ++;
			//printf("[%d]:%d,[%s]:[%d].\n",index,ClassificationOrder[index],__FUNCTION__,__LINE__);
		}	
	}
	for (index=1; index<=entryNum; index++) 
	{
			for (j=index+1;j<=entryNum;j++)
			{

					if(ClassificationOrder[index] < ClassificationOrder[j])
					{
						tmp =ClassificationOrder[j];
					
						ClassificationOrder[j]=ClassificationOrder[index];
						ClassificationOrder[index]=tmp;

                        tmp = order[j];
						
						order[j]=order[index];
						order[index]=tmp;
					}
			}	
		}

	#if 1
	for (index=1; index<=entryNum; index++) 
	{
		fprintf(stderr,"[%d]:%d %d\n",index,ClassificationOrder[index],order[index]);
	}
	#endif
	return valid_num;
}
void rtl_set_vlanprio(int prio_mark, int sk_mark, int flag)
{
//    fprintf(stderr, "prio_mark = %d, sk_mark =%d, flag = %d [%s:%d]\n",prio_mark,sk_mark,flag,__FUNCTION__, __LINE__);
    unsigned char str_dscp[64]={0}, mark_match[64]={0}, command[512]={0};
    unsigned char str_chain[16] = {0};
    unsigned char str_interf[8] = {0};
    unsigned char *wan_interface = "eth1";
    unsigned char *lan_interface = "br0";
	int first_three_dscp = -1;
	
    if(flag&WAN_FLAG)
    {
        sprintf(str_chain, "%s", PREROUTING);
        sprintf(str_interf, "%s %s", "-i", lan_interface);
    }
    else if(flag&LAN_FLAG)
    {
        sprintf(str_chain, "%s", POSTROUTING);
        sprintf(str_interf, "%s %s", "-o", lan_interface);
    }
    else
    {
        return;
    }

    sprintf(mark_match,"%s %s %s %d",match,mark,"--mark",sk_mark);

//    fprintf(stderr, "mark_match = %s, [%s:%d]\n",mark_match, __FUNCTION__, __LINE__);

    if(prio_mark == -1)
    {
		    printf("vlan priority unchanged!\n");
    }
    else if(prio_mark == -2)
    {
        //auto mark vlan priority based on dscp value
        /*
                    dscp -> ethernet priority
	          		  0      ->  0
	           		  0x8	 ->  3
					  0x10	 ->  4
	        		  0x18	 ->  5
	        		  0x28	 ->  6
	        		  0x38	 ->  7
         */      
         
        //0->0
        sprintf(str_dscp,"%s %s %s %d",match,"dscp","--dscp", 0);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_dscp, mark_match, jump, VLANPRIORITY, set_vlanpri, 0);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);

		 //8->3
        sprintf(str_dscp,"%s %s %s %d",match,"dscp","--dscp", 8);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_dscp, mark_match, jump, VLANPRIORITY, set_vlanpri, 3);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);

		//16->4
        sprintf(str_dscp,"%s %s %s %d",match,"dscp","--dscp", 16);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_dscp, mark_match, jump, VLANPRIORITY, set_vlanpri, 4);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);

		//24->5
        sprintf(str_dscp,"%s %s %s %d",match,"dscp","--dscp", 24);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_dscp, mark_match, jump, VLANPRIORITY, set_vlanpri, 5);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);

		//40->6
        sprintf(str_dscp,"%s %s %s %d",match,"dscp","--dscp", 40);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_dscp, mark_match, jump, VLANPRIORITY, set_vlanpri, 6);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);

		//56->7
        sprintf(str_dscp,"%s %s %s %d",match,"dscp","--dscp", 56);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_dscp, mark_match, jump, VLANPRIORITY, set_vlanpri, 7);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
    }
    else if(prio_mark >=0&&prio_mark<=7)
    {
        
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d", 
                Iptables, ADD, str_chain,_table, mangle_table, str_interf, mark_match, jump, VLANPRIORITY, set_vlanpri, prio_mark);
        printf("[%s]:[%d],SET Ethernet Priority Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
    }
    else
    {
        fprintf(stderr, "wrong vlan priority %d!\n", prio_mark);
    }
    return;
}
void rtl_set_dscp(int dscp_mark, int sk_mark, int flag)
{
    unsigned char str_vlan[64]={0}, mark_match[64]={0}, command[512]={0};
    unsigned char str_chain[16] = {0};
    unsigned char str_interf[8] = {0};
    unsigned char *wan_interface = "eth1";
    unsigned char *lan_interface = "br0";
		 
    if(flag&WAN_FLAG)
    {
        sprintf(str_chain, "%s", PREROUTING);
        sprintf(str_interf, "%s %s", "-i", lan_interface);
	}
    else if(flag&LAN_FLAG)
    {
        sprintf(str_chain, "%s", POSTROUTING);
        sprintf(str_interf, "%s %s", "-o", lan_interface);
    }
    else
    {
        return;
    }
//    fprintf(stderr, "str_chain = %s, str_interf = %s, [%s:%d]\n",str_chain, str_interf, __FUNCTION__, __LINE__);

    sprintf(mark_match,"%s %s %s %d",match,mark,"--mark",sk_mark);
//    fprintf(stderr, "mark_match = %s, [%s:%d]\n",mark_match, __FUNCTION__, __LINE__);

    if(dscp_mark == -1)
    {
            //not do set dscp value
    }
    else if(dscp_mark == -2)
    {
        //atuo mark dscp based upon the EthernetPriority value
        /*
                   ethernet priority->dscp
                   1   ->  0
                   2   ->  0
                   0   ->  0
                   3   ->  0x8
                   4   ->  0x10
                   5   ->  0x18
                   6   ->  0x28
                   7   ->  0x38
                
             */
        //0->0
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 0);
        sprintf(command, "%s %s %s %s %s %s % s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 0);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
    
        #if 0
        //1->0
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 1);     
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 0);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
    
        //2->0
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 2);
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 0);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
        #endif
            
        //3->8
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 3);
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 8);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
        
        //4->16
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 4);
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 16);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
        
        //5->24
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 5);
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 24);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
        
        //6->40
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 6);
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 40);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
        
        //7->56
        sprintf(str_vlan,"%s %s %s %d",match,"vlanpriority","--prio-value", 7);
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf, str_vlan, mark_match, jump, DSCP, set_dscp, 56);
        //printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
                
    }
    else if(dscp_mark >=0)
    {		
        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d", 
            Iptables, ADD, str_chain, _table, mangle_table, str_interf,  mark_match, jump, DSCP, set_dscp, dscp_mark);
        printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
        system(command);
    }
    else
    {
            printf("wrong dscp mark %d!\n", dscp_mark);
    }
    return;
        
}

void rtl_process_dfltClassifyFortr069(char *interface, char *wan_addr)
{
	int DefaultQueue;
	int DefaultPolicer;
	int DefaultTrafficClass;
	int DefaultDSCPMark;
	int DefaultEthernetPriorityMark;
	unsigned char policer_cmd[128]={0};
	unsigned char match_cmd[512]={0};
	unsigned char command[512]={0};
	unsigned char str_dscp[32],str_pri[32],str_vlan[64];
	unsigned char tmp[32];
	printf("set default queue management!\n");
	apmib_get(MIB_QOS_DEF_QUEUE, (void *)&DefaultQueue);
	apmib_get(MIB_QOS_DEF_POLICER, (void *)&DefaultPolicer);
	apmib_get(MIB_QOS_DEF_TRAFFIC_CLASS, (void *)&DefaultTrafficClass);
	apmib_get(MIB_QOS_DEF_DSCP_MARK, (void *)&DefaultDSCPMark);
	apmib_get(MIB_QOS_DEF_ETHERNET_PRIO_MARK, (void *)&DefaultEthernetPriorityMark);
	
	if(DefaultPolicer!=-1)
	{
		rtl_process_ClassPolicer(DefaultPolicer,policer_cmd);
	}
	if(DefaultQueue!=-1)
	{
		
	}
	else if(DefaultTrafficClass!=-1)
	{
		
	}

//    fprintf(stderr, "DefaultDSCPMark = %d, DefaultEthernetPriorityMark = %d[%s:%d]\n",DefaultDSCPMark, DefaultEthernetPriorityMark, __FUNCTION__, __LINE__);

    if(DefaultDSCPMark == -2 && DefaultEthernetPriorityMark != -2)
    {
        rtl_set_dscp(DefaultDSCPMark, 0, WAN_FLAG);
        rtl_set_dscp(DefaultDSCPMark, 0, LAN_FLAG);
        rtl_set_vlanprio(DefaultEthernetPriorityMark,0, WAN_FLAG);
        rtl_set_vlanprio(DefaultEthernetPriorityMark,0, LAN_FLAG);
    }
    else if(DefaultDSCPMark != -2)
    {
        rtl_set_vlanprio(DefaultEthernetPriorityMark,0, WAN_FLAG);
        rtl_set_vlanprio(DefaultEthernetPriorityMark,0, LAN_FLAG);
        rtl_set_dscp(DefaultDSCPMark, 0, WAN_FLAG);
        rtl_set_dscp(DefaultDSCPMark, 0, LAN_FLAG);
    }
    else
    {
        fprintf(stderr, "DSCPmark and Ethernet Priority MUST NOT both be -2!\n");
    }

#if 0
	/*set dscp */
	if(DefaultDSCPMark == -1)
	{
		//do nothing
	}
	else if(DefaultDSCPMark == -2)
	{
		//atuo mark dscp
		/*
			ethernet priority->dscp
			1	->	0
			2	->	0
			0	->	0
			3	->	0x8
			4	->	0x10
			5	->	0x18
			6	->	0x28
			7	->	0x38
			
		*/
		
		//0->0
		sprintf(str_pri,"%s %d","--prio-value",0);
		sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
		
		sprintf(match_cmd,"%s",str_vlan);

		sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,match_cmd, jump, DSCP, set_dscp, 0);
		//printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);
		//3->8
		sprintf(str_pri,"%s %d","--prio-value",3);
		sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
		
		sprintf(match_cmd,"%s",str_vlan);

		sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,match_cmd, jump, DSCP, set_dscp,8);
		//printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);

		//4->16
		sprintf(str_pri,"%s %d","--prio-value",4);
		sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
		
		sprintf(match_cmd,"%s",str_vlan);

		sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,match_cmd, jump, DSCP, set_dscp,16);
		//printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);

		//5->24
		sprintf(str_pri,"%s %d","--prio-value",5);
		sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
		
		sprintf(match_cmd,"%s",str_vlan);
		sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,match_cmd, jump, DSCP, set_dscp,24);
		//printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);

		//6->40
		sprintf(str_pri,"%s %d","--prio-value",6);
		sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
		
		sprintf(match_cmd,"%s",str_vlan);

		sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,match_cmd, jump, DSCP, set_dscp,40);
		//printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);

		//7->56
		sprintf(str_pri,"%s %d","--prio-value",7);
		sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
		
		sprintf(match_cmd,"%s",str_vlan);

		sprintf(command, "%s %s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,match_cmd, jump, DSCP, set_dscp,56);
		//printf("--[%s]:[%d]--,SET DSCP Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);
		
	
	}
	else if(DefaultDSCPMark>=0)
	{
		
		sprintf(command,"%s %s %s %s %s %s %s %s %d", Iptables, ADD, PREROUTING, _table, mangle_table , jump, DSCP, set_dscp, DefaultDSCPMark);
		system(command);

	}

	/*set ethernet priority */
	if(DefaultEthernetPriorityMark == -1)
	{
		//do noting
	}
	else if(DefaultEthernetPriorityMark == -2)
	{
		//to-do auto mark vlan priority based on dscp
		
	}
	else if(DefaultEthernetPriorityMark>=0)
	{
		//to do kernel modify vlan priority
		//iptables -A PREROUTING -t mangle  -j VLANPRIORITY --set-vlanpri 1
	
		sprintf(command, "%s %s %s %s %s %s %s %s %d", 
        Iptables, ADD, PREROUTING, _table, mangle_table,  jump, VLANPRIORITY, set_vlanpri, DefaultEthernetPriorityMark);
		//printf("[%s]:[%d],SET vlanpri Command = %s\n",__FUNCTION__,__LINE__,command);
		system(command);
		
	}
    #endif
	
	return;
}

void set_classifyRuleforTr069(char *interface, char *wan_addr, int dscp_match[MAX_QOS_CLASS_NUM+1][6] )
{
	int entryNum=0;
	QOSCLASS_T entry,class_entry,class_entry2;
	int index,j;
	unsigned char tmp_args[20] = {0};
	unsigned char *wan_interface = "eth1";
    unsigned char *lan_interface = "br0";
    unsigned char tmp_str1[32], tmp_str2[32];
    unsigned char str_src_ip[32] = {0}, str_dest_ip[32] = {0},str_mac1[128]={0}, str_mac2[128]={0};
    unsigned char str_protocol[128], str_protocol2[128],str_dest_port[32], str_src_port[32], Str_Protocol[128], Str_Protocol2[128];
    unsigned char str_src_mac[128], str_dest_mac[128];
    unsigned char str_interf[20];
	unsigned char str_ack[32],str_ipL[32],str_dscp[32],str_pri[32],str_vid[64],str_vlan[64];
	unsigned char invert_flag[8] ={0},mask[8]={0};
	unsigned char str_eth[64],str_ssap[32],str_dsap[32],str_crtl[32],str_oui[32];
	unsigned char str_snap[160]={0};
	unsigned int maskLen=0;
	int mark_value[4]={0};
	int ClassQueue = -1;
	int queueFlag=0;
	unsigned char queueId[8];
	unsigned char command[512]={0};
	unsigned char l2_cmd[512]={0};
	unsigned char l2_match[512]={0};
	unsigned char match_cmd[512]={0};
	unsigned char l3_match[1024]={0}, l3_match2[1024] = {0};
	unsigned char l3_cmd[512]={0}, l3_cmd2[512] = {0};
	unsigned char mark_match[32]={0};
	unsigned char policer_cmd[256]={0};
	int order[MAX_QOS_CLASS_NUM+1]={0};
	int class_qid[MAX_QOS_QUEUE_NUM+1]={0};
	int up_mark_handle;
	int down_mark_handle;
	int tmp=-1;
	int uplink=0,downlink=0;
	struct in_addr * ipaddr=NULL;
	int valid_num = 0;

	apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	
	printf("--set qos classify Rule :%d!\n",entryNum);
	
	valid_num = rtl_processClassifyOrder(order);

    if(valid_num == 0)
    {
        fprintf(stderr, "no valid classify entry!\n");
        goto out;
    }
	
	for (index=1; index<=valid_num; index++) 
	{
			printf("\nclassify table index:%d orderindex:%d [%s]:[%d]\n",order[index],index,__FUNCTION__,__LINE__);
			memset(&class_entry, '\0', sizeof(class_entry));
			downlink=uplink=queueFlag=0;
			tmp =-1;
			
			*((char *)&class_entry) = (char)(order[index]);
			apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
			
			/*interface*/

		 	if(strlen(class_entry.ClassInterface)){
				if(strstr(class_entry.ClassInterface, "WAN")){
					sprintf(str_interf, "-i %s",wan_interface  ); 
					downlink=1;
					
				}	
		        else if(strstr(class_entry.ClassInterface, "LAN")){
		            sprintf(str_interf, "-i %s",lan_interface  );  
					uplink=1;
					
	        	}
	 		}

	        else{
	            //sprintf(str_interf, "%s", "");
				downlink=uplink=1;
        	}	
			//printf("intf:%s,[%s]:[%d]\n",str_interf,__FUNCTION__,__LINE__);
	        /*dip*/    
			ipaddr=((struct in_addr *)class_entry.DestIP);
	        if(ipaddr->s_addr)//DestIP is not empty
	        {
	        	
	            strcpy(tmp_str1,inet_ntoa(*((struct in_addr*)class_entry.DestIP)));
				if(class_entry.DestIPExclude)// exclude Dest IP
				{
					sprintf(invert_flag,"%s",NOT);
				}	
				else	
					sprintf(invert_flag,"%s","");
				
				if(strlen(class_entry.DestMask))//DestMask is not empty
				{
					strcpy(tmp_str2,inet_ntoa(*((struct in_addr*)class_entry.DestMask)));
					maskLen=get_netmask_length(tmp_str2);
					sprintf(mask,"/%d",maskLen);
					if(maskLen<=32)
						sprintf(mask,"/%d",maskLen);
					else	
						sprintf(mask,"%s","");
				}
				else
					sprintf(mask,"%s","");
				
				sprintf(str_dest_ip, "%s %s %s%s",invert_flag, _dest, tmp_str1,mask);   
	        }
	        else
	            sprintf(str_dest_ip, "%s", "");
			
			//printf("str_dest_ip:%s,[%s]:[%d]\n",str_dest_ip,__FUNCTION__,__LINE__);
			ipaddr=((struct in_addr *)class_entry.SourceIP);
			
	        /*sip*/
	        if(ipaddr->s_addr)//SourceIP is not empty
	        {
	            strcpy(tmp_str1, inet_ntoa(*((struct in_addr*)class_entry.SourceIP)));
				if(class_entry.SourceIPExclude)// exclude Source IP
				{
					sprintf(invert_flag,"%s",NOT);
				}	
				else	
					sprintf(invert_flag,"%s","");
				
				if(strlen(class_entry.SourceMask))//DestMask is not empty
				{
					strcpy(tmp_str2,inet_ntoa(*((struct in_addr*)class_entry.SourceMask)));
					maskLen=get_netmask_length(tmp_str2);
					if(maskLen<=32)
						sprintf(mask,"/%d",maskLen);
					else	
						sprintf(mask,"%s","");
				}		
				else
					sprintf(mask,"%s","");
				
				sprintf(str_src_ip, "%s %s %s%s",invert_flag, _src, tmp_str1, mask);   
	        }
	        else
	            sprintf(str_src_ip, "%s", "");

			//printf("str_src_ip:%d,%s,[%s]:[%d]\n",strlen(class_entry.SourceIP),str_src_ip,__FUNCTION__,__LINE__);
			
			tmp =-1;	
			
	        /*protocol*/
		    if(class_entry.Protocol == -1)
            {
                if(class_entry.TCPACK)
                {   
                    sprintf(str_protocol, "%s %s", _protocol, _tcp);
                    sprintf(str_protocol2, "%s", "");
                }
                else 
                {
                    if(class_entry.DestPort != -1 || class_entry.SourcePort != -1)
                    {
                        sprintf(str_protocol, "%s %s", _protocol, _tcp);
                        sprintf(str_protocol2, "%s %s", _protocol, _udp);
                    }        
                    else
                    {
                        sprintf(str_protocol, "%s", "");
                        sprintf(str_protocol2, "%s", "");
                    }
                }
            }
            else if(class_entry.Protocol == 6)
            {
                if(class_entry.ProtocolExclude == 0)
                {
                    sprintf(str_protocol, "%s %s", _protocol, _tcp);
                    sprintf(str_protocol2, "%s", "");
                }
                else
                {   
                    if(class_entry.DestPort != -1 || class_entry.SourcePort != -1)
                    {
                        sprintf(str_protocol, "%s %s", _protocol, _udp);
                        sprintf(str_protocol2, "%s", "");
                    }
                    else
                    {
                        sprintf(str_protocol, "%s %s %s", NOT, _protocol, _tcp);
                        sprintf(str_protocol2, "%s", "");
                    }
                }
            }
            else if(class_entry.Protocol == 17)
            {
                if(class_entry.ProtocolExclude == 0)
                {
                    sprintf(str_protocol, "%s %s", _protocol, _udp);
                    sprintf(str_protocol2, "%s", "");
                }
                else
                {  
                    if(class_entry.DestPort != -1 || class_entry.SourcePort != -1)
                    {
                        sprintf(str_protocol, "%s %s", _protocol, _tcp);
                        sprintf(str_protocol2, "%s", "");
                    }
                    else
                    {
                        if(class_entry.TCPACK)
                        {
                            sprintf(str_protocol, "%s %s",_protocol, _tcp);
                            sprintf(str_protocol2, "%s", "");
                        }
                        else
                        {
                            sprintf(str_protocol, "%s %s %s", NOT, _protocol, _udp);
                            sprintf(str_protocol2, "%s", "");
                        }
                    }
                }
            }
            else
            {
                if(class_entry.ProtocolExclude == 0)
                {
                    sprintf(str_protocol, "%s %d", _protocol, class_entry.Protocol);
                    sprintf(str_protocol2, "%s", "");
                }
                else
                {
                    if(class_entry.TCPACK)
                    {
                        sprintf(str_protocol, "%s %s", _protocol, _tcp);
                        sprintf(str_protocol2, "%s", "");
                    }
                    else 
                    {
                        if(class_entry.DestPort != -1 || class_entry.SourcePort != -1)
                        {
                            sprintf(str_protocol, "%s %s", _protocol, _tcp);
                            sprintf(str_protocol2, "%s %s", _protocol, _udp);
                        }        
                        else
                        {
                            sprintf(str_protocol, "%s %s %d", NOT, _protocol, class_entry.Protocol);
                            sprintf(str_protocol2, "%s", "");
                        }
                    }
                }
            }

           /*Port only for tcp/udp*/
           if(class_entry.Protocol == 6 || class_entry.Protocol == 17 || class_entry.Protocol == -1 || 
            (class_entry.Protocol !=6 && class_entry.Protocol !=17 && class_entry.Protocol !=-1 && class_entry.ProtocolExclude == 1))
            {
                if(class_entry.DestPort == -1)
                {
                    sprintf(str_dest_port, "%s", "");
                }
                else
                {
                    if(class_entry.DestPortExclude)
                    {
                        sprintf(invert_flag,"%s",NOT);
                    }
                    else
                    {
                        sprintf(invert_flag,"%s","");
                    }
                    if(class_entry.DestPortRangeMax>class_entry.DestPort)
                    {
                        sprintf(str_dest_port, "%s %s %d:%d",invert_flag, dport, class_entry.DestPort,class_entry.DestPortRangeMax);
                    }
                    else
                    {
                        sprintf(str_dest_port, "%s %s %d",invert_flag, dport, class_entry.DestPort);
                        printf("DestPortRangeMax is ignored!\n");
                    }
                }
    
                //printf("str_dest_port:%s,[%s]:[%d]\n",str_dest_port,__FUNCTION__,__LINE__);
                if(class_entry.SourcePort == -1)
                {
                    sprintf(str_src_port, "%s", "");
                }
                else
                {
                    if(class_entry.SourcePortExclude)
                    {
                        sprintf(invert_flag,"%s",NOT);
                    }
                    else
                    {
                        sprintf(invert_flag,"%s","");
                    }
                    if(class_entry.SourcePortRangeMax>class_entry.SourcePort)
                    {
                        sprintf(str_src_port, "%s %s %d:%d",invert_flag, sport, class_entry.SourcePort,class_entry.SourcePortRangeMax);
                    }
                    else
                    {
                        sprintf(str_src_port, "%s %s %d", invert_flag, sport, class_entry.SourcePort);
                        printf("SourcePortRangeMax is ignored!\n");
                    }
                }
            }
            else
            {
                if(class_entry.DestPort != -1 || class_entry.SourcePort != -1)
                {
                    printf("invalid protocol for port!\n",class_entry.Protocol);
                    sprintf(str_dest_port, "%s", "");
                    sprintf(str_src_port, "%s", "");
                    //goto errout;
                }
            }
			
			/*TCPACK*/
            if(((class_entry.Protocol == 6)&&(class_entry.ProtocolExclude==0))||(class_entry.Protocol == -1)||
            ((class_entry.Protocol !=6) && (class_entry.Protocol !=-1) && (class_entry.ProtocolExclude != 0)))
            {
                if(class_entry.TCPACK)
                {
                    if(class_entry.TCPACKExclude)
                    {
                        sprintf(str_ack,"%s","--tcp-flags ACK NONE");
                    }
                    else
                    {
                        sprintf(str_ack,"%s","--tcp-flags ACK ACK");
                    }
                }
                else
                {
                    sprintf(str_ack,"%s","");
                }
            }
            else
            {
                if(class_entry.TCPACK)
                {            
                    printf("invalid protocol for tcp ack!\n",class_entry.Protocol);
                    sprintf(str_ack, "%s", "");
                    //goto errout;
                }
            }

            if(strlen(str_protocol))
            {
                sprintf(Str_Protocol, "%s %s %s %s", str_protocol, str_dest_port, str_src_port, str_ack);
                if(strlen(str_protocol2))
                {
                    sprintf(Str_Protocol2, "%s %s %s %s", str_protocol2, str_dest_port, str_src_port, str_ack);
                }
                else
                {
                    sprintf(Str_Protocol2, "%s", "");
                }
            }
            else
            {
                sprintf(Str_Protocol, "%s", "");
                sprintf(Str_Protocol2, "%s", "");
            }
            //fprintf(stderr, "%s:%d, protocol = %s\n", __FUNCTION__, __LINE__,Str_Protocol);

			/*mac*/
			if(((class_entry.DestMACAddress[0]==0)&&(class_entry.DestMACAddress[1]==0)
				&&(class_entry.DestMACAddress[2]==0)&&(class_entry.DestMACAddress[3]==0)
				&&(class_entry.DestMACAddress[4]==0)&&(class_entry.DestMACAddress[5]==0))
				&&((class_entry.SourceMACAddress[0]==0)&&(class_entry.SourceMACAddress[1]==0)
				&&(class_entry.SourceMACAddress[2]==0)&&(class_entry.SourceMACAddress[3]==0)
				&&(class_entry.SourceMACAddress[4]==0)&&(class_entry.SourceMACAddress[5]==0)))
			{
				sprintf(str_mac1,"%s","");
				sprintf(str_mac2,"%s","");
			}
			else
	        {
	        	/*dmac*/
	            //if(strlen(class_entry.DestMACAddress))
	            if(!((class_entry.DestMACAddress[0]==0)&&(class_entry.DestMACAddress[1]==0)
					&&(class_entry.DestMACAddress[2]==0)&&(class_entry.DestMACAddress[3]==0)
					&&(class_entry.DestMACAddress[4]==0)&&(class_entry.DestMACAddress[5]==0)))
	            {   
	            	if(class_entry.DestMACExclude)
	                {
						sprintf(invert_flag,"%s",NOT);
					}
					else
					{
						sprintf(invert_flag,"%s","");
					}
	                sprintf(tmp_str1, "%02x:%02x:%02x:%02x:%02x:%02x",
	                    class_entry.DestMACAddress[0], 
	                    class_entry.DestMACAddress[1], 
	                    class_entry.DestMACAddress[2], 
	                    class_entry.DestMACAddress[3], 
	                    class_entry.DestMACAddress[4],
	                    class_entry.DestMACAddress[5]);
					if(!((class_entry.DestMACMask[0]==0)&&(class_entry.DestMACMask[1]==0)
					&&(class_entry.DestMACMask[2]==0)&&(class_entry.DestMACMask[3]==0)
					&&(class_entry.DestMACMask[4]==0)&&(class_entry.DestMACMask[5]==0)))
					{
						 sprintf(tmp_str2, "%02x:%02x:%02x:%02x:%02x:%02x",
		                    class_entry.DestMACMask[0], 
		                    class_entry.DestMACMask[1], 
		                    class_entry.DestMACMask[2], 
		                    class_entry.DestMACMask[3], 
		                    class_entry.DestMACMask[4],
		                    class_entry.DestMACMask[5]);
						  sprintf(str_dest_mac, "%s %s %s/%s",invert_flag, mac_dst, tmp_str1,tmp_str2); 
					}
					else
	                	sprintf(str_dest_mac, "%s %s %s",invert_flag, mac_dst, tmp_str1); 
					
	                printf("str_dest_mac:%s,[%s]:[%d]\n", str_dest_mac,__FUNCTION__,__LINE__); 
	            }
	            else
	                sprintf(str_dest_mac,"%s","");
				/*smac*/
	            if(!((class_entry.SourceMACAddress[0]==0)&&(class_entry.SourceMACAddress[1]==0)
				&&(class_entry.SourceMACAddress[2]==0)&&(class_entry.SourceMACAddress[3]==0)
				&&(class_entry.SourceMACAddress[4]==0)&&(class_entry.SourceMACAddress[5]==0)))
	            {
	            	if(class_entry.SourceMACExclude)
	                {
						sprintf(invert_flag,"%s",NOT);
					}
					else
					{
						sprintf(invert_flag,"%s","");
					}
					
	                sprintf(tmp_str1, "%02x:%02x:%02x:%02x:%02x:%02x",
		                class_entry.SourceMACAddress[0], 
		                class_entry.SourceMACAddress[1], 
		                class_entry.SourceMACAddress[2], 
		                class_entry.SourceMACAddress[3], 
		                class_entry.SourceMACAddress[4],
		                class_entry.SourceMACAddress[5]);
					
	               
					if(!((class_entry.SourceMACMask[0]==0)&&(class_entry.SourceMACMask[1]==0)
					&&(class_entry.SourceMACMask[2]==0)&&(class_entry.SourceMACMask[3]==0)
					&&(class_entry.SourceMACMask[4]==0)&&(class_entry.SourceMACMask[5]==0)))
					{
						 sprintf(tmp_str2, "%02x:%02x:%02x:%02x:%02x:%02x",
		                    class_entry.SourceMACMask[0], 
		                    class_entry.SourceMACMask[1], 
		                    class_entry.SourceMACMask[2], 
		                    class_entry.SourceMACMask[3], 
		                    class_entry.SourceMACMask[4],
		                    class_entry.SourceMACMask[5]);
						  sprintf(str_src_mac, "%s %s %s/%s",invert_flag, mac_src, tmp_str1,tmp_str2); 
					}
					else
	                	sprintf(str_src_mac, "%s %s %s",invert_flag, mac_src, tmp_str1); 
	            }
	            else
	                sprintf(str_src_mac,"%s","");
				
				printf("str_src_mac:%s,[%s]:[%d]\n", str_src_mac,__FUNCTION__,__LINE__); 
				//iptables -A PREROUTING -t mangle -m mac --mac-source 00:11:22:33:44:55/ff:ff:ff:ff:ff:ff -j MARK --set-mark 13

				if(strlen(str_src_mac))
					sprintf(str_mac1, "%s %s %s", match, _mac, str_src_mac);
				else
					 sprintf(str_mac1, "%s", "");

				if(strlen(str_dest_mac))
					sprintf(str_mac2, "%s %s %s", match, _mac, str_dest_mac);
				else
					sprintf(str_mac2, "%s", "");
	        }
	         
			
			//printf("str_mac:%s,[%s]:[%d]\n",str_mac,__FUNCTION__,__LINE__); 
			
			/*length*/
			if((class_entry.IPLengthMin)||(class_entry.IPLengthMax))
			{
			
                if(class_entry.IPLengthExclude)
                {
                    sprintf(invert_flag,"%s",NOT);
                }
                else
                {
                    sprintf(invert_flag,"%s","");
                }


                if(class_entry.IPLengthMax == 0)
                {
					sprintf(str_ipL,"%s %s %s %s %d:65535",match,"length",invert_flag,"--length",class_entry.IPLengthMin);
                }
                else if(class_entry.IPLengthMin<class_entry.IPLengthMax)
                {
                    //iptables -A PREROUTING -t mangle -m length --length 40 -j MARK --set-mark 13
                    sprintf(str_ipL,"%s %s %s %s %d:%d",match,"length",invert_flag,"--length",class_entry.IPLengthMin,class_entry.IPLengthMax);
                }
                else if(class_entry.IPLengthMin==class_entry.IPLengthMax)
                {
                    sprintf(str_ipL,"%s %s %s %s %d",match,"length",invert_flag,"--length",class_entry.IPLengthMin);
                }
                else
                {
                    printf("IPLengthMax %d should be greater than IPLengthMin %d\n",
                            class_entry.IPLengthMax,class_entry.IPLengthMin);
                    sprintf(str_ipL,"%s","");
                    //goto errout;
                }   
            }
			else
			{
				sprintf(str_ipL,"%s","");
			}
			
			//printf("str_ipL:%s,[%s]:[%d]\n", str_ipL,__FUNCTION__,__LINE__); 
			/*DSCP check*/
			if(class_entry.DSCPCheck!=-1)
			{
				if(class_entry.DSCPExclude)
				{
					sprintf(invert_flag,"%s",NOT);
				}
				else
				{
					sprintf(invert_flag,"%s","");
				}
				//iptables -A PREROUTING -t mangle -m dscp --dscp 40 -j MARK --set-mark 13
				sprintf(str_dscp,"%s %s %s %s %d",match,"dscp",invert_flag,"--dscp",class_entry.DSCPCheck);
			}
			else
			{
				sprintf(str_dscp,"%s","");	
			}
			
			//printf("str_dscp:%s,[%s]:[%d]\n", str_dscp,__FUNCTION__,__LINE__); 
			if(class_entry.EthernetPriorityCheck!=-1)
			{
				/*Ethernet priority check*/
				if((class_entry.EthernetPriorityCheck<=7)&&(class_entry.EthernetPriorityCheck>=0))
				{
					if(class_entry.EthernetPriorityExclude)
					{
						sprintf(invert_flag,"%s",NOT);
					}
					else
					{
						sprintf(invert_flag,"%s","");
					}
					//iptables -A PREROUTING -t mangle -m vlanpriority --prio-value 4 --vid-value 5 -j DROP
					sprintf(str_pri,"%s %s %d",invert_flag,"--prio-value",class_entry.EthernetPriorityCheck);
				}
				else if(class_entry.EthernetPriorityCheck==-1)
				{
					sprintf(str_pri,"%s","");
				}
                else
                {
                    printf(" invalid vlan priority %d\n",class_entry.EthernetPriorityCheck);
						goto errout;
                }

				sprintf(str_vlan,"%s %s %s",match,"vlanpriority",str_pri);
			}
			else
			{
				sprintf(str_vlan,"");
			}

			if(class_entry.VLANIDCheck!=-1)
			{
				if((class_entry.VLANIDCheck>=0)&&(class_entry.VLANIDCheck<4095))
				{
					if(class_entry.VLANIDExclude)
					{
						sprintf(invert_flag,"%s",NOT);
					}
					else
					{
						sprintf(invert_flag,"%s","");
					}
					sprintf(str_vid,"%s %s %s %s %d",match,"vlanid",invert_flag,"--vid-value",class_entry.VLANIDCheck);
				}
			}
			else if(class_entry.VLANIDCheck == -1)
			{
				sprintf(str_vid,"%s","");
			}
            else
            {
			    printf("invalid vlan id %d\n",class_entry.VLANIDCheck);
				goto errout;			
			}  
			
			//printf("str_vlan:%s,[%s]:[%d]\n", str_vlan,__FUNCTION__,__LINE__); 
#if 0//dhcp related optin
		/*identify one or more LAN devices*/
		if(strlen(class_entry.SourceVendorClassID)||strlen(class_entry.DestVendorClassID)
			||strlen(class_entry.SourceClientID)||strlen(class_entry.DestClientID)
			||strlen(class_entry.SourceUserClassID)||strlen(class_entry.DestUserClassID)
			||strlen(class_entry.SourceVendorSpecificInfo)||strlen(class_entry.DestVendorSpecificInfo)	
		)
		{
			char  sip[32];
			char  sip_end[32];
			char  dip[32];
			char  dip_end[32];
			int dhcpFlag=0;
			int dhcpret=0;
			rtl_dhcp_info dhcp_info;
			memset(&dhcp_info,0,sizeof(rtl_dhcp_info));
			
			if(strlen(class_entry.SourceVendorClassID)
				||strlen(class_entry.SourceClientID)
				||strlen(class_entry.SourceUserClassID)
				||strlen(class_entry.SourceVendorSpecificInfo))
			{
				if(strlen(class_entry.SourceVendorClassID)){
					strcpy(dhcp_info.VendorClassID,class_entry.SourceVendorClassID);
					strcpy(dhcp_info.VendorClassIDMode,class_entry.SourceVendorClassIDMode);
					dhcpFlag |=DHCP_VENDORCLASSID;
				}	
				if(strlen(class_entry.SourceClientID)){
					strcpy(dhcp_info.ClientID,class_entry.SourceClientID);
					
					dhcpFlag |=DHCP_CLIENTID;
				}
				if(strlen(class_entry.SourceUserClassID)){
					strcpy(dhcp_info.UserClassID,class_entry.SourceUserClassID);
				
					
					dhcpFlag |=DHCP_USERCLASSID;
				}	
				if(strlen(class_entry.SourceVendorSpecificInfo)){
					strcpy(dhcp_info.VendorSpecificInfo,class_entry.SourceVendorSpecificInfo);
					strcpy(dhcp_info.VendorSpecificInfoMode,class_entry.SourceVendorSpecificInfoMode);
					
					dhcpFlag |=DHCP_VENDORSPECIFICINfO;
				}
				dhcpret=rtl_process_classifyByDhcpOption(&dhcp_info,dhcpFlag,sip,sip_end);
				if(dhcpret)
				{
					if(((struct in_addr *)class_entry.SourceIP)->s_addr==0)//SourceIP empty
					{
						
						sprintf(str_src_ip, "%s %s %s %s %s", match, ip_range, src_rnage,sip,sip_end);
					}
					else
					{
						
					}
				}

			}
			if(strlen(class_entry.DestVendorClassID)
				||strlen(class_entry.DestClientID)
				||strlen(class_entry.DestUserClassID)
				||strlen(class_entry.DestVendorSpecificInfo))
			{
				if(strlen(class_entry.DestVendorClassID)){
					strcpy(dhcp_info.VendorClassID,class_entry.DestVendorClassID);
					strcpy(dhcp_info.VendorClassIDMode,class_entry.DestVendorClassIDMode);
					dhcpFlag |=DHCP_VENDORCLASSID;
				}	
				if(strlen(class_entry.DestClientID)){
					strcpy(dhcp_info.ClientID,class_entry.DestClientID);
					
					dhcpFlag |=DHCP_CLIENTID;
				}
				if(strlen(class_entry.DestUserClassID)){
					strcpy(dhcp_info.UserClassID,class_entry.DestUserClassID);
				
					
					dhcpFlag |=DHCP_USERCLASSID;
				}	
				if(strlen(class_entry.DestVendorSpecificInfo)){
					strcpy(dhcp_info.VendorSpecificInfo,class_entry.DestVendorSpecificInfo);
					strcpy(dhcp_info.VendorSpecificInfoMode,class_entry.DestVendorSpecificInfoMode);
					
					dhcpFlag |=DHCP_VENDORSPECIFICINfO;
				}
				dhcpret=rtl_process_classifyByDhcpOption(&dhcp_info,dhcpFlag,dip,dip_end);
				
				if(dhcpret)
				{
					if(((struct in_addr *)class_entry.DestIP)->s_addr==0)//destIP empty
					{
						
						sprintf(str_src_ip, "%s %s %s %s %s", match, ip_range, dst_rnage,dip,dip_end);
					}
					else
					{
						
					}
				}

			}
		}
#endif
    
#define ETHERNET_TYPE	0
#define SNAP_TYPE		1
		
//        fprintf(stderr, "%s:%d, tmp = %d, str_eth = %s\n", __FUNCTION__, __LINE__,tmp,str_eth);
		/*ethernet type*/
//        fprintf(stderr, "%s:%d, ethertype = %d\n", __FUNCTION__, __LINE__, class_entry.Ethertype);
        tmp = -1;
		if(class_entry.Ethertype!=-1)
		{
			
			if(class_entry.EthertypeExclude)
			{
				
				if(class_entry.Ethertype==ETHERNET_TYPE)
				{
					tmp =SNAP_TYPE;
				}
				else if(class_entry.Ethertype==SNAP_TYPE)
				{
					tmp =ETHERNET_TYPE;
				}
			}
			else
			{
				if(class_entry.Ethertype==ETHERNET_TYPE)
				{
					tmp =ETHERNET_TYPE;
				}
				else if(class_entry.Ethertype==SNAP_TYPE)
				{
					tmp =SNAP_TYPE;
				}
				
			}	
			
			if(tmp==ETHERNET_TYPE)
			{
                fprintf(stderr, "%s:%d, tmp = %d, str_eth = %s\n", __FUNCTION__, __LINE__,tmp,str_eth);
				sprintf(str_eth,"%s %s %s %s %s", match,_IEEE_802_3, NOT, _etype, "LENGTH");
				
			}
			else if(tmp==SNAP_TYPE)
			{
			
                fprintf(stderr, "%s:%d, tmp = %d, str_eth = %s\n", __FUNCTION__, __LINE__,tmp,str_eth);
				sprintf(str_eth,"%s %s %s %s",match,_IEEE_802_3, _etype, "LENGTH");
			}
            else
            {
				sprintf(str_eth,"%s","");
                printf("Ethertype %d is wrong and Ethertype is ignored!\n",class_entry.Ethertype);
            }
				
		}
		else
		{
			sprintf(str_eth,"%s","");	
		}

        fprintf(stderr, "%s:%d, tmp = %d, str_eth = %s\n", __FUNCTION__, __LINE__,tmp,str_eth);
		if(tmp==SNAP_TYPE||tmp == -1)
		{
			/*SSAP and DSAP*/
			if((class_entry.SSAP!=-1)||(class_entry.DSAP!=-1))
			{
			    if(class_entry.SSAP != -1)
                {         
			        if((class_entry.DSAP !=-1)&&((class_entry.SSAP != class_entry.DSAP)||(class_entry.SSAPExclude != class_entry.DSAPExclude)))
                    {
                        printf("SSAP and DSAP must be same and DSAP is ignored!\n");
                    }
				    if(class_entry.SSAPExclude )
				    {
					    sprintf(invert_flag,"%s",NOT);
				    }
				    else
				    {
					    sprintf(invert_flag,"%s","");
				    }	
				
				    sprintf(str_ssap,"%s %s %d",invert_flag,_sap,class_entry.SSAP);
			    }
                else
                {
				    if(class_entry.DSAPExclude )
				    {
					    sprintf(invert_flag,"%s",NOT);
				    }
				    else
				    {
					    sprintf(invert_flag,"%s","");
				    }	
				
				    sprintf(str_ssap,"%s %s %d",invert_flag,_sap,class_entry.DSAP);
                }
             }
			else
			{
				sprintf(str_ssap,"%s","");
				
			}
			
			/*llc control*/
			if((class_entry.LLCControl!=-1))
			{
				if(class_entry.LLCControlExclude)
				{
					sprintf(invert_flag,"%s",NOT);
				}
				else
				{
					sprintf(invert_flag,"%s","");
				}	
				
				sprintf(str_crtl,"%s %s %d",invert_flag,_ctrl,class_entry.LLCControl);

			}
			else
			{
				sprintf(str_crtl,"%s","");
				
			}
			/*SNAPOUI*/
			if((class_entry.SNAPOUI!=-1))
			{
				if(class_entry.SNAPOUIExclude)
				{
					sprintf(invert_flag,"%s",NOT);
				}
				else
				{
					sprintf(invert_flag,"%s","");
				}	
			
				sprintf(str_oui,"%s %s %d",invert_flag,_oui,class_entry.SNAPOUI);
						
			}
			else
			{
				sprintf(str_oui,"%s","");
				
			}
			if((tmp == -1)&&(strlen(str_ssap)||strlen(str_crtl)||strlen(str_oui)))
				sprintf(str_snap,"%s %s %s %s %s %s %s",match, _IEEE_802_3, _etype, "LENGTH", str_ssap,str_crtl,str_oui);
			else if(tmp==SNAP_TYPE) 
				sprintf(str_snap,"%s %s %s",str_ssap,str_crtl,str_oui);
            else
                sprintf(str_snap,"%s","");
			
		}
		else
			sprintf(str_snap,"");
        
        fprintf(stderr, "%s:%d, tmp = %d, str_snap = %s\n", __FUNCTION__, __LINE__,tmp,str_snap);

        if(strlen(str_dest_ip) != 0 ||strlen(str_src_ip) != 0 || strlen(Str_Protocol) != 0 ||
           strlen(str_ipL) != 0 || strlen(str_dscp) != 0 || 
           strlen(str_vlan) != 0 || strlen(str_vid) != 0 || strlen(str_eth) != 0 || strlen(str_snap) != 0)
        {
			sprintf(l3_match,"%s %s %s %s %s %s %s %s",str_dest_ip, str_src_ip, 
                	Str_Protocol,str_ipL,str_dscp,str_vlan, str_eth, str_snap);
			
			if(strlen(Str_Protocol) != 0 &&strlen(Str_Protocol2) != 0)
            {
                sprintf(l3_match2,"%s %s %s %s %s %s %s %s",str_dest_ip, str_src_ip, 
                    Str_Protocol2,str_ipL,str_dscp,str_vlan, str_eth, str_snap);
            }
            else
            {
                 sprintf(l3_match2, "%s", "");
            }   

        }
  		else
		{
		    sprintf(l3_match, "%s", "");
			sprintf(l3_match2, "%s", "");
		}
		printf("l3_match1 Command = %s--[%s]:[%d]--\n",l3_match,__FUNCTION__,__LINE__);
        printf("l3_match2 Command = %s--[%s]:[%d]--\n",l3_match2,__FUNCTION__,__LINE__);
		
		if(class_entry.ForwardingPolicy)
		{
			//be referenced in the Layer3Forwarding table
			
		}
        
        //printf("ClassQueue:%d[%s]:[%d].\n",ClassQueue,__FUNCTION__,__LINE__);
        if((class_entry.TrafficClass==-1)&&(class_entry.ClassQueue==-1)&&(class_entry.ClassApp==-1))
		{
			printf("One of TrafficClass,ClassQueue and ClassApp MUST be specified\n");
			goto errout;
		}
        else if(((class_entry.TrafficClass!=-1)&&(class_entry.ClassQueue!=-1))||((class_entry.ClassQueue!=-1)&&(class_entry.ClassApp!=-1))||((class_entry.TrafficClass!=-1)&&(class_entry.ClassApp!=-1)))
        {
			printf("Only of TrafficClass,ClassQueue and ClassApp SHOULD be specified\n");
            goto errout;
        }

        
		if((class_entry.ClassQueue!=-1)||(class_entry.TrafficClass!=-1))
		{   
		    ClassQueue = -1;
			//classify to a queue
			if(class_entry.ClassQueue!=-1)
			{
				//classify to policer
				if(class_entry.ClassPolicer!=-1)
				{
					rtl_process_ClassPolicer(class_entry.ClassPolicer,policer_cmd);
				}
				else
				{
                    sprintf(policer_cmd,"%s","");
				}
				ClassQueue=class_entry.ClassQueue;
				goto process;
				
			}
			else if(class_entry.TrafficClass!=-1)
			{
			    
				rtl_find_qid_by_trafficClass(class_entry.TrafficClass,class_qid);
				for (j=1;j<MAX_QOS_QUEUE_NUM+1;j++)
				{
				    //fprintf(stderr, "%s:%d, j = %d, class_qid = %d\n",__FUNCTION__,__LINE__, j, class_qid[j]);
					if(class_qid[j])
					{
						/*qid: j*/
						ClassQueue= j;
						goto process;
					}
				}
				
			}
process:
			{
				printf("ClassQueue:%d[%s]:[%d].\n",ClassQueue,__FUNCTION__,__LINE__);
				queueFlag=rtl_getQueueFlag(ClassQueue);
                dscp_match[index][0] = queueFlag;
				//uplink engress wan intf
				if((uplink &&(queueFlag&WAN_FLAG))||(downlink &&(queueFlag&LAN_FLAG)))
                {
                    if(uplink &&(queueFlag&WAN_FLAG)) 
                    {
                        up_mark_handle= (index<<2)+2;
                        dscp_match[index][1] = up_mark_handle;
                        
                        fprintf(stderr, "%s:%d, up_mark_handle = %d, index = %d\n", __FUNCTION__, __LINE__, up_mark_handle, index);

						if( (strlen(l3_match) || strlen(l3_match2) || strlen(str_mac1)) && strlen(str_mac2) )	// pre & post
						{
							if(strlen(l3_match))
							{
								if(strlen(str_mac1)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
			                            Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match, str_mac1, jump, MARK, set_mark, up_mark_handle-2);
			                    }
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
			                            Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match, jump, MARK, set_mark, up_mark_handle-2);
								}

								fprintf(stderr, "uplink l3 command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
		                        system(command);

								if(strlen(str_protocol)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d", 
	                                    Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", wan_interface,str_protocol, "-m", mark, _mark, up_mark_handle-2, str_mac2, jump, MARK, set_mark, up_mark_handle);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d", 
	                                    Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", wan_interface, "-m", mark, _mark, up_mark_handle-2, str_mac2, jump, MARK, set_mark, up_mark_handle);
								}
								
								printf("DestMACAddress command1 = %s\n", command);
                            	system(command);
								
							}
							
	                        if(strlen(l3_match2))
	                        {
								if(strlen(str_mac1)){
		                            sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
		                                    Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match2, str_mac1, jump, MARK, set_mark, up_mark_handle-2);
		                        }
								else{
									 sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
		                                    Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match2, jump, MARK, set_mark, up_mark_handle-2);
								}

								printf("uplink l3 command2 = %s\n", command);
	                            system(command);

								if(strlen(str_protocol2)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d", 
										Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", wan_interface,str_protocol2, "-m", mark, _mark, up_mark_handle-2, str_mac2, jump, MARK, set_mark, up_mark_handle);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d", 
										Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", wan_interface, "-m", mark, _mark, up_mark_handle-2, str_mac2, jump, MARK, set_mark, up_mark_handle);
								}
								
								printf("DestMACAddress command2 = %s\n", command);
								system(command);
	                        }

							if(strlen(str_mac1) && !strlen(l3_match) && !strlen(l3_match2)){
							    sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
									Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface, str_mac1, jump, MARK, set_mark, up_mark_handle-2);

								printf("uplink l3 command2 = %s\n", command);
	                            system(command);

								sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d", 
										Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", wan_interface, "-m", mark, _mark, up_mark_handle-2, str_mac2, jump, MARK, set_mark, up_mark_handle);
								printf("DestMACAddress command2 = %s\n", command);
								system(command);
							}							
						}
						else if(strlen(str_mac2))	// post
						{					
							sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
                                    Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", wan_interface, str_mac2, jump, MARK, set_mark, up_mark_handle);
							printf("DestMACAddress command1 = %s\n", command);
                        	system(command);

						}
						else	//pre
						{
							if(strlen(l3_match))
							{
								if(strlen(str_mac1)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
		                            Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match, str_mac1, jump, MARK, set_mark, up_mark_handle);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
		                            Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match, jump, MARK, set_mark, up_mark_handle);
								}
								
								fprintf(stderr, "uplink l3 command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
		                        system(command);
							}
							
	                        if(strlen(l3_match2))
	                        {
								if(strlen(str_mac1)){
									 sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
	                                    Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match2, str_mac1, jump, MARK, set_mark, up_mark_handle);
								}
								else{
									 sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
	                                    Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface,l3_match2, jump, MARK, set_mark, up_mark_handle); 
								}
	                            printf("uplink l3 command2 = %s\n", command);
	                            system(command);
	                        }

							if(strlen(str_mac1) && !strlen(l3_match) && !strlen(l3_match2)){
								sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
	                                    Iptables, ADD, PREROUTING, _table, mangle_table, "-i", lan_interface, str_mac1, jump, MARK, set_mark, up_mark_handle);
								printf("uplink l3 command2 = %s\n", command);
	                            system(command);
							}
						}

                        sprintf(queueId,"%d:%d",WAN_FLAG,(ClassQueue+2));
						
                        //to-do: policer process
                        if(strlen(l3_match) || strlen(l3_match2) || strlen(str_mac2) || strlen(str_mac1))
                        {
	                        sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d %s %s %s", 
	                                _tc, _filter, _add, _dev, wan_interface, _parent, "2:0", _protocol2, _ip, _prio, "100",_handle,up_mark_handle, _fw, _classid,queueId );
	                            
	                        fprintf(stderr, "uplink filter command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
	                        system(command);
                        }
                    }
                    if(downlink &&(queueFlag&LAN_FLAG))
                    {
                        down_mark_handle= (index<<2)+1;
                        dscp_match[index][2] = down_mark_handle;
                        
                        fprintf(stderr, "%s:%d, down_mark_handle = %d, index = %d\n", __FUNCTION__, __LINE__, down_mark_handle, index);

						if( (strlen(l3_match) || strlen(l3_match2) || strlen(str_mac2)) && strlen(str_mac1) )   // pre & post
						{							
							if(strlen(l3_match))
							{
								if(strlen(str_protocol)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
											Iptables, ADD, PREROUTING, _table, mangle_table, "-i", wan_interface,str_protocol, str_mac1, jump, MARK, set_mark, down_mark_handle -2);
								}	
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
											Iptables, ADD, PREROUTING, _table, mangle_table, "-i", wan_interface, str_mac1, jump, MARK, set_mark, down_mark_handle -2);
								}

								printf("DestMACAddress command2 = %s\n", command);
	                            system(command);

 								if(strlen(str_mac2)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %s %d",				 
		                            	Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", lan_interface,"-m", mark, _mark, down_mark_handle-2, l3_match, str_mac2, jump, MARK, set_mark, down_mark_handle);
 								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d",				 
		                            	Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", lan_interface,"-m", mark, _mark, down_mark_handle-2, l3_match, jump, MARK, set_mark, down_mark_handle);
								}
								fprintf(stderr, "down l3 command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
	                        	system(command);
							}
							
	                        if(strlen(l3_match2))
	                        {
								if(strlen(str_protocol2)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
                                   		Iptables, ADD, PREROUTING, _table, mangle_table, "-i", wan_interface,str_protocol2,  str_mac1, jump, MARK, set_mark, down_mark_handle-2);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
                                   		Iptables, ADD, PREROUTING, _table, mangle_table, "-i", wan_interface,  str_mac1, jump, MARK, set_mark, down_mark_handle-2);
								}

								printf("DestMACAddress command3 = %s\n", command);
                            	system(command);

								if(strlen(str_mac2)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %s %d",
										Iptables, ADD, POSTROUTING, _table, mangle_table,"-o", lan_interface, "-m", mark, _mark, down_mark_handle-2, l3_match2, str_mac2, jump, MARK, set_mark, down_mark_handle);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d",
										Iptables, ADD, POSTROUTING, _table, mangle_table,"-o", lan_interface, "-m", mark, _mark, down_mark_handle-2, l3_match2, jump, MARK, set_mark, down_mark_handle);
								}
	                            printf("downlink l3 command2 = %s\n", command);
	                            system(command);

								
	                        }

							if(strlen(str_mac2) && !strlen(l3_match) && !strlen(l3_match2)){
								sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
                                   		Iptables, ADD, PREROUTING, _table, mangle_table, "-i", wan_interface, str_mac1, jump, MARK, set_mark, down_mark_handle-2);
								printf("DestMACAddress command1 = %s\n", command);
                        		system(command);
								
								sprintf(command, "%s %s %s %s %s %s %s %s %s %s %d %s %s %s %s %d",
										Iptables, ADD, POSTROUTING, _table, mangle_table,"-o", lan_interface, "-m", mark, _mark, down_mark_handle-2, str_mac2, jump, MARK, set_mark, down_mark_handle);
								 printf("downlink l3 command2 = %s\n", command);
	                            system(command);
							}
							
						}
						else if(strlen(str_mac1))	// pre
						{
							sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
                                    Iptables, ADD, PREROUTING, _table, mangle_table, "-i", wan_interface, str_mac1, jump, MARK, set_mark, down_mark_handle);
                            printf("DestMACAddress command1 = %s\n", command);
                            system(command);
						}
						else	// post
						{
							if(strlen(l3_match))
							{
								if(strlen(str_mac2)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
	                            	Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", lan_interface,l3_match, str_mac2, jump, MARK, set_mark, down_mark_handle);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
	                            	Iptables, ADD, POSTROUTING, _table, mangle_table, "-o", lan_interface,l3_match, jump, MARK, set_mark, down_mark_handle);
								}
								fprintf(stderr, "down l3 command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
	                        	system(command);
							}
							
	                        if(strlen(l3_match2))
	                        {
								if(strlen(str_mac2)){
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d", 
										Iptables, ADD, POSTROUTING, _table, mangle_table,"-o", lan_interface, l3_match2, str_mac2, jump, MARK, set_mark, down_mark_handle);
								}
								else{
									sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
										Iptables, ADD, POSTROUTING, _table, mangle_table,"-o", lan_interface, l3_match2, jump, MARK, set_mark, down_mark_handle);
								}
	                           
	                            printf("downlink l3 command2 = %s\n", command);
	                            system(command);
	                        }

							if(strlen(str_mac2) && !strlen(l3_match) && !strlen(l3_match2)){

								sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %d", 
										Iptables, ADD, POSTROUTING, _table, mangle_table,"-o", lan_interface, str_mac2, jump, MARK, set_mark, down_mark_handle);
	                           
	                            printf("downlink l3 command3 = %s\n", command);
	                            system(command);
								
							}
						}
				
                        sprintf(queueId,"%d:%d",LAN_FLAG,(ClassQueue+2));
                        //to-do: polilcer process
                        if(strlen(l3_match) || strlen(l3_match2) || strlen(str_mac1) || strlen(str_mac2))
                        {
							sprintf(command, "%s %s %s %s %s %s %s %s %s %s %s %s %d %s %s %s", 
                                _tc, _filter, _add, _dev, lan_interface, _parent, "1:0", _protocol2, _ip, _prio, "100",_handle,down_mark_handle, _fw, _classid,queueId );
                            
	                        fprintf(stderr, "downlink filter command = %s[%s:%d]\n", command, __FUNCTION__, __LINE__);
	                        system(command);
                        }
                    }
                }
                else
				{
					printf("\nuplink:%d,downlink:%d,queueFlag:%d,find no queue related to classify!\n",uplink,downlink,queueFlag);
				}

                dscp_match[index][3] = 0;
                dscp_match[index][4] = 0;
                dscp_match[index][5] = 0;
		    }
        }
		else if(class_entry.ClassApp!=-1)
		{
			rtl_process_ClassAPP(index,uplink,downlink,l3_match,l3_match2,class_entry.ClassApp,dscp_match[index]);
		}
	}

out:
	return;
errout:
	printf("invalid argument!\n");
	goto out;
}

void set_PriorityforTr069(int dscp_match[MAX_QOS_CLASS_NUM+1][6])
{
    int valid_num = 0, entryNum, index;
    int app_index = -1;
    TR098_APPCONF_T app_entry;
    QOSCLASS_T class_entry;
    int order[MAX_QOS_CLASS_NUM+1] = {0};
    int downlink = 0, uplink = 0;

    apmib_get(MIB_QOS_CLASS_TBL_NUM, (void *)&entryNum);
	
	printf("----set dscp and ethernet priority----\n",entryNum);
	
	valid_num = rtl_processClassifyOrder(order);

    if(valid_num == 0)
    {
        fprintf(stderr, "no valid classify entry!\n");
        goto out;
    }
	
    for(index = 1; index<=valid_num; index ++)
    {
//        printf("\n set dscp mark for classify table entry %d[%s]:[%d]\n",order[index],__FUNCTION__,__LINE__);
	    memset(&class_entry, '\0', sizeof(class_entry));
            
	    *((char *)&class_entry) = (char)(order[index]);
	    apmib_get(MIB_QOS_CLASS_TBL, (void *)&class_entry);
        
        if(strstr(class_entry.ClassInterface, "WAN")){
            //sprintf(str_interf, "-i %s",wan_interface  ); 
            downlink=1;
        }   
        else if(strstr(class_entry.ClassInterface, "LAN")){
            //sprintf(str_interf, "-i %s",lan_interface  );  
            uplink=1;
        }
        else{
            //sprintf(str_interf, "%s", "");
            downlink=uplink=1;
        } 
//        fprintf(stderr, "uplink = %d, downlink = %d\n", uplink, downlink,__FUNCTION__,__LINE__);
//        fprintf(stderr, "dscp_match = %d, %d, %d, %d, %d, %d, [%s:%d]\n", 
//                dscp_match[index][0],dscp_match[index][1],dscp_match[index][2],dscp_match[index][3],dscp_match[index][4],dscp_match[index][5],
//                __FUNCTION__, __LINE__);
        if(uplink&&(dscp_match[index][0]&WAN_FLAG))
        {
            if(class_entry.DSCPMark == -2 && class_entry.EthernetPriorityMark != -2)
            {
                rtl_set_dscp(class_entry.DSCPMark, dscp_match[index][1], WAN_FLAG);
                rtl_set_vlanprio(class_entry.EthernetPriorityMark,dscp_match[index][1], WAN_FLAG);
            }
            else if(class_entry.DSCPMark != -2)
            {
                rtl_set_vlanprio(class_entry.EthernetPriorityMark,dscp_match[index][1], WAN_FLAG);
                rtl_set_dscp(class_entry.DSCPMark, dscp_match[index][1], WAN_FLAG);
            }
            else
            {
                fprintf(stderr, "DSCPmark and Ethernet Priority MUST NOT both be -2!\n");
            }
        }
        if(downlink&&(dscp_match[index][0]&LAN_FLAG))
        {
            if(class_entry.DSCPMark == -2 && class_entry.EthernetPriorityMark != -2)
            {
                rtl_set_dscp(class_entry.DSCPMark, dscp_match[index][2], LAN_FLAG);
                rtl_set_vlanprio(class_entry.EthernetPriorityMark,dscp_match[index][2], LAN_FLAG);
            }
            else if(class_entry.DSCPMark != -2)
            {
                rtl_set_vlanprio(class_entry.EthernetPriorityMark,dscp_match[index][2], LAN_FLAG);
                rtl_set_dscp(class_entry.DSCPMark, dscp_match[index][2], LAN_FLAG);
            }
            else
            {
                fprintf(stderr, "DSCPmark and Ethernet Priority MUST NOT both be -2!\n");
            }
        }

        if(class_entry.ClassApp!=-1)
        {
            app_index = class_entry.ClassApp;
            
            memset(&app_entry, '\0', sizeof(app_entry));
            
                
            *((char *)&app_entry) = (char)(app_index);
            apmib_get(MIB_TR098_QOS_APP_TBL, (void *)&app_entry);
            
            if(uplink&&(dscp_match[index][3]&WAN_FLAG))
            {
                if(app_entry.default_dscp_mark == -2 && app_entry.default_8021p_mark != -2)
                {
                    rtl_set_dscp(app_entry.default_dscp_mark, dscp_match[index][4], WAN_FLAG);
                    rtl_set_vlanprio(app_entry.default_8021p_mark,dscp_match[index][4], WAN_FLAG);
                }
                else if(app_entry.default_dscp_mark != -2)
                {
                    rtl_set_vlanprio(app_entry.default_8021p_mark,dscp_match[index][4], WAN_FLAG);
                    rtl_set_dscp(app_entry.default_dscp_mark, dscp_match[index][4], WAN_FLAG);
                }
                else
                {
                    fprintf(stderr, "DSCPmark and Ethernet Priority MUST NOT both be -2!\n");
                }
            }
            if(downlink&&(dscp_match[index][3]&LAN_FLAG))
            {
                if(app_entry.default_dscp_mark == -2 && app_entry.default_8021p_mark != -2)
                {
                    rtl_set_dscp(app_entry.default_dscp_mark, dscp_match[index][5], LAN_FLAG);
                    rtl_set_vlanprio(app_entry.default_8021p_mark,dscp_match[index][5], LAN_FLAG);
                }
                else if(app_entry.default_dscp_mark != -2)
                {
                    rtl_set_vlanprio(app_entry.default_8021p_mark,dscp_match[index][5], LAN_FLAG);
                    rtl_set_dscp(app_entry.default_dscp_mark, dscp_match[index][5], LAN_FLAG);
                }
                else
                {
                    fprintf(stderr, "DSCPmark and Ethernet Priority MUST NOT both be -2!\n");
                }
            }
        }

    }
out:
    return;
errout:
    printf("invalid argument!\n");
    goto out;

}

void set_QosRuleforTr069(char *interface, char *wan_addr)
{
	int enabled=0;
	int QoS_Enabled=0;
	char *br_interface="br0";
	char *pInterface="eth1";
	char PROC_QOS[128]={0};
    int dscp_match[MAX_QOS_CLASS_NUM+1][6];
    
	printf("\n---set_QosRuleforTr069!\n");
	#if defined (CONFIG_APP_EBTABLES)
	//ebtables -F
	RunSystemCmd(NULL_FILE, Ebtables, FLUSH, NULL_STR);
	#endif
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, X, _table, mangle_table, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, Z, _table, mangle_table, NULL_STR);
	
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", br_interface, "root", NULL_STR);

	//To avoid rule left when wan changed
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", pInterface, "root", NULL_STR);
	RunSystemCmd(NULL_FILE, "tc", "qdisc", "del", "dev", "ppp0", "root", NULL_STR);
	sprintf(PROC_QOS, "%s", "0,");


	apmib_get(MIB_QOS_ENABLE, (void *)&enabled);
	apmib_get( MIB_QOS_ENABLED, (void *)&QoS_Enabled);
	if(enabled||QoS_Enabled)
	{
		sprintf(PROC_QOS, "%s", "1,");
	}

	// echo /proc/qos should before tc rules because of qos patch (CONFIG_RTL_QOS_PATCH in kernel)
	RunSystemCmd("/proc/qos", "echo", PROC_QOS, NULL_STR);
	if(enabled)
	{
		rtl_process_ClassQueue();
		set_classifyRuleforTr069(interface,wan_addr,dscp_match);
		rtl_process_dfltClassifyFortr069(interface, wan_addr);
		set_PriorityforTr069(dscp_match);
	}
} 
#endif

#if defined(CONFIG_APP_TR069)
void SetRuleFortr069(char *interface, char *wan_addr)
{
	int cwmp_flag = 0;
	int conReqPort = 0;
	char acsUrl[CWMP_ACS_URL_LEN+1] = {0};
	char acsUrlRange[2*(CWMP_ACS_URL_LEN+1)] = {0};
	char conReqPortRange[2*(5+1)] = {0};
	int qosFlag = -1;
//	char strPID[10];
//	int pid=-1;
		
//printf("\r\n wan_addr=[%s],__[%s-%u]\r\n",wan_addr,__FILE__,__LINE__);
	apmib_get( MIB_CWMP_FLAG, (void *)&cwmp_flag );
#if 0	 //disabled Since webpage form handler has modified
	if(isFileExist(TR069_PID_FILE))
	{
		
		
	}
	else
	{
		if(cwmp_flag & CWMP_FLAG_AUTORUN)
		{
			unsigned char acsUrltmp[CWMP_ACS_URL_LEN+1];
			unsigned char *notifyList;
	#if 0
			notifyList=malloc(CWMP_NOTIFY_LIST_LEN);
			
			if(notifyList==NULL)
			{
				fprintf(stderr,"\r\n ERR:notifyList malloc fail! __[%s-%u]",__FILE__,__LINE__);
			}
			else
			{
				char *lineptr = NULL;
				char *str;
				int firstline = 1;
				
				
				memset(notifyList,0x00,CWMP_NOTIFY_LIST_LEN);
				apmib_get(MIB_CWMP_NOTIFY_LIST,(void *)notifyList);
				
				if(strlen(notifyList) == 0)
				{
					system("echo \"\" > /var/CWMPNotify.txt");
					
				}
				else
				{
				
					lineptr = notifyList;
					
					// A1]A2]A3[B1]B2]B3
					str = strsep(&lineptr,"[");
					
					//A1]A2]A3
					while(str != NULL)
					{
						char *strptr = str;
						char *str1,*str2,*str3;
						char tmpStr[5];
						char *insertStr=NULL;
						
						insertStr=malloc(strlen(str));
						
						if(insertStr != NULL)
						{
							memset(insertStr,0x00,strlen(str));
												
							//A1]A2]A3
							str1 = strsep(&strptr,"]");
							sprintf(insertStr,"%s",str1);
							//A1
							
							//A2]A3
							str2 = strsep(&strptr,"]");
							//A2
							memset(tmpStr,0x00,sizeof(tmpStr));
							sprintf(tmpStr," %s",str2);
							strcat(insertStr,tmpStr);
							
							//A3
							str3 = strsep(&strptr,"]");
							//A3
							memset(tmpStr,0x00,sizeof(tmpStr));
							sprintf(tmpStr," %s\n",str3);
							strcat(insertStr,tmpStr);
		
	//fprintf(stderr,"\r\n insertStr=[%s] __[%s-%u]",insertStr,__FILE__,__LINE__);
																	
							
							if(firstline == 1)
								write_line_to_file("/var/CWMPNotify.txt", 1, insertStr);
							else
								write_line_to_file("/var/CWMPNotify.txt", 2, insertStr);
							
							firstline = 0;
							
							if(insertStr)
								free(insertStr);
														
						}
						
						str = strsep(&lineptr,"["); //get next line
					}
				}
				
				if(notifyList)
					free(notifyList);
			}
		#endif
			//read flatfs content before start cwmp
			//if(RunSystemCmd(NULL_FILE, "flatfsd", "-r", NULL_STR) !=0){
			//	printf("Read Flatfs Faile, Please check again\n");
			//}
			//system("flatfsd -r");
			apmib_get( MIB_CWMP_ACS_URL, (void *)acsUrltmp);
			
			//system("/bin/cwmpClient &");
			//memset(acsURLStr,0x00,sizeof(acsURLStr));
			sprintf(acsURLStr,"%s",acsUrltmp);
		}
		
	}
#endif	
	if(cwmp_flag & CWMP_FLAG_AUTORUN)
	{
		apmib_get( MIB_CWMP_CONREQ_PORT, (void *)&conReqPort);
		if(conReqPort >0 && conReqPort<65535)
		{
			//char tmpStr[CWMP_ACS_URL_LEN] = {0};
			
			apmib_get( MIB_CWMP_ACS_URL, (void *)acsUrl);
//printf("\r\n acsUrl=[%s],__[%s-%u]\r\n",acsUrl,__FILE__,__LINE__);
			if((strstr(acsUrl,"https://") != 0 || strstr(acsUrl,"http://") != 0) && strlen(acsUrl) != 0)
			{
				char *lineptr = acsUrl;
				char *str=NULL;

//printf("\r\n lineptr=[%s],__[%s-%u]\r\n",lineptr,__FILE__,__LINE__);
				
				str = strsep(&lineptr,"/");
//printf("\r\n str=[%s],__[%s-%u]\r\n",str,__FILE__,__LINE__);
				str = strsep(&lineptr,"/");
//printf("\r\n str=[%s],__[%s-%u]\r\n",str,__FILE__,__LINE__);
				str = strsep(&lineptr,"/");
//printf("\r\n str=[%s],__[%s-%u]\r\n",str,__FILE__,__LINE__);

				if(str != NULL && strlen(str) != 0)
				{
					sprintf(acsUrlRange,"%s-%s",str,str);
					
					sprintf(conReqPortRange,"%d:%d",conReqPort,conReqPort);
					//iptables -A INPUT -p tcp -m iprange --src-range $ACS_URL-$ACS_URL --dport $CWMP_CONREQ_PORT:$CWMP_CONREQ_PORT -i $WAN -d $EXT_IP -j ACCEPT	  
					//printf("\r\n acsUrlRange=[%s],__[%s-%u]\r\n",acsUrlRange,__FILE__,__LINE__);			
					//printf("\r\n conReqPortRange=[%s],__[%s-%u]\r\n",conReqPortRange,__FILE__,__LINE__);			
					//printf("\r\n interface=[%s],__[%s-%u]\r\n",interface,__FILE__,__LINE__);			
					//printf("\r\n wan_addr=[%s],__[%s-%u]\r\n",wan_addr,__FILE__,__LINE__);			
					//iptables -A INPUT -p tcp --dport 4567:4567 -i eth1 -d 172.21.69.21 -j ACCEPT	 		
					//RunSystemCmd(NULL_FILE, "iptables", "-A", "INPUT", "-p", "tcp", "-m", "iprange", "--src-range", acsUrlRange, "--dport", conReqPortRange, "-i", interface, "-d", wan_addr, "-j", "ACCEPT", NULL_STR);
					RunSystemCmd(NULL_FILE, "iptables", "-A", "INPUT", "-p", "tcp", "--dport", conReqPortRange, "-i", interface, "-d", wan_addr, "-j", "ACCEPT", NULL_STR);
					RunSystemCmd(NULL_FILE, "iptables", "-I", "PREROUTING","-t","nat", "-p", "tcp", "--dport", conReqPortRange, "-i", interface, "-d", wan_addr, "-j", "ACCEPT", NULL_STR);
				}
			}			
			
			
			
		}				
		
	}	
#if defined (QOS_OF_TR069)
	apmib_get(MIB_QOS_ENABLE,(void *)&qosFlag); 
	if(qosFlag) 	// enable tr098 qos will make webpage qos invalid
		set_QosRuleforTr069(interface, wan_addr);
#endif
}




void start_tr069(void)
{
	int lan_if = 0;
	int wan_if = 0;
	int port1, port2, port3, port4, port5;
	int bitRate1, bitRate2, bitRate3, bitRate4, bitRate5;
	char mode1[5]="", mode2[5]="", mode3[5]="", mode4[5]="", mode5[5]="";
	char cmd[512];
	int cwmp_flag = 0;

#if defined(CONFIG_USER_CWMP_WITH_TR181)
	/* 
	 * Following code is only for TR-181 data models
 	 */
	// port1
	apmib_get( MIB_CWMP_SW_PORT1_DISABLE, (void *)&port1);
	if (port1 == 1) {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		system("echo set eth if0 Enable false > /proc/rtl865x/tr181_eth_set");
	}
	else {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		apmib_get( MIB_CWMP_SW_PORT1_MAXBITRATE, (void *)&bitRate1);
		if (bitRate1 == 0) {
			bitRate1 = -1;
			apmib_set( MIB_CWMP_SW_PORT1_MAXBITRATE, (void *)&bitRate1);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if0 MaxBitRate %d > /proc/rtl865x/tr181_eth_set", bitRate1);
		system(cmd);

		usleep(100000);

		apmib_get( MIB_CWMP_SW_PORT1_DUPLEXMODE, (void *)mode1);
		if (strcmp(mode1, "") == 0) {
			sprintf(mode1, "%s", "Auto");
			apmib_set( MIB_CWMP_SW_PORT1_DUPLEXMODE, (void *)mode1);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if0 DuplexMode %s > /proc/rtl865x/tr181_eth_set", mode1);
		system(cmd);
	}

	usleep(100000);

	// port2
	apmib_get( MIB_CWMP_SW_PORT2_DISABLE, (void *)&port2);
	if (port2 == 1) {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		system("echo set eth if1 Enable false > /proc/rtl865x/tr181_eth_set");
	}
	else {

		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		apmib_get( MIB_CWMP_SW_PORT2_MAXBITRATE, (void *)&bitRate2);
		if (bitRate2 == 0) {
			bitRate2 = -1;
			apmib_set( MIB_CWMP_SW_PORT2_MAXBITRATE, (void *)&bitRate2);
		}
		memset(cmd, 0, sizeof(cmd));
		//printf("%s-%s-%d: bitRate=%d\n", __FILE__, __FUNCTION__, __LINE__, bitRate2);
		sprintf(cmd, "echo set eth if1 MaxBitRate %d > /proc/rtl865x/tr181_eth_set", bitRate2);
		system(cmd);

		usleep(100000);

		apmib_get( MIB_CWMP_SW_PORT2_DUPLEXMODE, (void *)mode2);
		if (strcmp(mode2, "") == 0) {
			sprintf(mode2, "%s", "Auto");
			apmib_set( MIB_CWMP_SW_PORT2_DUPLEXMODE, (void *)mode2);
		}
		memset(cmd, 0, sizeof(cmd));
		//printf("%s-%s-%d: mode=%s\n", __FILE__, __FUNCTION__, __LINE__, mode2);
		sprintf(cmd, "echo set eth if1 DuplexMode %s > /proc/rtl865x/tr181_eth_set", mode2);
		system(cmd);
	}

	usleep(100000);

	// port3
	apmib_get( MIB_CWMP_SW_PORT3_DISABLE, (void *)&port3);
	if (port3 == 1) {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		system("echo set eth if2 Enable false > /proc/rtl865x/tr181_eth_set");
	}
	else {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		apmib_get( MIB_CWMP_SW_PORT3_MAXBITRATE, (void *)&bitRate3);
		if (bitRate3 == 0) {
			bitRate3 = -1;
			apmib_set( MIB_CWMP_SW_PORT3_MAXBITRATE, (void *)&bitRate3);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if2 MaxBitRate %d > /proc/rtl865x/tr181_eth_set", bitRate3);
		system(cmd);

		usleep(100000);

		apmib_get( MIB_CWMP_SW_PORT3_DUPLEXMODE, (void *)mode3);
		if (strcmp(mode3, "") == 0) {
			sprintf(mode3, "%s", "Auto");
			apmib_set( MIB_CWMP_SW_PORT3_DUPLEXMODE, (void *)mode3);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if2 DuplexMode %s > /proc/rtl865x/tr181_eth_set", mode3);
		system(cmd);
	}

	usleep(100000);

	// port4
	apmib_get( MIB_CWMP_SW_PORT4_DISABLE, (void *)&port4);
	if (port4 == 1) {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		system("echo set eth if3 Enable false > /proc/rtl865x/tr181_eth_set");
	}
	else {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		apmib_get( MIB_CWMP_SW_PORT4_MAXBITRATE, (void *)&bitRate4);
		if (bitRate4 == 0) {
			bitRate4 = -1;
			apmib_set( MIB_CWMP_SW_PORT4_MAXBITRATE, (void *)&bitRate4);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if3 MaxBitRate %d > /proc/rtl865x/tr181_eth_set", bitRate4);
		system(cmd);

		usleep(100000);

		apmib_get( MIB_CWMP_SW_PORT4_DUPLEXMODE, (void *)mode4);
		if (strcmp(mode4, "") == 0) {
			sprintf(mode4, "%s", "Auto");
			apmib_set( MIB_CWMP_SW_PORT4_DUPLEXMODE, (void *)mode4);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if3 DuplexMode %s > /proc/rtl865x/tr181_eth_set", mode4);
		system(cmd);
	}

	usleep(100000);

	// port5
	apmib_get( MIB_CWMP_SW_PORT5_DISABLE, (void *)&port5);
	if (port5 == 1) {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		system("echo set eth if4 Enable false > /proc/rtl865x/tr181_eth_set");
	}
	else {
		//printf("%s-%s-%d\n", __FILE__, __FUNCTION__, __LINE__);
		apmib_get( MIB_CWMP_SW_PORT5_MAXBITRATE, (void *)&bitRate5);
		if (bitRate5 == 0) {
			bitRate5 = -1;
			apmib_set( MIB_CWMP_SW_PORT5_MAXBITRATE, (void *)&bitRate5);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if4 MaxBitRate %d > /proc/rtl865x/tr181_eth_set", bitRate5);
		system(cmd);

		usleep(100000);

		apmib_get( MIB_CWMP_SW_PORT5_DUPLEXMODE, (void *)mode5);
		if (strcmp(mode5, "") == 0) {
			sprintf(mode5, "%s", "Auto");
			apmib_set( MIB_CWMP_SW_PORT5_DUPLEXMODE, (void *)mode5);
		}
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "echo set eth if4 DuplexMode %s > /proc/rtl865x/tr181_eth_set", mode5);
		system(cmd);
	}

	usleep(100000);
#endif

	apmib_get( MIB_CWMP_LAN_ETHIFDISABLE, (void *)&lan_if );
	if (lan_if == 1)
		system("ifconfig eth0 down");
	else
		system("ifconfig eth0 up");

	apmib_get( MIB_CWMP_WAN_ETHIFDISABLE, (void *)&wan_if );
	if (wan_if == 1)
		system("ifconfig eth1 down");
	else
		system("ifconfig eth1 up");

	apmib_get( MIB_CWMP_FLAG, (void *)&cwmp_flag );
	if((cwmp_flag & CWMP_FLAG_AUTORUN) && isFileExist("/bin/cwmpClient") )
	{
		system("/bin/cwmpClient &");
		
	}	
}
#endif
#ifdef MULTI_PPPOE
void setMulPppoeRules(int argc, char** argv)
{
	//dzh add for multi-pppoe route set and lan-partition set	
	if(argc >=3 && argv[2] && (strcmp(argv[2], "pppoe")==0))
	{							
		system("ifconfig |grep 'P-t-P' | cut  -d ':' -f 2 | cut -d ' ' -f 1 > /etc/ppp/ppp_local");			
		system("ifconfig |grep 'P-t-P' | cut  -d ':' -f 3 | cut -d ' ' -f 1 > /etc/ppp/ppp_remote");
		system("ifconfig |grep 'ppp'| cut -d ' ' -f 1 > /etc/ppp/ppp_device");			
		system("cat /etc/ppp/ppp_local | wc -l > /etc/ppp/lineNumber");					
		if(0 == get_info())
		{		
 #ifdef MULTI_PPP_DEBUG		
			printf("get info error\n");
 #endif
			return ;
		}
		//print_info();
		if(argc >=4 && argv[3]) //if exist pppoe interface,set interface information
		{
			//set route 
			int index ;
			char command[100];
			char flushCmd[100];
			for( index = 0 ; index < pppNumbers ; ++index)
			{
			 	#ifdef MULTI_PPP_DEBUG	
				printf("the ppp_name is:%s\n",infos[index].ppp_name);
				printf("the argv[3] is:%s\n",argv[3]);			
				#endif
				if(!strcmp(infos[index].ppp_name,argv[3]))//match the interface
				{
					int sub_index;								
					//set subnet rules char SubNet[4][30];
					//SubNet[infos[index].order-1];
					/*
					for(sub_index = 0 ; sub_index < SubNets[infos[index].order-1].SubnetCount; ++sub_index)
					{						
						//-i eth0
						sprintf(command,
							"iptables -t mangle -A PREROUTING -i eth0 -m iprange --src-range %s-%s -j MARK --set-mark %d",
							SubNets[infos[index].order-1].startip[sub_index],SubNets[infos[index].order-1].endip[sub_index],
							infos[index].order+sub_index+100);		
						printf("%s\n",command); 					
						system(command);
						
						sprintf(command,"ip rule add fwmark %d table %d pref %d",
							infos[index].order+sub_index+100,
							infos[index].order+30,
							infos[index].order+sub_index+100);
						printf("%s\n",command); 					
						system(command);

						sprintf(command,"iptables -t nat -A POSTROUTING  -m iprange --src-range %s-%s -o %s -j MASQUERADE",
								SubNets[infos[index].order-1].startip[sub_index],SubNets[infos[index].order-1].endip[sub_index],
									infos[index].ppp_name);
						printf("%s\n",command); 					
						system(command);

					}						
					*/
					
					FILE* pF;// = fopen("/etc/ppp/flushCmds","w+");
					char path[50];
					sprintf(path,"/etc/ppp/%s.cmd",argv[3]);
					pF = fopen(path,"wt");
						
					system("ip rule del table 100 >/dev/null 2>&1");
					system("ip route del table 100 >/dev/null 2>&1");
					system(" ip rule add from  192.168.1.0/24 table 100 prio 32765");					
					system("ip route add default dev br0 table 100");		
					
					#ifdef MULTI_PPP_DEBUG					
					printf("%s\n",command); 					
					#endif	
					
					//system(command);					
					
					sprintf(command,"ip rule add from %s table %d",
						SubNet[infos[index].order-1],
						infos[index].order+30);
					#ifdef MULTI_PPP_DEBUG
					printf("%s\n",command); 					
					#endif
					system(command);

					//flush command
					fprintf(pF,"ip rule del table %d >/dev/null 2>&1 \n",infos[index].order+30);																							
					//iptables -A POSTROUTING -t nat  -s 192.168.1.0/25 -o ppp0 -j MASQUERADE

					sprintf(command,"iptables -A POSTROUTING -t nat -s %s -o %s -j MASQUERADE",
						SubNet[infos[index].order-1],
						infos[index].ppp_name);
					#ifdef MULTI_PPP_DEBUG					
					printf("%s\n",command); 	
					#endif
					system(command);								
					
					//set route
					
					sprintf(command,"ip route add %s dev %s table %d",infos[index].server_ip,
							infos[index].ppp_name,infos[index].order+10);
					#ifdef MULTI_PPP_DEBUG					
					printf("%s\n",command); 
					#endif
					system(command);	

					fprintf(pF,"ip route del table %d >/dev/null 2>&1 \n",infos[index].order+10);
					
					sprintf(command,"ip rule add from %s table %d",infos[index].client_ip,
						infos[index].order+10); 
					#ifdef MULTI_PPP_DEBUG					
					printf("%s\n",command); 	
					#endif
					system(command);	

					fprintf(pF,"ip rule del table %d >/dev/null 2>&1 \n",infos[index].order+10);								
					//set lan-partion					
					sprintf(command,"ip route add default via %s dev %s table %d",
						infos[index].server_ip,infos[index].ppp_name,
							infos[index].order+30);

					fprintf(pF,"ip route del table %d >/dev/null 2>&1 \n",infos[index].order+30);	
					#ifdef MULTI_PPP_DEBUG				
					printf("%s\n",command); 					
					#endif
					system(command);

					sprintf(command,"ip route add %s dev br0 table %d",Br0NetSectAddr,
							infos[index].order+30);
					#ifdef MULTI_PPP_DEBUG					
					printf("%s\n",command); 					
					#endif
					system(command);
					//iptables -A POSTROUTING -t nat -m iprange --src-range 192.168.1.1-192.168.1.50 -o ppp0 -j MASQUERADE	
					break;
				}//end if				
			}//end for
		}//end if		
		
	}//end if	
}
#endif
void setRulesWithOutDevice(int opmode, int wan_dhcp , char* pInterface_wanPhy,char* Interface_wanPhy)
{
	int intVal=0, natEnabled=0;
	int intVal1=0;
	int intVal2 = 0;
	int dyn_rt_support=0;
	int intVal_num=0;
	int hw_nat_support=0;
	int my_wan_type = 0;
	unsigned long	dos_enabled = 0;	
	char wan_type[8];
#ifdef CONFIG_RTL_HW_NAPT
	int ivalue = 0; 
#endif		
#if defined(CONFIG_REFINE_BR_FW_RULE) 
	int br_rule_refine = 1;
#endif

#if defined(CONFIG_APP_EBTABLES)&&defined(CONFIG_EBTABLES_KERNEL_SUPPORT)
	RunSystemCmd(NULL_FILE,Ebtables,FLUSH,NULL_STR);
	RunSystemCmd(NULL_FILE,Ebtables,X,NULL_STR);
	RunSystemCmd(NULL_FILE,Ebtables,Z,NULL_STR);
#endif
	RunSystemCmd("/proc/sys/net/ipv4/ip_forward", "echo", "0", NULL_STR);//don't enable ip_forward before set MASQUERADE
	RunSystemCmd("/proc/fast_nat", "echo", "2", NULL_STR);//clean conntrack table before set new rules
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables,_table, nat_table, FLUSH, POSTROUTING, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables,_table, nat_table, FLUSH, PREROUTING, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, _table, mangle_table, NULL_STR);		
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, INPUT, NULL_STR);		
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, OUTPUT, NULL_STR);	
	RunSystemCmd(NULL_FILE, Iptables, FLUSH, FORWARD, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, POLICY, OUTPUT, ACCEPT, NULL_STR);
#ifdef CONFIG_IPV6	
	RunSystemCmd(NULL_FILE, Ip6tables, FLUSH, INPUT, NULL_STR);		
	RunSystemCmd(NULL_FILE, Ip6tables, FLUSH, OUTPUT, NULL_STR);	
	RunSystemCmd(NULL_FILE, Ip6tables, FLUSH, FORWARD, NULL_STR);
	RunSystemCmd(NULL_FILE, Ip6tables, POLICY, OUTPUT, ACCEPT, NULL_STR);
#endif
	if(opmode != BRIDGE_MODE){
		RunSystemCmd(NULL_FILE, Iptables, POLICY, INPUT, DROP, NULL_STR);
	}else{
		RunSystemCmd(NULL_FILE, Iptables, POLICY, INPUT, ACCEPT, NULL_STR);
	}
	if(opmode != 3){
		RunSystemCmd(NULL_FILE, Iptables, POLICY, FORWARD, DROP, NULL_STR);
	}else{
		RunSystemCmd(NULL_FILE, Iptables, POLICY, FORWARD, ACCEPT, NULL_STR);
	}

	//#redefine url and schedule filter

	if(isFileExist("/bin/routed")){
		dyn_rt_support=1;
	}
	if(isFileExist("/proc/hw_nat")){
		hw_nat_support=1;
	}
	if(dyn_rt_support ==1 && opmode != BRIDGE_MODE){
		apmib_get(MIB_NAT_ENABLED, (void *)&natEnabled);
		if(natEnabled==0){
			RunSystemCmd(NULL_FILE, Iptables, POLICY, INPUT, ACCEPT, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, POLICY, FORWARD, ACCEPT, NULL_STR);
			//RunSystemCmd("/proc/fast_nat", "echo", "0", NULL_STR);//disable fastpath when nat is disabled
			return 0;
		}
	}
	if(opmode == BRIDGE_MODE)
		return 0;

	//url filter setting
	apmib_get(MIB_URLFILTER_ENABLED,  (void *)&intVal);
	apmib_get(MIB_URLFILTER_TBL_NUM,  (void *)&intVal_num);

#if defined(CONFIG_RTL_FAST_FILTER)
	system("rtk_cmd filter flush");
#else
	RunSystemCmd("/proc/filter_table", "echo", "flush", NULL_STR);
	RunSystemCmd("/proc/filter_table", "echo", "init", "3",  NULL_STR);
#endif
	if(intVal !=0 && intVal_num>0){
//		RunSystemCmd("/proc/url_filter", "echo", " ", NULL_STR);
		setURLFilter();
#if 0
defined(CONFIG_RTL_HW_NAPT)
		if(opmode==0){
			RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);//disable h/w nat when url filter enabled
		}
#endif
	}else{
//		RunSystemCmd("/proc/url_filter", "echo", "0", NULL_STR);//disable url filter
#if defined(CONFIG_RTL_FAST_FILTER)
#else
		RunSystemCmd("/proc/filter_table", "echo", "flush", NULL_STR);
#endif
#if 0
defined(CONFIG_RTL_HW_NAPT)
		if(opmode==0){
			apmib_get(MIB_SUBNET_MASK,(void*)&ivalue);
			if((ivalue&HW_NAT_LIMIT_NETMASK)!=HW_NAT_LIMIT_NETMASK)
			{
					RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
			}
			else
			{
				RunSystemCmd("/proc/hw_nat", "echo", "1", NULL_STR);//enable h/w nat when url filter disable
			}
		}
#endif

	}
#if defined(CONFIG_RTL_HW_NAPT)

	RunSystemCmd("/proc/hw_nat", "echo", "9", NULL_STR);
	my_wan_type = 0;
	my_wan_type = wan_dhcp + 80;
	sprintf(wan_type, "%d", my_wan_type);
	RunSystemCmd("/proc/hw_nat", "echo", wan_type, NULL_STR);
	#if 0//defined(CONFIG_RTL_EXT_PORT_SUPPORT)
	if(opmode!=GATEWAY_MODE)
		RunSystemCmd("/proc/ext_port", "echo", "0", NULL_STR);
	else
		RunSystemCmd("/proc/ext_port", "echo", "1", NULL_STR);
#endif
#else
	RunSystemCmd("/proc/sw_nat", "echo", "9", NULL_STR);
#endif

	////////////////////////////////////////////////////
	//ip filter setting
	intVal = 0;
	apmib_get(MIB_IPFILTER_ENABLED,  (void *)&intVal);
	apmib_get(MIB_IPFILTER_TBL_NUM,  (void *)&intVal_num);
	if(intVal ==1 && intVal_num>0){
			//set ip filter
			setIpFilter();
			#if defined(CONFIG_REFINE_BR_FW_RULE) 
			br_rule_refine = 0;
			#endif
	}

	intVal = 0;
	apmib_get(MIB_MACFILTER_ENABLED,  (void *)&intVal);
	apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&intVal_num);
	if(intVal==1 && intVal_num>0){
		//set mac filter
		setMACFilter();
		#if defined(CONFIG_REFINE_BR_FW_RULE) 
		br_rule_refine = 0;
		#endif
	}
	if((SET_PARENT_CONTROL_LIST==g_parentControlFlag)||(DELETE_PARENT_CONTROL_LIST==g_parentControlFlag))
	{
	 printf("set parent control...\n");
	 setParentContrl(g_parentControlFlag,g_parentControlListTableNumberFlag);
	}
	else
	{
	 getParentContrlStatus();
	 setParentContrl(g_parentControlFlag,g_parentControlListTableNumberFlag);
	}

	intVal=0;
	apmib_get(MIB_PORTFILTER_ENABLED,  (void *)&intVal);
	apmib_get(MIB_PORTFILTER_TBL_NUM, (void *)&intVal_num);
	if(intVal==1 && intVal_num>0){
		setPortFilter();
		#if defined(CONFIG_REFINE_BR_FW_RULE) 
		br_rule_refine = 0;
		#endif
	}	
	///////////////////////////////////////////////////////////
	apmib_get(MIB_VPN_PASSTHRU_L2TP_ENABLED, (void *)&intVal);
	if(intVal ==0){
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport, "1701", jump, DROP, NULL_STR);
	}
	else if(intVal == 1){
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, sport, "1701", jump, ACCEPT, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport, "1701", jump, ACCEPT, NULL_STR);
	}
	apmib_get(MIB_VPN_PASSTHRU_PPTP_ENABLED, (void *)&intVal2);
	if(intVal2 ==0){
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _tcp, dport, "1723", jump, DROP, NULL_STR);
	}
	else if(intVal2 == 1){
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _tcp, dport, "1723", jump, ACCEPT, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _tcp, sport, "1723", jump, ACCEPT, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, "47", jump, ACCEPT, NULL_STR); //GRE
	}

	intVal = 0;
	apmib_get(MIB_VPN_PASSTHRU_IPSEC_ENABLED, (void *)&intVal);
	if(intVal == 0){
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport, "500", jump, DROP, NULL_STR);
		/*When ipsec enable->disable, ipsec may not send udp port 500 packet, 
		so drop udp port 4500 too when ipsec passthru disabled.*/
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport, "4500", jump, DROP, NULL_STR);
	}

	/*
	if((intVal == 1) || (intVal2 == 1)){
		//RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _icmp, jump, ACCEPT, NULL_STR);
	//	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pInterface_wanPhy, jump, ACCEPT, NULL_STR);
	}
	*/
	///////////////////////////////////////////////////////////
	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, match, _udp, in, Interface_wanPhy, "--destination" , "224.0.0.0/4", jump, ACCEPT, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, "114" , in, Interface_wanPhy, "--destination" , "224.0.0.0/4", jump, ACCEPT, NULL_STR);	
	///////////////////////////////////////////////////////////
	RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, match, mstate, state, RELATED_ESTABLISHED, jump, ACCEPT, NULL_STR);
	//iptables -I FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu
#if defined(CONFIG_REFINE_BR_FW_RULE) 
	if (br_rule_refine) 
	{
		RunSystemCmd(NULL_FILE, Iptables, INSERT, FORWARD, in, "br0", jump, ACCEPT, NULL_STR);	
		RunSystemCmd(NULL_FILE, Iptables, INSERT, FORWARD, _protocol, _tcp, tcp_flags, MSS_FLAG1, MSS_FLAG2, jump, TCPMSS, clamp, NULL_STR);	
		///////////////////////////////////////////////////////////
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, match, mstate, state, INVALID, jump, DROP, NULL_STR);	
	} else
#endif
	{
	RunSystemCmd(NULL_FILE, Iptables, INSERT, FORWARD, _protocol, _tcp, tcp_flags, MSS_FLAG1, MSS_FLAG2, jump, TCPMSS, clamp, NULL_STR);	
	///////////////////////////////////////////////////////////
	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, match, mstate, state, INVALID, jump, DROP, NULL_STR);	

	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, "br0", jump, ACCEPT, NULL_STR);	
	}
	///////////////////////////////////////////////////////////
	if(wan_dhcp==4){			
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pInterface_wanPhy, match, mstate, state, RELATED_ESTABLISHED, jump, ACCEPT, NULL_STR);
	}	
	///////////////////////////////////////////////////////////
	
	RunSystemCmd("/tmp/firewall_igd", "echo", "1", NULL_STR);//disable fastpath when port filter is enabled

	apmib_get(MIB_DOS_ENABLED, (void *)&dos_enabled);
	if(dos_enabled > 0){
#if 0
defined(CONFIG_RTL_HW_NAPT)
	if(opmode == GATEWAY_MODE)
		RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
#endif
	}
	setDoS(dos_enabled, opmode);
	if(wan_dhcp==PPTP){
	
		RunSystemCmd(NULL_FILE, Iptables, _table, nat_table, ADD, POSTROUTING, out, pInterface_wanPhy, jump, MASQUERADE, NULL_STR);
	}

	if(wan_dhcp == L2TP){
		
		RunSystemCmd(NULL_FILE, Iptables, _table, nat_table, ADD, POSTROUTING, out, pInterface_wanPhy, jump, MASQUERADE, NULL_STR);
	}
}

#ifdef CONFIG_8021Q_VLAN_SUPPORTED
#define LINUX_VLAN_REINIT_FILE "/var/system/linux_vlan_reinit"
static int getBridgeNum(void)
{
	char reinitBuf[320];
	char *token=NULL, *saveStr=NULL;
	FILE *fp = NULL;
	int br_num=0;

	if(!isFileExist(LINUX_VLAN_REINIT_FILE))
		return 0;
		
	fp = fopen(LINUX_VLAN_REINIT_FILE, "r");
	if(!fp)
		return -1;

	memset(reinitBuf, 0, sizeof(reinitBuf));
	fgets(reinitBuf, sizeof(reinitBuf), fp);

	token = strtok_r(reinitBuf, ":", &saveStr);
	do{
		if(!token)
			break;
		
		if(memcmp(token, "eth", 3)!=0 && memcmp(token, "wlan", 4)!=0)
		{
			br_num = strtol(token, NULL, 10);
			if(br_num<0 || br_num>MAX_VLAN_CONFIG_NUM)
				br_num = 0;

			break;
		}

		token = strtok_r(NULL, ":", &saveStr);
	}
	while(token);

	fclose(fp);
		
	return br_num;
}
#endif
#ifdef CONFIG_APP_RTL_QUAGGA
void set_dynamic_route_rules(void)
{
	system("iptables -A INPUT -d 224.0.0.5 -j ACCEPT");
	system("iptables -A INPUT -d 224.0.0.6 -j ACCEPT");
	system("iptables -A INPUT -d 224.0.0.9 -j ACCEPT");

}
#endif

int setFirewallIptablesRules(int argc, char** argv)
{
	int opmode=-1;
	int qosFlag = -1;
	int wan_dhcp=-1;
	char iface[20], *pInterface="eth1";
	char *pInterface_wanPhy="eth1";
	char Interface_wanPhy[20]="eth1";
	int wlaniface=0, get_wanip=0;
	struct in_addr wanaddr;
	char IpAddr[30], *strIp;
	char WanIpAddr[30], *strWanIp;
	char WanPhyIpAddr[30];
	char IpAddrBuf[30];
	char LanIpAddr[30];
	int intVal=0, natEnabled=0;
	int intVal1=0;
	int intVal2 = 0;
	unsigned long	dos_enabled = 0;
	int dyn_rt_support=0;
	int intVal_num=0;
	int hw_nat_support=0;
	int my_wan_type = 0;	
#ifdef MULTI_PPPOE
	int isNeedSetOnce = 1;
#endif
	int wan_type=0;
	//add for DMZ 
	//echo 192.168.1.0/24 >/etc/ppp/br0_info
	char *br0info[50];
	char *bInterface = "br0";
	int get_br0ip =0;
	int get_br0mask =0;
	char Br0NetSectAddr[30];
	char * strbr0Ip ,* strbr0Mask ;
	struct in_addr br0addr,br0mask;
	unsigned int numofone ;	
	char NumStr[10]; 
	struct stat status;
#ifdef CONFIG_RTL_HW_NAPT
	int ivalue = 0; 
#endif
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	struct in_addr ipaddr;
	char ipaddr_str[16];
#endif
#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
	int web_access_port = 0;
	unsigned char port_buff[16] = {'\0'};
	unsigned char ip_buff[64] = {'\0'};
#endif


    if(argc >=4 && argv[2] && argv[3] )
    { 
     if(strcmp(argv[2], "addParentControl")==0)
     { 
        g_parentControlListTableNumberFlag=atoi(argv[3]);
	 	g_parentControlFlag=SET_PARENT_CONTROL_LIST;
     }
	 else if(strcmp(argv[2], "deleteParentControl")==0)
	 {
	    g_parentControlListTableNumberFlag=atoi(argv[3]);
	 	g_parentControlFlag=DELETE_PARENT_CONTROL_LIST;
	 }
	 else
	 {
	 	g_parentControlFlag=DEFAULT_PARENT_CONTROL_IGNORE_ACT;
	 }
    }

	printf("Init Firewall Rules....\n");
#ifdef MULTI_PPPOE	

	wait_lable:
	if (isFileExist("/etc/ppp/firewall_lock") == 1) {	
		system("rm -f /etc/ppp/firewall_lock");
	}else{
		sleep(1);
		goto wait_lable;
	}
	if(argc >=4 && argv[3] && (!strncmp(argv[3],"ppp",3)))
	{
		if(!isFileExist("/etc/ppp/hasPppoedevice"))
		{
			system("echo 1 > /etc/ppp/hasPppoedevice");
			isNeedSetOnce = 1;		
		}else{		
			isNeedSetOnce = 0;		
		}
	}
#endif
	memset(WanPhyIpAddr,'\0',30);
	apmib_get(MIB_OP_MODE, (void *)&opmode);
	apmib_get(MIB_WAN_DHCP, (void *)&wan_dhcp);
	if(opmode == WISP_MODE){
		apmib_get(MIB_WISP_WAN_ID, (void *)&wlaniface);
		sprintf(iface, "wlan%d", wlaniface);
#if defined(CONFIG_SMART_REPEATER)			
		getWispRptIfaceName(iface,wlaniface);
		//strcat(iface, "-vxd");
#endif
		pInterface = iface;
		pInterface_wanPhy=iface;
		if (wan_dhcp == PPPOE || wan_dhcp == PPTP || wan_dhcp == L2TP )
#ifdef MULTI_PPPOE
			if(argc >=4 && argv[3])
				pInterface = argv[3];
			else 
				pInterface="ppp0";
#else
#ifdef SUPPORT_ZIONCOM_RUSSIA
			if(argc!=-1) 
#endif
			pInterface="ppp0";
#endif
	}else{
		if(opmode == GATEWAY_MODE){
			if (wan_dhcp == PPPOE || wan_dhcp == PPTP || wan_dhcp == L2TP || wan_dhcp == USB3G)
#ifdef MULTI_PPPOE
			if(argc >=4 && argv[3])
				pInterface = argv[3];
			else 
				pInterface="ppp0";
#else
#if defined(SUPPORT_ZIONCOM_RUSSIA) || defined(_ALPHA_DUAL_WAN_SUPPORT_)
			if(argc!=-1) 
#endif
			pInterface="ppp0";
#endif
			#if defined(CONFIG_4G_LTE_SUPPORT)
			if (lte_wan()) {
				pInterface = "usb0";
				pInterface_wanPhy="usb0";
			}
			#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */
		}
	}
	get_wanip = getInAddr(pInterface, IP_ADDR_T, (void *)&wanaddr);
	if( get_wanip ==0){   //get wan ip fail
		printf("No wan ip currently!\n");
		goto EXIT_setFirewallIptablesRules;
	}else{
		strWanIp = inet_ntoa(wanaddr);
		strcpy(WanIpAddr, strWanIp);
	}

#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	memset(&ipaddr, 0, sizeof(ipaddr));
	if(wan_dhcp == PPPOE)
	{
		getInAddr(Interface_wanPhy,IP_ADDR_T,(void *)&ipaddr);
		if(ipaddr.s_addr>0)
			strcpy(ipaddr_str, inet_ntoa(ipaddr));
	}
#endif
;
	//flush fast natp table
	//RunSystemCmd("/proc/net/flush_conntrack", "echo", "1", NULL_STR);
#ifdef MULTI_PPPOE
	if(isNeedSetOnce)
		setRulesWithOutDevice(opmode,wan_dhcp,pInterface_wanPhy,Interface_wanPhy);
#else
		setRulesWithOutDevice(opmode,wan_dhcp,pInterface_wanPhy,Interface_wanPhy);
#endif
#ifdef CONFIG_APP_RTL_QUAGGA
if((opmode == WISP_MODE)||(opmode == GATEWAY_MODE)){
	set_dynamic_route_rules();
}
#endif
	intVal = 0;
	apmib_get( MIB_WEB_WAN_ACCESS_ENABLED, (void *)&intVal);
	if(intVal==1){
		#if defined(CONFIG_RTL_WEB_WAN_ACCESS_PORT)
		apmib_get( MIB_WEB_WAN_ACCESS_PORT, (void *)&web_access_port);
		sprintf(port_buff, "%d", web_access_port);
		sprintf(ip_buff, "%s:80", WanIpAddr);
		//printf("%s %d port_buff=%s web_access_port=%d \n", __FUNCTION__, __LINE__, port_buff, web_access_port);		
		RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, _dest, WanIpAddr, dport, port_buff, jump, DNAT, "--to", ip_buff, NULL_STR);
		#ifdef _ALPHA_DUAL_WAN_SUPPORT_	
		if(ipaddr.s_addr>0){
			sprintf(ip_buff, "%s:80", ipaddr_str);
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, _protocol, _tcp, _dest, ipaddr_str, dport, port_buff, jump, DNAT, "--to", ip_buff, NULL_STR);
			if (((web_access_port>=1) && (web_access_port<=65535)) && (web_access_port != 80)){
				RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _protocol, _tcp,	dport, "80", in, pInterface, _dest, ipaddr_str, jump, DROP, NULL_STR);
			}
		}
		#endif
		if (((web_access_port>=1) && (web_access_port<=65535)) && (web_access_port != 80)){
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _protocol, _tcp,	dport, "80", in, pInterface, _dest, WanIpAddr, jump, DROP, NULL_STR);
		}
		#endif
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _tcp,  dport, "80:80", in, pInterface, _dest, WanIpAddr, jump, ACCEPT, NULL_STR);
#ifdef _ALPHA_DUAL_WAN_SUPPORT_	
		if(ipaddr.s_addr>0)
			RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _tcp,	dport, "80:80", in, Interface_wanPhy, _dest, ipaddr_str, jump, ACCEPT, NULL_STR);
#endif
	}else{
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _tcp,  dport, "80:80", in, pInterface, _dest, WanIpAddr, jump, DROP, NULL_STR);
	}

RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp,  dport, "1900:1900", in, pInterface, _dest, WanIpAddr, jump,DROP, NULL_STR);

#ifdef _ALPHA_DUAL_WAN_SUPPORT_	
	if(ipaddr.s_addr>0)
	{		
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _tcp,	dport, "!", "80:80", in, Interface_wanPhy, _dest, ipaddr_str, jump, ACCEPT, NULL_STR);
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp,	dport, "!", "1900:1900", in, Interface_wanPhy, _dest, ipaddr_str, jump, ACCEPT, NULL_STR);
	}
#endif

	// SNMP setting
#ifdef CONFIG_SNMP
	intVal = 0;
	apmib_get(MIB_SNMP_ENABLED, (void *)&intVal);
	if (intVal == 1) {
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp, dport, "161:161", in, pInterface, _dest, WanIpAddr, jump, ACCEPT, NULL_STR);
		/*???where script*/
		RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table, nat_table, in, pInterface, _protocol, _udp, dport, "161", _dest, WanIpAddr, jump, REDIRECT, "--to-port", "161", NULL_STR);
	}
#endif

#ifdef CONFIG_SIXRD_SUPPORT
		int val = 0;
		if(!apmib_get(MIB_IPV6_ORIGIN_TYPE,&val)){
			fprintf(stderr, "Read MIB_IPV6_ORIGIN_TYPE Error\n");
			return -1;			
			}
		else{
				if(val==IPV6_ORIGIN_6RD)
				RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, pInterface, _protocol, "41", jump, ACCEPT,  NULL_STR);
				/* ipv6 protocol type is 41 */
			}
#endif
#ifdef CONFIG_DSLITE_SUPPORT
	// set dslite firewall here.
	set_dslite_firewall_rules();
#endif

	apmib_get(MIB_PORTFW_ENABLED,  (void *)&intVal);
	apmib_get(MIB_PORTFW_TBL_NUM, (void *)&intVal_num);
	if(intVal==1 && intVal_num>0){
		setPortForward(pInterface, WanIpAddr);
	}

#if 0
	// Move to set_init
	apmib_get(MIB_CUSTOM_PASSTHRU_ENABLED, (void *)&intVal);
	RunSystemCmd("/proc/custom_Passthru", "echo", (intVal & 0x1)?"1":"0", NULL_STR);
#endif


	//dzh modify
	apmib_get(MIB_WAN_DHCP,(void *)&wan_type);
	if(wan_type==PPTP)
	{
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, "47", in, pInterface_wanPhy, jump, ACCEPT, NULL_STR);		
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _tcp, sport ,"1723" ,in, pInterface_wanPhy, jump, ACCEPT, NULL_STR);		
	}
	else if(wan_type==L2TP)
	{
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp, sport ,"1701" ,in, pInterface_wanPhy, jump, ACCEPT, NULL_STR);
	}
	apmib_get(MIB_VPN_PASSTHRU_IPSEC_ENABLED, (void *)&intVal);
	if(intVal ==0){
#ifdef MULTI_PPPOE	
	if(isNeedSetOnce){
#endif	
		/*jwj: move this code to setRulesWithOutDevice()*/
		//RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport, "500", jump, DROP, NULL_STR);		
		system("echo 2 > /proc/fast_nat");
#ifdef MULTI_PPPOE	
	}
#endif	
	}else{
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, dport, "500", in ,pInterface, out, "br0", jump, ACCEPT, NULL_STR);
	}

	if(wan_dhcp == DHCP_CLIENT || wan_dhcp == PPTP || wan_dhcp == L2TP)
	{
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _udp, dport, "68", in ,pInterface, jump, ACCEPT, NULL_STR);	
	}
	//add for DMZ
	get_br0ip = getInAddr(bInterface, IP_ADDR_T,(void *)&br0addr);
	if(get_br0ip ==0 ){
		printf("No ip currently!\n");
		goto EXIT_setFirewallIptablesRules;
	}
	strcpy(LanIpAddr,inet_ntoa(br0addr));
	get_br0mask = getInAddr(bInterface, NET_MASK_T,(void *)&br0mask);
	if( get_br0mask ==0 ){
		printf("No MASK currently!\n");
		goto EXIT_setFirewallIptablesRules;
	}
	br0addr.s_addr &= br0mask.s_addr ;
	for(numofone =0;br0mask.s_addr;++numofone)
		br0mask.s_addr &= br0mask.s_addr-1;
	sprintf (NumStr, "%d", numofone);
	strcpy(Br0NetSectAddr,inet_ntoa(br0addr));
	strcat(Br0NetSectAddr,"/");
	strcat(Br0NetSectAddr,NumStr);
	//echo 192.168.1.0/24 >/etc/ppp/br0_info
//	char *br0info[50];
#ifdef MULTI_PPPOE
	sprintf(br0info,"echo %s > /etc/ppp/br0_info",Br0NetSectAddr);
	system(br0info);
#endif	
	apmib_get( MIB_DMZ_ENABLED, (void *)&intVal);
	if(intVal ==1){
		apmib_get( MIB_DMZ_HOST,  (void *)IpAddrBuf);
		strIp = inet_ntoa(*((struct in_addr *)IpAddrBuf));
		if(strcmp(strIp, "0.0.0.0")){
			strcpy(IpAddr, strIp);
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, _table , nat_table, _protocol, ALL, _dest, WanIpAddr, jump, DNAT, "--to", IpAddr, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, DEL, INPUT, _protocol, _tcp, dport, "80:80", in, pInterface, _dest, WanIpAddr, jump, DROP, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pInterface, _dest, IpAddr, _protocol, ALL, jump, ACCEPT, NULL_STR);
			//add for DMZ
			RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table,_src,LanIpAddr,_dest, IpAddr, jump, ACCEPT, NULL_STR);
			RunSystemCmd(NULL_FILE, Iptables, ADD, POSTROUTING, _table, nat_table,_src,Br0NetSectAddr,_dest, IpAddr, jump, "SNAT","--to", WanIpAddr, NULL_STR);

			/*jwj: To avoid pakcets whose dip is lan ip forward to lan pc.*/
			RunSystemCmd(NULL_FILE, Iptables, ADD, PREROUTING, in, pInterface, _dest, Br0NetSectAddr, jump, DROP, NULL_STR);
		}
	}

	intVal = 0;
	apmib_get( MIB_PING_WAN_ACCESS_ENABLED, (void *)&intVal);
	if(intVal==1){
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _icmp, icmp_type, echo_request,  in, pInterface, _dest, WanIpAddr, jump, ACCEPT, NULL_STR);
#ifdef _ALPHA_DUAL_WAN_SUPPORT_		
		if(ipaddr.s_addr>0)
			RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _icmp, icmp_type, echo_request,  in, Interface_wanPhy, _dest, ipaddr_str, jump, ACCEPT, NULL_STR);
#endif
	}else{
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, _icmp, icmp_type, echo_request,  in, pInterface, _dest, WanIpAddr, jump, DROP, NULL_STR);
	}

	intVal = 0;
	apmib_get( MIB_IGMP_PROXY_DISABLED, (void *)&intVal);
	if(intVal==0){
#ifdef SUPPORT_ZIONCOM_RUSSIA	
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, "2", in, Interface_wanPhy, jump, ACCEPT, NULL_STR);		
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, Interface_wanPhy, jump, ACCEPT, NULL_STR);
#else		
		RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, "2", in, pInterface, jump, ACCEPT, NULL_STR);
#endif
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, match, _udp, in, pInterface, "--destination" , "224.0.0.0/4", jump, ACCEPT, NULL_STR);
		apmib_get( MIB_WAN_DHCP, (void *)&intVal);
		//if wan is pptp(4) or l2tp(6), add this rule to permit multicast transter from wan to lan
		if(intVal==4 || intVal==6){
#ifdef MULTI_PPPOE	
	if(isNeedSetOnce){
#endif					
			RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, _udp, match, _udp, in, Interface_wanPhy, "--destination" , "224.0.0.0/4", jump, ACCEPT, NULL_STR);
#ifdef MULTI_PPPOE	
		}
#endif	
		}
	}

//modify	
	RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, "br0", jump, ACCEPT, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, "eth0", jump, ACCEPT, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, "lo", jump, ACCEPT, NULL_STR);
//	RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, NOT, pInterface, jump, ACCEPT, NULL_STR);

#ifdef CONFIG_8021Q_VLAN_SUPPORTED
	int br_idx=0, br_num=0;
	char br_iface[10] = {0};
	br_num = getBridgeNum();
	if(br_num>0 && br_num<=MAX_VLAN_CONFIG_NUM)
	{
		for(br_idx=1; br_idx<br_num; br_idx++)
		{
			sprintf(br_iface, "br%d", br_idx);
			RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, br_iface, jump, ACCEPT, NULL_STR);
		}
	}
#endif

#ifdef CONFIG_RTK_VLAN_WAN_TAG_SUPPORT
 	apmib_get( MIB_VLAN_WAN_BRIDGE_TAG, (void *)&intVal);
	if(intVal!=0)
	{
    	RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, "br1", jump, ACCEPT, NULL_STR);
	}
#endif

	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, _protocol, "50", in, pInterface, out, "br0", jump, ACCEPT, NULL_STR);
	RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pInterface, match, mstate, state, RELATED_ESTABLISHED, jump, ACCEPT, NULL_STR);

	if(wan_dhcp == L2TP)		
		RunSystemCmd(NULL_FILE, Iptables, ADD, FORWARD, in, pInterface_wanPhy, match, mstate, state, RELATED_ESTABLISHED, jump, ACCEPT, NULL_STR);
	/*when layered driver enable, permit all icmp packet but icmp request...*/
	//RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, _protocol, "2", in, pInterface, jump, ACCEPT, NULL_STR);



	if(dyn_rt_support ==1){
		apmib_get(MIB_NAT_ENABLED, (void *)&natEnabled);
#if defined(CONFIG_ROUTE)
		apmib_get(MIB_RIP_ENABLED, (void *)&intVal);
		apmib_get(MIB_RIP_WAN_RX, (void *)&intVal1);
#endif
		if(natEnabled==1 && intVal==1){
			if(intVal1==1){
				RunSystemCmd(NULL_FILE, Iptables, ADD, INPUT, in, pInterface, _protocol, _udp, dport, "520", jump, ACCEPT, NULL_STR);
			}
		}
	}


	RunSystemCmd(NULL_FILE, Iptables, _table, nat_table, ADD, POSTROUTING, out, pInterface, jump, MASQUERADE, NULL_STR);

	//fix the issue of WISP mode+PPPoE, lan pc can't access DUT
	if(opmode == WISP_MODE && wan_dhcp == PPPOE && isFileExist("/etc/ppp/link"))
	{		
		struct in_addr ip_addr, subnet_mask, net_addr;
		unsigned char netIp[16], maskIp[16], ipAddr[16];	
		unsigned char cmdbuf[128];
		
		apmib_get(MIB_IP_ADDR,  (void *)&ip_addr);	
		sprintf(ipAddr, "%s", inet_ntoa(ip_addr));
//		printf("%s:%d ipAddr=%s\n",__FUNCTION__,__LINE__,ipAddr);	
		apmib_get(MIB_SUBNET_MASK,	(void *)&subnet_mask);	
		sprintf(maskIp, "%s", inet_ntoa(subnet_mask));
//		printf("%s:%d maskIp=%s\n",__FUNCTION__,__LINE__,maskIp);	
		net_addr.s_addr=ip_addr.s_addr & subnet_mask.s_addr;	
		sprintf(netIp, "%s", inet_ntoa(net_addr));
//		printf("%s:%d netIp=%s\n",__FUNCTION__,__LINE__,netIp);

		sprintf(cmdbuf, "route del -net %s netmask %s dev br0 > /dev/null 2>&1", netIp, maskIp);
		system(cmdbuf);
		
		sprintf(cmdbuf, "route add -net %s netmask %s dev br0", netIp, maskIp);
		system(cmdbuf);
	}

#ifdef CONFIG_IPV6
	//add rule to avoid DOS attack
#ifdef SUPPORT_DEFEAT_IP_SPOOL_DOS
	RunSystemCmd(NULL_FILE, Ip6tables, POLICY, INPUT, DROP, NULL_STR);
	RunSystemCmd(NULL_FILE, Ip6tables, INSERT, INPUT, in, bInterface, _src, "fe80::/64", jump, ACCEPT, NULL_STR);

	char prefix_buf[256];
	if(isFileExist("/var/radvd.conf"))
	{
		FILE *fp=NULL;
		char line_buf[128];
		char *pline=NULL;
		
		if((fp=fopen("/var/radvd.conf", "r"))!=NULL)
		{
			while(fgets(line_buf, sizeof(line_buf), fp))
			{
				line_buf[strlen(line_buf)-1]=0;
				if((pline=strstr(line_buf, "prefix"))!=NULL)
				{
					strcpy(prefix_buf, line_buf+7);					
					RunSystemCmd(NULL_FILE, Ip6tables, ADD, INPUT, in, bInterface, _src, prefix_buf, jump, ACCEPT, NULL_STR);
				}
			}
			fclose(fp);
		}		
	}	

	dhcp6sCfgParam_t dhcp6sCfgParam;
	memset(&dhcp6sCfgParam, 0, sizeof(dhcp6sCfgParam));
	apmib_get(MIB_IPV6_DHCPV6S_PARAM,(void *)&dhcp6sCfgParam);
	
	if(dhcp6sCfgParam.enabled)
	{
		if(dhcp6sCfgParam.addr6PoolS && dhcp6sCfgParam.addr6PoolE)
		{
			sprintf(prefix_buf, "%s-%s", dhcp6sCfgParam.addr6PoolS, dhcp6sCfgParam.addr6PoolE);			
			RunSystemCmd(NULL_FILE, Ip6tables, ADD, INPUT, in, bInterface, match, ip_range, src_rnage, prefix_buf, jump, ACCEPT, NULL_STR);
		}
	}	
	RunSystemCmd(NULL_FILE, Ip6tables, ADD, INPUT, in, bInterface, jump, DROP, NULL_STR);	
#endif
#endif
/*
	RunSystemCmd("/proc/sys/net/ipv4/ip_conntrack_max", "echo", "1280", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established", "echo", "600", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/netfilter/ip_conntrack_udp_timeout", "echo", "60", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_time_wait", "echo", "5", NULL_STR);
	RunSystemCmd("/proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_close", "echo", "5", NULL_STR);
*/

//hyking:packet from wan is NOT allowed
#if 0 //defined(CONFIG_RTL_LAYERED_DRIVER_ACL)
	RunSystemCmd(NULL_FILE, "iptables", ADD, INPUT, jump, ACCEPT, NULL_STR);
#else
//	RunSystemCmd(NULL_FILE, "iptables", ADD, INPUT, "!", in, pInterface, jump, ACCEPT, NULL_STR);
#endif

	//RunSystemCmd("/proc/sys/net/ipv4/conf/eth1/arp_ignore", "echo", "1", NULL_STR);
#ifdef MULTI_PPPOE	
	if(argc >=4 && argv[3])
		set_QoS(opmode, wan_dhcp, wlaniface,argv[3]);
	else
		set_QoS(opmode, wan_dhcp, wlaniface,"ppp0");
#else
	set_QoS(opmode, wan_dhcp, wlaniface);
			
#endif		

#ifdef CONFIG_RTK_VOIP
	set_voip_parameter(pInterface);
#endif

#ifdef MULTI_PPPOE
	setMulPppoeRules(argc,argv);	
#endif
#ifdef CONFIG_RTL_HW_NAPT
	update_hwnat_setting();
#endif

#if defined(CONFIG_RTL_HARDWARE_IPV6_SUPPORT)
	#if 0 //check in function update_hw_ipv6_status
	if (opmode == GATEWAY_MODE){
		if ( apmib_get( MIB_QOS_ENABLED, (void *)&qosFlag) == 0) {
			printf("Get enabled flag error! \n");
			return -1;
		}

		if(qosFlag)
			RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "0", NULL_STR);
		else
			RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "1", NULL_STR);
	}
	else if ((opmode==BRIDGE_MODE) ||(opmode==WISP_MODE))
		RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "0", NULL_STR);
	#endif
	update_hw_ipv6_status();
#endif

	#if defined(CONFIG_APP_TR069)
	// enable tr069 connection request rule 
		SetRuleFortr069(pInterface, WanIpAddr);
	#endif //#if defined(CONFIG_APP_TR069)
	system("echo 1 > /etc/ppp/firewall_lock");

#ifdef USE_MINIUPNPD_V1.8 
	apmib_get(MIB_UPNP_ENABLED, (void *)&intVal);
	if(intVal==1)
		system("/bin/iptables_init.sh > /dev/null 2>&1");	
#endif

EXIT_setFirewallIptablesRules:
	RunSystemCmd("/proc/sys/net/ipv4/ip_forward", "echo", "1", NULL_STR);
	return 0;

}


#ifdef CONFIG_RTL_HW_NAPT
int update_hwnat_setting()
{
	int opmode=0;
	//fprintf(stderr,"--------------%s:%d -------------------------\n",__FUNCTION__,__LINE__);
	if(!apmib_get(MIB_OP_MODE, (void *)&opmode))
		goto error;
//for wisp and bridge(?) mode
	{
		if(opmode==BRIDGE_MODE)
		{
			RunSystemCmd("/proc/hw_nat", "echo", "2", NULL_STR);
			return 0;
		}
		else if(opmode==WISP_MODE)
		{
			RunSystemCmd("/proc/hw_nat", "echo", "3", NULL_STR);
			return 0;
		}
	}
//for subMask
	{
		int ivalue=0;
		if(!apmib_get(MIB_SUBNET_MASK,(void*)&ivalue))
			goto error;
		if((htonl(ivalue) & HW_NAT_LIMIT_NETMASK) != HW_NAT_LIMIT_NETMASK)
		{
			RunSystemCmd("/proc/hw_nat", "echo", "-1", NULL_STR);
			#ifdef CONFIG_8021Q_VLAN_SUPPORTED
			setLinuxVlan();
			#endif
			return 0;
		}		
	}
#ifdef CONFIG_RTK_VOIP
	//for voip
	{
		voipCfgParam_t  voipCfgParam ={0};
		if(!apmib_get(MIB_VOIP_CFG, (void*)&voipCfgParam))
			goto error;
		
		if(!voipCfgParam.hwnat_enable)
		{
			RunSystemCmd(HW_NAT_FILE, "echo", "0", NULL_STR);
			return 0;
		}
	}
#endif

//for url filter
	{
		int urlfilter_enable=0,urlfilter_num=0;
		if(!apmib_get(MIB_URLFILTER_ENABLED,  (void *)&urlfilter_enable))
			goto error;
		if(!apmib_get(MIB_URLFILTER_TBL_NUM,  (void *)&urlfilter_num))
			goto error;
		if(opmode == GATEWAY_MODE && urlfilter_enable!=0 && urlfilter_num>0)
		{
			RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
			return 0;
		}
	}
//for dos
	{
		int dos_enabled=0;
		if(!apmib_get(MIB_DOS_ENABLED, (void *)&dos_enabled))
			goto error;
		if(dos_enabled>0)
		{
			RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
			return 0;
		}
	}

//for l2tp,pptp
	{
    	int wan_dhcp;

        apmib_get(MIB_WAN_DHCP, (void *)&wan_dhcp);
        if ((wan_dhcp==L2TP) || (wan_dhcp==PPTP)) {
			RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
        	return 0;
		}
    }

	#if defined(QOS_OF_TR069) && !defined(CONFIG_RTL_HW_QOS_SUPPORT)
		int tr098_qos_enabled;
		apmib_get(MIB_QOS_ENABLE, (void *)&tr098_qos_enabled);
		if(tr098_qos_enabled)
		{
			RunSystemCmd("/proc/hw_nat", "echo", "0", NULL_STR);
			return 0;
		}
	#endif

	//printf("%s:%d\n",__FUNCTION__,__LINE__);
	RunSystemCmd(HW_NAT_FILE, "echo", "1", NULL_STR);
	return 0;
error:
	printf("update hardware nat error!\n");
	RunSystemCmd(HW_NAT_FILE, "echo", "-1", NULL_STR);
	return -1;
}
#endif


#if defined(CONFIG_RTL_HARDWARE_IPV6_SUPPORT)
int update_hw_ipv6_status()
{	
	int intVal = -1;
	int opmode = -1;
	int qosFlag = 0, ipv6_port_num = 0;
	int intVal_num = 0, ipv6_num = 0, index = 0;
	IPFILTER_T ip_entry;
	PORTFILTER_T port_entry;
	int QoS_Rule_EntryNum=0;
	IPQOS_T entry;

	if(apmib_get(MIB_OP_MODE, (void *)&opmode) == 0) {
		printf("Get op mode error! \n");
		return -1;
	}
	

	if (opmode == GATEWAY_MODE){

		//qos
		if ( apmib_get( MIB_QOS_ENABLED, (void *)&qosFlag) == 0) {
			printf("Get enabled flag error! \n");
			return -1;
		}

		if(qosFlag){
			#ifdef CONFIG_RTL_HW_QOS_SUPPORT
			apmib_get( MIB_QOS_RULE_TBL_NUM, (void *)&QoS_Rule_EntryNum);

			if(QoS_Rule_EntryNum > 0){
				for (index=1; index<=QoS_Rule_EntryNum; index++) {
					*((char *)&entry) = (char)index;
					apmib_get(MIB_QOS_RULE_TBL, (void *)&entry);
					if(strcmp(entry.ip6_src,"") != 0)
						break;
				}
				if(index == (QoS_Rule_EntryNum + 1))
					qosFlag = 0;
			}
			else
				qosFlag = 0;
			#endif
		}

		//check ipv6 ipfilter
		ipv6_num = 0;
		apmib_get(MIB_IPFILTER_ENABLED,  (void *)&intVal);
		apmib_get(MIB_IPFILTER_TBL_NUM,  (void *)&intVal_num);
		if(intVal ==1 && intVal_num>0){
			for(index = 1; index <= intVal_num ; index++){
				memset(&ip_entry, '\0', sizeof(ip_entry));
				*((char *)&ip_entry) = (char)index;
				apmib_get(MIB_IPFILTER_TBL, (void *)&ip_entry);
				#ifdef CONFIG_IPV6
				if(ip_entry.ipVer==IPv6){
					ipv6_num++;
				}
				#endif
			}
		}
		
		//check ipv6 portfilter
		ipv6_port_num = 0;
		apmib_get(MIB_PORTFILTER_ENABLED,  (void *)&intVal);
		apmib_get(MIB_PORTFILTER_TBL_NUM, (void *)&intVal_num);
		if(intVal ==1 && intVal_num>0){
			for (index=1; index<=intVal_num; index++) {
				memset(&port_entry, '\0', sizeof(port_entry));
				*((char *)&port_entry) = (char)index;
				apmib_get(MIB_PORTFILTER_TBL, (void *)&port_entry);
				#ifdef CONFIG_IPV6
				if(port_entry.ipVer == IPv6){
					ipv6_port_num++;
				}
				#endif
			}
		}
		printf("%s %d qosFlag=%d ipv6_num=%d ipv6_port_num=%d \n", __FUNCTION__, __LINE__, qosFlag, ipv6_num, ipv6_port_num);
		if(qosFlag || ipv6_num || ipv6_port_num){
			//RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "0", NULL_STR);
			RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "by_ipv6_route 0", NULL_STR);
		}
		else{
			//RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "1", NULL_STR);
			RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "by_ipv6_route 1", NULL_STR);
		}
	}
	else if ((opmode==BRIDGE_MODE) ||(opmode==WISP_MODE)){
		//RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "0", NULL_STR);
		RunSystemCmd(PROC_HWIPV6_ROUTING_FILE, "echo", "by_ipv6_route 0", NULL_STR);
	}

}
#endif



