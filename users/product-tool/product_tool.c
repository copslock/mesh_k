/*********************************************************************************
  *Copyright(C),2019
  *FileName: product_tool.c
  *Author: lhw
  *Version: 
  *Date: 2019-06-20
  *Description: production tool for factory.
  *History:
**********************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>               /* localtime, time */
#include <sys/sysinfo.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <ctype.h>
#include <errno.h>
#include "apmib.h"

//#define DEBUG_PRODUCT_TOOL

#ifdef DEBUG_PRODUCT_TOOL
#define DTRACE(fmt, ...)    printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DTRACE(fmt, ...)
#endif

#define DPrintf(fmt, ...)    printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define RECV_MAX_LEN	1024
#define SEND_MAX_LEN	1024
#define CLI_MAX_ARGV	32

#define LISTEN_PORT		6000

#define USING_APMIB

#define KEY_EXPORT_PATH       "/sys/class/gpio/export"
#define KEY1_VALUE_PATH       "/sys/class/gpio/gpio34/value"
#define KEY2_VALUE_PATH       "/sys/class/gpio/gpio54/value"

#define ROUTER_SET_FLG_FUN_TEST    0
#define ROUTER_SET_FLG_MAC         1
#define ROUTER_SET_FLG_SN          2
#define ROUTER_SET_FLG_AGING       3
#define ROUTER_SET_FLG_PAIRING     4

#define AGING_TIME    (12*60*60)

