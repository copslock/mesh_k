#ifndef __APPFUN_H__
#define __APPFUN_H__

/* System include files */
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ctype.h>
#include <regex.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/wireless.h>
#include <time.h>
#include <sys/wait.h> 
#include <net/if_arp.h> 
#include <arpa/inet.h> 

#include "apmib.h"
#include "cJSON.h"
#include "trace.h"

#define MIB_MAP_DEVICE_NAME        2389
#define IFACE_FLAG_T 0x01
#define IP_ADDR_T 0x02
#define NET_MASK_T 0x04
#define HW_ADDR_T 0x08
#define HW_NAT_LIMIT_NETMASK 0xFFFFFF00 //for arp table 512 limitation,

#define ARP_TABLE_MAX_NUM 256
#define _PATH_DHCPS_LEASES	"/var/lib/misc/udhcpd.leases"
#define _PATH_DHCPS_PID	"/var/run/udhcpd.pid" 
#define _PATH_DEVICE_MAC_BRAND "/etc/device_mac_brand.txt"
#define MAC_BCAST_ADDR		(unsigned char *) "\xff\xff\xff\xff\xff\xff"
#define IP_ADDR_T 0x02
#define HW_ADDR_T 0x08

#define APP_HTTP_ERR 		1001	//协议头部错误
#define APP_JSON_ERR 		1002	//JSON格式错误
#define APP_GETTING_ERR 	1003	//获取配置失败
#define APP_SETTING_SUC 	1004	//设置配置成功
#define APP_SETTING_ERR 	1005	//设置配置错误
#define APP_CHILD_UPGRADE 	1006	//子设备开始升级提醒
#define APP_SAME_GROUP		1007	//家长控制分组相同报错

#define MAX_L2_LIST_NUM    256


typedef struct app_wl_sta_info 
{
	unsigned char rssi;
	unsigned long tx_rate;
	unsigned long rx_rate;

	unsigned char addr[32];
	unsigned char model[32];
	unsigned char link_time[64];
	
} APP_WL_STA_INFO_T, *APP_WL_STA_INFO_Tp;

typedef struct app_wl_info
{
	int sta_num;
	struct app_wl_sta_info sta_info[256];
}APP_WL_INFO_T,*APP_WL_INFO_Tp;

typedef struct rtk_lan_device_info{
	unsigned char hostname[64];
	unsigned int ip;
	unsigned char mac[6];
	unsigned int expires;
	unsigned char conType;
	unsigned char brand[16];
	unsigned char on_link;
	//get from proc/rtl865x/asicCounter
	unsigned int rx_bytes;
	unsigned int tx_bytes;

    unsigned char slave_flg;    //device link in mesh slave
    unsigned int rx_speed;      
    unsigned int tx_speed;
    unsigned char	rssi;
	unsigned int linktime;		//link time
	
}RTK_LAN_DEVICE_INFO_T, *RTK_LAN_DEVICE_INFO_Tp;

enum LAN_LINK_TYPE {
	RTK_LINK_ERROR =0,
	RTK_ETHERNET,
	RTK_WIRELESS_5G,
	RTK_WIRELESS_2G
};

typedef struct rtk_arp_entry{	
	unsigned int ip;
	unsigned char mac[6];	
}RTK_ARP_ENTRY_T, *RTK_ARP_ENTRY_Tp;
#define ARP_TABLE_MAX_NUM 256
struct rtk_dhcp_client_info {
	unsigned char hostname[64];
	unsigned int ip;
	unsigned char mac[6];
	unsigned int expires;
	unsigned int linktime;
};
typedef struct rtk_l2Info_{
	unsigned char mac[6];
	int portNum;
} rtk_l2Info;
typedef struct rtk_asicConterInfo_{
	unsigned int rxBytes;
	unsigned int txBytes;
} rtk_asicConterInfo;

/*
**	parse /tmp/topology_json 
*/

#if 0
typedef struct mesh_child_info{
	char mac[32];
	char ip[32];
	char rssi[32];
	char device_name[32];
	char link_time[32];
	char master[32];
	int mem_num;		//终端数量
}MESH_CHILD_INFO,*P_MESH_CHILD_INFO;
typedef struct mesh_main_info{
    char mac[32];
    char ip[32];
    char hostname[32];
    char brand[32];
}MESH_MAIN_INFO,*P_MESH_MAIN_INFO;
typedef struct mesh_sta_info{
    char mac[32];
    char ip[32];
    char rssi[32];
    char hostname[32];
    char brand[32];
    char link_time[32];
    char master[32];
}MESH_STA_INFO,*P_MESH_STA_INFO;
typedef struct mesh_dev_info{
	MESH_MAIN_INFO main_info;
	MESH_CHILD_INFO child_info[12];
    MESH_STA_INFO sta_info[256];
    int sta_num;						//除了主节点的其他设备
    int child_num;						//MESH 子节点
}MESH_DEV_INFO,*P_MESH_DEV_INFO;

#else 
typedef struct mesh_sta_info{
    char mac[32];
    char ip[32];
    char rssi[32];
    char hostname[32];
    char brand[32];
    char link_time[32];
    char master[32];
}MESH_STA_INFO,*P_MESH_STA_INFO;

typedef struct mesh_neighbor_info{
    char mac[32];
	char ip[32];
    char rssi[32];
    char name[32];
    char band[32];
	char master[32];
	char link_time[32];
	struct mesh_sta_info sta_info[256];
	unsigned int sta_num; 
}MESH_NEIGHBOR_INFO,*P_MESH_NEIGHBOR_INFO;

typedef struct mesh_child_info{
	char mac[32];
	char ip[32];
	char rssi[32];
	char device_name[32];
	char link_time[32];
	char master[32];
	struct mesh_sta_info sta_info[256];
	unsigned int sta_num; 
}MESH_CHILD_INFO,*P_MESH_CHILD_INFO;

typedef struct mesh_main_info{
    char mac[32];
    char ip[32];
    char hostname[32];
    char brand[32];
	struct mesh_neighbor_info neighbor_info[12];
	unsigned int neighbor_num;
	struct mesh_sta_info sta_info[256];
	unsigned int sta_num; 
}MESH_MAIN_INFO,*P_MESH_MAIN_INFO;

typedef struct mesh_dev_info{
	MESH_MAIN_INFO main_info;
	MESH_CHILD_INFO child_info[12];
    MESH_STA_INFO sta_info[256];
    int sta_num;						//除了主节点的其他设备
    int child_num;						//MESH 子节点
}MESH_DEV_INFO,*P_MESH_DEV_INFO;


#endif

/****end****/


extern int str_to_mac(char *p_str, char *buffer);
extern int send_return_code(int fd, int ret_code);
extern int __inline__ string_to_dec(char *string, int *val);
extern int get_link_time(int link_second, char * p_date);
extern bool _isdigit(char c);
extern int app_init();
extern int get_mesh_json_sta_info( P_MESH_DEV_INFO mesh_json);

#endif