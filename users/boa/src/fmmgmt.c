/*
 *      Web server handler routines for management (password, save config, f/w update)
 *
 *      Authors: David Hsu	<davidhsu@realtek.com.tw>
 *
 *      $Id: fmmgmt.c,v 1.45 2009/09/03 05:04:42 keith_huang Exp $
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/reboot.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>


#include "boa.h"
#include "globals.h"
#include "apmib.h"
#include "apform.h"
#include "utility.h"
#include "mibtbl.h"
#include "asp_page.h"
#include "cJSON.h"


#ifdef SUPER_NAME_SUPPORT
#include "auth.h"
#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
#include "web_voip.h"
#include "voip_flash_mib.h"
#include "voip_flash_tool.h"
#endif

#if defined(POWER_CONSUMPTION_SUPPORT)
#include "powerCon.h"
#endif

#define DEFAULT_GROUP		"administrators"
#define ACCESS_URL		"/"

#ifdef CONFIG_RTL_WAPI_SUPPORT
#define MTD1_SIZE 0x2d0000	//Address space: 0x2d0000
#define WAPI_SIZE 0x10000	//Address space: 64K
#define WAPI_AREA_BASE (MTD1_SIZE-WAPI_SIZE)
#endif

#define MAX_L2_LIST_NUM    256

extern int Decode(unsigned char *ucInput, unsigned int inLen, unsigned char *ucOutput);

extern void log_boaform(char *form, request *req);

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
extern void Stop_Domain_Query_Process(void);
extern unsigned char WaitCountTime;
#endif
//static char superName[MAX_NAME_LEN]={0}, superPass[MAX_NAME_LEN]={0};
//static char userName[MAX_NAME_LEN]={0}, userPass[MAX_NAME_LEN]={0};
int isUpgrade_OK=0;
int isFWUPGRADE=0;
#ifdef BOA_WITH_SSL
int fwInProgress=0;
#endif
int isCFGUPGRADE=0;
int isREBOOTASP=0;
int Reboot_Wait=0;
int isCFG_ONLY=0;
#if defined(CONFIG_APP_FWD)
int isCountDown=0;
#endif
#ifdef LOGIN_URL
static void delete_user(request *wp);
static int add_user(request *wp);

#endif
int configlen = 0;

int opModeHandler(request *wp, char *tmpBuf);
int find_head_offset(char *upload_data);

/////////////////////////////////////////////////////////////////////////////
static int check_config_tag(unsigned char *data, int total_len)
{
	char tag_local[32],tag_config[32];
	memset(tag_local,0,sizeof(tag_local));
	apmib_get(MIB_CONFIG_TAG,tag_local);
	printf("tag_local(%s)\n",tag_local);
	if(strcmp(tag_local,"")==0) {
		return 1;
	}
	memset(tag_config,0,sizeof(tag_config));
	if(tlv_simple_mib_get(MIB_CONFIG_TAG,data,total_len,tag_config)<0)
	{
		return 0;
	}
	if(strcmp(tag_config,"")==0) {		
		return 0;
	}
	if(strcmp(tag_local,tag_config)==0)
	{		
		return 1;
	}
	return 0;
}
static int check_config_valid(unsigned char *data, int total_len)
{
	int len=0, status=1;
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_Tp phwHeader;
#endif
	int isHdware=0;
	PARAM_HEADER_Tp pHeader;
#ifdef COMPRESS_MIB_SETTING
	COMPRESS_MIB_HEADER_Tp pCompHeader;
	unsigned char *expFile=NULL;
	unsigned int expandLen=0;
	int complen=0;
#endif
	unsigned char isValidfw = 0;
	char *ptr;
	do {
		if (
#ifdef COMPRESS_MIB_SETTING
			memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)
#else
			memcmp(&data[len], CURRENT_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) 
#endif
		) {
			if(isValidfw)			
				break;
		}

	if(
	#ifdef COMPRESS_MIB_SETTING
		memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN)==0
	#else
		memcmp(&data[len], HW_SETTING_HEADER_TAG, TAG_LEN)==0 ||
		memcmp(&data[len], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN)==0 ||
		memcmp(&data[len], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)
	#endif
	)
	{
		isHdware=1;
	}
	else
	{
		isHdware=0;
	}

#ifdef COMPRESS_MIB_SETTING
		pCompHeader =(COMPRESS_MIB_HEADER_Tp)&data[complen];
#ifdef _LITTLE_ENDIAN_
		pCompHeader->compRate = WORD_SWAP(pCompHeader->compRate);
		pCompHeader->compLen = DWORD_SWAP(pCompHeader->compLen);
#endif
		expFile=malloc(pCompHeader->compLen*pCompHeader->compRate);
		if (NULL==expFile) {
			printf("malloc for expFile error!!\n");
			return 0;
		}
		expandLen = Decode(data+complen+sizeof(COMPRESS_MIB_HEADER_T), pCompHeader->compLen, expFile);
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader = (HW_PARAM_HEADER_Tp)expFile;
		else
#endif
		pHeader = (PARAM_HEADER_Tp)expFile;
#else
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader = (HW_PARAM_HEADER_Tp)expFile;
		else
#endif
		pHeader = (PARAM_HEADER_Tp)&data[len];
#endif
#ifdef _LITTLE_ENDIAN_
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader->len = DWORD_SWAP(phwHeader->len);
		else
#endif
		pHeader->len = WORD_SWAP(pHeader->len);
#endif
#ifdef HEADER_LEN_INT
		if(isHdware)
			len += sizeof(HW_PARAM_HEADER_T);
		else
#endif
		len += sizeof(PARAM_HEADER_T);
#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif
#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
#ifdef COMPRESS_MIB_SETTING
			if(isHdware == 0)
			{
				status=check_config_tag(ptr,pHeader->len);
				if(0 == status) {					
					break;
				}	
			}
#endif			
#ifdef COMPRESS_MIB_SETTING
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			isValidfw=1;
			continue;
	}while (
#ifdef COMPRESS_MIB_SETTING
	(complen < total_len)
#else
	(len < total_len)
#endif	
	);
	return status;
}
static int updateConfigIntoFlash(unsigned char *data, int total_len, int *pType, int *pStatus)
{
	int len=0, status=1, type=0, ver, force;
#ifdef HEADER_LEN_INT
	HW_PARAM_HEADER_Tp phwHeader;
	int isHdware=0;
#endif
	PARAM_HEADER_Tp pHeader;
#ifdef COMPRESS_MIB_SETTING
	COMPRESS_MIB_HEADER_Tp pCompHeader;
	unsigned char *expFile=NULL;
	unsigned int expandLen=0;
	int complen=0;
	int compLen_of_header=0;
	short compRate=0;
#endif
	char *ptr;
	unsigned char isValidfw = 0;
	#if 0
	if(0 == check_config_valid(data,total_len))
	{
		printf("%s %d\n",__FUNCTION__,__LINE__);
		pStatus=0;
		return 0;
	}
	#endif
	do {
		if (
#ifdef COMPRESS_MIB_SETTING
			memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)
#else
			memcmp(&data[len], CURRENT_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) &&
			memcmp(&data[len], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) 
#endif
		) {
			if (isValidfw == 1)
				break;
		}
#ifdef HEADER_LEN_INT
	if(
	#ifdef COMPRESS_MIB_SETTING
		memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN)==0
	#else
		memcmp(&data[len], HW_SETTING_HEADER_TAG, TAG_LEN)==0 ||
		memcmp(&data[len], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN)==0 ||
		memcmp(&data[len], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)
	#endif
	)
	{
		isHdware=1;
	}
#endif
#ifdef COMPRESS_MIB_SETTING
		pCompHeader =(COMPRESS_MIB_HEADER_Tp)&data[complen];
		compRate = WORD_SWAP(pCompHeader->compRate);
		compLen_of_header = DWORD_SWAP(pCompHeader->compLen);
		/*decompress and get the tag*/
#ifdef RTK_MIB_TAG_CHECK
		expFile=malloc(compLen_of_header*WORD_SWAP(pCompHeader->realcompRate));
#else
		expFile=malloc(compLen_of_header*compRate);
#endif
		if (NULL==expFile) {
			printf("malloc for expFile error!!\n");
			return 0;
		}
		expandLen = Decode(data+complen+sizeof(COMPRESS_MIB_HEADER_T), compLen_of_header, expFile);
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader = (HW_PARAM_HEADER_Tp)expFile;
		else
#endif
		pHeader = (PARAM_HEADER_Tp)expFile;
#else
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader = (HW_PARAM_HEADER_Tp)expFile;
		else
#endif
		pHeader = (PARAM_HEADER_Tp)&data[len];
#endif
		
#ifdef _LITTLE_ENDIAN_
#ifdef HEADER_LEN_INT
		if(isHdware)
			phwHeader->len = WORD_SWAP(phwHeader->len);
		else
#endif
		pHeader->len = HEADER_SWAP(pHeader->len);
#endif
#ifdef HEADER_LEN_INT
		if(isHdware)
			len += sizeof(HW_PARAM_HEADER_T);
		else
#endif
		len += sizeof(PARAM_HEADER_T);

        /*in case use wrong version config.dat*/
        #define MAX_CONFIG_LEN 1024*1024
        #define MIN_CONFIG_LEN 8*1024
#ifdef HEADER_LEN_INT
		if(isHdware)
        {
            if((phwHeader->len > MAX_CONFIG_LEN)||(phwHeader->len < MIN_CONFIG_LEN))
            {
                printf("INVALID config.data FILE\n");
                status = 0;
                break;
            }
        }else
#endif
        {
            if((pHeader->len > MAX_CONFIG_LEN)||(pHeader->len < MIN_CONFIG_LEN))
            {
                printf("INVALID config.data FILE\n");
                status = 0;
                break;
            }
        }

		if ( sscanf((char *)&pHeader->signature[TAG_LEN], "%02d", &ver) != 1)
			ver = -1;
			
		force = -1;
		if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 1; // update
		}
		else if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN)) {
			isValidfw = 1;
			force = 2; // force
		}
		else if ( !memcmp(pHeader->signature, CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN)) {
			isValidfw = 1;
			force = 0; // upgrade
		}

		if ( force >= 0 ) {
#if 0
			if ( !force && (ver < CURRENT_SETTING_VER || // version is less than current
				(pHeader->len < (sizeof(APMIB_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif

#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
					if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
						status = 0;
						break;
					}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
//#ifdef _LITTLE_ENDIAN_
//			swap_mib_word_value((APMIB_Tp)ptr);
//#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		#ifndef VOIP_SUPPORT_TLV_CFG
			flash_voip_import_fix(&((APMIB_Tp)ptr)->voipCfgParam, &pMib->voipCfgParam);
#endif
#endif

#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(CURRENT_SETTING, (char *)&data[complen], compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(CURRENT_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(CURRENT_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}
#else
			
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			type |= CURRENT_SETTING;
			continue;
		}


		if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 1;	// update
		}
		else if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 2;	// force
		}
		else if ( !memcmp(pHeader->signature, DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 0;	// upgrade
		}

		if ( force >= 0 ) {
#if 0
			if ( (ver < DEFAULT_SETTING_VER) || // version is less than current
				(pHeader->len < (sizeof(APMIB_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif

#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}

//#ifdef _LITTLE_ENDIAN_
//			swap_mib_word_value((APMIB_Tp)ptr);
//#endif

// added by rock /////////////////////////////////////////
#ifdef VOIP_SUPPORT
		#ifndef VOIP_SUPPORT_TLV_CFG
			flash_voip_import_fix(&((APMIB_Tp)ptr)->voipCfgParam, &pMibDef->voipCfgParam);
#endif
#endif

#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(DEFAULT_SETTING, (char *)&data[complen], compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
		
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(DEFAULT_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(DEFAULT_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += compLen_of_header+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}	
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif
			type |= DEFAULT_SETTING;
			continue;
		}

		if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 1;	// update
		}
		else if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 2;	// force
		}
		else if ( !memcmp(pHeader->signature, HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ) {
			isValidfw = 1;
			force = 0;	// upgrade
		}

		if ( force >= 0 ) {
#if 0
			if ( (ver < HW_SETTING_VER) || // version is less than current
				(pHeader->len < (sizeof(HW_SETTING_T)+1)) ) { // length is less than current
				status = 0;
				break;
			}
#endif
#ifdef COMPRESS_MIB_SETTING
#ifdef HEADER_LEN_INT
			if(isHdware)
				ptr = (char *)(expFile+sizeof(HW_PARAM_HEADER_T));
			else
#endif
			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
#else
			ptr = &data[len];
#endif
			

#ifdef COMPRESS_MIB_SETTING
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				DECODE_DATA(ptr, phwHeader->len);
			else
#endif
			DECODE_DATA(ptr, pHeader->len);
#endif
#ifdef HEADER_LEN_INT
			if(isHdware)
			{
				if ( !CHECKSUM_OK((unsigned char *)ptr, phwHeader->len)) {
				status = 0;
				break;
				}
			}
			else
#endif
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
#ifdef COMPRESS_MIB_SETTING
			apmib_updateFlash(HW_SETTING, (char *)&data[complen], compLen_of_header + sizeof(COMPRESS_MIB_HEADER_T), force, ver);
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				apmib_updateFlash(HW_SETTING, ptr, phwHeader->len-1, force, ver);
			else
#endif
			apmib_updateFlash(HW_SETTING, ptr, pHeader->len-1, force, ver);
#endif

#ifdef COMPRESS_MIB_SETTING
			complen += compLen_of_header + sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}
#else
#ifdef HEADER_LEN_INT
			if(isHdware)
				len += phwHeader->len;
			else
#endif
			len += pHeader->len;
#endif

			type |= HW_SETTING;
			continue;
		}
	}
#ifdef COMPRESS_MIB_SETTING	
	while (complen < total_len);

	if (expFile) {
		free(expFile);
		expFile=NULL;
	}
#else
	while (len < total_len);
#endif

	*pType = type;
	*pStatus = status;
#ifdef COMPRESS_MIB_SETTING	
	return complen;
#else
	return len;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/*
void sig_alm(int signo)
{
	if(isUpgrade_OK ==1){	
		reboot( RB_AUTOBOOT);
		return;
	}

}
*/
///////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_SNMP
void formSetSNMP(request *wp, char *path, char *query)
{
	
		char *submitUrl;
        char *strValue;
        int     snmpEnabled;
        struct in_addr ip;
        char tmpBuf[100];
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

        strValue = (char *)req_get_cstream_var(wp, ("snmp_enabled"), "");
		if(!strcmp(strValue, "ON")){
                snmpEnabled = 1;
        } else {
                snmpEnabled = 0;
        }
        if (!apmib_set(MIB_SNMP_ENABLED, (void *)&snmpEnabled)) {
                strcpy(tmpBuf, ("Set SNMP enabled error!"));
                goto setErr;
        }

        strValue = (char *)req_get_cstream_var(wp, ("snmp_name"), "");
        if (strValue[0]) {
                if (!apmib_set(MIB_SNMP_NAME, (void *)strValue)) {
                        strcpy(tmpBuf, ("Set SNMP location error!"));
                        goto setErr;
                }
        }

        strValue = (char *)req_get_cstream_var(wp, ("snmp_location"), "");
        if (strValue[0]) {
                if (!apmib_set(MIB_SNMP_LOCATION, (void *)strValue)) {
                        strcpy(tmpBuf, ("Set SNMP location error!"));
                        goto setErr;
                }
        }

        strValue = (char *)req_get_cstream_var(wp, ("snmp_contact"), "");
        if (strValue[0]) {
                if (!apmib_set(MIB_SNMP_CONTACT, (void *)strValue)) {
                        strcpy(tmpBuf, ("Set SNMP contact error!"));
                        goto setErr;
                }
        }
		
        strValue = (char *)req_get_cstream_var(wp, ("snmp_rwcommunity"), "");
        if (strValue[0]) {
                if (!apmib_set(MIB_SNMP_RWCOMMUNITY, (void *)strValue)) {
                        strcpy(tmpBuf, ("Set SNMP community error!"));
                        goto setErr;
                }
        }


        strValue = (char *)req_get_cstream_var(wp, ("snmp_rocommunity"), "");
        if (strValue[0]) {
                if (!apmib_set(MIB_SNMP_ROCOMMUNITY, (void *)strValue)) {
                        strcpy(tmpBuf, ("Set SNMP community error!"));
                        goto setErr;
                }
        }

        strValue = (char *)req_get_cstream_var(wp, ("snmp_trap1"), "");
        if (strValue[0]) {
                if (!inet_aton(strValue, &ip) ) {
                        strcpy(tmpBuf, ("Invalid Trap Receiver 1 IP-address value!"));
                        goto setErr;
                }
                if (!apmib_set(MIB_SNMP_TRAP_RECEIVER1, (void *)&ip)) {
                        strcpy(tmpBuf, ("Set Trap Receiver 1 IP-address error!"));
                        goto setErr;
                }
        }

        strValue = (char *)req_get_cstream_var(wp, ("snmp_trap2"), "");
        if (strValue[0]) {
                if (!inet_aton(strValue, &ip) ) {
                        strcpy(tmpBuf, ("Invalid Trap Receiver 2 IP-address value!"));
                        goto setErr;
                }
                if (!apmib_set(MIB_SNMP_TRAP_RECEIVER2, (void *)&ip)) {
                        strcpy(tmpBuf, ("Set Trap Receiver 2 IP-address error!"));
                        goto setErr;
                }
        }

        strValue = (char *)req_get_cstream_var(wp, ("snmp_trap3"), "");
        if (strValue[0]) {
                if (!inet_aton(strValue, &ip) ) {
                        strcpy(tmpBuf, ("Invalid Trap Receiver 3 IP-address value!"));
                        goto setErr;
                }
                if (!apmib_set(MIB_SNMP_TRAP_RECEIVER3, (void *)&ip)) {
                        strcpy(tmpBuf, ("Set Trap Receiver 3 IP-address error!"));
                        goto setErr;
                }
        }

        apmib_update_web(CURRENT_SETTING);

#ifndef NO_ACTION
        run_init_script("all");
#endif
		OK_MSG(submitUrl);
        return;

setErr:
		ERR_MSG(tmpBuf);
}
#endif /* CONFIG_SNMP */

///////////////////////////////////////////////////////////////////////////////
void formSaveConfig(request *wp, char *path, char *query)
{
	char *strRequest;
	char *buf, *ptr=NULL;
	unsigned char checksum;
	int len, len1;
	char tmpBuf[200];
	CONFIG_DATA_T type=0;
	//char *submitUrl;
	char lan_ip_buf[30], lan_ip[30];
	
	len1 = sizeof(PARAM_HEADER_T) + sizeof(APMIB_T) + sizeof(checksum) + 100;  // 100 for expansion
	len = csHeader.len;
#ifdef _LITTLE_ENDIAN_
#ifdef VOIP_SUPPORT
	// rock: don't need swap here
	// 1. write to private space (ex: flash)
	// 2. read from private space (ex: flash)
#else
	len  = WORD_SWAP(len);
#endif
#endif
	len += sizeof(PARAM_HEADER_T) + 100;
	if (len1 > len)
		len = len1;

	buf = malloc(len);
	if ( buf == NULL ) {
		strcpy(tmpBuf, "Allocate buffer failed!");		
		goto back;
	}

	strRequest = req_get_cstream_var(wp, ("save-cs"), "");
	if (strRequest[0]){
		type |= CURRENT_SETTING;
        save_cs_to_Mesh1();
	}

	strRequest = req_get_cstream_var(wp, ("save"), "");
	if (strRequest[0]){
		type |= CURRENT_SETTING;
        save_cs_to_Mesh1();
	}

	strRequest = req_get_cstream_var(wp, ("save-hs"), "");
	if (strRequest[0])
		type |= HW_SETTING;

	strRequest = req_get_cstream_var(wp, ("save-ds"), "");
	if (strRequest[0])
		type |= DEFAULT_SETTING;

	strRequest = req_get_cstream_var(wp, ("save-all"), "");
	if (strRequest[0])
		type |= HW_SETTING | DEFAULT_SETTING | CURRENT_SETTING;
	if (type) {
		send_redirect_perm(wp, "/config.dat");
		return;
	}

	strRequest = req_get_cstream_var(wp, ("reset"), "");
	if ((strRequest[0] && strcmp(strRequest,"Reset") == 0) || (strRequest[0] && strcmp(strRequest,"重置") == 0)) {
#ifdef RTL_DEF_SETTING_IN_FW
		system("flash reset");
#else
		if ( !apmib_updateDef() ) {
			free(ptr);
			strcpy(tmpBuf, "Write default to current setting failed!\n");
			free(buf);
			goto back;
		}
#endif
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
		//To clear 802.1x certs
		//RunSystemCmd(NULL_FILE, "rsCert","-rst", NULL_STR);
		system("rsCert -rst");
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT
		//To clear CA files
		system("storeWapiFiles -reset");
#endif

#if defined(CONFIG_RTL_92D_SUPPORT) || defined(CONFIG_POCKET_AP_SUPPORT)
		Reboot_Wait = 60;
#else
		Reboot_Wait = 40;
#endif
#ifdef HOME_GATEWAY
		sprintf(tmpBuf, "%s","<script>dw(saveconf_reload_setting)</script><br><br><script>dw(saveconf_router_booting)</script><br><script>dw(saveconf_not_turnoff_reboot)</script><br>");
#else
		sprintf(tmpBuf, "%s", "<script>dw(saveconf_reload_setting)</script><br><br><script>dw(saveconf_aprouter_booting)</script><br>");
#endif
		//ERR_MSG(tmpBuf);
		apmib_reinit();
		apmib_update_web(CURRENT_SETTING);	// update configuration to flash
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf);
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
		OK_MSG_FW(tmpBuf, submitUrl,Reboot_Wait,lan_ip);
		if(ptr != NULL) {
			free(ptr);
		}
		/* Reboot DUT. Keith */
		isUpgrade_OK=1;
		REBOOT_WAIT_COMMAND(2);		
		return;
	}

back:
	ERR_MSG(tmpBuf);
	return;
}

void formUploadConfig(request *wp, char *path, char *query)
{
	int status=0;
	char tmpBuf[200];
	CONFIG_DATA_T type=0;
	char *submitUrl;
	char lan_ip_buf[30], lan_ip[30];
	int head_offset=0;

#ifdef RTK_MIB_TAG_CHECK
	int cpuType;
	int valid_config=1;
#endif

#if defined(CONFIG_APP_FWD)
#define FWD_CONF "/var/fwd.conf"
	int newfile = 1;
	extern int get_shm_id();
	extern int clear_fwupload_shm();
	int shm_id = get_shm_id();	
#endif
	
	head_offset = find_head_offset((char *)wp->upload_data);
	//fprintf(stderr,"####%s:%d head_offset=%d###\n",  __FILE__, __LINE__ , head_offset);
	if (head_offset == -1) {
		strcpy(tmpBuf, "<script>dw(saveconf_invalid)</script>");
		goto back;
	}
#ifdef RTK_MIB_TAG_CHECK
	cpuType=getRTKCpuType();
	if(cpuType >= RTK_CPU_MAX){
		printf("get cpu type error\n");
	}
#ifdef COMPRESS_MIB_SETTING
	if(!memcmp(&wp->upload_data[head_offset], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) ||
		!memcmp(&wp->upload_data[head_offset], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) ||
		!memcmp(&wp->upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN))
	{
		
		COMPRESS_MIB_HEADER_Tp pCompHeader;
		pCompHeader =(COMPRESS_MIB_HEADER_Tp)(&wp->upload_data[head_offset]);
		if(cpuType != ((pCompHeader->mibTag & RTK_CPU_TYPE_MASK) >> RTK_CPU_TYPE_OFFSET))
			valid_config=0;
	}
#endif
#endif	
	if(
#ifdef 	RTK_MIB_TAG_CHECK
		valid_config &&
#endif
#ifdef COMPRESS_MIB_SETTING
		(!memcmp(&wp->upload_data[head_offset], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) ||
		!memcmp(&wp->upload_data[head_offset], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) ||
		!memcmp(&wp->upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN))
#else
		(!memcmp(&wp->upload_data[head_offset], CURRENT_SETTING_HEADER_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], DEFAULT_SETTING_HEADER_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
		!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN))
#endif
	) {
		updateConfigIntoFlash((unsigned char *)&wp->upload_data[head_offset], (wp->upload_len-head_offset), (int *)&type, &status);
	}
	if (status == 0 || type == 0) { // checksum error
		strcpy(tmpBuf, "<script>dw(saveconf_invalid)</script>");
		goto back;
	}
	else {
		if (type) { // upload success
#ifdef CONFIG_RTL_802_1X_CLIENT_SUPPORT
			//To clear 802.1x certs
			//RunSystemCmd(NULL_FILE, "rsCert","-rst", NULL_STR);
			system("rsCert -rst");
#endif
#ifdef CONFIG_RTL_WAPI_SUPPORT 
			//To clear CA files
			system("storeWapiFiles -reset");
#endif
		}

#ifdef HOME_GATEWAY
		sprintf(tmpBuf, ("%s"), "Update successfully!<br><br>Update in progressing.<br>Do not turn off or reboot the Device during this time.<br>");
#else
		sprintf(tmpBuf, ("%s"), "Update successfully!<br><br>Update in progress.<br> Do not turn off or reboot the AP during this time.");
#endif			
		Reboot_Wait = 45;
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
			
		apmib_reinit();
		apmib_update_web(CURRENT_SETTING);	// update configuration to flash
		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#ifdef REBOOT_CHECK
		sprintf(lastUrl,"%s",submitUrl);
		sprintf(okMsg,"%s",tmpBuf);
		countDownTime = Reboot_Wait;
		send_redirect_perm(wp, COUNTDOWN_PAGE);
		/*Reboot DUT in main loop*/
		isCFGUPGRADE=1;
#else
		OK_MSG_FW(tmpBuf, submitUrl,Reboot_Wait,lan_ip);
		
		/* Reboot DUT. Keith */
		isUpgrade_OK=1;
		REBOOT_WAIT_COMMAND(2);		
#endif		
		return;
	}
back:
#if defined(CONFIG_APP_FWD)		
	clear_fwupload_shm(shm_id);
#endif
	ERR_MSG(tmpBuf);
	return;
}
#ifdef CONFIG_APP_OPENVPN
void formSaveOpenvpnClientConfig(request *wp, char *path, char *query)
{
	char *strRequest;	

	strRequest = req_get_cstream_var(wp, ("saveOpenvpnClientConfig"), "");
	
	if (strRequest[0]) 
	{
		//system("cp /var/openvpn/client.conf /web");
		send_redirect_perm(wp, "/client.ovpn");
	}

	return;	
}
#endif
///////////////////////////////////////////////////////////////////////////////

#if 0 //Keith. move to utility.c
void kill_processes(void)
{


	printf("upgrade: killing tasks...\n");
	
	kill(1, SIGTSTP);		/* Stop init from reforking tasks */
	kill(1, SIGSTOP);		
	kill(2, SIGSTOP);		
	kill(3, SIGSTOP);		
	kill(4, SIGSTOP);		
	kill(5, SIGSTOP);		
	kill(6, SIGSTOP);		
	kill(7, SIGSTOP);		
	//atexit(restartinit);		/* If exit prematurely, restart init */
	sync();

	signal(SIGTERM,SIG_IGN);	/* Don't kill ourselves... */
	setpgrp(); 			/* Don't let our parent kill us */
	sleep(1);
	signal(SIGHUP, SIG_IGN);	/* Don't die if our parent dies due to
					 * a closed controlling terminal */
	
}
#endif //#if 0 //Keith. move to utility.c

//////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE

#define SQSH_SIGNATURE		((char *)"sqsh")
#define SQSH_SIGNATURE_LE       ((char *)"hsqs")

#define IMAGE_ROOTFS 2
#define IMAGE_KERNEL 1
#define GET_BACKUP_BANK 2
#define GET_ACTIVE_BANK 1

#define GOOD_BANK_MARK_MASK 0x80000000  //goo abnk mark must set bit31 to 1

#define NO_IMAGE_BANK_MARK 0x80000000  
#define OLD_BURNADDR_BANK_MARK 0x80000001 
#define BASIC_BANK_MARK 0x80000002           
#define FORCEBOOT_BANK_MARK 0xFFFFFFF0  //means always boot/upgrade in this bank

#ifndef CONFIG_MTD_NAND
char *Kernel_dev_name[2]=
 {
   "/dev/mtdblock0", "/dev/mtdblock2"
 };
char *Rootfs_dev_name[2]=
 {
   "/dev/mtdblock1", "/dev/mtdblock3"
 };

#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
char *Web_dev_name[2]=
{
	"/dev/mtdblock0", "/dev/mtdblock2"
};
#endif
#endif
#else
char *Kernel_dev_name[2]=
 {
   "/dev/mtdblock2", "/dev/mtdblock5"
 };
char *Rootfs_dev_name[2]=
 {
   "/dev/mtdblock3", "/dev/mtdblock6"
 };

#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
char *Web_dev_name[2]=
{
	"/dev/mtdblock2", "/dev/mtdblock5"
};
#endif
#endif


#endif

static int get_actvie_bank()
{
	FILE *fp;
	char buffer[2];
	int bootbank;
	fp = fopen("/proc/bootbank", "r");
	
	if (!fp) {
		fprintf(stderr,"%s\n","Read /proc/bootbank failed!\n");
	}else
	{
			//fgets(bootbank, sizeof(bootbank), fp);
			fgets(buffer, sizeof(buffer), fp);
			fclose(fp);
	}
	bootbank = buffer[0] - 0x30;	
	if ( bootbank ==1 || bootbank ==2)
		return bootbank;
	else
		return 1;	
}

void get_bank_info(int dual_enable,int *active,int *backup)
{
	int bootbank=0,backup_bank;
	
	bootbank = get_actvie_bank();	

	if(bootbank == 1 )
	{
		if( dual_enable ==0 )
			backup_bank =1;
		else
			backup_bank =2;
	}
	else if(bootbank == 2 )
	{
		if( dual_enable ==0 )
			backup_bank =2;
		else
			backup_bank =1;
	}
	else
	{
		bootbank =1 ;
		backup_bank =1 ;
	}	

	*active = bootbank;
	*backup = backup_bank;	

	//fprintf(stderr,"get_bank_info active_bank =%d , backup_bank=%d  \n",*active,*backup); //mark_debug	   
}
static unsigned long header_to_mark(int  flag, IMG_HEADER_Tp pHeader)
{
	unsigned long ret_mark=NO_IMAGE_BANK_MARK;
	//mark_dual ,  how to diff "no image" "image with no bank_mark(old)" , "boot with lowest priority"
	if(flag) //flag ==0 means ,header is illegal
	{
		if( (pHeader->burnAddr & GOOD_BANK_MARK_MASK) )
			ret_mark=pHeader->burnAddr;	
		else
			ret_mark = OLD_BURNADDR_BANK_MARK;
	}
	return ret_mark;
}

// return,  0: not found, 1: linux found, 2:linux with root found
static int check_system_image(int fh,IMG_HEADER_Tp pHeader)
{
	// Read header, heck signature and checksum
	int i, ret=0;		
	char image_sig[4]={0};
	char image_sig_root[4]={0};
	
        /*check firmware image.*/
	if ( read(fh, pHeader, sizeof(IMG_HEADER_T)) != sizeof(IMG_HEADER_T)) 
     		return 0;	
	
	memcpy(image_sig, FW_HEADER, SIGNATURE_LEN);
	memcpy(image_sig_root, FW_HEADER_WITH_ROOT, SIGNATURE_LEN);

	if (!memcmp(pHeader->signature, image_sig, SIGNATURE_LEN))
		ret=1;
	else if  (!memcmp(pHeader->signature, image_sig_root, SIGNATURE_LEN))
		ret=2;
	else{
		printf("no sys signature at !\n");
	}				
       //mark_dual , ignore checksum() now.(to do) 
	return (ret);
}

static int check_rootfs_image(int fh)
{
	// Read header, heck signature and checksum
	int i;
	unsigned short sum=0, *word_ptr;
	unsigned long length=0;
	unsigned char rootfs_head[SIGNATURE_LEN];		
	
	if ( read(fh, &rootfs_head, SIGNATURE_LEN ) != SIGNATURE_LEN ) 
     		return 0;	
	
	if ( memcmp(rootfs_head, SQSH_SIGNATURE, SIGNATURE_LEN) && memcmp(rootfs_head, SQSH_SIGNATURE_LE, SIGNATURE_LEN)) {
		printf("no rootfs signature at !\n");
		return 0;
	}
	
	return 1;
}

static int get_image_header(int fh,IMG_HEADER_Tp header_p)
{
	int ret=0;
	//check 	CODE_IMAGE_OFFSET2 , CODE_IMAGE_OFFSET3 ?
	//ignore check_image_header () for fast get header , assume image are same offset......	
	// support CONFIG_RTL_FLASH_MAPPING_ENABLE ? , scan header ...
#ifndef CONFIG_MTD_NAND
	lseek(fh, CODE_IMAGE_OFFSET, SEEK_SET);		
#else
	lseek(fh, CODE_IMAGE_OFFSET-NAND_BOOT_SETTING_SIZE, SEEK_SET);	
#endif
	ret = check_system_image(fh,header_p);

	//assume , we find the image header in CODE_IMAGE_OFFSET
#ifndef CONFIG_MTD_NAND
	lseek(fh, CODE_IMAGE_OFFSET, SEEK_SET);	
#else
	lseek(fh, CODE_IMAGE_OFFSET-NAND_BOOT_SETTING_SIZE, SEEK_SET);	
#endif
	
	return ret;	
}

 int check_bank_image(int bank)
{
	int i,ret=0;	
    int fh,fh_rootfs;

#ifdef MTD_NAME_MAPPING
	char linuxMtd[16],rootfsMtd[16];
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	char linuxMtd2[16],rootfsMtd2[16];
#endif

	if(rtl_name_to_mtdblock(FLASH_LINUX_NAME,linuxMtd) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME,rootfsMtd) == 0){
		printf("cannot mapping %s and %s\n",FLASH_LINUX_NAME,FLASH_ROOTFS_NAME);
		return 0;
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if(rtl_name_to_mtdblock(FLASH_LINUX_NAME2,linuxMtd2) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME2,rootfsMtd2) == 0){
		printf("cannot mapping %s and %s\n",FLASH_LINUX_NAME2,FLASH_ROOTFS_NAME2);
		return 0;
	}
#endif

	Kernel_dev_name[0] = linuxMtd;
	Rootfs_dev_name[0] = rootfsMtd;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE	
	Kernel_dev_name[1] = linuxMtd2;
	Rootfs_dev_name[1] = rootfsMtd2;
#endif
#endif

    
	char *rootfs_dev = Rootfs_dev_name[bank-1];	
	char *kernel_dev = Kernel_dev_name[bank-1];	
	IMG_HEADER_T header;
           	
	fh = open(kernel_dev, O_RDONLY);
	if ( fh == -1 ) {
      		printf("Open file failed!\n");
		return 0;
	}
	ret = get_image_header(fh,&header);			
	
	close(fh);	
	if(ret==2)
        {	
	      	fh_rootfs = open(rootfs_dev, O_RDONLY);
		if ( fh_rootfs == -1 ) {
      		printf("Open file failed!\n");
		return 0;
		}
              ret=check_rootfs_image(fh_rootfs);
		close(fh_rootfs);	  
	  }
	return ret;
}

int write_header_bankmark(char *kernel_dev, unsigned long bankmark)
{
	int ret=0,fh,numWrite;
	IMG_HEADER_T header,*header_p;
	char buffer[200]; //mark_debug
	
	header_p = &header;
	fh = open(kernel_dev, O_RDWR);

	if ( fh == -1 ) {
      		printf("Open file failed!\n");
		return -1;
	}
	ret = get_image_header(fh,&header);

	if(!ret)
		return -2; //can't find active(current) imager header ...something wrong

	//fh , has been seek to correct offset	

	header_p->burnAddr = bankmark;

	//sprintf(buffer, ("write_header_bankmark kernel_dev =%s , bankmark=%x \n"), kernel_dev , header_p->burnAddr);
       //fprintf(stderr, "%s\n", buffer); //mark_debug	
       
	 //move to write image header will be done in get_image_header
	numWrite = write(fh, (char *)header_p, sizeof(IMG_HEADER_T));
	
	close(fh);
	
	return 0;	//success
}


// return,  0: not found, 1: linux found, 2:linux with root found

unsigned long get_next_bankmark(char *kernel_dev,int dual_enable)
{
    unsigned long bankmark=NO_IMAGE_BANK_MARK;
    int ret=0,fh;
    IMG_HEADER_T header; 	
	
	fh = open(kernel_dev, O_RDONLY);
	if ( fh == -1 ) {
      		fprintf(stderr,"%s\n","Open file failed!\n");
		return NO_IMAGE_BANK_MARK;
	}
	ret = get_image_header(fh,&header);	

	//fprintf(stderr,"get_next_bankmark = %s , ret = %d \n",kernel_dev,ret); //mark_debug

	bankmark= header_to_mark(ret, &header);	
	close(fh);
	//get next boot mark

	if( bankmark < BASIC_BANK_MARK)
		return BASIC_BANK_MARK;
	else if( (bankmark ==  FORCEBOOT_BANK_MARK) || (dual_enable == 0)) //dual_enable = 0 ....
	{
		return FORCEBOOT_BANK_MARK;//it means dual bank disable
	}
	else
		return bankmark+1;
	
}

// set mib at the same time or get mib to set this function? 
int set_dualbank(int enable)
{	
	int ret =0, active_bank=0, backup_bank=0;
	unsigned long bankmark=0;
	
#ifdef MTD_NAME_MAPPING
	char linuxMtd[16],rootfsMtd[16];
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	char linuxMtd2[16],rootfsMtd2[16];
#endif

	if(rtl_name_to_mtdblock(FLASH_LINUX_NAME,linuxMtd) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME,rootfsMtd) == 0){
		printf("cannot mapping %s and %s\n",FLASH_LINUX_NAME,FLASH_ROOTFS_NAME);
		return 0;
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if(rtl_name_to_mtdblock(FLASH_LINUX_NAME2,linuxMtd2) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME2,rootfsMtd2) == 0){
		printf("cannot mapping %s and %s\n",FLASH_LINUX_NAME2,FLASH_ROOTFS_NAME2);
		return 0;
	}
#endif

	Kernel_dev_name[0] = linuxMtd;
	Rootfs_dev_name[0] = rootfsMtd;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE	
	Kernel_dev_name[1] = linuxMtd2;
	Rootfs_dev_name[1] = rootfsMtd2;
#endif
#endif

	get_bank_info(enable,&active_bank,&backup_bank);    	
	if(enable)
	{
		//set_to mib to 1.??		
		bankmark = get_next_bankmark(Kernel_dev_name[backup_bank-1],enable);		
		ret = write_header_bankmark(Kernel_dev_name[active_bank-1], bankmark);
	}
	else //disable this
	{
		//set_to mib to 0 .??		
		ret = write_header_bankmark(Kernel_dev_name[active_bank-1], FORCEBOOT_BANK_MARK);		
	}	
	if(!ret)
	{
   	       apmib_set( MIB_DUALBANK_ENABLED, (void *)&enable);
		//fprintf(stderr,"set_dualbank enable =%d ,ret2 =%d  \n",enable,ret2); //mark_debug			
	}
	
	return ret; //-1 fail , 0 : ok
}

// need to reject this function if dual bank is disable
int  boot_from_backup()
{
	int ret =0, active_bank=0, backup_bank=0;
	unsigned long bankmark=0;	
	

	get_bank_info(1,&active_bank,&backup_bank);    

	ret = check_bank_image(backup_bank);	
	if(!ret)
	    return -2;	
	    
#ifdef MTD_NAME_MAPPING
	char linuxMtd[16],rootfsMtd[16];
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	char linuxMtd2[16],rootfsMtd2[16];
#endif
	

	if(rtl_name_to_mtdblock(FLASH_LINUX_NAME,linuxMtd) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME,rootfsMtd) == 0){
		printf("cannot mapping %s and %s\n",FLASH_LINUX_NAME,FLASH_ROOTFS_NAME);
		return 0;
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if(rtl_name_to_mtdblock(FLASH_LINUX_NAME2,linuxMtd2) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME2,rootfsMtd2) == 0){
		printf("cannot mapping %s and %s\n",FLASH_LINUX_NAME2,FLASH_ROOTFS_NAME2);
		return 0;
	}
#endif

	Kernel_dev_name[0] = linuxMtd;
	Rootfs_dev_name[0] = rootfsMtd;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE	
	Kernel_dev_name[1] = linuxMtd2;
	Rootfs_dev_name[1] = rootfsMtd2;
#endif
#endif

	bankmark = get_next_bankmark(Kernel_dev_name[active_bank-1],1);
	
	ret = write_header_bankmark(Kernel_dev_name[backup_bank-1], bankmark);

	return ret; //-2 , no kernel , -1 fail , 0 : ok}
}
#endif

