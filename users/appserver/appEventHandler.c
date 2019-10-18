
/*

File         : appEventHandler.c
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
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include<arpa/inet.h>
#include "../boa/apmib/apmib.h"
//#include "../boa/src/utility.h"
#include "trace.h"
#include "appEventHandler.h"
#include "appUpload.h"


static int status_code[5];
static SS_STATUS_Tp pStatus=NULL;

extern unsigned char *fwVersion;

char WLAN_IF[32] = "wlan0";
static const char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

WLAN_RATE_T rate_11n_table_20M_LONG[]={
	{MCS0, 	"6.5"},
	{MCS1, 	"13"},
	{MCS2, 	"19.5"},
	{MCS3, 	"26"},
	{MCS4, 	"39"},
	{MCS5, 	"52"},
	{MCS6, 	"58.5"},
	{MCS7, 	"65"},
	{MCS8, 	"13"},
	{MCS9, 	"26"},
	{MCS10, 	"39"},
	{MCS11, 	"52"},
	{MCS12, 	"78"},
	{MCS13, 	"104"},
	{MCS14, 	"117"},
	{MCS15, 	"130"},
	{0}
};
WLAN_RATE_T rate_11n_table_20M_SHORT[]={
	{MCS0, 	"7.2"},
	{MCS1, 	"14.4"},
	{MCS2, 	"21.7"},
	{MCS3, 	"28.9"},
	{MCS4, 	"43.3"},
	{MCS5, 	"57.8"},
	{MCS6, 	"65"},
	{MCS7, 	"72.2"},
	{MCS8, 	"14.4"},
	{MCS9, 	"28.9"},
	{MCS10, 	"43.3"},
	{MCS11, 	"57.8"},
	{MCS12, 	"86.7"},
	{MCS13, 	"115.6"},
	{MCS14, 	"130"},
	{MCS15, 	"144.5"},
	{0}
};
WLAN_RATE_T rate_11n_table_40M_LONG[]={
	{MCS0, 	"13.5"},
	{MCS1, 	"27"},
	{MCS2, 	"40.5"},
	{MCS3, 	"54"},
	{MCS4, 	"81"},
	{MCS5, 	"108"},
	{MCS6, 	"121.5"},
	{MCS7, 	"135"},
	{MCS8, 	"27"},
	{MCS9, 	"54"},
	{MCS10, 	"81"},
	{MCS11, 	"108"},
	{MCS12, 	"162"},
	{MCS13, 	"216"},
	{MCS14, 	"243"},
	{MCS15, 	"270"},
	{0}
};
WLAN_RATE_T rate_11n_table_40M_SHORT[]={
	{MCS0, 	"15"},
	{MCS1, 	"30"},
	{MCS2, 	"45"},
	{MCS3, 	"60"},
	{MCS4, 	"90"},
	{MCS5, 	"120"},
	{MCS6, 	"135"},
	{MCS7, 	"150"},
	{MCS8, 	"30"},
	{MCS9, 	"60"},
	{MCS10, 	"90"},
	{MCS11, 	"120"},
	{MCS12, 	"180"},
	{MCS13, 	"240"},
	{MCS14, 	"270"},
	{MCS15, 	"300"},
	{0}
};

static struct ifstatRate wanRate;

int get_2_5g_dev_num()
{
	char *buff = NULL;
	int devNum = 0;
	int i = 0;
	int found=0;	
	WLAN_STA_INFO_Tp pInfo;

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		printf("Allocate buffer failed!\n");
		return 0;
	}
	/* 获取5G */
	if ( getWlStaInfo("wlan0",	(WLAN_STA_INFO_Tp)buff ) < 0 ) {
		printf("Read wlan0 sta info failed!\n");
	}
	else
	{
		for (i=1; i<=MAX_STA_NUM; i++) 
		{
			pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
			if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) 
			{
		
				found++;
			}
		}
	}
	free(buff);
	DTRACE(DTRACE_APPSERVER, "found = %d\n",found);//the number of 2.4g 
	
	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		printf("Allocate buffer failed!\n");
		return 0;
	}

	/* 获取2.4G */
	if ( getWlStaInfo("wlan1",	(WLAN_STA_INFO_Tp)buff ) < 0 ) {
		printf("Read wlan1 sta info failed!\n");
	}
	else
	{
		for (i=1; i<=MAX_STA_NUM; i++) 
		{
			pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
			if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) 
			{			
				found++;
			}
		}
	}
	free(buff);
	DTRACE(DTRACE_APPSERVER, "found = %d\n", found);

	return found;
}


/*
	return dev num;
*/
int get_2_5g_dev_info(APP_WL_INFO_Tp p_wl_dev)
{
	char *buff = NULL;
	int devNum = 0;
	int i = 0;
	int found=0;	
	
	int rateid=0;
	char txrate[20] = {0};
	char buffer[32] = {0};
	WLAN_STA_INFO_Tp pInfo;

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		p_wl_dev->sta_num = 0;
		printf("Allocate buffer failed!\n");
		return 0;
	}
	/* 获取5G */
	if ( getWlStaInfo("wlan0",	(WLAN_STA_INFO_Tp)buff ) < 0 ) {
		printf("Read wlan sta info failed!\n");
		p_wl_dev->sta_num = 0;
		return 0;
	}

	for (i=1; i<=MAX_STA_NUM; i++) 
	{
		pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
		if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) 
		{
		
			if(pInfo->txOperaRates >= 0xA0) {
				//sprintf(txrate, "%d", pInfo->acTxOperaRate); 
				set_11ac_txrate(pInfo, txrate);
			} else if((pInfo->txOperaRates & 0x80) != 0x80){	
				if(pInfo->txOperaRates%2){
					sprintf(txrate, "%d%s",pInfo->txOperaRates/2, ".5"); 
				}else{
					sprintf(txrate, "%d",pInfo->txOperaRates/2); 
				}
			}else{
				if((pInfo->ht_info & 0x1)==0){ //20M
					if((pInfo->ht_info & 0x2)==0){//long
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}else if((pInfo->ht_info & 0x1)==0x1){//40M
					if((pInfo->ht_info & 0x2)==0){//long
						
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_40M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16; rateid++){
							if(rate_11n_table_40M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}
				
			}	
			found++;
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", 
				pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5]);
			strcpy(p_wl_dev->sta_info[i-1].addr, buffer);
			memset(buffer, 0, sizeof(buffer));
			get_link_time(pInfo->link_time, buffer);
			strcpy(p_wl_dev->sta_info[i-1].link_time, buffer);			
			sprintf(buffer, "%sMbps", txrate);
			p_wl_dev->sta_info[i-1].tx_rate = txrate;
			sprintf(buffer, "%d", pInfo->rssi);
			p_wl_dev->sta_info[i-1].rssi = pInfo->rssi;
		}
	}
	free(buff);
	DTRACE(DTRACE_APPSERVER, "5g found = %d\n",found);
	
	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		printf("Allocate buffer failed!\n");
		p_wl_dev->sta_num = found;
		return found;
	}
	DTRACE(DTRACE_APPSERVER, "========== get 2.4G sta ========\n");

	/* 获取2.4G */
	if ( getWlStaInfo("wlan1",	(WLAN_STA_INFO_Tp)buff ) < 0 ) {
		printf("Read wlan sta info failed!\n");
		p_wl_dev->sta_num = found;
		return found;
	}

	for (i=1; i<=MAX_STA_NUM; i++) 
	{
		pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
		if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) 
		{
		
			if(pInfo->txOperaRates >= 0xA0) {
				//sprintf(txrate, "%d", pInfo->acTxOperaRate); 
				set_11ac_txrate(pInfo, txrate);
			} else if((pInfo->txOperaRates & 0x80) != 0x80){	
				if(pInfo->txOperaRates%2){
					sprintf(txrate, "%d%s",pInfo->txOperaRates/2, ".5"); 
				}else{
					sprintf(txrate, "%d",pInfo->txOperaRates/2); 
				}
			}else{
				if((pInfo->ht_info & 0x1)==0){ //20M
					if((pInfo->ht_info & 0x2)==0){//long
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}else if((pInfo->ht_info & 0x1)==0x1){//40M
					if((pInfo->ht_info & 0x2)==0){//long
						
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_40M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16; rateid++){
							if(rate_11n_table_40M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}
				
			}	
			sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", 
				pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5]);
			strcpy(p_wl_dev->sta_info[i+found-1].addr, buffer);
			memset(buffer, 0, sizeof(buffer));
			get_link_time(pInfo->link_time, buffer);
			strcpy(p_wl_dev->sta_info[i+found-1].link_time, buffer);
			sprintf(buffer, "%sMbps", txrate);
			p_wl_dev->sta_info[i+found-1].tx_rate = txrate;
			sprintf(buffer, "%d", pInfo->rssi);
			p_wl_dev->sta_info[i+found-1].rssi = pInfo->rssi;
			found++;
		}
	}
	free(buff);
	DTRACE(DTRACE_APPSERVER, "5g + 2.4g found = %d\n", found);
	p_wl_dev->sta_num = found;

	return found;
}
/*
	return dev num;
*/
int get_2_5g_vap_dev_info(APP_WL_INFO_Tp p_wl_dev)
{
	char *buff = NULL;
	int devNum = 0;
	int i = 0;
	int found=0;	
	
	int rateid=0;
	char txrate[20] = {0};
	char buffer[32] = {0};
	WLAN_STA_INFO_Tp pInfo;

	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		p_wl_dev->sta_num = 0;
		printf("Allocate buffer failed!\n");
		return 0;
	}
	/* 获取5G */
	if ( getWlStaInfo("wlan0-va1",	(WLAN_STA_INFO_Tp)buff ) < 0 ) {
		printf("Read wlan sta info failed!\n");
		p_wl_dev->sta_num = 0;
		return 0;
	}

	for (i=1; i<=MAX_STA_NUM; i++) 
	{
		pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
		if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) 
		{
		
			if(pInfo->txOperaRates >= 0xA0) {
				//sprintf(txrate, "%d", pInfo->acTxOperaRate); 
				set_11ac_txrate(pInfo, txrate);
			} else if((pInfo->txOperaRates & 0x80) != 0x80){	
				if(pInfo->txOperaRates%2){
					sprintf(txrate, "%d%s",pInfo->txOperaRates/2, ".5"); 
				}else{
					sprintf(txrate, "%d",pInfo->txOperaRates/2); 
				}
			}else{
				if((pInfo->ht_info & 0x1)==0){ //20M
					if((pInfo->ht_info & 0x2)==0){//long
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}else if((pInfo->ht_info & 0x1)==0x1){//40M
					if((pInfo->ht_info & 0x2)==0){//long
						
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_40M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16; rateid++){
							if(rate_11n_table_40M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}
				
			}	
			found++;
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", 
				pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5]);
			strcpy(p_wl_dev->sta_info[i-1].addr, buffer);
			memset(buffer, 0, sizeof(buffer));
			get_link_time(pInfo->link_time, buffer);
			strcpy(p_wl_dev->sta_info[i-1].link_time, buffer);			
			sprintf(buffer, "%sMbps", txrate);
			p_wl_dev->sta_info[i-1].tx_rate = txrate;
			sprintf(buffer, "%d", pInfo->rssi);
			p_wl_dev->sta_info[i-1].rssi = pInfo->rssi;
		}
	}
	free(buff);
	DTRACE(DTRACE_APPSERVER, "5g found = %d\n",found);
	
	buff = calloc(1, sizeof(WLAN_STA_INFO_T) * (MAX_STA_NUM+1));
	if ( buff == 0 ) {
		printf("Allocate buffer failed!\n");
		p_wl_dev->sta_num = found;
		return found;
	}
	DTRACE(DTRACE_APPSERVER, "========== get 2.4G sta ========\n");

	/* 获取2.4G */
	if ( getWlStaInfo("wlan1-va1",	(WLAN_STA_INFO_Tp)buff ) < 0 ) {
		printf("Read wlan sta info failed!\n");
		p_wl_dev->sta_num = found;
		return found;
	}

	for (i=1; i<=MAX_STA_NUM; i++) 
	{
		pInfo = (WLAN_STA_INFO_Tp)&buff[i*sizeof(WLAN_STA_INFO_T)];
		if (pInfo->aid && (pInfo->flag & STA_INFO_FLAG_ASOC)) 
		{
		
			if(pInfo->txOperaRates >= 0xA0) {
				//sprintf(txrate, "%d", pInfo->acTxOperaRate); 
				set_11ac_txrate(pInfo, txrate);
			} else if((pInfo->txOperaRates & 0x80) != 0x80){	
				if(pInfo->txOperaRates%2){
					sprintf(txrate, "%d%s",pInfo->txOperaRates/2, ".5"); 
				}else{
					sprintf(txrate, "%d",pInfo->txOperaRates/2); 
				}
			}else{
				if((pInfo->ht_info & 0x1)==0){ //20M
					if((pInfo->ht_info & 0x2)==0){//long
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_20M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_20M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}else if((pInfo->ht_info & 0x1)==0x1){//40M
					if((pInfo->ht_info & 0x2)==0){//long
						
						for(rateid=0; rateid<16;rateid++){
							if(rate_11n_table_40M_LONG[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_LONG[rateid].rate);
								break;
							}
						}
					}else if((pInfo->ht_info & 0x2)==0x2){//short
						for(rateid=0; rateid<16; rateid++){
							if(rate_11n_table_40M_SHORT[rateid].id == pInfo->txOperaRates){
								sprintf(txrate, "%s", rate_11n_table_40M_SHORT[rateid].rate);
								break;
							}
						}
					}
				}
				
			}	
			sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", 
				pInfo->addr[0],pInfo->addr[1],pInfo->addr[2],pInfo->addr[3],pInfo->addr[4],pInfo->addr[5]);
			strcpy(p_wl_dev->sta_info[i+found-1].addr, buffer);
			memset(buffer, 0, sizeof(buffer));
			get_link_time(pInfo->link_time, buffer);
			strcpy(p_wl_dev->sta_info[i+found-1].link_time, buffer);
			sprintf(buffer, "%sMbps", txrate);
			p_wl_dev->sta_info[i+found-1].tx_rate = txrate;
			sprintf(buffer, "%d", pInfo->rssi);
			p_wl_dev->sta_info[i+found-1].rssi = pInfo->rssi;
			found++;
		}
	}
	free(buff);
	DTRACE(DTRACE_APPSERVER, "Vap 5g + 2.4g found = %d\n", found);
	p_wl_dev->sta_num = found;

	return found;
}


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

#if 0
////////////////
int getWlSiteSurveyResult(char *interface, SS_STATUS_Tp pStatus )
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
   wrq.u.data.pointer = (caddr_t)pStatus;

   if ( pStatus->number == 0 )
	   wrq.u.data.length = sizeof(SS_STATUS_T);
   else
	   wrq.u.data.length = sizeof(pStatus->number);

   if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSDB, &wrq) < 0){
	   close( skfd );
   return -1;
   }
   close( skfd );
#else
   return -1 ;
#endif

   return 0;
}
int getWlBssInfo(char *interface, RTK_BSS_INFOp pInfo)
{
#ifndef NO_ACTION
	int skfd=0;
	struct iwreq wrq;

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;
	/* Get wireless name */
	if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0)
	/* If no wireless name : no wireless extensions */
	{
		close( skfd );
		return -1;
	}

	wrq.u.data.pointer = (caddr_t)pInfo;
	wrq.u.data.length = sizeof(RTK_BSS_INFO);

	if (iw_get_ext(skfd, interface, SIOCGIWRTLGETBSSINFO, &wrq) < 0)
	{
		close( skfd );
		return -1;
	}
	close( skfd );
#else
	memset(pInfo, 0, sizeof(RTK_BSS_INFO)); 
#endif

	return 0;
}
int translateChar(char*input,char* output,int bufLen)
{
	int i=0,j=0;
	bzero(output,bufLen);
	for(i=0;i<bufLen;i++)
	{
		switch(input[i])
		{
			case '\0':
				output[j]='\0';
				return 0;
			case '<':
				if(j+5>=bufLen)
					return -1;
				strcpy(output+j,"&lt;");
				j+=strlen("&lt;");
				break;
			case '>':
				if(j+5>=bufLen)
					return -1;
				strcpy(output+j,"&gt;");
				j+=strlen("&gt;");
				break;
			case '"':
				if(j+7>=bufLen)
					return -1;
				strcpy(output+j,"&quot;");
				j+=strlen("&quot;");
				break;
			case 39:
				if(j+6>=bufLen)
					return -1;
				strcpy(output+j,"&#39;");
				j+=strlen("&#39;");
				break;				
			default:
				if(j+2>=bufLen) return -1;
				output[j++]=input[i];
				break;
		}
	}
	return -1;
}
#endif

/////////////////////////////////////////////////////////////////////////////
int app_getWlSiteSurveyRequest(char *interface, int *pStatus)
{
#ifndef NO_ACTION
    int skfd=0;
    struct iwreq wrq;
    unsigned char result;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd==-1)
		return -1;

    /* Get wireless name */
    if ( iw_get_ext(skfd, interface, SIOCGIWNAME, &wrq) < 0){
      /* If no wireless name : no wireless extensions */
      close( skfd );
        return -1;
	}
    wrq.u.data.pointer = (caddr_t)&result;
    wrq.u.data.length = sizeof(result);

    if (iw_get_ext(skfd, interface, SIOCGIWRTLSCANREQ, &wrq) < 0){
    	//close( skfd );
	//return -1;
	}
    close( skfd );

    if ( result == 0xff )
    	*pStatus = -1;
    else
	*pStatus = (int) result;
#else
	*pStatus = -1;
#endif
#ifdef CONFIG_RTK_MESH 
	// ==== modified by GANTOE for site survey 2008/12/26 ==== 
	return (int)*(char*)wrq.u.data.pointer; 
#else
	return 0;
#endif
}

int scanning_environment()
{
	int wait_time = 0;
	int status;
	char *strVal = NULL;
	unsigned char res;
	
	DTRACE(DTRACE_APPSERVER, "enter scannig env....................\n");
	// issue scan request
	wait_time = 0;
	while (1) {
		strVal = "wlan0";
		if(strVal[0])
		{
			sprintf(WLAN_IF, "%s", strVal);				
		}
		 
		// ==== modified by GANTOE for site survey 2008/12/26 ==== 
		switch(app_getWlSiteSurveyRequest(WLAN_IF, &status)) 
		{ 
			case -2: 
				printf("Auto scan running!!\n"); 
				break; 
			case -1: 
				printf("Site-survey request failed!\n"); 
				break; 
			default: 
				break; 
		} 

		if (status != 0) {	// not ready
			if (wait_time++ > 15) 
			{
				printf("wlsurvey_scan_request_timout\n");
				return -1;
			}
#ifdef	CONFIG_RTK_MESH
			// ==== modified by GANTOE for site survey 2008/12/26 ==== 
			usleep(1000000 + (rand() % 2000000));
#else
			sleep(1);
#endif
		}
		else
			break;
	}

	// wait until scan completely
	wait_time = 0;
	while (1) {
	res = 1;	// only request request status
	if ( getWlSiteSurveyResult(WLAN_IF, (SS_STATUS_Tp)&res) < 0 ) {
		printf("wlsurvey_read_site_error\n");
		free(pStatus);
		pStatus = NULL;
		return -1;
	}
	if (res == 0xff) {	 // in progress
#if (defined(CONFIG_RTL_92D_SUPPORT) && defined (CONFIG_POCKET_ROUTER_SUPPORT)) || defined(CONFIG_RTL_DFS_SUPPORT)
		/*prolong wait time due to scan both 2.4G and 5G */
		if (wait_time++ > 20) 
#else
		if (wait_time++ > 20) 
#endif		
		{
			printf("<script>dw(wlsurvey_scan_timeout)</script>");
			free(pStatus);
			pStatus = NULL;
			return -1;
		}
		sleep(1);
	}
	else
		break;
	}

	DTRACE(DTRACE_APPSERVER, "scanning wlan1...........\n");
	// issue scan request
	wait_time = 0;
	while (1) {
		strVal = "wlan1";
		if(strVal[0])
		{
			sprintf(WLAN_IF, "%s", strVal); 			
		}
		 
		// ==== modified by GANTOE for site survey 2008/12/26 ==== 
		switch(app_getWlSiteSurveyRequest(WLAN_IF, &status)) 
		{ 
			case -2: 
				printf("Auto scan running!!\n"); 
				break; 
			case -1: 
				printf("Site-survey request failed!\n"); 
				break; 
			default: 
				break; 
		} 

		if (status != 0) {	// not ready
			if (wait_time++ > 15) 
			{
				DTRACE(DTRACE_APPSERVER, "wlsurvey_scan_request_timout\n");
				return -1;
			}
#ifdef	CONFIG_RTK_MESH
			// ==== modified by GANTOE for site survey 2008/12/26 ==== 
			usleep(1000000 + (rand() % 2000000));
#else
			sleep(1);
#endif
		}
		else
			break;
	}

	// wait until scan completely
	wait_time = 0;
	while (1) {
	res = 1;	// only request request status
	if ( getWlSiteSurveyResult(WLAN_IF, (SS_STATUS_Tp)&res) < 0 ) {
		DTRACE(DTRACE_APPSERVER, "wlsurvey_read_site_error\n");
		free(pStatus);
		pStatus = NULL;
		return -1;
	}
	if (res == 0xff) {	 // in progress
#if (defined(CONFIG_RTL_92D_SUPPORT) && defined (CONFIG_POCKET_ROUTER_SUPPORT)) || defined(CONFIG_RTL_DFS_SUPPORT)
		/*prolong wait time due to scan both 2.4G and 5G */
		if (wait_time++ > 20) 
#else
		if (wait_time++ > 20) 
#endif		
		{
			printf("wlsurvey_scan_timeout\n");
			free(pStatus);
			pStatus = NULL;
			return -1;
		}
		sleep(1);
	}
	else
		break;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int get_wl_environment_tbl(APP_ENV_INFO_T *p_env_sta_info)
{
	int nBytesSent=0, i;
	int num_2g  = 0;
#ifdef CONFIG_RTK_MESH 
	int mesh_enable = 0; 
	char meshidbuf[40];
#endif 
	BssDscr *pBss;
	char tmpBuf[MAX_MSG_BUFFER_SIZE], ssidbuf[40], tmp1Buf[10], tmp2Buf[20], wpa_tkip_aes[20],wpa2_tkip_aes[20], tmp3Buf[128];
	WLAN_MODE_T mode;
	bss_info bss;
	char *p_if = "wlan0"; 
	
	DTRACE(DTRACE_APPSERVER, "enter wlsiteservertbl 5G\n");

	if (pStatus==NULL) {
		pStatus = calloc(1, sizeof(SS_STATUS_T));
		if ( pStatus == NULL ) {
			printf("Allocate buffer failed!\n");
			return 0;
		}
	}

	pStatus->number = 0; // request BSS DB

	DTRACE(DTRACE_APPSERVER, "WLAN_IF = %s\n", p_if);

	if ( getWlSiteSurveyResult(p_if, pStatus) < 0 ) {
		//ERR_MSG("Read site-survey status failed!");
		printf("Read site-survey status failed!\n");
		free(pStatus); //sc_yang
		pStatus = NULL;
		return 0;
	}

	if ( !apmib_get( MIB_WLAN_MODE, (void *)&mode) ) {
		printf("Get MIB_WLAN_MODE MIB failed!\n");
		return 0;
	}
#ifdef CONFIG_RTK_MESH
	if(mode == AP_MESH_MODE || mode == MESH_MODE) {
		if( !apmib_get( MIB_WLAN_MESH_ENABLE, (void *)&mesh_enable) ) {
			printf("mesh_enable = %d error\n", mesh_enable);
			return 0;
		}
	}
#endif
	if ( getWlBssInfo(p_if, &bss) < 0) {
		printf("Get bssinfo failed!\n");
		return 0;
	}
	int rptEnabled=0;
#if defined(CONFIG_SMART_REPEATER)
	int opmode=0;
	apmib_get(MIB_OP_MODE, (void *)&opmode);
#endif
	DTRACE(DTRACE_APPSERVER, "wlan_idx = %d\n", wlan_idx);
	if(wlan_idx == 0)
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
	else
		apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);

	//printf(" ===> pStatus->number = [%d]\n", pStatus->number);
	
	if ( pStatus->number < 1 )
	{
		p_env_sta_info->sta_num = 0;
		return 0;
	}
	else 
	{
		p_env_sta_info->sta_num =  num_2g  = pStatus->number;
	}
	
	for (i=0; i<pStatus->number && pStatus->number!=0xff; i++) 
	{
		pBss = &pStatus->bssdb[i];
		snprintf(tmpBuf, 200, ("%02x:%02x:%02x:%02x:%02x:%02x"),
		pBss->bdBssId[0], pBss->bdBssId[1], pBss->bdBssId[2],
		pBss->bdBssId[3], pBss->bdBssId[4], pBss->bdBssId[5]);
		strcpy(p_env_sta_info->sta_info[i].addr, tmpBuf);
		
		memcpy(ssidbuf, pBss->bdSsIdBuf, pBss->bdSsId.Length);
		ssidbuf[pBss->bdSsId.Length] = '\0';
		translateChar(ssidbuf,tmp3Buf,sizeof(tmp3Buf));
		strcpy(p_env_sta_info->sta_info[i].ssid, ssidbuf);
		
		p_env_sta_info->sta_info[i].rssi = pBss->rssi;

		p_env_sta_info->sta_info[i].channel = pBss->ChannelNumber;

		p_env_sta_info->sta_info[i].bdBrates = 433;

	}
	
	if (pStatus != NULL)
		free(pStatus); //sc_yang
	pStatus = NULL;

	//printf("======> enter wlsiteservertbl 2.4G\n");

	if (pStatus==NULL) {
		pStatus = calloc(1, sizeof(SS_STATUS_T));
		if ( pStatus == NULL ) {
			printf("Allocate buffer failed!\n");
			return 0;
		}
	}

	pStatus->number = 0; // request BSS DB
	p_if = "wlan1";
	//printf("WLAN_IF = %s\n", p_if);

	if ( getWlSiteSurveyResult(p_if, pStatus) < 0 ) {
		//ERR_MSG("Read site-survey status failed!");
		printf("Read site-survey status failed!\n");
		free(pStatus); //sc_yang
		pStatus = NULL;
		return 0;
	}

	if ( !apmib_get( MIB_WLAN_MODE, (void *)&mode) ) {
		printf("Get MIB_WLAN_MODE MIB failed!\n");
		return 0;
	}
#ifdef CONFIG_RTK_MESH
	if(mode == AP_MESH_MODE || mode == MESH_MODE) {
		if( !apmib_get( MIB_WLAN_MESH_ENABLE, (void *)&mesh_enable) ) {
			printf("mesh_enable = %d error\n", mesh_enable);
			return 0;
		}
	}
#endif
	if ( getWlBssInfo(p_if, &bss) < 0) {
		printf("Get bssinfo failed!\n");
		return 0;
	}
#if defined(CONFIG_SMART_REPEATER)
	apmib_get(MIB_OP_MODE, (void *)&opmode);
#endif
	//printf("wlan_idx = %d\n", wlan_idx);
	if(wlan_idx == 0)
		apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
	else
		apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);

	DTRACE(DTRACE_APPSERVER, "pStatus->number = [%d]\n", pStatus->number);
	
	if ( pStatus->number < 1 )
	{
		p_env_sta_info->sta_num = num_2g;
		return 0;
	}
	else 
		p_env_sta_info->sta_num = pStatus->number + num_2g;
	
	for (i=0; i<pStatus->number && pStatus->number!=0xff; i++) 
	{
		pBss = &pStatus->bssdb[i];
		snprintf(tmpBuf, 200, ("%02x:%02x:%02x:%02x:%02x:%02x"),
		pBss->bdBssId[0], pBss->bdBssId[1], pBss->bdBssId[2],
		pBss->bdBssId[3], pBss->bdBssId[4], pBss->bdBssId[5]);
		strcpy(p_env_sta_info->sta_info[num_2g+i].addr, tmpBuf);
		
		memcpy(ssidbuf, pBss->bdSsIdBuf, pBss->bdSsId.Length);
		ssidbuf[pBss->bdSsId.Length] = '\0';
		translateChar(ssidbuf,tmp3Buf,sizeof(tmp3Buf));
		strcpy(p_env_sta_info->sta_info[num_2g+i].ssid, ssidbuf);
		
		p_env_sta_info->sta_info[num_2g+i].rssi = pBss->rssi;

		p_env_sta_info->sta_info[num_2g+i].channel = pBss->ChannelNumber;

		p_env_sta_info->sta_info[num_2g+i].bdBrates = 72;
		
	}
	
	if (pStatus != NULL)
		free(pStatus); //sc_yang
	pStatus = NULL;

	
	return 0;
}

