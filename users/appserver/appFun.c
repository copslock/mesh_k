#include "appFun.h"
//#include "appEventHandler.h"

typedef enum { IP_ADDR, DST_IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;
#define RTF_UP			0x0001          //route usable
#define RTF_GATEWAY		0x0002          //destination is a gateway
#define _PATH_PROCNET_ROUTE	"/proc/net/route"
//changes in following table should be synced to MCS_DATA_RATEStr[] in 8190n_proc.c

/* 022311667782 to 02:23:11:66:77:82 */
int str_to_mac(char *p_str, char *buffer)
{
	int i;
	int step = 0;
	memset(buffer, 0, sizeof(buffer));


	//printf("in mac buffer = %s\n", p_str);
	if (strlen(p_str)!= 12)
		return -1;

	for(i=0; i<5; i++)
	{
		strncat(buffer, p_str+step, 2);
		strcat(buffer, ":");
		step+=2;
	}

	strncat(buffer, p_str+step, 2);
	//printf("out of mac buffer = %s\n", buffer);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int isConnectPPP()
{
	struct stat status;

	if ( stat("/etc/ppp/link", &status) < 0)
		return 0;

	return 1;
}
static int SetWlan_idx(char * wlan_iface_name)
{
	int idx;
	
		idx = atoi(&wlan_iface_name[4]);
		if (idx >= NUM_WLAN_INTERFACE) {
				printf("invalid wlan interface index number!\n");
				return 0;
		}
		wlan_idx = idx;
		vwlan_idx = 0;
	
#ifdef MBSSID		
		
		if (strlen(wlan_iface_name) >= 9 && wlan_iface_name[5] == '-' &&
				wlan_iface_name[6] == 'v' && wlan_iface_name[7] == 'a') {
				idx = atoi(&wlan_iface_name[8]);
				if (idx >= NUM_VWLAN_INTERFACE) {
					printf("invalid virtual wlan interface index number!\n");
					return 0;
				}
				
				vwlan_idx = idx+1;
				idx = atoi(&wlan_iface_name[4]);
				wlan_idx = idx;
		}
#endif	
#ifdef UNIVERSAL_REPEATER
				if (strlen(wlan_iface_name) >= 9 && wlan_iface_name[5] == '-' &&
						!memcmp(&wlan_iface_name[6], "vxd", 3)) {
					vwlan_idx = NUM_VWLAN_INTERFACE;
					idx = atoi(&wlan_iface_name[4]);
					wlan_idx = idx;
				}
	
#endif				
				
return 1;		
}

#if 0
#ifdef CONFIG_SMART_REPEATER
int getWispRptIface(char**pIface,int wlanId)
{
	int rptEnabled=0,wlanMode=0,opMode=0;
	char wlan_wanIfName[16]={0};
	if(wlanId == 0)
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
	else if(1 == wlanId)
		apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);
	else return -1;
	apmib_get(MIB_OP_MODE,(void *)&opMode);
	if(opMode!=WISP_MODE)
		return -1;
	apmib_save_wlanIdx();
	
	sprintf(wlan_wanIfName,"wlan%d",wlanId);
	SetWlan_idx(wlan_wanIfName);
	//for wisp rpt mode,only care root ap
	apmib_get(MIB_WLAN_MODE, (void *)&wlanMode);
	if((AP_MODE==wlanMode || AP_MESH_MODE==wlanMode || MESH_MODE==wlanMode) && rptEnabled)
	{
		if(wlanId == 0)
			*pIface = "wlan0-vxd";
		else if(1 == wlanId)
			*pIface = "wlan1-vxd";
		else return -1;
	}else
	{
		char * ptmp = strstr(*pIface,"-vxd");
		if(ptmp)
			memset(ptmp,0,sizeof(char)*strlen("-vxd"));
	}
	apmib_recov_wlanIdx();
	return 0;
}

#endif
#endif


static int re865xIoctl(char *name, unsigned int arg0, unsigned int arg1, unsigned int arg2, unsigned int arg3)
{
	unsigned int args[4];
	struct ifreq ifr;
	int sockfd;

	args[0] = arg0;
	args[1] = arg1;
	args[2] = arg2;
	args[3] = arg3;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("fatal error socket\n");
		return -3;
	}

	strcpy((char*)&ifr.ifr_name, name);
	((unsigned int *)(&ifr.ifr_data))[0] = (unsigned int)args;

	if (ioctl(sockfd, SIOCDEVPRIVATE, &ifr)<0)
	{
		perror("device ioctl:");
		close(sockfd);
		return -1;
	}
	close(sockfd);
	return 0;
}
#define RTL8651_IOCTL_GETWANLINKSTATUS 2000
#if 0
int getWanLink(char *interface)
{
	unsigned int    ret;
	unsigned int    args[0];

	re865xIoctl(interface, RTL8651_IOCTL_GETWANLINKSTATUS, (unsigned int)(args), 0, (unsigned int)&ret) ;
	return ret;
}



/////////////////////////////////////////////////////////////////////////////
int getInAddr( char *interface, ADDR_T type, void *pAddr )
{
    struct ifreq ifr;
    int skfd=0, found=0;
    struct sockaddr_in *addr;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return 0;
		
    strcpy(ifr.ifr_name, interface);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0){
    	close( skfd );
		return (0);
	}
    if (type == HW_ADDR) {
    	if (ioctl(skfd, SIOCGIFHWADDR, &ifr) >= 0) {
		memcpy(pAddr, &ifr.ifr_hwaddr, sizeof(struct sockaddr));
		found = 1;
	}
    }
    else if (type == IP_ADDR) {
	if (ioctl(skfd, SIOCGIFADDR, &ifr) == 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    }
    else if (type == SUBNET_MASK) {
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) >= 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
    }
	else if (type == DST_IP_ADDR)
	{
		if (ioctl(skfd, SIOCGIFDSTADDR, &ifr) >= 0) {
		addr = ((struct sockaddr_in *)&ifr.ifr_addr);
		*((struct in_addr *)pAddr) = *((struct in_addr *)&addr->sin_addr);
		found = 1;
	}
	}
    close( skfd );
    return found;

}
int getDefaultRoute(char *interface, struct in_addr *route)
{
	char buff[1024], iface[16];
	char gate_addr[128], net_addr[128], mask_addr[128];
	int num, iflags, metric, refcnt, use, mss, window, irtt;
	FILE *fp = fopen(_PATH_PROCNET_ROUTE, "r");
	char *fmt;
	int found=0;
	unsigned long addr;

	if (!fp) {
		printf("Open %s file error.\n", _PATH_PROCNET_ROUTE);
		return 0;
    }

	fmt = "%16s %128s %128s %X %d %d %d %128s %d %d %d";

	while (fgets(buff, 1023, fp)) {
		num = sscanf(buff, fmt, iface, net_addr, gate_addr,
		     		&iflags, &refcnt, &use, &metric, mask_addr, &mss, &window, &irtt);
		if (num < 10 || !(iflags & RTF_UP) || !(iflags & RTF_GATEWAY) || strcmp(iface, interface))
	    		continue;
		sscanf(gate_addr, "%lx", &addr );
		*route = *((struct in_addr *)&addr);

		found = 1;
		break;
	}

    	fclose(fp);
    	return found;
}

