/*

File         : appEventHandler.h
Status       : Current
Description  : 

Author       : haopeng
Contact      : 376915244@qq.com

Description  : Primary released

## Please log your description here for your modication ##

Revision     : 
Modifier     : 
Description  : 
*/


#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/wireless.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>


#include "cJSON.h"
#include "apmib.h"
#include "appFun.h"
#include "../boa/src/deviceProcIf.h"



//#include "../boa/src/utility.h"



#define MAX_MSG_BUFFER_SIZE 256
#define _DHCPD_PROG_NAME	"udhcpd"
#define _DHCPD_PID_PATH		"/var/run"
#define _CONFIG_SCRIPT_PROG	"init.sh"
#define _FIREWALL_SCRIPT_PROG	"firewall.sh"
#define _QOS_SCRIPT_PROG	    "ip_qos.sh"


#ifdef __i386__
	#define _CONFIG_SCRIPT_PATH	"."
	#define _LITTLE_ENDIAN_
#else
	#define _CONFIG_SCRIPT_PATH	"/bin"
#endif


#define MAC_STR_LEN 17 		//XX:XX:XX:XX:XX:XX

#if 1
enum APP_EVENT_REQUEST {
	APP_NONE_CON_REQ = 100,
	WIRELESS_SETTINGS_REQ,
	HOME_SETTINGS_REQ,
	LINK_DEVICE_SETTINGS_REQ,
	CHILD_DEVICE_SETTINGS_REQ,
	OPTION_SETTINGS_REQ,
	GUEST_NETWORK_SETTINGS_REQ,
	PARENTAL_SETTINGS_REQ,
	NETWORK_SETTINGS_REQ,
	QOS_SETTINGS_REQ,
	MAC_FILTER_REQ,
	IP_FILTER_REQ,
	URL_FILTER_REQ,
	UPGRADE_REQ,
	ENVIRONMENT_REQ,
	SYSTEM_SETTINGS_REQ,	//115
	LOGIN_SETTINGS_REQ,
	TIME_ZONE_SETTINGS_REQ,
	APP_PWD_SETTINGS_REQ,
	LANG_SETTINGS_REQ,
	CONFIGURE_ROUTER_EVENT
};

#endif
struct user_net_device_stats {
    unsigned long long rx_packets;	/* total packets received       */
    unsigned long long tx_packets;	/* total packets transmitted    */
    unsigned long long rx_bytes;	/* total bytes received         */
    unsigned long long tx_bytes;	/* total bytes transmitted      */
    unsigned long rx_errors;	/* bad packets received         */
    unsigned long tx_errors;	/* packet transmit problems     */
    unsigned long rx_dropped;	/* no space in linux buffers    */
    unsigned long tx_dropped;	/* no space available in linux  */
    unsigned long rx_multicast;	/* multicast packets received   */
	unsigned long tx_multicast;	/* multicast packets transmitted   */
	unsigned long rx_unicast;	/* unicast packets received   */
	unsigned long tx_unicast;	/* unicast packets transmitted   */
	unsigned long rx_broadcast;	/* broadcast packets received   */
	unsigned long tx_broadcast;	/* broadcast packets transmitted   */
    unsigned long rx_compressed;
    unsigned long tx_compressed;
    unsigned long collisions;

    /* detailed rx_errors: */
    unsigned long rx_length_errors;
    unsigned long rx_over_errors;	/* receiver ring buff overflow  */
    unsigned long rx_crc_errors;	/* recved pkt with crc error    */
    unsigned long rx_frame_errors;	/* recv'd frame alignment error */
    unsigned long rx_fifo_errors;	/* recv'r fifo overrun          */
    unsigned long rx_missed_errors;	/* receiver missed packet     */
    /* detailed tx_errors */
    unsigned long tx_aborted_errors;
    unsigned long tx_carrier_errors;
    unsigned long tx_fifo_errors;
    unsigned long tx_heartbeat_errors;
    unsigned long tx_window_errors;
};

typedef struct rtk_wlan_sta_info 
{
 unsigned short aid;
 unsigned char  addr[6];
 unsigned long  tx_packets;
 unsigned long  rx_packets;
 unsigned long  expired_time; // 10 msec unit
 unsigned short flag;
 unsigned char  txOperaRates;
 unsigned char  rssi;
 unsigned long  link_time;  // 1 sec unit
 unsigned long  tx_fail;
 unsigned long  tx_bytes;
 unsigned long  rx_bytes;
 unsigned char  network;
 unsigned char  ht_info; // bit0: 0=20M mode, 1=40M mode; bit1: 0=longGI, 1=shortGI
 unsigned char  RxOperaRate;
 unsigned char  resv[5];
} RTK_WLAN_STA_INFO_T, *RTK_WLAN_STA_INFO_Tp;