#if 0
int getWlStaInfo( char *interface,  RTK_WLAN_STA_INFO_Tp pInfo )
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
    wrq.u.data.length = sizeof(RTK_WLAN_STA_INFO_T) * (MAX_STA_NUM+1);
    memset(pInfo, 0, sizeof(RTK_WLAN_STA_INFO_T) * (MAX_STA_NUM+1));

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
#endif
void set_11ac_txrate(WLAN_STA_INFO_Tp pInfo,char* txrate)
{
	char channelWidth=0;//20M 0,40M 1,80M 2
	char shortGi=0;
	char rate_idx=pInfo->txOperaRates-0xA0;
	if(!txrate)return;
/*
	TX_USE_40M_MODE		= BIT(0),
	TX_USE_SHORT_GI		= BIT(1),
	TX_USE_80M_MODE		= BIT(2)
*/
	if(pInfo->ht_info & 0x4)
		channelWidth=2;
	else if(pInfo->ht_info & 0x1)
		channelWidth=1;
	else
		channelWidth=0;
	if(pInfo->ht_info & 0x2)
		shortGi=1;

	sprintf(txrate, "%d", VHT_MCS_DATA_RATE[channelWidth][shortGi][rate_idx]>>1);
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
short whichWlanIfIs(PHYBAND_TYPE_T phyBand)
{
	int i;
	int ori_wlan_idx=wlan_idx;
	int ori_vwlan_idx= vwlan_idx;
	int ret=-1;
	
	for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
	{
		unsigned char wlanif[10];
		memset(wlanif,0x00,sizeof(wlanif));
		sprintf((char *)wlanif, "wlan%d",i);
		if(SetWlan_idx((char *)wlanif))
		{
			int phyBandSelect;
			apmib_get(MIB_WLAN_PHY_BAND_SELECT, (void *)&phyBandSelect);
			if(phyBandSelect == phyBand)
			{
				ret = i;
				break;			
			}
		}						
	}
	
	wlan_idx=ori_wlan_idx;
	vwlan_idx=ori_vwlan_idx;
	return ret;		
}
#endif

/*
 *convert integer type to string format,string pointer point to the final strings 
 *once the function has been completely executed
*/
void int_to_str(int i, char *string)  
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


/*
 *	 BASE64 encodings ,Encode a buffer from "string" into "outbuf"
 */
void encode64_passwd(char *outbuf, char *string, int outlen)
{
	unsigned long	shiftbuf;
	char			*cp, *op;
	int				x, i, j, shift;

	op = outbuf;
	*op = '\0';
	cp = string;
	while (*cp) {
/*
 *		Take three characters and create a 24 bit number in shiftbuf
 */
		shiftbuf = 0;
		for (j = 2; j >= 0 && *cp; j--, cp++) {
			shiftbuf |= ((*cp & 0xff) << (j * 8));
		}
/*
 *		Now convert shiftbuf to 4 base64 letters.  The i,j magic calculates
 *		how many letters need to be output.
 */
		shift = 18;
		for (i = ++j; i < 4 && op < &outbuf[outlen] ; i++) {
			x = (shiftbuf >> shift) & 0x3f;
			*op++ = base64_table[(shiftbuf >> shift) & 0x3f];
			shift -= 6;
		}
/*
 *		Pad at the end with '='
 */
		while (j-- > 0) {
			*op++ = '=';
		}
		*op = '\0';
	}
}

/*
*Function:
*Find and get the string in source string,
*
*/       
static int str_parse(char *source, char *begin, char *end, char *target)
{
	int n = strlen(begin);
	char *first, *second;
	first = strstr(source, begin);
	if (first != NULL)
	{
		second = strstr(first+n, end);
		if (second != NULL)
		{
			memcpy(target, first + n, second - first - n);
		}
	}
   return 0;
}

/*
	check the target strings from app client, return the index flag if get the target string
*/
int get_app_request_type(const char* sourceStr)
{
	char *targetStr = sourceStr;

	//str_parse(sourceStr,"\"", "\"", targetStr);
	
	DTRACE(DTRACE_APPSERVER, "[appserver][%s-%d]source targetStr = %s strlen(targetStr) = %d\n",
		__FUNCTION__, __LINE__, targetStr, strlen(targetStr));
	/* 无线配置 */
	if(!strncmp(targetStr,"RequestWirelessSettings",strlen("RequestWirelessSettings"))) return  WIRELESS_SETTINGS_REQ;
	/* 主页数据 */
	if(!strncmp(targetStr,"RequestHomeSettings",strlen("RequestHomeSettings"))) return  HOME_SETTINGS_REQ;
	/* 连接设备 */
	if(!strncmp(targetStr,"RequestLinkDevSettings",strlen("RequestLinkDevSettings"))) return  LINK_DEVICE_SETTINGS_REQ;
	/* 子节点 */
	if(!strncmp(targetStr,"RequestChildDevSettings",strlen("RequestChildDevSettings"))) return	CHILD_DEVICE_SETTINGS_REQ;
	/* 设置 */
	if(!strncmp(targetStr,"RequestOptionSettings",strlen("RequestOptionSettings"))) return	OPTION_SETTINGS_REQ;	
	/* 访客网络 */
	if(!strncmp(targetStr,"RequestGuestNetworkSettings",strlen("RequestGuestNetworkSettings"))) return  GUEST_NETWORK_SETTINGS_REQ;	
	/* 家长控制 */			  
	if(!strncmp(targetStr,"RequestParentalSettings",strlen("RequestParentalSettings"))) return  PARENTAL_SETTINGS_REQ;	
	/* 上网设置 */
	if(!strncmp(targetStr,"RequestNetworkSettings",strlen("RequestNetworkSettings"))) return  NETWORK_SETTINGS_REQ;
	/* MAC过滤 */
	if(!strncmp(targetStr,"RequestMacFilterSettings",strlen("RequestMacFilterSettings"))) return  MAC_FILTER_REQ;
	/* ip过滤 */
	if(!strncmp(targetStr,"RequestIpFilterSettings",strlen("RequestIpFilterSettings"))) return  IP_FILTER_REQ;
	/* url过滤 */
	if(!strncmp(targetStr,"RequestUrlFilterSettings",strlen("RequestUrlFilterSettings"))) return  URL_FILTER_REQ;
	/* 更新 */
	if(!strncmp(targetStr,"RequestUpgradeSettings",strlen("RequestUpgradeSettings"))) return  UPGRADE_REQ;
	/* 周围环境 */
	if(!strncmp(targetStr,"RequestEnvironmentSettings",strlen("RequestEnvironmentSettings"))) return  ENVIRONMENT_REQ;
	/* 系统设置 */
	if(!strncmp(targetStr,"RequestSystemSettings",strlen("RequestSystemSettings"))) return  SYSTEM_SETTINGS_REQ;
	/* QOS */
	if(!strncmp(targetStr,"RequestQosSettings",strlen("RequestQosSettings"))) return  QOS_SETTINGS_REQ;
	/* 登陆设置 */
	if(!strncmp(targetStr,"RequestLoginSettings",strlen("RequestLoginSettings"))) return  LOGIN_SETTINGS_REQ;
	/* 时区设置 */
	if(!strncmp(targetStr,"RequestTimeZoneSettings",strlen("RequestTimeZoneSettings"))) return  TIME_ZONE_SETTINGS_REQ;
	/* 登录密码设置 */
	if(!strncmp(targetStr,"RequestAppPwdSettings",strlen("RequestAppPwdSettings"))) return  APP_PWD_SETTINGS_REQ;
	/* 语言设置 */
	if(!strncmp(targetStr,"RequestLanguageSettings",strlen("RequestLanguageSettings"))) return  LANG_SETTINGS_REQ;

	return CONFIGURE_ROUTER_EVENT;
}


/*
*check the target strings from app client, return the index flag if get the target string
*/
int get_target_string_flag(const char* sourceStr)
{
	char targetStr[64];
	memset(&targetStr, 0x00, sizeof(targetStr));

	/*App client request settings of router*/
	str_parse(sourceStr,"=", "&", targetStr);
	printf("[%s-%d]source targetStr = %s\n", __FUNCTION__, __LINE__, targetStr);

	/*App client configure router*/
	return CONFIGURE_ROUTER_EVENT;
}


/*
* parse data from cJSON struct  to jason string format,after call this function
* jason string will be saved in send_buff
*/
int generate_jason_strings(char send_buff[],cJSON *jason_struct)
{
	char *out;
	char *p_tmp = NULL;
	char *data_head,*data_tail;
	int count = 0;
	char http_head[512];
	int http_head_len = 0;
	int j=0;
	int json_len=0;
	
	j += sprintf(http_head+j, "HTTP/1.1 200 OK\r\n");
	j += sprintf(http_head+j, "Content-type: text/html\r\n");
	j += sprintf(http_head+j, "Expires: Thu, 15 Dec 2016 12:00:00 GMT\r\n");
	j += sprintf(http_head+j, "Connection: close\r\n");
	j += sprintf(http_head+j, "Pragma: no-cache\r\n");

	p_tmp = out = cJSON_Print(jason_struct);

	json_len = strlen(out);

	//printf("-------- out1 = >>%s<<\n-------- strlen(out) = %d\n", out, strlen(out) );
	cJSON_Delete(jason_struct);	
	data_head = strchr(out,'{');
	count = 0;
	while(out!=NULL)
	{
		data_tail = strchr(out,'}');
		if(data_tail != NULL)
			count++;
		out = ++data_tail;
		if(2==count)
		break;
	}

	//sprintf(http_head+j, "Content-Length: %d\r\n\r\n",data_tail-data_head);
	sprintf(http_head+j, "Content-Length: %d\r\n\r\n", json_len);
	http_head_len = sprintf(send_buff,http_head);
	sprintf(send_buff+http_head_len,data_head);
	
	DTRACE(DTRACE_APPSERVER, "[%s-%d]send_buff = %s\n",__FUNCTION__, __LINE__, send_buff);
	
	if( p_tmp != NULL )
		free(p_tmp);
	
	return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
static char *get_name(char *name, char *p)
{
    while (isspace(*p))
	p++;
    while (*p) {
	if (isspace(*p))
	    break;
	if (*p == ':') {	/* could be an alias */
	    char *dot = p, *dotname = name;
	    *name++ = *p++;
	    while (isdigit(*p))
		*name++ = *p++;
	    if (*p != ':') {	/* it wasn't, backup */
		p = dot;
		name = dotname;
	    }
	    if (*p == '\0')
		return NULL;
	    p++;
	    break;
	}
	*name++ = *p++;
    }
    *name++ = '\0';
    return p;
}
/* type define */

////////////////////////////////////////////////////////////////////////////////
static int get_dev_fields(int type, char *bp, struct user_net_device_stats *pStats)
{
    switch (type) {
    case 3:
	sscanf(bp,
	"%Lu %Lu %lu %lu %lu %lu %lu %lu %Lu %Lu %lu %lu %lu %lu %lu %lu",
	       &pStats->rx_bytes,
	       &pStats->rx_packets,
	       &pStats->rx_errors,
	       &pStats->rx_dropped,
	       &pStats->rx_fifo_errors,
	       &pStats->rx_frame_errors,
	       &pStats->rx_compressed,
	       &pStats->rx_multicast,

	       &pStats->tx_bytes,
	       &pStats->tx_packets,
	       &pStats->tx_errors,
	       &pStats->tx_dropped,
	       &pStats->tx_fifo_errors,
	       &pStats->collisions,
	       &pStats->tx_carrier_errors,
	       &pStats->tx_compressed);
	break;

    case 2:
	sscanf(bp, "%Lu %Lu %lu %lu %lu %lu %Lu %Lu %lu %lu %lu %lu %lu",
	       &pStats->rx_bytes,
	       &pStats->rx_packets,
	       &pStats->rx_errors,
	       &pStats->rx_dropped,
	       &pStats->rx_fifo_errors,
	       &pStats->rx_frame_errors,

	       &pStats->tx_bytes,
	       &pStats->tx_packets,
	       &pStats->tx_errors,
	       &pStats->tx_dropped,
	       &pStats->tx_fifo_errors,
	       &pStats->collisions,
	       &pStats->tx_carrier_errors);
	pStats->rx_multicast = 0;
	break;

    case 1:
	sscanf(bp, "%Lu %lu %lu %lu %lu %Lu %lu %lu %lu %lu %lu",
	       &pStats->rx_packets,
	       &pStats->rx_errors,
	       &pStats->rx_dropped,
	       &pStats->rx_fifo_errors,
	       &pStats->rx_frame_errors,

	       &pStats->tx_packets,
	       &pStats->tx_errors,
	       &pStats->tx_dropped,
	       &pStats->tx_fifo_errors,
	       &pStats->collisions,
	       &pStats->tx_carrier_errors);
	pStats->rx_bytes = 0;
	pStats->tx_bytes = 0;
	pStats->rx_multicast = 0;
	break;
    }
    return 0;
}

static int getStats(char *interface, struct user_net_device_stats *pStats)
{
 	FILE *fh;
  	char buf[512];
	int type;

	fh = fopen(_PATH_PROCNET_DEV, "r");
	if (!fh) {
		printf("Warning: cannot open %s\n",_PATH_PROCNET_DEV);
		return -1;
	}
	fgets(buf, sizeof buf, fh);	/* eat line */
	fgets(buf, sizeof buf, fh);

  	if (strstr(buf, "compressed"))
		type = 3;
	else if (strstr(buf, "bytes"))
		type = 2;
	else
		type = 1;

	while (fgets(buf, sizeof buf, fh)) {
		char *s, name[40];
		s = get_name(name, buf);
		if ( strcmp(interface, name))
			continue;
		get_dev_fields(type, s, pStats);
		fclose(fh);
		return 0;
    	}
	fclose(fh);
	return -1;
}

void killSomeDaemon(void)
{
	system("killall -9 sleep 2> /dev/null");
       system("killall -9 routed 2> /dev/null");
//	system("killall -9 pppoe 2> /dev/null");
//	system("killall -9 pppd 2> /dev/null");
//	system("killall -9 pptp 2> /dev/null");
	system("killall -9 dnrd 2> /dev/null");
	system("killall -9 ntpclient 2> /dev/null");
//	system("killall -9 miniigd 2> /dev/null");	//comment for miniigd iptables rule recovery
	system("killall -9 lld2d 2> /dev/null");
//	system("killall -9 l2tpd 2> /dev/null");	
//	system("killall -9 udhcpc 2> /dev/null");	
//	system("killall -9 udhcpd 2> /dev/null");	
	system("killall -9 reload 2> /dev/null");		
	system("killall -9 iapp 2> /dev/null");	
	system("killall -9 wscd 2> /dev/null");
	system("killall -9 mini_upnpd 2> /dev/null");
	system("killall -9 iwcontrol 2> /dev/null");
	system("killall -9 auth 2> /dev/null");
	system("killall -9 disc_server 2> /dev/null");
	system("killall -9 igmpproxy 2> /dev/null");
	system("echo 1,1 > /proc/br_mCastFastFwd");
	system("killall -9 syslogd 2> /dev/null");
	system("killall -9 klogd 2> /dev/null");
	
	system("killall -9 ppp_inet 2> /dev/null");
#ifdef WLAN_HS2_CONFIG	
	system("killall -9 hs2 2> /dev/null");	
#endif
#ifdef CONFIG_IPV6
	system("killall -9 dhcp6c 2> /dev/null");
	system("killall -9 dhcp6s 2> /dev/null");
	system("killall -9 radvd 2> /dev/null");
	system("killall -9 ecmh 2> /dev/null");
	//kill mldproxy
	system("killall -9 mldproxy 2> /dev/null");
#endif
#ifdef CONFIG_SNMP
	system("killall -9 snmpd 2> /dev/null");
	system("rm -f /var/run/snmpd.pid");
#endif
}

///////////////////////////////////////////////////////////
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

void run_init_script(char *arg)
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

#ifdef RTK_MESH_CONFIG
	#define RTK_CAPWAP_UI_CONFIG_FILE		"/tmp/capwap_config"
	FILE *file = NULL;
	char cmd[100];
	int capwapMode, sleep_count=0, val=1;
	apmib_get(MIB_CAPWAP_MODE, (void *)&capwapMode);
	if(capwapMode & CAPWAP_AUTO_CONFIG_ENABLE)
	{
		sprintf(cmd, "echo 1 > %s", RTK_CAPWAP_UI_CONFIG_FILE);
		system(cmd);
		pid = find_pid_by_name("WTP");	
		if(pid > 0){
			kill(pid, SIGUSR1);
			printf("Send SIGUSR1 signal to WTP\n");

			while(val && sleep_count<=5)
			{
				sleep(1);
				sleep_count++;			
				file = fopen(RTK_CAPWAP_UI_CONFIG_FILE, "r");
				char tmpbuf[10] = {0};
				if(file){
					fgets(tmpbuf,10,file);
					val = atoi(tmpbuf);
					fclose(file);
				}
			}
		}		
		else
			printf("WTP cannot be found...\n");
		printf("<%s>%d: capwapMode=%d count=%d\n",__FUNCTION__,__LINE__,capwapMode,sleep_count);
	}
#endif

#ifdef RTK_USB3G
	system("killall -9 mnet 2> /dev/null");
	system("killall -9 hub-ctrl 2> /dev/null");
	system("killall -9 usb_modeswitch 2> /dev/null");
    system("killall -9 ppp_inet 2> /dev/null");
    system("killall -9 pppd 2> /dev/null");
    system("rm /etc/ppp/connectfile >/dev/null 2>&1");
#endif /* #ifdef RTK_USB3G */

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	Stop_Domain_Query_Process();
	Reset_Domain_Query_Setting();
#endif

#if defined(CONFIG_RTL_ULINKER)
{
	extern int kill_ppp(void);
	int wan_mode, op_mode;

	apmib_get(MIB_OP_MODE,(void *)&op_mode);
	apmib_get(MIB_WAN_DHCP,(void *)&wan_mode);
	if(wan_mode == PPPOE && op_mode == GATEWAY_MODE)
		kill_ppp();
	
	stop_dhcpc();
	stop_dhcpd();
	clean_auto_dhcp_flag();
	disable_bridge_dhcp_filter();
}
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
/*	
       	if (pid)
               	waitpid(pid, NULL, 0);
   	else 
*/ 
	if (pid == 0) {
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



int set_system_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  SystemSettings  \n");
	cJSON * tmp;
	char cmd[32] = "";
	char childX[32] = "";
	char send_conf_buff[5120] = "";
	int i = 0;	
	int x = 0;
	
	int iCount = cJSON_GetArraySize(jason_obj);  

	for (; i < iCount; ++i) 
	{
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "reboot");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *reboot = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "reboot = %s\n", reboot);
		if (!strcmp("on", reboot))
		{
			send_return_code(fd, APP_SETTING_SUC);
			sleep(1);
			system("reboot");
		}

		tmp = cJSON_GetObjectItem(pItem, "reset");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *reset = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "reset = %s\n", reset);
		if (!strcmp("on", reset))
		{
			DTRACE(DTRACE_APPSERVER,"reset is on\n");			
			send_return_code(fd, APP_SETTING_SUC);
			apmib_updateDef();
			sleep(1);
			system("reboot");
			return EXIT_SUCCESS;
		}

		tmp = cJSON_GetObjectItem(pItem, "timerRestartEnable");	
		if (tmp == NULL)
		{
			printf("upgrade error\n");
			goto retError;
		}
		char *timerRestartEnable = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timerRestartEnable = %s\n", timerRestartEnable);
		apmib_set(MIB_DEV_RESTART_ENABLE, (void *) timerRestartEnable);

		tmp = cJSON_GetObjectItem(pItem, "time");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *time = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"time = %s\n", time);
		apmib_set(MIB_DEV_RESTART_TIME, (void *) time);

	}

	apmib_update_web(CURRENT_SETTING);
	
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_SUCCESS;

}

int app_upgrade()
{
	
	return 0;
}
int set_upgrade_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  UpgradeSettings  \n");
	cJSON * tmp;
	cJSON * childItem;
	char cmd[32] = {0};
	char childX[32] = {0};
	char buffer[256] = {0};
	char send_conf_buff[5120] = {0};
	int i = 0;	
	int x = 0;
	int ret = 0;
	#define LOCATION_FILE_PATH "/tmp/fw.bin"
	
	int iCount = cJSON_GetArraySize(jason_obj);  

	for (; i < iCount; ++i) 
	{
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "mainDevName");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *mainDevName = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "mainDevName = %s\n", mainDevName);

		tmp = cJSON_GetObjectItem(pItem, "upgrade");	
		if (tmp == NULL)
		{
			printf("upgrade error\n");
			goto retError;;
		}
		char *upgrade = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "upgrade = %s\n", upgrade);

		if (!strcmp(upgrade, "on"))
		{
			tmp = cJSON_GetObjectItem(pItem, "url");  
			if (tmp == NULL)
			{
				DTRACE(DTRACE_APPSERVER, "url is null\n");
				goto retError;
			}
			char *url = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "main dev upgrade url = %s\n", url);
			remoteChekUpgrade();
			ret = app_performUpgrade();
			if (1 == ret)
			{
				send_return_code(fd, APP_SETTING_SUC);
				prepareFirmware(LOCATION_FILE_PATH);
				return EXIT_SUCCESS;
			}
			else {
				printf("upgrade error !!!! \n");
				goto retError;
			}

		}
		tmp = cJSON_GetObjectItem(pItem, "childNum");  
		if (tmp == NULL)
		{
			printf("childNum error\n");
			goto retError;
		}
		char *childNum = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"childNum = %s atoi(groupNum) = %d\n", childNum, atoi(childNum));

		for (x = 0; x < atoi(childNum); x++) 
		{  
			sprintf(childX, "child%d", x+1);
			DTRACE(DTRACE_APPSERVER, "childX = %s, strlen(childX) = %d\n", childX, strlen(childX));
			childItem = cJSON_GetObjectItem(pItem, childX);  
			if (childItem == NULL) continue;
			
			cJSON* pChildItem = cJSON_GetArrayItem(childItem, 0);  
			if (NULL == pChildItem)
			{  
			
				DTRACE(DTRACE_APPSERVER, "childX = %s continue\n", childX);
				continue;  
			}  
			tmp = cJSON_GetObjectItem(pChildItem, "childName");  
			if (tmp == NULL)
			{
				printf("groupName is null\n");
				goto retError;
			}
			char *childName = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "childName = %s\n", childName);
			
			tmp = cJSON_GetObjectItem(pChildItem, "upgrade");  
			if (tmp == NULL)
			{
				printf("groupTime is null\n");
				goto retError;
			}
			char *upgrade = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "upgrade = %s\n", upgrade);

			if (!strcmp(upgrade, "on"))
			{
				tmp = cJSON_GetObjectItem(pChildItem, "url");  
				if (tmp == NULL)
				{
					DTRACE(DTRACE_APPSERVER, "url is null\n");
					goto retError;
				}
				char *url = tmp->valuestring;
				DTRACE(DTRACE_APPSERVER, "child dev upgrade url = %s\n", url);
				remoteChekUpgrade();
				ret = app_performUpgrade();
				if (1 == ret)
				{
					send_return_code(fd, APP_CHILD_UPGRADE);
					sprintf(buffer, "rm %s", LOCATION_FILE_PATH);
					system(buffer);
					return EXIT_SUCCESS;
				}
				else 
					goto retError;
			}
			
		}
	}