int find_head_offset(char *upload_data)
{
	int head_offset=0 ;
	char *pStart=NULL;
	int iestr_offset=0;
	char *dquote;
	char *dquote1;

	if (upload_data==NULL) {
		//fprintf(stderr, "upload data is NULL\n");
		return -1;
	}

	pStart = strstr(upload_data, WINIE6_STR);
	if (pStart == NULL) {
		pStart = strstr(upload_data, LINUXFX36_FWSTR);
		if (pStart == NULL) {
			pStart = strstr(upload_data, MACIE5_FWSTR);
			if (pStart == NULL) {
				pStart = strstr(upload_data, OPERA_FWSTR);
				if (pStart == NULL) {
					pStart = strstr(upload_data, "filename=");
					if (pStart == NULL) {
						return -1;
					}
					else {
						dquote =  strstr(pStart, "\"");
						if (dquote !=NULL) {
							dquote1 = strstr(dquote, LINE_FWSTR);
							if (dquote1!=NULL) {
								iestr_offset = 4;
								pStart = dquote1;
							}
							else {
								return -1;
							}
						}
						else {
							return -1;
						}
					}
				}
				else {
					iestr_offset = 16;
				}
			} 
			else {
				iestr_offset = 14;
			}
		}
		else {
			iestr_offset = 26;
		}
	}
	else {
		iestr_offset = 17;
	}
	//fprintf(stderr,"####%s:%d %d###\n",  __FILE__, __LINE__ , iestr_offset);
	head_offset = (int)(((unsigned long)pStart)-((unsigned long)upload_data)) + iestr_offset;
	return head_offset;
}

int FirmwareUpgrade(char *upload_data, int upload_len, int is_root, char *buffer)
{
	int head_offset=0 ;
	int isIncludeRoot=0;
	int		 len;
	int          locWrite;
	int          numLeft;
	int          numWrite;
	IMG_HEADER_Tp pHeader;
	int flag=0, startAddr=-1, startAddrWeb=-1;
	int update_fw=0, update_cfg=0;
	int fh;
	//unsigned char cmdBuf[30];
	//Support WAPI/openssl, the flash MUST up to 4m
/*
#if defined(CONFIG_RTL_WAPI_SUPPORT) || defined(HTTP_FILE_SERVER_SUPPORTED) || defined(CONFIG_APP_TR069)
	int fwSizeLimit = 0x400000;
#elif defined( CONFIG_RTK_VOIP )
	int fwSizeLimit = 0x400000;
#else
	int fwSizeLimit = 0x200000;
#endif
*/
	int fwSizeLimit = CONFIG_FLASH_SIZE;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	int active_bank,backup_bank;
	int dual_enable =0;
#endif

	unsigned char isValidfw = 0;

#ifdef MTD_NAME_MAPPING
	char webpageMtd[16],linuxMtd[16],rootfsMtd[16];
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	char webpageMtd2[16],linuxMtd2[16],rootfsMtd2[16];
#endif
#endif

#if defined(CONFIG_APP_FWD)
#define FWD_CONF "/var/fwd.conf"
	int newfile = 1;
	extern int get_shm_id();
	extern int clear_fwupload_shm();
	int shm_id = get_shm_id();
#endif

#ifdef MTD_NAME_MAPPING
	if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME,webpageMtd) == 0
		|| rtl_name_to_mtdblock(FLASH_LINUX_NAME,linuxMtd) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME,rootfsMtd) == 0){
		strcpy(buffer, "cannot find webpage/linux/rootfs mtdblock!");
		goto ret_upload;
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME2,webpageMtd2) == 0
		|| rtl_name_to_mtdblock(FLASH_LINUX_NAME2,linuxMtd2) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME2,rootfsMtd2) == 0){
		strcpy(buffer, "cannot find webpage/linux/rootfs mtdblock!");
		goto ret_upload;
	}
#endif
#endif

	if (isCFG_ONLY == 0) {
		/*
		#ifdef CONFIG_RTL_8196B
			sprintf(cmdBuf, "echo \"4 %d\" > /proc/gpio", (Reboot_Wait+12));
		#else	
			sprintf(cmdBuf, "echo \"4 %d\" > /proc/gpio", (Reboot_Wait+20));
		#endif
		
			system(cmdBuf);
		*/
		system("ifconfig br0 down 2> /dev/null");
	}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	apmib_get(MIB_DUALBANK_ENABLED,(void *)&dual_enable);   
	get_bank_info(dual_enable,&active_bank,&backup_bank);        
#endif

#ifdef MTD_NAME_MAPPING
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		Kernel_dev_name[0] = linuxMtd;
		Kernel_dev_name[1] = linuxMtd2;
		Rootfs_dev_name[0] = rootfsMtd;
		Rootfs_dev_name[1] = rootfsMtd2;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE
		Web_dev_name[0] = webpageMtd;
		Web_dev_name[1] = webpageMtd2;
#endif
#endif
#endif
	head_offset = find_head_offset(upload_data);
//	fprintf(stderr,"####%s:%d head_offset=%d upload_data=%p###\n",  __FILE__, __LINE__ , head_offset, upload_data);
	if (head_offset == -1) {       
		strcpy(buffer, "Invalid file format1!");
		goto ret_upload;
	}
#ifdef DEFSETTING_AUTO_UPDATE
	system("flash save-defsetting");
#endif
	while ((head_offset+sizeof(IMG_HEADER_T)) < upload_len) {
		locWrite = 0;
		pHeader = (IMG_HEADER_Tp) &upload_data[head_offset];
		len = pHeader->len;
#ifdef _LITTLE_ENDIAN_
		len  = DWORD_SWAP(len);
#endif
		numLeft = len + sizeof(IMG_HEADER_T) ;
		// check header and checksum
		if (!memcmp(&upload_data[head_offset], FW_HEADER, SIGNATURE_LEN) ||
			!memcmp(&upload_data[head_offset], FW_HEADER_WITH_ROOT, SIGNATURE_LEN)) {
			isValidfw = 1;
			flag = 1;
		}
		else if (!memcmp(&upload_data[head_offset], WEB_HEADER, SIGNATURE_LEN)) {
			isValidfw = 1;
			flag = 2;
		}
		else if (!memcmp(&upload_data[head_offset], ROOT_HEADER, SIGNATURE_LEN)) {
			isValidfw = 1;
			flag = 3;
			isIncludeRoot = 1;
		}else if (
#ifdef COMPRESS_MIB_SETTING
				!memcmp(&upload_data[head_offset], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) ||
				!memcmp(&upload_data[head_offset], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) ||
				!memcmp(&upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)
#else
				!memcmp(&upload_data[head_offset], CURRENT_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], DEFAULT_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], HW_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&upload_data[head_offset], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) 
#endif			
				) {
			int type=0, status=0, cfg_len;
			cfg_len = updateConfigIntoFlash((unsigned char *)&upload_data[head_offset],configlen , &type, &status);

			if (status == 0 || type == 0) { // checksum error
				strcpy(buffer, "Invalid configuration file!");
				goto ret_upload;
			}
			else { // upload success
				strcpy(buffer, "Update successfully!");
				head_offset += cfg_len;
				isValidfw = 1;
				update_cfg = 1;
			}
			continue;
		}
		else {
			if (isValidfw == 1)
				break;
			strcpy(buffer, ("Invalid file format2!"));
			goto ret_upload;
		}

		if (len > fwSizeLimit) { //len check by sc_yang 
			sprintf(buffer, ("Image len exceed max size 0x%x ! len=0x%x</b><br>"),fwSizeLimit, len);
			goto ret_upload;
		}
		if ( (flag == 1) || (flag == 3)) {
			if ( !fwChecksumOk(&upload_data[sizeof(IMG_HEADER_T)+head_offset], len)) {
				sprintf(buffer, ("Image checksum mismatched! len=0x%x, checksum=0x%x</b><br>"), len,
					*((unsigned short *)&upload_data[len-2]) );
				goto ret_upload;
			}
		}
		else {
			char *ptr = &upload_data[sizeof(IMG_HEADER_T)+head_offset];
			if ( !CHECKSUM_OK((unsigned char *)ptr, len) ) {
				sprintf(buffer, ("Image checksum mismatched! len=0x%x</b><br>"), len);
				goto ret_upload;
			}
		}

#ifndef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
		if (flag == 3) // rootfs
		{
#ifndef MTD_NAME_MAPPING
			fh = open(FLASH_DEVICE_NAME1, O_RDWR);
#else
			fh = open(rootfsMtd, O_RDWR);
#endif

#if defined(CONFIG_APP_FWD)	
#ifndef MTD_NAME_MAPPING
			write_line_to_file(FWD_CONF, (newfile==1?1:2), FLASH_DEVICE_NAME1);
#else
			write_line_to_file(FWD_CONF, (newfile==1?1:2), rootfsMtd);
#endif
			newfile = 2;
#endif			
		}
		else if(flag == 1)	//linux
		{
#ifndef MTD_NAME_MAPPING
			fh = open(FLASH_DEVICE_NAME, O_RDWR);
#else
			fh = open(linuxMtd,O_RDWR);
#endif
#if defined(CONFIG_APP_FWD)			
#ifndef MTD_NAME_MAPPING
			write_line_to_file(FWD_CONF, (newfile==1?1:2), FLASH_DEVICE_NAME);
#else
			write_line_to_file(FWD_CONF, (newfile==1?1:2), linuxMtd);
#endif
			newfile = 2;
#endif			
		}
		else	//web
		{
#ifndef MTD_NAME_MAPPING
			fh = open(FLASH_DEVICE_NAME, O_RDWR);
#else
			fh = open(webpageMtd, O_RDWR);
#endif
#if defined(CONFIG_APP_FWD)			
#ifndef MTD_NAME_MAPPING
			write_line_to_file(FWD_CONF, (newfile==1?1:2), FLASH_DEVICE_NAME);
#else
			write_line_to_file(FWD_CONF, (newfile==1?1:2), webpageMtd);
#endif
			newfile = 2;
#endif			
		}
#else
		if (flag == 3) //rootfs
		{
			fh = open(Rootfs_dev_name[backup_bank-1], O_RDWR);
			
#if defined(CONFIG_APP_FWD)			
			write_line_to_file(FWD_CONF, (newfile==1?1:2), Rootfs_dev_name[backup_bank-1]);
			newfile = 2;
#endif			
		}
		else if (flag == 1) //linux
		{
			fh = open(Kernel_dev_name[backup_bank-1], O_RDWR);
#if defined(CONFIG_APP_FWD)			
			write_line_to_file(FWD_CONF, (newfile==1?1:2), Kernel_dev_name[backup_bank-1]);
			newfile = 2;
#endif			
		}
		else //web
		{
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_WEB_BACKUP_ENABLE)
			fh = open(Web_dev_name[backup_bank-1],O_RDWR);
#if defined(CONFIG_APP_FWD)			
			write_line_to_file(FWD_CONF, (newfile==1?1:2), Web_dev_name[backup_bank-1]);
			newfile = 2;
#endif			
#else		
#ifndef MTD_NAME_MAPPING
			fh = open(FLASH_DEVICE_NAME, O_RDWR);
#else
			fh = open(webpageMtd,O_RDWR);
#endif
#if defined(CONFIG_APP_FWD)			
#ifndef MTD_NAME_MAPPING
			write_line_to_file(FWD_CONF, (newfile==1?1:2), FLASH_DEVICE_NAME);
#else
			write_line_to_file(FWD_CONF, (newfile==1?1:2), webpageMtd);
#endif
			newfile = 2;
#endif	
#endif		
		}
#endif

		if ( fh == -1 ) {
			strcpy(buffer, ("File open failed!"));
			goto ret_upload;
		} else {
			if (flag == 1) {
				if (startAddr == -1) {
					//startAddr = CODE_IMAGE_OFFSET;
					startAddr = pHeader->burnAddr ;
#ifdef _LITTLE_ENDIAN_
					startAddr = DWORD_SWAP(startAddr);
#endif
#ifdef CONFIG_MTD_NAND
					startAddr = startAddr - NAND_BOOT_SETTING_SIZE;
#endif


				}
			}
			else if (flag == 3) {
				if (startAddr == -1) {
					startAddr = 0; // always start from offset 0 for 2nd FLASH partition
				}
			}
			else {
				if (startAddrWeb == -1) {
					//startAddr = WEB_PAGE_OFFSET;
					startAddr = pHeader->burnAddr ;
#ifdef _LITTLE_ENDIAN_
					startAddr = DWORD_SWAP(startAddr);
#endif
#ifdef CONFIG_MTD_NAND
					startAddr = startAddr - NAND_BOOT_SETTING_SIZE;		
#endif
				}
				else
					startAddr = startAddrWeb;
			}
			lseek(fh, startAddr, SEEK_SET);
			
#if defined(CONFIG_APP_FWD)			
			{
				char tmpStr[20]={0};
				sprintf(tmpStr,"\n%d",startAddr);
				write_line_to_file(FWD_CONF, (newfile==1?1:2), tmpStr);
				
				newfile = 2;
			}
#endif			
			
			
						
			if (flag == 3) {
				locWrite += sizeof(IMG_HEADER_T); // remove header
				numLeft -=  sizeof(IMG_HEADER_T);
				system("ifconfig br0 down 2> /dev/null");
				system("ifconfig eth0 down 2> /dev/null");
				system("ifconfig eth1 down 2> /dev/null");
				system("ifconfig ppp0 down 2> /dev/null");
				system("ifconfig wlan0 down 2> /dev/null");
				system("ifconfig wlan0-vxd down 2> /dev/null");		
				system("ifconfig wlan0-va0 down 2> /dev/null");		
				system("ifconfig wlan0-va1 down 2> /dev/null");		
				system("ifconfig wlan0-va2 down 2> /dev/null");		
				system("ifconfig wlan0-va3 down 2> /dev/null");
				system("ifconfig wlan0-wds0 down 2> /dev/null");
				system("ifconfig wlan0-wds1 down 2> /dev/null");
				system("ifconfig wlan0-wds2 down 2> /dev/null");
				system("ifconfig wlan0-wds3 down 2> /dev/null");
				system("ifconfig wlan0-wds4 down 2> /dev/null");
				system("ifconfig wlan0-wds5 down 2> /dev/null");
				system("ifconfig wlan0-wds6 down 2> /dev/null");
				system("ifconfig wlan0-wds7 down 2> /dev/null");
#if defined(CONFIG_RTL_92D_SUPPORT)	
				system("ifconfig wlan1 down 2> /dev/null");
				system("ifconfig wlan1-vxd down 2> /dev/null");		
				system("ifconfig wlan1-va0 down 2> /dev/null");		
				system("ifconfig wlan1-va1 down 2> /dev/null");		
				system("ifconfig wlan1-va2 down 2> /dev/null");		
				system("ifconfig wlan1-va3 down 2> /dev/null");
				system("ifconfig wlan1-wds0 down 2> /dev/null");
				system("ifconfig wlan1-wds1 down 2> /dev/null");
				system("ifconfig wlan1-wds2 down 2> /dev/null");
				system("ifconfig wlan1-wds3 down 2> /dev/null");
				system("ifconfig wlan1-wds4 down 2> /dev/null");
				system("ifconfig wlan1-wds5 down 2> /dev/null");
				system("ifconfig wlan1-wds6 down 2> /dev/null");
				system("ifconfig wlan1-wds7 down 2> /dev/null");
#endif
				kill_processes();
				sleep(2);
			}
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
			if (flag == 1) {  //kernel image
				pHeader->burnAddr = get_next_bankmark(Kernel_dev_name[active_bank-1],dual_enable);	//replace the firmware header with new bankmark //mark_debug		
			}
#endif

#if defined(CONFIG_APP_FWD)
			{
				char tmpStr[20]={0};
				
				sprintf(tmpStr,"\n%d",numLeft);
				write_line_to_file(FWD_CONF, (newfile==1?1:2), tmpStr);
				sprintf(tmpStr,"\n%d\n",locWrite+head_offset);
				write_line_to_file(FWD_CONF, (newfile==1?1:2), tmpStr);					
				newfile = 2;
			}

#else //#if defined(CONFIG_APP_FWD)
			numWrite = write(fh, &(upload_data[locWrite+head_offset]), numLeft);
			if (numWrite < numLeft) {
				sprintf(buffer, ("File write failed. locWrite=%d numLeft=%d numWrite=%d Size=%d bytes."), locWrite, numLeft, numWrite, upload_len);
				goto ret_upload;
			}

#endif //#if defined(CONFIG_APP_FWD)
			
			locWrite += numWrite;
			numLeft -= numWrite;
			sync();
			close(fh);

			head_offset += len + sizeof(IMG_HEADER_T) ;
			startAddr = -1 ; //by sc_yang to reset the startAddr for next image
			update_fw = 1;
		}
	} //while //sc_yang   

	//fprintf(stderr,"####isUpgrade_OK###\n");
#ifndef NO_ACTION
	isUpgrade_OK=1;
	
#if defined(CONFIG_APP_FWD)
	{			
			char tmpStr[20]={0};
			
			sprintf(tmpStr,"%d",shm_id);
			
			write_line_to_file("/var/fwd.ready", 1, tmpStr);
			
			sync();
			exit(0);
	}
#else	//#if defined(CONFIG_APP_FWD)
	REBOOT_WAIT_COMMAND(2);		
	for(;;);
#endif //#if defined(CONFIG_APP_FWD)

	
#else
#ifdef VOIP_SUPPORT
	// rock: for x86 simulation
	if (update_cfg && !update_fw) {
		if (apmib_reinit()) {
			//reset_user_profile();  // re-initialize user password
		}
	}
#endif
#endif

	return 1;
ret_upload:	
	fprintf(stderr, "%s\n", buffer);
	
#if defined(CONFIG_APP_FWD)		
	clear_fwupload_shm(shm_id);
#endif

	return 0;
}
//////////////////////////////////////////////////////////////////////////////
#if defined(CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE)
void formDualFirmware(request *wp, char *path, char *query)
{
	char *strRequest, *submitUrl, *strVal;
	unsigned char enableDualFW=0, whichBand=0;
	unsigned char tmpBuf[200];
	
	//displayPostDate(wp->post_data);
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	
	strVal = req_get_cstream_var(wp, ("active"), "");
	if(strVal[0])
	{
		if(strcmp(strVal,"save") == 0)
		{
//fprintf(stderr,"\r\n apply setting,__[%s-%u]",__FILE__,__LINE__);								
			strVal = req_get_cstream_var(wp, ("dualFw"), "");
			if (strVal[0])
			{
				enableDualFW = 1;
			}
			set_dualbank(enableDualFW);

			
		}
		else if(strcmp(strVal,"reboot") == 0)
		{

			if( boot_from_backup() == 0)
			{
			 	strcpy(tmpBuf, ("Rebooting !!~~~~Please wait for 40~50secs! "));
				 goto setReboot;
			}	
			else {
				strcpy(tmpBuf, ("Reboot Fail!!The image in Backup Bank maybe corrupted!! "));
       	              goto setErr;			
			}
			
		}
	}
	
	apmib_update_web(CURRENT_SETTING);

#ifndef NO_ACTION
	run_init_script("all");
#endif

	OK_MSG(submitUrl);		

	return;

setErr:
	ERR_MSG(tmpBuf);
	return ;

setReboot:
	ERR_MSG(tmpBuf);
	REBOOT_WAIT_COMMAND(2);		
}
#endif

#if defined(CONFIG_USBDISK_UPDATE_IMAGE)
extern int firmware_len;
extern char *firmware_data;
void formUploadFromUsb(request *wp, char * path, char * query)
{
	int oneReadMax = 4096;
	int oneRead = 0;
	int fileLen=0;
	char *buff = NULL;
	char tmpBuf[200];
	char *submitUrl;
	char lan_ip[30];	
	char lan_ip_buf[30];
    	FILE *       fd;
	struct stat fileStat={0};
	int readLen=0,i=0;	
	 if(!isFileExist(USB_UPLOAD_FORM_PATH))
	 {
      		strcpy(tmpBuf, ("Error!form ware is not exist in usb storage!\n"));
	 	goto ret_err;
	 }
	 stat(USB_UPLOAD_FORM_PATH,&fileStat);
	 fileLen=fileStat.st_size;
	fd = open(USB_UPLOAD_FORM_PATH, O_RDONLY);
	if (!fd){
      		strcpy(tmpBuf, ("Open image file  failed!\n"));
	 	goto ret_err;
	}
	lseek(fd, 0L, SEEK_SET);
	printf("		<read image from usb storage device>\n");
	buff=malloc(fileLen+17);
	if(buff == NULL)
	{
      	sprintf(tmpBuf, ("malloc %d failed !\n"),fileLen+17);
	 		goto ret_err;
	}
	bzero(buff,fileLen+17);
	
	strcpy(buff,WINIE6_STR);
	buff[13]=0x0d;
	buff[14]=0x0a;
	buff[15]=0x0d;
	buff[16]=0x0a;
	
	 printf("\n buff=%s len=%d\n",buff,17);
	readLen=read(fd,buff+17,fileLen);
	if(readLen!=fileLen)
	{
		sprintf(tmpBuf, ("read %d but file len is %d, read fail!\n"),readLen,fileLen);
	 		goto ret_err;
	}
#if 0
	/* read image from file to buff */
	 do{
		 buff = realloc(buff, fileLen + oneReadMax);
		 if(buff == NULL)
		 {
      			strcpy(tmpBuf, ("my god breallco failed !\n"));
	 		goto ret_err;
		 }
		oneRead = read(fd, (void *)(buff + fileLen), oneReadMax);
		fileLen += oneRead;
		printf(".");
		if(oneRead == -1)
		{
			printf("file read error!\n");
	 		goto ret_err;
		 }
	 }while(oneRead == oneReadMax);
	 printf("\n");

	free(wp->post_data);
	wp->post_data = buff;
	wp->post_data_len = fileLen;
#endif
	wp->upload_data=buff;
		
	wp->upload_len=fileLen+17;
#if defined(CONFIG_APP_FWD)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
	extern int get_shm_id();
	extern set_shm_id(int id);
	extern int clear_fwupload_shm();
	extern char *shm_memory;
	int shm_id = get_shm_id();	
	clear_fwupload_shm(shm_id);
	//if(shm_id == 0)
	{
		/* Generate a System V IPC key */ 
		key_t key;
		key = ftok("/bin/fwd", 0xE04);
				
		/* Allocate a shared memory segment */
		shm_id = shmget(key, wp->upload_len, IPC_CREAT | IPC_EXCL | 0666);	
					 
		if (shm_id == -1) {
					
			return -2;
		}
		set_shm_id(shm_id);
		/* Attach the shared memory segment */
		shm_memory = (char *)shmat(shm_id, NULL, 0);
		//printf("%s:%d get_shm_id()=%d shm_memory=%p\n",__FUNCTION__,__LINE__,get_shm_id(),shm_memory);
		memcpy(shm_memory,wp->upload_data,wp->upload_len);
		free(wp->upload_data);
		wp->upload_data=shm_memory;
	}
#endif
	firmware_data=wp->upload_data;
	firmware_len=wp->upload_len;

	formUpload(wp, NULL, NULL);/*further check and upload */
	return;
ret_err:
	ERR_MSG(tmpBuf);
	return;
	 
}
#endif