int getWanInfo(char *pWanIP, char *pWanMask, char *pWanDefIP, char *pWanHWAddr)
{
	DHCP_T dhcp;
	OPMODE_T opmode=-1;
	unsigned int wispWanId=0;
	char *iface=NULL;
	struct in_addr	intaddr;
	struct sockaddr hwaddr;
	unsigned char *pMacAddr;
	int isWanPhyLink = 0;	
	if ( !apmib_get( MIB_WAN_DHCP, (void *)&dhcp) )
		return -1;
  
  if ( !apmib_get( MIB_OP_MODE, (void *)&opmode) )
		return -1;

	if( !apmib_get(MIB_WISP_WAN_ID, (void *)&wispWanId))
		return -1;
	
	if ( dhcp == PPPOE || dhcp == PPTP || dhcp == L2TP || dhcp == USB3G ) { /* # keith: add l2tp support. 20080515 */
#ifdef MULTI_PPPOE
	if(dhcp == PPPOE){
		extern char  ppp_iface[32];
		iface = ppp_iface;
	}
#else
	iface = "ppp0";
#endif
		
		if ( !isConnectPPP() )
			iface = NULL;
	}
	else if (opmode == WISP_MODE){
		if(0 == wispWanId)
			iface = "wlan0";
		else if(1 == wispWanId)
			iface = "wlan1";
#ifdef CONFIG_SMART_REPEATER
		if(getWispRptIface(&iface,wispWanId)<0)
					return -1;
#endif			
	}
	else
		iface = "eth1";


	
#if defined(CONFIG_4G_LTE_SUPPORT)
	if (lte_wan()) {
		iface = "usb0";
		isWanPhyLink = 1;
	} else
#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */

	if(opmode != WISP_MODE)
	{
		if(iface){
			if((isWanPhyLink = getWanLink("eth1")) < 0){
				sprintf(pWanIP,"%s","0.0.0.0");
			}
		}	
	}

	
	if ( iface && getInAddr(iface, IP_ADDR, (void *)&intaddr ) && ((isWanPhyLink >= 0)) )
		sprintf(pWanIP,"%s",inet_ntoa(intaddr));
	else
		sprintf(pWanIP,"%s","0.0.0.0");

	if ( iface && getInAddr(iface, SUBNET_MASK, (void *)&intaddr ) && ((isWanPhyLink >= 0) ))
		sprintf(pWanMask,"%s",inet_ntoa(intaddr));
	else
		sprintf(pWanMask,"%s","0.0.0.0");

	if ( iface && getDefaultRoute(iface, &intaddr) && ((isWanPhyLink >= 0) )) {			
			sprintf(pWanDefIP,"%s",inet_ntoa(intaddr));
	}	
	else {
			sprintf(pWanDefIP,"%s","0.0.0.0");	
	}

	
#ifdef _ALPHA_DUAL_WAN_SUPPORT_
	if (dhcp == PPPOE)
	{
		if (getInAddr("eth1", IP_ADDR, (void *)&intaddr ) && ((isWanPhyLink >= 0)) ) {
			strcat(pWanIP, ", ");
			strcat(pWanIP, inet_ntoa(intaddr));
		}
		else
			strcat(pWanIP, ", 0.0.0.0");

		if (getInAddr("eth1", SUBNET_MASK, (void *)&intaddr ) && ((isWanPhyLink >= 0) )) {
			strcat(pWanMask, ", ");
			strcat(pWanMask, inet_ntoa(intaddr));
		}
		else
			strcat(pWanMask, ", 0.0.0.0");
		
		if (getDefaultRoute("eth1", &intaddr) && ((isWanPhyLink >= 0) )) {
			strcat(pWanDefIP, ", ");
			strcat(pWanDefIP, inet_ntoa(intaddr));
		}
		else
			strcat(pWanDefIP, ", 0.0.0.0");
	}
#endif

	//To get wan hw addr
	if(opmode == WISP_MODE) {
		if(0 == wispWanId)
			iface = "wlan0";
		else if(1 == wispWanId)
			iface = "wlan1";
#ifdef CONFIG_SMART_REPEATER
		if(getWispRptIface(&iface,wispWanId)<0)
					return -1;
#endif			
	}	
	else
		iface = "eth1";
	
#if defined(CONFIG_4G_LTE_SUPPORT)
	if (lte_wan()) {
		iface = "usb0";
	}
#endif /* #if defined(CONFIG_4G_LTE_SUPPORT) */
	if ( getInAddr(iface, HW_ADDR, (void *)&hwaddr ) ) 
	{
		pMacAddr = (unsigned char *)hwaddr.sa_data;
		sprintf(pWanHWAddr,"%02x:%02x:%02x:%02x:%02x:%02x",pMacAddr[0], pMacAddr[1],pMacAddr[2], pMacAddr[3], pMacAddr[4], pMacAddr[5]);
	}
	else
		sprintf(pWanHWAddr,"%s","00:00:00:00:00:00");

	return 0;
}
#endif

int send_return_code(int fd, int ret_code)
{
	char buffer[1024] = {0};
	char send_buff[4096] = {0};
	char http_head[512];
	int http_head_len = 0;
	int j=0;
	j += sprintf(http_head+j, "HTTP/1.1 200 OK\r\n");
	j += sprintf(http_head+j, "Content-type: text/html\r\n");
	j += sprintf(http_head+j, "Expires: Thu, 15 Dec 2016 12:00:00 GMT\r\n");
	j += sprintf(http_head+j, "Connection: close\r\n");
	j += sprintf(http_head+j, "Pragma: no-cache\r\n");


	sprintf(buffer, "retCode=%d", ret_code); 

	sprintf(http_head+j, "Content-Length: %d\r\n\r\n",strlen(buffer));
	http_head_len = sprintf(send_buff, http_head);
	sprintf(send_buff+http_head_len, buffer);

	if (ret_code != APP_HTTP_ERR)
		DTRACE(DTRACE_APPSERVER, "[%s-%d]send_buff = %s\n", __FUNCTION__, __LINE__, send_buff);

	if( send(fd, send_buff, strlen(send_buff), 0) != strlen(send_buff) ) 
	{
		perror("send configuration error"); 
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;


}
// Validate digit
bool _isdigit(char c)
{
    return ((c >= '0') && (c <= '9'));
}

int __inline__ string_to_dec(char *string, int *val)
{
	int idx;
	int len = strlen(string);

	for (idx=0; idx<len; idx++) {
		if ( !_isdigit(string[idx]))
			return 0;
	}

	*val = strtol(string, (char**)NULL, 10);
	return 1;
}

int get_link_time(int link_second, char * p_date)
{
    time_t nSeconds;
	struct tm * pTM ;
    
    nSeconds = time(NULL) - link_second;
	pTM = localtime(&nSeconds);

	sprintf(p_date, "%04d-%02d-%02d %02d:%02d", pTM->tm_year+1900, pTM->tm_mon+1, pTM->tm_mday, pTM->tm_hour, pTM->tm_min);
	//printf("link date = %s\n", p_date);

	return 0;

}
int __inline__ apmib_update_web(int type)
{
	int ret;

#if defined(CONFIG_RTL_ULINKER)
	/*
		For auto mode, we need to keep two wlan mib settings for ap/client.
		Currently, we use WLAN0_VAP5 for save AP value and WLAN0_VAP6 for Client
		When user save value to root ap, we will copy it to corresponding mib.
	*/
	if (type == CURRENT_SETTING)
	{
		extern int set_domain_name_query_ready(int val);
		set_domain_name_query_ready(2);
	
		dbg_wlan_mib(1);
		if (pMib->ulinker_auto == 1)
			pMib->wlan[0][0].wlanDisabled = 0;

		if (pMib->wlan[0][0].wlanMode == ULINKER_WL_AP)
		{
			pMib->ulinker_cur_wl_mode = ULINKER_WL_AP;
			pMib->ulinker_lst_wl_mode = ULINKER_WL_CL;

		#if defined(UNIVERSAL_REPEATER)
			if (pMib->repeaterEnabled1 == 1) {
				ulinker_wlan_mib_copy(&pMib->wlan[0][ULINKER_RPT_MIB], &pMib->wlan[0][0]);
			}
			else 
		#endif
			{
				ulinker_wlan_mib_copy(&pMib->wlan[0][ULINKER_AP_MIB], &pMib->wlan[0][0]);
			}
		}
		else if (pMib->wlan[0][0].wlanMode == ULINKER_WL_CL)
		{
			pMib->ulinker_cur_wl_mode = ULINKER_WL_CL;
			pMib->ulinker_lst_wl_mode = ULINKER_WL_AP;

			ulinker_wlan_mib_copy(&pMib->wlan[0][ULINKER_CL_MIB], &pMib->wlan[0][0]);
		}

		/* 
			backup repeater value, because auto mode need to keep repeater disable, 
			we backup this value and restore it when device switch to manual mode. 
		*/
		if (pMib->ulinker_auto == 0)
		{
			pMib->ulinker_repeaterEnabled1 = pMib->repeaterEnabled1;
			pMib->ulinker_repeaterEnabled2 = pMib->repeaterEnabled2;
		}
		dbg_wlan_mib(2);
	}
#endif

	ret = apmib_update(type);

	if (ret == 0)
		return 0;

	if (type & CURRENT_SETTING) {
		save_cs_to_file();
	}
	return ret;
}

int timezone_init()
{
	char value[32] = {0};
	int enabled = 0;

	sprintf(value, "%s", "-8 1");
	apmib_set(MIB_NTP_TIMEZONE, (void *)value);

	enabled = 1; 
	apmib_set(MIB_NTP_ENABLED, (void *)&enabled);

	apmib_update_web(CURRENT_SETTING);
//Brad modify for system re-init method
#if 0
	pid = find_pid_by_name("ntp.sh");
	if(pid)
		kill(pid, SIGTERM);

	pid = fork();
		if (pid)
		waitpid(pid, NULL, 0);
		else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _NTP_SCRIPT_PROG);
		execl( tmpBuf, _NTP_SCRIPT_PROG, NULL);
				exit(1);
		}
#endif
#ifndef NO_ACTION
	//run_init_script("all");
#endif
	return 0;
}

int app_init()
{
	int val;
	
	apmib_get(MIB_LED_ENABLE, (void *)&val);
	if (val == 1)
	{
		DTRACE(DTRACE_APPSERVER,"led mib is on\n");
		system("echo 1 > /proc/gpio");
	}
	else
	{
		DTRACE(DTRACE_APPSERVER,"led mib is off\n");
		system("echo 0 > /proc/gpio");
	}

	//timezone_init();

	
	return 0;
}


/* Macro definition */
static int _is_hex(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'F')) ||
            ((c >= 'a') && (c <= 'f')));
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