retSucess:

	//request *wp;
	//char * path;
	//char * query;
	//formUpload(wp, path, query);


	//app_upgrade();
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;
	
retError:
	sprintf(buffer, "rm %s", LOCATION_FILE_PATH);
	system(buffer);
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;	
}

int set_urlfilter_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  set_urlfilter_conf  \n");
	cJSON * tmp;
	cJSON * childItem;
	char send_conf_buff[5120] = {0};
	int i = 0;	
	int intVal = 0;
	int entryNum = 0;
	int pid;
	char tmpBuf[128] = {0};
	URLFILTER_T urlEntry, urlEntrytmp;
	void *pEntry;
	int j = 0;
	
	int iCount = cJSON_GetArraySize(jason_obj);  

	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "enable");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *enable = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "enable = %s\n", enable);
		if (!strcmp(enable, "on"))
			intVal = 1;
		else
			intVal = 0;
		
		if ( apmib_set(MIB_URLFILTER_ENABLED, (void *)&intVal) == 0) {
			printf("Set url filter enabled flag error!\n");
			goto retError;
		}
		if (intVal == 0)
		{
			printf("Set url filter close!\n");
			goto setOk_filter;
		}
		
		intVal = 0;
		apmib_set(MIB_URLFILTER_MODE, (void *)&intVal);//set backlist
		
		tmp = cJSON_GetObjectItem(pItem, "method");	//add:新增,delete:删除
		if (tmp == NULL)
		{
			DTRACE(DTRACE_APPSERVER, "method is null!");
			send_return_code(fd, APP_SETTING_SUC);
			return EXIT_SUCCESS;
		}
		char *method = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "method = %s\n", method);

		tmp = cJSON_GetObjectItem(pItem, "url");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *url = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "url = %s\n", url);	
		
		if ( !url[0] ) {
			printf("Error! No url keyword to set.\n");
			goto retError;
		}
		else
		{
			for(j=0; url[j]!='\0'; j++)
				url[j] = tolower(url[j]);
			
			strcpy((char *)urlEntry.urlAddr, url);
			urlEntry.ruleMode = 0;
		}

		if ( !strcmp(method, "add") )
		{	//add same url rule check
			apmib_get(MIB_URLFILTER_TBL_NUM, (void *)&entryNum);
			for(j=1; j<=entryNum; j++)
			{
				memset(&urlEntrytmp, 0x00, sizeof(urlEntrytmp));
				*((char *)&urlEntrytmp) = (char)j;
				if ( apmib_get(MIB_URLFILTER_TBL, (void *)&urlEntrytmp))
				{
					if(urlEntry.ruleMode == urlEntrytmp.ruleMode)
					if (strlen(urlEntry.urlAddr) == strlen(urlEntrytmp.urlAddr))
					{
						if (!memcmp(urlEntrytmp.urlAddr, urlEntry.urlAddr, strlen(urlEntry.urlAddr)))
						{
							DTRACE(DTRACE_APPSERVER, "the url is on table ,not set again !!!\n");
							send_return_code(fd, APP_SETTING_SUC);
							return EXIT_SUCCESS;
						}
					}
				}
			}
			apmib_set(MIB_URLFILTER_DEL, (void*)&urlEntry);
			apmib_set(MIB_URLFILTER_ADD, (void*)&urlEntry);
		}
		else if ( !strcmp(method, "delete") )
		{
			DTRACE(DTRACE_APPSERVER, "MIB_IPFILTER_DEL pEntry.urlAddr = %s!!!\n", urlEntry.urlAddr);
			DTRACE(DTRACE_APPSERVER, "MIB_IPFILTER_DEL pEntry.ruleMode = %d!!!\n", urlEntry.ruleMode);
			apmib_set(MIB_URLFILTER_DEL, (void*)&urlEntry);

			/* Delete entry */
			if ( !apmib_get(MIB_URLFILTER_TBL_NUM, (void *)&entryNum)) {
				DTRACE(DTRACE_APPSERVER, "Get entry number error!\n");
				goto retError;
			}
			for (i=entryNum; i>0; i--) {
				*((char *)&urlEntrytmp) = (char)i;

				if ( apmib_get(MIB_URLFILTER_TBL, (void *)&urlEntrytmp))
				{
					if(urlEntry.ruleMode == urlEntrytmp.ruleMode)
					if (strlen(urlEntry.urlAddr) == strlen(urlEntrytmp.urlAddr))
					{
						if (!memcmp(urlEntrytmp.urlAddr, urlEntry.urlAddr, strlen(urlEntry.urlAddr)))
						{
							if ( !apmib_set(MIB_URLFILTER_DEL, (void *)&urlEntrytmp)) {
								DTRACE(DTRACE_APPSERVER, "Delete table entry error!\n");
								apmib_update(CURRENT_SETTING);
								goto retError;
							}
						}
					}
				}
			}
		}		
	}
	
setOk_filter:

	apmib_update_web(CURRENT_SETTING);
			
	send_return_code(fd, APP_SETTING_SUC);
#if !defined(MULTI_WAN_SUPPORT)
	#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
	#endif
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;


}

int set_ipfilter_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  MacFilterSettings  \n");
	cJSON * tmp;
	cJSON * childItem;
	char tmpBuf[128] = {0};
	IPFILTER_T ipEntry, ipentrytmp;
	int i = 0;	
	int intVal = 0;
	int entryNum = 0;
	int iCount = cJSON_GetArraySize(jason_obj);  
	char send_conf_buff[5120] = {0};
	int pid;
	

	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "enable");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *enable = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"enable = %s\n", enable);
		if (!strcmp(enable, "on"))
			intVal = 1;
		else
			intVal = 0;
		if ( apmib_set(MIB_IPFILTER_ENABLED, (void *)&intVal) == 0) {
			goto retError;
		}

		tmp = cJSON_GetObjectItem(pItem, "method");	//add:新增,delete:删除
		if (tmp == NULL)
		{
			send_return_code(fd, APP_SETTING_SUC);
			return EXIT_SUCCESS;
		}
		char *method = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "method = %s\n", method);

		tmp = cJSON_GetObjectItem(pItem, "ip");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *ip = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER," ip = %s\n", ip);
		inet_aton(ip, (struct in_addr *)&ipEntry.ipAddr);

		//string_to_hex(mac, macEntry.macAddr, 12)

		tmp = cJSON_GetObjectItem(pItem, "comment");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *comment = tmp->valuestring;		

		DTRACE(DTRACE_APPSERVER," comment = %s\n", comment);
		if (ip != NULL){
			strcpy((char *)ipEntry.comment, comment);
			DTRACE(DTRACE_APPSERVER,"macEntry.comment = %s !!!\n", ipEntry.comment);
		}
		
		apmib_get(MIB_IPFILTER_TBL_NUM, (void *)&entryNum);
		if ( !strcmp(method, "add") )
		{
			DTRACE(DTRACE_APPSERVER,"MIB_MACFILTER_ADD !!!\n");
			ipEntry.protoType = PROTO_BOTH;
			int j = 0;

			for(j=1; j<=entryNum; j++)
			{
				memset(&ipentrytmp, 0x00, sizeof(ipentrytmp));
				*((char *)&ipentrytmp) = (char)j;
				if ( apmib_get(MIB_IPFILTER_TBL, (void *)&ipentrytmp))
				{
					if ((*((unsigned int*)ipentrytmp.ipAddr)) == (*((unsigned int*)ipEntry.ipAddr)))
					{
						goto setOk_filter;
					}
				}
			}
			apmib_set(MIB_IPFILTER_ADD, (void*)&ipEntry);
		}
		else if ( !strcmp(method, "delete") )
		{
			DTRACE(DTRACE_APPSERVER,"MIB_IPFILTER_DEL pEntry.comment = %s!!!\n", ipEntry.comment);
			ipEntry.protoType = PROTO_BOTH;
			int j = 0;

			for(j=1; j<=entryNum; j++)
			{
				memset(&ipentrytmp, 0x00, sizeof(ipentrytmp));
				*((char *)&ipentrytmp) = (char)j;
				if ( apmib_get(MIB_IPFILTER_TBL, (void *)&ipentrytmp))
				{
					if ((*((unsigned int*)ipentrytmp.ipAddr)) == (*((unsigned int*)ipEntry.ipAddr)))
					{
						DTRACE(DTRACE_APPSERVER,"MIB_IPFILTER_DEL ipentrytmp.comment = %s!!!\n", ipentrytmp.comment);
						apmib_set(MIB_IPFILTER_DEL, (void*)&ipentrytmp);
						goto setOk_filter;
					}
				}
			}
		}
	}  

setOk_filter:
	apmib_update_web(CURRENT_SETTING);
	send_return_code(fd, APP_SETTING_SUC);
#if !defined(MULTI_WAN_SUPPORT)
	#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
	#endif
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_macfilter_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  MacFilterSettings  \n");
	cJSON * tmp;
	cJSON * childItem;
	MACFILTER_T macEntry, macEntrytmp;
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int intVal = 0;
	int pid;
	char tmpBuf[128] = {0};
	char send_conf_buff[5120] = {0};
	
	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "enable");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *enable = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"enable = %s\n", enable);
		if (!strcmp(enable, "on"))
		{
			intVal = 1;
		}
		else
		{
			intVal = 0;
		}
		
		apmib_set(MIB_MACFILTER_ENABLED, (void *)&intVal);


		tmp = cJSON_GetObjectItem(pItem, "method");	//add:新增,delete:删除
		if (tmp == NULL)
		{
			send_return_code(fd, APP_SETTING_SUC);
			return EXIT_SUCCESS;
		}
		char *method = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"method = %s\n", method);

		tmp = cJSON_GetObjectItem(pItem, "mac");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *mac = tmp->valuestring;
		char strMac[32];
		DTRACE(DTRACE_APPSERVER,"mac = %s strlen(mac) = %d\n", mac, strlen(mac));
		sscanf(mac, "%2X:%2X:%2X:%2X:%2X:%2X",
		&macEntry.macAddr[0], &macEntry.macAddr[1], 
		&macEntry.macAddr[2], &macEntry.macAddr[3],
		&macEntry.macAddr[4], &macEntry.macAddr[5]);

		DTRACE(DTRACE_APPSERVER,"mac == %02x:%02x:%02x:%02x:%02x:%02x\n",
		macEntry.macAddr[0], macEntry.macAddr[1], 
		macEntry.macAddr[2], macEntry.macAddr[3],
		macEntry.macAddr[4], macEntry.macAddr[5]);

		//string_to_hex(mac, macEntry.macAddr, 12)

		tmp = cJSON_GetObjectItem(pItem, "comment");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *comment = tmp->valuestring;		

		DTRACE(DTRACE_APPSERVER," comment = %s\n", comment);
		if (mac != NULL){
			strcpy((char *)macEntry.comment, comment);
			printf("macEntry.comment = %s !!!\n", macEntry.comment);
		}

		if ( !strcmp(method, "add") )
		{
			DTRACE(DTRACE_APPSERVER,"MIB_MACFILTER_ADD !!!\n");

			int entryNum = 0;
			int j = 0;
			apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&entryNum);
			DTRACE(DTRACE_APPSERVER," [%s-%d]entryNum = %d\n", __FUNCTION__, __LINE__, entryNum);
			for(j=1;j<=entryNum;j++)
			{
				memset(&macEntrytmp, 0x00, sizeof(macEntrytmp));
				*((char *)&macEntrytmp) = (char)j;
				if ( apmib_get(MIB_MACFILTER_TBL, (void *)&macEntrytmp))
				{
					DTRACE(DTRACE_APPSERVER,"[%s-%d]entryNum = %d MAC0 %02x:%02x:%02x:%02x:%02x:%02x\n",
						__FUNCTION__, __LINE__, macEntry.macAddr[0], macEntry.macAddr[1], macEntry.macAddr[2], macEntry.macAddr[3], macEntry.macAddr[4], macEntry.macAddr[5]);
					if (!memcmp(macEntrytmp.macAddr, macEntry.macAddr, 6))
					{
						printf("the mac is on table ,not set again !!!\n");
						send_return_code(fd, APP_SETTING_SUC);
						return EXIT_SUCCESS;
					}
						
				}
			}
			apmib_set(MIB_MACFILTER_DEL, (void *)&macEntry);
			apmib_set(MIB_MACFILTER_ADD, (void *)&macEntry);

		}
		else if ( !strcmp(method, "delete") )
		{
			DTRACE(DTRACE_APPSERVER,"MIB_MACFILTER_DEL !!!\n");

			int entryNum = 0;
			int j = 0;
			apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&entryNum);
			DTRACE(DTRACE_APPSERVER," [%s-%d]entryNum = %d\n", __FUNCTION__, __LINE__, entryNum);
			for(j=1;j<=entryNum;j++)
			{
				memset(&macEntrytmp, 0x00, sizeof(macEntrytmp));
				*((char *)&macEntrytmp) = (char)j;
				if ( apmib_get(MIB_MACFILTER_TBL, (void *)&macEntrytmp))
				{
					DTRACE(DTRACE_APPSERVER,"[%s-%d]entryNum = %d MAC0 %02x:%02x:%02x:%02x:%02x:%02x\n",
						__FUNCTION__, __LINE__, macEntry.macAddr[0], macEntry.macAddr[1], macEntry.macAddr[2], macEntry.macAddr[3], macEntry.macAddr[4], macEntry.macAddr[5]);
					if (!memcmp(macEntrytmp.macAddr, macEntry.macAddr, 6))
					{
						printf("find the mac !!!\n");
						break;
					}
				}
			}
			if ( !apmib_set(MIB_MACFILTER_DEL, (void *)&macEntrytmp)) {
				printf("Delete table entry error!\n");
				goto retError;
			}
		}
	}  
	
setOk_filter:
	apmib_update_web(CURRENT_SETTING);
	send_return_code(fd, APP_SETTING_SUC);
#if !defined(MULTI_WAN_SUPPORT)
#ifndef NO_ACTION
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_add_sn_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  QosSettings  \n");
	cJSON * tmp;
	cJSON * childItem;
	char cmd[32] = "\0";
	char send_conf_buff[5120] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	
	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "addSn");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *addSn = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"addSn = %s\n", addSn);
		
		//apmib_update(CURRENT_SETTING);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;
	
retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;

}

int set_qos_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  QosSettings  \n");
	cJSON * tmp;
	cJSON * childItem;
	char groupx[32] = {0};
	char send_conf_buff[5120] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int x = 0;	
	int auto_uplink = 0;
	int auto_downlink = 0;

	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "enable");	
		if (tmp == NULL)
		{
			printf("qos enable is not \n");
			goto retError;
		}
		
		char *enable = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"enable = %s\n", enable);
		if ( !strcmp(enable, "on") )
			intVal = 1;
		else
			intVal = 0;
		if ( apmib_set( MIB_QOS_ENABLED, (void *)&intVal) == 0) {
			printf(" [%s-%d] apmib_set error!!!\n", __FUNCTION__, __LINE__);
		}
		if (0 == intVal)			
			goto retOK;
		
		tmp = cJSON_GetObjectItem(pItem, "bandwidth_downlink");  
		if (tmp == NULL)
		{
			printf("bandwidth_downlink error\n");
			goto retError;
		}
		char *bandwidth_downlink = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"bandwidth_downlink = %s \n", bandwidth_downlink);
		string_to_dec(bandwidth_downlink, &intVal);
		if ( apmib_set( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&intVal) == 0) {
			printf(" [%s-%d] apmib_set error!!!\n", __FUNCTION__, __LINE__);
		}

		tmp = cJSON_GetObjectItem(pItem, "bandwidth");  
		if (tmp == NULL)
		{
			printf("bandwidth error\n");
			goto retError;
		}
		char *bandwidth = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"bandwidth = %s \n", bandwidth);
		string_to_dec(bandwidth, &intVal);
		if ( apmib_set( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&intVal) == 0) {
			printf(" [%s-%d] apmib_set error!!!\n", __FUNCTION__, __LINE__);
		}

		auto_uplink = 0;
		if ( apmib_set( MIB_QOS_AUTO_UPLINK_SPEED, (void *)&auto_uplink) == 0) {
			printf("[%s-%d] apmib_set MIB_QOS_AUTO_UPLINK_SPEED error!!!\n", __FUNCTION__, __LINE__);
		}
		auto_downlink = 0;
		if ( apmib_set( MIB_QOS_AUTO_DOWNLINK_SPEED, (void *)&auto_downlink) == 0) {	// set auto & mannual status after rule are taken effects
			printf("[%s-%d] apmib_set MIB_QOS_AUTO_DOWNLINK_SPEED error!!!\n", __FUNCTION__, __LINE__);
		}
	}  

retOK:
	apmib_update_web(CURRENT_SETTING);
#ifndef NO_ACTION
	char tmpBuf[64] = {0};
	int pid;
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) {
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _QOS_SCRIPT_PROG);
		execl( tmpBuf, _QOS_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif

	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}


int set_network_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  NetworkSettings \n");
	cJSON * tmp;
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	char send_conf_buff[5120] = {0};

	for (; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		
		tmp = cJSON_GetObjectItem(pItem, "wanType");  
		if (tmp == NULL)
		{
			goto retError;
		}

		char *wanType = tmp->valuestring;
		int wan_mode = DHCP_DISABLED;
		if (!strcmp(wanType, "autoIp"))
		{
			wan_mode = DHCP_CLIENT;
		}
		else if(!strcmp(wanType, "fixedIp"))
		{
			wan_mode = DHCP_DISABLED;

			
			tmp = cJSON_GetObjectItem(pItem, "wan_ip");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *wan_ip = tmp->valuestring;
			struct in_addr inIp, inMask, dns1, dns2, inGateway;
			if( !inet_aton(wan_ip, &inIp) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_WAN_IP_ADDR, (void *)&inIp))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "wan_mask");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *wan_mask = tmp->valuestring;
			if( !inet_aton(wan_mask, &inMask) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_WAN_SUBNET_MASK, (void *)&inMask))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "wan_gateway");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *wan_gateway = tmp->valuestring;
			if( !inet_aton(wan_gateway, &inGateway) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_WAN_DEFAULT_GATEWAY, (void *)&inGateway))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "dns1");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *dns_1 = tmp->valuestring;
			if( !inet_aton(dns_1, &dns1) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_DNS1, (void *)&dns1))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "dns2");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *dns_2 = tmp->valuestring;
			if( !inet_aton(dns_2, &dns2) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_DNS2, (void *)&dns2))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
		}
		else if(!strcmp(wanType, "ppp"))
		{
			wan_mode = PPPOE;

			
			tmp = cJSON_GetObjectItem(pItem, "pppUserName");	
			if (tmp == NULL)
			{
				goto retError;
			}
			char *pppUserName = tmp->valuestring;
			apmib_set(MIB_PPP_USER_NAME, (void *)pppUserName);
			
			tmp = cJSON_GetObjectItem(pItem, "pppPassword");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *pppPassword = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER,"[%s-%d]pppPassword = %s!!!\n", __FUNCTION__, __LINE__, pppPassword);
			apmib_set(MIB_PPP_PASSWORD, (void *)pppPassword);
			
			tmp = cJSON_GetObjectItem(pItem, "pppMtuSize");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *pppMtuSize = tmp->valuestring;
			int mtuSize = 0;
			mtuSize = strtol(pppMtuSize, (char**)NULL, 10);
			if ( apmib_set(MIB_PPP_MTU_SIZE, (void *)&mtuSize) == 0) {
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
		}
		else if(!strcmp(wanType, "bridge"))
		{
			wan_mode = DHCP_CLIENT;
		}
		
		apmib_set(MIB_WAN_DHCP, (void *)&wan_mode);


		apmib_update_web(CURRENT_SETTING);	// update to flash
		
#ifndef NO_ACTION
		app_run_init_script("all");				 
#endif

		
	}  
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}
#define PARENT_CONCTRL_MAC_NUM 64

typedef struct parent_conctrl_t{
	char enable[32];
	char name[32];
	char time[32];
	char mac_num[32];
	char mac_list[PARENT_CONCTRL_MAC_NUM][32];
}PARENT_CONCTRL_T,*P_PARENT_CONCTRL_T;

/* the init mothed  of groupx mib*/
int parent_conctrl_init_group(const int x)
{
	if (x<0 || x>PARENT_CONCTRL_GROUPS_NUMBER)
	{
		printf("parent conctrl id x = %d not valid \n", x);
		return -1;
	}
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_NAME+x,   (void *)"");
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_ENABLE+x, (void *)"");
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_TIME+x,   (void *)"");
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_LIST+x,   (void *)"");
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_NUM+x,    (void *)"");

	return 0;
}
/* get group info from id x*/
int parent_conctrl_get_groupx_from_id(int x, PARENT_CONCTRL_T *p_group)
{
	int i = 0;
	char value[2048] = {0};
	char buffer[2048] = {0};
	int num = 0;
	char *p_str = NULL;
	char *p_tmp = NULL;
	
	if (x < 0 || x > PARENT_CONCTRL_GROUPS_NUMBER)
		return -1;

	apmib_get(MIB_PARENT_CONCTRL_GROUP0_NAME+x,   (void *)p_group->name);
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_ENABLE+x, (void *)p_group->enable);
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_NUM+x,    (void *)p_group->mac_num);
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_TIME+x,   (void *)p_group->time);

	num = atoi(p_group->mac_num);
	if (num > 0)
	{	
		i = 0;
		apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x, (void *)value);
		p_tmp = value;
		
		while((p_tmp = strstr(p_tmp, ",")) !=NULL)
		{
			memset(buffer, 0, sizeof(buffer));
			strncpy(buffer, ++p_tmp, 17);
			//printf("%s-%d buffer = %s\n", __FUNCTION__, __LINE__, buffer);
			strcpy(p_group->mac_list[i], buffer);
			DTRACE(DTRACE_APPSERVER,"%s-%d p_group->mac_list[i] = %s\n", __FUNCTION__, __LINE__, p_group->mac_list[i]);
			i++;			
		}
		
	}

	return num;
}

/* RETURN  NULL GROUP ID , error return -1*/
int parent_conctrl_get_null_group_id()
{
	int i = 0;
	char value[32] = {0};
	char buffer[32] = {0};
	int num = 0;
	
	for(i=0; i<PARENT_CONCTRL_GROUPS_NUMBER; i++)
	{
		apmib_get(MIB_PARENT_CONCTRL_GROUP0_NAME+i, (void *)value);
		DTRACE(DTRACE_APPSERVER,"[%s][%d] i = %d value = %s, num = %d\n", __FUNCTION__, __LINE__, i, value, num);
		if (strlen(value) == 0)
			return num;
		else 
			num++;
	}

	return -1;
}

int parent_conctrl_add_group(PARENT_CONCTRL_T *p_group, const int x)
{
	char buffer[1024]={0};
	char value[1024]={0};
	int i;
	int mac_num = 0;

	if (x < 0 || x > PARENT_CONCTRL_GROUPS_NUMBER)
{
		printf("error\n");
		return -1;
	}
	parent_conctrl_init_group(x);
	DTRACE(DTRACE_APPSERVER,"==== to add a new group %d %s\n", x, buffer);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_TIME+x, p_group->time);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_ENABLE+x, (void *)p_group->enable);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_NAME+x, (void *)p_group->name);
	DTRACE(DTRACE_APPSERVER,"set   MIB_PARENT_CONCTRL_GROUP0_NAME+%d = %s\n", x, p_group->name);
	apmib_get(MIB_PARENT_CONCTRL_VALID_NUM, (void *)value);
	DTRACE(DTRACE_APPSERVER,"get old valid  num = %s\n", value);
	sprintf(buffer, "%d", atoi(value)+1);
	DTRACE(DTRACE_APPSERVER,"set valid  num = %s\n", buffer);
	apmib_set(MIB_PARENT_CONCTRL_VALID_NUM, (void *)buffer);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_LIST+x, (void *)"");

	mac_num = atoi(p_group->mac_num);

	apmib_set(MIB_PARENT_CONCTRL_GROUP0_NUM+x, (void *)p_group->mac_num);
	DTRACE(DTRACE_APPSERVER, "set MIB_PARENT_CONCTRL_GROUP0_NUM+%d = %s\n", x, buffer);
	
	for (i=0; i < mac_num; i++)
	{
		DTRACE(DTRACE_APPSERVER, "will add mac_list[%d] = %s\n", i, p_group->mac_list[i]);
		if (strlen(p_group->mac_list[i])<=0)
			return -1;
		apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x, (void *)value);
		DTRACE(DTRACE_APPSERVER, "===>get MIB_PARENT_CONCTRL_GROUP0_LIST+%d = %s\n", x, value);
		sprintf(buffer, "%s,%s", value, p_group->mac_list[i]);
		apmib_set(MIB_PARENT_CONCTRL_GROUP0_LIST+x, (void *)buffer);
		DTRACE(DTRACE_APPSERVER, "===>set MIB_PARENT_CONCTRL_GROUP0_LIST+%d  = %s\n", x, buffer);
	}

	return 0;
}