#ifdef CONFIG_APP_SAMBA
void formSamba(request *wp, char *path, char *query)
{
	int samba_enabled_orig, samba_enabled_new, anon_access=0;
	char *tmpStr,*submitUrl ;	
	char tmpBuf[100], cmdBuf[100];

	if(!apmib_get(MIB_SAMBA_ENABLED, (void *)&samba_enabled_orig))
		return;
	
	tmpStr = req_get_cstream_var(wp, "sambaEnabled", "");  
	//printf("[%s:%d] upnp_enabled = %s\n",__FUNCTION__,__LINE__, tmpStr);

	
	if(!strcmp(tmpStr, "ON"))
		samba_enabled_new = 1 ;
	else 
		samba_enabled_new = 0 ;

	if(samba_enabled_orig == samba_enabled_new)
		return;

	if(!apmib_set(MIB_SAMBA_ENABLED, (void *)&samba_enabled_new))
		return;

setOk_samba:
	apmib_update_web(CURRENT_SETTING);
#ifndef RTK_REINIT_SUPPORT 	
	system("killall usbStorageAppController 2>/dev/null");
	if(samba_enabled_new){
#ifdef SAMBA_WEB_SUPPORT
		apmib_get(MIB_STORAGE_ANON_ENABLE,(void*)&anon_access);
#endif
		sprintf(cmdBuf,"usbStorageAppController -a %d &",anon_access);
		system(cmdBuf);
	}	
#else
	sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
	system(cmdBuf);
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	if(submitUrl)
		OK_MSG(submitUrl);
	return;

setErr_upnp:
	ERR_MSG(tmpBuf);
	return;
}
#endif

#ifdef CONFIG_APP_MINIDLNA
#define MINIDLNA_CONF_FILE "/var/minidlna.conf"

void setMinidlnaSharedFolder(){
	int i, num;
	char tmp[10] = {0};
	char buf[100] = {0};
	MINIDLNA_SHAREFOLDER_T entry;
	if(!apmib_get(MIB_MINIDLNA_SHAREDFOLDER_TBL_NUM, (void *)&num))
		return;
	
	if(num){
		for(i = 1; i <= num; i++){
			*((char *)&entry) = (char)i;
		if(!apmib_get(MIB_MINIDLNA_SHAREDFOLDER_TBL, (void *)&entry))
			return;
		if(strlen(entry.minidlna_sharefolder_name) == 0)
			return;
		
		bzero(buf, sizeof(buf));
		bzero(tmp, sizeof(tmp));
		snprintf(buf, sizeof(buf), "%s", "media_dir=");
		if(strcmp("audio", entry.minidlna_sharedfolder_type) == 0) 
			snprintf(tmp, sizeof(tmp), "%s", "A,");
		else if(strcmp("video", entry.minidlna_sharedfolder_type) == 0) 
			snprintf(tmp, sizeof(tmp), "%s", "V,");
		else if(strcmp("images", entry.minidlna_sharedfolder_type) == 0) 
			snprintf(tmp, sizeof(tmp), "%s", "P,");
		if(strlen(tmp))
			strcat(buf,tmp);
		
		strcat(buf, entry.minidlna_sharefolder_name);

		//TODO
		//write buf to the /var/minidlna.conf	at the original media_dir=/tmp/usb/sda6/Media location
		}
	}
	
}

void formMiniDlna(request *wp, char *path, char *query)
{
	int minidlna_enabled_orig, minidlna_enabled_new;
	char *tmpStr,*submitUrl ;	
	char tmpBuf[100], cmdBuf[100];

	if(!apmib_get(MIB_MINIDLNA_ENABLED, (void *)&minidlna_enabled_orig))
		return;
	
	tmpStr = req_get_cstream_var(wp, "miniDLNAEnabled", "");  
	//printf("[%s:%d] upnp_enabled = %s\n",__FUNCTION__,__LINE__, tmpStr);

	
	if(!strcmp(tmpStr, "ON"))
		minidlna_enabled_new = 1 ;
	else 
		minidlna_enabled_new = 0 ;

	if(minidlna_enabled_orig == minidlna_enabled_new)
		return;

	if(!apmib_set(MIB_MINIDLNA_ENABLED, (void *)&minidlna_enabled_new))
		return;

setOk_samba:
	apmib_update_web(CURRENT_SETTING);
#ifndef RTK_REINIT_SUPPORT 	
	system("killall minidlna 2>/dev/null");
	if(minidlna_enabled_new){
		system("cp /etc/minidlna.conf /var/minidlna.conf");		
		//TODO : write share folder to /var/minidlna.conf	
		setMinidlnaSharedFolder();			
		system("minidlna -R -f /var/minidlna.conf");	
	}	
#else
	sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
	system(cmdBuf);
#endif

	submitUrl = req_get_cstream_var(wp, "submit-url", "");	 // hidden page
	if(submitUrl)
		OK_MSG(submitUrl);
	return;

setErr_upnp:
	ERR_MSG(tmpBuf);
	return;
}
#endif

#if 0//def SAMBA_WEB_SUPPORT
void formDiskCreateFolder(request *wp, char * path, char * query)
{
	char *submitUrl,*strLocation,*strFolder;
	char cmdBuffer[40];
	
	strLocation = req_get_cstream_var(wp,("Location"),"");
	strFolder = req_get_cstream_var(wp,("newfolder"),"");

	memset(cmdBuffer,'\0',40);
	snprintf(cmdBuffer,40,"mkdir %s/%s",strLocation,strFolder);
	system(cmdBuffer);

setOk_DiskCreateFolder:
	apmib_update_web(CURRENT_SETTING);
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;
}

void formDiskCreateShare(request *wp, char * path, char * query)
{
	char *submitUrl,*strDisplayName,*strShareAll,*strDelete,*strSelect,*strGroup;
	char *strDirNum,*strLocation;
	char cmdBuffer[50];;
	char tmpBuff[100];
	FILE	*fp;
	STORAGE_USER_T		s_user;
	STORAGE_GROUP_T		s_group;
	STORAGE_GROUP_T		s_groups[2] = {0};
	int					number,i,j;

	strDisplayName = req_get_cstream_var(wp,("displayname"),""); 
	strShareAll = req_get_cstream_var(wp,("shareall"),""); 
	strGroup = req_get_cstream_var(wp,("Group"),"");
	strDirNum = req_get_cstream_var(wp,("DirNum"),"");
	strLocation = req_get_cstream_var(wp,("Location"),"");

	if(strShareAll[0]){
		//printf("in strShareAll\n");
		memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
		*((char*)&s_group) = (char)atoi(strGroup);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
		s_group.storage_group_sharefolder_flag = 1;
		strcpy(s_group.storage_group_sharefolder,strLocation);
		strcpy(s_group.storage_group_displayname,strDisplayName);
		
		*((char*)&s_groups) = (char)atoi(strGroup);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)(s_groups));
		memcpy(&(s_groups[1]),&s_group,sizeof(STORAGE_GROUP_T));
		apmib_set(MIB_STORAGE_GROUP_MOD,(void*)s_groups);

		/*memset(cmdBuffer,'\0',50);
		snprintf(cmdBuffer,50,"chgrp %s %s",s_group.storage_group_name,strLocation);
		system(cmdBuffer);*/
		
		storage_UpdateSambaConf();
		goto setOk_DiskCreateShare;
	}

	for(i = 0;i < atoi(strDirNum);i++)
	{
		//delete Dir
		memset(cmdBuffer,'\0',50);
		snprintf(cmdBuffer,50,"delete%d",i);
		strDelete =  req_get_cstream_var(wp,(cmdBuffer),""); 
		memset(cmdBuffer,'\0',50);
		if(strDelete[0]){
			snprintf(cmdBuffer,50,"rm -rf %s",strDelete);
			system(cmdBuffer);
		}
		apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
		for(j = 0;j < number;j++)
		{
			memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
			*((char*)&s_group) = (char)(j+1);
			apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

			if(s_group.storage_group_sharefolder_flag == 1){
				if(!strcmp(s_group.storage_group_sharefolder,strDelete)){
					s_group.storage_group_sharefolder_flag = 0;
					memset(s_group.storage_group_sharefolder,'\0',MAX_FOLDER_NAME_LEN);
					memset(s_group.storage_group_displayname,'\0',MAX_DISPLAY_NAME_LEN);

					*((char*)&s_groups) = (char)(j+1);
					apmib_get(MIB_STORAGE_GROUP_TBL,(void*)(s_groups));
					memcpy(&(s_groups[1]),&s_group,sizeof(STORAGE_GROUP_T));
					apmib_set(MIB_STORAGE_GROUP_MOD,(void*)s_groups);
					break;
				}
			}
		}

		snprintf(cmdBuffer,50,"select%d",i);
		strSelect =  req_get_cstream_var(wp,(cmdBuffer),""); 
		if(!strSelect[0])
			continue;

		apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
		for(j = 0;j < number;j++)
		{
			memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
			*((char*)&s_group) = (char)(j+1);
			apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

			//printf("flag2:%d.\n",s_group.storage_group_sharefolder_flag);
			if(atoi(strGroup) == (j+1)){
				if(s_group.storage_group_sharefolder_flag == 1){
					memset(tmpBuff,'\0',100);
					strcpy(tmpBuff,"group is already have share folder");
					goto setErr_DiskCreateShare;
				}
				strcpy(s_group.storage_group_sharefolder,strSelect);
				strcpy(s_group.storage_group_displayname,strDisplayName);
				s_group.storage_group_sharefolder_flag = 1;

				*((char*)&s_groups) = (char)atoi(strGroup);
				apmib_get(MIB_STORAGE_GROUP_TBL,(void*)(s_groups));
				memcpy(&(s_groups[1]),&s_group,sizeof(STORAGE_GROUP_T));
				apmib_set(MIB_STORAGE_GROUP_MOD,(void*)s_groups);

				/*memset(cmdBuffer,'\0',50);
				snprintf(cmdBuffer,50,"chgrp %s %s",s_group.storage_group_name,strSelect);
				system(cmdBuffer);*/
				
				storage_UpdateSambaConf();
				goto setOk_DiskCreateShare;
			}
		}
	}

setOk_DiskCreateShare:
	apmib_update_web(CURRENT_SETTING);
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;
	
setErr_DiskCreateShare:
	ERR_MSG(tmpBuff);

}

void formDiskCfg(request *wp, char * path, char * query)
{
	char *submitUrl,*strLocation,*strDeleteAll,*strDeleteSelect,*strDeleteVal;
	int number,i,shareNum = 0;
	char tmpBuff[20];
	
	STORAGE_GROUP_T	s_group;
	STORAGE_GROUP_T s_groups[2];
		
	submitUrl = req_get_cstream_var(wp, "submit_url", "");	
	
	if(strcmp(submitUrl,"/storage_createsharefolder.htm")){
		strDeleteAll =  req_get_cstream_var(wp, "Delete_All", "");
		if(strDeleteAll[0]){
			apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
			for(i = 0;i < number;i++)
			{
				memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
				*((char*)&s_group) = (char)(i+1);
				apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

				if(s_group.storage_group_sharefolder_flag == 1){
					s_group.storage_group_sharefolder_flag = 0;
					memset(s_group.storage_group_sharefolder,'\0',MAX_FOLDER_NAME_LEN);
					memset(s_group.storage_group_displayname,'\0',MAX_DISPLAY_NAME_LEN);
					
					memset(s_groups,'\0',2*sizeof(STORAGE_GROUP_T));
					*((char*)s_groups) = (char)(i+1);
					apmib_get(MIB_STORAGE_GROUP_TBL,(void*)s_groups);
					memcpy(&(s_groups[1]),&s_group,sizeof(STORAGE_GROUP_T));
					apmib_set(MIB_STORAGE_GROUP_MOD,(void*)s_groups);
				}
			}
			goto setOk_DiskCfg;
		}
				
		strDeleteSelect =  req_get_cstream_var(wp, "Delete_Selected", "");
		apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
		for(i = 0;i < number;i++)
		{
			memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
			*((char*)&s_group) = (char)(i+1);
			apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

			if(s_group.storage_group_sharefolder_flag == 1){
				memset(tmpBuff,'\0',20);
				snprintf(tmpBuff,20,"delete%d",i);
				strDeleteVal = req_get_cstream_var(wp, tmpBuff, "");

				if(!strcmp(strDeleteVal,s_group.storage_group_name)){
					s_group.storage_group_sharefolder_flag = 0;
					memset(s_group.storage_group_sharefolder,'\0',MAX_FOLDER_NAME_LEN);
					memset(s_group.storage_group_displayname,'\0',MAX_DISPLAY_NAME_LEN);

					memset(s_groups,'\0',2*sizeof(STORAGE_GROUP_T));
					*((char*)s_groups) = (char)(i+1);
					apmib_get(MIB_STORAGE_GROUP_TBL,(void*)s_groups);
					memcpy(&(s_groups[1]),&s_group,sizeof(STORAGE_GROUP_T));
					apmib_set(MIB_STORAGE_GROUP_MOD,(void*)s_groups);
				}
			}
		}
		storage_UpdateSambaConf();
	}else{
		strLocation =  req_get_cstream_var(wp, "Create_Share", "");
		apmib_set(MIB_STORAGE_FOLDER_LOCAL,(void*)strLocation);
		goto setOk_DiskCfg;
	}				
setOk_DiskCfg:
	apmib_update_web(CURRENT_SETTING);
	if(submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;

}

void formDiskManagementAnon(request *wp, char * path, char * query)
{
	char *submitUrl,*strAnonDiskEnable,*strAnonEnable;
	char *strAnonFtpEnable;
	int mib_val = 0;
	char cmdBuffer[100] = {0};

	strAnonEnable = req_get_cstream_var(wp,("AnonEnabled"),"");

	if(!strcmp(strAnonEnable,"ON")){
		mib_val = 1;
		apmib_set(MIB_STORAGE_ANON_ENABLE,(void*)&mib_val);
		#if 0
		//strAnonFtpEnable = req_get_cstream_var(wp,("anonymous_ftp_enable"),""); 
		strAnonDiskEnable = req_get_cstream_var(wp,("anonymous_disk_enable"),"");		
		/*if(!strcmp(strAnonFtpEnable,"enabled")){
			mib_val = 1;
			apmib_set(MIB_STORAGE_ANON_FTP_ENABLE,(void*)&mib_val);
		}else{
			mib_val = 0;
			apmib_set(MIB_STORAGE_ANON_FTP_ENABLE,(void*)&mib_val);
		}*/

		if(!strcmp(strAnonDiskEnable,"enabled")){
			mib_val = 1;
			apmib_set(MIB_STORAGE_ANON_DISK_ENABLE,(void*)&mib_val);
		}else{
			mib_val = 0;
			apmib_set(MIB_STORAGE_ANON_DISK_ENABLE,(void*)&mib_val);
		}
		#endif
	}else{
		mib_val = 0;
		apmib_set(MIB_STORAGE_ANON_ENABLE,(void*)&mib_val);
		
		apmib_set(MIB_STORAGE_ANON_DISK_ENABLE,(void*)&mib_val);
		//apmib_set(MIB_STORAGE_ANON_FTP_ENABLE,(void*)&mib_val);
	}

setOk_AnonAccessCfg:
	apmib_update_web(CURRENT_SETTING);
	system("killall usbStorageAppController 2>/dev/null");
	snprintf(cmdBuffer,100,"usbStorageAppController -a %d &",mib_val);
	system(cmdBuffer);
	//storage_UpdateSambaConf();
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;

}

void formDiskManagementUser(request *wp, char * path, char * query)
{
	char *submitUrl, *strDeleAll,*strVal,*strUserIndex;
	char tmpBuff[20];
	char cmdBuffer[30];
	int number,i;
	STORAGE_USER_T	s_user;
	int	index;
	
	submitUrl = req_get_cstream_var(wp, "submit_url", "");
	if(strcmp(submitUrl,"/storage_edituser.htm")){
		apmib_get(MIB_STORAGE_USER_TBL_NUM,(void*)&number);
		strDeleAll = req_get_cstream_var(wp,("Delete_All"),"");
		if(strDeleAll[0]){
		
			for(i = 0; i < number;i++)
			{
				memset(&s_user,'\0',sizeof(STORAGE_USER_T));
				*((char*)&s_user) = (char)(i+1);
				apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
			
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"deluser %s",s_user.storage_user_name);
				system(cmdBuffer);
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"smbpasswd -del %s",s_user.storage_user_name);
				system(cmdBuffer);
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"rm -rf /home/%s",s_user.storage_user_name);
				system(cmdBuffer);
			}

			apmib_set(MIB_STORAGE_USER_DELALL,(void*)&s_user);
			goto setOk_deleteUser;
		}


		for(i = number;i > 0;i--)
		{
			memset(tmpBuff,'\0',20);
			snprintf(tmpBuff, 20, "select%d", i);
			strVal =  req_get_cstream_var(wp,tmpBuff,"");

			if(!strcmp(strVal,"ON")){
				*((char*)&s_user) = (char)i;
				apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
	
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"deluser %s",s_user.storage_user_name);
				system(cmdBuffer);
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"smbpasswd -del %s",s_user.storage_user_name);
				system(cmdBuffer);
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"rm -rf /home/%s",s_user.storage_user_name);
				system(cmdBuffer);
			
				apmib_set(MIB_STORAGE_USER_DEL,(void*)&s_user);
			}	
		}
	}else{
		strUserIndex = req_get_cstream_var(wp, "userindex", "");
		index = atoi(strUserIndex);
		if(strUserIndex[0])
			apmib_set(MIB_STORAGE_USER_EDIT_INDEX,(void*)&index);
		goto setOk_deleteUser;
	}
	
setOk_deleteUser:
	apmib_update_web(CURRENT_SETTING);
	
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;
}

void formDiskManagementGroup(request *wp, char * path, char * query)
{
	char *submitUrl, *strDeleAll,*strVal,*strGroupIndex;
	char tmpBuff[20];
	char cmdBuffer[30];
	int number,i,user_num,j;
	STORAGE_GROUP_T	s_group;
	STORAGE_USER_T	s_user;
	STORAGE_USER_T	s_users[2] = {0};
	int			 index;
	submitUrl = req_get_cstream_var(wp, "submit_url", "");
		
	if(strcmp(submitUrl,"/storage_editgroup.htm")){
		apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
		apmib_get(MIB_STORAGE_USER_TBL_NUM,(void*)&user_num);

		/*Delete All Group Process*/
		strDeleAll = req_get_cstream_var(wp,("Delete_All"),"");
		if(strDeleAll[0]){
			for(i = 0;i < user_num;i++)
			{
				memset(&s_user,'\0',sizeof(STORAGE_USER_T));
				*((char*)&s_user) = (char)(i+1);
				apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
				
				strcpy(s_user.storage_user_group,"--");
				*((char*)s_users) = (char)(i+1);
				apmib_get(MIB_STORAGE_USER_TBL,(void*)(s_users));
				memcpy(&(s_users[1]),&s_user,sizeof(STORAGE_USER_T));
				apmib_set(MIB_STORAGE_USER_MOD,(void*)s_users);

				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"deluser %s",s_user.storage_user_name);
				system(cmdBuffer);
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"rm -rf  /home/%s",s_user.storage_user_name);
				system(cmdBuffer);
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"adduser %s",s_user.storage_user_name);
				system(cmdBuffer);
				//may be need modify

			}

			for(i = 0;i < number;i++)
			{
				memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
				*((char*)&s_group) = (char)(i+1);
				apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
					
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"delgroup %s",s_group.storage_group_name);
				system(cmdBuffer);

			}
			
			apmib_set(MIB_STORAGE_GROUP_DELALL,(void*)&s_group);
			storage_UpdateSambaConf();
			goto setOk_deleteGroup;
		}
	

		/*Delete Selected Group Process*/
		for(i = number;i > 0;i--)
		{
			memset(tmpBuff,'\0',20);
			snprintf(tmpBuff, 20, "select%d", i);
			strVal =  req_get_cstream_var(wp,tmpBuff,"");

			if(!strcmp(strVal,"ON")){
				*((char*)&s_group) = (char)i;
				apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
				apmib_set(MIB_STORAGE_GROUP_DEL,(void*)&s_group);

				
				memset(cmdBuffer,'\0',30);
				snprintf(cmdBuffer,30,"delgroup %s",s_group.storage_group_name);
				system(cmdBuffer);

				//apmib_get(MIB_STORAGE_USER_TBL_NUM,(void*)&user_num);
				for(j = 0;j < user_num;j++)
				{
					*((char*)&s_user) = (char)(j+1);
					apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
					
					if(!strcmp(s_user.storage_user_group,s_group.storage_group_name)){
						memset(s_user.storage_user_group,'\0',10);
						strcpy(s_user.storage_user_group,"--");
						*((char*)s_users) = (char)(j+1);
						apmib_get(MIB_STORAGE_USER_TBL,(void*)(s_users));
						memcpy(&(s_users[1]),&s_user,sizeof(STORAGE_USER_T));
						apmib_set(MIB_STORAGE_USER_MOD,(void*)s_users);

						memset(cmdBuffer,'\0',30);
						snprintf(cmdBuffer,30,"deluser %s",s_user.storage_user_name);
						system(cmdBuffer);
						memset(cmdBuffer,'\0',30);
						snprintf(cmdBuffer,30,"rm -rf  /home/%s",s_user.storage_user_name);
						system(cmdBuffer);
						memset(cmdBuffer,'\0',30);
						snprintf(cmdBuffer,30,"adduser %s",s_user.storage_user_name);
						system(cmdBuffer);
						//may be need modify
					}
				}
						
			}
		}
		storage_UpdateSambaConf();
		goto setOk_deleteGroup;
	}else{
		strGroupIndex = req_get_cstream_var(wp, "groupindex", "");
		index = atoi(strGroupIndex);
		if(strGroupIndex[0]){
			apmib_set(MIB_STORAGE_GROUP_EDIT_INDEX,(void*)&index);
		}
		goto setOk_deleteGroup;
	}
		
setOk_deleteGroup:
	apmib_update_web(CURRENT_SETTING);
			
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;
}

void formDiskCreateUser(request *wp, char * path, char * query)
{
		char *submitUrl, *strName, *strPasswd, *strConfPasswd,*strGroup;
		char tmpBuff[100];
		char cmdBuffer[100];
		STORAGE_USER_T	s_user;
		STORAGE_GROUP_T	s_group;
		
		unsigned char number,i;

		strName = req_get_cstream_var(wp,("username"),"");
		strPasswd = req_get_cstream_var(wp,("newpass"),"");
		strConfPasswd = req_get_cstream_var(wp,("confpass"),"");
		strGroup = req_get_cstream_var(wp,("Group"),"");

		if(!strName[0]){
			strcpy(tmpBuff, ("userName should not be NULL!"));
			goto setErr_createUser;
		}

		if(!strPasswd[0] || !strConfPasswd[0]){
			strcpy(tmpBuff, ("passwd or confpasswd should not be NULL!"));
			goto setErr_createUser;
		}

		if(strcmp(strPasswd,strConfPasswd)){
			strcpy(tmpBuff, ("passwd should be equal to confpasswd"));
			goto setErr_createUser;
		}

		if(!strcmp(strName,"root") || !strcmp(strName,"nobody")){
			strcpy(tmpBuff,"user Name should not be nobody or root");
			goto setErr_createUser;
		}
		
		apmib_get(MIB_STORAGE_USER_TBL_NUM,(void*)&number);
		if(number >= MAX_USER_NUM){
			snprintf(tmpBuff,100,"user num shoule not be more than %d",MAX_USER_NUM);
			goto setErr_createUser;
		}
		
		for(i = 0;i <number;i++)
		{
			memset(&s_user,'\0',sizeof(STORAGE_USER_T));
			*((char*)&s_user) = (char)(i+1);
			apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);

			if(strlen(s_user.storage_user_name) == strlen(strName)
				&& strncmp(s_user.storage_user_name,strName,strlen(strName))){
				strcpy(tmpBuff, ("user name is already exist,Please choose another user name"));
				goto setErr_createUser;
			}
		}

		*((char*)&s_group) = (char)(atoi(strGroup));
		if(atoi(strGroup) == 0){
			memset(&s_user,'\0',sizeof(STORAGE_USER_T));
			strcpy(s_user.storage_user_group,"--");	
		}else{
			*((char*)&s_group) = (char)(atoi(strGroup));
			apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
			memset(&s_user,'\0',sizeof(STORAGE_USER_T));
			strcpy(s_user.storage_user_group,s_group.storage_group_name);
		}	
		
		number++;
		apmib_set(MIB_STORAGE_USER_TBL_NUM,(void*)&number);
		
		strcpy(s_user.storage_user_name,strName);
		strcpy(s_user.storage_user_password,strPasswd);
		apmib_set(MIB_STORAGE_USER_ADD,(void*)&s_user);

		memset(cmdBuffer,'\0',100);
		if(atoi(strGroup) == 0){
			snprintf(cmdBuffer,100,"adduser %s",strName);
		}else{
			snprintf(cmdBuffer,100,"adduser -G %s %s",s_group.storage_group_name,strName);
		}

		system(cmdBuffer);
		memset(cmdBuffer,'\0',100);
		snprintf(cmdBuffer,100,"smbpasswd %s %s",strName,strPasswd);
		system(cmdBuffer);
	
	setOk_createUser:
		apmib_update_web(CURRENT_SETTING);
	
		submitUrl = req_get_cstream_var(wp, "submit-url", "");	
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
		return;
	
	setErr_createUser:
		ERR_MSG(tmpBuff);
}


void formDiskEditUser(request *wp, char * path, char * query)
{
	char 	*submitUrl, *strOrigPasswd,*strNewPasswd,*strConfPasswd,*strGroup;
	int	    index;
	char	tmpBuff[100];
	char	cmdBuffer[50];
	
	STORAGE_USER_T 	s_user;
	STORAGE_GROUP_T	s_group;
	STORAGE_USER_T	s_users[2] = {0};

	memset(tmpBuff,'\0',100);
	memset(cmdBuffer,'\0',50);
	strOrigPasswd = req_get_cstream_var(wp,("origpass"),"");
	strNewPasswd = req_get_cstream_var(wp,("newpass"),"");
	strConfPasswd = req_get_cstream_var(wp,("confpass"),"");
	strGroup = req_get_cstream_var(wp,("Group"),"");

	apmib_get(MIB_STORAGE_USER_EDIT_INDEX,(void*)&index);
	memset(&s_user,'\0',sizeof(STORAGE_USER_T));
	*((char*)&s_user) = (char)index;
	apmib_get(MIB_STORAGE_USER_TBL,(void*)&s_user);
	
	if(strcmp(strOrigPasswd,s_user.storage_user_password)){
		strcpy(tmpBuff,"Orig Password is wrong,Please Enter the password again");
		goto setError_EditUser;
	}
	if(!strNewPasswd[0] || !strConfPasswd[0]){
		strcpy(tmpBuff,"newpassword or confpassword should not be empty");
		goto setError_EditUser;
	}
	if(strcmp(strNewPasswd,strConfPasswd)){
		strcpy(tmpBuff,"newpassword is not equal confpassword");
		goto setError_EditUser;
	}

	strcpy(s_user.storage_user_password,strNewPasswd);	
	if(atoi(strGroup) == 0)
		strcpy(s_user.storage_user_group,"--");
	else{
		memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
		*((char*)&s_group) = (char)atoi(strGroup);
		apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
		strcpy(s_user.storage_user_group,s_group.storage_group_name);
	}

	*((char*)s_users) = (char)index;
	apmib_get(MIB_STORAGE_USER_TBL,(void*)s_users);
	memcpy(&(s_users[1]),&s_user,sizeof(STORAGE_USER_T));
	apmib_set(MIB_STORAGE_USER_MOD,(void*)s_users);

	snprintf(cmdBuffer,50,"smbpasswd -del %s",s_user.storage_user_name);
	system(cmdBuffer);
	memset(cmdBuffer,'\0',50);
	snprintf(cmdBuffer,50,"smbpasswd %s %s",s_user.storage_user_name,s_user.storage_user_password);
	system(cmdBuffer);
	
setOk_EditUser:
	apmib_update_web(CURRENT_SETTING);
	storage_UpdateSambaConf();
		
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;	

setError_EditUser:
	ERR_MSG(tmpBuff);
}

void formDiskEditGroup(request *wp, char * path, char * query)
{
	char 	*submitUrl, *strAccess;
	int		 index;
	
	STORAGE_GROUP_T s_group;
	STORAGE_GROUP_T	s_groups[2] = {0};
	strAccess = req_get_cstream_var(wp,("Access"),"");

	apmib_get(MIB_STORAGE_GROUP_EDIT_INDEX,(void*)&index);
	memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
	*((char*)&s_group) = (char)index;
	apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);
	strcpy(s_group.storage_group_access,strAccess);

	*((char*)s_groups) = (char)index;
	apmib_get(MIB_STORAGE_GROUP_TBL,(void*)s_groups);
	memcpy(&(s_groups[1]),&s_group,sizeof(STORAGE_GROUP_T));
	apmib_set(MIB_STORAGE_GROUP_MOD,(void*)s_groups);

setOk_EditGroup:
	apmib_update_web(CURRENT_SETTING);
	storage_UpdateSambaConf();
		
	submitUrl = req_get_cstream_var(wp, "submit-url", "");	
	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
	return;	
}