typedef enum { IP_ADDR, DST_IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;

static char* g_argv_array[CLI_MAX_ARGV];
static char g_mib_mac[8][13];
static char g_mib_sn[24];
static char g_key1_sta[2];
static char g_key2_sta[2];

extern char *fwVersion;


static int _is_hex(char c)
{
    return (((c >= '0') && (c <= '9')) ||
            ((c >= 'A') && (c <= 'F')) ||
            ((c >= 'a') && (c <= 'f')));
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

char** get_argv(const char* string)
{
	char*	p;
	int		n;

	n=0;
	memset(g_argv_array, 0, CLI_MAX_ARGV*sizeof(char *));
	p = (char* )string;
	while(*p)
	{
		g_argv_array[n] = p ;
		while( *p != ' '  &&  *p ) p++ ;
		*p++ = '\0';
		while( *p == ' '  &&  *p ) p++ ;
		n++ ;
		if (n == CLI_MAX_ARGV) break;
	}

	return (char** )&g_argv_array ;
}


int get_argc(const char* string)
{
	int		argc;
	char*	p;

	argc=0;
	p=(char*)string;

	while(*p)
	{
		if( *p != ' '  &&  *p )
		{
			argc++ ;
			while( *p != ' '  &&  *p ) p++;
			continue ;
		}
		p++ ;
	}
	if (argc >= CLI_MAX_ARGV) argc = CLI_MAX_ARGV-1;
	DTRACE("get_argc=%d\n", argc);
	return argc;
}

//去除头尾所有chr字符
void trimchr(char *str, char chr)
{
    char *p;
    int len;

    p = str;
    while(*str == chr)
    {
        while(*p != '\0')
        {
            *p = *(p+1);
            p++;
        }
    }

    len = strlen(str);
    if(len > 0)
    {
        p = str + len - 1;
        while(len--)
        {
            if(*p == chr)
            {
                *p = '\0';
                p--;
            }
            else
            {
                break;
            }
        }
    }
    
}

//从buf里搜素mib的值
//返回：成功返回buf里下一个条目的地址
//      失败返回NULL
char* GetMibValueFromBuf(const char *buf_in, 
                               int len_in,
                               const char *mib_str,  
                               char *buf_out,
                               int len_out)
{
    int id_len;
    char *p;
    int i = 0;
    int j = 0;

    id_len = strlen(mib_str);
    for(i = 0; i < len_in; i++)
    {
        if(mib_str[0] == buf_in[i])
        {
            if(strncmp(buf_in+i, mib_str, id_len) == 0)
            {
                if(buf_in[i+id_len] == '=')
                {
                    p = (char*)(buf_in+i+id_len+1);
                    while((*p != '\n')&& (*p != '\r')&&(*p != '\0'))
                    {
                        if(i+id_len+1+j >= len_in)
                        {
                            DPrintf("err: (i+id_len+1+j = %d) >= (len_in = %d).\n", i+id_len+1+j, len_in);
                            return NULL;
                        }
                        
                        if(j < len_out)
                        {
                            buf_out[j] = *p;
                        }
                        else
                        {
                            DPrintf("err: (j = %d) < (len_out = %d).\n", j, len_out);
                            return NULL;
                        }
                        j++;
                        p++;
                        
                    }

                    if(j < len_out)
                    {
                        buf_out[j] = '\0';
                    }
                    else
                    {
                        DPrintf("err2: (j = %d) < (len_out = %d).\n", j, len_out);
                        return NULL;
                    }
                    
                    trimchr(buf_out, '\"');    //输出带""的去除""

                    while(p + 1 < buf_in+len_in)
                    {
                        p++;
                        if((*p != '\n')&& (*p != '\r')&&(*p != '\0'))
                            break;
                    }
                    
                    return p;
                }
            }
        }
    }

    DPrintf("err: return NULL, i = %d, j = %d .\n", i, j);
    return NULL;
}

#ifdef    USING_APMIB

int ReadMAC()
{
    unsigned char buf[100];
    int old_wlan_idx;
    int old_vwlan_idx;

    memset(buf, 0, sizeof(buf));
    if(apmib_get(MIB_HW_NIC1_ADDR, buf) > 0)
    {  
        memset(&g_mib_mac[0][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[0][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WAN = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    }
    else
    {
        DPrintf("read mac error1.\n");
		return 0;
    }

    if(apmib_get(MIB_HW_NIC0_ADDR, buf) > 0)
    {  
        memset(&g_mib_mac[1][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[1][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("LAN = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    }
    else
    {
        DPrintf("read mac error2.\n");
		return 0;
    }

    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;
    wlan_idx = 0;
    vwlan_idx = 0;
    if(apmib_get(MIB_HW_WLAN_ADDR, buf) > 0)
    {  
        memset(&g_mib_mac[2][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[2][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WLAN0 = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } 
    else
    {
        DPrintf("read mac error3.\n");
        wlan_idx = old_wlan_idx;
        vwlan_idx = old_vwlan_idx;
		return 0;
    }

    if(apmib_get(MIB_HW_WLAN_ADDR1, buf) > 0)
    {  
        memset(&g_mib_mac[3][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[3][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WLAN0-va0 = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } 
    else
    {
        DPrintf("read mac error4.\n");
        wlan_idx = old_wlan_idx;
        vwlan_idx = old_vwlan_idx;
		return 0;
    }

    if(apmib_get(MIB_HW_WLAN_ADDR2, buf) > 0)
    {  
        memset(&g_mib_mac[4][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[4][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WLAN0-va1 = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } 
    else
    {
        DPrintf("read mac error5.\n");
        wlan_idx = old_wlan_idx;
        vwlan_idx = old_vwlan_idx;
		return 0;
    }

    wlan_idx = 1;
    vwlan_idx = 1;
    if(apmib_get(MIB_HW_WLAN_ADDR, buf) > 0)
    {  
        memset(&g_mib_mac[5][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[5][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WLAN1 = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } 
    else
    {
        DPrintf("read mac error6.\n");
        wlan_idx = old_wlan_idx;
        vwlan_idx = old_vwlan_idx;
		return 0;
    }

    if(apmib_get(MIB_HW_WLAN_ADDR1, buf) > 0)
    {  
        memset(&g_mib_mac[6][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[6][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WLAN1-va0 = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } 
    else
    {
        DPrintf("read mac error6.\n");
        wlan_idx = old_wlan_idx;
        vwlan_idx = old_vwlan_idx;
		return 0;
    }

    if(apmib_get(MIB_HW_WLAN_ADDR2, buf) > 0)
    {  
        memset(&g_mib_mac[7][0], 0, sizeof(g_mib_mac[0]));
        sprintf(&g_mib_mac[7][0], "%02X%02X%02X%02X%02X%02X", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

        DPrintf("WLAN1-va1 = %02X%02X%02X%02X%02X%02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    } 
    else
    {
        DPrintf("read mac error7.\n");
        wlan_idx = old_wlan_idx;
        vwlan_idx = old_vwlan_idx;
		return 0;
    }
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    
    return 1;
}

void MACPlusPlus(unsigned char * mac)
{
    mac[5]++;
    if(mac[5] == 0x00)
    {
        mac[4]++;
        if(mac[4] == 0)
        {
            mac[3]++;
        }
    }
}

int SetDefAndCurrApmib(int id, void *value)
{
    if(!apmib_set(id, value))
        return 0;
        
    if(!apmib_setDef(id, value))
        return 0;

    return 1;
}

int SetBackhaulParam(char *mac)
{
    int old_wlan_idx;
    int old_vwlan_idx;
    int mib_val;
    int i;

	
	unsigned int seed = 0;
	int randomData = open("/dev/urandom", O_RDONLY);
	
	if (randomData < 0)
	{
		// something went wrong, use fallback
		seed = time(NULL) +rand();
	}
	else 
	{
		char myRandomData[50];
		ssize_t result = read(randomData, myRandomData, sizeof myRandomData);
	
		if (result < 0)
		{
			// something went wrong, use fallback
			seed = time(NULL) +rand();
		}
	
		for (i = 0; i < 50; i++)
		{
			seed += (unsigned char)myRandomData[i];
	
			if (i % 5 == 0)
			{
				seed = seed * 10;
			}
		}
	}
	
	
	srand(seed);
	char SSIDDic[62]       = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
	char NetworkKeyDic[83] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxy"
	                         "z1234567890~!@#0^&*()_+{}[]:;..?";

	
	char ssid[21], key[31];
	
	strcpy(ssid, "EasyMeshBH-");
	ssid[20] = '\0';
	key[30] = '\0';
	
	// randomly generate SSID post-fix
	
	for (i = 11; i < 14; i++)
	{
		ssid[i] = SSIDDic[rand() % 62];
	}

    strncpy(&ssid[14], &mac[6], 6);
	
	// randomly generate network key
	for (i = 0; i < 30; i++)
	{
		key[i] = NetworkKeyDic[rand() % 83];
	}
	
	
    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;

    vwlan_idx = 1;

    for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
    {
//        mib_val    = 0;
//		if (!apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
//		{
//            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
//            goto SET_ERR;
//		}
//        if (!apmib_setDef(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
//		{
//            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
//            goto SET_ERR;
//		}
        
		mib_val = ENCRYPT_WPA2;
		if (!apmib_set(MIB_WLAN_ENCRYPT, (void *)&mib_val))
		{
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
            goto SET_ERR;
		}
        if (!apmib_setDef(MIB_WLAN_ENCRYPT, (void *)&mib_val))
		{
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
            goto SET_ERR;
		}
		mib_val = WPA_AUTH_PSK;
		if (!apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
		{
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
            goto SET_ERR;
		}
        if (!apmib_setDef(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
		{
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
            goto SET_ERR;
		}

        
        if (!apmib_set(MIB_WLAN_SSID, (void *)ssid)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_SSID\n");
            goto SET_ERR;
        }
        if (!apmib_setDef(MIB_WLAN_SSID, (void *)ssid)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_SSID\n");
            goto SET_ERR;
        }
        
        if (!apmib_set(MIB_WLAN_WPA_PSK, (void *)key)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
            goto SET_ERR;
        }
        if (!apmib_setDef(MIB_WLAN_WPA_PSK, (void *)key)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
            goto SET_ERR;
        }

        mib_val = WPA_CIPHER_AES;
	    if (!apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
	    {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
            goto SET_ERR;
	    }
         if (!apmib_setDef(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
	    {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
            goto SET_ERR;
	    }

	    mib_val = 1;
	    if (!apmib_set(MIB_WLAN_HIDDEN_SSID, (void *)&mib_val))
        {
    		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_HIDDEN_SSID\n");
    		goto SET_ERR;
	    }
        if (!apmib_setDef(MIB_WLAN_HIDDEN_SSID, (void *)&mib_val))
        {
    		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_HIDDEN_SSID\n");
    		goto SET_ERR;
	    }

        mib_val = 0x40; // backhaul value
        if (!apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mib_val))
        {
    		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_MAP_BSS_TYPE\n");
    		goto SET_ERR;
	    }
        if (!apmib_setDef(MIB_WLAN_MAP_BSS_TYPE, (void *)&mib_val))
        {
    		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_MAP_BSS_TYPE\n");
    		goto SET_ERR;
	    }

    }

    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;

    return 1;

SET_ERR:
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    return 0;
  
}

int FactorySet(char *mac)
{
	int i, j;
	int mibVal = 1;
    int old_wlan_idx;
    int old_vwlan_idx;
    char buf[100];

    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;

	for (i = 0; i < 2; i++)
	{
		wlan_idx = i;

		for (j = 0; j < 6; j++)
		{
			vwlan_idx = j;
			apmib_set(MIB_WLAN_DOT11K_ENABLE, (void *) &mibVal);
            apmib_setDef(MIB_WLAN_DOT11K_ENABLE, (void *) &mibVal);
			apmib_set(MIB_WLAN_DOT11V_ENABLE, (void *) &mibVal);
            apmib_setDef(MIB_WLAN_DOT11V_ENABLE, (void *) &mibVal);
		}
	}

    mibVal = DHCP_CLIENT;
	apmib_set(MIB_DHCP, (void *)&mibVal);
    apmib_setDef(MIB_DHCP, (void *)&mibVal);

	mibVal = 480;
	apmib_set(MIB_DHCP_LEASE_TIME, (void *)&mibVal);
    apmib_setDef(MIB_DHCP_LEASE_TIME, (void *)&mibVal);
	// Set to agent
	mibVal = 2;
	apmib_set(MIB_MAP_CONTROLLER, (void *)&mibVal);
    apmib_setDef(MIB_MAP_CONTROLLER, (void *)&mibVal);
	// Enable repeater
	mibVal = 1;
	apmib_set(MIB_REPEATER_ENABLED1, (void *)&mibVal);
    apmib_setDef(MIB_REPEATER_ENABLED1, (void *)&mibVal);
	mibVal = 0;
	apmib_set(MIB_REPEATER_ENABLED2, (void *)&mibVal);
    apmib_setDef(MIB_REPEATER_ENABLED2, (void *)&mibVal);

   
	 wlan_idx  = 0;
	 vwlan_idx = 0;
	 mibVal = 1;
	 apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
     apmib_setDef(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
	 apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);
     apmib_setDef(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);

	 wlan_idx  = 1;
	 vwlan_idx = 0;
	 mibVal = 1;
	 apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
     apmib_setDef(MIB_WLAN_STACTRL_ENABLE, (void *)&mibVal);
	 mibVal = 0;
	 apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);
     apmib_setDef(MIB_WLAN_STACTRL_PREFER, (void *)&mibVal);

	// Enable vxd on 5g, set mode and enable wsc on vxd
	mibVal    = 0;
	wlan_idx  = 0;
	vwlan_idx = 5;
	apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
    apmib_setDef(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
	apmib_set(MIB_WLAN_WSC_DISABLE, (void *)&mibVal);
    apmib_setDef(MIB_WLAN_WSC_DISABLE, (void *)&mibVal);
	mibVal = 1;
	apmib_set(MIB_WLAN_MODE, (void *)&mibVal);
    apmib_setDef(MIB_WLAN_MODE, (void *)&mibVal);
	// Set bss type to 128 for vxd
	mibVal = 0x80;
	apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);
    apmib_setDef(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);

	// mibVal    = 0;
	wlan_idx  = 1;
	vwlan_idx = 5;
	// // Turn on vxd on 2.4g
	// apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mibVal);
	// apmib_set(MIB_WLAN_WSC_DISABLE, (void *)&mibVal);
	// mibVal = 1;
	// apmib_set(MIB_WLAN_MODE, (void *)&mibVal);
	// Set bss type to 0 for vxd
	mibVal = 0x00;
	apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);
    apmib_setDef(MIB_WLAN_MAP_BSS_TYPE, (void *)&mibVal);

	// if different from prev role, reset this mib to 0
	mibVal = 0;
	apmib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);
    apmib_setDef(MIB_MAP_CONFIGURED_BAND, (void *)&mibVal);

    mibVal = 14;
    wlan_idx  = 0;
	vwlan_idx = 0;
	apmib_set(MIB_HW_REG_DOMAIN, (void *)&mibVal);
    
    wlan_idx  = 1;
	vwlan_idx = 0;
    apmib_set(MIB_HW_REG_DOMAIN, (void *)&mibVal);

    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;

    if(SetBackhaulParam(mac) <= 0)
    {
        return 0;
    }

    sprintf(buf, "KS-LINK_WM126");
    if (SetDefAndCurrApmib(MIB_USER_NAME, buf) <= 0)
	{
		DPrintf("set MIB_USER_NAME err.\n");
        return 0;
	}
    sprintf(buf, "kslink123");
    if (SetDefAndCurrApmib(MIB_USER_PASSWORD, buf) <= 0)
	{
		DPrintf("set MIB_USER_PASSWORD err.\n");
        return 0;
	}
    mibVal = 0;
    if (SetDefAndCurrApmib(MIB_FIRST_LOGIN, (void *)&mibVal) <= 0)
	{
		DPrintf("set MIB_FIRST_LOGIN err.\n");
        return 0;
	}
    sprintf(buf, "1:00");
    if (SetDefAndCurrApmib(MIB_DEV_RESTART_TIME, buf) <= 0)
	{
		DPrintf("set MIB_DEV_RESTART_TIME err.\n");
        return 0;
	}
    mibVal = 1;
    if (SetDefAndCurrApmib(MIB_LED_ENABLE, (void *)&mibVal) <= 0)
	{
		DPrintf("set MIB_LED_ENABLE err.\n");
        return 0;
	}
    
    return 1;
}



int SetMAC(char * mac)
{
    unsigned char set_mac[8][6];
    char buf[100];
    int old_wlan_idx;
    int old_vwlan_idx;
    int i;
    int mib_val;

    if(!string_to_hex(mac, &set_mac[0][0], 12))    //WAN mac
    {
        DPrintf("err!!! string_to_hex fail, mac = %s .\n", mac);
		return 0;
    }

    memcpy(&set_mac[1][0], &set_mac[0][0], 6);    
    MACPlusPlus(&set_mac[1][0]);                 //LAN mac
    memcpy(&set_mac[2][0], &set_mac[1][0], 6);    //WLAN0 mac (5G) 
    memcpy(&set_mac[3][0], &set_mac[2][0], 6);
    MACPlusPlus(&set_mac[3][0]);                 //WLAN0-va0 mac (5G)
    memcpy(&set_mac[4][0], &set_mac[3][0], 6);
    MACPlusPlus(&set_mac[4][0]);                 //WLAN0-va1 mac (5G)
    memcpy(&set_mac[5][0], &set_mac[4][0], 6);
    MACPlusPlus(&set_mac[5][0]);                 //WLAN1 mac (2.4G)
    memcpy(&set_mac[6][0], &set_mac[5][0], 6);
    MACPlusPlus(&set_mac[6][0]);                 //WLAN1-va0 mac (2.4G)
    memcpy(&set_mac[7][0], &set_mac[6][0], 6);
    MACPlusPlus(&set_mac[7][0]);                 //WLAN1-va1 mac (2.4G)
    
	if (apmib_set(MIB_HW_NIC1_ADDR, &set_mac[0][0]) <= 0)
	{
		DPrintf("set mac error1.\n");
		return 0;
	}

    if (apmib_set(MIB_HW_NIC0_ADDR, &set_mac[1][0]) <= 0)
	{
		DPrintf("set mac error2.\n");
		return 0;
	}

    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;
    wlan_idx = 0;
    vwlan_idx = 0;
    if (apmib_set(MIB_HW_WLAN_ADDR, &set_mac[2][0]) <= 0)
	{
		DPrintf("set mac error3.\n");
        goto SET_ERR;
	}

    if (apmib_set(MIB_HW_WLAN_ADDR1, &set_mac[3][0]) <= 0)
	{
		DPrintf("set mac error4.\n");
        goto SET_ERR;
	}
    if (apmib_set(MIB_HW_WLAN_ADDR2, &set_mac[4][0]) <= 0)
	{
		DPrintf("set mac error5.\n");
        goto SET_ERR;
	}

    wlan_idx = 1;
    vwlan_idx = 1;
    if (apmib_set(MIB_HW_WLAN_ADDR, &set_mac[5][0]) <= 0)
	{
		DPrintf("set mac error6.\n");
       goto SET_ERR;
	}
    if (apmib_set(MIB_HW_WLAN_ADDR1, &set_mac[6][0]) <= 0)
	{
		DPrintf("set mac error7.\n");
        goto SET_ERR;
	}
    if (apmib_set(MIB_HW_WLAN_ADDR2, &set_mac[7][0]) <= 0)
	{
		DPrintf("set mac error8.\n");
        goto SET_ERR;
	}

    vwlan_idx = 0;
    for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
    {
        sprintf(buf, "KS_%02X%02X%02X", set_mac[0][3], set_mac[0][4], set_mac[0][5]);
        if (apmib_set(MIB_WLAN_SSID, buf) <= 0)
    	{
    		DPrintf("set MIB_WLAN%d_SSID err.\n", wlan_idx);
            goto SET_ERR;
    	}
        if (apmib_setDef(MIB_WLAN_SSID, buf) <= 0)
    	{
    		DPrintf("set def MIB_WLAN%d_SSID err.\n", wlan_idx);
            goto SET_ERR;
    	}

        mib_val    = 0;
        if (!apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
            goto SET_ERR;
        }
        if (!apmib_setDef(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
            goto SET_ERR;
        }
        
        mib_val = ENCRYPT_WPA2;
        if (!apmib_set(MIB_WLAN_ENCRYPT, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
            goto SET_ERR;
        }
        if (!apmib_setDef(MIB_WLAN_ENCRYPT, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
            goto SET_ERR;
        }
        mib_val = WPA_AUTH_PSK;
        if (!apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
            goto SET_ERR;
        }
        if (!apmib_setDef(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
            goto SET_ERR;
        }

        strcpy(buf, "kslink123");
        if (!apmib_set(MIB_WLAN_WPA_PSK, (void *)buf)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
            goto SET_ERR;
        }
        if (!apmib_setDef(MIB_WLAN_WPA_PSK, (void *)buf)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
            goto SET_ERR;
        }

        mib_val = WPA_CIPHER_AES;
        if (!apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
            goto SET_ERR;
        }
         if (!apmib_setDef(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
            goto SET_ERR;
        }
    }
    sprintf(buf, "KS_GUEST_%02X%02X%02X", set_mac[0][3], set_mac[0][4], set_mac[0][5]);
    vwlan_idx = 2;
    for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
    {
        if (apmib_set(MIB_WLAN_SSID, buf) <= 0)
    	{
    		DPrintf("set MIB_WLAN%d_SSID err.\n", wlan_idx);
            goto SET_ERR;
    	}
        if (apmib_setDef(MIB_WLAN_SSID, buf) <= 0)
    	{
    		DPrintf("set def MIB_WLAN%d_SSID err.\n", wlan_idx);
            goto SET_ERR;
    	}
    }
    
    sprintf(buf, "KS_MESH_%02X%02X%02X", set_mac[0][3], set_mac[0][4], set_mac[0][5]);
    if (apmib_set(MIB_MAP_DEVICE_NAME, buf) <= 0)
	{
		DPrintf("set MIB_MAP_DEVICE_NAME err.\n");
        goto SET_ERR;
	}
    if (apmib_setDef(MIB_MAP_DEVICE_NAME, buf) <= 0)
	{
		DPrintf("set def MIB_MAP_DEVICE_NAME err.\n");
        goto SET_ERR;
	}

    mib_val = 1;
    
    if (apmib_set(MIB_AUTO_WLAN_ENABLED, (void *)&mib_val) <= 0)
    {
        DPrintf("set MIB_AUTO_WLAN_ENABLED err.\n");
        goto SET_ERR;
    }
    if (apmib_setDef(MIB_AUTO_WLAN_ENABLED,  (void *)&mib_val) <= 0)
    {
        DPrintf("set def MIB_MAP_DEVICE_NAME err.\n");
        goto SET_ERR;
    }
    
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;

    if(FactorySet(mac) <= 0)
    {
        DPrintf("MeshFactorySet err.\n");
        return 0;
    }
    
    int ret;

    ret = apmib_update(HW_SETTING);
    if(ret <= 0)
    {
        DPrintf("apmib_update HW_SETTING fail, ret = %d, hsHeader.len = %d, customerHwHeader.len = %d\n",
               ret, hsHeader.len, customerHwHeader.len);
		return 0;
    }

    ret = apmib_update(DEFAULT_SETTING);
    if(ret <= 0)
    {
        DPrintf("apmib_update DEFAULT_SETTING fail, ret = %d, hsHeader.len = %d, customerHwHeader.len = %d\n",
               ret, hsHeader.len, customerHwHeader.len);
		return 0;
    }

    ret = apmib_update(CURRENT_SETTING);
    if(ret <= 0)
    {
        DPrintf("apmib_update CURRENT_SETTING fail, ret = %d, hsHeader.len = %d, customerHwHeader.len = %d\n",
               ret, hsHeader.len, customerHwHeader.len);
		return 0;
    }
    
    //读取出来验证
    ReadMAC();

    for(i = 0; i < 8; i++)
    {  
        sprintf(buf, "%02X%02X%02X%02X%02X%02X", set_mac[i][0], set_mac[i][1], set_mac[i][2],
                    set_mac[i][3], set_mac[i][4], set_mac[i][5]);
        if(strcmp(buf, &g_mib_mac[i][0]) != 0)
        {
            DPrintf("mac cmp err%d, buf = %s, mac = %s .\n", i, buf, &g_mib_mac[i][0]);
            return 0;
        }
    }
   
    return 1;

SET_ERR:
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    return 0;
    
}

int ReadSN()
{
    if(apmib_get(MIB_CUSTOMER_HW_SERIAL_NUM, g_mib_sn) > 0)
    {
        DPrintf("SN = %s\n", g_mib_sn);
        return 1;
    }
    else
    {
        DPrintf("read sn error.\n");
		return 0;
    }
}




int SetSN(char * sn)
{
	if (apmib_set(MIB_CUSTOMER_HW_SERIAL_NUM, sn) > 0)
	{
        if(apmib_update(CUSTOMER_HW_SETTING) <= 0)
        {
            DPrintf("apmib_update fail\n");
			return 0;
        }
		ReadSN();

		if (strcasecmp(sn, g_mib_sn) == 0)
		{
			return 1;
		}
		else 
		{
			DPrintf("set sn fail, sn = %s, sn_result = %s\n", sn, g_mib_sn);
			return 0;
		}

		return 1;
	}
	else 
	{
		DPrintf("set sn error.\n");
		return 0;
	}
}

//int SetMibSetFlg()
//{
//    unsigned char flg = 1;
//    
//    if (apmib_set(MIB_CUSTOMER_HW_SET_FLG, &flg) > 0)
//	{
//        if(apmib_update(CUSTOMER_HW_SETTING) <= 0)
//        {
//            DPrintf("apmib_update fail.\n");
//    		return 0;
//        }
//	}
//	else 
//	{
//		DPrintf("apmib_set fail.\n");
//		return 0;
//	}
//
//    return 1;
//}
#else

int ReadMAC()
{
    char cmd_buf[200];
    FILE *f_mac;
    char *p;
    
    sprintf(cmd_buf, "flash get HW_NIC0_ADDR > /tmp/mac");
    system(cmd_buf);
    sprintf(cmd_buf, "flash get HW_NIC1_ADDR >> /tmp/mac");
    system(cmd_buf);
    sprintf(cmd_buf, "flash get HW_WLAN0_WLAN_ADDR >> /tmp/mac");
    system(cmd_buf);
    sprintf(cmd_buf, "flash get HW_WLAN1_WLAN_ADDR >> /tmp/mac");
    system(cmd_buf);
    f_mac = fopen("/tmp/mac", "r");
    if(f_mac == NULL)
    {
        DPrintf("read mac error.\n");
		return 0;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    fread(cmd_buf, 1, sizeof(cmd_buf), f_mac);

	
	
	
	p = cmd_buf;
	
	p = GetMibValueFromBuf(p, (sizeof(cmd_buf) - (p - cmd_buf)), "HW_NIC0_ADDR", &g_mib_mac[0][0], sizeof(g_mib_mac[0]));
	
	if (p == NULL)
	{
		DPrintf("GetMibValueFromBuf error1.\n");
		return 0;
	}
	
	
	p = GetMibValueFromBuf(p, (sizeof(cmd_buf) - (p - cmd_buf)), "HW_NIC1_ADDR", &g_mib_mac[1][0], sizeof(g_mib_mac[0]));
	
	if (p == NULL)
	{
		DPrintf("GetMibValueFromBuf error2.\n");
		return 0;
	}
	
	
	p = GetMibValueFromBuf(p, (sizeof(cmd_buf) - (p - cmd_buf)), "HW_WLAN0_WLAN_ADDR", &g_mib_mac[2][0],
		 sizeof(g_mib_mac[0]));
	
	if (p == NULL)
	{
		DPrintf("GetMibValueFromBuf error3.\n");
		return 0;
	}
	
	
	p = GetMibValueFromBuf(p, (sizeof(cmd_buf) - (p - cmd_buf)), "HW_WLAN1_WLAN_ADDR", &g_mib_mac[3][0],
		 sizeof(g_mib_mac[0]));
	
	if (p == NULL)
	{
		DPrintf("GetMibValueFromBuf error4.\n");
		return 0;
	}
	
	

    fclose(f_mac);

    return 1;
}

int SetMAC(char *mac)
{	
    char cmd_buf[100];

    sprintf(cmd_buf, "flash set HW_NIC0_ADDR %s", mac);
	system(cmd_buf);
    sprintf(cmd_buf, "flash set HW_NIC1_ADDR %s", mac);
	system(cmd_buf);
    sprintf(cmd_buf, "flash set HW_WLAN0_WLAN_ADDR %s", mac);
	system(cmd_buf);
    sprintf(cmd_buf, "flash set HW_WLAN1_WLAN_ADDR %s", mac);
	system(cmd_buf);

    ReadMAC();
    if(strcasecmp(mac, &g_mib_mac[0][0]) == 0)
    {
        return 1;
    }
    else
    {
        DPrintf("err: set mac fail, mac = %s, mac_result = %s\n", mac, &g_mib_mac[0][0]);
        return 0;
    }
}

int ReadSN()
{
    char cmd_buf[100];
    FILE *f_sn;
    
    sprintf(cmd_buf, "flash get CUSTOMER_HW_SERIAL_NUM > /tmp/sn");
    system(cmd_buf);
    f_sn = fopen("/tmp/sn", "r");
    if(f_sn == NULL)
    {
        DPrintf("read sn error.\n");
		return 0;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    fread(cmd_buf, 1, sizeof(cmd_buf), f_sn);
    if(GetMibValueFromBuf(cmd_buf, sizeof(cmd_buf), "CUSTOMER_HW_SERIAL_NUM", g_mib_sn, sizeof(g_mib_sn)) == NULL)
    {
        DPrintf("GetMibValueFromBuf error.\n");
		return 0;
    }

    fclose(f_sn);

    return 1;
}

int SetSN(char *sn)
{
    char cmd_buf[100];
    
    sprintf(cmd_buf, "flash set CUSTOMER_HW_SERIAL_NUM %s",sn);
    system(cmd_buf);

    ReadSN();
    if(strcasecmp(sn, g_mib_sn) == 0)
    {
        return 1;
    }
    else
    {
        DPrintf("err: set sn fail, sn = %s, sn_result = %s\n", sn, g_mib_sn);
        return 0;
    }
}

//int SetMibSetFlg()
//{
//    char cmd_buf[100];
//    sprintf(cmd_buf, "flash set CUSTOMER_HW_SET_FLG 1");
//    system(cmd_buf);
//
//    return 1;
//}

#endif

int SetMibSetFlg(int bit_flg, int sta)
{
    unsigned int flg = 0;
    
    if(apmib_get(MIB_CUSTOMER_HW_SET_FLG, &flg)  <= 0)
    {
        DPrintf("get mib MIB_CUSTOMER_HW_SET_FLG fail\n");
        return 0;
    }

    if(sta == 1)
    {
        flg = (flg | (1<<bit_flg));
    }
    else
    {
        flg = (flg & (~(1<<bit_flg)));
    }
    if (apmib_set(MIB_CUSTOMER_HW_SET_FLG, &flg) > 0)
	{
        if(apmib_update(CUSTOMER_HW_SETTING) <= 0)
        {
            DPrintf("apmib_update fail.\n");
    		return 0;
        }
	}
	else 
	{
		DPrintf("set mib MIB_CUSTOMER_HW_SET_FLG fail.\n");
		return 0;
	}

    return 1;
}

int IsDefaultMACOrSN()
{
    if(strcasecmp(&g_mib_mac[0][0], "00e04c8196c9") == 0)
        return 1;
    if(strcasecmp(&g_mib_mac[1][0], "00e04c8196c1") == 0)
        return 1;
    if(strcasecmp(&g_mib_mac[2][0], "00e04c8196c1") == 0)
        return 1;
    if(strcasecmp(&g_mib_mac[3][0], "00e04c8196d1") == 0)
        return 1;

    if(strcasecmp(g_mib_sn, "000000000000000000000") == 0)
        return 1;

    return 0;
}

static int GetInAddr( char *interface, ADDR_T type, void *pAddr )
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

int SetToController()
{
    int old_wlan_idx;
    int old_vwlan_idx;
    int i,j;
    int mib_val;
    char key[MAX_PSK_LEN];
//    char buf[100];
    
    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;

    mib_val = 1;
    for (i = 0; i < 2; i++)
	{
		wlan_idx = i;

		for (j = 0; j < 6; j++)
		{
			vwlan_idx = j;
			apmib_set(MIB_WLAN_DOT11K_ENABLE, (void *) &mib_val);
			apmib_set(MIB_WLAN_DOT11V_ENABLE, (void *) &mib_val);
		}
	}

    mib_val = DHCP_SERVER;
	apmib_set(MIB_DHCP, (void *)&mib_val);

	mib_val = 480;
	apmib_set(MIB_DHCP_LEASE_TIME, (void *)&mib_val);
    // Set to controller
	mib_val = 1;
	apmib_set(MIB_MAP_CONTROLLER, (void *)&mib_val);
	apmib_get(MIB_OP_MODE, (void *)&mib_val);
	if(WISP_MODE != mib_val) {
		// Disable repeater
		mib_val = 0;
		apmib_set(MIB_REPEATER_ENABLED1, (void *)&mib_val);
		apmib_set(MIB_REPEATER_ENABLED2, (void *)&mib_val);
		// Disable vxd
		mib_val    = 1;
		wlan_idx  = 0;
		vwlan_idx = 5;
		apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val);
		wlan_idx  = 1;
		vwlan_idx = 5;
		apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val);
	}

	// if different from prev role, reset this mib to 0
	mib_val = 0;
	apmib_set(MIB_MAP_CONFIGURED_BAND, (void *)&mib_val);


	// enable va0 on both wlan0 and wlan1
	mib_val    = 0;
	wlan_idx  = 0;
	vwlan_idx = 1;
	apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val);
	mib_val = ENCRYPT_WPA2;
	apmib_set(MIB_WLAN_ENCRYPT, (void *)&mib_val);
	mib_val = WPA_AUTH_PSK;
	apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mib_val);

	mib_val    = 0;
	wlan_idx  = 1;
	vwlan_idx = 1;
	apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val);
	mib_val = ENCRYPT_WPA2;
	apmib_set(MIB_WLAN_ENCRYPT, (void *)&mib_val);
	mib_val = WPA_AUTH_PSK;
	apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mib_val);

	mib_val = 0x20; // fronthaul value
	int val;
	for (i = 0; i < 2; i++) 
   {
		for (j = 0; j < 5; j++) 
        {
			wlan_idx  = i;
			vwlan_idx = j;
			if (!apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&val))
				goto SET_ERR;
			if (val == 0) // only set to fronthaul if this interface is enabled
				apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mib_val);
		}
	}

	wlan_idx  = 0;
	vwlan_idx = 0;
	mib_val = 1;
	apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mib_val);
	apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mib_val);

	wlan_idx  = 1;
	vwlan_idx = 0;
	mib_val = 1;
	apmib_set(MIB_WLAN_STACTRL_ENABLE, (void *)&mib_val);
	mib_val = 0;
	apmib_set(MIB_WLAN_STACTRL_PREFER, (void *)&mib_val);

    vwlan_idx = 1;
    for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
    {
        if(!apmib_get(MIB_WLAN_WPA_PSK, (void *) key))
        {
            DPrintf("[Error] : Failed to get AP mib MIB_WLAN_WPA_PSK\n");
			goto SET_ERR;
        }
		if (!apmib_set(MIB_WLAN_WSC_PSK, (void *) key))
		{
			DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
			goto SET_ERR;
		}
		
		
		mib_val = WSC_AUTH_WPA2PSK;
		apmib_set(MIB_WLAN_WSC_AUTH, (void *) &mib_val);
		mib_val = WSC_ENCRYPT_AES;
		apmib_set(MIB_WLAN_WSC_ENC, (void *) &mib_val);
		mib_val = 1;
		apmib_set(MIB_WLAN_WSC_CONFIGURED, (void *) &mib_val);
		mib_val = WPA_CIPHER_AES;
		apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *) &mib_val);
		
		mib_val = 1;	
		if (!apmib_set(MIB_WLAN_HIDDEN_SSID, (void *) &mib_val))
		{
			DPrintf("[Error] : Failed to set AP mib MIB_WLAN_HIDDEN_SSID\n");
			goto SET_ERR;
		}
		
        mib_val = 0x40; // backhaul value
        apmib_set(MIB_WLAN_MAP_BSS_TYPE, (void *)&mib_val);
    }

//    vwlan_idx = 0;
//    for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
//    {
//       mib_val    = 0;
//       if (!apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
//       {
//           DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
//           goto SET_ERR;
//       }
//      
//       mib_val = ENCRYPT_WPA2;
//       if (!apmib_set(MIB_WLAN_ENCRYPT, (void *)&mib_val))
//       {
//           DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
//           goto SET_ERR;
//       }
//       
//       mib_val = WPA_AUTH_PSK;
//       if (!apmib_set(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
//       {
//           DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
//           goto SET_ERR;
//       }
//       
//       strcpy(buf, "kslink123");
//       if (!apmib_set(MIB_WLAN_WPA_PSK, (void *)buf)) 
//       {
//           DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
//           goto SET_ERR;
//       }
//       
//
//       mib_val = WPA_CIPHER_AES;
//       if (!apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
//       {
//           DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
//           goto SET_ERR;
//       }
//        
//    }
    
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    return 1;

SET_ERR:
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    return 0;

}

//mode 0-default is agent  1-set to controller 
int SetMeshParam(char *ssid, char *key, int mode)
{
    int old_wlan_idx;
    int old_vwlan_idx;
    int mib_val;
    
    if(strlen(ssid) >= MAX_SSID_LEN || strlen(key) >= MAX_PSK_LEN)
        return 0;

    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;

    mib_val = 0x20; // fronthaul value
	int val,i,j;
	
	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 5; j++)
		{
			wlan_idx = i;
			vwlan_idx = j;
	
			if (!apmib_get(MIB_WLAN_WLAN_DISABLED, (void *) &val))
				return - 1;
	
			if (val == 0) // only set to fronthaul if this interface is enabled
				SetDefAndCurrApmib(MIB_WLAN_MAP_BSS_TYPE, (void *) &mib_val);
		}
	}

    //set backhaul AP (wlan0-va0)
    vwlan_idx = 1;

    for(wlan_idx = 0; wlan_idx < 2; wlan_idx++)
    {
//        mib_val    = 0;
//		if (!SetDefAndCurrApmib(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
//		{
//            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
//            goto SET_ERR;
//		}
		mib_val = ENCRYPT_WPA2;
		if (!SetDefAndCurrApmib(MIB_WLAN_ENCRYPT, (void *)&mib_val))
		{
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
            goto SET_ERR;
		}
		mib_val = WPA_AUTH_PSK;
		if (!SetDefAndCurrApmib(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
		{
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
            goto SET_ERR;
		}

        
        if (!SetDefAndCurrApmib(MIB_WLAN_SSID, (void *)ssid)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_SSID\n");
            goto SET_ERR;
        }
        
        if (!SetDefAndCurrApmib(MIB_WLAN_WPA_PSK, (void *)key)) 
        {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
            goto SET_ERR;
        }

        mib_val = WPA_CIPHER_AES;
	    if (!SetDefAndCurrApmib(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
	    {
            DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
            goto SET_ERR;
	    }

	    mib_val = 1;
	    if (!SetDefAndCurrApmib(MIB_WLAN_HIDDEN_SSID, (void *)&mib_val))
        {
    		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_HIDDEN_SSID\n");
    		goto SET_ERR;
	    }

        mib_val = 0x40; // backhaul value
        if (!SetDefAndCurrApmib(MIB_WLAN_MAP_BSS_TYPE, (void *)&mib_val))
        {
    		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_MAP_BSS_TYPE\n");
    		goto SET_ERR;
	    }

    }

    wlan_idx  = 1;
	vwlan_idx = 5;
	mib_val = 0x80;
	SetDefAndCurrApmib(MIB_WLAN_MAP_BSS_TYPE, (void *)&mib_val);

    //set backhaul STA (wlan0-vxd)
    wlan_idx  = 0;
	vwlan_idx = 5;
    
    mib_val    = 0;
    if (!SetDefAndCurrApmib(MIB_WLAN_WLAN_DISABLED, (void *)&mib_val))
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WLAN_DISABLED\n");
        goto SET_ERR;
    }
    mib_val = ENCRYPT_WPA2;
    if (!SetDefAndCurrApmib(MIB_WLAN_ENCRYPT, (void *)&mib_val))
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_ENCRYPT\n");
        goto SET_ERR;
    }
    mib_val = WPA_AUTH_PSK;
    if (!SetDefAndCurrApmib(MIB_WLAN_WPA_AUTH, (void *)&mib_val))
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_AUTH\n");
        goto SET_ERR;
    }    
    if (!SetDefAndCurrApmib(MIB_WLAN_SSID, (void *)ssid)) 
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_SSID\n");
        goto SET_ERR;
    }  
    if (!SetDefAndCurrApmib(MIB_WLAN_WPA_PSK, (void *)key)) 
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
        goto SET_ERR;
    }
    mib_val = WPA_CIPHER_AES;
    if (!SetDefAndCurrApmib(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&mib_val))
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
        goto SET_ERR;
    }
    mib_val = 0;
    if (!SetDefAndCurrApmib(MIB_WLAN_WSC_DISABLE, (void *)&mib_val))
    {
        DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA2_CIPHER_SUITE\n");
        goto SET_ERR;
    }
//    
//    if (!SetDefAndCurrApmib(MIB_WLAN_WSC_SSID, (void *) ssid))
//	{
//		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
//		goto SET_ERR;
//	}
//    if (!SetDefAndCurrApmib(MIB_WLAN_WSC_PSK, (void *) key))
//	{
//		DPrintf("[Error] : Failed to set AP mib MIB_WLAN_WPA_PSK\n");
//		goto SET_ERR;
//	}			
//	mib_val = WSC_AUTH_WPA2PSK;
//	SetDefAndCurrApmib(MIB_WLAN_WSC_AUTH, (void *) &mib_val);
//	mib_val = WSC_ENCRYPT_AES;
//	SetDefAndCurrApmib(MIB_WLAN_WSC_ENC, (void *) &mib_val);
//	mib_val = 1;
//	SetDefAndCurrApmib(MIB_WLAN_WSC_CONFIGURED, (void *) &mib_val);
//	mib_val = WPA_CIPHER_AES;
//	SetDefAndCurrApmib(MIB_WLAN_WPA2_CIPHER_SUITE, (void *) &mib_val);

    if(mode == 1)
    {
        if(SetToController()<=0)
        {
            DPrintf("[Error] : Failed SetToController\n");
            goto SET_ERR;
        }
    }
        
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;

    
    if(apmib_update(CURRENT_SETTING) <= 0)
    {
        DPrintf("apmib_update CURRENT_SETTING fail");
		goto SET_ERR;
    }
    if(apmib_update(DEFAULT_SETTING) <= 0)
    {
        DPrintf("apmib_update DEFAULT_SETTING fail");
		goto SET_ERR;
    }
    
    return 1;

SET_ERR:
    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    return 0;
  
}

void TimerCheckAging()
{
    unsigned long sec;
    struct sysinfo info;
    unsigned int flg = 0;
    
    sysinfo(&info);
    sec = (unsigned long) info.uptime ;
    if(sec > AGING_TIME)
    {
        if(apmib_get(MIB_CUSTOMER_HW_SET_FLG, &flg)  > 0)
        {
            if(!(flg & (1 << ROUTER_SET_FLG_AGING)))
            {
                flg = (flg | (1<<ROUTER_SET_FLG_AGING));
                DTRACE("set mib MIB_CUSTOMER_HW_SET_FLG.\n");
                if (apmib_set(MIB_CUSTOMER_HW_SET_FLG, &flg) > 0)
            	{
                    if(apmib_update(CUSTOMER_HW_SETTING) <= 0)
                    {
                        DPrintf("apmib_update CUSTOMER_HW_SETTING fail.\n");
                    }
            	}
            	else 
            	{
            		DPrintf("set mib MIB_CUSTOMER_HW_SET_FLG fail.\n");
            	}
            }
        }
    }
    
}

int main()
{
	int		sock_fd;
	struct in_addr	local_ip;
	
	int 	nlen;
	char	recv_buf[RECV_MAX_LEN];
	char	send_buf[SEND_MAX_LEN];

	int		argc;
	char**	argv=NULL;

	struct	sockaddr_in local,remote;
	int		addrlen;

	fd_set rfds;
	int retval;

    char mac[13] = "000000000000";
    char sn[22] = "000000000000000000000";
    int fd_key1;
    int fd_key2;
    int ret;
    char cmd_buf[100];
    struct timeval time_out;
    unsigned long sec;
    struct sysinfo info;
    int mib_val;

    apmib_init();
    memset(g_mib_mac, 0, sizeof(g_mib_mac));
    memset(g_mib_sn, 0, sizeof(g_mib_sn));
    ReadMAC();
    ReadSN();
    
	memset( &local, 0, sizeof(local) );
	local.sin_family = AF_INET;
	local.sin_port = htons(LISTEN_PORT);

	//strcpy(local_ip,"192.168.1.254");

    GetInAddr("br0", IP_ADDR, (void *)&local_ip );
    DPrintf("local ip addr = %s\n", inet_ntoa(local_ip));
	local.sin_addr = local_ip;

	sock_fd = socket( AF_INET,SOCK_DGRAM, 0 );
	if ( sock_fd < 0 ) 
    {
		DPrintf("socket error.\n");
		return 0;
	}
	
	int n = 1;
	if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n)) == -1)
	{
		close(sock_fd);
		DPrintf("setsockopt failed.\n");
		return 0;
	}

	if (bind(sock_fd,(struct sockaddr*)&local,sizeof(local)) < 0) {
		DPrintf("bind error.\n");
		return 0;
	}

    printf("###product_tool start v0.10 %s %s###\n",__DATE__, __TIME__);
    
	while (1) 
	{  
        TimerCheckAging();
		FD_ZERO(&rfds);
		FD_SET(sock_fd, &rfds);
        time_out.tv_sec = 1;
        time_out.tv_usec = 0;
		retval = select(sock_fd + 1,&rfds, NULL, NULL, &time_out);

		if(retval <= 0 || !FD_ISSET(sock_fd, &rfds))
			continue;
		
		bzero(recv_buf,sizeof(recv_buf));
		bzero(send_buf,sizeof(send_buf));
		addrlen = sizeof(remote);
		nlen = recvfrom(sock_fd,recv_buf,sizeof(recv_buf),0,(struct sockaddr*)&remote,(socklen_t*)&addrlen);
	
		if (nlen < 6)	//ROUTER头
			continue;

		DTRACE("recv len[%d] buf[%s]\n",nlen,recv_buf);

		argc = get_argc((const char *)recv_buf);
		argv = (char**)get_argv((const char *)recv_buf);
		DTRACE("argc=%d,argv=%s\n", argc, recv_buf);
		
		if(argc < 1) 
			continue;

 
		if (!strcmp(argv[0], "ROUTER" ))
		{
		    if(argc >= 2)
		    {
				if (!strcmp(argv[1], "set" ))
				{
                    if(argc >= 4)
                    {
    					if (!strcmp(argv[2], "mac"))
    					{
    						DTRACE("set mac %s\n", argv[3]);

                            if(strlen(argv[3]) == 12)
                            {
                                memset(mac, 0, sizeof(mac));
                                strcpy(mac, argv[3]);
                                if(SetMAC(mac))
                                {
        						    strcpy(send_buf,"ROUTER pass");
                                    SetMibSetFlg(ROUTER_SET_FLG_MAC, 1);
                                }
                                else
                                {
                                    strcpy(send_buf,"ROUTER fail");
                                    ReadMAC();    
                                }

                            }
                            else
                            {
                                strcpy(send_buf,"ROUTER fail");
                            }
        						nlen = strlen(send_buf);
        						sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
    					}
    					else if(!strcmp(argv[2], "sn"))
    					{
    						DTRACE("set sn %s\n", argv[3]);

                            if(strlen(argv[3]) == 21)
                            {
                                memset(sn, 0, sizeof(sn));
                                strcpy(sn, argv[3]);
                                if(SetSN(sn))
                                {
        						    strcpy(send_buf,"ROUTER pass");
                                    SetMibSetFlg(ROUTER_SET_FLG_SN, 1);
                                }
                                else
                                {
                                    strcpy(send_buf,"ROUTER fail");
                                    ReadSN();
                                }

                            }
                            else
                            {
                                strcpy(send_buf,"ROUTER fail");
                            }
    						nlen = strlen(send_buf);
    						sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
    					}
                        else if(!strcmp(argv[2], "led"))
    					{
    						DTRACE("set led %s\n", argv[3]);

                            if(strlen(argv[3]) == 1)
                            {
                                if(!strcmp(argv[3], "1"))
                                {
                                    mib_val = 1;
	                                apmib_set(MIB_LED_CONTROL_DISABLE, (void *)&mib_val);    //disable led control when run led test
                                    sprintf(cmd_buf, "echo 0 > /proc/gpio_color");   //red
                                    system(cmd_buf);
                                    sprintf(cmd_buf, "echo 1 > /proc/gpio_mode");
                                    system(cmd_buf);
                                }
                                else if(!strcmp(argv[3], "2"))
                                {
                                    mib_val = 1;
	                                apmib_set(MIB_LED_CONTROL_DISABLE, (void *)&mib_val);
                                    sprintf(cmd_buf, "echo 1 > /proc/gpio_color");   //green
                                    system(cmd_buf);
                                    sprintf(cmd_buf, "echo 1 > /proc/gpio_mode");
                                    system(cmd_buf);
                                    
                                }
                                else if(!strcmp(argv[3], "3"))
                                {
                                    mib_val = 1;
	                                apmib_set(MIB_LED_CONTROL_DISABLE, (void *)&mib_val);
                                    sprintf(cmd_buf, "echo 2 > /proc/gpio_color");   //bule
                                    system(cmd_buf);
                                    sprintf(cmd_buf, "echo 1 > /proc/gpio_mode");
                                    system(cmd_buf);
                                }
                                strcpy(send_buf,"ROUTER pass");
                            }
                            else
                            {
                                strcpy(send_buf,"ROUTER fail");
                            }
    						nlen = strlen(send_buf);
    						sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
    					}
                        else if(!strcmp(argv[2], "mesh"))
    					{
                             if(argc == 6)
                             { 
                                DTRACE("set mesh_ssid: %s mesh_key: %s mode: %s\n", argv[3], argv[4], argv[5]);
                                int mode;
                                if(!strcmp(argv[5], "1"))
                                {
                                    mode = 1;
                                }
                                else
                                {
                                    mode = 0;
                                }
                                if(SetMeshParam(argv[3], argv[4], mode))
                                {
                                    SetMibSetFlg(ROUTER_SET_FLG_PAIRING, 1);
                            	    strcpy(send_buf,"ROUTER pass");
                                }
                                else
                                {
                                    strcpy(send_buf,"ROUTER fail");
                                }

                                nlen = strlen(send_buf);
                                sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
                             }
    					}
                        else if(!strcmp(argv[2], "fun_test_flg"))
    					{
    						DTRACE("set fun_test_flg %s\n", argv[3]);

                            mib_val = 0;
	                        apmib_set(MIB_LED_CONTROL_DISABLE, (void *)&mib_val);
                            if(strlen(argv[3]) == 1)
                            {
                                if(!strcmp(argv[3], "1"))
                                {
                                    SetMibSetFlg(ROUTER_SET_FLG_FUN_TEST, 1);
                                }
                                else
                                {
                                    SetMibSetFlg(ROUTER_SET_FLG_FUN_TEST, 0);
                                }
                                strcpy(send_buf,"ROUTER pass");
                            }
                            else
                            {
                                strcpy(send_buf,"ROUTER fail");
                            }
    						nlen = strlen(send_buf);
    						sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
    					}
                    }
                    
				}
                else if(!strcmp(argv[1], "detect"))
				{
                    DTRACE("product_tool detect\n");
                    unsigned int flg = 0;
                   
                    apmib_get(MIB_CUSTOMER_HW_SET_FLG, &flg);
            		sysinfo(&info);
            		sec = (unsigned long) info.uptime ;
                    
					sprintf(send_buf,"ROUTER detect %s %s %s %s %s %d %ld \"%s\"", g_mib_sn, &g_mib_mac[0][0], &g_mib_mac[1][0], &g_mib_mac[2][0], &g_mib_mac[5][0], flg, sec, fwVersion);
					nlen = strlen(send_buf);
					sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
                    DTRACE("product_tool send--> %s\n", send_buf);
				}
                else if(!strcmp(argv[1], "get"))
				{
                    if(argc >= 3)
                    {
                        DTRACE("product_tool get %s\n", argv[2]);
                        if(!strcmp(argv[2], "key"))
    					{
                            memset(g_key1_sta, 0, sizeof(g_key1_sta));
                            memset(g_key2_sta, 0, sizeof(g_key2_sta));
                            fd_key1 = open(KEY1_VALUE_PATH, O_RDONLY);
                            if(fd_key1 < 0)
                            {
                                DPrintf("err: open  fd_key0 fail.\n");
                            }
                            else
                            {
                                ret = read(fd_key1, g_key1_sta, 1);
                                if(ret < 0)
                                {
                                    DPrintf("err: read  fd_key0 fail.\n");
                                }

                                close(fd_key1);
                                
                            }
                            fd_key2 = open(KEY2_VALUE_PATH, O_RDONLY);
                            if(fd_key2 < 0)
                            {
                                DPrintf("err: open  fd_key2 fail.\n");
                            }
                            else
                            {
                                ret = read(fd_key2, g_key2_sta, 1);
                                if(ret < 0)
                                {
                                    DPrintf("err: read  fd_key2 fail.\n");
                                }

                                close(fd_key2);
                                
                            }
                            sprintf(send_buf,"ROUTER get key %s %s", g_key1_sta, g_key2_sta);
        					nlen = strlen(send_buf);
        					sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
                            DTRACE("product_tool send--> %s\n", send_buf);
                        }
                    }
                }
                else if(!strcmp(argv[1], "default"))
				{
                    DTRACE("product_tool default\n");
                    
                    if ( !apmib_updateDef() )
                    {
                        strcpy(send_buf,"ROUTER fail");
                    }
                    else
                    {
                        apmib_reinit();
					    strcpy(send_buf,"ROUTER pass");
                    }
                    
					nlen = strlen(send_buf);
					sendto(sock_fd,(char *)send_buf,nlen,0,(struct sockaddr*)&remote,sizeof(remote));
                    DTRACE("product_tool send--> %s\n", send_buf);
				}
		    }
           
		}
		
		//continue
	}
	
}