/* get group id from group name */
int parent_conctrl_get_id_from_name(char *p_name)
{
	int i = 0;
	char value[512] = {0};
	
	if (strlen(p_name)<=0)
		return -1;
	
	for(i=0; i < PARENT_CONCTRL_GROUPS_NUMBER; i++)
	{
		apmib_get(MIB_PARENT_CONCTRL_GROUP0_NAME+i, (void *)value);
		if (!strcmp(p_name, value)){
			printf("parent conctrl group id is [%d]\n", i);
			return i;
		}
	}
	return -1;
}
/* delete the mac of gruop */
int parent_conctrl_del_group_mac(PARENT_CONCTRL_T *p_deal_mac)
{
	int i;
	int j;
	int x_id = 0;
	int d_mac_num = 0;
	int x_mac_num = 0;
	char value[1024] = {0};
	char buffer[1024] = {0};
	char mac_t[1024] = {0};
	PARENT_CONCTRL_T x_info;
	
	if ( strlen(p_deal_mac->name)<=0 )
		return -1;

	//printf("[%s][%d] p_del_mac->name = %s\n",__FUNCTION__, __LINE__, p_deal_mac->name);
	x_id = parent_conctrl_get_id_from_name(p_deal_mac->name);
	if(x_id < 0)
		return -1;
		
	DTRACE(DTRACE_APPSERVER, " get group name = %s, grup id = %d\n",p_deal_mac->name, x_id);

	parent_conctrl_get_groupx_from_id(x_id, &x_info);
	d_mac_num = atoi(p_deal_mac->mac_num);
	x_mac_num = atoi(x_info.mac_num);
	DTRACE(DTRACE_APPSERVER, "[%s][%d] d_mac_num = %d\n",__FUNCTION__, __LINE__, d_mac_num);
	for(i=0; i < d_mac_num; i++)
	{
		for(j = 0; j < x_mac_num; j++)
		{
			//printf("***** group mac[%s] vs [%s] ******\n",  x_info.mac_list[j], p_deal_mac->mac_list[i]);
			if(!strcmp(p_deal_mac->mac_list[i], x_info.mac_list[j]))
			{
				//printf("i = %d, delete mac >>>>%s\n", i, x_info.mac_list[j]);
				memset(x_info.mac_list[j], 0, sizeof(x_info.mac_list[j]));
				break;
			}
		}
	}

	for(j = 0; j < x_mac_num; j++)
	{
		DTRACE(DTRACE_APPSERVER, "\n\n j = %d, delete mac >>>>%s\n", j, x_info.mac_list[j]);
	}
	
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
	//printf("----------- before delete mac %d = %s\n", x_id, value);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)"");
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
	i = 0;
	for(j = 0; j < x_mac_num; j++)
	{
		//printf("will add mac_list[%d] = %s\n", j, x_info.mac_list[j]);
		if (strlen(x_info.mac_list[j])>0)
		{
			apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
			sprintf(buffer, "%s,%s", value, x_info.mac_list[j]);
			apmib_set(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)buffer);
			//printf("===>set MIB_PARENT_CONCTRL_GROUP0_LIST+%d  = %s\n", x_id, buffer);
			i++;
		}
	}
	sprintf(buffer, "%d", i);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_NUM+x_id, (void *)buffer);
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
	
	return 0;
}
/* delete the mac of gruop */
int parent_conctrl_add_group_mac(PARENT_CONCTRL_T *p_deal_mac)
{
	int i;
	int j;
	int x_id = 0;
	int d_mac_num = 0;
	int x_mac_num = 0;
	char value[1024] = {0};
	char buffer[1024] = {0};
	char mac_t[1024] = {0};
	PARENT_CONCTRL_T x_info;
	
	if ( strlen(p_deal_mac->name)<=0 )
		return -1;

	DTRACE(DTRACE_APPSERVER, "[%s][%d] p_del_mac->name = %s\n",__FUNCTION__, __LINE__, p_deal_mac->name);
	x_id = parent_conctrl_get_id_from_name(p_deal_mac->name);
	if(x_id < 0)
		return -1;
		
	//printf(" get group name = %s, grup id = %d\n",p_deal_mac->name, x_id);

	parent_conctrl_get_groupx_from_id(x_id, &x_info);
	d_mac_num = atoi(p_deal_mac->mac_num);
	x_mac_num = atoi(x_info.mac_num);
	DTRACE(DTRACE_APPSERVER, "[%s][%d] d_mac_num = %d\n",__FUNCTION__, __LINE__, d_mac_num);
	for(i=0; i < d_mac_num; i++)
	{
		for(j = 0; j < x_mac_num; j++)
		{
			printf("***** group mac[%s] vs [%s] ******\n",  x_info.mac_list[j], p_deal_mac->mac_list[i]);
			if(!strcmp(p_deal_mac->mac_list[i], x_info.mac_list[j]))
			{
				printf("i = %d, add mac >>>>%s\n", i, p_deal_mac->mac_list[i]);
				memset(p_deal_mac->mac_list[i], 0, sizeof(p_deal_mac->mac_list[i]));
				break;
			}
		}
	}

	for(j = 0; j < d_mac_num; j++)
	{
		DTRACE(DTRACE_APPSERVER, "\n\n j = %d, add mac >>>>%s\n", j, p_deal_mac->mac_list[j]);
	}
	
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
	DTRACE(DTRACE_APPSERVER, "----------- before add mac %d = %s\n", x_id, value);
	i = 0;
	for(j = 0; j < d_mac_num; j++)
	{
		DTRACE(DTRACE_APPSERVER, "will add mac_list[%d] = %s\n", j, p_deal_mac->mac_list[j]);
		if (strlen(p_deal_mac->mac_list[j])>0)
		{
			apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
			sprintf(buffer, "%s,%s", value, p_deal_mac->mac_list[j]);
			apmib_set(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)buffer);
			printf("===>set MIB_PARENT_CONCTRL_GROUP0_LIST+%d  = %s\n", x_id, buffer);
			i++;
		}
	}
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_NUM+x_id, (void *)value);
	sprintf(buffer, "%d", atoi(value)+i);
	apmib_set(MIB_PARENT_CONCTRL_GROUP0_NUM+x_id, (void *)buffer);
	apmib_get(MIB_PARENT_CONCTRL_GROUP0_LIST+x_id, (void *)value);
	DTRACE(DTRACE_APPSERVER, "----------- add delete mac = %s\n", value);
	
	return 0;
}

/* delete group */
int parent_conctrl_del_group(const char *group_name)
{
	int i;
	char value[1024] = {0};
	char buffer[1024] = {0};
	
	if ( strlen(group_name)<=0 )
		return -1;

	DTRACE(DTRACE_APPSERVER, "group_name = %s\n", group_name);
	for(i=0; i < PARENT_CONCTRL_GROUPS_NUMBER; i++)
	{
		apmib_get(MIB_PARENT_CONCTRL_GROUP0_NAME+i, (void *)value);
		printf("value = %s\n", value);
		if(!strcmp(value, group_name))
		{
			printf("i = %d\n", i);
			break;
		}
	}

	DTRACE(DTRACE_APPSERVER, "==== init group[%d] \n", i);
	parent_conctrl_init_group(i);
	/* valid group number delete one */
	apmib_get(MIB_PARENT_CONCTRL_VALID_NUM, (void *)value);
	DTRACE(DTRACE_APPSERVER,"get PARENT_CONCTRL_VALID_NUM = %s\n", value);
	sprintf(buffer, "%d", atoi(value)-1);
	apmib_set(MIB_PARENT_CONCTRL_VALID_NUM, (void *)buffer);
	DTRACE(DTRACE_APPSERVER, "set PARENT_CONCTRL_VALID_NUM = %s\n", buffer);
	
	return 0;
}

/* min to hh:mm */
int transform_str_to_min(char *p_time, unsigned int *min)
{
	char *p_tmp = NULL;
	char h[32] = {0};
	char m[32] = {0};
	unsigned int hour = 0;


	DTRACE(DTRACE_APPSERVER, "p_time = %s\n",p_time);

	if ( strlen(p_time)<=0 )
		return -1;

	if ( (p_tmp = strstr(p_time, ":"))==NULL )
		return -1;

	strncpy(h, p_time, p_tmp-p_time);
	strcpy(m, p_tmp+1); 
	*min = atoi(h)*60 + atoi(m);
	DTRACE(DTRACE_APPSERVER, "min = %d\n", *min);
	return 0;
}
typedef struct parental_control_dev_info
{
	int  dev_num;
	char mac[MAX_STA_NUM][32];
	char hostname[MAX_STA_NUM][64];
}PARENTAL_CONCTROL_DEV_INFO, *PARENTAL_CONCTROL_DEV_INFO_P;

int set_add_parental_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  set_add_parental_conf \n");
	cJSON * tmp;
	cJSON * childItem;
	char groupx[32] = {0};
	char value[1024] = {0};
	char buffer[1024] = {0};
	char send_conf_buff[2048] = {0};
	char staInfo[256]={0};
	char tmpInfo[64] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int x = 0;	
	int z;
	int j;
	int valid_num = 0;
	int minStart = 0;
	int minEnd = 0;
	int num_id, get_id, add_id, del_id, delall_id;
	int staNum = 0;
	int intVal = 0;
	void *pEntry;
	PARENT_CONCTRL_T group_info;
	PARENT_CONTRL_T parentContrlEntry, parentContrltmp;
#ifndef NO_ACTION
	int pid;
	char tmpBuf[64] = {0};
#endif	
	RTK_LAN_DEVICE_INFO_T devinfo[MAX_STA_NUM] = {0};
		
	rtk_get_lan_device_info(&staNum, devinfo, MAX_STA_NUM);

	DTRACE(DTRACE_APPSERVER, "enter set_add_parental_conf iCount = %d\n", iCount);
	
	for (i=0; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  

		tmp = cJSON_GetObjectItem(pItem, "networkEnable");  
		if (tmp == NULL)
		{
			//goto retError;
		}
		else 
		{
			char *networkEnable = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "networkEnable = %s\n", networkEnable);
		}

		tmp = cJSON_GetObjectItem(pItem, "Mon");  
		if (tmp == NULL)
			goto retError;
		char *Mon = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Mon = %s\n", Mon);
		tmp = cJSON_GetObjectItem(pItem, "Tue");  
		if (tmp == NULL)
			goto retError;
		char *Tue = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Tue = %s\n", Tue);
		tmp = cJSON_GetObjectItem(pItem, "Wed");  
		if (tmp == NULL)
			goto retError;
		char *Wed = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Wed = %s\n", Wed);
		tmp = cJSON_GetObjectItem(pItem, "Thu");  
		if (tmp == NULL)
			goto retError;
		char *Thu = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Thu = %s\n", Thu);
		tmp = cJSON_GetObjectItem(pItem, "Fri");  
		if (tmp == NULL)
			goto retError;
		char *Fri = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Fri = %s\n", Fri);
		tmp = cJSON_GetObjectItem(pItem, "Sat");  
		if (tmp == NULL)
			goto retError;
		char *Sat = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sat = %s\n", Sat);
		tmp = cJSON_GetObjectItem(pItem, "Sun");  
		if (tmp == NULL)
			goto retError;
		char *Sun = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sun = %s\n", Sun);

		tmp = cJSON_GetObjectItem(pItem, "timeStart");  
		if (tmp == NULL)
			goto retError;
		char *timeStart = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeStart = %s\n", timeStart);
		transform_str_to_min(timeStart, &minStart);
		
		tmp = cJSON_GetObjectItem(pItem, "timeEnd");  
		if (tmp == NULL)
			goto retError;
		char *timeEnd = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeEnd = %s\n", timeEnd);
		transform_str_to_min(timeEnd, &minEnd);

		cJSON* add_obj = cJSON_GetObjectItem(pItem, "groupDev");
		int iarray = cJSON_GetArraySize(add_obj);  
		DTRACE(DTRACE_APPSERVER, "groupDev iarray = %d\n",iarray);

		PARENTAL_CONCTROL_DEV_INFO dev_list;
		memset(&dev_list, 0, sizeof(dev_list));

		for( j=0; j<iarray; j++ )
		{
			cJSON* mac_array = cJSON_GetArrayItem(add_obj, j); 
			tmp = cJSON_GetObjectItem(mac_array, "mac");  
			if (tmp == NULL)
				goto retError;
			char *mac = tmp->valuestring;
			strcpy(&dev_list.mac[j], mac);
			tmp = cJSON_GetObjectItem(mac_array, "hostname");  
			if (tmp == NULL)
				goto retError;
			char *hostname = tmp->valuestring;		
			strcpy(&dev_list.hostname[j], hostname);
			dev_list.dev_num++;
		}

#if 1
		for ( j=0; j<dev_list.dev_num; j++ )
		{
			DTRACE(DTRACE_APPSERVER, "mac = %s\n",dev_list.mac[j]);
			DTRACE(DTRACE_APPSERVER, "hostname = %s\n",dev_list.hostname[j]);
		}
#endif
		num_id = MIB_PARENT_CONTRL_TBL_NUM;
		get_id = MIB_PARENT_CONTRL_TBL;
		add_id = MIB_PARENT_CONTRL_ADD; 
		del_id = MIB_PARENT_CONTRL_DEL;
		int entryNum = 0;
		memset(&parentContrlEntry, '\0', sizeof(parentContrlEntry));
		pEntry = (void *)&parentContrlEntry;

		/* same time check*/
		if((Mon||Tue||Wed||Thu||Fri||Sat||Sun)&& (minEnd-minStart>0))
		{
			apmib_get(MIB_PARENT_CONTRL_TBL_NUM, (void *)&entryNum);
			for(j=1;j<=entryNum;j++)
			{
				memset(&parentContrltmp, 0x00, sizeof(parentContrltmp));
				*((char *)&parentContrltmp) = (char)j;
				if ( apmib_get(MIB_PARENT_CONTRL_TBL, (void *)&parentContrltmp))
				{

#if 0
					printf("(table--%d)tmpMon=%d tmpTues=%d  tmpWed=%d  tmpThur=%d  tmpFri=%d tmpSat=%d  tmpSun=%d  tmpstart=%d  tmpend=%d \n", \
					j,parentContrltmp.parentContrlWeekMon,parentContrltmp.parentContrlWeekTues,parentContrltmp.parentContrlWeekWed,\
					parentContrltmp.parentContrlWeekThur,parentContrltmp.parentContrlWeekFri,parentContrltmp.parentContrlWeekSat, \
					parentContrltmp.parentContrlWeekSun, parentContrltmp.parentContrlStartTime,parentContrltmp.parentContrlEndTime);
#endif
					if((atoi(Mon)==parentContrltmp.parentContrlWeekMon)&&(atoi(Tue)==parentContrltmp.parentContrlWeekTues)&&
					(atoi(Wed)==parentContrltmp.parentContrlWeekWed)&&(atoi(Thu)==parentContrltmp.parentContrlWeekThur)&&
					(atoi(Fri)==parentContrltmp.parentContrlWeekFri)&&(atoi(Sat)==parentContrltmp.parentContrlWeekSat)&&
					(atoi(Sun)==parentContrltmp.parentContrlWeekSun)&&(minStart==parentContrltmp.parentContrlStartTime)&&
					(minEnd==parentContrltmp.parentContrlEndTime))
					{
						DTRACE(DTRACE_APPSERVER, "parent contrl rule exist\n");
						send_return_code(fd, APP_SAME_GROUP);
						return EXIT_FAILURE;
					} 				  
				}
			}
		}
		else
		{
			DTRACE(DTRACE_APPSERVER, "parent contrl rule table null\n");
			goto retError;
		}

		parentContrlEntry.parentContrlWeekMon 	= atoi(Mon);
		parentContrlEntry.parentContrlWeekTues 	= atoi(Tue);
		parentContrlEntry.parentContrlWeekWed 	= atoi(Wed);
		parentContrlEntry.parentContrlWeekThur 	= atoi(Thu);
		parentContrlEntry.parentContrlWeekFri 	= atoi(Fri);
		parentContrlEntry.parentContrlWeekSat 	= atoi(Sat);
		parentContrlEntry.parentContrlWeekSun 	= atoi(Sun);
		parentContrlEntry.parentContrlStartTime = minStart;
		parentContrlEntry.parentContrlEndTime 	= minEnd;
		memset(staInfo,0,sizeof(staInfo));
		for(i=0; i<dev_list.dev_num; i++)
		{
			if (dev_list.dev_num==1)
			{
				sprintf(tmpInfo, "%s", dev_list.mac[i]);
				DTRACE(DTRACE_APPSERVER, "tmpInfo = %s\n", tmpInfo);
			}
			else
			{
				sprintf(tmpInfo, "%s;", dev_list.mac[i]);
				DTRACE(DTRACE_APPSERVER, "tmpInfo = %s\n", tmpInfo);
			}
			strcat(staInfo, tmpInfo);
		}
		if(strlen(staInfo))
	        strncpy(parentContrlEntry.parentContrlTerminal, staInfo, strlen(staInfo));

	    if ( !apmib_get(num_id, (void *)&entryNum)) 
	    {
			printf("Get entry number error!\n");
			goto retError;
	    }
	    else
	    {
			if ( (entryNum + 1) > MAX_PARENT_CONTRL_TIME_NUM_LIST) 
			{
				printf("parent contrl table full\n");
				goto retError;
			}
	   }
		/* set to MIB. try to delete it first to avoid duplicate case*/
		apmib_set(del_id, pEntry);
		if ( apmib_set(add_id, pEntry) == 0) 
		{
			printf("Add table entry error!\n");
			goto retError;
		}
	}  
	intVal = 1;
	apmib_set(MIB_PARENT_CONTRL_ENABLED, (void *)&intVal);
	
	apmib_update(CURRENT_SETTING);
	send_return_code(fd, APP_SETTING_SUC);