static int getDhcpClient(char **ppStart, unsigned long *size, unsigned char *hname, unsigned int *ip, unsigned char *mac, unsigned int *lease, unsigned int *linktime)
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
	*linktime=entry.linktime;
	
	
	if(entry.hostname[0])
	{
		strcpy(hname, entry.hostname);
	}
	
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
	unsigned int ip, lease, linktime;
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
		ret = getDhcpClient(&ptr, &fileSize, hostname, &ip, mac, &lease, &linktime);
		//printf("%s:%d ret=%d, hostname = %s\n",__FUNCTION__,__LINE__,ret, hostname);

		if (ret < 0)
			break;
		if (ret == 0)
			continue;

		strcpy(pclient[idx].hostname, hostname);
		pclient[idx].ip=ip;
		memcpy(pclient[idx].mac, mac, 6);
		pclient[idx].expires=lease;
		pclient[idx].linktime=linktime;
		
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

static int string_to_hex(char *string, unsigned char *key, int len)
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
int rtk_get_lan_device_info(unsigned int *num, RTK_LAN_DEVICE_INFO_Tp pdevinfo,int max_num)
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
	l2_tab_num=get_info_from_l2_tab("/proc/rtl865x/l2", l2list);
	for(i=0;i<l2_tab_num;i++)
	{//assign all mac in pdevinfo, get mac index. if mac not exist, add it to pdevinfo arrary
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
	//printf("%s:%d arp_entry_num = %d\n",__FUNCTION__,__LINE__, arp_entry_num);

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
			pdevinfo[devInfoIdx].linktime=dhcp_client_info[i].linktime;
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


cJSON * creatJSONMeshStationOBJ(RTK_LAN_DEVICE_INFO_T *devinfo)
{
    cJSON *obj;
    char buf[512];
    
    obj = cJSON_CreateObject();
    if(obj == NULL)
        return NULL;

    sprintf(buf,"%02x%02x%02x%02x%02x%02x",devinfo->mac[0], devinfo->mac[1], devinfo->mac[2], devinfo->mac[3], devinfo->mac[4], devinfo->mac[5]);
    cJSON_AddStringToObject(obj, "station_mac", buf);
    sprintf(buf, "%d", devinfo->rssi);
    cJSON_AddStringToObject(obj, "station_rssi", buf);
    if(devinfo->conType == RTK_ETHERNET)
    {
        sprintf(buf,"ETHERNET");
    }
    else if(devinfo->conType == RTK_WIRELESS_5G)
    {
        sprintf(buf,"5G");
    }
    else if(devinfo->conType == RTK_WIRELESS_2G)
    {
        sprintf(buf,"2G");
    }
    else
    {
        sprintf(buf,"ERR");
    }
    cJSON_AddStringToObject(obj, "station_connected_band", buf);
    sprintf(buf, "%d", devinfo->rx_speed);
    cJSON_AddStringToObject(obj, "station_downlink", buf);
    sprintf(buf, "%d", devinfo->tx_speed);
    cJSON_AddStringToObject(obj, "station_uplink", buf);

    strcpy(buf,inet_ntoa((*((struct in_addr *)&(devinfo->ip)))));
    cJSON_AddStringToObject(obj, "station_ip", buf);
    cJSON_AddStringToObject(obj, "station_hostname", devinfo->hostname);
    cJSON_AddStringToObject(obj, "station_brand", devinfo->brand);
    cJSON_AddStringToObject(obj, "station_link_time", "null");
#if 0
            {
                "station_mac":"40331a573909",
                "station_rssi":"61",
                "station_connected_band":"5G",
                "station_downlink":"150",
                "station_uplink":"108"
                "station_ip":"192.168.1.10"
                "station_hostname":"mate20"
                "station_brand":"HUAWEI"
                "station_link_time":"68"
            }
    
#endif

    return obj;
}

int getMacIdx(RTK_LAN_DEVICE_INFO_T *devinfo, unsigned char mac[6], int max_num)
{
	int i = 0;

	for(i=0;i<max_num;i++)
	{
		if(memcmp(devinfo[i].mac, mac, 6)== 0)
		{
			return i;
		}
	}
	return max_num;
}

void addExtStationInfoToTopology(cJSON * obj, RTK_LAN_DEVICE_INFO_T *devinfo)
{
    cJSON *head;
    cJSON *pos;
    cJSON *station_mac;
    unsigned char mac[6];
    int idx;
    char buf[20];

    head = cJSON_GetObjectItem(obj, "mac_address");
    if(head != NULL)
    {
        if(string_to_hex(head->valuestring, mac, 12))
        {
            idx = getMacIdx(devinfo, mac, MAX_STA_NUM);
            if(idx < MAX_STA_NUM)
            {
                devinfo[idx].slave_flg = 1;    //mark mesh slave device
            }
        }
    }
    
    head = cJSON_GetObjectItem(obj, "station_info");
    if(head != NULL)
    {
        for(pos = head->child; pos != NULL; pos = pos->next)
        {
            station_mac = cJSON_GetObjectItem(pos, "station_mac");
            if(station_mac != NULL)
            {
                if(string_to_hex(station_mac->valuestring, mac, 12))
                {
                    idx = getMacIdx(devinfo, mac, MAX_STA_NUM);
                    if(idx < MAX_STA_NUM)
                    {
                        strcpy(buf, inet_ntoa((*((struct in_addr *)&(devinfo[idx].ip)))));
                        cJSON_AddStringToObject(pos, "station_ip", buf);
                        cJSON_AddStringToObject(pos, "station_hostname", devinfo[idx].hostname);
                        cJSON_AddStringToObject(pos, "station_brand", devinfo[idx].brand);
                        cJSON_AddStringToObject(pos, "station_link_time", "null");

                        devinfo[idx].slave_flg = 1;
                    }
                }
            }
        }
    }
    else
    {
        printf("[%s:%d] can't find station_info.", __FUNCTION__, __LINE__);
    }

    head = cJSON_GetObjectItem(obj, "child_devices");

    if(head != NULL)
    {
        for(pos = head->child; pos != NULL; pos = pos->next)
        {
            addExtStationInfoToTopology(pos, devinfo);
        }
    }
    else
    {
        printf("[%s:%d] can't find child_devices.", __FUNCTION__, __LINE__);
    }

    return;
}

/*
{
       "device_name":"EasyMesh_Device_host",
       "ip_addr":"192.168.1.211",
       "mac_address":"cc2d2110f0dc",
       "neighbor_devices":[
           {
               "neighbor_mac":"00e046614455",
               "neighbor_name":"EasyMesh_Device2",
               "neighbor_rssi":"43",
               "neighbor_band":"TBU"
           }
       ],
       "station_info":[
           {
               "station_mac":"40331a573909",
               "station_rssi":"61",
               "station_connected_band":"5G",
               "station_downlink":"150",
               "station_uplink":"108"
               "station_ip":"192.168.1.10"
               "station_hostname":"mate20"
               "station_brand":"HUAWEI"
               "station_link_time":"68"
           }
       ],
       "child_devices":[
           Object{...},
           Object{...}
       ]
   }
*/

cJSON *getMeshTopologyJSON()
{
    FILE *fp;
    cJSON *root;
    char buf[512];
    RTK_LAN_DEVICE_INFO_T devinfo[MAX_STA_NUM] = {0};
    int num = 0;
    cJSON *obj_station;
    int i;

    rtk_get_lan_device_info(&num, devinfo, MAX_STA_NUM);
    fp = fopen("/tmp/topology_json", "r");
	if (fp == NULL) 
    {
        struct in_addr	intaddr;
        struct sockaddr hwaddr;
        unsigned char *mac;
        
		root = cJSON_CreateObject();
        if(root == NULL)
        {
            printf("[%s:%d] cJSON_CreateObject fail.", __FUNCTION__, __LINE__);
            return -1;
        }
        if (!apmib_get( MIB_MAP_DEVICE_NAME, (void *)buf)) 
        {
			sprintf(buf, "%s", "null" );
		}
        cJSON_AddStringToObject(root, "device_name", buf);

        getInAddr("br0", IP_ADDR, (void *)&intaddr );
        sprintf(buf, "%s", inet_ntoa(intaddr) );
        cJSON_AddStringToObject(root, "ip_addr", buf);

        mac = (unsigned char *)hwaddr.sa_data;
		sprintf(buf,"%02x%02x%02x%02x%02x%02x",mac[0], mac[1],mac[2], mac[3], mac[4], mac[5]);
        cJSON_AddStringToObject(root, "mac_address", buf);
        
        cJSON_AddItemToObject(root, "neighbor_devices", cJSON_CreateArray());

        cJSON *obj_array;
        cJSON_AddItemToObject(root, "station_info", obj_array = cJSON_CreateArray());
        if(obj_array != NULL)
        {
            for(i = 0; i < num; i++)
            {
                if(devinfo[i].on_link == 1)
                {
                    obj_station = creatJSONMeshStationOBJ(&devinfo[i]);
                    cJSON_AddItemToArray(obj_array, obj_station);
                }
            }
        }
        
        cJSON_AddItemToObject(root, "child_devices", cJSON_CreateArray());
       
	}
    else
    {
		ssize_t read;
		size_t  len   = 0;
		char*	line  = NULL;
		read = getline(&line, &len, fp);
		fclose(fp);

        root = cJSON_Parse(line);
        free(line);

        if(root == NULL)
        {
            printf("[%s:%d] cJSON_Parse fail.", __FUNCTION__, __LINE__);
            return -1;
        }

        addExtStationInfoToTopology(root, devinfo);
        cJSON *obj;
        obj = cJSON_GetObjectItem(root, "station_info");
        if(obj != NULL)
        {
            for(i = 0; i < num; i++)
            {
                if(devinfo[i].on_link == 1 && devinfo[i].slave_flg == 0)
                {
                    obj_station = creatJSONMeshStationOBJ(&devinfo[i]);
                    cJSON_AddItemToArray(obj, obj_station);
                }
            }
        }

    }

    return root;
}

void CalcOnlineClientNum(cJSON *obj, int *num)
{
    cJSON *station_array;
    cJSON *head;
    cJSON *pos;

    station_array = cJSON_GetObjectItem(obj, "station_info");
    *num += cJSON_GetArraySize(station_array);

    head = cJSON_GetObjectItem(obj, "child_devices");

    if(head != NULL)
    {
        for(pos = head->child; pos != NULL; pos = pos->next)
        {
            CalcOnlineClientNum(pos, num);
        }
    }
    else
    {
        printf("[%s:%d] can't find child_devices.", __FUNCTION__, __LINE__);
    }

    return;
}
/*
{
       "device_name":"EasyMesh_Device_host",
       "ip_addr":"192.168.1.211",
       "mac_address":"cc2d2110f0dc",
       "neighbor_devices":[
           {
               "neighbor_mac":"00e046614455",
               "neighbor_name":"EasyMesh_Device2",
               "neighbor_rssi":"43",
               "neighbor_band":"TBU"
           }
       ],
       "station_info":[
           {
               "station_mac":"40331a573909",
               "station_rssi":"61",
               "station_connected_band":"5G",
               "station_downlink":"150",
               "station_uplink":"108"
               "station_ip":"192.168.1.10"
               "station_hostname":"mate20"
               "station_brand":"HUAWEI"
               "station_link_time":"68"
           }
       ],
       "child_devices":[
           Object{...},
           Object{...}
       ]
   }
*/
int add_child_node_to_neighbor(P_MESH_DEV_INFO mesh_json, P_MESH_CHILD_INFO child_info)
{
	int i;
	int j;
	P_MESH_MAIN_INFO main_node = &mesh_json->main_info;
	
	for(i=0; i<main_node->neighbor_num; i++)
	{
		if (!strcmp(main_node->neighbor_info[i].mac, child_info->mac))
		{
			if (strlen(child_info->ip)>0)
				strcpy(main_node->neighbor_info[i].ip, child_info->ip);

			if (strlen(main_node->neighbor_info[i].rssi)>0)
			{
				strcpy(child_info->rssi, main_node->neighbor_info[i].rssi);
			}

			for (j = 0; j<child_info->sta_num; j++)
			{
				strcpy(main_node->neighbor_info[i].sta_info[j].ip, child_info->sta_info[j].ip);
				strcpy(main_node->neighbor_info[i].sta_info[j].mac, child_info->sta_info[j].mac);
				strcpy(main_node->neighbor_info[i].sta_info[j].rssi, child_info->sta_info[j].rssi);
				strcpy(main_node->neighbor_info[i].sta_info[j].hostname, child_info->sta_info[j].hostname);
				strcpy(main_node->neighbor_info[i].sta_info[j].brand, child_info->sta_info[j].brand);
				strcpy(main_node->neighbor_info[i].sta_info[j].master, child_info->sta_info[j].master);
			}
		}
	}
	
	return 0;
}

/**/
int mesh_get_neighbor_id(P_MESH_DEV_INFO mesh_json, char *p_mac)
{
	int i;
	if (strlen(p_mac)<0)
		return -1;

	for (i = 0; i<mesh_json->main_info.neighbor_num; i++)
	{
		//printf("[%s][%d][zzk]mesh_json->main_info.neighbor_info[i].mac = %s, p_mac = %s\n", 
		//		__FUNCTION__, __LINE__, mesh_json->main_info.neighbor_info[i].mac, p_mac);
		if (!strcmp(mesh_json->main_info.neighbor_info[i].mac, p_mac))
		{
			return i;
		}
	}
	return -1;
}

/*
	add child's station info to MESH_STA_INFO and MESH_CHILD_INFO's struct mesh_sta_info.
*/
int get_station_info_json(cJSON *mesh_obj, char *master, 
	P_MESH_DEV_INFO mesh_json, P_MESH_CHILD_INFO child_json, P_MESH_NEIGHBOR_INFO p_neighbor_x)
{
	cJSON *arrayItem;
	cJSON *item;
	char buffer[512] = {0};
	int i = 0;
	
	//DTRACE(DTRACE_APPSERVER, "[%s][%d]child_json->mac = %s\n",__FUNCTION__, __LINE__, child_json->mac);

	arrayItem = cJSON_GetObjectItem(mesh_obj, "station_info");  
	if(arrayItem!=NULL)  
	{  
		int size=cJSON_GetArraySize(arrayItem);  
		for(i=0; i<size; i++)  
		{
			cJSON *object;
			//printf("------------------i=%d\n",i);  
			object=cJSON_GetArrayItem(arrayItem,i);  

			item = cJSON_GetObjectItem(object, "station_mac");  
			if(item != NULL)
			{
				memset(buffer, 0, sizeof(buffer));
				if (str_to_mac(item->valuestring, buffer) < 0)
					printf("get mac error!!!\n");
				else 
				{
					strcpy(mesh_json->sta_info[mesh_json->sta_num].mac, buffer);
					strcpy(child_json->sta_info[child_json->sta_num].mac, buffer);
					//strcpy(p_neighbor_x->sta_info[p_neighbor_x->sta_num].mac, buffer);
					//printf("000 child_json->sta_info[child_json->sta_num].mac is %s child_json->sta_num = %d\n",
					//	child_json->sta_info[child_json->sta_num].mac, child_json->sta_num);
				}
			}

			item=cJSON_GetObjectItem(object, "station_rssi");  
			if(item != NULL)
			{
				//printf("station_rssi is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[mesh_json->sta_num].rssi, item->valuestring);
				strcpy(child_json->sta_info[child_json->sta_num].rssi, item->valuestring);
				//strcpy(p_neighbor_x->sta_info[p_neighbor_x->sta_num].rssi, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "station_ip");  
			if(item != NULL)
			{
				//printf("station_ip is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[mesh_json->sta_num].ip, item->valuestring);
				strcpy(child_json->sta_info[child_json->sta_num].ip, item->valuestring);
				//strcpy(p_neighbor_x->sta_info[p_neighbor_x->sta_num].ip, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "station_hostname");  
			if(item != NULL)
			{
				//printf("station_hostname is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[mesh_json->sta_num].hostname, item->valuestring);
				strcpy(child_json->sta_info[child_json->sta_num].hostname, item->valuestring);
				//strcpy(p_neighbor_x->sta_info[p_neighbor_x->sta_num].hostname, item->valuestring);
			}

			if(strlen(master) > 0)
			{
				//printf("master is %s\n", master);
				strcpy(mesh_json->sta_info[mesh_json->sta_num].master, master);
				strcpy(child_json->sta_info[child_json->sta_num].master, master);
			}

			strcpy(&p_neighbor_x->sta_info[p_neighbor_x->sta_num], &mesh_json->sta_info[mesh_json->sta_num]);
			mesh_json->sta_num++;
			child_json->sta_num++;
			p_neighbor_x->sta_num++;
		}

	}

	return 0;
}

int get_mesh_json_sta_info(P_MESH_DEV_INFO mesh_json)
{
    cJSON *mesh_obj;
	int z = 0;
	int i = 0;
	int ret = 0;
	int online_num = 0;
	char buffer[512] = {0};

    mesh_obj = getMeshTopologyJSON();

#if 0
	char jasonData[10240] = "{\"device_name\":\"鏋佽仈wifi\",\"ip_addr\":\"192.168.1.10\",\"mac_address\":\"d05157b22e4b\",\"neighbor_devices\":[{\"neighbor_mac\":\"d05157b22e59\",\"neighbor_name\":\"KS_MESH_B22E58\",\"neighbor_rssi\":\"47\",\"neighbor_band\":\"5G\"},{\"neighbor_mac\":\"d05157b22e52\",\"neighbor_name\":\"KS_MESH_B22E51\",\"neighbor_rssi\":\"54\",\"neighbor_band\":\"5G\"}],\"station_info\":[{\"station_mac\":\"9487e0073cc2\",\"station_rssi\":\"47\",\"station_connected_band\":\"5G\",\"station_downlink\":\"780\",\"station_uplink\":\"780\"},{\"station_mac\":\"c49f4cae3166\",\"station_rssi\":\"61\",\"station_connected_band\":\"2G\",\"station_downlink\":\"150\",\"station_uplink\":\"6\"}],\"child_devices\":[{\"device_name\":\"KS_MESH_B22E58\",\"ip_addr\":\"192.168.1.105\",\"mac_address\":\"d05157b22e59\",\"neighbor_devices\":[],\"station_info\":[{\"station_mac\":\"482ca0bd865b\",\"station_rssi\":\"40\",\"station_connected_band\":\"5G\",\"station_downlink\":\"433\",\"station_uplink\":\"6\",\"station_ip\":\"192.168.1.101\",\"station_hostname\":\"RedmiNote7-hello\",\"station_brand\":\"Xiaomi\",\"station_link_time\":\"null\"},{\"station_mac\":\"9487e0073cc2\",\"station_rssi\":\"60\",\"station_connected_band\":\"5G\",\"station_downlink\":\"866\",\"station_uplink\":\"866\",\"station_ip\":\"192.168.1.100\",\"station_hostname\":\"MI8-xiaomishouji\",\"station_brand\":\"Xiaomi\",\"station_link_time\":\"null\"},{\"station_mac\":\"00e04d6a0b65\",\"station_rssi\":\"100\",\"station_connected_band\":\"ETHERNET\",\"station_downlink\":\"0\",\"station_uplink\":\"0\",\"station_ip\":\"192.168.1.16\",\"station_hostname\":\"---\",\"station_brand\":\"---\",\"station_link_time\":\"null\"}],\"child_devices\":[]},{\"device_name\":\"KS_MESH_B22E51\",\"ip_addr\":\"192.168.1.103\",\"mac_address\":\"d05157b22e52\",\"neighbor_devices\":[],\"station_info\":[{\"station_mac\":\"70ece4dd59b7\",\"station_rssi\":\"63\",\"station_connected_band\":\"5G\",\"station_downlink\":\"292\",\"station_uplink\":\"54\"},{\"station_mac\":\"4c49e3bc9df4\",\"station_rssi\":\"63\",\"station_connected_band\":\"2G\",\"station_downlink\":\"72\",\"station_uplink\":\"6\"}],\"child_devices\":[]}]}";
	mesh_obj = cJSON_Parse(jasonData);
	if (!mesh_obj)
	{
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
		return -1;
	}
#endif
	
	cJSON *tmp;
	cJSON *arrayItem;
	cJSON *item=cJSON_GetObjectItem(mesh_obj, "device_name"); 
	char *device_name = item->valuestring;
	//printf("====> device_name = %s\n", device_name);

	tmp=cJSON_GetObjectItem(mesh_obj, "ip_addr"); 
	if(tmp != NULL)
	{
		char *ip_addr = tmp->valuestring;
		//printf("====> ip_addr = %s\n", ip_addr);
		if (strlen(ip_addr)<=0)
			strcpy(mesh_json->main_info.ip, "0.0.0.0");
		else
			strcpy(mesh_json->main_info.ip, ip_addr);
	}


	tmp=cJSON_GetObjectItem(mesh_obj, "mac_address"); 
	if(tmp != NULL)
	{
		char *mac_address = tmp->valuestring;
		strcpy(mesh_json->main_info.ip, tmp->valuestring);
		if (strlen(mac_address)<=0)
			strcpy(mesh_json->main_info.mac, "00:00:00:00:00:00");
		else
			strcpy(mesh_json->main_info.mac, mac_address);
	}


	/**/
	arrayItem=cJSON_GetObjectItem(mesh_obj, "neighbor_devices");  
	if(arrayItem!=NULL)  
	{  
		int size=cJSON_GetArraySize(arrayItem);  
		//printf("main mesh have neighbor_devices number = %d.\n",size);  

		for(i=0; i<size; i++)  
		{
			cJSON *object;
			object=cJSON_GetArrayItem(arrayItem,i);  

			item = cJSON_GetObjectItem(object, "neighbor_mac");	
			if(item != NULL)
			{
				DTRACE(DTRACE_APPSERVER,"neighbor_mac is %s\n", item->valuestring);
				memset(buffer, 0, sizeof(buffer));
				ret = str_to_mac(item->valuestring, buffer);
				if (ret < 0)
					printf("get mac error!!!\n");
				else 
					strcpy(mesh_json->main_info.neighbor_info[i].mac, buffer);
			}

			item=cJSON_GetObjectItem(object, "neighbor_name");  
			if(item != NULL)
			{
				//printf("neighbor_name is %s\n", item->valuestring);
				strcpy(mesh_json->main_info.neighbor_info[i].name, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "neighbor_rssi");  
			if(item != NULL)
			{
				strcpy(mesh_json->main_info.neighbor_info[i].rssi, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "neighbor_band");  
			if(item != NULL)
			{
				//printf("station_hostname is %s\n", item->valuestring);
				strcpy(mesh_json->main_info.neighbor_info[i].band, item->valuestring);
			}

			mesh_json->main_info.neighbor_num ++;
		}

	}

	/*end*/

	arrayItem=cJSON_GetObjectItem(mesh_obj, "station_info");  
	if(arrayItem!=NULL)  
	{  
		int size=cJSON_GetArraySize(arrayItem);  
		//printf("main mesh have sta number = %d.\n",size);  

		for(i=0; i<size; i++)  
		{
			cJSON *object;
			object=cJSON_GetArrayItem(arrayItem,i);  

			item = cJSON_GetObjectItem(object, "station_mac");  
			if(item != NULL)
			{
				//printf("station_mac is %s\n", item->valuestring);
				memset(buffer, 0, sizeof(buffer));
				ret = str_to_mac(item->valuestring, buffer);
				if (ret < 0)
					printf("get mac error!!!\n");
				else 
				{
					strcpy(mesh_json->sta_info[i].mac, buffer);
					strcpy(mesh_json->main_info.sta_info[i].mac, buffer);
				}
			}

			item=cJSON_GetObjectItem(object, "station_rssi");  
			if(item != NULL)
			{
				//printf("station_rssi is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[i].rssi, item->valuestring);
				strcpy(mesh_json->main_info.sta_info[i].rssi, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "station_ip");  
			if(item != NULL)
			{
				//printf("station_ip is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[i].ip, item->valuestring);
				strcpy(mesh_json->main_info.sta_info[i].ip, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "station_hostname");  
			if(item != NULL)
			{
				//printf("station_hostname is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[i].hostname, item->valuestring);
				strcpy(mesh_json->main_info.sta_info[i].hostname, item->valuestring);
			}

			item=cJSON_GetObjectItem(object, "station_brand"); 
			if(item != NULL)
			{
				//printf("station_hostname is %s\n", item->valuestring);
				strcpy(mesh_json->sta_info[i].brand, item->valuestring);
				strcpy(mesh_json->main_info.sta_info[i].brand, item->valuestring);
			}

			apmib_get(MIB_MAP_DEVICE_NAME, (void *)buffer);
			if(strlen(buffer) > 0)
			{
				//printf("master is %s\n", device_name);
				strcpy(mesh_json->sta_info[i].master, buffer);
			}
			mesh_json->sta_num++;
			mesh_json->main_info.sta_num++;
		}

	}

	/* 主设备的第一个子节点 */
	arrayItem=cJSON_GetObjectItem(mesh_obj, "child_devices");  
	if(arrayItem!=NULL)  
	{ 
		int size = cJSON_GetArraySize(arrayItem);  
		//DTRACE(DTRACE_APPSERVER,"[%s][%d]child mesh has [%d] childs.\n",__FUNCTION__, __LINE__, size); 

		for(i=0; i<size; i++)  
		{
			cJSON *object;
			char *device_name = NULL;
			//printf("------------------i=%d\n",i);  
			object=cJSON_GetArrayItem(arrayItem,i);  

			item = cJSON_GetObjectItem(object, "mac_address");	
			if(item != NULL)
			{
				//DTRACE(DTRACE_APPSERVER, "mac_address is %s\n", item->valuestring);
				memset(buffer, 0, sizeof(buffer));
				ret = str_to_mac(item->valuestring, buffer);
				if (ret < 0)
				{
					printf("[%s][%d]get mac error!!!\n",__FUNCTION__, __LINE__);
				}
				else 
				{
					strcpy(mesh_json->child_info[mesh_json->child_num].mac, buffer);
				}
			}

			item = cJSON_GetObjectItem(object, "ip_addr");	
			if(item != NULL)
			{
				//printf("ip_addr is %s\n", item->valuestring);
				strcpy(mesh_json->child_info[mesh_json->child_num].ip, item->valuestring);
			}

			item = cJSON_GetObjectItem(object, "device_name");	
			if(item != NULL)
			{
				device_name = item->valuestring;
				//printf("device_name is %s\n", device_name);
				strcpy(mesh_json->child_info[mesh_json->child_num].device_name, device_name);
			}
			else
			{
				strcpy(mesh_json->child_info[mesh_json->child_num].device_name, "NULL");
			}

			int idx = mesh_get_neighbor_id(mesh_json, &mesh_json->child_info[mesh_json->child_num].mac);
			P_MESH_NEIGHBOR_INFO p_neighbor_x = &mesh_json->main_info.neighbor_info[idx]; 

			int z = 0;
			z = mesh_json->child_num;
			/* add station to child_info and neighbor */
			get_station_info_json(object, device_name, mesh_json, &mesh_json->child_info[z], p_neighbor_x);
			mesh_json->child_num++;

			add_child_node_to_neighbor(mesh_json, &mesh_json->child_info[z]);

			#if 0	//zzk not finish 
		
			/* 子节点的子节点 */
			item = cJSON_GetObjectItem(object, "child_devices");  
			if(item!=NULL)  
			{
				int j;
				cJSON *child_object;
				child_object = cJSON_GetArrayItem(item, j);  
				for(j=0; j<size; j++)  
				{
					get_station_info_json(child_object, device_name, mesh_json);
				}
			}
			#endif
		}
	}

    CalcOnlineClientNum(mesh_obj, &online_num);
    cJSON_Delete(mesh_obj);	
	//printf("online_num = %d\n", online_num);
	return 0;
}

int get_mac_s_link_time(P_MESH_DEV_INFO mesh_json, APP_WL_INFO_Tp wl_dev_info)
{
	int i = 0;
	int j = 0;

	if (mesh_json->sta_num <1 ||  wl_dev_info->sta_num < 1)
		return -1;

	for(i=0; i<mesh_json->sta_num; i++)
	{
		for(j=0; j<wl_dev_info->sta_num; j++)
		{
			if (!strcasecmp(mesh_json->sta_info[i].mac, wl_dev_info->sta_info[j].addr))
			{
				strcpy(mesh_json->sta_info[i].link_time, wl_dev_info->sta_info[j].link_time);
			}
		}
	}
	for(i=0; i<mesh_json->main_info.sta_num; i++)
	{
		for(j=0; j<wl_dev_info->sta_num; j++)
		{
			if (!strcasecmp(mesh_json->main_info.sta_info[i].mac, wl_dev_info->sta_info[j].addr))
			{
				strcpy(mesh_json->main_info.sta_info[i].link_time, wl_dev_info->sta_info[j].link_time);
			}
		}
	}
	return 0;
}

int get_sta_link_time(P_MESH_DEV_INFO mesh_json, RTK_LAN_DEVICE_INFO_Tp devinfo, int num)
{
	int i = 0;
	int j = 0;
	char mac_b[62] = {0};
	char time_b[62] = {0};
	struct tm* pTM;
	
	if (mesh_json->sta_num <1 ||  num < 1)
		return -1;

	for(i=0; i<mesh_json->sta_num; i++)
	{
		for(j=0; j<num; j++)
		{
			sprintf(mac_b,"%02X:%02X:%02X:%02X:%02X:%02X",devinfo[j].mac[0],devinfo[j].mac[1],devinfo[j].mac[2],devinfo[j].mac[3],devinfo[j].mac[4],devinfo[j].mac[5]);
			if (!strcasecmp(mesh_json->sta_info[i].mac, mac_b) && (devinfo[j].linktime > 0))
			{				
				pTM = localtime(&devinfo[j].linktime);
				sprintf(time_b, "%04d-%02d-%02d %02d:%02d", pTM->tm_year+1900, pTM->tm_mon+1, pTM->tm_mday, pTM->tm_hour, pTM->tm_min);
				strcpy(mesh_json->sta_info[i].link_time, time_b);
			}
		}
	}
	for(i=0; i<mesh_json->main_info.sta_num; i++)
	{
		for(j=0; j<num; j++)
		{

		
			sprintf(mac_b,"%02X:%02X:%02X:%02X:%02X:%02X",devinfo[j].mac[0],devinfo[j].mac[1],devinfo[j].mac[2],devinfo[j].mac[3],devinfo[j].mac[4],devinfo[j].mac[5]);
			if (!strcasecmp(mesh_json->main_info.sta_info[i].mac, mac_b) && (devinfo[j].linktime > 0))
			{
				pTM = localtime(&devinfo[j].linktime);
				sprintf(time_b, "%04d-%02d-%02d %02d:%02d", pTM->tm_year+1900, pTM->tm_mon+1, pTM->tm_mday, pTM->tm_hour, pTM->tm_min);
				strcpy(mesh_json->main_info.sta_info[i].link_time, time_b);
			}
		}
	}

	return 0;
}

int get_child_node_from_mac(P_MESH_DEV_INFO mesh_json, char *pmac, P_MESH_CHILD_INFO pdst_childx)
{
	int i = 0;
	int z = 0;
	if (pmac == NULL)
		return -1;

	for (i = 0; i < mesh_json->child_num; i++)
	{
		if (!strcmp(pmac, mesh_json->child_info[i].mac))
		{
		
			strcpy(pdst_childx->device_name, mesh_json->child_info[i].device_name);
			strcpy(pdst_childx->ip, mesh_json->child_info[i].ip);
			strcpy(pdst_childx->mac, mesh_json->child_info[i].mac);
			strcpy(pdst_childx->rssi, mesh_json->child_info[i].rssi);
			strcpy(pdst_childx->link_time, mesh_json->child_info[i].link_time);
			strcpy(pdst_childx->master, mesh_json->child_info[i].master);
			pdst_childx->sta_num = mesh_json->child_info[i].sta_num;
			for (z=0; z<mesh_json->child_info[i].sta_num; z++)
			{
				strcpy(pdst_childx->sta_info[z].mac, mesh_json->child_info[i].sta_info[z].mac);
				strcpy(pdst_childx->sta_info[z].ip, mesh_json->child_info[i].sta_info[z].ip);
				strcpy(pdst_childx->sta_info[z].rssi, mesh_json->child_info[i].sta_info[z].rssi);
				strcpy(pdst_childx->sta_info[z].hostname, mesh_json->child_info[i].sta_info[z].hostname);
				strcpy(pdst_childx->sta_info[z].link_time, mesh_json->child_info[i].sta_info[z].link_time);
				strcpy(pdst_childx->sta_info[z].master, mesh_json->child_info[i].sta_info[z].master);
			}
			
			return 0;
		}
	}
	return -1;
}

/**/
int get_link_time_from_mac(P_MESH_CHILD_INFO         dst_childx, APP_WL_INFO_Tp wl_dev_info)
{
	int i = 0;
	int j = 0;

	for(i=0; i<dst_childx->sta_num; i++)
	{
		for(j=0; j<wl_dev_info->sta_num; j++)
		{
			if (!strcasecmp(dst_childx->sta_info[i].mac, wl_dev_info->sta_info[j].addr))
			{
				strcpy(dst_childx->sta_info[i].link_time, wl_dev_info->sta_info[j].link_time);
				break;
			}
		}
	}

	return 0;
}


/*
	ChildDevSettings :send the child info to app
	in:	 mesh_json(all mesh node info)
		 pmac(child's mac)
	out: -1(fail); 0(sucess)

*/
int send_child_json(int fd, P_MESH_DEV_INFO mesh_json, char *pchildx_mac)
{
	cJSON *top_root;
	cJSON *parameters;
	cJSON *root;
	char send_conf_buff[20480] = {0};
	char buffer[214] = {0};
	char wanIp[32] = {0};
	char wanMask[32] = {0};
	char wanDefIp[32] = {0};
	char wanAdrr[32] = {0};
	char childx[32] = {0};
	int val = 0;
	int devNum = 0;
	int i = 0;
	int ret = 0;
	APP_WL_INFO_T wl_dev_info;

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	

	if (strlen(pchildx_mac)==0)
	{
		top_root = cJSON_CreateObject();
		cJSON_AddItemToObject(top_root, "ChildDevSettings", root = cJSON_CreateArray());
		cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
	
		memset(buffer, 0, sizeof(buffer));
		cJSON_AddStringToObject(parameters, "childStatus", "on");
		memset(buffer, 0, sizeof(buffer));
		cJSON_AddStringToObject(parameters, "linkQuality", "good");
		memset(buffer, 0, sizeof(buffer));
		apmib_get(MIB_CUSTOMER_HW_SERIAL_NUM, (void *)buffer);
		cJSON_AddStringToObject(parameters, "sn", buffer);
	
		apmib_get(MIB_MAP_DEVICE_NAME, (void *)buffer);
		cJSON_AddStringToObject(parameters, "location", buffer);
		
		apmib_get(MIB_LED_ENABLE, (void *)&val);
		if (1 == val)
			sprintf(buffer, "%s", "on");
		else
			sprintf(buffer, "%s", "off");
		cJSON_AddStringToObject(parameters, "LedEnable", buffer);
		
		getWanInfo(wanIp, wanMask, wanDefIp, wanAdrr);
		
		cJSON_AddStringToObject(parameters, "IP", wanIp);
	
		cJSON_AddStringToObject(parameters, "MAC", wanAdrr);
		
		//cJSON_AddStringToObject(parameters, "removeDev", "off");
	
		//devNum = get_2_5g_dev_info(&wl_dev_info);
	
		sprintf(buffer, "%d", mesh_json->main_info.sta_num);
		cJSON_AddStringToObject(parameters, "devNum", buffer);
		DTRACE(DTRACE_APPSERVER,"[%s][%d]devNum = %s\n", __FUNCTION__, __LINE__, buffer);		
		for(i = 0; i < mesh_json->main_info.sta_num; i++)
		{
			cJSON* array;
			cJSON* arrayobj;
			sprintf(childx, "dev%d",i+1);
			cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
			cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
			cJSON_AddStringToObject(arrayobj, "devModel", mesh_json->main_info.sta_info[i].hostname);
			cJSON_AddStringToObject(arrayobj, "mac", mesh_json->main_info.sta_info[i].mac);
			cJSON_AddStringToObject(arrayobj, "devLinkTime", mesh_json->main_info.sta_info[i].link_time);
			sprintf(buffer, "%dMbps", "100");
			cJSON_AddStringToObject(arrayobj, "devDownSpeed", buffer);
			sprintf(buffer, "-%d", 100 - atoi(mesh_json->main_info.sta_info[i].rssi));
			cJSON_AddStringToObject(arrayobj, "rssi", buffer);
		}
	}
	else
	{
		int x;
		MESH_CHILD_INFO dst_childx;
		memset(&dst_childx, 0, sizeof(dst_childx));
		ret = get_child_node_from_mac(mesh_json, pchildx_mac, &dst_childx);
		if (ret < 0)
			printf("error get_child_node_from_mac\n");

		devNum = get_2_5g_dev_info(&wl_dev_info);
		get_link_time_from_mac(&dst_childx, &wl_dev_info);

		top_root = cJSON_CreateObject();
		cJSON_AddItemToObject(top_root, "ChildDevSettings", root = cJSON_CreateArray());
		cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
		
		memset(buffer, 0, sizeof(buffer));
		cJSON_AddStringToObject(parameters, "childStatus", "on");
		
		memset(buffer, 0, sizeof(buffer));
		DTRACE(DTRACE_APPSERVER,"[%s][%d]neighbor mac %s, rssi %s\n", 
			__FUNCTION__, __LINE__, dst_childx.mac, dst_childx.rssi);
		if (atoi(dst_childx.rssi) > 60)
			cJSON_AddStringToObject(parameters, "linkQuality", "good");
		else if (atoi(dst_childx.rssi) > 30 && atoi(dst_childx.rssi) < 60)
			cJSON_AddStringToObject(parameters, "linkQuality", "better");
		else 
			cJSON_AddStringToObject(parameters, "linkQuality", "poor");
		memset(buffer, 0, sizeof(buffer));
		
		apmib_get(MIB_CUSTOMER_HW_SERIAL_NUM, (void *)buffer);
		cJSON_AddStringToObject(parameters, "sn", buffer);
		
		cJSON_AddStringToObject(parameters, "location", dst_childx.device_name);
		
		apmib_get(MIB_LED_ENABLE, (void *)&val);
		if (1 == val)
			sprintf(buffer, "%s", "on");
		else
			sprintf(buffer, "%s", "off");
		cJSON_AddStringToObject(parameters, "LedEnable", buffer);
		
		getWanInfo(wanIp, wanMask, wanDefIp, wanAdrr);
		
		cJSON_AddStringToObject(parameters, "IP", dst_childx.ip);
		
		cJSON_AddStringToObject(parameters, "MAC", dst_childx.mac);
		
		cJSON_AddStringToObject(parameters, "removeDev", "off");
		
		//devNum = get_2_5g_dev_info(&wl_dev_info);
		sprintf(buffer, "%d", dst_childx.sta_num);
		cJSON_AddStringToObject(parameters, "devNum", buffer);	
		
		for(i = 0; i < dst_childx.sta_num; i++)
		{
			cJSON* array;
			cJSON* arrayobj;
			sprintf(childx, "dev%d", i+1);
			cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
			cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
			cJSON_AddStringToObject(arrayobj, "devModel", dst_childx.sta_info[i].hostname);
			cJSON_AddStringToObject(arrayobj, "mac", dst_childx.sta_info[i].mac);
			cJSON_AddStringToObject(arrayobj, "devLinkTime", dst_childx.sta_info[i].link_time);
			sprintf(buffer, "%sMbps", "100");
			cJSON_AddStringToObject(arrayobj, "devDownSpeed", buffer);
			sprintf(buffer, "-%d", 100 - atoi(dst_childx.sta_info[i].rssi));
			cJSON_AddStringToObject(arrayobj, "rssi", buffer);
		}
	}
	
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return 0;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}

void convertIntToString(int i, char *string)  
{  
   int digits=0,j=0;   
   j=i;  
   for( digits=1;j>10;j/=10) 
   	{
     digits*=10; 
   	}
   for(;digits>0;digits/=10)  
    {  
     *string++='0'+i/digits;  
     i%=digits;  
    }  
   *string='\0';  
}  
int gahGetPeerMacbyIp(char *ipaddr, char* buf, char* localethname) 
{ 
	int     sockfd; 
	unsigned char *ptr; 
	struct arpreq arpreq; 
	struct sockaddr_in *sin; 
	struct sockaddr_storage ss; 
	char addr[INET_ADDRSTRLEN+1];

	memset(addr, 0, INET_ADDRSTRLEN+1); 
	memset(&ss, 0, sizeof(ss));

	sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sockfd == -1) { 
	    perror("socket error"); 
	    return -1; 
	} 
	sin = (struct sockaddr_in *) &ss; 
	sin->sin_family = AF_INET; 
	if (inet_pton(AF_INET, ipaddr, &(sin->sin_addr)) <= 0) { 
	    perror( "inet_pton error"); 
	    return -1; 
	} 
	sin = (struct sockaddr_in *) &arpreq.arp_pa; 
	memcpy(sin, &ss, sizeof(struct sockaddr_in)); 
	strcpy(arpreq.arp_dev, localethname); 
	arpreq.arp_ha.sa_family = AF_UNSPEC; 
	if (ioctl(sockfd, SIOCGARP, &arpreq) < 0) { 
		perror("ioctl SIOCGARP: "); 
		return -1; 
	} 
	ptr = (unsigned char *)arpreq.arp_ha.sa_data; 
	sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5)); 
	return 0; 
}

int refuse_guest_sta(char *p_dst_ip)
{
	int i = 0;
	APP_WL_INFO_T vap_wl_dev;
	char dst_mac[32] = {0};
	
	if (strlen(p_dst_ip)==0)
		return -1;

	gahGetPeerMacbyIp(p_dst_ip, dst_mac, "br0");
	//printf("dst_mac = %s\n", dst_mac);

	/* get guest network stations */	
	get_2_5g_vap_dev_info(&vap_wl_dev);
	for (i=0; i<vap_wl_dev.sta_num; i++)
	{
		//printf("vap_wl_dev.sta_info.addr = %s\n", vap_wl_dev.sta_info[i].addr);
		if (!strcasecmp(vap_wl_dev.sta_info[i].addr, dst_mac))
			return 1;
	}
	return 0;
}

#define _CONFIG_SCRIPT_PROG	"init.sh"
#define _DHCPD_PROG_NAME	"udhcpd"
#define _DHCPD_PID_PATH		"/var/run"
#define _DHCPC_PROG_NAME	"udhcpc"
#define _DHCPC_PID_PATH		"/etc/udhcpc"
#define _PATH_DHCPS_LEASES	"/var/lib/misc/udhcpd.leases"
#define MAX_MSG_BUFFER_SIZE 256

void app_run_init_script(char *arg)
{
#ifdef NO_ACTION
	// do nothing
#else
	int pid=0;
	int i;
	char tmpBuf[MAX_MSG_BUFFER_SIZE]={0};
	
#ifdef REBOOT_CHECK
	if(run_init_script_flag == 1){
#endif

	snprintf(tmpBuf, MAX_MSG_BUFFER_SIZE, "%s/%s.pid", _DHCPD_PID_PATH, _DHCPD_PROG_NAME);
	pid = getPid(tmpBuf);
	if ( pid > 0)
		kill(pid, SIGUSR1);
		
	usleep(1000);
	
	if ( pid > 0){
		system("killall -9 udhcpd 2> /dev/null");
		system("rm -f /var/run/udhcpd.pid 2> /dev/null");
	}

	//Patch: kill some daemons to free some RAM in order to call "init.sh gw all" more quickly
	//which need more tests especially for 8196c 2m/16m
	killSomeDaemon();
	
	system("killsh.sh");	// kill all running script	
#ifdef CONFIG_IPV6
	system("ip tunnel del tun 2> /dev/null");
#ifdef CONFIG_DSLITE_SUPPORT
	system("ip -6 tunnel del ds-lite 2> /dev/null"); //delete ds-lite tunnel
	system("rm -f /var/ds-lite.script 2> /dev/null");
#endif
#ifdef CONFIG_SIXRD_SUPPORT
	system("ip tunnel del tun6rd 2> /dev/null");
#endif
#endif

#ifdef REBOOT_CHECK
	run_init_script_flag = 0;
	needReboot = 0;
#endif
// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
	web_restart_solar();
#endif

		pid = fork();
		if (pid)
		{
			waitpid(pid, NULL, 0);
		}
		else if (pid == 0) {
#ifdef HOME_GATEWAY
			sprintf(tmpBuf, "%s gw %s", _CONFIG_SCRIPT_PROG, arg);
#elif defined(VOIP_SUPPORT) && defined(ATA867x)
			sprintf(tmpBuf, "%s ATA867x %s", _CONFIG_SCRIPT_PROG, arg);
#else
			sprintf(tmpBuf, "%s ap %s", _CONFIG_SCRIPT_PROG, arg);
#endif
			for(i=3; i<sysconf(_SC_OPEN_MAX); i++)
				close(i);
			sleep(1);
			system(tmpBuf);
			exit(1);
		}
#ifdef REBOOT_CHECK
	}
	else
	{
	}
#endif
#endif
}

int get_2_5g_ssid_pwd(char *p_ssid_2g, char *p_pwd_2g, char *p_ssid_5g, char *p_pwd_5g)
{
	int wlanif = 0;
	int ori_wlan_idx = wlan_idx;
	char wlanIfStr[32] = {0};
	char buffer[64] = {0};

	DTRACE(DTRACE_APPSERVER,"[%s-%d]===========\n", __FUNCTION__, __LINE__);

	wlanif = whichWlanIfIs(PHYBAND_5G);
	if(wlanif >= 0)
	{
		memset(wlanIfStr,0x00,sizeof(wlanIfStr));
		sprintf((char *)wlanIfStr, "wlan%d",wlanif);
		if(SetWlan_idx((char *)wlanIfStr))
		{
			memset(buffer, 0, sizeof(buffer));
			if ( !apmib_get( MIB_WLAN_SSID,  (void *)buffer) )
			{
				printf("apmib_get MIB_WLAN_SSID error!\n");
			}
			else
			{
				strcpy(p_ssid_5g, buffer);
			}
		}
		memset(buffer, 0, sizeof(buffer));
		if ( !apmib_get(MIB_WLAN_WPA_PSK,  (void *)buffer) )
		{
			printf("apmib_get MIB_WLAN_WPA_PSK error!\n");
		}
		else
		{
			strcpy(p_pwd_5g, buffer);
		}
		
		wlan_idx = ori_wlan_idx;
	}
	wlanif = whichWlanIfIs(PHYBAND_2G);
	if(wlanif >= 0)
	{
		memset(wlanIfStr,0x00,sizeof(wlanIfStr));
		sprintf((char *)wlanIfStr, "wlan%d",wlanif);
		if(SetWlan_idx((char *)wlanIfStr))
		{
			memset(buffer, 0, sizeof(buffer));
			apmib_get(MIB_WLAN_SSID, (void *)buffer);
			DTRACE(DTRACE_APPSERVER,"2G_ssid ssid = %s\n", buffer);
			if (strlen(buffer)>0)
				strcpy(p_ssid_2g, buffer);
		}
		memset(buffer, 0, sizeof(buffer));
		if ( !apmib_get(MIB_WLAN_WPA_PSK,  (void *)buffer) )
		{
			printf("apmib_get MIB_FIRST_LOGIN error!\n");
		}
		else
		{
			strcpy(p_pwd_2g, buffer);
		}
		DTRACE(DTRACE_APPSERVER,"2G_ssid pwd = %s\n", buffer);
		wlan_idx = ori_wlan_idx;
	}
	return 0;
}
int set_hide_ssid(char *p_hide_ssid)
{
	int wlanif = 0;
	int intVal = 0;
	char wlanIfStr[64] = {0};
	char cmd[128] = {0}; 
	
	int ori_wlan_idx = wlan_idx;

	if (strlen(p_hide_ssid)<=0)
	{
		printf("boardcast ssid is null\n");
		return -1;
	}

	wlanif = whichWlanIfIs(PHYBAND_5G);
	if(wlanif >= 0)
	{
		memset(wlanIfStr,0x00,sizeof(wlanIfStr));
		sprintf((char *)wlanIfStr, "wlan%d",wlanif);
		if(SetWlan_idx((char *)wlanIfStr))
		{
			if (p_hide_ssid != NULL)
			{
				if (!strcmp(p_hide_ssid, "on"))
				{
					intVal = 0;
				}
				else 
				{
					intVal = 1;
				}
				apmib_set(MIB_WLAN_HIDDEN_SSID, (void *)&intVal);
				sprintf(cmd, "iwpriv %s set_mib hiddenAP=%d",wlanIfStr, intVal);
				system(cmd);
			}
			
			wlan_idx = ori_wlan_idx;
		}


	}
	wlanif = whichWlanIfIs(PHYBAND_2G);
	if(wlanif >= 0)
	{
		memset(wlanIfStr,0x00,sizeof(wlanIfStr));
		sprintf((char *)wlanIfStr, "wlan%d",wlanif);
		DTRACE(DTRACE_APPSERVER,"2G_ssid wlanIfStr = %s\n", wlanIfStr);
		if(SetWlan_idx((char *)wlanIfStr))
		{
			if (p_hide_ssid != NULL)
			{
	
				if (!strcmp(p_hide_ssid, "on"))
					intVal = 0;
				else 
					intVal = 1;
				apmib_set(MIB_WLAN_HIDDEN_SSID, (void *)&intVal);
				sprintf(cmd, "iwpriv %s set_mib hiddenAP=%d",wlanIfStr, intVal);
				system(cmd);
			}
			wlan_idx = ori_wlan_idx;
		}
	}

	apmib_update_web(CURRENT_SETTING);
	return 0;
}
unsigned char *gettoken(const unsigned char *str,unsigned int index,unsigned char symbol)
{
	static char tmp[50];
	unsigned char tk[50]; //save symbol index
	char *ptmp;
	int i,j,cnt=1,start,end;
	//scan symbol and save index
	
	memset(tmp, 0x00, sizeof(tmp));
	
	for (i=0;i<strlen((char *)str);i++)
	{          
		if (str[i]==symbol)
		{
			tk[cnt]=i;
			cnt++;
		}
	}
	
	if (index>cnt-1)
	{
		return NULL;
	}
			
	tk[0]=0;
	tk[cnt]=strlen((char *)str);
	
	if (index==0)
		start=0;
	else
		start=tk[index]+1;

	end=tk[index+1];
	
	j=0;
	for(i=start;i<end;i++)
	{
		tmp[j]=str[i];
		j++;
	}
		
	return (unsigned char *)tmp;
}
#define TZ_FILE "/etc/TZ"
void set_timeZone(void)
{
	unsigned int daylight_save = 1;
	char daylight_save_str[5];
	char time_zone[8];
	char command[100], str_datnight[100],str_cmd[128]={0};
	unsigned char *str_tz1;
	
	apmib_get( MIB_DAYLIGHT_SAVE,  (void *)&daylight_save);
	memset(daylight_save_str, 0x00, sizeof(daylight_save_str));
	sprintf(daylight_save_str,"%u",daylight_save);
	
	apmib_get( MIB_NTP_TIMEZONE,  (void *)&time_zone);

	if(daylight_save == 0)
		sprintf( str_datnight, "%s", "");
	else if(strcmp(time_zone,"9 1") == 0)
		sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"8 1") == 0)
		sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"7 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"6 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"6 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"5 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"5 3") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/02:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"4 3") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.2.0/00:00:00,M3.2.0/00:00:00");
	else if(strcmp(time_zone,"3 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.1.0/00:00:00,M10.5.0/00:00:00");
	else if(strcmp(time_zone,"3 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M2.2.0/00:00:00,M10.2.0/00:00:00");
	else if(strcmp(time_zone,"1 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/00:00:00,M10.5.0/01:00:00");
	else if(strcmp(time_zone,"0 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/01:00:00,M10.5.0/02:00:00");
	else if(strcmp(time_zone,"-1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-2 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-2 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/03:00:00,M10.5.0/04:00:00");
	else if(strcmp(time_zone,"-2 3") == 0)
	        sprintf( str_datnight, "%s", "PDT,M4.5.5/00:00:00,M9.5.5/00:00:00");
	else if(strcmp(time_zone,"-2 5") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/03:00:00,M10.5.5/04:00:00");
	else if(strcmp(time_zone,"-2 6") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.5/02:00:00,M10.1.0/02:00:00");
	else if(strcmp(time_zone,"-3 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-4 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/04:00:00,M10.5.0/05:00:00");
	else if(strcmp(time_zone,"-9 4") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.5.0/02:00:00,M4.1.0/03:00:00");
	else if(strcmp(time_zone,"-10 2") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.5.0/02:00:00,M4.1.0/03:00:00");
	else if(strcmp(time_zone,"-10 4") == 0)
	        sprintf( str_datnight, "%s", "PDT,M10.1.0/02:00:00,M4.1.0/03:00:00");
	else if(strcmp(time_zone,"-10 5") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.5.0/02:00:00,M10.5.0/03:00:00");
	else if(strcmp(time_zone,"-12 1") == 0)
	        sprintf( str_datnight, "%s", "PDT,M3.2.0/03:00:00,M10.1.0/02:00:00");
	else
	        sprintf( str_datnight, "%s", "");

	str_tz1 = gettoken((unsigned char*)time_zone, 0, ' ');
	
	if(strcmp(time_zone,"3 1") == 0 ||
		strcmp(time_zone,"-3 4") == 0 ||
	 	strcmp(time_zone,"-4 3") == 0 ||
	 	strcmp(time_zone,"-5 3") == 0 ||
	 	strcmp(time_zone,"-9 4") == 0 ||
	 	strcmp(time_zone,"-9 5") == 0
		)
	{
         sprintf( command, "GMT%s:30%s", str_tz1, str_datnight);
	}
	else
		sprintf( command, "GMT%s%s", str_tz1, str_datnight); 
	unlink(TZ_FILE);
	sprintf(str_cmd,"echo %s >%s",command,TZ_FILE);
	system(str_cmd);
}