//changes in following table should be synced to VHT_MCS_DATA_RATE[] in 8812_vht_gen.c
// 				20/40/80,	ShortGI,	MCS Rate 
const unsigned short VHT_MCS_DATA_RATE[3][2][30] = 
{	{	{13, 26, 39, 52, 78, 104, 117, 130, 156, 156,
		 26, 52, 78, 104, 156, 208, 234, 260, 312, 312, 
		 39, 78, 117, 156, 234, 312, 351, 390, 468, 520},					// Long GI, 20MHz
		 
		{14, 29, 43, 58, 87, 116, 130, 144, 173, 173,
		 29, 58, 87, 116, 173, 231, 260, 289, 347, 347,
		 43, 86, 130, 173, 260, 347, 390, 433, 520, 578}			},		// Short GI, 20MHz
		 
	{	{27, 54, 81, 108, 162, 216, 243, 270, 324, 360, 
		 54, 108, 162, 216, 324, 432, 486, 540, 648, 720, 
		 81, 162, 243, 342, 486, 648, 729, 810, 972, 1080}, 				// Long GI, 40MHz
		 
		{30, 60, 90, 120, 180, 240, 270, 300,360, 400, 
		 60, 120, 180, 240, 360, 480, 540, 600, 720, 800,
		 90, 180, 270, 360, 540, 720, 810, 900, 1080, 1200}			},		// Short GI, 40MHz
		 
	{	{59, 117,  176, 234, 351, 468, 527, 585, 702, 780,
		 117, 234, 351, 468, 702, 936, 1053, 1170, 1404, 1560, 
		 176, 351, 527, 702, 1053, 1408, 1408, 1745, 2106, 2340}, 			// Long GI, 80MHz
		 
		{65, 130, 195, 260, 390, 520, 585, 650, 780, 867, 
		 130, 260, 390, 520, 780, 1040, 1170, 1300, 1560, 1733, 
		 195, 390, 585, 780, 1170, 1560, 1560, 1950, 2340, 2600}	}		// Short GI, 80MHz
		 
};
#define SSID_LEN	32
		 
#if defined(CONFIG_RTK_MESH) ||  defined(CONFIG_RTL_819X)  /*add for RTL819X since wlan driver default include mesh data*/
		 //by GANTOE for site survey 2008/12/26
#define MESHID_LEN 32 
#endif 
		 
#define	MAX_BSS_DESC	64
#define MAX_MSG_BUFFER_SIZE 256

typedef struct bss_info
{
    unsigned char state;
    unsigned char channel;
    unsigned char txRate;
    unsigned char bssid[6];
    unsigned char rssi, sq;	// RSSI  and signal strength
    unsigned char ssid[SSID_LEN+1];
} RTK_BSS_INFO, *RTK_BSS_INFOp;


typedef enum _Capability {
    cESS 		= 0x01,
    cIBSS		= 0x02,
    cPollable		= 0x04,
    cPollReq		= 0x01,
    cPrivacy		= 0x10,
    cShortPreamble	= 0x20,
} Capability;

typedef struct _bss_info {
    unsigned char state;
    unsigned char channel;
    unsigned char txRate;
    unsigned char bssid[6];
    unsigned char rssi, sq;	// RSSI  and signal strength
    unsigned char ssid[SSID_LEN+1];
} bss_info;

typedef struct _OCTET_STRING {
    unsigned char *Octet;
    unsigned short Length;
} OCTET_STRING;
typedef	struct _IbssParms {
    unsigned short	atimWin;
} IbssParms;

typedef enum _BssType {
    infrastructure = 1,
    independent = 2,
} BssType;

typedef struct _BssDscr {
    unsigned char bdBssId[6];
    unsigned char bdSsIdBuf[SSID_LEN];
    OCTET_STRING  bdSsId;

#if defined(CONFIG_RTK_MESH) || defined(CONFIG_RTL_819X) 
	//by GANTOE for site survey 2008/12/26
	unsigned char bdMeshIdBuf[MESHID_LEN]; 
	OCTET_STRING bdMeshId; 
#endif 
    BssType bdType;
    unsigned short bdBcnPer;			// beacon period in Time Units
    unsigned char bdDtimPer;			// DTIM period in beacon periods
    unsigned long bdTstamp[2];			// 8 Octets from ProbeRsp/Beacon
    IbssParms bdIbssParms;			// empty if infrastructure BSS
    unsigned short bdCap;				// capability information
    unsigned char ChannelNumber;			// channel number
    unsigned long bdBrates;
    unsigned long bdSupportRates;		
    unsigned char bdsa[6];			// SA address
    unsigned char rssi, sq;			// RSSI and signal strength
    unsigned char network;			// 1: 11B, 2: 11G, 4:11G
	// P2P_SUPPORT
	unsigned char	p2pdevname[33];		
	unsigned char	p2prole;	
	unsigned short	p2pwscconfig;		
	unsigned char	p2paddress[6];	
	unsigned char   stage;
	
	#if defined(CONFIG_RTL_COMAPI_WLTOOLS)
    unsigned char	    wpa_ie_len;
    unsigned char	    wpa_ie[256];
    unsigned char	    rsn_ie_len;    
    unsigned char	    rsn_ie[256];
    #endif       
} BssDscr, *pBssDscr;

typedef struct _sitesurvey_status {
    unsigned char number;
    unsigned char pad[3];
    BssDscr bssdb[MAX_BSS_DESC];
} SS_STATUS_T, *SS_STATUS_Tp;

typedef struct app_env_sta_info 
{
	unsigned char  rssi;
	unsigned char  channel;
	unsigned long  bdBrates;
	unsigned char  addr[32];
	unsigned char  ssid[64];
	
} APP_ENV_STA_INFO_T, *APP_ENV_STA_INFO_Tp;


typedef struct app_env_info
{
	int sta_num;
	struct app_env_sta_info sta_info[256];
}APP_ENV_INFO_T,*APP_ENV_INFO_Tp;
void int_to_str(int i, char *string);
void encode64_passwd(char *outbuf, char *string, int outlen);
static int str_parse(char *source, char *begin, char *end, char *target);
int get_target_string_flag(const char* sourceStr);
int generate_jason_strings(char send_buff[],cJSON *jason_struct);

/*
* once get request configuration from client ,call below func to send related settings to client
*/
int app_event_handler(int fd,char *buffer);
int parse_Set_conf( int fd,char *jasonData);