#ifndef NO_ACTION

	pid = fork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) 
	{
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_del_all_parental_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  set_del_all_parental_conf \n");
	char send_conf_buff[2048] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int j = 0;
	int num_id, get_id, add_id, del_id, delall_id;
	void *pEntry;
	PARENT_CONTRL_T parentContrlEntry;
	char *networkEnable = 0;
#ifndef NO_ACTION
	int pid;
	char tmpBuf[64] = {0};
#endif	
	int entryNum = 0;

	DTRACE(DTRACE_APPSERVER, "enter set_del_all_parental_conf iCount = %d\n", iCount);
	
	for (i=0; i < iCount; ++i) 
	{  
		num_id = MIB_PARENT_CONTRL_TBL_NUM;
		get_id = MIB_PARENT_CONTRL_TBL;
		add_id = MIB_PARENT_CONTRL_ADD;
		del_id = MIB_PARENT_CONTRL_DEL;

		memset(&parentContrlEntry, '\0', sizeof(parentContrlEntry));
		pEntry = (void *)&parentContrlEntry;

		if ( !apmib_get(num_id, (void *)&entryNum)) 
	    {
			printf("Get entry number error!\n");
			goto retError;
	    }
		
		for (j=1; j<=entryNum; j++)
		{
			*((char *)pEntry) = (char)j;
			if ( !apmib_get(get_id, pEntry)) {
				printf("Get table entry error!\n");
				goto retError;
			}
			if ( !apmib_set(del_id, pEntry)) {
				printf("Delete table entry error!\n");
				goto retError;
			}
		}
		
		apmib_update(CURRENT_SETTING);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	
#ifndef NO_ACTION

	pid = fork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) 
	{
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_del_parental_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  set_del_parental_conf \n");
	cJSON * tmp;
	cJSON * childItem;
	char groupx[32] = {0};
	char value[1024] = {0};
	char buffer[1024] = {0};
	char send_conf_buff[2048] = {0};
	char staInfo[256]={0};
	char tmpInfo[64] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int x = 0;	
	int z;
	int j;
	int valid_num = 0;
	int minStart = 0;
	int minEnd = 0;
	int num_id, get_id, add_id, del_id, delall_id;
	void *pEntry;
	PARENT_CONCTRL_T group_info;
	PARENT_CONTRL_T parentContrlEntry, parentContrltmp;
	char *networkEnable = 0;
#ifndef NO_ACTION
	int pid;
	char tmpBuf[64] = {0};
#endif	

	DTRACE(DTRACE_APPSERVER, "enter set_add_parental_conf iCount = %d\n", iCount);
	
	for (i=0; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  

		tmp = cJSON_GetObjectItem(pItem, "select");  
		if (tmp == NULL)
			goto retError;
		char *select = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "select = %s\n", select);


		cJSON* childItem;

		childItem = cJSON_GetObjectItem(pItem, select);  
		if (childItem == NULL) continue;
		
		cJSON* pChildItem = cJSON_GetArrayItem(childItem, 0);  
		if (NULL == pChildItem)
		{  
			printf("pChildItem is null. continue\n");
			continue;  
		}  
		
		tmp = cJSON_GetObjectItem(pChildItem, "networkEnable");  
		if (tmp == NULL)
		{
			printf("groupName is null\n");
			goto retError;
		}
		else 
		{
			networkEnable = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "networkEnable = %s\n", networkEnable);
		}

		tmp = cJSON_GetObjectItem(pChildItem, "Mon");  
		if (tmp == NULL)
		{
			printf("Mon is null\n");
			goto retError;
		}
		char *Mon = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Mon = %s\n", Mon);

		tmp = cJSON_GetObjectItem(pChildItem, "Tue");  
		if (tmp == NULL)
		{
			printf("Tue is null\n");
			goto retError;
		}
		char *Tue = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Tue = %s\n", Tue);

		tmp = cJSON_GetObjectItem(pChildItem, "Wed");  
		if (tmp == NULL)
		{
			printf("Wed is null\n");
			goto retError;
		}
		char *Wed = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Wed = %s\n", Wed);

		tmp = cJSON_GetObjectItem(pChildItem, "Thu");  
		if (tmp == NULL)
		{
			printf("Thu is null\n");
			goto retError;
		}
		char *Thu = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Thu = %s\n", Thu);

		tmp = cJSON_GetObjectItem(pChildItem, "Fri");  
		if (tmp == NULL)
		{
			printf("Fri is null\n");
			goto retError;
		}
		char *Fri = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Fri = %s\n", Fri);

		tmp = cJSON_GetObjectItem(pChildItem, "Sat");  
		if (tmp == NULL)
		{
			printf("Sat is null\n");
			goto retError;
		}
		char *Sat = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sat = %s\n", Sat);

		tmp = cJSON_GetObjectItem(pChildItem, "Sun");  
		if (tmp == NULL)
		{
			printf("Sun is null\n");
			goto retError;
		}
		char *Sun = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sun = %s\n", Sun);

		tmp = cJSON_GetObjectItem(pChildItem, "timeStart");  
		if (tmp == NULL)
		{
			printf("timeStart is null\n");
			goto retError;
		}
		char *timeStart = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeStart = %s\n", timeStart);

		tmp = cJSON_GetObjectItem(pChildItem, "timeEnd");  
		if (tmp == NULL)
		{
			printf("timeEnd is null\n");
			goto retError;
		}
		char *timeEnd = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeEnd = %s\n", timeEnd);

		cJSON* del_obj = cJSON_GetObjectItem(pChildItem, "groupDev");
		int iarray = cJSON_GetArraySize(del_obj);  

		PARENTAL_CONCTROL_DEV_INFO dev_list;
		memset(&dev_list, 0, sizeof(dev_list));
		for( j=0; j<iarray; j++ )
		{
			cJSON* mac_array = cJSON_GetArrayItem(del_obj, j); 
			tmp = cJSON_GetObjectItem(mac_array, "mac");  
			if (tmp == NULL)
				goto retError;
			char *mac = tmp->valuestring;
			strcpy(&dev_list.mac[j], mac);
			tmp = cJSON_GetObjectItem(mac_array, "hostname");  
			if (tmp == NULL)
				goto retError;
			char *hostname = tmp->valuestring;		
			strcpy(&dev_list.hostname[j], hostname);
			dev_list.dev_num++;
		}
#if 0
		for ( j=0; j<dev_list.dev_num; j++ )
		{
			printf("mac = %s\n", dev_list.mac[j]);
			printf("hostname = %s\n", dev_list.hostname[j]);
		}
#endif
		num_id = MIB_PARENT_CONTRL_TBL_NUM;
		get_id = MIB_PARENT_CONTRL_TBL;
		add_id = MIB_PARENT_CONTRL_ADD;
		del_id = MIB_PARENT_CONTRL_DEL;

		int entryNum = 0;
		memset(&parentContrlEntry, '\0', sizeof(parentContrlEntry));
		pEntry = (void *)&parentContrlEntry;
		
		memset(staInfo,0,sizeof(staInfo));
		for(i=0; i<dev_list.dev_num; i++)
		{
			if (dev_list.dev_num==1)
			{
				sprintf(tmpInfo, "%s", dev_list.mac[i]);
			}
			else
			{
				sprintf(tmpInfo, "%s;", dev_list.mac[i]);
			}
			DTRACE(DTRACE_APPSERVER, "tmpInfo = %s\n", tmpInfo);
			strcat(staInfo, tmpInfo);
		}
		
		if(strlen(staInfo))
	        strncpy(parentContrlEntry.parentContrlTerminal, staInfo, strlen(staInfo));
	    if ( !apmib_get(num_id, (void *)&entryNum)) 
	    {
			printf("Get entry number error!\n");
			goto retError;
	    }
		sscanf(select, "group%d", &i);
		DTRACE(DTRACE_APPSERVER, "select id  = %d\n", i);
		*((char *)pEntry) = (char)i;
		if ( !apmib_get(get_id, pEntry)) {
			printf("Get table entry error!\n");
			goto retError;
		}
		if ( !apmib_set(del_id, pEntry)) {
			printf("Delete table entry error!\n");
			goto retError;
		}

		apmib_update(CURRENT_SETTING);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	
#ifndef NO_ACTION

	pid = fork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) 
	{
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}
int set_modify_parental_conf(int fd, cJSON *jason_obj)
{	
	cJSON * tmp;
	cJSON * childItem;
	char groupx[32] = {0};
	char value[1024] = {0};
	char buffer[1024] = {0};
	char send_conf_buff[2048] = {0};
	char staInfo[256]={0};
	char tmpInfo[64] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int x = 0;	
	int z;
	int j;
	int valid_num = 0;
	int minStart = 0;
	int minEnd = 0;
	int minStart1 = 0;
	int minEnd1 = 0;
	int select_id = 0;
	int entryNum = 0;
	int num_id, get_id, add_id, del_id, delall_id;
	void *pEntry;
	PARENT_CONCTRL_T group_info;
	PARENT_CONTRL_T parentContrlEntry, parentContrltmp;
#ifndef NO_ACTION
	int pid;
	char tmpBuf[64] = {0};
#endif	

	DTRACE(DTRACE_APPSERVER, "enter set_modify_parental_conf iCount = %d\n", iCount);
	
	for (i=0; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  

		tmp = cJSON_GetObjectItem(pItem, "networkEnable");  
		if (tmp == NULL)
		{
			printf("groupName is null\n");
			//goto retError;
		}
		else
		{
			char *networkEnable1 = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "networkEnable = %s\n", networkEnable1);
		}

		tmp = cJSON_GetObjectItem(pItem, "Mon");  
		if (tmp == NULL)
		{
			printf("Mon is null\n");
			goto retError;
		}
		char *Mon1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Mon = %s\n", Mon1);

		tmp = cJSON_GetObjectItem(pItem, "Tue");  
		if (tmp == NULL)
		{
			printf("Tue is null\n");
			goto retError;
		}
		char *Tue1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Tue = %s\n", Tue1);

		tmp = cJSON_GetObjectItem(pItem, "Wed");  
		if (tmp == NULL)
		{
			printf("Wed is null\n");
			goto retError;
		}
		char *Wed1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Wed = %s\n", Wed1);

		tmp = cJSON_GetObjectItem(pItem, "Thu");  
		if (tmp == NULL)
		{
			printf("Thu is null\n");
			goto retError;
		}
		char *Thu1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Thu = %s\n", Thu1);

		tmp = cJSON_GetObjectItem(pItem, "Fri");  
		if (tmp == NULL)
		{
			printf("Fri is null\n");
			goto retError;
		}
		char *Fri1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Fri = %s\n", Fri1);

		tmp = cJSON_GetObjectItem(pItem, "Sat");  
		if (tmp == NULL)
		{
			printf("Sat is null\n");
			goto retError;
		}
		char *Sat1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sat = %s\n", Sat1);

		tmp = cJSON_GetObjectItem(pItem, "Sun");  
		if (tmp == NULL)
		{
			printf("Sun is null\n");
			goto retError;
		}
		char *Sun1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sun = %s\n", Sun1);

		tmp = cJSON_GetObjectItem(pItem, "timeStart");  
		if (tmp == NULL)
		{
			printf("timeStart is null\n");
			goto retError;
		}
		char *timeStart1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeStart = %s\n", timeStart1);
		transform_str_to_min(timeStart1, &minStart1);

		tmp = cJSON_GetObjectItem(pItem, "timeEnd");  
		if (tmp == NULL)
		{
			printf("timeEnd is null\n");
			goto retError;
		}
		char *timeEnd1 = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeEnd = %s\n", timeEnd1);
		transform_str_to_min(timeEnd1, &minEnd1);

		cJSON* modify_obj = cJSON_GetObjectItem(pItem, "groupDev");
		int iarray1 = cJSON_GetArraySize(modify_obj);  
		DTRACE(DTRACE_APPSERVER, "groupDev iarray = %d\n",iarray1);

		PARENTAL_CONCTROL_DEV_INFO dev_list1;
		memset(&dev_list1, 0, sizeof(dev_list1));
		for( j=0; j<iarray1; j++ )
		{
			cJSON* mac_array = cJSON_GetArrayItem(modify_obj, j); 
			tmp = cJSON_GetObjectItem(mac_array, "mac");  
			if (tmp == NULL)
				goto retError;
			char *mac = tmp->valuestring;
			strcpy(&dev_list1.mac[j], mac);
			tmp = cJSON_GetObjectItem(mac_array, "hostname");  
			if (tmp == NULL)
				goto retError;
			char *hostname = tmp->valuestring;		
			strcpy(&dev_list1.hostname[j], hostname);
			dev_list1.dev_num++;
		}
		#if 0
		//printf("dev_list.dev_num = %d\n", dev_list.dev_num);
		for ( j=0; j<dev_list1.dev_num; j++ )
		{
			printf("mac = %s\n", dev_list1.mac[j]);
			printf("hostname = %s\n", dev_list1.hostname[j]);
		}
		#endif
		tmp = cJSON_GetObjectItem(pItem, "select");  
		if (tmp == NULL)
			goto retError;
		char *select = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "select = %s\n", select);
		
		sscanf(select, "group%d", &select_id);
		DTRACE(DTRACE_APPSERVER, "select id  = %d\n", select_id);

		cJSON* childItem;
		childItem = cJSON_GetObjectItem(pItem, select);  
		if (childItem == NULL)
		{
			printf("cJSON_GetObjectItem select = %s error\n", select);
			goto retError;
		}
		
		cJSON* pChildItem = cJSON_GetArrayItem(childItem, 0);  
		if (NULL == pChildItem)
		{  
			printf("select = %s continue\n", select);
			continue;  
		}  
		
		tmp = cJSON_GetObjectItem(pChildItem, "networkEnable");  
		if (tmp == NULL)
		{
			printf("groupName is null\n");
			goto retError;
		}
		else
		{
			char *networkEnable = tmp->valuestring;
			printf("networkEnable = %s\n", networkEnable);
		}

		tmp = cJSON_GetObjectItem(pChildItem, "Mon");  
		if (tmp == NULL)
		{
			printf("Mon is null\n");
			goto retError;
		}
		char *Mon = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Mon = %s\n", Mon);

		tmp = cJSON_GetObjectItem(pChildItem, "Tue");  
		if (tmp == NULL)
		{
			printf("Tue is null\n");
			goto retError;
		}
		char *Tue = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Tue = %s\n", Tue);

		tmp = cJSON_GetObjectItem(pChildItem, "Wed");  
		if (tmp == NULL)
		{
			printf("Wed is null\n");
			goto retError;
		}
		char *Wed = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Wed = %s\n", Wed);

		tmp = cJSON_GetObjectItem(pChildItem, "Thu");  
		if (tmp == NULL)
		{
			printf("Thu is null\n");
			goto retError;
		}
		char *Thu = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Thu = %s\n", Thu);

		tmp = cJSON_GetObjectItem(pChildItem, "Fri");  
		if (tmp == NULL)
		{
			printf("Fri is null\n");
			goto retError;
		}
		char *Fri = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Fri = %s\n", Fri);

		tmp = cJSON_GetObjectItem(pChildItem, "Sat");  
		if (tmp == NULL)
		{
			printf("Sat is null\n");
			goto retError;
		}
		char *Sat = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sat = %s\n", Sat);

		tmp = cJSON_GetObjectItem(pChildItem, "Sun");  
		if (tmp == NULL)
		{
			printf("Sun is null\n");
			goto retError;
		}
		char *Sun = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "Sun = %s\n", Sun);

		tmp = cJSON_GetObjectItem(pChildItem, "timeStart");  
		if (tmp == NULL)
		{
			printf("timeStart is null\n");
			goto retError;
		}
		char *timeStart = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeStart = %s\n", timeStart);

		tmp = cJSON_GetObjectItem(pChildItem, "timeEnd");  
		if (tmp == NULL)
		{
			printf("timeEnd is null\n");
			goto retError;
		}
		char *timeEnd = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "timeEnd = %s\n", timeEnd);

		cJSON* del_obj = cJSON_GetObjectItem(pChildItem, "groupDev");
		int iarray = cJSON_GetArraySize(del_obj);  
		DTRACE(DTRACE_APPSERVER, "groupDev iarray = %d\n",iarray);

		PARENTAL_CONCTROL_DEV_INFO dev_list;
		memset(&dev_list, 0, sizeof(dev_list));
		for( j=0; j<iarray; j++ )
		{
			cJSON* mac_array = cJSON_GetArrayItem(del_obj, j); 
			tmp = cJSON_GetObjectItem(mac_array, "mac");  
			if (tmp == NULL)
				goto retError;
			char *mac = tmp->valuestring;
			strcpy(&dev_list.mac[j], mac);
			tmp = cJSON_GetObjectItem(mac_array, "hostname");  
			if (tmp == NULL)
				goto retError;
			char *hostname = tmp->valuestring;		
			strcpy(&dev_list.hostname[j], hostname);
			dev_list.dev_num++;
		}
		#if 0
		//printf("dev_list.dev_num = %d\n", dev_list.dev_num);
		for ( j=0; j<dev_list.dev_num; j++ )
		{
			printf("mac = %s\n", dev_list.mac[j]);
			printf("hostname = %s\n", dev_list.hostname[j]);
		}
		#endif
		/* same time check*/
		if((Mon1||Tue1||Wed1||Thu1||Fri1||Sat1||Sun1)&& (minEnd1-minStart1>0))
		{
			apmib_get(MIB_PARENT_CONTRL_TBL_NUM, (void *)&entryNum);
			for (j=1; j<=entryNum; j++)
			{
				memset(&parentContrltmp, 0x00, sizeof(parentContrltmp));
				*((char *)&parentContrltmp) = (char)j;
				if ( j!=select_id && apmib_get(MIB_PARENT_CONTRL_TBL, (void *)&parentContrltmp))
				{				
					DTRACE(DTRACE_APPSERVER, "(table--%d)tmpMon=%d tmpTues=%d  tmpWed=%d  tmpThur=%d  tmpFri=%d tmpSat=%d  tmpSun=%d  tmpstart=%d  tmpend=%d \n", \
					j,parentContrltmp.parentContrlWeekMon,parentContrltmp.parentContrlWeekTues,parentContrltmp.parentContrlWeekWed,\
					parentContrltmp.parentContrlWeekThur,parentContrltmp.parentContrlWeekFri,parentContrltmp.parentContrlWeekSat, \
					parentContrltmp.parentContrlWeekSun, parentContrltmp.parentContrlStartTime,parentContrltmp.parentContrlEndTime);

					if((atoi(Mon1)==parentContrltmp.parentContrlWeekMon)&&(atoi(Tue1)==parentContrltmp.parentContrlWeekTues)&&
					(atoi(Wed1)==parentContrltmp.parentContrlWeekWed)&&(atoi(Thu1)==parentContrltmp.parentContrlWeekThur)&&
					(atoi(Fri1)==parentContrltmp.parentContrlWeekFri)&&(atoi(Sat1)==parentContrltmp.parentContrlWeekSat)&&
					(atoi(Sun1)==parentContrltmp.parentContrlWeekSun)&&(minStart1==parentContrltmp.parentContrlStartTime)&&
					(minEnd1==parentContrltmp.parentContrlEndTime))
					{
						printf("parent contrl rule exist\n");
						send_return_code(fd, APP_SAME_GROUP);
						return EXIT_FAILURE;
					} 				  
				}
			}
		}
		else
		{
			printf("parent contrl rule table null\n");
			goto retError;
		}

		num_id = MIB_PARENT_CONTRL_TBL_NUM;
		get_id = MIB_PARENT_CONTRL_TBL;
		add_id = MIB_PARENT_CONTRL_ADD;
		del_id = MIB_PARENT_CONTRL_DEL;
		memset(&parentContrlEntry, '\0', sizeof(parentContrlEntry));
		pEntry = (void *)&parentContrlEntry;
		
		memset(staInfo,0,sizeof(staInfo));
		for(i=0; i<dev_list.dev_num; i++)
		{
			if (dev_list.dev_num==1)
			{
				sprintf(tmpInfo, "%s", dev_list.mac[i]);
			}
			else
			{
				sprintf(tmpInfo, "%s;", dev_list.mac[i]);
			}
			strcat(staInfo, tmpInfo);
		}
		
		if(strlen(staInfo))
	        strncpy(parentContrlEntry.parentContrlTerminal, staInfo, strlen(staInfo));
		
	    if ( !apmib_get(num_id, (void *)&entryNum)) 
	    {
			printf("Get entry number error!\n");
			goto retError;
	    }

		/* delete old rule */
		*((char *)pEntry) = (char)select_id;
		if ( !apmib_get(get_id, pEntry)) {
			printf("Get table entry error!\n");
			goto retError;
		}
		if ( !apmib_set(del_id, pEntry)) {
			printf("Delete table entry error!\n");
			goto retError;
		}

		entryNum = 0;
		memset(&parentContrlEntry, '\0', sizeof(parentContrlEntry));
		pEntry = (void *)&parentContrlEntry;
		parentContrlEntry.parentContrlWeekMon  	= atoi(Mon1);
		parentContrlEntry.parentContrlWeekTues 	= atoi(Tue1);
		parentContrlEntry.parentContrlWeekWed  	= atoi(Wed1);
		parentContrlEntry.parentContrlWeekThur 	= atoi(Thu1);
		parentContrlEntry.parentContrlWeekFri  	= atoi(Fri1);
		parentContrlEntry.parentContrlWeekSat  	= atoi(Sat1);
		parentContrlEntry.parentContrlWeekSun  	= atoi(Sun1);
		parentContrlEntry.parentContrlStartTime = minStart1;
		parentContrlEntry.parentContrlEndTime 	= minEnd1;
		memset(staInfo,0,sizeof(staInfo));
		for(i=0; i<dev_list1.dev_num; i++)
		{
			if (dev_list1.dev_num==1)
			{
				sprintf(tmpInfo, "%s", dev_list1.mac[i]);
			}
			else
			{
				sprintf(tmpInfo, "%s;", dev_list1.mac[i]);
			}
			strcat(staInfo, tmpInfo);
		}
		if(strlen(staInfo)>0)
		{
			strncpy(parentContrlEntry.parentContrlTerminal, staInfo, strlen(staInfo));			
			printf("set parentContrlTerminal = %s\n", parentContrlEntry.parentContrlTerminal);
		}

		if ( !apmib_get(num_id, (void *)&entryNum)) 
		{
			printf("Get entry number error!\n");
			goto retError;
		}
		else
		{
			if ( (entryNum + 1) > MAX_PARENT_CONTRL_TIME_NUM_LIST) 
			{
				printf("parent contrl table full\n");
				goto retError;
			}
		}
		if ( apmib_set(add_id, pEntry) == 0) 
		{
			printf("Add table entry error!\n");
			goto retError;
		}

		apmib_update(CURRENT_SETTING);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	
#ifndef NO_ACTION

	pid = fork();
	if (pid) 
	{
		waitpid(pid, NULL, 0);
	}
	else if (pid == 0) 
	{
		snprintf(tmpBuf, 100, "%s/%s", _CONFIG_SCRIPT_PATH, _FIREWALL_SCRIPT_PROG);
		execl( tmpBuf, _FIREWALL_SCRIPT_PROG, NULL);
		exit(1);
	}
#endif
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_parental_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  ParentalSetings \n");
	cJSON * tmp;
	cJSON * childItem;
	char groupx[32] = {0};
	char value[1024] = {0};
	char buffer[1024] = {0};
	char send_conf_buff[2048] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int x = 0;	
	int z;
	int j;
	int valid_num = 0;
	PARENT_CONCTRL_T group_info;
	
	for (; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "parentalGroupsSet");	
		if (tmp == NULL)
			goto retError;
		char *groupsSet = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "groupsSet = %s, strlen(%d)\n", groupsSet, strlen(groupsSet));


		tmp = cJSON_GetObjectItem(pItem, "groupName");  
		if (tmp == NULL)
			goto retError;
		char *groupName = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER, "groupName = %s\n", groupName);
		strcpy(group_info.name, groupName);

		//add:添加分组/delete:删除分组/modify:修改分组
		if (!strcmp(groupsSet, "add"))
		{
			int add_macs = 0;
			DTRACE(DTRACE_APPSERVER, "=========== add ==== %s\n", groupName);
			apmib_get(MIB_PARENT_CONCTRL_VALID_NUM, (void *)value);
			if (strlen(value)<=0)
				valid_num = 0;
			else if (atoi(value)>6){
				printf("valid group too large!\n");
				goto retError;
			}
			else
				valid_num = atoi(value);
			DTRACE(DTRACE_APPSERVER, "valid_num = %d\n", valid_num);
			
			tmp = cJSON_GetObjectItem(pItem, "networkEnable");  
			if (tmp == NULL)
				goto retError;
			char *networkEnable = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "networkEnable = %s\n", networkEnable);
			
			strcpy(group_info.enable, networkEnable);
			tmp = cJSON_GetObjectItem(pItem, "groupTime");  
			if (tmp == NULL)
				goto retError;
			char *groupTime = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "groupTime = %s\n", groupTime);
			strcpy(group_info.time, groupTime);
			
			cJSON* add_obj = cJSON_GetObjectItem(pItem, "addDev");
			int iarray = cJSON_GetArraySize(add_obj);  
			DTRACE(DTRACE_APPSERVER, "addDev iarray = %d\n",iarray);

			for( j=0; j<iarray; j++ )
			{
				cJSON* mac_array = cJSON_GetArrayItem(add_obj, j); 
				tmp = cJSON_GetObjectItem(mac_array, "mac");  
				if (tmp == NULL)
					goto retError;
				char *mac = tmp->valuestring;
				if (!strlen(mac))
					goto retError;
				strcpy(group_info.mac_list[j], mac);
				add_macs++;
			}
			sprintf(buffer, "%d", add_macs);
			strcpy(group_info.mac_num, buffer);
			z = parent_conctrl_get_null_group_id();
			if (-1 == z)
				goto retError;

			parent_conctrl_add_group(&group_info, z);
			send_return_code(fd, APP_SETTING_SUC);
			return EXIT_SUCCESS;
			
		}
		else if (!strcmp(groupsSet, "delete"))
		{
			parent_conctrl_del_group(groupName);
		}
		else if (!strcmp(groupsSet, "modify"))
		{
			int add_macs = 0;
			int del_macs = 0;
			int x_id = 0;

			tmp = cJSON_GetObjectItem(pItem, "networkEnable");  
			if (tmp == NULL)
				goto retError;
			char *networkEnable = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "networkEnable = %s\n", networkEnable);
			
			strcpy(group_info.enable, networkEnable);
			tmp = cJSON_GetObjectItem(pItem, "groupTime");  
			if (tmp == NULL)
				goto retError;
			char *groupTime = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER, "groupTime = %s\n", groupTime);
			strcpy(group_info.time, groupTime);

			x_id = parent_conctrl_get_id_from_name(groupName);
			if (x_id<0)
				goto retError;
			
			cJSON* del_obj = cJSON_GetObjectItem(pItem, "deleteDev");			
			cJSON* add_obj = cJSON_GetObjectItem(pItem, "addDev");
			
			if (del_obj != NULL )
			{
				int del_array = cJSON_GetArraySize(del_obj);  
				DTRACE(DTRACE_APPSERVER, "delDev iarray = %d\n",del_array);

				//memset(group_info, 0, sizeof(group_info));

				for( j=0; j<del_array; j++ )
				{
					cJSON* mac_array = cJSON_GetArrayItem(del_obj, j); 
					tmp = cJSON_GetObjectItem(mac_array, "mac");  
					if (tmp == NULL)
					{
						break;
					}
					char *mac = tmp->valuestring;
					if (!strlen(mac))
					{
						break;
					}
					strcpy(group_info.mac_list[j], mac);
					//printf("----del group_info.mac_list[j] = %s\n", group_info.mac_list[j]);
					del_macs++;
				}
				sprintf(buffer, "%d", del_macs);
				strcpy(group_info.mac_num, buffer);
				
				parent_conctrl_del_group_mac(&group_info);
			}
			else 
			{
				printf("delete array is null\n");
			}
			
			if (add_obj != NULL )
			{
				//memset(group_info, 0, sizeof(group_info));				
				int add_array = cJSON_GetArraySize(add_obj);  
				DTRACE(DTRACE_APPSERVER, "addDev iarray = %d\n",add_array);
				for( j=0; j<add_array; j++ )
				{
					cJSON* mac_array = cJSON_GetArrayItem(add_obj, j); 
					tmp = cJSON_GetObjectItem(mac_array, "mac");  
					if (tmp == NULL)
						goto retError;
					char *mac = tmp->valuestring;
					if (!strlen(mac))
						goto retError;
					strcpy(group_info.mac_list[j], mac);
					//printf("----add group_info.mac_list[j] = %s\n", group_info.mac_list[j]);
					add_macs++;
				}
				sprintf(buffer, "%d", add_macs);
				strcpy(group_info.mac_num, buffer);

				parent_conctrl_add_group_mac(&group_info);
			}		

			apmib_set(MIB_PARENT_CONCTRL_GROUP0_TIME+x_id, (void *)groupTime);
			apmib_set(MIB_PARENT_CONCTRL_GROUP0_ENABLE+x_id, (void *)networkEnable);
			
			tmp = cJSON_GetObjectItem(pItem, "groupNameChange");  
			if (tmp != NULL)
			{
				char *groupNameChange = tmp->valuestring;
				if(strlen(groupNameChange)>0)
				{
					apmib_set(MIB_PARENT_CONCTRL_GROUP0_NAME+x_id, (void *)groupNameChange);
					DTRACE(DTRACE_APPSERVER, "set MIB_PARENT_CONCTRL_GROUP0_NAME%d = %s\n",x_id, groupNameChange);
				}
				else
				{
					apmib_set(MIB_PARENT_CONCTRL_GROUP0_NAME+x_id, (void *)groupName);
					DTRACE(DTRACE_APPSERVER,"set MIB_PARENT_CONCTRL_GROUP0_NAME%d = %s\n",x_id, groupName);
				}
			}
			else
			{
				apmib_set(MIB_PARENT_CONCTRL_GROUP0_NAME+x_id, (void *)groupName);
				DTRACE(DTRACE_APPSERVER,"set MIB_PARENT_CONCTRL_GROUP0_NAME%d = %s\n",x_id, groupName);
			}
		}
		else 
		{
			goto retError;
		}

		apmib_update(CURRENT_SETTING);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_guest_network(char *guestEnable, char *guestSsid, char *guestPwd, char *guestTimes)
{
	char *submit_url;
	char *str_val = NULL;
	int len;
	int val;
	char tmpBuf[MAX_MSG_BUFFER_SIZE] = {0}; 
	int old_wlan_idx;
	int old_vwlan_idx;

	old_wlan_idx = wlan_idx;
	old_vwlan_idx = vwlan_idx;

	vwlan_idx = 2;
	wlan_idx = 0;
	for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
	{
		if(!strcmp(guestEnable, "off"))
		{
			val = 1;
			if (apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&val) == 0)
			{
				strcpy(tmpBuf, ("<script>dw(wlbasic_set_disable_flag)</script>"));
				goto setErr;
			}
		}
		else
		{
			val = 0;
			if (apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&val) == 0)
			{
				strcpy(tmpBuf, ("<script>dw(wlbasic_set_disable_flag)</script>"));
				goto setErr;
			}
		}
		
		if(strlen(guestSsid))
		{
			if (apmib_set(MIB_WLAN_SSID, (void *)guestSsid) == 0)
			{
				printf("[error] set guest ssid error\n");
				goto setErr;
			}
		}
		
		val = AP_MODE;
		if ( apmib_set( MIB_WLAN_MODE, (void *)&val) == 0) 
		{
			printf("[error] set guest mode error\n");
			goto setErr;
		}
		
		if(strlen(guestPwd))
		{
//			  val = 1;
//			  if ( apmib_set(MIB_WLAN_WPA2_PRE_AUTH, (void *)&val) == 0) 
//			  {
//				strcpy(tmpBuf, ("<script>dw(wl_Set_MIB_WLAN_WPA_CIPHER_SUITE_fai)</script>"));
//				goto setErr;
//			}			

			val = WPA_AUTH_PSK;
			if ( apmib_set(MIB_WLAN_WPA_AUTH, (void *)&val) == 0) 
			{
				printf("[error] set guest auth apa psk error\n");
				goto setErr;
			}
			
			val = ENCRYPT_WPA2;
			if (apmib_set(MIB_WLAN_ENCRYPT, (void *)&val) == 0)
			{
				printf("[error] set guest auth wpa2 error\n");
				goto setErr;
			}
			val = WPA_CIPHER_AES;
			if ( apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&val) == 0) 
			{
				printf("[error] set guest auth aes error\n");
				goto setErr;							
			}

			val = WSC_ENCRYPT_AES;
			apmib_set(MIB_WLAN_WSC_ENC, (void *)&val);
			val = WPA_AUTH_PSK;
			apmib_set(MIB_WLAN_WSC_AUTH, (void *)&val);
			val = WDS_ENCRYPT_DISABLED;
			apmib_set( MIB_WLAN_WDS_ENCRYPT, (void *)&val);
			
			val = 0;	
			if (apmib_set(MIB_WLAN_PSK_FORMAT, (void *)&val) == 0)
			{
				printf("[error] set guest auth psk format error\n");
				goto setErr;
			}
			
			len = strlen(guestPwd);
			if (len == 0 || len > (MAX_PSK_LEN - 1) || len < MIN_PSK_LEN)
			{
				printf("[error] guest auth pwd len error\n");
				goto setErr;
			}
			
			
			if (!apmib_set(MIB_WLAN_WPA_PSK, (void *)guestPwd))
			{
				printf("[error] set MIB_WLAN_WPA_PSK error\n");
				goto setErr;
			}
		}
		else
		{
			val = ENCRYPT_DISABLED;
			if (apmib_set(MIB_WLAN_ENCRYPT, (void *)&val) == 0)
			{
				printf("[error] set MIB_WLAN_ENCRYPT error\n");
				goto setErr;
			}
		}
	}
				
	if(strlen(guestTimes))
	{
		val = atoi(guestTimes);
		if (apmib_set(MIB_GUEST_NEWORK_VALID_TIME, (void *)&val) == 0)
		{
			printf("MIB_GUEST_NEWORK_VALID_TIME set err.\n");
			goto setErr;
		}
	}

	val = 1;
	if (apmib_set(MIB_GUEST_NEWORK_SET_FLG, (void *)&val) == 0)
	{
		printf("MIB_GUEST_NEWORK_SET_FLG set err.\n");
	}
   
	wlan_idx = old_wlan_idx;
	vwlan_idx = old_vwlan_idx;

	return 0;

setErr:
	wlan_idx = old_wlan_idx;
	vwlan_idx = old_vwlan_idx;
	return -1;
}