void formDiskCreateGroup(request *wp, char * path, char * query)
{
		char *submitUrl, *strName,*strAccess;
		char tmpBuff[100];
		char cmdBuffer[100];
		STORAGE_GROUP_T	s_group;
		unsigned char	 number,i;
	
		strName = req_get_cstream_var(wp,("groupname"),"");
		strAccess = req_get_cstream_var(wp,("Access"),"");

		if(!strcmp(strName,"root") || !strcmp(strName,"nobody")){
			strcpy(tmpBuff,"group name should not be root or nobody");
			goto setErr_createGroup;
		}
			
		apmib_get(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
		
		if(number >= MAX_GROUP_NUM){
			snprintf(tmpBuff,100,"group num shoule not be more than %d",MAX_GROUP_NUM);
			goto setErr_createGroup;
		}

		for(i = 0;i <number;i++)
		{
			memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
			*((char*)&s_group) = (char)(i+1);
			apmib_get(MIB_STORAGE_GROUP_TBL,(void*)&s_group);

			if(!strcmp(strName,s_group.storage_group_name)){
				strcpy(tmpBuff,("group name repeat"));
				goto setErr_createGroup;
			}

			if(strlen(s_group.storage_group_name) == strlen(strName)
				&& strncmp(s_group.storage_group_name,strName,strlen(strName))){
				strcpy(tmpBuff, ("group name is already exist,Please choose another group name"));
				goto setErr_createGroup;
			}
		}
	
		number++;
		apmib_set(MIB_STORAGE_GROUP_TBL_NUM,(void*)&number);
	
		memset(&s_group,'\0',sizeof(STORAGE_GROUP_T));
		strcpy(s_group.storage_group_name,strName);
		strcpy(s_group.storage_group_access,strAccess);
		s_group.storage_group_sharefolder_flag = 0;
		apmib_set(MIB_STORAGE_GROUP_ADD,(void*)&s_group);

		memset(cmdBuffer,'\0',100);
		snprintf(cmdBuffer,100,"addgroup %s",strName);
		system(cmdBuffer);
		
	setOk_createGroup:
		apmib_update_web(CURRENT_SETTING);
		
		submitUrl = req_get_cstream_var(wp, "submit-url", "");	
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
		return;
		
	setErr_createGroup:
		ERR_MSG(tmpBuff);
}
#endif
#if defined(CONFIG_APP_ZIGBEE)
int zigbee_dev_list(request *wp, int argc, char **argv)
{
	int nBytesSent=0, i, found=0;
	FILE* fp;
	char buffer[128] = {0};	
    char u64IEEEAddress[64];
	char status[16] = {0};
	int brightness[16] = {0};
	system("zigbeed light_status");
	sleep(1);
	fp = fopen("/tmp/zigbee_dev_list","r");
	if(fp)
	{
        while(fgets(buffer,sizeof(buffer),fp))
		{
			if(!strncmp(buffer,"IEEE Address",strlen("IEEE Address")))
			{
				strcpy(u64IEEEAddress,buffer+strlen("IEEE Address: "));
			}
			if(!strncmp(buffer,"status",strlen("status")))
			{
				strcpy(status,buffer+strlen("status:"));
				if(atoi(status)==1)
				{
					strcpy(status,"On");
				}
				else if(atoi(status)==0)
				{
					strcpy(status,"Off");
				}
			}
			if(!strncmp(buffer,"brightness",strlen("brightness")))
			{
				strcpy(brightness,buffer+strlen("brightness:"));
			}
			else
			{
				continue;
			}
        }
		nBytesSent += req_format_write(wp,	
			("<tr class=\"tbl_body\"><td><font size=2>%s</td>"
		"<td><font size=2>%s</td>"
		"<td><font size=2>%s</td>"
		"</tr>"),u64IEEEAddress,status,brightness);
		fclose(fp);
	}
	else
	{
		nBytesSent += req_format_write(wp,
	   		("<tr class=\"tbl_body\"><td><font size=2>---</td>"
			"<td><font size=2>---</td>"
	     		"<td><font size=2>---</td>"
			"</tr>"));
	}
	return nBytesSent;
}
void formZigBee(request *wp, char *path, char *query)
{
	char *submitUrl;
	char *strRequest;
	char *refresh;
	char *zigbeeCmd;
	char *brightness;
	char cmd[64] = {0};
	submitUrl = req_get_cstream_var(wp, ("submit-url"), "");
	strRequest = req_get_cstream_var(wp, ("zigbee_request"), "");
	zigbeeCmd = req_get_cstream_var(wp, ("ZigBeeCmd"), "");
	refresh = req_get_cstream_var(wp, ("refresh"), "");
	if(refresh[0])
	{
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
	}
	else if(strRequest[0])
	{
		if(!strcmp(zigbeeCmd,"0"))//off
		{
			system("zigbeed light_off");
		}
		if(!strcmp(zigbeeCmd,"1"))//on
		{
			system("zigbeed light_on");
		}
		if(!strcmp(zigbeeCmd,"2"))//set brightness
		{
			brightness = req_get_cstream_var(wp, ("brightness"), "");
			sprintf(cmd,"zigbeed set_brightness %s",brightness);
			system(cmd);
		}
		if(!strcmp(zigbeeCmd,"3"))//permit join
		{
			system("zigbeed permit_join");
		}
		if (submitUrl[0])
			send_redirect_perm(wp, submitUrl);
	}
}

#endif
void formUpload(request *wp, char * path, char * query)
{
	//int fh;
	int len;
	int locWrite;
	int numLeft;
	//int numWrite;
	IMG_HEADER_Tp pHeader;
	char tmpBuf[200];
#ifndef REBOOT_CHECK
	char lan_ip_buf[30];
	char lan_ip[30];
#endif
	char *submitUrl;
	int flag=0, startAddr=-1;
	int isIncludeRoot=0;
#ifndef NO_ACTION
	//int pid;
#endif
	int head_offset=0;
	int update_fw=0, update_cfg=0;
	//Support WAPI/openssl, the flash MUST up to 4m
/*
#if defined(CONFIG_RTL_WAPI_SUPPORT) || defined(HTTP_FILE_SERVER_SUPPORTED)
	int fwSizeLimit = 0x400000;
#elif defined( CONFIG_RTK_VOIP )
	int fwSizeLimit = 0x400000;
#else
	int fwSizeLimit = 0x200000;
#endif
*/
	int fwSizeLimit = CONFIG_FLASH_SIZE;
	unsigned char isValidfw = 0;

#if defined(CONFIG_APP_FWD)
#define FWD_CONF "/var/fwd.conf"
	int newfile = 1;
	extern int get_shm_id();
	extern int clear_fwupload_shm();
	int shm_id = get_shm_id();	
#endif

#ifdef CONFIG_APP_TR069
    /*   
     * According to TR-098 spec of InternetGatewayDevice.ManagementServer.UpgradesManaged:
     *      Indicates whether or not the ACS will manage upgrades for the CPE. 
     *      If true(1), the CPE SHOULD NOT use other means other than the ACS to seek out available upgrades. 
     *      If false(0), the CPE MAY use other means for this purpose.
     */
    int value = 0;
    apmib_get(MIB_CWMP_ACS_UPGRADESMANAGED, (void *)&value);
    if(1==value)
    {
    	printf("[%s:%d] firmware upgrade fail due to MIB_CWMP_ACS_UPGRADESMANAGED is 1.\n", __FUNCTION__, __LINE__);
        snprintf(tmpBuf, sizeof(tmpBuf), ("%s"), "Update Failed!<br><br>ACS has take control of device upgrade and ONLY ACS can upgrade device!<br>");
        goto ret_upload;
    }
#endif

#ifndef REBOOT_CHECK
	apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
	sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
#endif
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");
	//fprintf(stderr,"####%s:%d submitUrl=%s###\n",  __FILE__, __LINE__ , submitUrl);
	//support multiple image
	head_offset = find_head_offset((char *)wp->upload_data);
	//fprintf(stderr,"####%s:%d %d wp->upload_data=%p###\n",  __FILE__, __LINE__ , head_offset, wp->upload_data);
	//fprintf(stderr,"####%s:%d content_length=%s###contenttype=%s###\n",  __FILE__, __LINE__ ,wp->content_length , wp->content_type);
	if (head_offset == -1) {
		strcpy(tmpBuf, "<b>Invalid file format3!");
		goto ret_upload;
	}
	while ((head_offset+sizeof(IMG_HEADER_T)) <  wp->upload_len) {
		locWrite = 0;
		pHeader = (IMG_HEADER_Tp) &wp->upload_data[head_offset];
		len = pHeader->len;
#ifdef _LITTLE_ENDIAN_
		len  = DWORD_SWAP(len);
#endif    
		numLeft = len + sizeof(IMG_HEADER_T);
		// check header and checksum
		if (!memcmp(&wp->upload_data[head_offset], FW_HEADER, SIGNATURE_LEN) ||
		    !memcmp(&wp->upload_data[head_offset], FW_HEADER_WITH_ROOT, SIGNATURE_LEN)) {
		    	isValidfw = 1;
			flag = 1;
			//Reboot_Wait = Reboot_Wait+ 50;
		} else if (!memcmp(&wp->upload_data[head_offset], WEB_HEADER, SIGNATURE_LEN)) {
			isValidfw = 1;
			flag = 2;
			//Reboot_Wait = Reboot_Wait+ 40;
		} else if (!memcmp(&wp->upload_data[head_offset], ROOT_HEADER, SIGNATURE_LEN)) {
			isValidfw = 1;
			flag = 3;
			//Reboot_Wait = Reboot_Wait+ 60;
			isIncludeRoot = 1;	
		}else if ( 
#ifdef COMPRESS_MIB_SETTING
				!memcmp(&wp->upload_data[head_offset], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) ||
				!memcmp(&wp->upload_data[head_offset], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) ||
				!memcmp(&wp->upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)
#else
				!memcmp(&wp->upload_data[head_offset], CURRENT_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], CURRENT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], CURRENT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], DEFAULT_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], DEFAULT_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], DEFAULT_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN) 
#endif
			) {
#if 1
			strcpy(tmpBuf, ("<b>Invalid file format! Should upload fireware but not config dat!"));
			goto ret_upload;
#else
#ifdef COMPRESS_MIB_SETTING
				COMPRESS_MIB_HEADER_Tp pHeader_cfg;
				pHeader_cfg = (COMPRESS_MIB_HEADER_Tp)&wp->upload_data[head_offset];
				if(!memcmp(&wp->upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)) {
					head_offset +=  pHeader_cfg->compLen+sizeof(COMPRESS_MIB_HEADER_T);
					configlen = head_offset;
				}
				else {
					head_offset +=  pHeader_cfg->compLen+sizeof(COMPRESS_MIB_HEADER_T);
				}
#else
#ifdef HEADER_LEN_INT
				if(!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_FORCE_TAG, TAG_LEN) ||
				!memcmp(&wp->upload_data[head_offset], HW_SETTING_HEADER_UPGRADE_TAG, TAG_LEN))
				{
					HW_PARAM_HEADER_Tp phwHeader_cfg;
					phwHeader_cfg = (HW_PARAM_HEADER_Tp)&wp->upload_data[head_offset];
					head_offset +=  phwHeader_cfg->len+sizeof(HW_PARAM_HEADER_T);
				}
				else
#endif
				{
					PARAM_HEADER_Tp pHeader_cfg;
					pHeader_cfg = (PARAM_HEADER_Tp)&wp->upload_data[head_offset];
					head_offset +=  pHeader_cfg->len+sizeof(PARAM_HEADER_T);
				}
#endif
				isValidfw = 1;
				update_cfg = 1;
				continue;
#endif
		}
		else {
			if (isValidfw == 1)
				break;
			strcpy(tmpBuf, "<b>Invalid file format4!");
			goto ret_upload;
		}

		if (len > fwSizeLimit) { //len check by sc_yang
			sprintf(tmpBuf, "<b>Image len exceed max size 0x%x ! len=0x%x</b><br>",fwSizeLimit, len);
			goto ret_upload;
		}
#ifdef CONFIG_RTL_WAPI_SUPPORT
		if((flag == 3) && (len>WAPI_AREA_BASE)) {
			sprintf(tmpBuf, "<b>Root image len 0x%x exceed 0x%x which will overwrite wapi area at flash ! </b><br>", len, WAPI_AREA_BASE);
			goto ret_upload;
		}
#endif
		if ( (flag == 1) || (flag == 3)) {
			if ( !fwChecksumOk((char *)&wp->upload_data[sizeof(IMG_HEADER_T)+head_offset], len)) {
				sprintf(tmpBuf, "<b>Image checksum mismatched! len=0x%x, checksum=0x%x</b><br>", len,
					*((unsigned short *)&wp->upload_data[len-2]) );
				goto ret_upload;
			}
		}
		else {
			char *ptr = (char *)&wp->upload_data[sizeof(IMG_HEADER_T)+head_offset];
			if ( !CHECKSUM_OK((unsigned char *)ptr, len) ) {
				sprintf(tmpBuf, "<b>Image checksum mismatched! len=0x%x</b><br>", len);
				goto ret_upload;
			}
		}
#ifdef HOME_GATEWAY
#ifdef REBOOT_CHECK
		sprintf(tmpBuf, "Upload successfully (size = %d bytes)!<br><br>Firmware update in progress.", wp->upload_len);
#else
		sprintf(tmpBuf, "Upload successfully (size = %d bytes)!<br><br>Firmware update in progress.<br> Do not turn off or reboot the AP during this time.", wp->upload_len);
#endif
#else
		sprintf(tmpBuf, "Upload successfully (size = %d bytes)!<br><br>Firmware update in progress.<br> Do not turn off or reboot the AP during this time.", wp->upload_len);
#endif
		//sc_yang
		head_offset += len + sizeof(IMG_HEADER_T);
		startAddr = -1 ; //by sc_yang to reset the startAddr for next image
		update_fw = 1;
	} //while //sc_yang    

	isFWUPGRADE = 1;

#if defined(CONFIG_DOMAIN_NAME_QUERY_SUPPORT)
	Stop_Domain_Query_Process();
	WaitCountTime=2;
#endif

#if defined(CONFIG_RTL_819X)
#ifdef RTL_8367R_DUAL_BAND
	Reboot_Wait = (wp->upload_len/69633)+57+5+15;
#elif defined(RTL_8367R_8881a_DUAL_BAND)
	Reboot_Wait = (wp->upload_len/69633)+57+5+25;
#elif defined(CONFIG_RTL_8198C)
	Reboot_Wait = (wp->upload_len/19710)+50+5;
#else
	Reboot_Wait = (wp->upload_len/69633)+57+5;
#endif
	if (update_cfg==1 && update_fw==0) {
		strcpy(tmpBuf, "<b>Update successfully!");
		Reboot_Wait = (wp->upload_len/69633)+45+5;
		isCFG_ONLY= 1;
	}
#else
	Reboot_Wait = (wp->upload_len/43840)+35;
	if (update_cfg==1 && update_fw==0) {
		strcpy(tmpBuf, "<b>Update successfully!");
		Reboot_Wait = (wp->upload_len/43840)+30;
		isCFG_ONLY= 1;
	}
#endif

#ifdef REBOOT_CHECK
	sprintf(lastUrl,"%s","/status.htm");
	sprintf(okMsg,"%s",tmpBuf);
	countDownTime = Reboot_Wait;
	send_redirect_perm(wp, COUNTDOWN_PAGE);
#else
	OK_MSG_FW(tmpBuf, submitUrl,Reboot_Wait,lan_ip);
#endif
	return;

ret_upload:
	
#if defined(CONFIG_APP_FWD)		
	clear_fwupload_shm(shm_id);
#endif
	Reboot_Wait=0;
	ERR_MSG(tmpBuf);
}

void formRemoteUp(request *wp, char * path, char * query)
{
    char tmpBuf[200]={0};
	sprintf(tmpBuf, "Upload successfully (size = %d bytes)!<br><br>Firmware update in progress.<br> Do not turn off or reboot the AP during this time.", wp->upload_len);
#if 0
	Reboot_Wait = (wp->upload_len/69633)+57+5;

	if (update_cfg==1 && update_fw==0) {
		strcpy(tmpBuf, "<b>Update successfully!");
		Reboot_Wait = (wp->upload_len/69633)+45+5;
		isCFG_ONLY= 1;
	}


	sprintf(lastUrl,"%s","/status.htm");
	sprintf(okMsg,"%s",tmpBuf);
	countDownTime = Reboot_Wait;
	send_redirect_perm(wp, COUNTDOWN_PAGE);

	return;

ret_upload:
	Reboot_Wait=0;
#endif
	ERR_MSG(tmpBuf);
}

/////////////////////////////////////////////////////////////////////////////
void formPasswordSetup(request *wp, char *path, char *query)
{
	char *submitUrl, *strUser, *strPassword;
	char tmpBuf[100];
	
#ifdef SUPER_NAME_SUPPORT
	char *superName = NULL;
	char *superPassword = NULL;
	
	if(wp->auth_flag == SUPER_FLAG || wp->auth_flag == NO_LOGIN) 
	{
		//check superName and newSuperPass valid
		superName=req_get_cstream_var(wp, "superName", "");
		superPassword=req_get_cstream_var(wp, "newSuperPass", "");

		if(!superName[0]){
			strcpy(tmpBuf, ("ERROR:superName cannot be empty."));
			goto setErr_pass;
		}
	
		if(superName[0] && !superPassword[0]){
			strcpy(tmpBuf, ("ERROR:superPassword cannot be empty."));
			goto setErr_pass;
		}
	}
#endif

	//check userName and newPass valid
	strUser = req_get_cstream_var(wp, "userName", "");
	strPassword = req_get_cstream_var(wp, "newPass", "");

	//if(!strUser[0])
	//{
	//	strcpy(tmpBuf, ("ERROR: userName cannot be empty."));
	//	goto setErr_pass;
	//}
	if ( !strUser[0] && strPassword[0] )
	{
		strcpy(tmpBuf, ("ERROR: userName is empty,Password should be empty."));
		goto setErr_pass;
	}
	
	if ( strUser[0] && !strPassword[0] ) {
		strcpy(tmpBuf, ("ERROR: Password cannot be empty."));
		goto setErr_pass;
	}


#ifdef SUPER_NAME_SUPPORT
	if(wp->auth_flag == SUPER_FLAG || wp->auth_flag == NO_LOGIN) 
	{
		//set MIB_SUPER_NAME and MIB_SUPER_PASSWORD 
		if ( !apmib_set(MIB_SUPER_NAME, (void *)superName) ) {
			strcpy(tmpBuf, ("ERROR: Set superName to MIB database failed."));
			goto setErr_pass;
		}
		if ( !apmib_set(MIB_SUPER_PASSWORD, (void *)superPassword) ) {
			strcpy(tmpBuf, ("ERROR: Set superPassword to MIB database failed."));
			goto setErr_pass;
		}	

		// Check if userName is the same as superName 
		if ( !apmib_get(MIB_SUPER_NAME, (void *)tmpBuf)) {
			strcpy(tmpBuf, ("ERROR: Get supervisor name MIB error!"));
			goto setErr_pass;
		}
		if ( !strcmp(strUser, tmpBuf)) {
			strcpy(tmpBuf, ("ERROR: Cannot use the same user name as supervisor."));
			goto setErr_pass;
		}

	}		
#endif	

	//set MIB_USER_NAME and MIB_USER_PASSWORD 
	if ( !apmib_set(MIB_USER_NAME, (void *)strUser) ) {
		strcpy(tmpBuf, ("ERROR: Set userName to MIB database failed."));
	//	fprintf(stderr, "###%s:%d  Set userName to MIB database failed.###\n", __FILE__, __LINE__);
		goto setErr_pass;
	}
	if ( !apmib_set(MIB_USER_PASSWORD, (void *)strPassword) ) {
		strcpy(tmpBuf, ("ERROR: Set user password to MIB database failed."));
		goto setErr_pass;
	}

	/* Retrieve next page URL */
	apmib_update_web(CURRENT_SETTING);
		
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page


#ifdef LOGIN_URL
	if (strUser[0])
		submitUrl = WEB_PAGE_LOGIN;

    delete_user(wp);
#endif

//#ifdef REBOOT_CHECK
//	{
//		char tmpMsg[300];
//		char lan_ip_buf[30], lan_ip[30];
//#ifdef RTK_REINIT_SUPPORT
//		char cmdBuf[64]={0};
//#endif		
//		sprintf(tmpMsg, "%s","<script>dw(password_change_setting)</script><br><br><script>dw(saveconf_not_turnoff_reboot)</script>");
//		apmib_get( MIB_IP_ADDR,  (void *)lan_ip_buf) ;
//		sprintf(lan_ip,"%s",inet_ntoa(*((struct in_addr *)lan_ip_buf)) );
//#ifdef RTK_REINIT_SUPPORT
//		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
//		system(cmdBuf);
//#endif
//		OK_MSG_FW(tmpMsg, submitUrl,APPLY_COUNTDOWN_TIME,lan_ip);
//#ifdef REBOOT_CHECK
//		run_init_script_flag = 1;
//#endif		
//#ifndef NO_ACTION
//		run_init_script("all");
//#endif	
//	}
//#else
//	OK_MSG(submitUrl);
//#endif
    send_redirect_perm(wp, submitUrl);
	return;

setErr_pass:
	ERR_MSG(tmpBuf);
}

////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void formStats(request *wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
}

#ifdef CONFIG_RTK_MESH
void formMeshStatus(request *wp, char *path, char *query)
{
	char *submitUrl;

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	if (submitUrl[0])
		send_redirect_perm(wp, submitUrl);
}
#endif // CONFIG_RTK_MESH
 
/////////////////////////////////////////////////////////////////////////////
int  ntpHandler(request *wp, char *tmpBuf, int fromWizard)
{
	int enabled=0, ntpServerIdx ;
	struct in_addr ipAddr ;
	char *tmpStr ;
//Brad add for daylight save	
	int dlenabled=0;
//Brad add end	
	if (fromWizard) {
		tmpStr = req_get_cstream_var(wp, ("enabled"), "");  
		if(!strcmp(tmpStr, "ON"))
			enabled = 1 ;
		else 
			enabled = 0 ;

		if ( apmib_set( MIB_NTP_ENABLED, (void *)&enabled) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_ntp;
		}
//Brad add for daylight save		
		tmpStr = req_get_cstream_var(wp, ("dlenabled"), "");  
		if(!strcmp(tmpStr, "ON"))
			dlenabled = 1 ;
		else 
			dlenabled = 0 ;

		if ( apmib_set( MIB_DAYLIGHT_SAVE, (void *)&dlenabled) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_ntp;
		}
//Brad add end		
	}	
	else
		enabled = 1;
	if(enabled){
		tmpStr = req_get_cstream_var(wp, ("ntpServerId"), "");  
		if(tmpStr[0]){
			ntpServerIdx = tmpStr[0] - '0' ;
			if ( apmib_set(MIB_NTP_SERVER_ID, (void *)&ntpServerIdx) == 0) {
				strcpy(tmpBuf, ("Set Time Zone error!"));
				goto setErr_ntp;
			}
		}
		tmpStr = req_get_cstream_var(wp, ("timeZone"), "");  
		if(tmpStr[0]){
			if ( apmib_set(MIB_NTP_TIMEZONE, (void *)tmpStr) == 0) {
					strcpy(tmpBuf, ("Set Time Zone error!"));
				goto setErr_ntp;
		}
		}

		tmpStr = req_get_cstream_var(wp, ("ntpServerIp1"), "");  
		if(tmpStr[0]){
			//inet_aton(tmpStr, &ipAddr);
			if ( apmib_set(MIB_NTP_SERVER_IP1, (void *)tmpStr) == 0) {
				strcpy(tmpBuf, ("Set NTP server error!"));
				goto setErr_ntp;
			} 
			}
		tmpStr = req_get_cstream_var(wp, ("ntpServerIp2"), "");  
		if(tmpStr[0]){
			//inet_aton(tmpStr, &ipAddr);
			if ( apmib_set(MIB_NTP_SERVER_IP2,(void *)tmpStr ) == 0) {
				strcpy(tmpBuf, ("Set NTP server IP error!"));
				goto setErr_ntp;
			}
		}
	}
	return 0 ;	
setErr_ntp:
	return -1 ;
	
}
void formNtp(request *wp, char *path, char *query)
{
	char *submitUrl,*strVal, *tmpStr;
	char tmpBuf[100];
	int enabled=0;
//Brad add for daylight save	
	int dlenabled=0;
//Brad add end	
#ifndef NO_ACTION
//	int pid;
#endif
	int time_value=0;
	int cur_year=0;
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	strVal = req_get_cstream_var(wp, ("save"), "");   
#if defined(APPLY_CHANGE_DIRECT_SUPPORT)
	if(strVal[0]==0){
		strVal = req_get_cstream_var(wp, ("save_apply"), "");
	}
#endif
	tmpStr = req_get_cstream_var(wp, ("timeZone"), "");  
	if(tmpStr[0]){
		if ( apmib_set(MIB_NTP_TIMEZONE, (void *)tmpStr) == 0) {
				strcpy(tmpBuf, ("Set Time Zone error!"));
			goto setErr_end;
		}
	}
//Brad add for daylight save		
	tmpStr = req_get_cstream_var(wp, ("dlenabled"), "");  
	if(!strcmp(tmpStr, "ON"))
		dlenabled = 1 ;
	else 
		dlenabled = 0 ;
	if ( apmib_set( MIB_DAYLIGHT_SAVE, (void *)&dlenabled) == 0) {
		strcpy(tmpBuf, ("Set dl enabled flag error!"));
		goto setErr_end;
	}
//Brad add end		
	set_timeZone();
	if(strVal[0]){		
		struct tm tm_time;
		time_t tm;
		memcpy(&tm_time, localtime(&tm), sizeof(tm_time));
		tm_time.tm_sec = 0;
		tm_time.tm_min = 0;
		tm_time.tm_hour = 0;
		tm_time.tm_isdst = -1;  /* Be sure to recheck dst. */
		strVal = req_get_cstream_var(wp, ("year"), "");	
		cur_year= atoi(strVal);
		tm_time.tm_year = atoi(strVal) - 1900;
		strVal = req_get_cstream_var(wp, ("month"), "");	
		tm_time.tm_mon = atoi(strVal)-1;
		strVal = req_get_cstream_var(wp, ("day"), "");	
		tm_time.tm_mday = atoi(strVal);
		strVal = req_get_cstream_var(wp, ("hour"), "");	
		tm_time.tm_hour = atoi(strVal);
		strVal = req_get_cstream_var(wp, ("minute"), "");	
		tm_time.tm_min = atoi(strVal);
		strVal = req_get_cstream_var(wp, ("second"), "");	
		tm_time.tm_sec = atoi(strVal);
		tm = mktime(&tm_time);
		if(tm < 0){
			sprintf(tmpBuf, "set Time Error\n");
			goto setErr_end;
		}
		if(stime(&tm) < 0){
			sprintf(tmpBuf, "set Time Error\n");
			goto setErr_end;
		}

		apmib_set( MIB_SYSTIME_YEAR, (void *)&cur_year);
		time_value = tm_time.tm_mon;
		apmib_set( MIB_SYSTIME_MON, (void *)&time_value);
		time_value = tm_time.tm_mday;
		apmib_set( MIB_SYSTIME_DAY, (void *)&time_value);
		time_value = tm_time.tm_hour;
		apmib_set( MIB_SYSTIME_HOUR, (void *)&time_value);
		time_value = tm_time.tm_min;
		apmib_set( MIB_SYSTIME_MIN, (void *)&time_value);
		time_value = tm_time.tm_sec;
		apmib_set( MIB_SYSTIME_SEC, (void *)&time_value);
		
		

		tmpStr = req_get_cstream_var(wp, ("enabled"), "");  
		if(!strcmp(tmpStr, "ON"))
			enabled = 1 ;
		else 
			enabled = 0 ;
		if ( apmib_set( MIB_NTP_ENABLED, (void *)&enabled) == 0) {
			strcpy(tmpBuf, ("Set enabled flag error!"));
			goto setErr_end;
		}

	}
	if (enabled == 0)		
		goto  set_ntp_end;
	
	if(ntpHandler(wp, tmpBuf, 0) < 0)
		goto setErr_end ;

set_ntp_end:
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
	run_init_script("all");
#endif

    time(&current_time);
    add_user(wp);
	OK_MSG(submitUrl);
	return;

setErr_end:
	ERR_MSG(tmpBuf);
}


void formPocketWizard(request *wp, char *path, char *query)
{
	char *tmpStr, *strVal;
	char tmpBuf[100];
	char varName[20];
	int i=0;
	int mode=-1;
	int val;
	int wlBandMode;
	int band2G5GSelect;
	int dns_changed=0;
    char ssidbuf[33];
#if defined(CONFIG_RTL_ULINKER)
	int ulinker_auto_changed;	
#endif

//displayPostDate(wp->post_data);

/*
	strVal = req_get_cstream_var(wp, "band0", "");
	val = strtol( strVal, (char **)NULL, 10);
	val = (val + 1);
	apmib_set( MIB_WLAN_BAND, (void *)&val);
*/
		
#if defined(CONFIG_RTL_ULINKER)
	tmpStr = req_get_cstream_var(wp, "otg_auto_val", "");
	if(tmpStr[0] != 0)
	{
		apmib_get(MIB_ULINKER_AUTO, (void *)&ulinker_auto_changed);
		val = atoi(tmpStr);
		apmib_set(MIB_ULINKER_AUTO, (void *)&val);

		if (ulinker_auto_changed != val)
			ulinker_auto_changed = 1;
		else
			ulinker_auto_changed = 0;	
	}
#endif
	
#ifdef HOME_GATEWAY
	if(tcpipWanHandler(wp, tmpBuf, &dns_changed) < 0){
		goto setErr_end;	
	}
#endif
		
#if defined(CONFIG_RTL_92D_SUPPORT)		
	tmpStr = req_get_cstream_var(wp, "wlBandMode", "");
	if(tmpStr[0] != 0)
	{
		wlBandMode = atoi(tmpStr);
		apmib_set(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlBandMode);
	}
	
	apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlBandMode);
	if(wlBandMode == BANDMODEBOTH)
	{					
		unsigned char wlanIfStr[10];				
		
		for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
		{
			unsigned char wlanif[10];
			memset(wlanif,0x00,sizeof(wlanif));
			sprintf(wlanif, "wlan%d",i);
			if(SetWlan_idx(wlanif))
			{
				int intVal;	
#if defined(CONFIG_RTL_92D_SUPPORT) && defined(CONFIG_RTL_92D_DMDP) && !defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
				intVal = DMACDPHY;
#else
				intVal = SMACSPHY;
#endif
				apmib_set(MIB_WLAN_MAC_PHY_MODE, (void *)&intVal);
				intVal = 0;
				apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&intVal);
			}						
		}
	
		/* 92d rule, 5g must up in wlan0 */
		/* phybandcheck */
		if(whichWlanIfIs(PHYBAND_5G) != 0)
		{
			swapWlanMibSetting(0,1);			
		}
		
		
	}
	else if(wlBandMode == BANDMODESINGLE)
	{
		unsigned int wlanif;
		
		for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
		{
			unsigned char wlanif[10];
			memset(wlanif,0x00,sizeof(wlanif));
			sprintf(wlanif, "wlan%d",i);
			if(SetWlan_idx(wlanif))
			{
				int intVal;
				intVal = SMACSPHY;
				apmib_set(MIB_WLAN_MAC_PHY_MODE, (void *)&intVal);
				intVal = 1;
				apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&intVal);
			}						
		}
		
		tmpStr = req_get_cstream_var(wp, "Band2G5GSupport", ""); //wlan0 PHYBAND_TYPE
		if(tmpStr[0] != 0)
		{
			band2G5GSelect = atoi(tmpStr);			
		}
		
		wlanif = whichWlanIfIs(band2G5GSelect);
		
		/* 92d rule, 5g must up in wlan0 */
		/* phybandcheck */
		if(wlanif != 0)
		{
			swapWlanMibSetting(0,1);			
		}
		
		wlan_idx = 0 ;		
		
		val = 0;
		apmib_set( MIB_WLAN_WLAN_DISABLED, (void *)&val); // enable wlan0 and disable wlan1
		
		
	}
#endif //#if defined(CONFIG_RTL_92D_SUPPORT)
				
	wlan_idx = 0 ;
	tmpStr = req_get_cstream_var(wp, "pocket_ssid", "");	
            //strSSID is BSSID, below transfer it to SSID. This fix the issue of AP ssid contains "
           
       
	if(tmpStr[0] != 0){
		if( bssid_to_ssid(tmpStr,ssidbuf) < 0)
			apmib_set(MIB_WLAN_SSID, (void *)tmpStr);
		else
			apmib_set(MIB_WLAN_SSID, (void *)ssidbuf);
	}
		
	for(i = 0 ; i<NUM_WLAN_INTERFACE ; i++)
	{
		wlan_idx = i;
		vwlan_idx = 0;
		
		if(i == 1)
		{
			if(wlBandMode != BANDMODEBOTH) // single band, no need process wlan1
				continue;
				
			tmpStr = req_get_cstream_var(wp, "pocket_ssid1", "");
			if(tmpStr[0] != 0){
				if( bssid_to_ssid(tmpStr,ssidbuf) < 0)
					apmib_set(MIB_WLAN_SSID, (void *)tmpStr);
				else
					apmib_set(MIB_WLAN_SSID, (void *)ssidbuf);
			}
		}
		sprintf(varName, "mode%d", i);
		tmpStr = req_get_cstream_var(wp, varName, "");
		if(tmpStr[0])
		{
			val = atoi(tmpStr);
			apmib_set( MIB_WLAN_MODE, (void *)&val);
		}
		
		
		
		sprintf(varName, "method%d", i);
		tmpStr = req_get_cstream_var(wp, varName, "");
		if(tmpStr[0])
		{
			val = atoi(tmpStr);
			if(val == ENCRYPT_DISABLED)
			{
				ENCRYPT_T encrypt = ENCRYPT_DISABLED;
				apmib_set( MIB_WLAN_ENCRYPT, (void *)&encrypt);
			}
			else if(val == ENCRYPT_WEP)
			{
				if(wepHandler(wp, tmpBuf, i) < 0)
				{
					goto setErr_end;
				}
			}
			else if(val > ENCRYPT_WEP && val <= WSC_AUTH_WPA2PSKMIXED)
			{
				if(wpaHandler(wp, tmpBuf, i) < 0)
				{
					goto setErr_end;
				}
			}
		}

#if defined(WLAN_PROFILE)
		if(addWlProfileHandler(wp, tmpBuf, i) < 0){
			//submitUrl = req_get_cstream_var(wp, ("submit-url-wlan2"), "");   // hidden page
			//goto setErr_end;
		}
		
#endif //#if defined(WLAN_PROFILE)


	}
	
	