int set_guestnetwork_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER, "======  GuestNetworkSettings \n");
	int i = 0;	
	int valTmp = 0;
	int repeater_enabled = 0;
	int old_vwlan_idx = 0;
	int old_wlan_idx = 0;
	int idx;
	int disabled;
	int band_no;
	int val;
	char send_conf_buff[5120] = {0};
	char tmpBuf[32] = {0};
	cJSON * tmp;
	char *strVal = "75";
	int iCount = cJSON_GetArraySize(jason_obj);  

	DTRACE(DTRACE_APPSERVER,"iCount = %d\n", iCount);
	for (; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "guestEnable");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *guestEnable = tmp->valuestring;

		tmp = cJSON_GetObjectItem(pItem, "guestSsid");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *guestSsid = tmp->valuestring;

		tmp = cJSON_GetObjectItem(pItem, "guestPwd");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *guestPwd = tmp->valuestring;

		tmp = cJSON_GetObjectItem(pItem, "guestTimes");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *guestTimes = tmp->valuestring;

		DTRACE(DTRACE_APPSERVER,"guestEnable = %s\n", guestEnable);
		DTRACE(DTRACE_APPSERVER,"guestSsid = %s\n", guestSsid);
		DTRACE(DTRACE_APPSERVER,"guestPwd = %s\n", guestPwd);
		DTRACE(DTRACE_APPSERVER,"guestTimes = %s\n", guestTimes);

		if (0 > set_guest_network(guestEnable, guestSsid, guestPwd, guestTimes))
			goto retError;
		
	}  
	apmib_update_web(CURRENT_SETTING);

	send_return_code(fd, APP_SETTING_SUC);
#ifndef NO_ACTION
	app_run_init_script("bridge");
#endif
	return EXIT_SUCCESS;

retError:
	
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;

}


int set_option_conf(int fd, cJSON *jason_obj)
{	
	printf("======  OptionSettings \n");
	cJSON * tmp;
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;
	char send_conf_buff[5120] = {0};
	
	printf("iCount = %d\n", iCount);
	for (; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "assistantEnable");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *assistantEnable = tmp->valuestring;
		
		DTRACE(DTRACE_APPSERVER,"removeDev = %s\n", assistantEnable);
		

		//apmib_update(CURRENT_SETTING);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}


int set_childdev_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  ChildDevSettings \n");
	cJSON * tmp;
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;
	int val = 0;
	char send_conf_buff[5120] = {0};
	
	DTRACE(DTRACE_APPSERVER,"iCount = %d\n", iCount);
	for (; i < iCount; ++i) {  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "location");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *location = tmp->valuestring;
		tmp = cJSON_GetObjectItem(pItem, "LedEnable");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *LedEnable =  tmp->valuestring;
		tmp = cJSON_GetObjectItem(pItem, "IP");
		if (tmp == NULL)
		{
			goto retError;
		}
		char *IP = tmp->valuestring;
		tmp = cJSON_GetObjectItem(pItem, "MAC");
		if (tmp == NULL)
		{
			goto retError;
		}
		char *MAC = tmp->valuestring;
		/*
		tmp = cJSON_GetObjectItem(pItem, "removeDev");
		if (tmp == NULL)
		{
			goto retError;
		}
		char *removeDev = tmp->valuestring;
		*/
		if ( MAC == NULL || IP == NULL )
		{
			goto retError;
		}
		DTRACE(DTRACE_APPSERVER,"333 = %d\n", iCount);
		//apmib_set(MIB_WLAN_WPA_PSK,  (void *)pskValue);

		DTRACE(DTRACE_APPSERVER,"location = %s\n", location);
		if (strlen(location))
			apmib_set(MIB_MAP_DEVICE_NAME, (void *)location);
		DTRACE(DTRACE_APPSERVER,"LedEnable = %s\n", LedEnable);
		DTRACE(DTRACE_APPSERVER,"IP = %s\n", IP);
		DTRACE(DTRACE_APPSERVER,"MAC = %s\n", MAC);
		//DTRACE(DTRACE_APPSERVER,"removeDev = %s\n", removeDev);

		if (!strcmp(LedEnable, "on"))
		{
			val = 1;
			apmib_set(MIB_LED_ENABLE, (void *)&val);
		}
		else 
		{
			val = 0;
			apmib_set(MIB_LED_ENABLE, (void *)&val);
		}

	}  
	apmib_update_web(CURRENT_SETTING);
#ifndef NO_ACTION
	//app_run_init_script("bridge");
#endif
		/*send status flag string to client*/
	send_return_code(fd, APP_SETTING_SUC);
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;

}


int set_wireless_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  WirelessSettings \n");
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;
	int wlanif = 0;
	int intVal = 0;
	char send_conf_buff[5120] = {0};
	char wlanIfStr[64] = {0};
	char pwd_5g[64] = {0};
	char pwd_2g[64] = {0};
	char ssid_5g[64] = {0};
	char ssid_2g[64] = {0};
	char cmd[128] = {0};
	int ori_wlan_idx = wlan_idx;
	cJSON * tmp;
	char *broadcast = NULL;
	DTRACE(DTRACE_APPSERVER,"iCount = %d\n", iCount);
	for (; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "ssid");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *ssid	= tmp->valuestring;
		
		tmp = cJSON_GetObjectItem(pItem, "pskValue");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *pskValue	 = tmp->valuestring;
		
		tmp = cJSON_GetObjectItem(pItem, "broadcast");
		if (tmp == NULL)
		{
			goto retError;
		}
		else
		{
			broadcast = tmp->valuestring;  
		}
		
		DTRACE(DTRACE_APPSERVER,"ssid = %s,pskValue = %s,broadcast = %s\n",ssid,pskValue,broadcast);

		if (ssid == NULL || pskValue == NULL)
		{
			printf("ssid,pskValue,broadcast is null\n");
			goto retError;
		}

		get_2_5g_ssid_pwd(ssid_2g,pwd_2g,ssid_5g,pwd_5g);
		DTRACE(DTRACE_APPSERVER,"ssid_2g=%s,pwd_2g=%s,ssid_5g=%s,pwd_5g=%s\n",ssid_2g,pwd_2g,ssid_5g,pwd_5g);
		if (!strcmp(ssid_2g, ssid) && !strcmp(ssid_5g, ssid) && !strcmp(pwd_2g, pskValue) && !strcmp(pwd_5g, pskValue))
		{
			if (set_hide_ssid(broadcast)<0)
			{
				printf("set hide ssid error!\n");
				goto retError;
			}
			
			send_return_code(fd, APP_SETTING_SUC);
			return EXIT_SUCCESS;
		}
		
		
		wlanif = whichWlanIfIs(PHYBAND_5G);
		if(wlanif >= 0)
		{
			memset(wlanIfStr,0x00,sizeof(wlanIfStr));
			sprintf((char *)wlanIfStr, "wlan%d",wlanif);
			if(SetWlan_idx((char *)wlanIfStr))
			{
				intVal = 0;
				apmib_set(MIB_WLAN_WPA2_PRE_AUTH, (void *)&intVal);
				
				intVal = WPA_AUTH_PSK;
				apmib_set(MIB_WLAN_WPA_AUTH, (void *)&intVal);

				intVal = WPA_CIPHER_MIXED;
				apmib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&intVal);

				intVal = NO_MGMT_FRAME_PROTECTION;
				apmib_set(MIB_WLAN_IEEE80211W, (void *)&intVal);

				intVal = 0;
				apmib_set(MIB_WLAN_SHA256_ENABLE, (void *)&intVal);

				intVal = 0;
				apmib_set(MIB_WLAN_PSK_FORMAT, (void *)&intVal);


				unsigned long reKeyTime = 86400;
				apmib_set(MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&reKeyTime);

				intVal = ENCRYPT_WPA2_MIXED;
				apmib_set(MIB_WLAN_ENCRYPT, (void *)&intVal);

				if ( !apmib_set( MIB_WLAN_SSID,  (void *)ssid) )
				{
					printf("apmib_get MIB_WLAN_SSID error!\n");
					goto retError;
				}
			}
	
			if ( !apmib_set(MIB_WLAN_WPA_PSK,  (void *)pskValue) )
			{
				printf("apmib_get MIB_WLAN_WPA_PSK error!\n");
				goto retError;
			}
			DTRACE(DTRACE_APPSERVER,"5G_ssid pwd = %s\n", pskValue);

			if (broadcast != NULL)
			{
				if (!strcmp(broadcast, "on"))
					intVal = 0;
				else 
					intVal = 1;
				apmib_set(MIB_WLAN_HIDDEN_SSID, (void *)&intVal);
			}

			wlan_idx = ori_wlan_idx;
		}
		wlanif = whichWlanIfIs(PHYBAND_2G);
		if(wlanif >= 0)
		{
			memset(wlanIfStr,0x00,sizeof(wlanIfStr));
			sprintf((char *)wlanIfStr, "wlan%d",wlanif);
			DTRACE(DTRACE_APPSERVER,"2G_ssid wlanIfStr = %s\n", wlanIfStr);
			if(SetWlan_idx((char *)wlanIfStr))
			{
				intVal = 0;
				apmib_set(MIB_WLAN_WPA2_PRE_AUTH, (void *)&intVal);
				
				intVal = WPA_AUTH_PSK;
				apmib_set(MIB_WLAN_WPA_AUTH, (void *)&intVal);

				intVal = WPA_CIPHER_MIXED;
				apmib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&intVal);

				intVal = NO_MGMT_FRAME_PROTECTION;
				apmib_set(MIB_WLAN_IEEE80211W, (void *)&intVal);

				intVal = 0;
				apmib_set(MIB_WLAN_SHA256_ENABLE, (void *)&intVal);

				intVal = 0;
				apmib_set(MIB_WLAN_PSK_FORMAT, (void *)&intVal);


				unsigned long reKeyTime = 86400;
				apmib_set(MIB_WLAN_WPA_GROUP_REKEY_TIME, (void *)&reKeyTime);

				intVal = ENCRYPT_WPA2_MIXED;
				apmib_set(MIB_WLAN_ENCRYPT, (void *)&intVal);
				apmib_set(MIB_WLAN_SSID, (void *)ssid);
				DTRACE(DTRACE_APPSERVER,"2G_ssid ssid = %s\n", ssid);
			}
			
			if ( !apmib_set(MIB_WLAN_WPA_PSK,  (void *)pskValue) )
			{
				printf("apmib_get MIB_FIRST_LOGIN error!\n");
				goto retError;
			}
			DTRACE(DTRACE_APPSERVER,"2G_ssid pwd = %s\n", pskValue);

			if (broadcast != NULL)
			{

				if (!strcmp(broadcast, "on"))
					intVal = 0;
				else 
					intVal = 1;
				apmib_set(MIB_WLAN_HIDDEN_SSID, (void *)&intVal);
			}
			wlan_idx = ori_wlan_idx;
		}

	}  
	/*send status flag string to client*/
	send_return_code(fd, APP_SETTING_SUC);
	apmib_update_web(CURRENT_SETTING);
#ifndef NO_ACTION
	app_run_init_script("bridge");
#endif

	return EXIT_SUCCESS;
retError:
	wlan_idx = ori_wlan_idx;
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_login_conf(int fd, cJSON *jason_obj)
{	
	cJSON * tmp;
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;
	int enabled = 0;
	int langType = 0;
	int valInt = 0;
	int ntp_server_id = 0;
	char buffer[32] = {0};
	char cmd[64] = {0};
	char send_conf_buff[5120] = {0};

	DTRACE(DTRACE_APPSERVER,"======  LoginSettings iCount = %d\n", iCount);

	for (; i < iCount; ++i) 
	{  
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			printf("cJSON_GetArrayItem == null\n");
			continue;  
		}  

		tmp = cJSON_GetObjectItem(pItem, "password");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *password = tmp->valuestring;
		if (strlen(password)<=0)
		{
			goto retError;
		}
		apmib_set(MIB_USER_PASSWORD, (void *)password);

		tmp = cJSON_GetObjectItem(pItem, "timezone");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *timezone = tmp->valuestring;
		if (strlen(timezone)<=0)
		{
			goto retError;
		}
		enabled = 1; 
		apmib_set(MIB_NTP_ENABLED, (void *)&enabled);
		apmib_set(MIB_NTP_TIMEZONE, (void *)timezone);

		system("killall -9 ntp_inet 2>/dev/null");
		
		/* prepare requested info for ntp daemon */
		apmib_get( MIB_NTP_SERVER_ID,  (void *)&ntp_server_id);

		memset(buffer, 0, sizeof(buffer));
		if(ntp_server_id == 0)
			apmib_get( MIB_NTP_SERVER_IP1,  (void *)buffer);
		else if(ntp_server_id == 1)
			apmib_get( MIB_NTP_SERVER_IP2,  (void *)buffer);
		else if(ntp_server_id == 2)
			apmib_get( MIB_NTP_SERVER_IP3,  (void *)buffer);
		else if(ntp_server_id == 3)
			apmib_get( MIB_NTP_SERVER_IP4,  (void *)buffer);
		else if(ntp_server_id == 4)
			apmib_get( MIB_NTP_SERVER_IP5,  (void *)buffer);
		else if(ntp_server_id == 5)
			apmib_get( MIB_NTP_SERVER_IP6,  (void *)buffer);
		if (strlen(buffer)==0)
		{
			sprintf(buffer, "%s", "131.188.3.220");
			valInt = 0;
			apmib_set( MIB_NTP_SERVER_ID,  (void *)&valInt);
			apmib_set( MIB_NTP_SERVER_IP1,  (void *)buffer);
		}
		set_timeZone();
		sprintf(cmd, "ntp_inet -x %s", buffer);
		system(cmd);

		tmp = cJSON_GetObjectItem(pItem, "language");  
		if (tmp == NULL)
		{
			//goto retError;
		}
		else
		{
			char *lang = tmp->valuestring;
			if (!strcmp(lang, "cn"))
			{
				langType = SIMPLE_CHINESE;
			}
			else
			{
				langType = ENGLISH;
			}
			apmib_set(MIB_WEB_LANGUAGE,(void*)&langType);
		}
		
		tmp = cJSON_GetObjectItem(pItem, "wanType");  
		if (tmp == NULL)
		{
			goto retError;
		}

		char *wanType = tmp->valuestring;
		
		int wan_mode = DHCP_DISABLED;
		if (!strcmp(wanType, "autoIp"))
		{
			wan_mode = DHCP_CLIENT;
		}
		else if(!strcmp(wanType, "fixedIp"))
		{
			wan_mode = DHCP_DISABLED;

			tmp = cJSON_GetObjectItem(pItem, "wan_ip");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *wan_ip = tmp->valuestring;
			struct in_addr inIp, inMask, dns1, dns2, inGateway;
			if( !inet_aton(wan_ip, &inIp) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_WAN_IP_ADDR, (void *)&inIp))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "wan_mask");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *wan_mask = tmp->valuestring;
			if( !inet_aton(wan_mask, &inMask) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_WAN_SUBNET_MASK, (void *)&inMask))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "wan_gateway");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *wan_gateway = tmp->valuestring;
			if( !inet_aton(wan_gateway, &inGateway) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_WAN_DEFAULT_GATEWAY, (void *)&inGateway))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "dns1");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *dns_1 = tmp->valuestring;
			if( !inet_aton(dns_1, &dns1) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_DNS1, (void *)&dns1))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
			
			tmp = cJSON_GetObjectItem(pItem, "dns2");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *dns_2 = tmp->valuestring;
			if( !inet_aton(dns_2, &dns2) )
			{
				printf("[%s-%d]inet_aton error!!!\n", __FUNCTION__, __LINE__);
			}
			if( !apmib_set(MIB_DNS2, (void *)&dns2))
			{
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
		}
		else if(!strcmp(wanType, "ppp"))
		{
			wan_mode = PPPOE;

			
			tmp = cJSON_GetObjectItem(pItem, "pppUserName");	
			if (tmp == NULL)
			{
				goto retError;
			}
			char *pppUserName = tmp->valuestring;
			apmib_set(MIB_PPP_USER_NAME, (void *)pppUserName);
			
			tmp = cJSON_GetObjectItem(pItem, "pppPassword");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *pppPassword = tmp->valuestring;
			DTRACE(DTRACE_APPSERVER,"[%s-%d]pppPassword = %s!!!\n", __FUNCTION__, __LINE__, pppPassword);
			apmib_set(MIB_PPP_PASSWORD, (void *)pppPassword);
			
			tmp = cJSON_GetObjectItem(pItem, "pppMtuSize");  
			if (tmp == NULL)
			{
				goto retError;
			}
			char *pppMtuSize = tmp->valuestring;
			int mtuSize = 0;
			mtuSize = strtol(pppMtuSize, (char**)NULL, 10);
			if ( apmib_set(MIB_PPP_MTU_SIZE, (void *)&mtuSize) == 0) {
				printf("[%s-%d]apmib_set error!!!\n", __FUNCTION__, __LINE__);
			}
		}
		else if(!strcmp(wanType, "bridge"))
		{
			wan_mode = DHCP_CLIENT;
		}
		
		apmib_set(MIB_WAN_DHCP, (void *)&wan_mode);

		tmp = cJSON_GetObjectItem(pItem, "firstLogin");  
		if (tmp == NULL)
		{
			goto retError;
		}
		char *firstLogin = tmp->valuestring;
		if (strlen(firstLogin)<=0)
		{
			goto retError;
		}
		valInt = atoi(firstLogin);
		apmib_set(MIB_FIRST_LOGIN, (void*)&valInt);

	}  
	send_return_code(fd, APP_SETTING_SUC);
	apmib_update_web(CURRENT_SETTING);	// update to flash
		
#ifndef NO_ACTION
	app_run_init_script("all");				 
#endif	
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;
}

int set_timezone_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  set_timezone_conf  \n");
	cJSON * tmp;
	char cmd[64] = {0};
	char buffer[64] = {0};
	char send_conf_buff[5120] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;
	int ntp_server_id = 0;
	int enabled = 0;
	int intVal = 0;
	
	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "timeZone");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *timeZone = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"timeZone = %s\n", timeZone);

		apmib_set(MIB_NTP_TIMEZONE, (void *)timeZone);

		enabled = 1; 
		apmib_set(MIB_NTP_ENABLED, (void *)&enabled);

		system("killall -9 ntp_inet 2>/dev/null");
		
		/* prepare requested info for ntp daemon */
		apmib_get( MIB_NTP_SERVER_ID,  (void *)&ntp_server_id);

		memset(buffer, 0, sizeof(buffer));
		if(ntp_server_id == 0)
			apmib_get( MIB_NTP_SERVER_IP1,  (void *)buffer);
		else if(ntp_server_id == 1)
			apmib_get( MIB_NTP_SERVER_IP2,  (void *)buffer);
		else if(ntp_server_id == 2)
			apmib_get( MIB_NTP_SERVER_IP3,  (void *)buffer);
		else if(ntp_server_id == 3)
			apmib_get( MIB_NTP_SERVER_IP4,  (void *)buffer);
		else if(ntp_server_id == 4)
			apmib_get( MIB_NTP_SERVER_IP5,  (void *)buffer);
		else if(ntp_server_id == 5)
			apmib_get( MIB_NTP_SERVER_IP6,  (void *)buffer);

		if (strlen(buffer)==0)
		{
			sprintf(buffer, "%s", "131.188.3.220");
			intVal = 0;
			apmib_set( MIB_NTP_SERVER_ID,  (void *)&intVal);
			apmib_set( MIB_NTP_SERVER_IP1,  (void *)buffer);
		}
		
		set_timeZone();
		sprintf(cmd, "ntp_inet -x %s", buffer);
		printf("ntp start \n", cmd);
		system(cmd);
		
	}  
	send_return_code(fd, APP_SETTING_SUC);

	apmib_update_web(CURRENT_SETTING);
	//Brad modify for system re-init method
#ifndef NO_ACTION
	app_run_init_script("all");
#endif
	
	return EXIT_SUCCESS;
	
retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;

}

int set_app_pwd_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  set_app_pwd_conf  \n");
	cJSON * tmp;
	cJSON * childItem;
	char cmd[32] = "\0";
	char send_conf_buff[5120] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int enabled = 0;
	
	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "password");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *password = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"password = %s\n", password);
		apmib_set(MIB_USER_PASSWORD, (void *)password);
	}  
	send_return_code(fd, APP_SETTING_SUC);

	apmib_update_web(CURRENT_SETTING);
	//Brad modify for system re-init method
#ifndef NO_ACTION
	//app_run_init_script("all");
#endif
	
	return EXIT_SUCCESS;
	
retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;

}

int set_language_conf(int fd, cJSON *jason_obj)
{	
	DTRACE(DTRACE_APPSERVER,"======  set_language_conf  \n");
	cJSON * tmp;
	cJSON * childItem;
	char cmd[32] = "\0";
	char send_conf_buff[5120] = {0};
	int iCount = cJSON_GetArraySize(jason_obj);  
	int i = 0;	
	int langType = 0;
	
	for (; i < iCount; ++i) 
	{
		int intVal;
		cJSON* pItem = cJSON_GetArrayItem(jason_obj, i);  
		if (NULL == pItem)
		{  
			continue;  
		}  
		tmp = cJSON_GetObjectItem(pItem, "language");	
		if (tmp == NULL)
		{
			goto retError;
		}
		char *language = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"language = %s\n", language);
		if (!strcmp(language, "cn"))
		{
			langType = SIMPLE_CHINESE;
		}
		else
		{
			langType = ENGLISH;
		}
		apmib_set(MIB_WEB_LANGUAGE,(void*)&langType);
	}  
	send_return_code(fd, APP_SETTING_SUC);
	apmib_update_web(CURRENT_SETTING);
	
	return EXIT_SUCCESS;
	
retError:
	send_return_code(fd, APP_SETTING_ERR);
	return EXIT_FAILURE;

}

int send_system_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	char childx[32] = "";
	int val = 0;
	int i = 0;

	
	DTRACE(DTRACE_APPSERVER,"SYSTEM_SETTINGS_REQ\n");
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "SystemSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

#if 0
{
	"SystemSettings":[{
		"mainDevVersion":"",			//版本号
		"reboot":"on/off",			//立即重启
		"reset":"on/off",				//恢复出厂设置
		"timerRestartEnable":"on/off",	//定时重启
		"time":""					//定时重启时间
	}]
}


#endif
	memset(buffer, 0x00, sizeof(buffer));
	apmib_get(MIB_DEV_VERSION, (void *) buffer);
	DTRACE(DTRACE_APPSERVER,"MIB_DEV_VERSION =%s\n", buffer);
	cJSON_AddStringToObject(parameters, "mainDevVersion", buffer);
	cJSON_AddStringToObject(parameters, "reboot", "off");
	cJSON_AddStringToObject(parameters, "reset", "off");
	memset(buffer, 0x00, sizeof(buffer));
	apmib_get(MIB_DEV_RESTART_ENABLE, (void *) buffer);
	DTRACE(DTRACE_APPSERVER,"MIB_DEV_RESTART_ENABLE =%s\n", buffer);
	if(strlen(buffer) <= 0)
	{
		sprintf(buffer, "%s", "off");
	}
	cJSON_AddStringToObject(parameters, "timerRestartEnable", buffer);
	memset(buffer, 0x00, sizeof(buffer));
	apmib_get(MIB_DEV_RESTART_TIME, (void *) buffer);
	DTRACE(DTRACE_APPSERVER,"MIB_DEV_RESTART_TIME =%s\n", buffer);
	if(strlen(buffer) <= 0)
	{
		sprintf(buffer, "%s", "00:00");
	}
	cJSON_AddStringToObject(parameters, "time", buffer);

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_env_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[20480];
	char buffer[512] = {0};
	char childx[32] = "";
	int val = 0;
	int intVal;
	int i = 0;
	APP_ENV_INFO_T env_sta_info;
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "EnvironmentGettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	scanning_environment();

	get_wl_environment_tbl(&env_sta_info);

#if 0
{
	"EnvironmentGettings":[{
		"ssidNum":"",
		"ssidX":[{
			"ssid":"",
			"mac":"",
			"signal":"",
			"channel":"",
			"speed":""
		}]
	}]
}

#endif

	sprintf(buffer, "%d", env_sta_info.sta_num);
	cJSON_AddStringToObject(parameters, "ssidNum", buffer);
	
	for(i = 0; i < env_sta_info.sta_num; i++)
	{
		cJSON* array;
		cJSON* arrayobj;
		sprintf(childx, "ssid%d", i+1);
		cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
		cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
		cJSON_AddStringToObject(arrayobj, "ssid", env_sta_info.sta_info[i].ssid);
		cJSON_AddStringToObject(arrayobj, "mac", env_sta_info.sta_info[i].addr);
		sprintf(buffer, "-%d", 100 - env_sta_info.sta_info[i].rssi);
		cJSON_AddStringToObject(arrayobj, "signal", buffer);
		sprintf(buffer, "%d", env_sta_info.sta_info[i].channel);
		cJSON_AddStringToObject(arrayobj, "channel", buffer);
		sprintf(buffer, "%ld", env_sta_info.sta_info[i].bdBrates);
		cJSON_AddStringToObject(arrayobj, "speed", buffer);
	}

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}


int send_upgrade_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	char childx[32] = "";
	int val = 0;
	int intVal;
	int childNum = 0;
	int i = 0;
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "UpgradeSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

#if 0
	{
		"UpgradeSettings":[{
			"mainDevName":"",		
			"mainDevVersion":"",		
			"mainNewVersion":"",
			"mainNewNote":"",		
			"childNum":"",
			"upgrade":"",
			"childX":[{
				"childName":"",
				"childLocalVersion":"",
				"childNewVersion":"",		
				"childNewNote":""		
			}]
		}]
	}
#endif

	MESH_DEV_INFO mesh_json_s;
	memset(&mesh_json_s, 0, sizeof(mesh_json_s));
	get_mesh_json_sta_info(&mesh_json_s);

	DTRACE(DTRACE_APPSERVER,"mesh_json_s.main_info.hostname = %s\n", mesh_json_s.main_info.hostname);
	memset(buffer, 0, sizeof(buffer));
	apmib_get(MIB_MAP_DEVICE_NAME, (void *)buffer);
	cJSON_AddStringToObject(parameters, "mainDevName", buffer);
	cJSON_AddStringToObject(parameters, "url", "");

	sprintf(buffer, "%s", fwVersion );
	cJSON_AddStringToObject(parameters, "mainDevVersion", buffer);
	
	sprintf(buffer, "%s", "off" );
	cJSON_AddStringToObject(parameters, "upgrade", buffer);
	
	sprintf(buffer, "%d", mesh_json_s.child_num );
	cJSON_AddStringToObject(parameters, "childNum", buffer);
	childNum = atoi(buffer);// tmp code
	
	for(i = 0; i < childNum; i++)
	{
		cJSON* array;
		cJSON* arrayobj;
		sprintf(childx, "child%d",i+1);
		cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
		cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
		cJSON_AddStringToObject(arrayobj, "childName",mesh_json_s.main_info.neighbor_info[i].name);
		cJSON_AddStringToObject(arrayobj, "childLocalVersion", fwVersion);
		cJSON_AddStringToObject(arrayobj, "upgrade", "off");
		cJSON_AddStringToObject(arrayobj, "url", "");
	}

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}

	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_urlfilter_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	char childx[32] = "";
	int entryNum = 0;
	int entryEnable = 0;
	int j = 0;
	int blackNum = 0;
	URLFILTER_T urlEntry;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "UrlFilterSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