#if defined(CONFIG_RTL_ULINKER) //repeater mode: clone wlan setting to wlan-vxd and modify wlan ssid
		int wlan_mode;
		int rptEnabled;
		int wlanvxd_mode;
		

		if(wlan_idx == 0)
			apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
		else
			apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);



		apmib_get(MIB_WLAN_MODE, (void *)&wlan_mode);
		
		if(wlan_mode != CLIENT_MODE && wlan_mode != WDS_MODE && rptEnabled == 1)
		{
			int isUpnpEnabled=0;
			int ori_vwlan_idx = vwlan_idx;
			char ssidBuf[64];
			
			
			vwlan_idx = NUM_VWLAN_INTERFACE;
			
			
			/* get original setting in vxd interface */
			apmib_get(MIB_WLAN_WSC_UPNP_ENABLED, (void *)&isUpnpEnabled);
			apmib_get(MIB_WLAN_MODE, (void *)&wlanvxd_mode);
			
			if(!ulinker_auto_changed)						
			ulinker_wlan_mib_copy(&pMib->wlan[wlan_idx][NUM_VWLAN_INTERFACE], &pMib->wlan[wlan_idx][0]);
			
			/* restore original setting in vxd interface and repeater ssid*/			
			apmib_set(MIB_WLAN_WSC_UPNP_ENABLED, (void *)&isUpnpEnabled);
			apmib_set(MIB_WLAN_MODE, (void *)&wlanvxd_mode);
			
			vwlan_idx = ori_vwlan_idx;
			
			/* add "-ext" at last of wlan ssid */
			apmib_get( MIB_WLAN_SSID,  (void *)ssidBuf);

			if(!ulinker_auto_changed)
			{
			if(wlan_idx == 0)
				apmib_set(MIB_REPEATER_SSID1, (void *)&ssidBuf);
			else
				apmib_set(MIB_REPEATER_SSID2, (void *)&ssidBuf);

			
				if(strlen(ssidBuf)<sizeof(ssidBuf)+4&&!ulinker_auto_changed)
			{
				strcat(ssidBuf,"-ext");
				apmib_set( MIB_WLAN_SSID,  (void *)ssidBuf);
				apmib_set( MIB_WLAN_WSC_SSID, (void *)ssidBuf);
				}
			}
		}
#endif	
	
	
	apmib_update_web(CURRENT_SETTING);

#if defined(CONFIG_RTL_ULINKER)
	if (ulinker_auto_changed == 1) {
		char *submitUrl;
		needReboot = 1;
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
		sprintf(lastUrl,"%s",submitUrl);
		send_redirect_perm(wp, "/reload.htm");
		return ;
	}
#endif


#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif
	tmpStr = req_get_cstream_var(wp, ("method0"), "");
	REBOOT_WAIT("/wizard.htm");
	
	return ;
setErr_end:
	
	OK_MSG1(tmpBuf,"/wizard.htm");
	return ;
}


#if defined(MIB_TLV)
extern int mib_search_by_id(const mib_table_entry_T *mib_tbl, unsigned short mib_id, unsigned char *pmib_num, const mib_table_entry_T **ppmib, unsigned int *offset);
extern mib_table_entry_T mib_root_table[];
#else
extern int update_linkchain(int fmt, void *Entry_old, void *Entry_new, int type_size);
#endif
void formWizard(request *wp, char *path, char *query)
{
	char *tmpStr;
	char tmpBuf[100];
	char varName[20];
	int i;
	int showed_wlan_num;
	int wlBandMode;
#ifdef HOME_GATEWAY	
	int dns_changed=0;
#endif	
	int mode=-1;
	char *submitUrl;
	char buffer[200];
	struct in_addr inLanaddr_orig, inLanaddr_new;
	struct in_addr inLanmask_orig, inLanmask_new;
	int	entryNum_resvdip;
	DHCPRSVDIP_T entry_resvdip, checkentry_resvdip;
	int link_type;
	struct in_addr private_host, tmp_private_host, update;	
	struct in_addr dhcpRangeStart, dhcpRangeEnd;
#ifdef MIB_TLV
	char pmib_num[10]={0};
	mib_table_entry_T *pmib_tl = NULL;
	unsigned int offset;
#endif
#ifdef MULTI_WAN_SUPPORT
	int wan_index=0;
#endif
//displayPostDate(wp->post_data);
		

	apmib_get( MIB_IP_ADDR,  (void *)buffer); //save the orig lan subnet
	memcpy((void *)&inLanaddr_orig, buffer, 4);
	
	apmib_get( MIB_SUBNET_MASK,  (void *)buffer); //save the orig lan mask
	memcpy((void *)&inLanmask_orig, buffer, 4);
#ifdef HOME_GATEWAY
	if(opModeHandler(wp, tmpBuf) < 0)
		goto setErr_end;	
#endif

	if(ntpHandler(wp, tmpBuf, 1) < 0)
		goto setErr_end;
	set_timeZone();
		
	if(tcpipLanHandler(wp, tmpBuf) < 0){
		submitUrl = req_get_cstream_var(wp, ("submit-url-lan"), "");   // hidden page
		goto setErr_end;
	}

#ifdef HOME_GATEWAY
#ifdef MULTI_WAN_SUPPORT
	wan_index=1;	

	if ( !apmib_set( MIB_WANIFACE_CURRENT_IDX, (void *)&wan_index) )
	{
		fprintf(stderr,"%s:%d get MIB_WANIFACE_CURRENT_IDX fail!wan_index=%d\n",__FUNCTION__,__LINE__,wan_index);
		return;
	}
	
	formMultiWanTcpipSetup(wp,path,query);
#endif
	if(tcpipWanHandler(wp, tmpBuf, &dns_changed) < 0){
		submitUrl = req_get_cstream_var(wp, ("submit-url-wan"), "");   // hidden page
		goto setErr_end;	
	}
#endif

#if defined(CONFIG_RTL_92D_SUPPORT)

	tmpStr = req_get_cstream_var(wp, "wlBandMode", "");
	if(tmpStr[0] != 0)
	{
		wlBandMode = atoi(tmpStr);
		apmib_set(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlBandMode);
	}

	for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
	{
		unsigned char wlanif[10];
		memset(wlanif,0x00,sizeof(wlanif));
		sprintf(wlanif, "wlan%d",i);
		if(SetWlan_idx(wlanif))
		{
			int intVal;
			
			intVal = 1;
			apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&intVal);
		}						
	}
#endif

	for(i=0 ; i < wlan_num ;i++){	
		wlan_idx = i ;
		sprintf(WLAN_IF, "wlan%d", wlan_idx);
		if(wlanHandler(wp, tmpBuf,&mode, i) < 0){
		submitUrl = req_get_cstream_var(wp, ("submit-url-wlan1"), "");   // hidden page
		goto setErr_end;
	}	
		
		sprintf(varName, "method%d", i);
		tmpStr = req_get_cstream_var(wp, varName, "");
	if(tmpStr[0] && tmpStr[0] == '1'){
			if(wepHandler(wp, tmpBuf, i) < 0){
			submitUrl = req_get_cstream_var(wp, ("submit-url-wlan2"), "");   // hidden page
			goto setErr_end;
		}
	}	
		if(wpaHandler(wp, tmpBuf, i) < 0){
		submitUrl = req_get_cstream_var(wp, ("submit-url-wlan2"), "");   // hidden page
		goto setErr_end;
	}


		
#if defined(WLAN_PROFILE)
		if(addWlProfileHandler(wp, tmpBuf, i) < 0){
			//submitUrl = req_get_cstream_var(wp, ("submit-url-wlan2"), "");   // hidden page
			//goto setErr_end;
		}
		
#endif //#if defined(WLAN_PROFILE)


	}
		
#if defined(CONFIG_RTL_92D_SUPPORT)		
	apmib_get(MIB_WLAN_BAND2G5G_SELECT, (void *)&wlBandMode);
	if(BANDMODEBOTH == wlBandMode)
	{
		unsigned char wlanIfStr[10];				
				
		for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
		{
			unsigned char wlanif[10];
			memset(wlanif,0x00,sizeof(wlanif));
			sprintf(wlanif, "wlan%d",i);
			if(SetWlan_idx(wlanif))
			{
				int intVal;
#if defined(CONFIG_RTL_92D_SUPPORT) && defined(CONFIG_RTL_92D_DMDP) && !defined(CONFIG_RTL_DUAL_PCIESLOT_BIWLAN_D)
				intVal = DMACDPHY;
#else
				intVal = SMACSPHY;
#endif
				apmib_set(MIB_WLAN_MAC_PHY_MODE, (void *)&intVal);				
			}						
		}
		
		/* 92d rule, 5g must up in wlan0 */
		/* phybandcheck */
		if(whichWlanIfIs(PHYBAND_5G) != 0)
		{
			swapWlanMibSetting(0,1);			
		}
	}
	else
	{
		int band2G5GSelect;
		int intVal;		
		
		for(i=0 ; i<NUM_WLAN_INTERFACE ; i++)
		{
			unsigned char wlanif[10];
			memset(wlanif,0x00,sizeof(wlanif));
			sprintf(wlanif, "wlan%d",i);
			if(SetWlan_idx(wlanif))
			{				
				intVal = SMACSPHY;
				apmib_set(MIB_WLAN_MAC_PHY_MODE, (void *)&intVal);				
			}						
		}
		
		tmpStr = req_get_cstream_var(wp, "Band2G5GSupport", "");
		
		if(tmpStr[0] != 0)
		{
			band2G5GSelect = atoi(tmpStr);			
		}
		
		/* 92d rule, 5g must up in wlan0 */
		/* phybandcheck */
		if(whichWlanIfIs(band2G5GSelect) != 0)
		{
			swapWlanMibSetting(0,1);			
		}
		apmib_save_wlanIdx();
		wlan_idx = 1;
		intVal = 1;
		apmib_set( MIB_WLAN_WLAN_DISABLED, (void *)&intVal); // disable wlan1
		apmib_recov_wlanIdx();
	}
#endif //#if defined(CONFIG_RTL_92D_SUPPORT)
	
	apmib_update_web(CURRENT_SETTING);
	apmib_get( MIB_IP_ADDR,  (void *)buffer); //check the new lan subnet
	memcpy((void *)&inLanaddr_new, buffer, 4);
		
	apmib_get( MIB_SUBNET_MASK,  (void *)buffer); //check the new lan mask
	memcpy((void *)&inLanmask_new, buffer, 4);
	
	if((inLanaddr_orig.s_addr & inLanmask_orig.s_addr) != (inLanaddr_new.s_addr & inLanmask_new.s_addr)){
		
		//check static dhcp ip 
		apmib_get(MIB_DHCPRSVDIP_TBL_NUM, (void *)&entryNum_resvdip);
		link_type = 8; //DHCPRSVDIP_ARRY_T
		for (i=1; i<=entryNum_resvdip; i++) {
			memset(&checkentry_resvdip, '\0', sizeof(checkentry_resvdip));
			*((char *)&entry_resvdip) = (char)i;
			apmib_get(MIB_DHCPRSVDIP_TBL, (void *)&entry_resvdip);
			memcpy(&checkentry_resvdip, &entry_resvdip, sizeof(checkentry_resvdip));
			memcpy((void *)&private_host, &(entry_resvdip.ipAddr), 4);
			if((inLanaddr_new.s_addr & inLanmask_new.s_addr) != (private_host.s_addr & inLanmask_new.s_addr)){
				update.s_addr = inLanaddr_new.s_addr & inLanmask_new.s_addr;
				tmp_private_host.s_addr  = ~(inLanmask_new.s_addr) & private_host.s_addr;
				update.s_addr = update.s_addr | tmp_private_host.s_addr;
				memcpy((void *)&(checkentry_resvdip.ipAddr), &(update), 4);
#if defined(MIB_TLV)
				offset=0;//must initial first for mib_search_by_id
				mib_search_by_id(mib_root_table, MIB_DHCPRSVDIP_TBL, (unsigned char *)pmib_num, &pmib_tl, &offset);
				update_tblentry(pMib,offset,entryNum_resvdip,pmib_tl,&entry_resvdip, &checkentry_resvdip);
#else
				update_linkchain(link_type, &entry_resvdip, &checkentry_resvdip , sizeof(checkentry_resvdip));
#endif

			}
		}
		apmib_get( MIB_DHCP_CLIENT_START,  (void *)buffer); //save the orig dhcp start 
		memcpy((void *)&dhcpRangeStart, buffer, 4);
		apmib_get( MIB_DHCP_CLIENT_END,  (void *)buffer); //save the orig dhcp end 
		memcpy((void *)&dhcpRangeEnd, buffer, 4);
		
		if((dhcpRangeStart.s_addr & inLanmask_new.s_addr) != (inLanaddr_new.s_addr & inLanmask_new.s_addr)){
			update.s_addr = inLanaddr_new.s_addr & inLanmask_new.s_addr;
			tmp_private_host.s_addr  = ~(inLanmask_new.s_addr) & dhcpRangeStart.s_addr;
			update.s_addr = update.s_addr | tmp_private_host.s_addr;
			memcpy((void *)&(dhcpRangeStart), &(update), 4);
			apmib_set(MIB_DHCP_CLIENT_START, (void *)&dhcpRangeStart);
		}
		if((dhcpRangeEnd.s_addr & inLanmask_new.s_addr) != (inLanaddr_new.s_addr & inLanmask_new.s_addr)){
			update.s_addr = inLanaddr_new.s_addr & inLanmask_new.s_addr;
			tmp_private_host.s_addr  = ~(inLanmask_new.s_addr) & dhcpRangeEnd.s_addr;
			update.s_addr = update.s_addr | tmp_private_host.s_addr;
			memcpy((void *)&(dhcpRangeEnd), &(update), 4);
			apmib_set(MIB_DHCP_CLIENT_END, (void *)&dhcpRangeEnd);
		}
		apmib_update_web(CURRENT_SETTING);
	}
#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif
	submitUrl = req_get_cstream_var(wp, ("next_url"), "");
#ifdef MULTI_WAN_SUPPORT
#ifdef APPLY_CHANGE_DIRECT_SUPPORT	
	REBOOT_NOWAIT("/wizard.htm");
#else	
	OK_MSG("/wizard.htm");
#endif
#else
	REBOOT_WAIT("/wizard.htm");
#endif
	return ;
setErr_end:
	
	OK_MSG1(tmpBuf,"/wizard.htm");
	return ;

}

///////////////////////////////////////////////////////////////////////////////////////////////
int logout=0 ;
void formLogout(request *wp, char *path, char *query)
{
	char *logout_str, *return_url;
	logout_str = req_get_cstream_var(wp, ("logout"), "");
	if (logout_str[0]) {
		logout = 1 ;
	}

        
#ifdef CSRF_SECURITY_PATCH
        log_boaform("formLogout", wp);  //To set formLogout valid at security_tbl
#endif

#ifdef LOGIN_URL
		delete_user(wp);
	    send_redirect_perm(wp, WEB_PAGE_LOGIN);
	    return;
#endif		

	return_url = req_get_cstream_var(wp, ("return-url"), "");

#ifdef REBOOT_CHECK
	send_redirect_perm(wp, return_url);	
#else
        OK_MSG(return_url);
#endif

	return;
}
#define _PATH_SYSCMD_LOG "/tmp/syscmd.log"

void formSysCmd(request *wp, char *path, char *query)
{
	char  *submitUrl, *sysCmd;
#ifndef NO_ACTION
	char tmpBuf[100];
#endif
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	sysCmd = req_get_cstream_var(wp, "sysCmd", "");   // hidden page

#ifndef NO_ACTION
	if(sysCmd[0]){
		snprintf(tmpBuf, 100, "%s 2>&1 > %s",sysCmd,  _PATH_SYSCMD_LOG);
		system(tmpBuf);
	}
#endif
		send_redirect_perm(wp, submitUrl);
	return;
}

int sysCmdLog(request *wp, int argc, char **argv)
{
        FILE *fp;
	char  buf[150];
	int nBytesSent=0;

        fp = fopen(_PATH_SYSCMD_LOG, "r");
        if ( fp == NULL )
                goto err1;
        while(fgets(buf,150,fp)){
		nBytesSent += req_format_write(wp, ("%s"), buf);
        }
	fclose(fp);
	unlink(_PATH_SYSCMD_LOG);
err1:
	return nBytesSent;
}

#if defined(CONFIG_RTL_ULINKER)

void formUlkOpMode(request *wp, char *path, char *query)
{
	char *submitUrl;
	char *tmpStr;
	int ulinker_auto, opmode, wlanMode, rpt_enabled;
	char tmpBuf[100];

//displayPostDate(wp->post_data);
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page


	tmpStr = req_get_cstream_var(wp, ("ulinker_auto"), "");
	if(tmpStr[0])
	{
		ulinker_auto = tmpStr[0] - '0' ;
		apmib_set( MIB_ULINKER_AUTO, (void *)&ulinker_auto);
		
		if(ulinker_auto == 0)
		{
			int selVal;
			tmpStr = req_get_cstream_var(wp, ("ulinker_manual_Sel"), "");
			if(tmpStr[0])
			{
				selVal = tmpStr[0] - '0';
				
				switch(selVal)
				{
					case 0:
ulinker_wlan_mib_copy(&pMib->wlan[0][0], &pMib->wlan[0][ULINKER_AP_MIB]);
						opmode = BRIDGE_MODE;
						wlanMode = AP_MODE;
						rpt_enabled = 0; 
						break;
					case 1:
ulinker_wlan_mib_copy(&pMib->wlan[0][0], &pMib->wlan[0][ULINKER_CL_MIB]);						
						opmode = BRIDGE_MODE;
						wlanMode = CLIENT_MODE;
						rpt_enabled = 0; 
						break;
					case 2:
ulinker_wlan_mib_copy(&pMib->wlan[0][0], &pMib->wlan[0][ULINKER_AP_MIB]);
						opmode = GATEWAY_MODE;
						wlanMode = AP_MODE;
						rpt_enabled = 0; 
						break;
					case 3:
ulinker_wlan_mib_copy(&pMib->wlan[0][0], &pMib->wlan[0][ULINKER_RPT_MIB]);						
						opmode = BRIDGE_MODE;
						wlanMode = AP_MODE;
						rpt_enabled = 1; 
						break;
					case 4:
ulinker_wlan_mib_copy(&pMib->wlan[0][0], &pMib->wlan[0][ULINKER_RPT_MIB]);
						opmode = WISP_MODE;
						wlanMode = AP_MODE;
						rpt_enabled = 1; 
						break;
				}
				apmib_set( MIB_OP_MODE, (void *)&opmode);
				apmib_set( MIB_WLAN_MODE, (void *)&wlanMode);
				pMib->wlan[wlan_idx][NUM_VWLAN_INTERFACE].wlanMode = CLIENT_MODE;
				
				if(wlanMode == CLIENT_MODE) //set cipher suit to AES and encryption to wpa2 only if wpa2 mixed mode is set
				{
					
					ENCRYPT_T encrypt;
					int intVal;
					apmib_get( MIB_WLAN_ENCRYPT, (void *)&encrypt);
					if(encrypt == ENCRYPT_WPA2_MIXED)
					{
						intVal =   WPA_CIPHER_AES ;
						encrypt = ENCRYPT_WPA2;
						
						apmib_set(MIB_WLAN_WPA_CIPHER_SUITE, (void *)&intVal);
						apmib_set(MIB_WLAN_WPA2_CIPHER_SUITE, (void *)&intVal);
						apmib_set(MIB_WLAN_ENCRYPT, (void *)&encrypt);
					}
				}
	
				if(wlan_idx == 0)
				{
					apmib_set( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled);						
				}
				else
				{
					apmib_set( MIB_REPEATER_ENABLED2, (void *)&rpt_enabled);
				}
				pMib->wlan[wlan_idx][NUM_VWLAN_INTERFACE].wlanDisabled = (rpt_enabled?0:1);
				
			}
		}
	}
	
	apmib_update_web(CURRENT_SETTING);

#if defined(CONFIG_RTL_ULINKER)
	if (ulinker_auto == 0) {
		char *submitUrl;
		needReboot = 1;
		submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
		sprintf(lastUrl,"%s",submitUrl);
		send_redirect_perm(wp, "/reload.htm");
		return ;
	}
#endif

#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif		

#ifdef REBOOT_CHECK
	REBOOT_WAIT(submitUrl);
#else //#ifdef REBOOT_CHECK	.
	OK_MSG(submitUrl);
#endif //#ifdef REBOOT_CHECK	


#ifndef NO_ACTION
	run_init_script("all");
#endif
return;

setErr:
	ERR_MSG(tmpBuf);
}
#endif//#if defined(CONFIG_RTL_ULINKER)

#ifdef HOME_GATEWAY
int  opModeHandler(request *wp, char *tmpBuf)
{
	char *tmpStr;
	int opmode, wanId;
	char repeaterSSID[40];
	tmpStr = req_get_cstream_var(wp, ("opMode"), "");  
	if(tmpStr[0]){
		opmode = tmpStr[0] - '0' ;
		if ( apmib_set(MIB_OP_MODE, (void *)&opmode) == 0) {
			strcpy(tmpBuf, ("Set Opmode error!"));
			goto setErr_opmode;
		}
	}
#if defined(CONFIG_SMART_REPEATER)
	if(opmode==2)
	{//wisp mode
#endif
		tmpStr = req_get_cstream_var(wp, ("wispWanId"), "");  
		if(tmpStr[0]){
			wanId = tmpStr[0] - '0' ;
			if ( apmib_set(MIB_WISP_WAN_ID, (void *)&wanId) == 0) {
				strcpy(tmpBuf, ("Set WISP WAN Id error!"));
				goto setErr_opmode;
			}
#if defined(CONFIG_SMART_REPEATER)
			int rpt_enabled = 1;
			char wlanifStr[20];
			int wlanMode;

			apmib_save_wlanIdx();
			if(wanId == 0)
			{
				apmib_set( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled);
				apmib_get( MIB_REPEATER_SSID1, (void *)repeaterSSID);
				rpt_enabled=0;
				apmib_set(MIB_REPEATER_ENABLED2,(void *)&rpt_enabled);
			}
			else
			{
				apmib_set( MIB_REPEATER_ENABLED2, (void *)&rpt_enabled);
				apmib_get( MIB_REPEATER_SSID2, (void *)repeaterSSID);
				rpt_enabled=0;
				apmib_set(MIB_REPEATER_ENABLED1,(void *)&rpt_enabled);
			}

			sprintf(wlanifStr, "wlan%d", wanId); 
			SetWlan_idx(wlanifStr);
			wlanMode = AP_MODE;
			apmib_set( MIB_WLAN_MODE, (void *)&wlanMode);
			
			sprintf(wlanifStr, "wlan%d-vxd", wanId); 
			SetWlan_idx(wlanifStr);
			wlanMode = CLIENT_MODE;
			apmib_set( MIB_WLAN_MODE, (void *)&wlanMode);
			apmib_set(MIB_WLAN_SSID,(void *)repeaterSSID);
			rpt_enabled = 0;
			apmib_set( MIB_WLAN_WLAN_DISABLED, (void *)&rpt_enabled);
			apmib_recov_wlanIdx();

#endif
		}
#if defined(CONFIG_SMART_REPEATER)
		else{//only one wlan:92c
			int rpt_enabled = 1;
			char wlanifStr[20]={0};
			int wlanMode;

			wanId=0;
			apmib_save_wlanIdx();
			apmib_set( MIB_REPEATER_ENABLED1, (void *)&rpt_enabled);
			apmib_get( MIB_REPEATER_SSID1, (void *)repeaterSSID);
			sprintf(wlanifStr, "wlan%d", wanId); 
			SetWlan_idx(wlanifStr);
			wlanMode = AP_MODE;
			apmib_set( MIB_WLAN_MODE, (void *)&wlanMode);

			sprintf(wlanifStr, "wlan%d-vxd", wanId); 
			SetWlan_idx(wlanifStr);
			wlanMode = CLIENT_MODE;
			apmib_set( MIB_WLAN_MODE, (void *)&wlanMode);
			apmib_set(MIB_WLAN_SSID,(void *)repeaterSSID);
			rpt_enabled = 0;
			apmib_set( MIB_WLAN_WLAN_DISABLED, (void *)&rpt_enabled);
			apmib_recov_wlanIdx();
		}
	}else //opmode is gw or bridge
	{	

		int rpt_enabled=0;
		apmib_set(MIB_REPEATER_ENABLED1, (void *)&rpt_enabled);
		apmib_set(MIB_REPEATER_ENABLED2,(void *)&rpt_enabled);
	}
#endif
	return 0;

setErr_opmode:
	return -1;

}
void formOpMode(request *wp, char *path, char *query)
{
	char *submitUrl;
	char tmpBuf[100];
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	if(opModeHandler(wp, tmpBuf) < 0)
			goto setErr;
	
	apmib_update_web(CURRENT_SETTING);
	
#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif		
#ifndef RTK_REINIT_SUPPORT
#ifdef REBOOT_CHECK
	REBOOT_WAIT(submitUrl);
#else //#ifdef REBOOT_CHECK	.
	OK_MSG(submitUrl);
#endif //#ifdef REBOOT_CHECK	
#else
	OK_MSG(submitUrl);
#endif

#ifndef NO_ACTION
	run_init_script("all");
#endif
return;

setErr:
	ERR_MSG(tmpBuf);
}
#endif

#ifdef REBOOT_CHECK
void formRebootCheck(request *wp, char *path, char *query)
{
	char *submitUrl;
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	apmib_update_web(CURRENT_SETTING);
#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif
#ifdef RTK_REINIT_SUPPORT
	{
		char lan_ip_buf[32]={0};
		char cmdBuf[64]={0};
		char tmpBuf[256]={0};
		struct in_addr	lan_ip_orig={0};
		struct in_addr	lan_mask_orig={0};
		struct in_addr	lan_ip={0};
		struct in_addr	lan_mask={0};
		struct in_addr	remote_ip={0};
		
		//fprintf(stderr,"%d %s:%d \n",time(NULL),__FUNCTION__,__LINE__);
		
		if(!getInAddr(BRIDGE_IF,IP_ADDR,&lan_ip_orig))
			goto REBOOTCHECK_FAIL;
		if(!getInAddr(BRIDGE_IF,SUBNET_MASK,&lan_mask_orig))
			goto REBOOTCHECK_FAIL;
		if(!apmib_get(MIB_IP_ADDR,((void *)&lan_ip)))
			goto REBOOTCHECK_FAIL;
		if(!apmib_get(MIB_SUBNET_MASK,((void *)&lan_mask)))
			goto REBOOTCHECK_FAIL;
		if(!inet_aton(wp->remote_ip_addr, &remote_ip))
			goto REBOOTCHECK_FAIL;

		sprintf(lan_ip_buf,"%s",inet_ntoa(lan_ip));
		fprintf(stderr,"send apply change event!\n");
		//fprintf(stderr,"%d %s:%d \n",time(NULL),__FUNCTION__,__LINE__);
#ifdef	REDUCE_WEBCLIENT_WAITTIME_SUPPORT
		sprintf(cmdBuf,"reinitCli -e %d&",REINIT_EVENT_APPLYCAHNGES);
#else
		sprintf(cmdBuf,"reinitCli -e %d",REINIT_EVENT_APPLYCAHNGES);
#endif
		system(cmdBuf);
		//fprintf(stderr,"%d %s:%d \n",time(NULL),__FUNCTION__,__LINE__);
		//fprintf(stderr,"%s:%d lan_ip_orig.s_addr=%x lan_mask_orig.s_addr=%x remote_ip.s_addr=%x lan_ip=%x\n",__FUNCTION__,__LINE__,lan_ip_orig.s_addr,lan_mask_orig.s_addr,remote_ip.s_addr,lan_ip.s_addr);
		
		if(((lan_ip_orig.s_addr&lan_mask_orig.s_addr)==(remote_ip.s_addr&lan_mask_orig.s_addr))//lan visit
			&& (memcmp(&lan_ip_orig,&lan_ip,sizeof(lan_ip))!=0) //lan ip change
		)
		{
			//fprintf(stderr,"%d %s:%d \n",time(NULL),__FUNCTION__,__LINE__);
			//fprintf(stderr,"%s:%d local_ip_addr=%s remote_ip_addr=%s\n",__FUNCTION__,__LINE__,wp->local_ip_addr,wp->remote_ip_addr);
			//OK_MSG_FW("<h4>Change setting successfully!<BR><BR>Do not turn off or reboot the Device during this time.</h4>", submitUrl, 0, lan_ip_buf);		
			snprintf(tmpBuf, sizeof(tmpBuf), "%s","<h4><script>dw(wlsurvey_change_setting_success)</script><br><br><script>dw(wlsurvey_donot_turn_off)</script></h4>");
			OK_MSG_FW(tmpBuf, submitUrl, 0, lan_ip_buf);
			//fprintf(stderr,"%d %s:%d \n",time(NULL),__FUNCTION__,__LINE__);
		}else
		{
			send_redirect_perm(wp,submitUrl);
		}

	}
#else
	REBOOT_WAIT(submitUrl);
#endif

	needReboot = 0;
	return;
REBOOTCHECK_FAIL:
		ERR_MSG("reboot check fail!\n");

}

#if defined(WLAN_PROFILE)
void formSiteSurveyProfile(request *wp, char *path, char *query)
{
	char *submitUrl, *strTmp, *addProfileTmp;
	char tmpBuf[100];
	char varName[20];
		
//displayPostDate(wp->post_data);	


	sprintf(varName, "wizardAddProfile%d", wlan_idx);
	addProfileTmp = req_get_cstream_var(wp, varName, "");
	
	if(addProfileTmp[0])	
	{
		int rptEnabled, wlan_mode;
		int ori_vwlan_idx=vwlan_idx;
		int profile_enabled_id, profile_num_id, profile_tbl_id;
		int profileEnabledVal=1;
		char iwprivCmd[600]={0};
		int entryNum;
		WLAN_PROFILE_T entry;
		int profileIdx;
		char ifname[10]={0}; //max is wlan0-vxd
		
		memset(iwprivCmd, 0x00, sizeof(iwprivCmd));
		apmib_get(MIB_WLAN_MODE, (void *)&wlan_mode);

		if(wlan_idx == 0)
			apmib_get(MIB_REPEATER_ENABLED1, (void *)&rptEnabled);
		else
			apmib_get(MIB_REPEATER_ENABLED2, (void *)&rptEnabled);

		
		if( (wlan_mode == AP_MODE || wlan_mode == AP_WDS_MODE || wlan_mode == AP_MESH_MODE) && (rptEnabled == 1))
		{
			sprintf(ifname,"wlan%d-vxd",wlan_idx);
			vwlan_idx = NUM_VWLAN_INTERFACE;
		}
		else
		{
			sprintf(ifname,"wlan%d",wlan_idx);
			vwlan_idx = 0;
		}
		
		if(wlan_idx == 0)
		{		
			profile_num_id = MIB_PROFILE_NUM1;
			profile_tbl_id = MIB_PROFILE_TBL1;
			profile_enabled_id = MIB_PROFILE_ENABLED1;
		}
		else
		{
			profile_num_id = MIB_PROFILE_NUM2;
			profile_tbl_id = MIB_PROFILE_TBL2;
			profile_enabled_id = MIB_PROFILE_ENABLED2;
		}

		apmib_set(profile_enabled_id, (void *)&profileEnabledVal);
		


		if(addWlProfileHandler(wp, tmpBuf, wlan_idx) < 0){
	printf("\r\n Add wireless profile fail__[%s-%u]\r\n",__FILE__,__LINE__);
			//strcpy(tmpBuf, ("Add wireless profile fail!"));
			//goto ss_err;
		}

		sprintf(iwprivCmd,"iwpriv %s set_mib ap_profile_enable=%d",ifname, profileEnabledVal);
		system(iwprivCmd);
		
		sprintf(iwprivCmd,"iwpriv %s set_mib ap_profile_num=0",ifname);
		system(iwprivCmd);

		apmib_get(profile_num_id, (void *)&entryNum);

		for(profileIdx=1; profileIdx<=entryNum;profileIdx++)
		{
			memset(iwprivCmd, 0x00, sizeof(iwprivCmd));
			memset(&entry, 0x00, sizeof(WLAN_PROFILE_T));
			*((char *)&entry) = (char)profileIdx;
			apmib_get(profile_tbl_id, (void *)&entry);
		


			// fix for :translate_control_code_sprintf will change length of ssid and PSK
			// which will lead to out-of-bands access.
			char  tmpTranBuffSSID[100];
			char  tmpTranBuffwpaPSK[200];
			strcpy(tmpTranBuffSSID, entry.ssid);
			strcpy(tmpTranBuffwpaPSK, entry.wpaPSK);
		
		
		
		
			translate_control_code_sprintf(tmpTranBuffSSID);
			translate_control_code_sprintf(tmpTranBuffwpaPSK);
			
			//iwpriv wlan0 set_mib ap_profile_add="open-ssid",0,0
			if(entry.encryption == ENCRYPT_DISABLED)
			{
				sprintf(iwprivCmd,"iwpriv %s set_mib ap_profile_add=\"%s\",%d,%d",ifname,tmpTranBuffSSID,0,0);
			}
			else if(entry.encryption == WEP64 || entry.encryption == WEP128)
			{
				char tmp1[400];
				if (entry.encryption == WEP64)			
					sprintf(tmp1,"%d,%d,%02x%02x%02x%02x%02x,%02x%02x%02x%02x%02x,%02x%02x%02x%02x%02x,%02x%02x%02x%02x%02x", 
						entry.auth,
						entry.wep_default_key,
						entry.wepKey1[0],entry.wepKey1[1],entry.wepKey1[2],entry.wepKey1[3],entry.wepKey1[4],
						entry.wepKey2[0],entry.wepKey2[1],entry.wepKey2[2],entry.wepKey2[3],entry.wepKey2[4],
						entry.wepKey3[0],entry.wepKey3[1],entry.wepKey3[2],entry.wepKey3[3],entry.wepKey3[4],
						entry.wepKey4[0],entry.wepKey4[1],entry.wepKey4[2],entry.wepKey4[3],entry.wepKey4[4]);
				else
					sprintf(tmp1,"%d,%d,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
						entry.auth,
						entry.wep_default_key,
						entry.wepKey1[0],entry.wepKey1[1],entry.wepKey1[2],entry.wepKey1[3],entry.wepKey1[4],entry.wepKey1[5],entry.wepKey1[6],entry.wepKey1[7],entry.wepKey1[8],
						entry.wepKey1[9],entry.wepKey1[10],entry.wepKey1[11],entry.wepKey1[12],
						entry.wepKey2[0],entry.wepKey2[1],entry.wepKey2[2],entry.wepKey2[3],entry.wepKey2[4],entry.wepKey2[5],entry.wepKey2[6],entry.wepKey2[7],entry.wepKey2[8],
						entry.wepKey2[9],entry.wepKey2[10],entry.wepKey2[11],entry.wepKey2[12],
						entry.wepKey3[0],entry.wepKey3[1],entry.wepKey3[2],entry.wepKey3[3],entry.wepKey3[4],entry.wepKey3[5],entry.wepKey3[6],entry.wepKey3[7],entry.wepKey3[8],
						entry.wepKey3[9],entry.wepKey3[10],entry.wepKey3[11],entry.wepKey3[12],
						entry.wepKey4[0],entry.wepKey4[1],entry.wepKey4[2],entry.wepKey4[3],entry.wepKey4[4],entry.wepKey4[5],entry.wepKey4[6],entry.wepKey4[7],entry.wepKey4[8],
						entry.wepKey4[9],entry.wepKey4[10],entry.wepKey4[11],entry.wepKey4[12]);	
				
				sprintf(iwprivCmd,"iwpriv %s set_mib ap_profile_add=\"%s\",%d,%s,",ifname,tmpTranBuffSSID,entry.encryption, tmp1);
			}
			else if(entry.encryption == 2 || entry.encryption == 4 || entry.encryption == 6) //wpa or wpa2
			{
				char tmp1[400];
				sprintf(tmp1, "%d,%s", entry.wpa_cipher, tmpTranBuffwpaPSK);
				sprintf(iwprivCmd,"iwpriv %s set_mib ap_profile_add=\"%s\",%d,0,\"%s\"",ifname,tmpTranBuffSSID,entry.encryption,tmp1 );
			}

	
			system(iwprivCmd);
		}

		vwlan_idx = ori_vwlan_idx;
		apmib_update_web(CURRENT_SETTING);
		
		
	}

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	
	strTmp = req_get_cstream_var(wp, "restartNow", ""); 
	if(strTmp[0])
	{
		
		//apmib_update_web(CURRENT_SETTING);
#ifdef REBOOT_CHECK
		run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
		run_init_script("all");
#endif
		REBOOT_WAIT(submitUrl);
		needReboot = 0;
	}
	else
	{
		send_redirect_perm(wp,submitUrl);
	}
	
}
#endif //#if defined(WLAN_PROFILE)

#ifdef CONFIG_APP_RTL_QUAGGA
void formquaggadebug(request *wp, char *path, char *query)
{
	char *submitUrl, *tmpStr;
	char tmpBuf[100];
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	/*tmpStr = req_get_cstream_var(wp, ("ospfclear"), "");  
	if(tmpStr[0]){
		snprintf(tmpBuf, 100, "echo \" \" > %s", "/var/log/ospfd.log");
		system(tmpBuf);
		//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to  SDKv2.5 from kernel 2.4
#ifdef RINGLOG
		system("rm /var/log/ospfd.log >/dev/null 2>&1");
#endif
		//### end
		send_redirect_perm(wp, submitUrl);
		return;
	}*/
	send_redirect_perm(wp, submitUrl);
return;

}

void formisisdebug(request *wp, char *path, char *query)
{
	char *submitUrl, *tmpStr;
	char tmpBuf[100];
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	/*tmpStr = req_get_cstream_var(wp, ("ospfclear"), "");  
	if(tmpStr[0]){
		snprintf(tmpBuf, 100, "echo \" \" > %s", "/var/log/ospfd.log");
		system(tmpBuf);
		//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to  SDKv2.5 from kernel 2.4
#ifdef RINGLOG
		system("rm /var/log/ospfd.log >/dev/null 2>&1");
#endif
		//### end
		send_redirect_perm(wp, submitUrl);
		return;
	}*/
	send_redirect_perm(wp, submitUrl);
return;
}


void formbgpdebug(request *wp, char *path, char *query)
{
	char *submitUrl, *tmpStr;
	char tmpBuf[100];
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	/*tmpStr = req_get_cstream_var(wp, ("ospfclear"), "");  
	if(tmpStr[0]){
		snprintf(tmpBuf, 100, "echo \" \" > %s", "/var/log/ospfd.log");
		system(tmpBuf);
		//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to  SDKv2.5 from kernel 2.4
#ifdef RINGLOG
		system("rm /var/log/ospfd.log >/dev/null 2>&1");
#endif
		//### end
		send_redirect_perm(wp, submitUrl);
		return;
	}*/
	send_redirect_perm(wp, submitUrl);
return;
}



#endif 


#endif //#ifdef REBOOT_CHECK
#ifdef SYS_DIAGNOSTIC
typedef enum { DIAGNOSTIC_DOING,DIAGNOSTIC_DONE,DIAGNOSTIC_INVALID_ARG}DIAGNOSTIC_STATUS;
#define SYS_DIAGNOSTIC_STATUS "/tmp/diagnostic_status"
void clean_diagnostic_status(void)
{
	struct stat status;
	if(stat(SYS_DIAGNOSTIC_STATUS,&status)==0)
	{
		unlink(SYS_DIAGNOSTIC_STATUS);
	}
}
DIAGNOSTIC_STATUS get_diagnostic_status(void)
{
	FILE *fp;
	char  buf[32] = {0};
	DIAGNOSTIC_STATUS ret;
	memset(buf,0,sizeof(buf));
	fp = fopen(SYS_DIAGNOSTIC_STATUS,"r");
	if(fp)
	{
		fgets(buf,sizeof(buf),fp);
		fclose(fp);
	}
	else
	{
		return DIAGNOSTIC_DOING;
	}
	if(!strncmp(buf,"doing",strlen("doing")))
	{
		ret = DIAGNOSTIC_DOING;
	}
	else if(!strncmp(buf,"done",strlen("done")))
	{
		ret = DIAGNOSTIC_DONE;
	}
	else if(!strncmp(buf,"invalid_arg",strlen("invalid_arg")))
	{
		ret = DIAGNOSTIC_INVALID_ARG;
	}
	return ret;
}
void formDiagnostic(request *wp, char *path, char *query)
{
	char *strRequest;
	char *str_diagType;
	char *str_wlanDiagType;
	char *submitUrl;
	char cmdBuf[256] = {0};
	char fileName[64] = {0};
	int val;
	DIAGNOSTIC_STATUS ret;
	strRequest = req_get_cstream_var(wp, ("sys_diagnostic"), "");
	str_diagType = req_get_cstream_var(wp, ("diag_type"), "");
	str_wlanDiagType = req_get_cstream_var(wp, ("wlan_diag_type"), "");
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	strcpy(fileName,"/sys_diagnostic.txt");
	if(strRequest[0])
	{
		if(!strcmp(str_diagType,"0"))/*system overview*/
		{
			strcpy(cmdBuf,"flash sys_diagnostic summary");
		}
		else if(!strcmp(str_diagType,"1"))/*wlan related*/
		{
			val = atoi(str_wlanDiagType);
			switch(val)
			{
				case 1:
					strcpy(cmdBuf,"flash sys_diagnostic wlan_scan");
					break;
				case 2:
					strcpy(cmdBuf,"flash sys_diagnostic wlan_connect");
					break;
				case 3:
					strcpy(cmdBuf,"flash sys_diagnostic wlan_throughput");
					break;
				default:
					break;
			}
		}
		clean_diagnostic_status();
		system(cmdBuf);
		while(1)
		{
			ret = get_diagnostic_status();
			if(DIAGNOSTIC_DOING == ret)
			{
				sleep(1);
				continue;
			}
			else if(DIAGNOSTIC_DONE == ret || DIAGNOSTIC_INVALID_ARG == ret)
			{
				break;
			}
		}
		if(DIAGNOSTIC_DONE == ret)
		{
			#ifdef CONFIG_RTL_WEBPAGES_IN_ROOTFS
			sprintf(cmdBuf,"rm -f /var%s",fileName);
			#else
			sprintf(cmdBuf,"rm -f /web%s",fileName);
			#endif
			system(cmdBuf);
			#ifdef CONFIG_RTL_WEBPAGES_IN_ROOTFS
			sprintf(cmdBuf,"cp /tmp/dumpDiagnosticLog.txt /var%s",fileName);
			#else
			sprintf(cmdBuf,"cp /tmp/dumpDiagnosticLog.txt /tmp%s",fileName);
			#endif
			system(cmdBuf);
			send_redirect_perm(wp, fileName);
		}
		else if(DIAGNOSTIC_INVALID_ARG == ret)
		{
			ERR_MSG("Invalid usage!\n");
		}
		return;
	}
	return;
}
#endif
void formSysLog(request *wp, char *path, char *query)
{
	char *submitUrl, *tmpStr;
	char tmpBuf[100];
	int enabled, rt_enabled;
	struct in_addr ipAddr ;
	
	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	tmpStr = req_get_cstream_var(wp, ("clear"), "");  
	if(tmpStr[0]){
		snprintf(tmpBuf, 100, "echo \" \" > %s", "/var/log/messages");
		system(tmpBuf);
		//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to  SDKv2.5 from kernel 2.4
#ifdef RINGLOG
		system("rm /var/log/messages.* >/dev/null 2>&1");
#endif
		//### end
		send_redirect_perm(wp, submitUrl);
		return;
	}

/*
 *	NOTE: If variable enabled (MIB_SCRLOG_ENABLED) bitmask modify(bitmap),
 *	 	Please modify driver rtl8190 reference variable (dot1180211sInfo.log_enabled in linux-2.4.18/drivers/net/rtl8190/8190n_cfg.h) 
 */
	apmib_get(MIB_SCRLOG_ENABLED, (void *)&enabled);
	
	tmpStr = req_get_cstream_var(wp, ("logEnabled"), "");  
	if(!strcmp(tmpStr, "ON")) {
		enabled |= 1;

		tmpStr = req_get_cstream_var(wp, ("syslogEnabled"), "");
		if(!strcmp(tmpStr, "ON"))
			enabled |= 2;		
		else
			enabled &= ~2;
		
		tmpStr = req_get_cstream_var(wp, ("wlanlogEnabled"), "");
		if(!strcmp(tmpStr, "ON")) 
			enabled |= 4;	
		else
			enabled &= ~4;
		
#ifdef HOME_GATEWAY
#ifdef DOS_SUPPORT
		tmpStr = req_get_cstream_var(wp, ("doslogEnabled"), "");
		if(!strcmp(tmpStr, "ON")) 
			enabled |= 8;		
		else
			enabled &= ~8;		
#endif
#endif

#ifdef CONFIG_RTK_MESH
		tmpStr = req_get_cstream_var(wp, ("meshlogEnabled"), "");
		if(!strcmp(tmpStr, "ON")) 
			enabled |= 16;	
		else
			enabled &= ~16;
#endif

	}
	else
		enabled &= ~1;						

	if ( apmib_set(MIB_SCRLOG_ENABLED, (void *)&enabled) == 0) {
		strcpy(tmpBuf, ("Set log enable error!"));
		goto setErr;
	}
	
	if(enabled & 1){
		tmpStr = req_get_cstream_var(wp, ("rtLogEnabled"), "");  

		if(!strcmp(tmpStr, "ON"))
			rt_enabled= 1;
		else
			rt_enabled= 0;
		if ( apmib_set(MIB_REMOTELOG_ENABLED, (void *)&rt_enabled) == 0) {
			strcpy(tmpBuf, ("Set remote log enable error!"));
			goto setErr;
		}

		tmpStr = req_get_cstream_var(wp, ("logServer"), "");  
		if(tmpStr[0]){
			inet_aton(tmpStr, &ipAddr);
			if ( apmib_set(MIB_REMOTELOG_SERVER, (void *)&ipAddr) == 0) {
				strcpy(tmpBuf, ("Set remote log server error!"));
				goto setErr;
			}
		}
	}
	apmib_update_web(CURRENT_SETTING);	
#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif
	OK_MSG(submitUrl);
	return;

setErr:
	ERR_MSG(tmpBuf);
}
unsigned char  interfacelist[20][20];
int interfaceList(request *wp, int argc, char **argv)
{
	FILE *fp;
	char  buf[2010];
	int i,j=0,k;
	int nBytesSent=0;
	system("rm /var/devlist.log");
	system("cat /proc/net/dev > /var/devlist.log");
	fp = fopen("/var/devlist.log", "r");
	memset(interfacelist,'\0',sizeof(interfacelist));
	memset(buf,'\0',sizeof(buf));
	if (fp == NULL)
		goto err1;
	j=0;
	while(fgets(buf,2000,fp)){
		for(i=0;i<2000;i++)
		{
			if((buf[i]=='e')&&(buf[i+1]=='t')&&(buf[i+2]=='h'))
			{
				if(buf[i+3]>'1')
					continue;
				if(buf[i-1]!='p')
					{
				k=0;
				interfacelist[j][k++]=buf[i];
				interfacelist[j][k++]=buf[i+1];
				interfacelist[j][k++]=buf[i+2];
				interfacelist[j][k++]=buf[i+3];
				interfacelist[j++][k++]='\0';

				i=i+3;
				nBytesSent += req_format_write(wp,	
	   		("<option value=%u>%s</option>"),j-1,interfacelist[j-1]);
			}
		
			}
			if((buf[i]=='b')&&(buf[i+1]=='r'))
			{
				k=0;
				interfacelist[j][k++]=buf[i];
				interfacelist[j][k++]=buf[i+1];
				interfacelist[j][k++]=buf[i+2];
				interfacelist[j++][k++]='\0';
				i=i+2;
				nBytesSent += req_format_write(wp,	
	   		("<option value=%u>%s</option>"),j-1,interfacelist[j-1]);
			}
			if((buf[i]=='p')&&(buf[i+1]=='p')&&(buf[i+2]=='p'))
			{
				k=0;
				interfacelist[j][k++]=buf[i];
				interfacelist[j][k++]=buf[i+1];
				interfacelist[j][k++]=buf[i+2];
				interfacelist[j][k++]=buf[i+3];
				interfacelist[j++][k++]='\0';
				i=i+3;
				nBytesSent += req_format_write(wp,	
	   		("<option value=%u>%s</option>"),j-1,interfacelist[j-1]);
			}
				
		}
		}
	fclose(fp);

err1:
	return nBytesSent;
}

#ifdef CONFIG_APP_RTL_QUAGGA
int ospflogList(request *wp, int argc, char **argv)
{
	FILE *fp;
	char  buf[200];
	int nBytesSent=0,enabled;
	apmib_get(MIB_OSPF_DEBUG, (void *)&enabled);
	
	if ( !(enabled & 1))
		goto err1;

	fp = fopen("/var/ospfd.log", "r");
	if (fp == NULL)
		goto err1;

        
	while(fgets(buf,200,fp)){
		nBytesSent += req_format_write(wp, ("%s"), buf);
	}
	fclose(fp);

err1:
	return nBytesSent;
}


int bgplogList(request *wp, int argc, char **argv)
{
	FILE *fp;
	char  buf[200];
	int nBytesSent=0,enabled;
	apmib_get(MIB_BGP_DEBUG, (void *)&enabled);
	
	if ( !(enabled & 1))
		goto err1;

	fp = fopen("/var/bgpd.log", "r");
	if (fp == NULL)
		goto err1;

        
	while(fgets(buf,200,fp)){
		nBytesSent += req_format_write(wp, ("%s"), buf);
	}
	fclose(fp);

err1:
	return nBytesSent;
}

int isislogList(request *wp, int argc, char **argv)
{
	FILE *fp;
	char  buf[200];
	int nBytesSent=0,enabled;
	apmib_get(MIB_ISIS_DEBUG, (void *)&enabled);
	
	if ( !(enabled & 1))
		goto err1;

	fp = fopen("/var/isisd.log", "r");
	if (fp == NULL)
		goto err1;

        
	while(fgets(buf,200,fp)){
		nBytesSent += req_format_write(wp, ("%s"), buf);
	}
	fclose(fp);

err1:
	return nBytesSent;
}

#endif

static int process_msg(char *msg, int is_wlan_only)
{
	char *p1, *p2;
	p1 = strstr(msg, "rlx-linux"); // host name
	if (p1 == NULL)
		return 0;

#ifdef CONFIG_RTK_MESH	
	if (is_wlan_only == 4) {
		p2 = strstr(p1, "msh");
		if (p2 && p2[4]==':')
			memcpy(p1, p2, strlen(p2)+1);
		else
			return 0;

	}else	
#endif

	if (is_wlan_only == 3){
		p2 = strstr(p1, "DoS");
		if (p2 && p2[3]==':'){
			memcpy(p1, p2, strlen(p2)+1);
		}else{
			p2 = strstr(p1, "wlan");	
			if ((p2 && p2[5]==':') || (p2 && p2[9]==':'))	{// vxd interface
				memcpy(p1, p2, strlen(p2)+1);
			}else	
				return 0;
			}	
	}else if (is_wlan_only == 2){
		p2 = strstr(p1, "DoS");
		if (p2 && p2[3]==':')
			memcpy(p1, p2, strlen(p2)+1);
		else
			return 0;

	}else{
		p2 = strstr(p1, "wlan");	
		if ((p2 && p2[5]==':') ||
			 (p2 && p2[9]==':'))	// vxd interface
			memcpy(p1, p2, strlen(p2)+1);
		else {
			if (is_wlan_only)
				return 0;

			// all system, just show all of them!
			p2 = strstr(p1, ": ");
			if (p2 == NULL)
				return 0;
			else
				memcpy(p1, p2+strlen(": "), strlen(p2)-strlen(": ")+1);
		}
	}
	return 1;
}


int sysLogList(request *wp, int argc, char **argv)
{
	FILE *fp;
	char  buf[200];
	int nBytesSent=0;
	int enabled;

//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to  SDKv2.5 from kernel 2.4
#ifdef RINGLOG
	char logname[32];
	int lognum = LOG_SPLIT;
#endif

//### end
	apmib_get(MIB_SCRLOG_ENABLED, (void *)&enabled);
	if ( !(enabled & 1))
		goto err1;
//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to  SDKv2.5 from kernel 2.4
#ifdef RINGLOG
		fp = fopen("/var/log/log_split", "r");
		if (fp == NULL)
			goto err1;
		fgets(buf,200,fp);
		lognum = atoi(buf);
		fclose(fp);
	
	while (lognum >= 0)
	{
		if (lognum > 0)
			snprintf(logname, 32, "/var/log/messages.%d", lognum-1);
		else if (lognum == 0)
			snprintf(logname, 32, "/var/log/messages");
		else
			goto err1;
	
		fp = fopen(logname, "r");
		if (fp == NULL)
			goto next_log;
#else
//### end
	fp = fopen("/var/log/messages", "r");
	if (fp == NULL)
		goto err1;
#endif
        
	while(fgets(buf,200,fp)){
		int ret=0;
		if (enabled&2) // system all
			ret = process_msg(buf, 0);
		else {
			if((enabled&0xC) == 0xC){ //both wlan and DoS
				ret = process_msg(buf, 3);
			}else if (enabled&4)	// wlan only
				ret = process_msg(buf, 1);
			else if (enabled&8)	//DoS only
				ret = process_msg(buf, 2);

#ifdef CONFIG_RTK_MESH			
			 if(enabled&16 && ret==0)	// mesh only
				ret = process_msg(buf, 4);
#endif

		}
		if (ret==0)
			continue;
		nBytesSent += req_format_write(wp, ("%s"), buf);
	}
	fclose(fp);

//### add by sen_liu 2011.4.21 sync the system log update (enlarge from 1 pcs to 8 pcs) to	SDKv2.5 from kernel 2.4
#ifdef RINGLOG
next_log:
	lognum--;
}
#endif
//### end
err1:
	return nBytesSent;
}