#if 0
{
"UrlFilterSettings":[{
	"enable":"",		
	"url":"",		
	"method":"",		
	"urlNum":"",
	"urlX":[{
		"url":""
	}]
}]
}



#endif 

	memset(buffer, 0, sizeof(buffer));
	apmib_get(MIB_URLFILTER_ENABLED, (void *)&entryEnable);
	DTRACE(DTRACE_APPSERVER,"[]entryEnable = %d\n", entryEnable);
	if(1 == entryEnable)
	{
		strcpy(buffer, "on");
	}
	else
		strcpy(buffer, "off");

	cJSON_AddStringToObject(parameters, "enable", buffer);
	apmib_get(MIB_URLFILTER_TBL_NUM, (void *)&entryNum);
	DTRACE(DTRACE_APPSERVER," [%s-%d]entryNum = %d\n", __FUNCTION__, __LINE__, entryNum);
	sprintf(buffer, "%d", entryNum);
	cJSON_AddStringToObject(parameters, "method", "");
	cJSON_AddStringToObject(parameters, "url", "");
	
	for(j=1;j<=entryNum;j++)
	{
		cJSON* array;
		cJSON* arrayobj;

		*((char *)&urlEntry) = (char)j;
		if ( apmib_get(MIB_URLFILTER_TBL, (void *)&urlEntry))
		{			
			if( 0 == urlEntry.ruleMode )//0:black rule 1:white rule
			{
				//blackNum = blackNum + 1;
				sprintf(childx, "url%d", ++blackNum);
				
				DTRACE(DTRACE_APPSERVER,"[%s-%d]childx = %s\n", __FUNCTION__, __LINE__, childx);
				cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
				cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
				DTRACE(DTRACE_APPSERVER,"[%s-%d]!!!!!urlEntry.urlAddr = %s\n", __FUNCTION__, __LINE__, urlEntry.urlAddr);
				cJSON_AddStringToObject(arrayobj, "url", urlEntry.urlAddr);
			}
		}
	}
	sprintf(buffer, "%d", blackNum);
	cJSON_AddStringToObject(parameters, "urlNum", buffer);
	
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}

int send_ipfilter_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	char childx[32] = "";
	int entryNum = 0;
	int entryEnable = 0;
	int j = 0;
	IPFILTER_T entry;
	char *ip;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "IpFilterSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

#if 0
{
	"IpFilterSettings":[{
		"enable":"",
		"ip":"",
		"method":"",
		"comment":"",
		"ipNum":"",
		"ipX":[{
			"ip":"",
			"comment":""
		}]
	}]
}


#endif 

	memset(buffer, 0, sizeof(buffer));
	apmib_get(MIB_IPFILTER_ENABLED, (void *)&entryEnable);
	DTRACE(DTRACE_APPSERVER,"[]entryEnable = %d\n", entryEnable);
	if(1 == entryEnable)
	{
		strcpy(buffer, "on");
	}
	else
		strcpy(buffer, "off");

	cJSON_AddStringToObject(parameters, "enable", buffer);
	apmib_get(MIB_IPFILTER_TBL_NUM, (void *)&entryNum);
	DTRACE(DTRACE_APPSERVER," [%s-%d]entryNum = %d\n", __FUNCTION__, __LINE__, entryNum);
	sprintf(buffer, "%d", entryNum);
	cJSON_AddStringToObject(parameters, "ipNum", buffer);
	cJSON_AddStringToObject(parameters, "method", "");
	cJSON_AddStringToObject(parameters, "ip", "");
	cJSON_AddStringToObject(parameters, "comment", "");
	
	for(j=1;j<=entryNum;j++)
	{
		cJSON* array;
		cJSON* arrayobj;
		
		sprintf(childx, "ip%d", j);
		cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
		cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());

		*((char *)&entry) = (char)j;
		if ( apmib_get(MIB_IPFILTER_TBL, (void *)&entry))
		{
			ip = inet_ntoa(*((struct in_addr *)entry.ipAddr));
			DTRACE(DTRACE_APPSERVER," ip%d is %s\n", j, ip);
			cJSON_AddStringToObject(arrayobj, "ip", ip);
			DTRACE(DTRACE_APPSERVER," ip%d comment is %s\n", j, entry.comment);
			cJSON_AddStringToObject(arrayobj, "comment", entry.comment);	
		}
	}

	
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}

int send_macfilter_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	char childx[32] = "";
	int entryNum = 0;
	int entryEnable = 0;
	int j = 0;
	MACFILTER_T macEntry, macEntrytmp;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "MacFilterSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

#if 0
{
"MacFilterSettings":[{
	"enable":"",		//开关
	"mac":"",		
	"method":"",		//add:新增,delete:删除
	"comment":"",		//标识
	"macNum":"",		//mac数量
	"macX":[{
		"mac":"",
		"comment":"",
	}]
}]
}

#endif 

	memset(buffer, 0, sizeof(buffer));
	apmib_get(MIB_MACFILTER_ENABLED, (void *)&entryEnable);
	DTRACE(DTRACE_APPSERVER,"[]entryEnable = %d\n", entryEnable);
	if(1 == entryEnable)
	{
		strcpy(buffer, "on");
	}
	else
		strcpy(buffer, "off");

	cJSON_AddStringToObject(parameters, "enable", buffer);
	apmib_get(MIB_MACFILTER_TBL_NUM, (void *)&entryNum);
	DTRACE(DTRACE_APPSERVER," [%s-%d]entryNum = %d\n", __FUNCTION__, __LINE__, entryNum);
	sprintf(buffer, "%d", entryNum);
	cJSON_AddStringToObject(parameters, "macNum", buffer);
	cJSON_AddStringToObject(parameters, "method", "");
	cJSON_AddStringToObject(parameters, "mac", "");
	cJSON_AddStringToObject(parameters, "comment", "");
	
	for(j=1;j<=entryNum;j++)
	{
		cJSON* array;
		cJSON* arrayobj;
		char macAddr[32]={0};
		
		sprintf(childx, "mac%d", j);
		cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
		cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
		memset(&macEntrytmp, 0x00, sizeof(macEntrytmp));
		*((char *)&macEntrytmp) = (char)j;
		if ( apmib_get(MIB_MACFILTER_TBL, (void *)&macEntrytmp))
		{
			sprintf(macAddr,"%02X:%02X:%02X:%02X:%02X:%02X",
				macEntrytmp.macAddr[0], macEntrytmp.macAddr[1], macEntrytmp.macAddr[2], macEntrytmp.macAddr[3], macEntrytmp.macAddr[4], macEntrytmp.macAddr[5]);
			DTRACE(DTRACE_APPSERVER," mac%d is %s\n", j, macAddr);
			cJSON_AddStringToObject(arrayobj, "mac", macAddr);
			DTRACE(DTRACE_APPSERVER," mac%d is %s\n", j, macEntrytmp.comment);
			cJSON_AddStringToObject(arrayobj, "comment", macEntrytmp.comment);	
		}
	}

	
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}

#if 0
int send_parntal_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[10240];
	char buffer[1024] = {0};
	char groupx[32] = {0};
	char value[1024] = {0};
	int val = 0;
	int groupNum = 0;
	int i = 0;
	int j = 0;
	int z = 0;
	int group_id = 0;
	
	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "ParentalSetings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
	//cJSON_AddStringToObject(parameters, "RadioOn", apmib_get( "RadioOn"));
	//sprintf(buffer, "%d", apmib_get("SSID1"));


#if 0
{
	"ParentalSetings":[{
		"parentalGroupsSet":"add/delete/modify",
		"networkEnable":"",	
		"groupName":"",			
		"groupTime":"",
		"addDev":[{"mac":""}],	
		"deleteDev":[{"mac":""}],
		"groupNum":"",	
		"groupX":[{
			"groupName":"",			
			"networkEnable":"",		
			"groupTime":"",
			"groupDev":[{"mac":""},{"mac":""}]
		}]
	}]
}


#endif
	memset(buffer, 0, sizeof(buffer));
	cJSON_AddStringToObject(parameters, "parentalGroupsSet", "");
	cJSON_AddStringToObject(parameters, "networkEnable", "");
	cJSON_AddStringToObject(parameters, "groupName", "");
	cJSON_AddStringToObject(parameters, "groupTime", "");
	
	cJSON *add_arrays;
	cJSON *add_object,*add_object1;
	cJSON *del_arrays;
	cJSON *del_object;

	cJSON_AddItemToObject(parameters, "addDev", add_arrays = cJSON_CreateArray());
	cJSON_AddItemToArray(add_arrays, add_object = cJSON_CreateObject());
	
	cJSON_AddItemToObject(parameters, "deleteDev", del_arrays = cJSON_CreateArray());
	cJSON_AddItemToArray(del_arrays, del_object = cJSON_CreateObject());

	apmib_get(MIB_PARENT_CONCTRL_VALID_NUM, (void*)value);
	groupNum = atoi(value);
	if(groupNum < 0)
	{
		DTRACE(DTRACE_APPSERVER,"valid groupNum error is %d\n", groupNum);
		goto retError;
	}
	else if(groupNum == 0)
	{
		cJSON_AddStringToObject(parameters, "groupNum", "0");
	}
	else
	{
		cJSON_AddStringToObject(parameters, "groupNum", value);
	}

	DTRACE(DTRACE_APPSERVER,"groupNum is  %d\n", groupNum);
	if (0 != groupNum)
	{
		group_id = 0;
		for (z=0; z < PARENT_CONCTRL_GROUPS_NUMBER; z++)
		{
			apmib_get(MIB_PARENT_CONCTRL_GROUP0_NAME+z, (void*)value);
			DTRACE(DTRACE_APPSERVER,"group%d Name = %s\n", z, value);
			if (strlen(value)<=0)
				continue;
			cJSON* array;
			cJSON* arrayobj;
			cJSON* arraygroup;
			PARENT_CONCTRL_T group_info;
			int groupx_num = 0;

			parent_conctrl_get_groupx_from_id(z, &group_info);
			
			group_id++;
			sprintf(groupx, "group%d", group_id);
			cJSON_AddItemToObject(parameters, groupx, array = cJSON_CreateArray());
			cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
			cJSON_AddStringToObject(arrayobj, "networkEnable", group_info.enable);
			cJSON_AddStringToObject(arrayobj, "groupName", group_info.name);
			cJSON_AddStringToObject(arrayobj, "groupTime", group_info.time);
			cJSON_AddItemToObject(arrayobj, "groupDev", arraygroup = cJSON_CreateArray());
			DTRACE(DTRACE_APPSERVER,"group_info.number = %s\n", group_info.mac_num);
			groupx_num = atoi(group_info.mac_num);

			if (0 == groupx_num)
			{
				cJSON* dev_object;
				cJSON_AddItemToArray(arraygroup, dev_object = cJSON_CreateObject());
			}
			else
			{
				for(j=0; j < groupx_num; j++)
				{
					cJSON* dev_object;
					cJSON_AddItemToArray(arraygroup, dev_object = cJSON_CreateObject());
					cJSON_AddStringToObject(dev_object, "mac", group_info.mac_list[j]);
					//DTRACE(DTRACE_APPSERVER,"group_info.mac_list[%d] = %s\n", j, group_info.mac_list[j]);
				}
			}

		}
	}
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}
#else

enum DEV_TYPE{
	TYPE_HOSTNAME = 0,
	TYPE_MAC
};

int get_dev_type(char *p_dst)
{
	if (strlen(p_dst) != strlen("00:00:00:00:00:00"))
		return TYPE_HOSTNAME;

	return TYPE_MAC;
}

int parental_contrl_get_dev_info(int type, char *p_dst)
{

	if (type == TYPE_MAC)
		;
		

	return 0;
}
int transform_parental_dev(char *p_list, PARENTAL_CONCTROL_DEV_INFO_P parental_dev)
{
	int ret = 0;
	char *p_tmp = 0;
	char buffer[128] = {0};
	int i;
	
	if (strlen(p_list)<=0)
		return -1;

	p_tmp = strstr(p_list, ";");
	if (p_tmp==NULL)
	{
		ret = get_dev_type(p_list); 	
		if (ret == TYPE_MAC)
		{
			strcpy(parental_dev->mac[parental_dev->dev_num], p_list);
			parental_dev->dev_num++;
		}
		else
		{
			strcpy(parental_dev->hostname[parental_dev->dev_num], p_list);
			parental_dev->dev_num++;
		}
	}
	else
	{
		while (p_tmp!=NULL)
		{
			memset(buffer, 0, sizeof(buffer));
			strncpy(buffer, p_list, p_tmp-p_list);
			ret = get_dev_type(buffer); 	
			if (ret == TYPE_MAC)
			{
				strcpy(parental_dev->mac[parental_dev->dev_num], buffer);
				parental_dev->dev_num++;
			}
			else
			{
				strcpy(parental_dev->hostname[parental_dev->dev_num], buffer);
				parental_dev->dev_num++;
	
			}
			p_list=p_tmp+1;
			p_tmp = strstr(p_list, ";");
		}
	}
	
	return 0;
}



/* min to hh:mm */
int transform_min_to_time(unsigned int min, char *p_time)
{
	unsigned int sec = 0;
	if (min<0)
	{
		strcpy(p_time, "00:00");
		return -1;
	}

	memset(p_time, 0, sizeof(p_time));
	
	sec = min%60;
	min = min/60;

	sprintf(p_time, "%02d:%02d", min, sec);
	
	return 0;
}


int send_parntal_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[10240];
	char buffer[1024] = {0};
	char tmpBuf[32] = {0};
	char groupx[32] = {0};
	char value[1024] = {0};
	int val = 0;
	int groupNum = 0;
	int i = 0;
	int j = 0;
	int z = 0;
	int group_id = 0;
	int parentEntryNum = 0;
	PARENT_CONTRL_T entry;
	
	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "ParentalSetings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

#if 0
{
	"ParentalSetings":[{
		"groupNum":"",	
		"groupX":[{
			"networkEnable":"",	
			"Mon":"",
			"Tue":"",
			"Wed":"",
			"Thu":"",
			"Fri":"",
			"Sat":"",
			"Sun":"",
			"timeStart":"",
			"timeEnd":"",
			"groupDev":[{"mac":""},...]
		}]
	}]
}



#endif
	memset(buffer, 0, sizeof(buffer));

	if ( !apmib_get(MIB_PARENT_CONTRL_TBL_NUM, (void *)&parentEntryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
	sprintf(buffer, "%d", parentEntryNum);
	cJSON_AddStringToObject(parameters, "groupNum", buffer);


	DTRACE(DTRACE_APPSERVER,"groupNum is  %d\n", parentEntryNum);
	if (0 != parentEntryNum)
	{
		for (i=1; i<=parentEntryNum; i++) 
		{
			memset(&entry, 0, sizeof(entry));
			*((char *)&entry) = (char)i;
			if ( !apmib_get(MIB_PARENT_CONTRL_TBL, (void *)&entry))
				return -1;

			#if 0
			DTRACE(DTRACE_APPSERVER,"\ntmpMon=%d tmpTues=%d	tmpWed=%d  tmpThur=%d  tmpFri=%d tmpSat=%d	tmpSun=%d  tmpstart=%d	tmpend=%d terminal=%s\n", \
				entry.parentContrlWeekMon,entry.parentContrlWeekTues,entry.parentContrlWeekWed,\
				entry.parentContrlWeekThur,entry.parentContrlWeekFri,entry.parentContrlWeekSat, \
				entry.parentContrlWeekSun, entry.parentContrlStartTime,entry.parentContrlEndTime,entry.parentContrlTerminal);	
			#endif
			cJSON* array;
			cJSON* arrayobj;
			cJSON* arraygroup;

			sprintf(groupx, "group%d", i);
			cJSON_AddItemToObject(parameters, groupx, array = cJSON_CreateArray());
			cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
			cJSON_AddStringToObject(arrayobj, "networkEnable", "on");
  			cJSON_AddStringToObject(arrayobj,"Mon",entry.parentContrlWeekMon?"1":"0");
  			cJSON_AddStringToObject(arrayobj,"Tue",entry.parentContrlWeekTues?"1":"0");
  			cJSON_AddStringToObject(arrayobj,"Wed",entry.parentContrlWeekWed?"1":"0");
  			cJSON_AddStringToObject(arrayobj,"Thu",entry.parentContrlWeekThur?"1":"0");
  			cJSON_AddStringToObject(arrayobj,"Fri",entry.parentContrlWeekFri?"1":"0");
  			cJSON_AddStringToObject(arrayobj,"Sat",entry.parentContrlWeekSat?"1":"0");
  			cJSON_AddStringToObject(arrayobj,"Sun",entry.parentContrlWeekSun?"1":"0");

			PARENTAL_CONCTROL_DEV_INFO  dev_info;
			memset(&dev_info, 0, sizeof(dev_info));
			int j;
			DTRACE(DTRACE_APPSERVER,"entry.parentContrlTerminal = %s\n", entry.parentContrlTerminal);
			transform_parental_dev(entry.parentContrlTerminal, &dev_info);
			
			DTRACE(DTRACE_APPSERVER,"dev_info.dev_num = %d\n", dev_info.dev_num);
			for (j=0; j<dev_info.dev_num; j++)
			{
				DTRACE(DTRACE_APPSERVER,"[show]name = %s mac  = %s\n", dev_info.hostname[j], dev_info.mac[j]);
			}

			transform_min_to_time(entry.parentContrlStartTime, buffer);
			cJSON_AddStringToObject(arrayobj,"timeStart", buffer);
			transform_min_to_time(entry.parentContrlEndTime, buffer);
			cJSON_AddStringToObject(arrayobj,"timeEnd",buffer);	

			cJSON_AddItemToObject(arrayobj, "groupDev", arraygroup = cJSON_CreateArray());
			if (0 == dev_info.dev_num)
			{
				cJSON* dev_object;
				cJSON_AddItemToArray(arraygroup, dev_object = cJSON_CreateObject());
			}
			else
			{
				for(j=0; j < dev_info.dev_num; j++)
				{
					cJSON* dev_object;
					cJSON_AddItemToArray(arraygroup, dev_object = cJSON_CreateObject());
					cJSON_AddStringToObject(dev_object, "mac", dev_info.mac[j]);
					cJSON_AddStringToObject(dev_object, "hostname", dev_info.hostname[j]);
				}
			}

		}
	}
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}

	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}


#endif
int send_guest_network_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	int val = 0;
	int old_vwlan_idx = vwlan_idx;
	int old_wlan_idx = wlan_idx;
	int disabled = 0;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "GuestNetworkSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
	//cJSON_AddStringToObject(parameters, "RadioOn", apmib_get( "RadioOn"));
	//sprintf(buffer, "%d", apmib_get("SSID1"));

	wlan_idx = 0;
	vwlan_idx = 2;			
	apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&disabled);
	if(disabled == 1)
		sprintf(buffer, "%s", "off");
	else
		sprintf(buffer, "%s", "on");
	cJSON_AddStringToObject(parameters, "guestEnable", buffer);
	
	apmib_get( MIB_WLAN_SSID, (void *)buffer);			
	cJSON_AddStringToObject(parameters, "guestSsid", buffer);

	memset(buffer, 0, sizeof(buffer));
	apmib_get(MIB_WLAN_WPA_PSK, (void *)buffer);
	cJSON_AddStringToObject(parameters, "guestPwd", buffer);

	if (!apmib_get(MIB_GUEST_NEWORK_VALID_TIME,  (void *)&val))
	{
		printf("get guest time val error\n");
		goto retError;
	}
	if(val<=0)
		sprintf(buffer, "%d", 0);
	else
		sprintf(buffer, "%d", val);
	
	cJSON_AddStringToObject(parameters, "guestTimes", buffer);
	
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	wlan_idx = old_wlan_idx;
	vwlan_idx = old_vwlan_idx;	
	return EXIT_SUCCESS;

retError:
	wlan_idx = old_wlan_idx;
	vwlan_idx = old_vwlan_idx;	
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_option_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	int val = 0;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "OptionSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
	//cJSON_AddStringToObject(parameters, "RadioOn", apmib_get( "RadioOn"));
	//sprintf(buffer, "%d", apmib_get("SSID1"));

#if 0
	memset(buffer, 0, sizeof(buffer));
	if ( !apmib_get( MIB_WLAN_SSID,  (void *)buffer) )
		return EXIT_FAILURE;
	cJSON_AddStringToObject(parameters, "assistantEnable", "on");
#endif

	memset(buffer, 0, sizeof(buffer));
#if 0	
	if ( !apmib_get( MIB_FIRST_LOGIN, (void *)&buffer) )
	{	
		return EXIT_FAILURE;
	}
#endif
	cJSON_AddStringToObject(parameters, "versionDiscovery", "true");
	
	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}
#if 0
{
	"ChildDevSettings":[{
		"childStatus":"",
		"linkQuality":"",
		"sn":"",
		"location":"",
		"LedEnable":"",
		"IP":"",
		"MAC":"",
		"removeDev":"",
		"devNum":"",
		"devX":[{
			"devModel":"",
			"devLinkTime":"",
			"devDownSpeed":""		
		}]	
	}]
}

#endif 

int send_childdev_conf_to_client(int fd, const char* sourceStr)
{
	char child_mac[32] = {0};
	char *p_tmp = NULL;
	int devNum = 0;
	int online_num;
	APP_WL_INFO_T wl_dev_info;
	MESH_DEV_INFO mesh_json;
    cJSON *mesh_root;
	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);
 
    mesh_root = getMeshTopologyJSON();
    CalcOnlineClientNum(mesh_root, &online_num);

/*
	char *out;
	out = cJSON_Print(mesh_root);
	printf("[send_childdev_conf_to_client] = ****%s***\n-------- strlen(out) = %d\n", out, strlen(out) );
	free(out);
    cJSON_Delete(mesh_root);	
*/
	memset(&mesh_json, 0, sizeof(mesh_json));
	get_mesh_json_sta_info(&mesh_json);

	devNum = get_2_5g_dev_info(&wl_dev_info);
	get_mac_s_link_time(&mesh_json, &wl_dev_info);

	/*  get wired time */
	RTK_LAN_DEVICE_INFO_T devinfo[MAX_STA_NUM] = {0};
	int num = 0;
	rtk_get_lan_device_info(&num, devinfo, MAX_STA_NUM);
	get_sta_link_time(&mesh_json, devinfo, num);

	p_tmp = strstr(sourceStr, "&");
	if (p_tmp != NULL)
	{
		strncpy(child_mac, p_tmp+strlen("&MAC="), MAC_STR_LEN);
		//printf("child_mac = %s\n", child_mac);
	}
	else
		memset(child_mac, 0, sizeof(child_mac));

	send_child_json(fd, &mesh_json, &child_mac);

	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_linkdev_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[10240];
	char buffer[512] = {0};
	char *iface=NULL;
	char childx[32] = {0};
	char txrate[20];
	char *buff = NULL;
	int devNum = 0;
	int i = 0;
	int i_2g = 0;
	int nBytesSent=0, found=0;	
	int rateid=0;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);
#if 0
	{
		"LinkDevSettings":[{
			"devNum":"",			//终端数量
			"devX":[{
				"devModel":"",		//终端型号
				"rssi":"",		//终端型号
				"devMac":"",		//终端MAC地址
				"devMaster":"", 	//终端上级设备
				"devLinkTime":"",	//终端连接时间
				"devDownSpeed":""	//终端下载速率
			}]
		}]
	}


#endif 
	MESH_DEV_INFO mesh_json;
	APP_WL_INFO_T wl_dev_info;
	memset(&mesh_json, 0, sizeof(mesh_json));
	get_mesh_json_sta_info(&mesh_json);
	devNum = get_2_5g_dev_info(&wl_dev_info);
	get_mac_s_link_time(&mesh_json, &wl_dev_info);

	/*  get wired time */
	RTK_LAN_DEVICE_INFO_T devinfo[MAX_STA_NUM] = {0};
	int num = 0;
	rtk_get_lan_device_info(&num, devinfo, MAX_STA_NUM);
	get_sta_link_time(&mesh_json, devinfo, num);
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "LinkDevSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	for (i=0; i<mesh_json.sta_num; i++) 
	{
		cJSON* array;
		cJSON* arrayobj;
		sprintf(childx, "dev%d", i+1);
		cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
		cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());

		cJSON_AddStringToObject(arrayobj, "devModel", mesh_json.sta_info[i].hostname);
		cJSON_AddStringToObject(arrayobj, "devMaster", mesh_json.sta_info[i].master);	
		cJSON_AddStringToObject(arrayobj, "devMac", mesh_json.sta_info[i].mac);	
		memset(buffer, 0, sizeof(buffer));
		cJSON_AddStringToObject(arrayobj, "devLinkTime", mesh_json.sta_info[i].link_time);
		sprintf(buffer, "%sMbps", "8");
		cJSON_AddStringToObject(arrayobj, "devDownSpeed", buffer);//(Mbps)
		sprintf(buffer, "-%d", 100 - atoi(mesh_json.sta_info[i].rssi));
		cJSON_AddStringToObject(arrayobj, "rssi", buffer);
	}

	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%d", mesh_json.sta_num);
	cJSON_AddStringToObject(parameters, "devNum", buffer);//子设备数量

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;
}

int get_mesh_child_info(cJSON *parameters)
{
	char buffer[512] = {0};
	char childx[32]  = {0};
	cJSON* json;
	int j;
	int child_num = 0;
	cJSON * tmp;
	
	FILE *fp = fopen("/tmp/topology_json", "r");
	if (fp == NULL) {
		printf("open topology_json error\n" );
		cJSON_AddStringToObject(parameters, "childNum", "0");
		return -1;
	}
	ssize_t read;
	size_t	len   = 0;
	char*	line  = NULL;
	read = getline(&line, &len, fp);
	if (read <= 0)
	{
		printf("open topology_json error\n" );
		cJSON_AddStringToObject(parameters, "childNum", "0");
		goto retError;
	}
	DTRACE(DTRACE_APPSERVER,"Retrieved line of length %zu :\n", read);
	DTRACE(DTRACE_APPSERVER,"%s\n", line);
	
	json = cJSON_Parse(line);
	if (!json)
	{
		cJSON_AddStringToObject(parameters, "childNum", "0");
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
		goto retError;
	}

	cJSON* add_obj = cJSON_GetObjectItem(json, "child_devices");
	int iarray = cJSON_GetArraySize(add_obj);  
	DTRACE(DTRACE_APPSERVER,"child_devices iarray = %d\n", iarray);
	if ( 0 == iarray )
	{
		cJSON_AddStringToObject(parameters, "childNum", "0");
		return 0;
	}

	for( j=0; j<iarray; j++ )
	{
		cJSON* mac_array = cJSON_GetArrayItem(add_obj, j); 
		tmp = cJSON_GetObjectItem(mac_array, "ip_addr");  
		if (tmp == NULL)
			goto retError;
		char *ip_addr = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"ip_addr = %s\n", ip_addr);

		tmp = cJSON_GetObjectItem(mac_array, "mac_address");  
		if (tmp == NULL)
			goto retError;
		char *mac_address = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"mac_address = %s\n", mac_address);

		tmp = cJSON_GetObjectItem(mac_array, "device_name");  
		if (tmp == NULL)
			goto retError;
		char *device_name = tmp->valuestring;
		DTRACE(DTRACE_APPSERVER,"device_name = %s\n", device_name);

		cJSON* send_array;
		cJSON* send_arrayobj;
		sprintf(childx, "child%d", j+1);
		cJSON_AddItemToObject(parameters, childx, send_array = cJSON_CreateArray());
		cJSON_AddItemToArray(send_array, send_arrayobj = cJSON_CreateObject());
		cJSON_AddStringToObject(send_arrayobj, "childName", device_name);
		cJSON_AddStringToObject(send_arrayobj, "childDevNum", "10");	

		if (str_to_mac(mac_address, buffer)<0)
		{
			strcpy(buffer, "00:00:00:00:00:00");
		}
		cJSON_AddStringToObject(send_arrayobj, "childDevMac", buffer);
		child_num++;
	}

	sprintf(buffer, "%d", child_num);
	cJSON_AddStringToObject(parameters, "childNum", buffer);
	fclose(fp);
	return 0;

retError:
	fclose(fp);
	return -1;
}
int send_home_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[20480];
	char buffer[512] = {0};
	char childx[32] = {0};
	char wlanIfStr[64] = {0};
	int val = 0;
	int i = 0;
	int wlanif = 0;
	int ori_wlan_idx = wlan_idx;
	int devNum = 0;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "HomeSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
	//cJSON_AddStringToObject(parameters, "RadioOn", apmib_get( "RadioOn"));
	//sprintf(buffer, "%d", apmib_get("SSID1"));

	MESH_DEV_INFO mesh_json_s;
	APP_WL_INFO_T wl_dev_info;
	memset(&mesh_json_s, 0, sizeof(mesh_json_s));
	get_mesh_json_sta_info(&mesh_json_s);
	devNum = get_2_5g_dev_info(&wl_dev_info);
	get_mac_s_link_time(&mesh_json_s, &wl_dev_info);

#if 0
{
	"HomeSettings":[{
		"wanUpSpeed":"",			//上传速率
		"wanDownSpeed":"",			//下载速率
		"mainDevSum":"",			//主设备终端数
		"mainDevName":"",			//主设备名称
		"linkStatus":"",			//连接状态：0：正在连接；1：连接正常；
		"childNum":"",				//子设备数量
		"childX":[{
			"childName":"",			//子设备名称
			"childDevNum":"",		//子设备终端数
			"childDevMac":""			//子设备mac地址
		}]
	}]
}
#endif 

	wanRate.ifname="eth1";
	getProcIfData(&wanRate);

	//printf("wanRate rxRate = %.2lf\n", wanRate.rxRate);
	//printf("wanRate txRate = %.2lf\n", wanRate.txRate);
	if (wanRate.txRate < 1000)
		sprintf(buffer, "%.2lfKbps", wanRate.txRate);
	else
		sprintf(buffer, "%.2lfMbps", wanRate.txRate/1000);	
	cJSON_AddStringToObject(parameters, "wanUpSpeed", buffer);

	if (wanRate.rxRate < 1000)
		sprintf(buffer, "%.2lfKbps", wanRate.rxRate);
	else
		sprintf(buffer, "%.2lfMbps", wanRate.rxRate/1000);
	cJSON_AddStringToObject(parameters, "wanDownSpeed", buffer);
	
	cJSON_AddStringToObject(parameters, "linkStatus", "1");

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
				goto retError;
			}
			cJSON_AddStringToObject(parameters, "mainDevSsid", buffer);
		}
		wlan_idx = ori_wlan_idx;
	}

    int online_num = 0;
    cJSON *mesh_root;
 
    mesh_root = getMeshTopologyJSON();
    CalcOnlineClientNum(mesh_root, &online_num);
    cJSON_Delete(mesh_root);	

	DTRACE(DTRACE_APPSERVER,"online_num = %d\n",online_num);
	sprintf(buffer, "%d", online_num);
	cJSON_AddStringToObject(parameters, "devSum", buffer);

	sprintf(buffer, "%d", mesh_json_s.main_info.sta_num);
	cJSON_AddStringToObject(parameters, "mainDevSum", buffer);
	apmib_get(MIB_MAP_DEVICE_NAME, (void *)buffer);
	cJSON_AddStringToObject(parameters, "mainDevName", buffer);

	//get_mesh_child_info(parameters);

	for (i=0; i<mesh_json_s.main_info.neighbor_num; i++)
	{
		cJSON* array;
		cJSON* arrayobj;
		sprintf(childx, "child%d", i+1);
		cJSON_AddItemToObject(parameters, childx, array = cJSON_CreateArray());
		cJSON_AddItemToArray(array, arrayobj = cJSON_CreateObject());
		cJSON_AddStringToObject(arrayobj, "childName", mesh_json_s.main_info.neighbor_info[i].name);
		sprintf(buffer, "%d", mesh_json_s.main_info.neighbor_info[i].sta_num);
		cJSON_AddStringToObject(arrayobj, "childDevNum", buffer);	
		cJSON_AddStringToObject(arrayobj, "childDevMac", mesh_json_s.main_info.neighbor_info[i].mac);	
	}

	sprintf(buffer, "%d", mesh_json_s.main_info.neighbor_num);
	cJSON_AddStringToObject(parameters, "childNum", buffer);

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;
	
retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_wlan_basic_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	char wlanIfStr[64] = {0};
	char ssid[64] = {0};
	char pwd[64] = {0};
	int val = 0;
	int wlanif = 0;
	int ori_wlan_idx = wlan_idx;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]===========\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "WirelessSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
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
				goto retError;
			}
			cJSON_AddStringToObject(parameters, "ssid", buffer);
		}

		if ( !apmib_get(MIB_WLAN_WPA_PSK,  (void *)buffer) )
		{
			DTRACE(DTRACE_APPSERVER,"apmib_get MIB_WLAN_WPA_PSK error!\n");
			goto retError;
		}
		cJSON_AddStringToObject(parameters, "pskValue", buffer);

		if ( !apmib_get( MIB_WLAN_HIDDEN_SSID, (void *)&val) )
		{
			DTRACE(DTRACE_APPSERVER,"apmib_get MIB_WLAN_HIDDEN_SSID error!\n");
			goto retError;
		}
		if (val==1)
			cJSON_AddStringToObject(parameters, "broadcast", "off");
		else
			cJSON_AddStringToObject(parameters, "broadcast", "on");
		
		wlan_idx = ori_wlan_idx;
	}
	wlanif = whichWlanIfIs(PHYBAND_2G);
	if(wlanif >= 0)
	{
		memset(wlanIfStr,0x00,sizeof(wlanIfStr));
		sprintf((char *)wlanIfStr, "wlan%d",wlanif);
		if(SetWlan_idx((char *)wlanIfStr))
		{
			apmib_get(MIB_WLAN_SSID, (void *)ssid);
			DTRACE(DTRACE_APPSERVER,"2G_ssid ssid = %s\n", ssid);
		}
		
		if ( !apmib_get(MIB_WLAN_WPA_PSK,  (void *)buffer) )
		{
			printf("apmib_get MIB_FIRST_LOGIN error!\n");
			goto retError;
		}
		DTRACE(DTRACE_APPSERVER,"2G_ssid pwd = %s\n", buffer);
		wlan_idx = ori_wlan_idx;
	}
	
	generate_jason_strings(send_conf_buff, top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	wlan_idx = ori_wlan_idx;
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}
int send_qos_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	int val = 0;
	int intVal;
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "QosSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	memset(buffer, 0, sizeof(buffer));
	if ( !apmib_get( MIB_WLAN_SSID,  (void *)buffer) )
		return -1;
	
	if ( !apmib_get( MIB_QOS_ENABLED, (void *)&intVal) )
		return -1;
	
	if (1 == intVal)
		sprintf(buffer, "%s", "on" );
	else
		sprintf(buffer, "%s", "off" );
	
	cJSON_AddStringToObject(parameters, "enable", buffer);

	if ( !apmib_get( MIB_QOS_MANUAL_UPLINK_SPEED, (void *)&intVal) )
		return -1;
	sprintf(buffer, "%d", intVal );
	cJSON_AddStringToObject(parameters, "bandwidth", buffer);

	if ( !apmib_get( MIB_QOS_MANUAL_DOWNLINK_SPEED, (void *)&intVal) )
		return -1;
	sprintf(buffer, "%d", intVal );
	cJSON_AddStringToObject(parameters, "bandwidth_downlink", buffer);

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_network_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	DHCP_T dhcp;
	int intval = 0;

	DTRACE(DTRACE_APPSERVER,"[%s-%d]===========\n", __FUNCTION__, __LINE__);

	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "NetworkSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());
	//cJSON_AddStringToObject(parameters, "RadioOn", apmib_get( "RadioOn"));
	//sprintf(buffer, "%d", apmib_get("SSID1"));

/*
{
	"NetworkSettings": [{
		"wanType": "autoIp/fixedIp/ppp/bridge",	//上网设置模式 
		"pppUserName": "",					//PPPoE 账号
		"pppPassword": "",					//PPPoE 密码
		"pppMtuSize": "",					//MTU
		"wan_ip": "",						//IP地址
		"wan_mask": "",						//子网掩码
		"wan_gateway": "",					//默认网关
		"dns1": "",
		"dns2": ""
    }]
}

*/

	memset(buffer, 0, sizeof(buffer));
	if ( !apmib_get(MIB_WAN_DHCP, (void *)&dhcp) )
		return -1;
	if ( DHCP_CLIENT == dhcp )
	{
		sprintf(buffer, "%s", "autoIp");
	}else if( DHCP_DISABLED == dhcp ) {
		sprintf(buffer, "%s", "fixedIp");
	}else if( PPPOE == dhcp ) {
		sprintf(buffer, "%s", "ppp");
	}else if( BRIDGE == dhcp ) {
		sprintf(buffer, "%s", "bridge");
	}	
	cJSON_AddStringToObject(parameters, "wanType", buffer);

	/* pppoe */
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_PPP_USER_NAME,  (void *)buffer);
	cJSON_AddStringToObject(parameters, "pppUserName", buffer);
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_PPP_PASSWORD,  (void *)buffer);
	cJSON_AddStringToObject(parameters, "pppPassword", buffer);
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_PPP_MTU_SIZE,  (void *)&intval);
	sprintf(buffer, "%d", intval);
	cJSON_AddStringToObject(parameters, "pppMtuSize", buffer);

	/* static ip */
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_WAN_IP_ADDR,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "wan_ip", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_WAN_SUBNET_MASK,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "wan_mask", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_WAN_DEFAULT_GATEWAY,  (void *)&buffer);
	if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
		strcpy(buffer, "0.0.0.0");
	cJSON_AddStringToObject(parameters, "wan_gateway", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_DNS1,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "dns1", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_DNS2,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "dns2", inet_ntoa(*((struct in_addr *)buffer)));

	
	generate_jason_strings(send_conf_buff, top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}
int send_login_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	int val = 0;
	int intVal = 0;
	DHCP_T dhcp;
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "LoginSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	apmib_get(MIB_USER_PASSWORD, (void *)buffer);
	cJSON_AddStringToObject(parameters, "password", buffer);

	apmib_get(MIB_FIRST_LOGIN, (void *)&intVal);
	if (intVal==0)
		cJSON_AddStringToObject(parameters, "firstLogin", "0");
	else
		cJSON_AddStringToObject(parameters, "firstLogin", "1");

	apmib_get(MIB_WEB_LANGUAGE,(void*)&intVal);
	if (intVal == SIMPLE_CHINESE)
	{
		sprintf(buffer, "%s", "cn");
	}
	else
	{
		sprintf(buffer, "%s", "en");
	}
	cJSON_AddStringToObject(parameters, "language", buffer);

	apmib_get(MIB_NTP_TIMEZONE, (void *)buffer);
	if (strlen(buffer)<=0)
	{
		strcpy(buffer, "-8 1");
	}
	cJSON_AddStringToObject(parameters, "timeZone", buffer);

	memset(buffer, 0, sizeof(buffer));
	if ( !apmib_get(MIB_WAN_DHCP, (void *)&dhcp) )
		return -1;
	if ( DHCP_CLIENT == dhcp )
	{
		sprintf(buffer, "%s", "autoIp");
	}else if( DHCP_DISABLED == dhcp ) {
		sprintf(buffer, "%s", "fixedIp");
	}else if( PPPOE == dhcp ) {
		sprintf(buffer, "%s", "ppp");
	}else if( BRIDGE == dhcp ) {
		sprintf(buffer, "%s", "bridge");
	}	
	cJSON_AddStringToObject(parameters, "wanType", buffer);

	/* pppoe */
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_PPP_USER_NAME,  (void *)buffer);
	cJSON_AddStringToObject(parameters, "pppUserName", buffer);
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_PPP_PASSWORD,  (void *)buffer);
	cJSON_AddStringToObject(parameters, "pppPassword", buffer);
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_PPP_MTU_SIZE,  (void *)&intVal);
	sprintf(buffer, "%d", intVal);
	cJSON_AddStringToObject(parameters, "pppMtuSize", buffer);

	/* static ip */
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_WAN_IP_ADDR,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "wan_ip", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_WAN_SUBNET_MASK,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "wan_mask", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_WAN_DEFAULT_GATEWAY,  (void *)&buffer);
	if (!memcmp(buffer, "\x0\x0\x0\x0", 4))
		strcpy(buffer, "0.0.0.0");
	cJSON_AddStringToObject(parameters, "wan_gateway", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_DNS1,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "dns1", inet_ntoa(*((struct in_addr *)buffer)));
	memset(buffer, 0, sizeof(buffer));
	apmib_get( MIB_DNS2,  (void *)&buffer);
	cJSON_AddStringToObject(parameters, "dns2", inet_ntoa(*((struct in_addr *)buffer)));

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}
int send_timezone_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "TimeZoneSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	apmib_get(MIB_NTP_TIMEZONE, (void *)buffer);
	if (strlen(buffer)<=0)
		goto retError;
	cJSON_AddStringToObject(parameters, "timeZone", buffer);

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_app_pwd_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "AppPwdSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	apmib_get(MIB_USER_PASSWORD, (void *)buffer);
	cJSON_AddStringToObject(parameters, "password", buffer);

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}

int send_language_conf_to_client(int fd)
{
	cJSON *top_root;
	cJSON *root;
	cJSON *parameters;
	char send_conf_buff[5120];
	char buffer[512] = {0};
	int langType = 0;
	
	memset(&send_conf_buff, 0x00, sizeof(send_conf_buff));	
	top_root = cJSON_CreateObject();
	cJSON_AddItemToObject(top_root, "LanguageSettings", root = cJSON_CreateArray());
	cJSON_AddItemToArray(root, parameters = cJSON_CreateObject());

	apmib_get(MIB_WEB_LANGUAGE,(void*)&langType);
	if (langType == SIMPLE_CHINESE)
	{
		sprintf(buffer, "%s", "cn");
	}
	else
	{
		sprintf(buffer, "%s", "en");
	}

	cJSON_AddStringToObject(parameters, "language", buffer);

	generate_jason_strings(send_conf_buff,top_root);

	/*send related parameters to app client*/
	if( send(fd, send_conf_buff, strlen(send_conf_buff), 0) != strlen(send_conf_buff) ) 
	{
		perror("send internet configuration error"); 
		goto retError;
	}
	return EXIT_SUCCESS;

retError:
	send_return_code(fd, APP_GETTING_ERR);
	return EXIT_FAILURE;

}


/*NOTE: just for test here, need remove once app client can support debug*/
char jason_test[]=
	"POST / HTTP/1.1\r\n"
	"content-Type: application/x-www-form-urlencoded\r\n"
	"Charset: utf-8\r\n"
	"User-Agent: Dalvik/1.6.0 (Linux; U; Android 4.4.2; PE-TL10 Build/HuaweiPE-TL10)\r\n"
	"Host: 192.168.1.1:5000\r\n"
	"Connection: Keep-Alive\r\n"
	"Accept-Encoding: gzip\r\n"
	"Content-Length: 256\r\n"

	"{\"InternetSettings\":\n"
	"[\n"
	"\t {\n"
	"\t \"wan_proto\": \"static\",\n"
	"\t \"wan_ipaddr\": \"192.168.1.111\",\n"
	"\t \"wan_static_netmask\": \"255.255.255.0\",\n"
	"\t \"wan_static_gateway\": \"192.168.1.1\",\n"
	"\t \"wan_primary_dns\":  \"192.168.1.1\",\n"
	"\t \"wan_static_secondary_dns\": \"192.168.1.1\"\n"
	"\t }\n"
	"\t ]"
	"\t}";

/*parse and  save the parameters to settings of router*/
int parse_Set_conf( int fd,char *jasonData)
{
	cJSON *json=NULL;
	cJSON *jason_obj=NULL; 
	char *out=NULL;
	char *wan_proto=NULL;
	char *input_passwd=NULL;
	char output_passwd[64] = {0};
	char cmd[64]={0};
	
	json = cJSON_Parse(jasonData);
	if (!json)
	{
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
		send_return_code(fd, APP_JSON_ERR);
	}
	else
	{
		DTRACE(DTRACE_APPSERVER,"====== parse network connection type  \n");
		if(jason_obj = cJSON_GetObjectItem(json, "WirelessSettings"))		
			set_wireless_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "ChildDevSettings"))		
			set_childdev_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "OptionSettings"))		
			set_option_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "GuestNetworkSettings"))		
			set_guestnetwork_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "ParentalSetings"))		
			set_parental_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "AddParentalSetings"))		
			set_add_parental_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "DeleteParentalSetings"))		
			set_del_parental_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "ModifyParentalSetings"))		
			set_modify_parental_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "DelAllParentalSetings")) 	
			set_del_all_parental_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "NetworkSettings"))		
			set_network_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "QosSettings"))		
			set_qos_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "AddsnSettings"))
			set_add_sn_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "MacFilterSettings"))
			set_macfilter_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "IpFilterSettings"))
			set_ipfilter_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "UrlFilterSettings"))
			set_urlfilter_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "UpgradeSettings"))
			set_upgrade_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "SystemSettings"))
			set_system_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "LoginSettings"))
			set_login_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "TimeZoneSettings"))
			set_timezone_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "AppPwdSettings"))
			set_app_pwd_conf(fd, jason_obj);
		else if(jason_obj = cJSON_GetObjectItem(json, "LanguageSettings"))
			set_language_conf(fd, jason_obj);
		else 
		{
			send_return_code(fd, APP_SETTING_ERR);
			return EXIT_SUCCESS;
		}
		/*send status flag string to client*/
		//send(fd, "SERVER_SET_SYSYEM_TOOLS_CONF_OK", strlen("SERVER_SET_SYSYEM_TOOLS_CONF_OK"), 0) ; 
	}

	//nvram_commit();
	return EXIT_SUCCESS;
}

/*
*handle event from app client,and send message to client when there are some value update from router
*parameters:
*    fd: File descriptor of accept() return 
*/
int app_event_handler(int fd,char *buffer)
{		
	//char http_head[512];
	int app_request_flag = APP_NONE_CON_REQ;
	char tmp_buff[2048] = "\0";
	char jason_data[2048] = "\0";
	char* ptr_jason_data = jason_data;
	int  data_length;
	char *data_begin = NULL;
	char *tmp=NULL;
	char *tmp2=NULL;
	char req_data[128] = {0};
	int i;

	memset(tmp_buff, 0, sizeof(tmp_buff));
	memset(jason_data, 0, sizeof(jason_data));
	memset(status_code, 0, sizeof(status_code));

	/*
	*check if there are any configure request from app client, 
	*if yes,then parse and  save the parameters to settings of router 
	*if no,exit
	*/
	data_begin = strchr(buffer,'{');

	tmp = strstr(buffer,"Content-Length:");
	if ( NULL == tmp )
	{
		send_return_code(fd, APP_HTTP_ERR);
		return EXIT_FAILURE;
	}
	tmp += 15;
	tmp2 = tmp;
	while(*tmp2 != '\r' && *tmp2 != '\n') 
	{
		tmp2++;
	}
	memcpy(tmp_buff,tmp,tmp2-tmp);
	data_length=atoi(tmp_buff);
	//printf("data_length = %d\n", data_length);
	if( 0 == data_length )
	{
		DTRACE(DTRACE_APPSERVER,"data_length == 0\n");
		send_return_code( fd, APP_HTTP_ERR );
		return EXIT_SUCCESS;
	}


	/*	两种情况：
	*	1.消息请求："RequestChildDevSettings&MAC=XX:XX:XX:XX:XX:XX";
	*	2.配置消息：JSON 格式
	*/
	if ( data_begin == NULL )
	{
		data_begin = strchr(buffer, 'R');
		if (data_begin == NULL)
		{
			printf("havn't more client event need addressed\n");
			send_return_code( fd, APP_JSON_ERR );
			return EXIT_SUCCESS;
		}
		else
		{
			/*处理request请求*/
			app_request_flag = get_app_request_type(data_begin);
		}
	}
	else
	{
		data_length = strlen(data_begin);
		DTRACE(DTRACE_APPSERVER,"data_length = %d\n", data_length);

	
		/*处理json设置*/
		memcpy(jason_data, data_begin, data_length);	
		DTRACE(DTRACE_APPSERVER,"[%s-%d]data_strlen = %d\n", __FUNCTION__, __LINE__, strlen(data_begin));
		DTRACE(DTRACE_APPSERVER,"[%s-%d]jason_data = %s\n", __FUNCTION__, __LINE__, jason_data);
		
		/*
		*once get app's data request,reponse related configuration to client pages.
		*/
		app_request_flag = get_target_string_flag(jason_data);
	}

	//printf("[appserver] app_request_flag = %d\n", app_request_flag);
	switch(app_request_flag)
	{
		/*  code */
		case WIRELESS_SETTINGS_REQ:
			send_wlan_basic_conf_to_client(fd);
		break;
		case HOME_SETTINGS_REQ:
			send_home_conf_to_client(fd);
		break;
		case LINK_DEVICE_SETTINGS_REQ:
			send_linkdev_conf_to_client(fd);
		break;
		case CHILD_DEVICE_SETTINGS_REQ:
			send_childdev_conf_to_client(fd, data_begin);
		break;
		case OPTION_SETTINGS_REQ:
			send_option_conf_to_client(fd);
		break;
		case GUEST_NETWORK_SETTINGS_REQ:
			send_guest_network_conf_to_client(fd);
		break;
		case PARENTAL_SETTINGS_REQ:
			send_parntal_conf_to_client(fd);
		break;
		case NETWORK_SETTINGS_REQ:
			send_network_conf_to_client(fd);
		break;
		case MAC_FILTER_REQ:
			send_macfilter_conf_to_client(fd);
		break;
		case IP_FILTER_REQ:
			send_ipfilter_conf_to_client(fd);
		break;
		case URL_FILTER_REQ:
			send_urlfilter_conf_to_client(fd);
		break;
		case QOS_SETTINGS_REQ:
			send_qos_conf_to_client(fd);
		break;
		case UPGRADE_REQ:
			DTRACE(DTRACE_APPSERVER,"UPGRADE_REQ\n");
			send_upgrade_conf_to_client(fd);
		break;
		case ENVIRONMENT_REQ:
			send_env_to_client(fd);
		break;
		case SYSTEM_SETTINGS_REQ:
			send_system_conf_to_client(fd);
		break;
		case LOGIN_SETTINGS_REQ:
			send_login_conf_to_client(fd);
		break;
		case TIME_ZONE_SETTINGS_REQ:
			send_timezone_conf_to_client(fd);
		break;
		case APP_PWD_SETTINGS_REQ:
			send_app_pwd_conf_to_client(fd);
		break;
		case LANG_SETTINGS_REQ:
			send_language_conf_to_client(fd);
		break;
		/*
		*get client configure request ,parse and  save the parameters to router
		*/
		case CONFIGURE_ROUTER_EVENT:
			parse_Set_conf(fd, jason_data);
		break;

		default:
		break;
	}
	/*
	*for reliability check purpose:
	*if checked there are some status_code bit is 1,means client havn't got configuration from server,then send conf again
	*/
	return EXIT_SUCCESS;	
}  