#ifdef  CONFIG_APP_SMTP_CLIENT
void formSmtpClient(request *wp, char *path, char *query)
{
	extern char* p_email_infor;
	char *send_form, *password,*send_to,*theme,*body, *attachment;
	char *timing,*year,*month,*day,*hour,*minute,*syslog_check,*usbstoragemsg,*now_time;
	char *tmpStr;
	int  enable,i,status;
	char tmpEmpty[] = "OFF";
	char tmpBuf[500];

	
	tmpStr= req_get_cstream_var(wp, ("smtpClientEnabled"), "");  
	if(!strcmp(tmpStr, "ON"))
		enable = 1;
	else
		enable = 0;
	/*if ( apmib_set(MIB_SMTP_CLIENT_ENABLED, (void *)&rt_enabled) == 0) {
			strcpy(tmpBuf, ("Set smtp client enable error!"));
			ERR_MSG(tmpBuf);
			return;
	}*/
	
	if(enable){
		syslog_check= req_get_cstream_var(wp, ("syslogmsg"), ""); 
		if(!strcmp(syslog_check, ""))
			syslog_check = tmpEmpty;
		send_form= req_get_cstream_var(wp, ("sendFrom"), "");
		if(!strcmp(send_form, ""))
			goto ipput_error;
		password= req_get_cstream_var(wp, ("password"), "");
		if(!strcmp(password, ""))
			goto ipput_error;
		send_to= req_get_cstream_var(wp, ("sendTo"), "");
		if(!strcmp(send_to, ""))
			goto ipput_error;
		theme= req_get_cstream_var(wp, ("theme"), "");
		if(!strcmp(theme, ""))
			theme = tmpEmpty;
		body = req_get_cstream_var(wp, ("msg"), "");
		if(!strcmp(body, ""))
			body = tmpEmpty;
		attachment = req_get_cstream_var(wp, ("attachment"), "");
		if(!strcmp(attachment, ""))
			attachment = tmpEmpty;
		usbstoragemsg = req_get_cstream_var(wp, ("usbstoragemsg"), "");
		if(!strcmp(usbstoragemsg, ""))
			usbstoragemsg = tmpEmpty;
		timing = req_get_cstream_var(wp, ("timing"), "");
		if(!strcmp(timing, "")){
			timing = tmpEmpty;
			year = tmpEmpty;
			month = tmpEmpty;
			day = tmpEmpty;
			hour = tmpEmpty;
			minute = tmpEmpty;
			now_time = tmpEmpty;
		}
		else{
			year = req_get_cstream_var(wp, ("year"), "");
			month = req_get_cstream_var(wp, ("month"), "");
			day = req_get_cstream_var(wp, ("day"), "");
			hour = req_get_cstream_var(wp, ("hour"), "");
			minute = req_get_cstream_var(wp, ("minute"), "");
			now_time  = req_get_cstream_var(wp, ("now_time"), "");
		}
		
		sprintf(tmpBuf,"%s \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" ","smtpclient",send_form,password,send_to,syslog_check,timing,year,month,day,hour,minute,now_time,theme,body,attachment,usbstoragemsg);
		status = system( tmpBuf);
	}
	strcpy(tmpBuf, ("Send Email End!"));
	//OK_MSG(tmpBuf);
	ERR_MSG(tmpBuf);
	return;
	
	ipput_error:
		strcpy(tmpBuf, ("Send Email Error!"));
		//OK_MSG(tmpBuf);
		ERR_MSG(tmpBuf);
}
#endif


#ifdef HOME_GATEWAY
#ifdef DOS_SUPPORT
void formDosCfg(request *wp, char *path, char *query)
{
	char	*submitUrl, *tmpStr;
	char	tmpBuf[100];
	int	floodCount=0,blockTimer=0;
	long	enabled = 0;

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page

	apmib_get(MIB_DOS_ENABLED, (void *)&enabled);

	tmpStr = req_get_cstream_var(wp, ("dosEnabled"), "");
	if(!strcmp(tmpStr, "ON")) {
		enabled |= 1;

		tmpStr = req_get_cstream_var(wp, ("sysfloodSYN"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 2;
			tmpStr = req_get_cstream_var(wp, ("sysfloodSYNcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_SYSSYN_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS SYSSYN_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~2;
		}
		tmpStr = req_get_cstream_var(wp, ("sysfloodFIN"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 4;
			tmpStr = req_get_cstream_var(wp, ("sysfloodFINcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_SYSFIN_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS SYSFIN_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~4;
		}
		tmpStr = req_get_cstream_var(wp, ("sysfloodUDP"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 8;
			tmpStr = req_get_cstream_var(wp, ("sysfloodUDPcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_SYSUDP_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS SYSUDP_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~8;
		}
		tmpStr = req_get_cstream_var(wp, ("sysfloodICMP"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x10;
			tmpStr = req_get_cstream_var(wp, ("sysfloodICMPcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_SYSICMP_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS SYSICMP_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~0x10;
		}
		tmpStr = req_get_cstream_var(wp, ("ipfloodSYN"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x20;
			tmpStr = req_get_cstream_var(wp, ("ipfloodSYNcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_PIPSYN_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS PIPSYN_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~0x20;
		}
		tmpStr = req_get_cstream_var(wp, ("ipfloodFIN"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x40;
			tmpStr = req_get_cstream_var(wp, ("ipfloodFINcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_PIPFIN_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS PIPFIN_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~0x40;
		}
		tmpStr = req_get_cstream_var(wp, ("ipfloodUDP"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x80;
			tmpStr = req_get_cstream_var(wp, ("ipfloodUDPcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_PIPUDP_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS PIPUDP_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~0x80;
		}
		tmpStr = req_get_cstream_var(wp, ("ipfloodICMP"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x100;
			tmpStr = req_get_cstream_var(wp, ("ipfloodICMPcount"), "");
			string_to_dec(tmpStr,&floodCount);
			if ( apmib_set(MIB_DOS_PIPICMP_FLOOD, (void *)&floodCount) == 0) {
				strcpy(tmpBuf, ("Set DoS PIPICMP_FLOOD error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~0x100;
		}
		tmpStr = req_get_cstream_var(wp, ("TCPUDPPortScan"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x200;

			tmpStr = req_get_cstream_var(wp, ("portscanSensi"), "");
			if( tmpStr[0]=='1' ) {
				enabled |= 0x800000;
			}
			else{
				enabled &= ~0x800000;
			}
		}
		else{
			enabled &= ~0x200;
		}
		tmpStr = req_get_cstream_var(wp, ("ICMPSmurfEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x400;
		}
		else{
			enabled &= ~0x400;
		}
		tmpStr = req_get_cstream_var(wp, ("IPLandEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x800;
		}
		else{
			enabled &= ~0x800;
		}
		tmpStr = req_get_cstream_var(wp, ("IPSpoofEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x1000;
		}
		else{
			enabled &= ~0x1000;
		}
		tmpStr = req_get_cstream_var(wp, ("IPTearDropEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x2000;
		}
		else{
			enabled &= ~0x2000;
		}
		tmpStr = req_get_cstream_var(wp, ("PingOfDeathEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x4000;
		}
		else{
			enabled &= ~0x4000;
		}
		tmpStr = req_get_cstream_var(wp, ("TCPScanEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x8000;
		}
		else{
			enabled &= ~0x8000;
		}
		tmpStr = req_get_cstream_var(wp, ("TCPSynWithDataEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x10000;
		}
		else{
			enabled &= ~0x10000;
		}
		tmpStr = req_get_cstream_var(wp, ("UDPBombEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x20000;
		}
		else{
			enabled &= ~0x20000;
		}
		tmpStr = req_get_cstream_var(wp, ("UDPEchoChargenEnabled"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x40000;
		}
		else{
			enabled &= ~0x40000;
		}
		tmpStr = req_get_cstream_var(wp, ("sourceIPblock"), "");
		if(!strcmp(tmpStr, "ON")) {
			enabled |= 0x400000;
			tmpStr = req_get_cstream_var(wp, ("IPblockTime"), "");
			string_to_dec(tmpStr,&blockTimer);
			if ( apmib_set(MIB_DOS_BLOCK_TIME, (void *)&blockTimer) == 0) {
				strcpy(tmpBuf, ("Set DoS IP Block Timer error!"));
				goto setErr;
			}
		}
		else{
			enabled &= ~0x400000;
		}
	}
	else
		enabled = 0;

	if ( apmib_set(MIB_DOS_ENABLED, (void *)&enabled) == 0) {
		strcpy(tmpBuf, ("Set DoS enable error!"));
		goto setErr;
	}

	apmib_update_web(CURRENT_SETTING);

#ifdef REBOOT_CHECK
	run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
	run_init_script("all");
#endif

	OK_MSG(submitUrl);
	return;

setErr:
	ERR_MSG(tmpBuf);
}
#endif
#endif


#ifdef LOGIN_URL

#define MAX_USER	5
#define ACCESS_TIMEOUT	 300	// 5m

#define MAGIC_NUMER	7168186

struct user_profile {
	int flag;
	time_t last_time;
	char ipaddr[32];		
};

static struct user_profile users[MAX_USER];

/*
 * Name: base64decode
 *
 * Description: Decodes BASE-64 encoded string
 */
static int base64decode(void *dst,char *src,int maxlen)
{
	int bitval,bits;
	int val;
	int len,x,y;

	len = strlen(src);
	bitval=0;
	bits=0;
	y=0;

	for(x=0;x<len;x++)
	{
		if ((src[x]>='A')&&(src[x]<='Z')) val=src[x]-'A'; else
			if ((src[x]>='a')&&(src[x]<='z')) val=src[x]-'a'+26; else
				if ((src[x]>='0')&&(src[x]<='9')) val=src[x]-'0'+52; else
					if (src[x]=='+') val=62; else
						if (src[x]=='-') val=63; else
							val=-1;
		if (val>=0)
		{
			bitval=bitval<<6;
			bitval+=val;
			bits+=6;
			while (bits>=8)
			{
				if (y<maxlen)
					((char *)dst)[y++]=(bitval>>(bits-8))&0xFF;
				bits-=8;
				bitval &= (1<<bits)-1;
			}
		}
	}
	if (y<maxlen)
		((char *)dst)[y++]=0;
	return y;
}

///////////////////////////////////////////////////////////////////
static void delete_user(request *wp)
{
	int i;
	for (i=0; i<MAX_USER; i++) {
		if (users[i].flag == MAGIC_NUMER && !strcmp(wp->remote_ip_addr, users[i].ipaddr)) {
			users[i].flag = 0;
			return;
		}			
	}
}

///////////////////////////////////////////////////////////////////
static int add_user(request *wp)
{
	int i;
	for (i=0; i<MAX_USER; i++) {
		if (users[i].flag == MAGIC_NUMER && strcmp(wp->remote_ip_addr, users[i].ipaddr) &&
			((unsigned long)current_time)-((unsigned long)users[i].last_time) < ACCESS_TIMEOUT )
			continue;
				
		users[i].flag = MAGIC_NUMER;		
		users[i].last_time = current_time;	
		strcpy(users[i].ipaddr, wp->remote_ip_addr);				
		return 0;
	}

	printf("boa: add_user error (exceed max connection)!\n");

	return -1;
}

///////////////////////////////////////////////////////////////////
int is_valid_user(request *wp)
{
	int i;
    char user_name[MAX_NAME_LEN];
    char user_password[MAX_NAME_LEN];

    apmib_get(MIB_USER_NAME, user_name);
    
    if (strcmp(user_name, "") == 0) 
    {
        return 1;
    }
    
	for (i=0; i<MAX_USER; i++) {
		if (users[i].flag == MAGIC_NUMER && !strcmp(wp->remote_ip_addr, users[i].ipaddr)) {
			if (((unsigned long)current_time)-((unsigned long)users[i].last_time) > ACCESS_TIMEOUT)
				return -1; // timeout

            users[i].last_time = current_time;
			return 1;
		}
	}

	return 0; // not a valid user
}

int isUserExists(char *name)
{
    char user_name[MAX_NAME_LEN];
    
    apmib_get(MIB_USER_NAME, user_name);
    if(strcmp(name, user_name) == 0)
        return 1;
    else
        return 0;
    
}

int isValidPassword(char *password)
{
    char user_password[MAX_NAME_LEN];

    apmib_get(MIB_USER_PASSWORD, user_password);
    if(strcmp(password, user_password) == 0)
        return 1;
    else
        return 0;
}
///////////////////////////////////////////////////////////////////
void formLogin(request *wp, char *path, char *query)
{
	char *strUser, *strPassword;
	char tmpbuf[200];
    int err_code = 0;

	strUser = req_get_cstream_var(wp, ("username"), "");
	strPassword = req_get_cstream_var(wp, ("password"), "");
	if ( strUser[0] && !strPassword[0] ) {
		err_code = 0;
		goto login_err;
	}

    base64decode(tmpbuf, strUser, sizeof(tmpbuf));
	if (!isUserExists(tmpbuf)) {
		err_code = 1;
		goto login_err;
	}

    base64decode(tmpbuf, strPassword, sizeof(tmpbuf));
    if (!isValidPassword(tmpbuf)) {
		err_code = 2;
		goto login_err;
	}
	
	if (add_user(wp) < 0) {
		err_code = 3;
		goto login_err;
	}

	send_redirect_perm(wp, (WEB_PAGE_HOME));
#ifdef CSRF_SECURITY_PATCH
    log_boaform("formLogin", wp);  //To set formLogin valid at security_tbl
#endif
	return;

login_err:
    sprintf(tmpbuf, WEB_PAGE_LOGIN "?err_code=%d", err_code);
	send_redirect_perm(wp, tmpbuf);
}
#endif // LOGIN_URL

#ifdef CONFIG_CPU_UTILIZATION
void formCpuUtilization(request *wp, char *path, char *query)
{
	char *submitUrl, *tmpStr;
	int enable, interval, oldStatus;
	char tmpbuf[200];

	submitUrl = req_get_cstream_var(wp, "submit-url", "");   // hidden page
	
	tmpStr = req_get_cstream_var(wp, ("enableCpuUtilization"), "");
	if(!strcmp(tmpStr, "ON"))
		enable = 1 ;
	else 
		enable = 0 ;

	apmib_get( MIB_ENABLE_CPU_UTILIZATION, (void *)&oldStatus );
	if ( oldStatus!=enable && apmib_set( MIB_ENABLE_CPU_UTILIZATION, (void *)&enable) == 0) {
		strcpy(tmpbuf, ("Set cpu utilization enabled flag error!"));
		goto setErr_end;
	}
	
	tmpStr = req_get_cstream_var(wp, ("cpuUtilizationInterval"), "");
	if(tmpStr[0])
	{
		FILE* fp;
		
		interval = atoi(tmpStr);
		if ( apmib_set( MIB_CPU_UTILIZATION_INTERVAL, (void *)&interval) == 0) {
			strcpy(tmpbuf, ("Set cpu utilization interval error!"));
			goto setErr_end;
		}
		fp = fopen("/var/cpu_info.conf", "w+");
		if( fp!=NULL ){
			fprintf(fp, "interval: %d", interval);
			fclose(fp);
		}
		fp = fopen("/var/run/cpu.pid", "r");
		if( fp!=NULL ){
			unsigned int pid; 
			fscanf(fp, "%ud", &pid);
			kill((pid_t)pid, 10);
			fclose(fp);
		}
	}

	apmib_update_web(CURRENT_SETTING);

	OK_MSG(submitUrl);
	return;

setErr_end:
	ERR_MSG(tmpbuf);
}

#endif // CONFIG_CPU_UTILIZATION

#if defined(POWER_CONSUMPTION_SUPPORT)
unsigned int pre_cpu_d4, pre_time_secs, max_cpu_delta=0;
unsigned int ethBytesCount_previous[5] = {0};

/* http://www.360doc.com/content/070213/11/17255_365683.html */
int getPowerConsumption(request *wp, int argc, char **argv)
{
	//char dev[80];
	//char *devPtr;
	FILE *stream;
	int i=1;
	//int j;
	//char logbuf[500];
	//unsigned int rxbytes=0,rxpackets=0,rxerrs=0,rxdrops=0,txbytes=0,txpackets=0,txerrs=0,txdrops=0,txcolles=0;
	//unsigned int txeth0packets=0;
	//unsigned int tmp1,tmp2,tmp3,tmp4;
	char askfor[20];

//	unsigned int totalPwrCon = 0;
	unsigned int totalPwrCon = (rand()%2 ? 10 :0);
	
	typedef enum { NO_LINK=0, NORMAL_LINK=1, EEE_LINK=2} ETHERNET_LINK_T;
	unsigned short isLink_eth0[5]={0};
	unsigned short ethLinkNum= 0, ethEeeLinkNum = 0;
	unsigned short perEthPwrCon = PWRCON_PER_ETHERNET;
	unsigned int perEthEeeMinus = PWRCON_PER_EEE_ETHERNET_LINK_MINUS; // mw*100
	unsigned int perEthEeePwrCon = PWRCON_PER_EEE_ETHERNET; // mw*100/Mbps
	unsigned int ethThroughPut[5] = {0};
	unsigned int ethEeeThroughPut_Total = 0;
	int ethPwrCon_Total = 0;
	
	typedef enum { CHIP_UNKNOWN=0, CHIP_RTL8188C=1, CHIP_RTL8192C=2} CHIP_VERSION_T;
	CHIP_VERSION_T chipVersion = CHIP_UNKNOWN;	
	
	typedef enum { CPU_NORMAL=0, CPU_SUSPEND=1} CPU_MODE_T;
	CPU_MODE_T cpuMode = CPU_NORMAL;
	unsigned short cpuPwrCon[3][2] = { {0,0},{PWRCON_CPU_NORMAL_88C,PWRCON_CPU_SUSPEND_88C},{PWRCON_CPU_NORMAL_92C,PWRCON_CPU_SUSPEND_92C} }; // 3:chipVersion; 2:cpu mode
	
	typedef enum { WLAN_OFF=0, WLAN_NO_LINK=1, WLAN_LINK=2} WLAN_STATE_T;
	WLAN_STATE_T wlanState = WLAN_OFF; 
	unsigned short wlanStatePwrCon[3][3] = { {0,0,0},{PWRCON_WLAN_OFF_88C,PWRCON_WLAN_NOLINK_88C,PWRCON_WLAN_LINK_88C},{PWRCON_WLAN_OFF_92C,PWRCON_WLAN_NOLINK_92C,PWRCON_WLAN_LINK_92C}}; //3:chipVersion; 3:wlanState
	int wlanOff = 0;
	
	typedef enum { WLAN_MCS8_15=0, WLAN_MCS0_7=1, WLAN_OFDM=2, WLAN_CCK=3} WLAN_TRAFFIC_STATE_T;
	WLAN_TRAFFIC_STATE_T wlanTrafficState = WLAN_MCS8_15;
	unsigned int wlanTrafficStatePwrCon[3][4] = { {0,0,0,0},{PWRCON_WLAN_TRAFFIC_MCS8_15_88C,PWRCON_WLAN_TRAFFIC_MCS0_7_88C,PWRCON_WLAN_TRAFFIC_OFDM_88C,PWRCON_WLAN_TRAFFIC_CCK_88C},{PWRCON_WLAN_TRAFFIC_MCS8_15_92C,PWRCON_WLAN_TRAFFIC_MCS0_7_92C,PWRCON_WLAN_TRAFFIC_OFDM_92C,PWRCON_WLAN_TRAFFIC_CCK_92C}}; //3:chipVersion; 4:wlanTrafficState
	unsigned int wlanTrafficStatePwrConZ[3][28] = { 
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
		{1000,1099,1188,1454,2014,3254,4271,8039,1082,1176,1289,1681,2768,4482,6275,10458,719,919,1225,1697,2377,3735,5038,7557,938,1681,2894,5865},
		{1000,1099,1188,1454,2014,3254,4271,8039,1082,1176,1289,1681,2768,4482,6275,10458,1000,1278,1705,2360,3306,5195,7008,10511,938,1681,2894,5865}
	};//3:chipVersion; 28:DataRate MCS15~1
	unsigned int wlanTrafficZ = 0;
	
	unsigned int tx_average = 0;
	unsigned short tx_average_multiply2 = 0;
	unsigned int rx_average = 0;
	unsigned int wlanTrafficStatePwrCon_Total;
		
	unsigned int cpuUtilizationPwrCon[3] = { 0,PWRCON_CPU_UTILIZATION_88C,PWRCON_CPU_UTILIZATION_92C}; //3:chipVersion;
	unsigned short cpu_utilization=0;
	
	unsigned short debug_check = 0;
	
	time_t current_secs;
	unsigned int time_delta = 1;						
#if 0	
	for(i=0 ;i<3;i++)
		for(j=0; j<1; j++)
			fprintf(stderr,"\r\n cpuUtilizationPwrCon[%d][%d]=[%f]",i,j,cpuUtilizationPwrCon[i][j]);
#endif			

	//get current system time in second.
	time(&current_secs);
	if(pre_time_secs == 0) //first time
	{
		pre_time_secs = (int)(current_secs);
		time_delta = 1;
	}
	else
	{
		time_delta = (int)(current_secs) - (int)(pre_time_secs);
		pre_time_secs = (int)(current_secs);
	}

		
	//get chipVersion
	stream = fopen ( "/var/pwrConDebug", "r" );
	if ( stream != NULL )
	{		
		char *strtmp;
		char line[100];				
		char strTmp[10];
		
		while (fgets(line, sizeof(line), stream))
		{
			strtmp = line;
			
			while(*strtmp == ' ')
			{
				strtmp++;
			}

			sscanf(strtmp,"%[01]",strTmp);

			debug_check=atoi(strTmp);
			
		}
		
		fclose ( stream );
	}

	
	if(debug_check)
		fprintf(stderr,"\r\n  === Pwr Con Debug ===");
	//get chipVersion
	chipVersion = getWLAN_ChipVersion();
#if 0	
	stream = fopen ( "/proc/wlan0/mib_rf", "r" );
	if ( stream != NULL )
	{		
		char *strtmp;
		char line[100];
								 
		while (fgets(line, sizeof(line), stream))
		{
			
			strtmp = line;
			while(*strtmp == ' ')
			{
				strtmp++;
			}
			

			if(strstr(strtmp,"RTL8192SE") != 0)
			{
				chipVersion = CHIP_UNKNOWN;
			}
			else if(strstr(strtmp,"RTL8188C") != 0)
			{
				if(debug_check)
					fprintf(stderr,"\r\n [%s]",strtmp);				
				chipVersion = CHIP_RTL8188C;
			}
			else if(strstr(strtmp,"RTL8192C") != 0)
			{
				if(debug_check)				
					fprintf(stderr,"\r\n [%s]",strtmp);				
				chipVersion = CHIP_RTL8192C;
			}
		}			
		fclose ( stream );
	}
#endif

	if(debug_check)
	{
		fprintf(stderr,"\r\n chipVersion=[%u]",chipVersion);
		fprintf(stderr,"\r\n");
	}
	
	//get cpu mode
	stream = fopen ( "/proc/suspend_check", "r" );
	if ( stream != NULL )
	{		
		char *strtmp;
		char line[100];
		
		while (fgets(line, sizeof(line), stream))
		{			
			//enable=1, winsize=5(10), high=3200, low=2200, suspend=1
			strtmp = strstr(line,"suspend");
			if(strtmp != NULL)
			{
				
				//suspend=1
				if(debug_check)
					fprintf(stderr,"\r\n [%s]",strtmp);
				sscanf(strtmp,"%*[^=]=%u",&cpuMode);								
			}
			
		}			
		fclose ( stream );
	}
	if(debug_check)
	{
		fprintf(stderr,"\r\n cpuMode=[%u]",cpuMode);
		fprintf(stderr,"\r\n cpuPwrCon=[%u]",cpuPwrCon[chipVersion][cpuMode]);
		fprintf(stderr,"\r\n");
	}
	totalPwrCon+=cpuPwrCon[chipVersion][cpuMode];
	
	//get Eth0 port link and bytesCount
	for(i=0; i<5; i++)
	{
		unsigned int ethBytesCount[5] = {0};
		
		isLink_eth0[i]=getEth0PortLink(i);
		if(isLink_eth0[i])
		{
			isLink_eth0[i] = NORMAL_LINK;
			if(getEthernetEeeState(i))
				isLink_eth0[i] = EEE_LINK;			
	}
		else
		{
			isLink_eth0[i] = NO_LINK;
		}
		
		ethBytesCount[i] = getEthernetBytesCount(i);
		
		if(time_delta <= 0)
			time_delta = 1;
		ethThroughPut[i] = (ethBytesCount[i] - ethBytesCount_previous[i])/time_delta;		
		ethBytesCount_previous[i] = ethBytesCount[i];
	}
	
	for(i=0; i<5; i++)
	{
		if(isLink_eth0[i] == NORMAL_LINK)
		{
			ethLinkNum++;
		}
		else if(isLink_eth0[i] == EEE_LINK)
		{
			ethEeeLinkNum++;
			ethEeeThroughPut_Total += ethThroughPut[i];
		}						
	}
	ethEeeThroughPut_Total *= 8; // transfer to bits.
	
	ethPwrCon_Total += ethLinkNum*perEthPwrCon;
	ethPwrCon_Total -= (ethEeeLinkNum*perEthEeeMinus)/100;
	ethPwrCon_Total += (((float)ethEeeThroughPut_Total*perEthEeePwrCon)/100)/1000000;
	
	
	if(debug_check)
	{
		fprintf(stderr,"\r\n Eth Link State:%u-%u-%u-%u-%u", isLink_eth0[0],isLink_eth0[1],isLink_eth0[2],isLink_eth0[3],isLink_eth0[4]);
		fprintf(stderr,"\r\n Eth ThroughPut:%u-%u-%u-%u-%u (bits/sec)", ethThroughPut[0]*8,ethThroughPut[1]*8,ethThroughPut[2]*8,ethThroughPut[3]*8,ethThroughPut[4]*8);
		fprintf(stderr,"\r\n ethEeeThroughPut_Total: %u (bits/sec)",ethEeeThroughPut_Total);
		fprintf(stderr,"\r\n perEthPwrCon Total: (%u*%u)-(%u*%u)/100+(%u*%u)/100/10^6 = %u",ethLinkNum,perEthPwrCon,ethEeeLinkNum,perEthEeeMinus,ethEeeThroughPut_Total,perEthEeePwrCon,ethPwrCon_Total);
		fprintf(stderr,"\r\n");
	}
	totalPwrCon+=ethPwrCon_Total;

	//get wlan state
	apmib_get( MIB_WLAN_WLAN_DISABLED, (void *)&wlanOff);
	if(wlanOff)
		wlanState = WLAN_OFF;
	else
	{
		wlanState = updateWlanifState("wlan0");								
	}										
					
	
	if(debug_check)
					{
		fprintf(stderr,"\r\n wlanState=[%u]",wlanState);	
		fprintf(stderr,"\r\n wlanStatePwrCon = [%u]",wlanStatePwrCon[chipVersion][wlanState]);
		fprintf(stderr,"\r\n");
	}
						
	totalPwrCon+=wlanStatePwrCon[chipVersion][wlanState];
		
	// get wlan traffic power consumption
	if(wlanState == WLAN_LINK)
	{
			//get chipVersion
		stream = fopen ( "/proc/wlan0/stats", "r" );
		if ( stream != NULL )
		{		
			char *strtmp;
			char line[100];
			while (fgets(line, sizeof(line), stream))
			{
				char *p;
				strtmp = line;
				
				
				while(*strtmp == ' ')
					strtmp++;
				
				
				if(strstr(strtmp,"tx_avarage") != 0)
				{
					char str1[10];
					
					if(debug_check)																
						fprintf(stderr,"\r\n [%s]",strtmp);
						
					//tx_avarage:    1449
					sscanf(strtmp, "%*[^:]:%s",str1);
					
					p = str1;
					while(*p == ' ')
						p++;
					
					tx_average = atoi(p);	
					tx_average*=8; // bytes->bits
					
					if(debug_check)
						fprintf(stderr,"\r\n tx_average=[%u]",tx_average);
				}
				else if(strstr(strtmp,"rx_avarage") != 0)
				{
					char str1[10];
					
					if(debug_check)																
						fprintf(stderr,"\r\n [%s]",strtmp);
						
					//rx_avarage:    1449
					sscanf(strtmp, "%*[^:]:%s",str1);
					
					p = str1;
					while(*p == ' ')
						p++;
					
					rx_average = atoi(p);	
					rx_average*=8; // bytes->bits
					
					if(debug_check)
						fprintf(stderr,"\r\n rx_average=[%u]",rx_average);
					}
				else if(strstr(strtmp,"cur_tx_rate") != 0)
				{
					char str1[10];
					unsigned short OFDM_CCK = 0;
					
					if(debug_check)
						fprintf(stderr,"\r\n [%s]",strtmp);
					
					//cur_tx_rate:   MCS[8-15]
					//cur_tx_rate:   MCS[0-7]
					//cur_tx_rate:   [1,2,5,11]
					//cur_tx_rate:   [6,9,12,18,24,36,48,54]
					sscanf(strtmp, "%*[^:]:%s",str1);
					p = str1;
					while(*p == ' ')
						p++;
					
					if(debug_check)
						fprintf(stderr,"\r\n p=[%s]",p);
											
					if(strstr(p, "MCS8") != 0 || strstr(p, "MCS9") != 0 ||
						 strstr(p, "MCS10") != 0 || strstr(p, "MCS11") != 0 ||
						 strstr(p, "MCS12") != 0 || strstr(p, "MCS13") != 0 ||
						 strstr(p, "MCS14") != 0 || strstr(p, "MCS15") != 0 )
					{
						wlanTrafficState = WLAN_MCS8_15;																																	
					}
					else if(strstr(p, "MCS0") != 0 || strstr(p, "MCS1") != 0 ||
									 strstr(p, "MCS2") != 0 || strstr(p, "MCS3") != 0 ||
									 strstr(p, "MCS4") != 0 || strstr(p, "MCS5") != 0 ||
									 strstr(p, "MCS6") != 0 || strstr(p, "MCS7") != 0 )
					{						
						wlanTrafficState = WLAN_MCS0_7;						
					}
					else
					{
						OFDM_CCK = atoi(p);
						
						if(OFDM_CCK == 1 || OFDM_CCK == 2 || OFDM_CCK == 5 || OFDM_CCK ==11)										 
						{
							wlanTrafficState = WLAN_CCK;													
						}
						else if(OFDM_CCK == 6 || OFDM_CCK == 9 || OFDM_CCK == 12 || OFDM_CCK == 18 ||
							      OFDM_CCK == 24 || OFDM_CCK == 36 || OFDM_CCK == 48 || OFDM_CCK == 54 )
						{
							wlanTrafficState = WLAN_OFDM;
						}
					}
					
					if(strstr(p, "MCS15") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][0];
					else if(strstr(p, "MCS14") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][1];
					else if(strstr(p, "MCS13") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][2];
					else if(strstr(p, "MCS12") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][3];						
					else if(strstr(p, "MCS11") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][4];
					else if(strstr(p, "MCS10") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][5];
					else if(strstr(p, "MCS9") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][6];
					else if(strstr(p, "MCS8") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][7];
					else if(strstr(p, "MCS7") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][8];
					else if(strstr(p, "MCS6") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][9];
					else if(strstr(p, "MCS5") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][10];
					else if(strstr(p, "MCS4") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][11];
					else if(strstr(p, "MCS3") != 0)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][12];
					else if(strstr(p, "MCS2") != 0)	
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][13];
					else if(strstr(p, "MCS1") != 0)	
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][14];
					else if(strstr(p, "MCS0") != 0)	
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][15];																
					else if(OFDM_CCK == 54)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][16];							
					else if(OFDM_CCK == 48)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][17];
					else if(OFDM_CCK == 36)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][18];
					else if(OFDM_CCK == 24)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][19];
					else if(OFDM_CCK == 18)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][20];
					else if(OFDM_CCK == 12)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][21];
					else if(OFDM_CCK == 9)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][22];
					else if(OFDM_CCK == 6)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][23];
					else if(OFDM_CCK == 11)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][24];
					else if(OFDM_CCK == 5)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][25];
					else if(OFDM_CCK == 2)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][26];
					else if(OFDM_CCK == 1)
						wlanTrafficZ = wlanTrafficStatePwrConZ[chipVersion][27];
															
				}
				
			}
			fclose(stream );
			
		}
	}
	
	if(debug_check)
		fprintf(stderr,"\r\n wlanTrafficState=[%u], wlanTrafficZ=[%u]",wlanTrafficState, wlanTrafficZ);

	switch(wlanTrafficState)
	{
		case WLAN_MCS8_15:
			//tx_average /= 1000000;
			if(tx_average > 95000000)
				tx_average = 95000000;
									
			wlanTrafficStatePwrCon_Total = ((((float)tx_average*wlanTrafficStatePwrCon[chipVersion][wlanTrafficState]*wlanTrafficZ)/1000)/100)/1000000;
			if(debug_check)
				fprintf(stderr,"\r\n wlanTrafficStatePwrCon_Total:(((%u*%u*%u)/1000)/100)/10^6 = [%u]",tx_average,wlanTrafficStatePwrCon[chipVersion][wlanTrafficState],wlanTrafficZ,wlanTrafficStatePwrCon_Total);
	
			totalPwrCon+=wlanTrafficStatePwrCon_Total;
			break;
		case WLAN_MCS0_7:
			//tx_average /= 1000000;
			if(tx_average > 90000000)
				tx_average = 90000000;
		
			wlanTrafficStatePwrCon_Total = ((((float)tx_average*wlanTrafficStatePwrCon[chipVersion][wlanTrafficState]*wlanTrafficZ)/1000)/100)/1000000;
			if(debug_check)
				fprintf(stderr,"\r\n wlanTrafficStatePwrCon_Total:(((%u*%u*%u)/1000)/100)/10^6 = [%u]",tx_average,wlanTrafficStatePwrCon[chipVersion][wlanTrafficState],wlanTrafficZ,wlanTrafficStatePwrCon_Total);
			
			totalPwrCon+=wlanTrafficStatePwrCon_Total;
			break;
		case WLAN_OFDM:
			//tx_average /= 1000000;
			if(tx_average > 25000000)
				tx_average = 25000000;
			
			wlanTrafficStatePwrCon_Total = ((((float)tx_average*wlanTrafficStatePwrCon[chipVersion][wlanTrafficState]*wlanTrafficZ)/1000)/100)/1000000;
			if(debug_check)
				fprintf(stderr,"\r\n wlanTrafficStatePwrCon_Total:(((%u*%u*%u)/1000)/100)/10^6 = [%u]",tx_average,wlanTrafficStatePwrCon[chipVersion][wlanTrafficState],wlanTrafficZ,wlanTrafficStatePwrCon_Total);
			
			totalPwrCon+=wlanTrafficStatePwrCon_Total;
			break;
		case WLAN_CCK:

		wlanTrafficStatePwrCon_Total = ((((float)tx_average*wlanTrafficStatePwrCon[chipVersion][wlanTrafficState]*wlanTrafficZ)/1000)/100)/1000000;
			if(debug_check)
				fprintf(stderr,"\r\n wlanTrafficStatePwrCon_Total:(((%u*%u*%u)/1000)/100)/10^6 = [%u]",tx_average, wlanTrafficStatePwrCon[chipVersion][wlanTrafficState],wlanTrafficZ,wlanTrafficStatePwrCon_Total);
			totalPwrCon+=wlanTrafficStatePwrCon_Total;
			break;						
	}
	
	//get CPU utilization
	stream = fopen ( "/proc/stat", "r" );
	if ( stream != NULL )
	{
		char buf[512];
		unsigned int d1, d2, d3, d4;
		
		fgets(buf, sizeof(buf), stream);	/* eat line */
				
		
		sscanf(buf, "cpu %d %d %d %d", &d1, &d2, &d3, &d4);
		fclose(stream);
				
		if(pre_cpu_d4 == 0)
		{
			pre_cpu_d4 = d4;
		}
		else
		{			
			
			unsigned int delta = 0;						
				
			delta = (d4 - pre_cpu_d4)/time_delta;
			
			pre_cpu_d4 = d4;
			if(delta > max_cpu_delta)
				max_cpu_delta = delta;
			
			cpu_utilization = 100 - (int)(delta*100/max_cpu_delta);

			if(debug_check)
				fprintf(stderr,"\r\n cpu_busy: (%u*%u)/100=[%u] ",cpu_utilization,cpuUtilizationPwrCon[chipVersion],((cpu_utilization*cpuUtilizationPwrCon[chipVersion])/100));

	}

	}

	if(cpuMode == CPU_NORMAL)
		totalPwrCon+=((cpu_utilization*cpuUtilizationPwrCon[chipVersion])/100);


	if(1 || strcmp(askfor,"all")==0){

		
		if(debug_check)
		fprintf(stderr,"\r\n totalPwrCon=%u",totalPwrCon);
			
		if(tx_average_multiply2)
			tx_average/=2;
			
		req_format_write(wp, "<interface><name>LAN</name><type>LAN</type><totalPwrCon>%d</totalPwrCon><wlanTx>%d</wlanTx><wlanRx>%d</wlanRx></interface>",totalPwrCon,tx_average,rx_average);
		
	}

	return 0;
	
}
#endif // #if defined(POWER_CONSUMPTION_SUPPORT)

#ifdef CONFIG_RTL_ETH_802DOT1X_CLIENT_MODE_SUPPORT
#define RS_CERT_START "-----BEGIN CERTIFICATE-----"
#define RS_CERT_END "-----END CERTIFICATE-----"

#define RS_RSA_PRIV_KEY_START "-----BEGIN RSA PRIVATE KEY-----"
#define RS_RSA_PRIV_KEY_END "-----END RSA PRIVATE KEY-----"
#define RS_PRIV_KEY_TIP "PRIVATE KEY-----"



void formUploadEth8021xUserCert(request *wp, char * path, char * query)
{
	char *submitUrl,*strVal, *deleteAllCerts, *user_certstart,*ca_certstart;
	char tmpBuf[MAX_MSG_BUFFER_SIZE]={0};
	int user_cert_len,ca_cert_len;
	char cmd[256];
	FILE *fp;
	char tryFormChange;
	char line[256];
	unsigned char userKeyPass[MAX_RS_USER_CERT_PASS_LEN+1];
	char certOk, userKeyOk;
	int wlanIdx_5G,wlanIdx_2G,rsBandSel;

	//printf("---%s:%d---sizeof(upload_data)=%d	upload_len=%d\n",__FUNCTION__,__LINE__,wp->upload_data,wp->upload_len);
	
	strVal = req_get_cstream_var_in_mime(wp, ("uploadCertType"), "",NULL);
	submitUrl = req_get_cstream_var_in_mime(wp, ("submit-url"), "",NULL);   // hidden page
	deleteAllCerts = req_get_cstream_var_in_mime(wp, ("delAllCerts"), "",NULL);   // hidden page

	if(deleteAllCerts[0]=='1')
	{
		//To delete all 802.1x certs
		system("rsCert -rst_eth");
		strcpy(tmpBuf,"Delete all 802.1x cerificates of ethernet success!");
		
	}
	else
	{
		//Initial
		tryFormChange=0;
		certOk=0;
		userKeyOk=0;	
	
		if(NULL == strstr(wp->upload_data,RS_CERT_START)|| NULL ==strstr(wp->upload_data,RS_CERT_END))
		{
			//printf("---%s:%d---No 802.1x cert inclued in upload file!\n",__FUNCTION__,__LINE__);
			strcpy(tmpBuf,"No 802.1x cert inclued in upload file!");
			tryFormChange=1;
		}

		if((tryFormChange==0)&&(!strcmp(strVal,"user")))
		{
			//if(NULL == strstr(wp->upload_data,RS_PRIV_KEY_TIP))				
			if((NULL ==strstr(wp->upload_data,RS_RSA_PRIV_KEY_START)) || (NULL ==strstr(wp->upload_data,RS_RSA_PRIV_KEY_END)))
			{			
				//printf("---%s:%d---No 802.1x private key inclued in upload file!\n",__FUNCTION__,__LINE__);
				strcpy(tmpBuf,"No 802.1x private key inclued in upload file!");
				tryFormChange=1;
			}
		}	
		if(!strcmp(strVal,"user"))
		{		
			user_certstart= req_get_cstream_var_in_mime(wp, ("radiusUserCert"), "",&user_cert_len);			
			
			if(tryFormChange==0)
			{
				
				fp=fopen(RS_USER_CERT_ETH,"w");
				if(NULL == fp)
				{
					sprintf(tmpBuf,"Can not open tmp RS cert(%s)!", RS_USER_CERT_5G);
					goto upload_ERR;
				}
				
						 
				fwrite(user_certstart,user_cert_len,0x1,fp);
				
				fclose(fp);
			}
			else
			{
				//To store user cert in tmp file: RS_USER_CERT_TMP
				fp=fopen(RS_USER_CERT_TMP,"w");
				if(NULL == fp)
				{
					sprintf(tmpBuf,"[2] Can not open tmp user cert(%s)!", RS_USER_CERT_TMP);
					goto upload_ERR;
				}
				fwrite(user_certstart,user_cert_len,0x1,fp);	
				fclose(fp);

				// try change user cert form from pfx to pem
				memset(userKeyPass, 0, sizeof(userKeyPass));
				apmib_get( MIB_ELAN_RS_USER_CERT_PASSWD, (void *)userKeyPass);
				
				sprintf(cmd, "openssl pkcs12 -in %s -nodes -out %s -passin pass:%s", RS_USER_CERT_TMP, RS_USER_CERT_ETH, userKeyPass);
				
				system(cmd);
				
				sleep(3); // wait for system(cmd) and avoid to open file failure;
				fp=fopen(RS_USER_CERT_ETH,"r");
				if(NULL == fp)
				{
					sprintf(tmpBuf,"[2] Can not open tmp user cert(%s)!Maybe you should upload your user certificate once again", RS_USER_CERT_ETH);
					goto upload_ERR;
				}
				
				while (fgets(line, sizeof(line), fp))
				{
					if((NULL != strstr(line,RS_CERT_START) ) || (NULL != strstr(line,RS_CERT_END) ))
						certOk=1;					
					//if(NULL != strstr(line,RS_PRIV_KEY_TIP))					
					if((NULL !=strstr(line,RS_RSA_PRIV_KEY_START)) || (NULL !=strstr(line,RS_RSA_PRIV_KEY_END)))
						userKeyOk=1;

					if((certOk == 1) && (userKeyOk == 1))
						break;
				}

				if((certOk != 1) || (userKeyOk != 1))
				{
					
					sprintf(cmd, "rm -rf %s", RS_USER_CERT_ETH);
					
					system(cmd);
					
					sprintf(tmpBuf,"Upload user cert failed. Please make sure: 1) uploaded file in pem or pfx form, 2) uploaded file contain user cert and user key.");
					goto upload_ERR;
				}

				fclose(fp);
			}

			//To store 802.1x user cert
			
			system("rsCert -wrUser_eth");
			
			strcpy(tmpBuf,"802.1x user cerificate and user key upload success!");
		}
		else if(!strcmp(strVal,"root"))
		{		
			ca_certstart= req_get_cstream_var_in_mime(wp, ("radiusRootCert"), "",&ca_cert_len);

			if(tryFormChange == 0)
			{
				fp=fopen(RS_ROOT_CERT_ETH,"w");
				if(NULL == fp)
				{
					sprintf(tmpBuf,"Can not open tmp RS cert(%s)!", RS_ROOT_CERT_ETH);
					goto upload_ERR;
				}
				
				fwrite(ca_certstart,ca_cert_len,0x1,fp);				
				fclose(fp);
			}
			else
			{
				// To store ca cert in tmp file: RS_ROOT_CERT_TMP
				fp=fopen(RS_ROOT_CERT_TMP,"w");
				if(NULL == fp)
				{
					sprintf(tmpBuf,"Can not open tmp RS cert(%s)!", RS_ROOT_CERT_TMP);
					goto upload_ERR;
				}
				fwrite(ca_certstart,ca_cert_len,0x1,fp);				
				fclose(fp);
				
				// try change ca cert form from der to pem
				
				sprintf(cmd, "openssl x509 -inform DER -in %s -outform PEM -out %s",RS_ROOT_CERT_TMP,RS_ROOT_CERT_ETH);
				
				
				system(cmd);
				
				sleep(3);	// wait for system(cmd) and avoid to open file failure;

				
				fp=fopen(RS_ROOT_CERT_ETH,"r");
				if(NULL == fp)
				{
					sprintf(tmpBuf,"[2] Can not open tmp RS cert(%s)!\nMaybe you should upload your root certificate once again!", RS_ROOT_CERT_ETH);
					goto upload_ERR;
				}
			
				while (fgets(line, sizeof(line), fp))
				{
					if((NULL != strstr(line,RS_CERT_START) ) || (NULL != strstr(line,RS_CERT_END) ))
					{
						certOk=1;
						break;
					}
				}

				if(certOk != 1)
				{
					
					sprintf(cmd, "rm -rf %s", RS_ROOT_CERT_ETH);
					
					system(cmd);
					
					strcpy(tmpBuf,"[2] No 802.1x cert inclued in upload file!");
					goto upload_ERR;
				}
				
				fclose(fp);
			}

			//To store 802.1x root cert
			
			system("rsCert -wrRoot_eth");
		
			strcpy(tmpBuf,"802.1x root cerificate upload success!");
		}
		else
		{
			sprintf(tmpBuf,"Upload cert type(%s) is not supported!", strVal);
			goto upload_ERR;
		}
	}
	
	OK_MSG1(tmpBuf, submitUrl);
	return;
	
upload_ERR:
	if(fp != NULL)
		fclose(fp);
	
	ERR_MSG(tmpBuf);
}
#endif

#ifdef CONFIG_RTL_TRANSMISSION
void formTransmissionBT(request *wp, char * path, char * query)
{
	char *downdir;
	char *updir;
	char *strptr;
	char *nextwebpage;
	char tmp[128];
	char cmd[128];
	int enabled;

	nextwebpage=req_get_cstream_var(wp, ("nextwebpage"),"");
	downdir=req_get_cstream_var(wp, ("btdownloaddir"),"");
//	updir=req_get_cstream_var(wp, ("btuploaddir"),"");
	strptr=req_get_cstream_var(wp, ("bt_enabled"),"");

	if(access(downdir, 0) != 0)
	{
		ERR_MSG("Directory Not Exists!!!");
		return;
	}

	if(strptr)
		enabled=atoi(strptr);

//	apmib_set(MIB_BT_UPLOAD_DIR,updir);
	apmib_set(MIB_BT_DOWNLOAD_DIR,downdir);
	apmib_set(MIB_BT_ENABLED,&enabled);
	apmib_update(CURRENT_SETTING);

	if(enabled)
	{
		if(access("/var/run/transmission.pid", 0) == 0)
		{
			ERR_MSG("There is transmission-daemon already running!");
			return;
		}
		sprintf(tmp, "%s/transmission-daemon", downdir);
		if(access(tmp, 0) != 0)
		{
			sprintf(cmd, "mkdir %s/transmission-daemon/", downdir);
			system(cmd);
		}
		sprintf(tmp, "%s/transmission-daemon/torrents", downdir);
		if(access(tmp, 0) != 0)
		{
			sprintf(cmd, "mkdir %s/transmission-daemon/torrents/", downdir);
			system(cmd);
		}
		sprintf(tmp, "%s/transmission-daemon/resume", downdir);
		if(access(tmp, 0) != 0)
		{
			sprintf(cmd, "mkdir %s/transmission-daemon/resume/", downdir);
			system(cmd);
		}
		sprintf(tmp, "%s/transmission-daemon/blocklists", downdir);
		if(access(tmp, 0) != 0)
		{
			sprintf(cmd, "mkdir %s/transmission-daemon/blocklists/", downdir);
			system(cmd);
		}
		sprintf(tmp, "/var/transmission");
		if(access(tmp, 0) != 0)
		{
			system("mkdir /var/transmission/");
		}

		sprintf(cmd, "ln -s %s/transmission-daemon/torrents/ /var/transmission/", downdir);
		system(cmd);
		sprintf(cmd, "ln -s %s/transmission-daemon/resume/ /var/transmission/", downdir);
		system(cmd);
		sprintf(cmd, "ln -s %s/transmission-daemon/blocklists/ /var/transmission/", downdir);
		system(cmd);
		sprintf(cmd, "transmission-daemon --log-error -w %s", downdir);
//		sprintf(cmd, "transmission-daemon --log-debug -w %s -e %s/log", downdir, downdir);
		system(cmd);
	}
	else
	{
		FILE *fp;
		char buf[16];
		int pid;
		
		fp=fopen("/var/run/transmission.pid","r");
		if(NULL == fp)
		{
			ERR_MSG("Can't open /var/run/transmission.pid!");
			return;
		}
		fgets(buf, sizeof(buf), fp);
		sscanf(buf, "%d", &pid);
		fclose(fp);

		sprintf(cmd,"kill -9 %d 2>/dev/null", pid);
		system(cmd);
		system("rm /var/transmission/torrents 2>/dev/null");
		system("rm /var/transmission/resume 2>/dev/null");
		system("rm /var/transmission/blocklists 2>/dev/null");
		system("rm /var/run/transmission.pid 2>/dev/null");
	}

	send_redirect_perm(wp,nextwebpage);
	return;
}
#endif
#ifdef GET_LAN_DEV_INFO_SUPPORT
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
struct arpMsg 
{
	struct ethhdr ethhdr;	 		/* Ethernet header */
	u_short htype;				/* hardware type (must be ARPHRD_ETHER) */
	u_short ptype;				/* protocol type (must be ETH_P_IP) */
	u_char  hlen;				/* hardware address length (must be 6) */
	u_char  plen;				/* protocol address length (must be 4) */
	u_short operation;			/* ARP opcode */
	u_char  sHaddr[6];			/* sender's hardware address */
	u_char  sInaddr[4];			/* sender's IP address */
	u_char  tHaddr[6];			/* target's hardware address */
	u_char  tInaddr[4];			/* target's IP address */
	u_char  pad[18];			/* pad for min. Ethernet payload (60 bytes) */
};
#if 0
int sendArpToCheckDevIsAlive(unsigned int dst_ip, unsigned int src_ip, unsigned char *mac)
{
	int	timeout = 1;
	int 	optval = 1;
	int	s;			/* socket */
	int	rv = 1;			/* return value */
	struct sockaddr addr;		/* for interface name */
	struct arpMsg	arp;
	fd_set		fdset;
	struct timeval	tm;
	time_t		prevTime;


	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		printf("Could not open raw socket");
		return -1;
	}
	
	printf("%s:%d \n",__FUNCTION__,__LINE__);
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		printf("Could not setsocketopt on raw socket");
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.h_dest, MAC_BCAST_ADDR, 6);	/* MAC DA */
	memcpy(arp.ethhdr.h_source, mac, 6);		/* MAC SA */
	arp.ethhdr.h_proto = htons(ETH_P_ARP);		/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp.sInaddr, &src_ip, sizeof(src_ip));		/* source IP address */
	memcpy(arp.sHaddr, mac, 6);			/* source hardware address */
	memcpy(arp.tInaddr, &dst_ip, sizeof(dst_ip));	/* target IP address */
	
	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, "br0");
	if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
		rv = 0;
	
	printf("%s:%d \n",__FUNCTION__,__LINE__);
	/* wait arp reply, and check it */
	tm.tv_usec = 0;
	time(&prevTime);
	while (timeout > 0) {
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = timeout;
		if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) {
			//printf("Error on ARPING request: %s", strerror(errno));
			if (errno != EINTR) rv = 0;
		} else if (FD_ISSET(s, &fdset)) {
			if (recv(s, &arp, sizeof(arp), 0) < 0 ) rv = 0;
			if (arp.operation == htons(ARPOP_REPLY) && 
			    bcmp(arp.tHaddr, mac, 6) == 0 && 
			    *((u_int *) arp.sInaddr) == dst_ip) {
				//memcpy(ret_hwaddr,arp.sHaddr,6);
				//printf("Valid arp reply receved for this address");
				rv = 0;
				break;
			}
		}
		timeout -= time(NULL) - prevTime;
		time(&prevTime);
	}
	printf("%s:%d \n",__FUNCTION__,__LINE__);
	close(s);
	printf("%salid arp replies for this address\n", rv ? "No v" : "V");	 
	return rv;
}
#endif
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
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");

    bzero(l2list, sizeof(l2list));
	l2_tab_num=get_info_from_l2_tab("/proc/rtl865x/l2", l2list);
    
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok, l2_tab_num = %d.\n", l2_tab_num);
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
	
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
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
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
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
	DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
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
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
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
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
//arp table
	arp_entry_num=get_arp_table_list("/proc/net/arp", arp_tab);
	//printf("%s:%d \n",__FUNCTION__,__LINE__);
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
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
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
//dhcp list
	rtk_get_dhcp_client_list(&dhcp_device_num, &dhcp_client_info);
	//printf("%s:%d dhcp_device_num=%d\n",__FUNCTION__,__LINE__,dhcp_device_num);
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
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
	
	DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
	devNum=getDstMacIdx(pdevinfo,mac_null,MAX_STA_NUM);
	for(i=0;i<devNum;i++)
	{
		if(!pdevinfo[i].hostname[0])
			strcpy(pdevinfo[i].hostname,"---");
		ret=rtk_get_device_brand(pdevinfo[i].mac, _PATH_DEVICE_MAC_BRAND, pdevinfo[i].brand);
		if(ret<0)
			strcpy(pdevinfo[i].brand,"---");
	}
    DTRACE(DTRACE_ASP_FMMGMT, "rtk_get_lan_device_info run ok.\n");
	*num=devNum;
	return 0;
	
}
int showClients(request *wp, int argc, char **argv)
{
	RTK_LAN_DEVICE_INFO_T devinfo[MAX_STA_NUM]={0};
	int num=0,nBytesSent=0,i=0;
	char deviceIp[32]={0};
	char deviceMac[32]={0};
	char connectType[32]={0};
	char linkState[16]={0};
	
	rtk_get_lan_device_info(&num,devinfo,MAX_STA_NUM);
	//printf("%s:%d client num=%d\n",__FUNCTION__,__LINE__,num);
	
	nBytesSent += req_format_write(wp,("<tr class=\"tbl_head\">"));
	nBytesSent += req_format_write(wp,("<td align=center><font size=2><b>name</b></td>"));
	nBytesSent += req_format_write(wp,("<td align=center><font size=2><b>ip address</b></td>"));
	nBytesSent += req_format_write(wp, ("\
	<td align=center><font size=2><b>mac address</b></td>\
	<td align=center><font size=2><b>dhcp expires</b></td>\
	<td align=center><font size=2><b>connect type</b></td>\
	<td align=center><font size=2><b>active</b></td>\
	<td align=center><font size=2><b>rx bytes</b></td>\
	<td align=center><font size=2><b>tx bytes</b></td>\
	<td align=center><font size=2><b>brand</b></td></tr>"));
	//	
	for(i=0;i<num;i++)
	{
		strcpy(deviceIp,inet_ntoa((*((struct in_addr *)&(devinfo[i].ip)))));
		sprintf(deviceMac,"%02x:%02x:%02x:%02x:%02x:%02x",devinfo[i].mac[0],devinfo[i].mac[1],devinfo[i].mac[2],devinfo[i].mac[3],devinfo[i].mac[4],devinfo[i].mac[5]);
		
		//printf("%s:%d \n",__FUNCTION__,__LINE__);
		switch(devinfo[i].conType)
		{
			case RTK_LINK_ERROR:
				sprintf(connectType,"-----");
				break;
			case RTK_ETHERNET:
				sprintf(connectType,"ethernet");
				break;
			case RTK_WIRELESS_5G:
				sprintf(connectType,"wifi 5G");
				break;
			case RTK_WIRELESS_2G:
				sprintf(connectType,"wifi 2.4G");
				break;
			default:
				break;
		}
		if(devinfo[i].on_link)
		{
			sprintf(linkState,"on");
		}else
		{
			sprintf(linkState,"off");
		}
		nBytesSent += req_format_write(wp,("<tr class=\"tbl_body\">"));
//printf("%s:%d \n",__FUNCTION__,__LINE__);
//printf((" %s %s %s %d %s %d %d %s\n"),
//	devinfo[i].hostname,deviceIp,deviceMac,devinfo[i].expires,connectType,devinfo[i].rx_bytes,devinfo[i].tx_bytes,devinfo[i].brand);
		//printf("%s:%d i=%d brand=%s rx=%d tx=%d\n",__FUNCTION__,__LINE__,i,devinfo[i].brand,devinfo[i].rx_bytes,devinfo[i].tx_bytes);
		nBytesSent += req_format_write(wp, ("<td>%s</td><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td>%d</td><td>%d</td><td>%s</td>\n"),
			devinfo[i].hostname,deviceIp,deviceMac,devinfo[i].expires,connectType,linkState,devinfo[i].rx_bytes,devinfo[i].tx_bytes,devinfo[i].brand);
	//	printf("%s:%d \n",__FUNCTION__,__LINE__);
		nBytesSent += req_format_write(wp,"</tr>");
	}
	return nBytesSent;
}

#endif
#ifdef HOME_GATEWAY
#ifdef CONFIG_APP_WEAVE
#define WEAVE_REG_TIMEOUT 50
void formWeave(request *wp, char *path, char *query)
{
	char tmpBuf[MAX_MSG_BUFFER_SIZE]={0};
	char cmdBuf[MAX_MSG_BUFFER_SIZE]={0};
	int weave_resgitered=0; 
	char *submitUrl=req_get_cstream_var(wp, ("submit-url"), "");

	char *command=req_get_cstream_var(wp, ("command"), "");
	//printf("%s:%d\n",__FUNCTION__,__LINE__);
	if(strcmp(command,"unregister device")==0)
	{
		weave_resgitered=0;
		apmib_set(MIB_WEAVE_REGISTERED,&weave_resgitered);

        // clear json file
        if(isFileExist(WEAVE_FILE_PATH))
        {
            memset(cmdBuf, 0x0, MAX_MSG_BUFFER_SIZE);
            sprintf(cmdBuf, "echo \"{}\" >%s", WEAVE_FILE_PATH);
            system(cmdBuf);

            memset(cmdBuf, 0x0, MAX_MSG_BUFFER_SIZE);
            sprintf(cmdBuf, "flash set_weaveConf %s", WEAVE_FILE_PATH);
            system(cmdBuf);

            memset(cmdBuf, 0x0, MAX_MSG_BUFFER_SIZE);
            system("flash set wlan1-vxd SSID 1b23456789098765432a1");
            system("flash set REPEATER_SSID2 1b23456789098765432a1");
        }
        system("killall weave_daemon");
        sprintf(tmpBuf, "Unregister succeed!!!");
#ifdef REBOOT_CHECK
        sprintf(lastUrl,"%s","/weave.htm");
        sprintf(okMsg,"%s",tmpBuf);
        int Reboot_Wait = 30;
        countDownTime = Reboot_Wait;
        send_redirect_perm(wp, COUNTDOWN_PAGE);
        run_init_script_flag = 1;
        run_init_script("all");
#else
        OK_MSG_FW(tmpBuf, submitUrl,Reboot_Wait,lan_ip);
#endif
        //system("init.sh gw all");
	}
	else if(strcmp(command,"register device")==0)
	{
		char strWanIP[16];
		char strWanMask[16];
		char strWanDefIP[16];
		char strWanHWAddr[18];
		int timeOut=0;
		
		char *registration_ticket=req_get_cstream_var(wp, ("weaveInfoText"), "");
		
		//printf("%s:%d\n",__FUNCTION__,__LINE__);
		getWanInfo(strWanIP,strWanMask,strWanDefIP,strWanHWAddr);
		if(strcmp(strWanIP,"0.0.0.0")==0)
		{
			sprintf(tmpBuf,"WAN not connect can't connect to cloud! can't register device!");
			goto FMWEAVE_ERROR;
		}
		if(!registration_ticket[0])
		{
			sprintf(tmpBuf,"Not get registration_ticket!\n");
			goto FMWEAVE_ERROR;
		}
		//printf("%s:%d\n",__FUNCTION__,__LINE__);

		sprintf(cmdBuf,"rm %s 2>/dev/null",WEAVE_FILE_PATH);
		system(cmdBuf);
		system("killall weave_daemon 2>/dev/null");
		sprintf(cmdBuf,"weave_daemon --registration_ticket=%s &",registration_ticket);
		system(cmdBuf);
		while(timeOut++<WEAVE_REG_TIMEOUT)
		{
			sleep(1);
			apmib_get(MIB_WEAVE_REGISTERED,&weave_resgitered);
			if(weave_resgitered)
				break;
		}
		if(timeOut>=WEAVE_REG_TIMEOUT)
		{
			FAIL_INFO("Fail! timeout!Please make sure wan connect to google cloud!\n",submitUrl);
			return;
		}
		SUCCESS_INFO("Register device success!\n",submitUrl);
		return;
		//printf("%s:%d\n",__FUNCTION__,__LINE__);
	}
	else if(strcmp(command,"register_success")==0)
	{
		
	}
	else if(strcmp(command,"register_failed")==0)
	{
		
	}
	else
	{
		sprintf(tmpBuf,"error! command is invalid!");
		goto FMWEAVE_ERROR;
	}
	if(submitUrl[0])
		send_redirect_perm(wp,submitUrl);
	else
		printf("error! submitUrl is null");
	//printf("%s:%d\n",__FUNCTION__,__LINE__);

	return;
FMWEAVE_ERROR:
	
	ERR_MSG(tmpBuf);

	return;
}
#endif
#endif


#ifdef CONFIG_APP_BT_REPEATER_CONFIG
void formBTRepeaterSetup(request *wp, char *path, char *query)
{
	char *submitUrl=req_get_cstream_var(wp, ("submit-url"), "");
	char *enabled=req_get_cstream_var(wp, ("enable"), "");
	unsigned int bt_repeater_config_enabled = 0;

	if(!strcmp(enabled, "ON")) {
		bt_repeater_config_enabled = 1;

	} else {	
		bt_repeater_config_enabled = 0;
		system("rm -R /var/local >/dev/null 2>&1");
		system("rm -R /var/dbus-1 >/dev/null 2>&1");
		
		system("killall dbus-daemon >/dev/null 2>&1");
		system("killall bluetoothd >/dev/null 2>&1");
		system("killall hciattach >/dev/null 2>&1");
		system("killall rtk_agent >/dev/null 2>&1");
		system("killall rtk_bt_repeater_config >/dev/null 2>&1");
		system("killall rtk_ble_repeater_config >/dev/null 2>&1");
	}
	
	apmib_set(MIB_BT_CONFIG_ENABLED,&bt_repeater_config_enabled);

	apmib_update_web(CURRENT_SETTING);

#ifndef NO_ACTION
	run_init_script("all");
#endif
	OK_MSG(submitUrl);
	return;
}
#endif


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

    DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
    rtk_get_lan_device_info(&num, devinfo, MAX_STA_NUM);
    DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
    fp = fopen("/tmp/topology_json", "r");
    DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
	if (fp == NULL) 
    {
        struct in_addr	intaddr;
        struct sockaddr hwaddr;
        unsigned char *mac;

        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
		root = cJSON_CreateObject();
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        if(root == NULL)
        {
            printf("[%s:%d] cJSON_CreateObject fail.", __FUNCTION__, __LINE__);
            return -1;
        }
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        if (!apmib_get( MIB_MAP_DEVICE_NAME, (void *)buf)) 
        {
			sprintf(buf, "%s", "null" );
		}
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        cJSON_AddStringToObject(root, "device_name", buf);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        getInAddr("br0", IP_ADDR, (void *)&intaddr );
        sprintf(buf, "%s", inet_ntoa(intaddr) );
        cJSON_AddStringToObject(root, "ip_addr", buf);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        mac = (unsigned char *)hwaddr.sa_data;
		sprintf(buf,"%02x%02x%02x%02x%02x%02x",mac[0], mac[1],mac[2], mac[3], mac[4], mac[5]);
        cJSON_AddStringToObject(root, "mac_address", buf);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        cJSON_AddItemToObject(root, "neighbor_devices", cJSON_CreateArray());
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        cJSON *obj_array;
        cJSON_AddItemToObject(root, "station_info", obj_array = cJSON_CreateArray());
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
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
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        cJSON_AddItemToObject(root, "child_devices", cJSON_CreateArray());
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
       
	}
    else
    {
		ssize_t read;
		size_t  len   = 0;
		char*	line  = NULL;
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
		read = getline(&line, &len, fp);
		fclose(fp);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        if(line == NULL)
        {
            printf("[%s:%d] getline fail.", __FUNCTION__, __LINE__);
            return -1;
        }
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        root = cJSON_Parse(line);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        free(line);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        if(root == NULL)
        {
            printf("[%s:%d] cJSON_Parse fail.", __FUNCTION__, __LINE__);
            return -1;
        }

        addExtStationInfoToTopology(root, devinfo);
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
        cJSON *obj;
        obj = cJSON_GetObjectItem(root, "station_info");
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");
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
        DTRACE(DTRACE_ASP_FMMGMT, "getMeshTopologyJSON run ok.\n");

    }

    return root;
}

int showMeshTopology(request *wp, int argc, char **argv)
{  
    cJSON *root;
    char *out;
    int ret;

    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    root = getMeshTopologyJSON();
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    if(root == NULL)
    {
        printf("[%s:%d] getMeshTopologyJSON fail.", __FUNCTION__, __LINE__);
        return -1;
    }
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    out = cJSON_Print(root);
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
	cJSON_Delete(root);	
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    if(out == NULL)
    {
        printf("[%s:%d] cJSON out fail.", __FUNCTION__, __LINE__);
        return -1;
    }
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    ret = req_format_write(wp, "%s", out);
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    free(out);
    DTRACE(DTRACE_ASP_FMMGMT, "showMeshTopology run ok.\n");
    return ret;
}

void CalcOnlineClientNum(cJSON *obj, int *num)
{
    cJSON *station_array;
    cJSON *head;
    cJSON *pos;

    station_array = cJSON_GetObjectItem(obj, "station_info");
    if(station_array != NULL)
    {
        *num += cJSON_GetArraySize(station_array);
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
            CalcOnlineClientNum(pos, num);
        }
    }
    else
    {
        printf("[%s:%d] can't find child_devices.", __FUNCTION__, __LINE__);
    }

    return;
}
int getTotalOnlineClientNum(request *wp, int argc, char **argv)
{
    int online_num = 0;
    cJSON *root;

    DTRACE(DTRACE_ASP_FMMGMT, "getTotalOnlineClientNum run ok.\n");
    root = getMeshTopologyJSON();
    DTRACE(DTRACE_ASP_FMMGMT, "getTotalOnlineClientNum run ok.\n");
    if(root == NULL)
    {
        printf("[%s:%d] getMeshTopologyJSON fail.", __FUNCTION__, __LINE__);
        return -1;
    }
    DTRACE(DTRACE_ASP_FMMGMT, "getTotalOnlineClientNum run ok.\n");
    CalcOnlineClientNum(root, &online_num);
    DTRACE(DTRACE_ASP_FMMGMT, "getTotalOnlineClientNum run ok.\n");
    cJSON_Delete(root);	
    DTRACE(DTRACE_ASP_FMMGMT, "getTotalOnlineClientNum run ok.\n");
     
    int ret = req_format_write(wp, "%d", online_num);
    return ret;
}

void formReboot(request *wp, char *path, char *query)
{
    char tmp_buf[200];

    sprintf(tmp_buf, "Rebooting !!~~~~Please wait for 40~50secs!");
    ERR_MSG(tmp_buf);
	REBOOT_WAIT_COMMAND(2);		
}

void formLEDControl(request *wp, char *path, char *query)
{
    char *str_val = NULL;
    int val;
    char tmp_buf[200];

    str_val = req_get_cstream_var(wp, "led_enabled", "");
    if (!strcmp(str_val, "0"))
    {
        val = 0;
        if (apmib_set( MIB_LED_ENABLE, (void *)&val) == 0) 
        {
  		    printf("set MIB_LED_ENABLE err.\n");
	    }
    }
	else
	{
        val = 1;
        if (apmib_set( MIB_LED_ENABLE, (void *)&val) == 0) 
        {
  		    printf("set MIB_LED_ENABLE err.\n");
	    } 
	}

    str_val = req_get_cstream_var(wp, ("submit-url"), "");
    if(str_val[0])
    {
	    send_redirect_perm(wp, str_val);	
    }
    apmib_update_web(CURRENT_SETTING);
    
#ifdef CSRF_SECURITY_PATCH
    log_boaform("fromLEDControl", wp);  //To set fromLEDControl valid at security_tbl
#endif

}

void formAddMeshNode(request *wp, char *path, char *query)
{
    char *str_val = NULL;
    char tmp_buf[200];

    str_val = req_get_cstream_var(wp, "sn", "");
    if(str_val[0])
    {
        if(strlen(str_val) != 21)
        {
            sprintf(tmp_buf, "sn len err!");
            goto setErr;
        }
    }
    else
    {
        sprintf(tmp_buf, "no sn find!");
        goto setErr;
    }

    str_val = req_get_cstream_var(wp, ("submit-url"), "");
    if(str_val[0])
    {
	    send_redirect_perm(wp, str_val);	
    }
#ifdef CSRF_SECURITY_PATCH
    log_boaform("formAddMeshNode", wp);  //To set formAddMeshNode valid at security_tbl
#endif
    return;
    
setErr:
	ERR_MSG(tmp_buf);
}

void fromTimerReboot(request *wp, char *path, char *query)
{
    char *str_val = NULL;
    int val;
    char tmp_buf[200];    

    str_val = req_get_cstream_var(wp, "timer_reboot_enabled", "");
    if (!strcmp(str_val, "1"))
    {
        sprintf(tmp_buf, "on");
        if (apmib_set( MIB_DEV_RESTART_ENABLE, (void *)tmp_buf) == 0) 
        {
  		    printf("set MIB_DEV_RESTART_ENABLE err.\n");
	    } 
    }
	else
	{
        sprintf(tmp_buf, "off");
        if (apmib_set( MIB_DEV_RESTART_ENABLE, (void *)tmp_buf) == 0) 
        {
  		    printf("set MIB_DEV_RESTART_ENABLE err.\n");
	    } 
	}
    str_val = req_get_cstream_var(wp, "timer_reboot_time", "");
    if(str_val[0])
    {
        if (apmib_set( MIB_DEV_RESTART_TIME, (void *)str_val) == 0) 
        {
  		    printf("set MIB_DEV_RESTART_TIME err.\n");
	    } 
    }
    apmib_update_web(CURRENT_SETTING);
    
    str_val = req_get_cstream_var(wp, ("submit-url"), "");
    if(str_val[0])
    {
	    send_redirect_perm(wp, str_val);	
    }

#ifdef CSRF_SECURITY_PATCH
    log_boaform("fromTimerReboot", wp);  //To set fromTimerReboot valid at security_tbl
#endif

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

void formNewWizard(request *wp, char *path, char *query)
{
    int dns_changed=0;
    char tmp_buf[100];
    char *submit_url;
    char *str_password;
    int mib_val;
    
    if(ntpHandler(wp, tmp_buf, 1) < 0)
		goto SET_ERR;
	set_timeZone();
		

	if(tcpipWanHandler(wp, tmp_buf, &dns_changed) < 0)
    {
		submit_url = req_get_cstream_var(wp, ("submit-url-wan"), "");   // hidden page
		goto SET_ERR;	
	}

    str_password = req_get_cstream_var(wp, "newPass", "");

	
	if (!str_password[0] ) 
    {
		strcpy(tmp_buf, ("ERROR: Password cannot be empty."));
		goto SET_ERR;
	}

    if(strlen(str_password) >= MAX_NAME_LEN)
    {
        strcpy(tmp_buf, ("ERROR: Password over 30 characters."));
		goto SET_ERR;
    }
    
	if ( !apmib_set(MIB_USER_PASSWORD, (void *)str_password) ) 
    {
		strcpy(tmp_buf, ("ERROR: Set user password to MIB database failed."));
		goto SET_ERR;
	}

    mib_val = 1;
    if (!apmib_set(MIB_FIRST_LOGIN, (void *)&mib_val))
	{
		DPrintf("set MIB_FIRST_LOGIN err.\n");
        goto SET_ERR;
	}

    if(!SetToController())
    {
        DPrintf("SetToController err.\n");
        goto SET_ERR;
    }
    
    apmib_update_web(CURRENT_SETTING);
    
    add_user(wp);
    
#ifdef REBOOT_CHECK
        run_init_script_flag = 1;
#endif
#ifndef NO_ACTION
        run_init_script("all");
#endif
//        submit_url = req_get_cstream_var(wp, ("next_url"), "");
//#ifdef MULTI_WAN_SUPPORT
//#ifdef APPLY_CHANGE_DIRECT_SUPPORT	
//        REBOOT_NOWAIT(WEB_PAGE_HOME);
//#else	
//        OK_MSG(WEB_PAGE_HOME);
//#endif
//#else
//        REBOOT_WAIT(WEB_PAGE_HOME);
//#endif

        send_redirect_perm(wp, WEB_PAGE_HOME);
        return ;
SET_ERR:
        
        OK_MSG1(tmp_buf,WEB_PAGE_WIZARD);
        return ;
}



