#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if.h>
#include "prmt_mngmtserver.h"
#include "prmt_utility.h"

#define CWMPPID  "/var/run/cwmp.pid"

/*star:20091228 START add for store parameterkey*/
//char gParameterKey[32+1];
/*star:20091228 END*/
char gConnectionRequestURL[256+1];

#ifdef MULTI_WAN_SUPPORT
static int getTR069WanIpv4Addr(unsigned char *addr_str, int size)
{
	if(addr_str==NULL || size<16)
		return -1;
	
	//get wan number of tr069
	int wan_index=0, found_tr069if=0, ret=0;
	WANIFACE_T WanIfaceEntry;
	struct in_addr tr069_wan_addr;
	memset(addr_str, 0x0, size);
	
	for(wan_index = 1; wan_index <= WANIFACE_NUM; ++wan_index)
	{
	    memset(&WanIfaceEntry,0,sizeof(WANIFACE_T));
	    getWanIfaceEntry(wan_index,&WanIfaceEntry);
	    if( WanIfaceEntry.enable!=0  && (WanIfaceEntry.applicationtype==APPTYPE_TR069 || WanIfaceEntry.applicationtype==APPTYPE_TR069_INTERNET))
	    {
	        found_tr069if=1;
	        break;
	    }
	}
	
	//get ipv4 addr of wan
	if(found_tr069if)
	{
		tr069_wan_addr.s_addr = 0;
		//get wan ip address
	    if(0==getInAddr(tr069WanIfName, IP_ADDR, &tr069_wan_addr))
	    {
	        CWMPDBG_FUNC(MODULE_CORE|MODULE_DATA_MODEL, LEVEL_ERROR, ("[%s:%d]FAILED in getInAddr!\n", __FUNCTION__, __LINE__));
	        return -1;
	    }
		snprintf(addr_str, size, "%s", inet_ntoa(tr069_wan_addr));
		return 0;
	}else{
		return -1;
	}
	
}
#else
//ifndef MULTI_WAN_SUPPORT
static int getTR069WanIpv4Addr(unsigned char *addr_str, int size)
{
	struct ifreq ifr;
	struct sockaddr_in *soaddr;
	struct in_addr addr;
	unsigned char ifname[16] = {0};
	int skfd=0, ret=-1;
	unsigned char path[CONN_REQ_PATH_LEN]={0};
	int port=0, wan_type=0;

	apmib_get(MIB_WAN_DHCP, (void *)&wan_type);
	switch(wan_type)
	{
	case PPPOE: //PPPoE
		sprintf(ifname, "ppp0");
		break;
	case DHCP_DISABLED: //static IP
	case DHCP_CLIENT: //dhcp client
	default:
		sprintf(ifname, "eth1");
		break;
	}
	//printf("ifname:%s\n", ifname);

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(skfd)
	{
		strcpy(ifr.ifr_name, ifname);
		if (ioctl(skfd, SIOCGIFADDR, &ifr) >= 0) 
		{
			soaddr = ((struct sockaddr_in *)&ifr.ifr_addr);
			addr = soaddr->sin_addr;
			snprintf(addr_str, size, "%s", inet_ntoa(addr));
			ret=0;
		}
		close(skfd);
	}
	
	return ret;
}
#endif
#ifdef TR069_ANNEX_G
int gDeviceNumber=0;
extern int gRestartStun;
extern int stunState;
extern int OldStunState;
extern pthread_cond_t cond;
extern pthread_mutex_t mutex;

/****ManageDevEntity*******************************************************************************************/
struct CWMP_OP tManageDevEntityLeafOP = { getManageDevEntity, NULL };
struct CWMP_PRMT tManageDevEntityLeafInfo[] =
{
/*(name,		type,		flag,		op)*/
{"ManufacturerOUI",	eCWMP_tSTRING,	CWMP_READ,	&tManageDevEntityLeafOP},
{"SerialNumber",	eCWMP_tSTRING,	CWMP_READ,	&tManageDevEntityLeafOP},
{"ProductClass",	eCWMP_tSTRING,	CWMP_READ,	&tManageDevEntityLeafOP},
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
#ifdef _PRMT_WT107_
{"Host",	eCWMP_tSTRING,	CWMP_READ,	&tManageDevEntityLeafOP},
#endif
/*ping_zhang:20081217 END*/
};
enum eManageDevEntityLeaf
{
	eMDManufacturerOUI,
	eMDSerialNumber,
	eMDProductClass,
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
#ifdef _PRMT_WT107_
	eMDHost
#endif
/*ping_zhang:20081217 END*/
};
struct CWMP_LEAF tManageDevEntityLeaf[] =
{
{ &tManageDevEntityLeafInfo[eMDManufacturerOUI] },
{ &tManageDevEntityLeafInfo[eMDSerialNumber] },
{ &tManageDevEntityLeafInfo[eMDProductClass] },
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
#ifdef _PRMT_WT107_
{ &tManageDevEntityLeafInfo[eMDHost] },
#endif
/*ping_zhang:20081217 END*/
{ NULL }
};

/****ManageableDevice*******************************************************************************************/
struct CWMP_PRMT tManageableDeviceOjbectInfo[] =
{
/*(name,	type,		flag,			op)*/
{"0",		eCWMP_tOBJECT,	CWMP_READ|CWMP_LNKLIST,	NULL}
};
enum eManageableDeviceOjbect
{
	eMD0
};
struct CWMP_LINKNODE tManageableDeviceObject[] =
{
/*info,  				leaf,			next,		sibling,		instnum)*/
{&tManageableDeviceOjbectInfo[eMD0],tManageDevEntityLeaf,	NULL,		NULL,			0},
};
/***********************************************************************************************/
#endif

/*******ManagementServer****************************************************************************************/
struct CWMP_OP tManagementServerLeafOP = { getMngmntServer,setMngmntServer };
struct CWMP_PRMT tManagementServerLeafInfo[] =
{
/*(name,				type,		flag,			op)*/
{"EnableCWMP",			eCWMP_tBOOLEAN,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"URL",					eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"Username",				eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"Password",				eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"PeriodicInformEnable",		eCWMP_tBOOLEAN,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"PeriodicInformInterval",		eCWMP_tUINT,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"PeriodicInformTime",			eCWMP_tDATETIME,CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"ParameterKey",			eCWMP_tSTRING,	CWMP_READ|CWMP_DENY_ACT,&tManagementServerLeafOP},
{"ConnectionRequestURL",		eCWMP_tSTRING,	CWMP_READ,		&tManagementServerLeafOP},
{"ConnectionRequestUsername",		eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"ConnectionRequestPassword",		eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"UpgradesManaged",			eCWMP_tBOOLEAN,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"KickURL",				eCWMP_tSTRING,	CWMP_READ,		&tManagementServerLeafOP},
{"DownloadProgressURL",			eCWMP_tSTRING,	CWMP_READ,		&tManagementServerLeafOP},
{"DefaultActiveNotificationThrottle",	eCWMP_tUINT,	CWMP_READ|CWMP_WRITE,		&tManagementServerLeafOP},
#ifdef TR069_ANNEX_G
{"UDPConnectionRequestAddress",       eCWMP_tSTRING,	CWMP_READ,	&tManagementServerLeafOP},
{"STUNEnable",                        eCWMP_tBOOLEAN,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"STUNServerAddress",                 eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"STUNServerPort",                    eCWMP_tUINT,		CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"STUNUsername",                      eCWMP_tSTRING,	CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"STUNPassword",                      eCWMP_tSTRING,	CWMP_PASSWORD|CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"STUNMaximumKeepAlivePeriod",        eCWMP_tINT,		CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"STUNMinimumKeepAlivePeriod",        eCWMP_tUINT,		CWMP_WRITE|CWMP_READ,	&tManagementServerLeafOP},
{"NATDetected",                       eCWMP_tBOOLEAN,	CWMP_READ,	&tManagementServerLeafOP},
{"ManageableDeviceNumberOfEntries",	eCWMP_tUINT,	CWMP_READ,		&tManagementServerLeafOP},
{"ManageableDeviceNotificationLimit",	eCWMP_tUINT,	CWMP_READ|CWMP_WRITE,		&tManagementServerLeafOP},
#endif
};
enum eManagementServerLeaf
{
	eMSEnableCWMP,
	eMSURL,
	eMSUsername,
	eMSPassword,
	eMSPeriodicInformEnable,
	eMSPeriodicInformInterval,
	eMSPeriodicInformTime,
	eMSParameterKey,
	eMSConnectionRequestURL,
	eMSConnectionRequestUsername,
	eMSConnectionRequestPassword,
	eMSUpgradesManaged,
	eMSKickURL,
	eMSDownloadProgressURL,
	eMSDefaultActiveNotificationThrottle,
#ifdef TR069_ANNEX_G
	eMSUDPConnectionRequestAddress,
	eMSSTUNEnable,
	eMSSTUNServerAddress,
	eMSSTUNServerPort,
	eMSSTUNUsername,
	eMSSTUNPassword,
	eMSSTUNMaximumKeepAlivePeriod,
	eMSSTUNMinimumKeepAlivePeriod,
	eMSNATDetected,
	eMSManageableDeviceNumberOfEntries,
	eMSManageableDeviceNotificationLimit,
#endif
};
struct CWMP_LEAF tManagementServerLeaf[] =
{
{ &tManagementServerLeafInfo[eMSEnableCWMP] },
{ &tManagementServerLeafInfo[eMSURL] },
{ &tManagementServerLeafInfo[eMSUsername] },
{ &tManagementServerLeafInfo[eMSPassword] },
{ &tManagementServerLeafInfo[eMSPeriodicInformEnable] },
{ &tManagementServerLeafInfo[eMSPeriodicInformInterval] },
{ &tManagementServerLeafInfo[eMSPeriodicInformTime] },
{ &tManagementServerLeafInfo[eMSParameterKey] },
{ &tManagementServerLeafInfo[eMSConnectionRequestURL] },
{ &tManagementServerLeafInfo[eMSConnectionRequestUsername] },
{ &tManagementServerLeafInfo[eMSConnectionRequestPassword] },
{ &tManagementServerLeafInfo[eMSUpgradesManaged] },
{ &tManagementServerLeafInfo[eMSKickURL] },
{ &tManagementServerLeafInfo[eMSDownloadProgressURL] },
{ &tManagementServerLeafInfo[eMSDefaultActiveNotificationThrottle] },
#ifdef TR069_ANNEX_G
{ &tManagementServerLeafInfo[eMSUDPConnectionRequestAddress] },
{ &tManagementServerLeafInfo[eMSSTUNEnable] },
{ &tManagementServerLeafInfo[eMSSTUNServerAddress] },
{ &tManagementServerLeafInfo[eMSSTUNServerPort] },
{ &tManagementServerLeafInfo[eMSSTUNUsername] },
{ &tManagementServerLeafInfo[eMSSTUNPassword] },
{ &tManagementServerLeafInfo[eMSSTUNMaximumKeepAlivePeriod] },
{ &tManagementServerLeafInfo[eMSSTUNMinimumKeepAlivePeriod] },
{ &tManagementServerLeafInfo[eMSNATDetected] },
{ &tManagementServerLeafInfo[eMSManageableDeviceNumberOfEntries] },
{ &tManagementServerLeafInfo[eMSManageableDeviceNotificationLimit] },
#endif
{ NULL	}
};
#ifdef TR069_ANNEX_G
struct CWMP_OP tMS_ManageableDevice_OP = { NULL, objManageDevice };
struct CWMP_PRMT tManagementServerObjectInfo[] =
{
/*(name,				type,		flag,			op)*/
{"ManageableDevice",			eCWMP_tOBJECT,	CWMP_READ,		&tMS_ManageableDevice_OP}
};
enum eManagementServerObject
{
	eMSManageableDevice
};
struct CWMP_NODE tManagementServerObject[] =
{
/*info,  						leaf,		next)*/
{&tManagementServerObjectInfo[eMSManageableDevice],	NULL,		NULL},
{NULL,							NULL,		NULL}
};
#endif

enum {
	EN_ENABLE_CWMP = 0,
	EN_URL,
	EN_USERNAME, 
	EN_PASSWORD,
	EN_PERIODIC_INFORM_ENABLE,
	EN_PERIODIC_INTERVAL,
	EN_PERIODIC_TIME,
	EN_PARAMETER_KEY,
	EN_CONNREQ_URL,
	EN_CONNREQ_USERNAME,
	EN_CONNREQ_PASSWORD,
	EN_UPGRADE_MANAGED,
	EN_KICKURL,
	EN_DOWNLOADURL,
	EN_DEFAULT_ACTIVE_NORTIFICATION_THROTTLE,
#ifdef TR069_ANNEX_G
	eMS_UDPConnectionRequestAddress,
	eMS_STUNEnable,
	eMS_STUNServerAddress,
	eMS_STUNServerPort,
	eMS_STUNUsername,
	eMS_STUNPassword,
	eMS_STUNMaximumKeepAlivePeriod,
	eMS_STUNMinimumKeepAlivePeriod,
	eMS_NATDetected,
	EN_MANAGEABLEDEVICENUMBER,
	EN_MANAGEABLE_DEVICE_NORTIFICATION_LIMIT,
#endif
};

void MgmtSrvSetParamKey(const char *key) {
/*star:20091228 START add for store parameterkey*/
#if 1
	unsigned char gParameterKey[32+1];
	gParameterKey[0]='\0';
	if (key)
	{
		strncpy(gParameterKey, key, sizeof(gParameterKey) -1);
		gParameterKey[sizeof(gParameterKey) -1]=0;
	}
	mib_set(MIB_CWMP_PARAMETERKEY,gParameterKey);
#else
	gParameterKey[0]='\0';
	if (key)
	{
		strncpy(gParameterKey, key, sizeof(gParameterKey) -1);
		gParameterKey[sizeof(gParameterKey) -1]=0;
	}
#endif
/*star:20091228 END*/
}

int getMngmntServer(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	//char	*lastname = entity->name;
	unsigned char buf[256+1]={0};
	unsigned char ch=0;
	unsigned int  in=0;
	int           in1=0;
	
	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;

	*type = entity->info->type;
	*data = NULL;
	switch(getIndexOf(tManagementServerLeaf, entity->info->name)) {
	case EN_ENABLE_CWMP:
		CONFIG_GET(MIB_CWMP_ENABLED, &in);
		*data = booldup(in);
		break;
	case EN_URL: //URL
		CONFIG_GET(MIB_CWMP_ACS_URL, buf);
		*data = strdup( buf );
		break;
	case EN_USERNAME: //Username
//	#if DEBUG
		CONFIG_GET(MIB_CWMP_ACS_USERNAME, buf);
		*data = strdup(buf);
//	#else
//		*data = strdup("");
//	#endif
		break;
	case EN_PASSWORD: // Password
	#if DEBUG
		CONFIG_GET(MIB_CWMP_ACS_PASSWORD, buf);
		*data = strdup(buf);
	#else
		*data = strdup("");
	#endif
		break;
	case EN_PERIODIC_INFORM_ENABLE: // #PeriodicInformEnable
		CONFIG_GET(MIB_CWMP_INFORM_ENABLE, &in);
		*data = booldup(in);
		break;	
	case EN_PERIODIC_INTERVAL: // PeriodicInformInterval
		CONFIG_GET(MIB_CWMP_INFORM_INTERVAL, &in);
		*data = uintdup(in);
		break;	
	case EN_PERIODIC_TIME: // PeriodicInformTime
		CONFIG_GET(MIB_CWMP_INFORM_TIME, &in);
		*data = timedup(in);
		break;	
	case EN_PARAMETER_KEY: // ParameterKey
		{
/*star:20091228 START add for store parameterkey*/
			unsigned char gParameterKey[32+1];
			CONFIG_GET(MIB_CWMP_PARAMETERKEY,gParameterKey);
/*star:20091228 END*/
		*data = strdup(gParameterKey);
		break;	
		}
	case EN_CONNREQ_URL: // ConnectionRequestURL
        {
            unsigned char ipAddr[16]={0}, path[CONN_REQ_PATH_LEN]={0};
            int port=0;
            if (0==getTR069WanIpv4Addr(ipAddr, sizeof(ipAddr))){
                apmib_get(MIB_CWMP_CONREQ_PORT, (void *)&port);
                apmib_get(MIB_CWMP_CONREQ_PATH, (void *)path);
                sprintf(buf, "http://%s:%d%s", ipAddr, port, path);
                *data = strdup(buf);
            }else
                *data = strdup("");
        }
		break;
	case EN_CONNREQ_USERNAME: // ConnectionRequestUsername
		CONFIG_GET(MIB_CWMP_CONREQ_USERNAME, buf);
		*data = strdup(buf);
		break;	
	case EN_CONNREQ_PASSWORD: // ConnectionRequestPassword
	#if DEBUG
		CONFIG_GET(MIB_CWMP_CONREQ_PASSWORD, buf);
		*data = strdup(buf);
	#else
		*data = strdup("");
	#endif
		break;	
	case EN_UPGRADE_MANAGED: // UpgradesManaged
		CONFIG_GET(MIB_CWMP_ACS_UPGRADESMANAGED, &ch);
		*data = booldup(ch);
		break;
	case EN_KICKURL:
		CONFIG_GET(MIB_CWMP_ACS_KICKURL, buf);
		*data = strdup(buf);
		break;	
	case EN_DOWNLOADURL:
		CONFIG_GET(MIB_CWMP_ACS_DOWNLOADURL, buf);
		*data = strdup(buf);
		break;		
	case EN_DEFAULT_ACTIVE_NORTIFICATION_THROTTLE:
		CONFIG_GET(MIB_CWMP_DEF_ACT_NOTIF_THROTTLE, &in);
		*data = uintdup(in);
		break;
#ifdef TR069_ANNEX_G
	case eMS_UDPConnectionRequestAddress:
		CONFIG_GET(MIB_CWMP_UDP_CONN_REQ_ADDR, buf);
		*data = strdup(buf);
		break;
		
	case eMS_STUNEnable:
		CONFIG_GET(MIB_CWMP_STUN_EN, &in);
		*data = booldup(in);
		break;
		
	case eMS_STUNServerAddress:
		CONFIG_GET(MIB_CWMP_STUN_SERVER_ADDR, buf);
		*data = strdup(buf);
		break;
		
	case eMS_STUNServerPort:
		CONFIG_GET(MIB_CWMP_STUN_SERVER_PORT, &in);
		*data = uintdup(in);
		break;
		
	case eMS_STUNUsername:
		CONFIG_GET(MIB_CWMP_STUN_USERNAME, buf);
		*data = strdup(buf);
		break;
		
	case eMS_STUNPassword:
		CONFIG_GET(MIB_CWMP_STUN_PASSWORD, buf);
		*data = strdup(buf);
		break;
		
	case eMS_STUNMaximumKeepAlivePeriod:
		CONFIG_GET(MIB_CWMP_STUN_MAX_KEEP_ALIVE_PERIOD, &in1);
		*data = intdup(in1);
		break;
		
	case eMS_STUNMinimumKeepAlivePeriod:
		CONFIG_GET(MIB_CWMP_STUN_MIN_KEEP_ALIVE_PERIOD, &in);
		*data = uintdup(in);
		break;
		
	case eMS_NATDetected:
		CONFIG_GET(MIB_CWMP_NAT_DETECTED, &in);
		*data = uintdup(in);
		break;

	case EN_MANAGEABLEDEVICENUMBER:
	{
/*star:20100127 START add to update gDeviceNumber, used for notification list check*/
		FILE *fp;
		int count=0;
		fp=fopen( TR111_DEVICEFILE, "r" );
		
		while( fp && fgets( buf,160,fp ) )
		{
			char *p;
			
			p = strtok( buf, " \n\r" );
			if( p && atoi(p)>0 )
			{
				count++;
			}
		}
		if(fp) fclose(fp);
		gDeviceNumber = count;
/*star:20100127 END*/		
		*data = uintdup(gDeviceNumber);
	}
		break;
	case EN_MANAGEABLE_DEVICE_NORTIFICATION_LIMIT:
		CONFIG_GET(MIB_CWMP_MANAGE_DEV_NOTIF_LIMIT, &in);
		*data = uintdup(in);
		break;
#endif
	default:
		return ERR_9005;
				
	}

	return 0;

}

extern void parse_xml_escaped_str(char *out, char *in);
int setMngmntServer(char *name, struct CWMP_LEAF *entity, int type, void *data)
{
	char	*lastname = entity->info->name;
	char 	*buf=data;
	char 	tmp[256+1]={0};
	int  	len=0;	
	unsigned int *pNum;
	unsigned char byte;
	unsigned int iVal;
	unsigned int flag;
	int          *pInt;
#ifdef MULTI_WAN_SUPPORT
    /* 
     * If MIB_NO_REINIT_FLAG is set, the corresponding mib set won't go into reinit flow
     * For URL, Username, Password, periodic inform enable, periodic inform interval, connection request username, connection request password:
     *      a. apmib_set() with MIB_NO_REINIT_FLAG flag.
     *      b. setValue() returns 0;
     *      c. update all of these arguments in .
     */
#else
    /*
     * setValue() returns 1 to make DUT reboot to take effect.
     */
#endif
	
	if( (name==NULL) || (entity==NULL)) return -1;

	if( entity->info->type!=type ) return ERR_9006;

	CWMPDBG_FUNC(MODULE_DATA_MODEL, LEVEL_DETAILED, ("name is <%s>\n", lastname));

	switch(getIndexOf(tManagementServerLeaf, entity->info->name)) {
	case EN_ENABLE_CWMP:
		pNum = (unsigned int *)data;
		CHECK_PARAM_NUM(*pNum, 0, 1);
		iVal = (*pNum == 0) ? 0 : 1;
		CONFIG_SET(MIB_CWMP_ENABLED, &iVal);	

		if (iVal == 0)
		{
			//int cwmppid = 0;
			
			CONFIG_GET(MIB_CWMP_FLAG, &flag);
			flag = flag & (~CWMP_FLAG_AUTORUN);
			CONFIG_SET(MIB_CWMP_FLAG, &flag);	

			//cwmppid = getPid((char*)CWMPPID);
			//printf("\ncwmppid=%d\n",cwmppid);
			//if(cwmppid > 0)
			//	kill(cwmppid, 15);
			return 1;
		}
		break;
	case EN_URL: //URL
		CHECK_PARAM_STR(buf, 0, 256+1);
#if defined(CTC_WAN_NAME)&&defined(CONFIG_BOA_WEB_E8B_CH)
		char tmpstr[256+1];
		CONFIG_GET(MIB_CWMP_ACS_URL, tmpstr);
		CONFIG_SET(MIB_CWMP_ACS_URL_OLD,tmpstr);
#endif
		char tmpstr[256+1] = {0};
        CONFIG_GET(MIB_CWMP_ACS_URL, tmpstr);
        CONFIG_SET(MIB_CWMP_ACS_URL_OLD, tmpstr);
		CONFIG_SET(MIB_CWMP_ACS_URL, buf);
		cwmpSettingChange(MIB_CWMP_ACS_URL);
		break;
	case EN_USERNAME: //Username
		CHECK_PARAM_STR(buf, 0, 256+1);
		parse_xml_escaped_str(tmp, buf);
#ifdef MULTI_WAN_SUPPORT
        CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_ACS_USERNAME, tmp);
#else
		CONFIG_SET(MIB_CWMP_ACS_USERNAME, tmp);
#endif
		cwmpSettingChange(MIB_CWMP_ACS_USERNAME);
		break;
	case EN_PASSWORD: // Password
		CHECK_PARAM_STR(buf, 0, 256+1);
		parse_xml_escaped_str(tmp, buf);
#ifdef MULTI_WAN_SUPPORT
		CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_ACS_PASSWORD, tmp);
#else
		CONFIG_SET(MIB_CWMP_ACS_PASSWORD, tmp);
#endif
		cwmpSettingChange(MIB_CWMP_ACS_PASSWORD);
		break;
	case EN_PERIODIC_INFORM_ENABLE: // #PeriodicInformEnable
		pNum = (unsigned int *)data;
		CHECK_PARAM_NUM(*pNum, 0, 1);
		iVal = (*pNum == 0) ? 0 : 1;
#ifdef MULTI_WAN_SUPPORT
		CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_INFORM_ENABLE, &iVal);	
#else
		CONFIG_SET(MIB_CWMP_INFORM_ENABLE, &iVal);	
#endif
		cwmpSettingChange(MIB_CWMP_INFORM_ENABLE);
		break;	
	case EN_PERIODIC_INTERVAL: // PeriodicInformInterval
		pNum = (unsigned int *)data;	
		if (*pNum < 1) return ERR_9007;		
#ifdef MULTI_WAN_SUPPORT
		CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_INFORM_INTERVAL, pNum);
#else
		CONFIG_SET(MIB_CWMP_INFORM_INTERVAL, pNum);
#endif
		cwmpSettingChange(MIB_CWMP_INFORM_INTERVAL);
		break;	
	case EN_PERIODIC_TIME: // PeriodicInformTime
		pNum = (unsigned int *)buf;
		printf("time is %u\n", *pNum);
#ifdef MULTI_WAN_SUPPORT
		CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_INFORM_TIME, buf);
#else
		CONFIG_SET(MIB_CWMP_INFORM_TIME, buf);
#endif
		cwmpSettingChange(MIB_CWMP_INFORM_TIME);
		break;	
	case EN_CONNREQ_USERNAME: // ConnectionRequestUsername
		CHECK_PARAM_STR(buf, 0, 256+1);
		parse_xml_escaped_str(tmp, buf);
#ifdef MULTI_WAN_SUPPORT
		CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_CONREQ_USERNAME, tmp);
#else
		CONFIG_SET(MIB_CWMP_CONREQ_USERNAME, tmp);
#endif
		cwmpSettingChange(MIB_CWMP_CONREQ_USERNAME);
		break;
	case EN_CONNREQ_PASSWORD: // ConnectionRequestPassword
		CHECK_PARAM_STR(buf, 0, 256+1);
		parse_xml_escaped_str(tmp, buf);
#ifdef MULTI_WAN_SUPPORT
		CONFIG_SET(MIB_NO_REINIT_FLAG | MIB_CWMP_CONREQ_PASSWORD, tmp);
#else
		CONFIG_SET(MIB_CWMP_CONREQ_PASSWORD, tmp);
#endif
		cwmpSettingChange(MIB_CWMP_CONREQ_PASSWORD);
		break;
	case EN_UPGRADE_MANAGED: // UpgradesManaged
		pNum = (unsigned int *)data;
		CHECK_PARAM_NUM(*pNum, 0, 1);
		byte = (*pNum == 0) ? 0 : 1;
		CONFIG_SET(MIB_CWMP_ACS_UPGRADESMANAGED, &byte);	
		break;
	case EN_DEFAULT_ACTIVE_NORTIFICATION_THROTTLE:
		pNum = (unsigned int *)data;
		iVal = (*pNum );
		CONFIG_SET(MIB_CWMP_DEF_ACT_NOTIF_THROTTLE, &iVal);
		break;
#ifdef TR069_ANNEX_G
	case eMS_STUNEnable:
		pNum = (unsigned int *)data;
		CHECK_PARAM_NUM(*pNum, 0, 1);
		iVal = (*pNum == 0) ? 0 : 1;
		CONFIG_SET(MIB_CWMP_STUN_EN, &iVal);	
		gRestartStun = 1;
		break;
		
	case eMS_STUNServerAddress:
		if(stunState != STUN_BINDING_CHANGE)
		{
			CHECK_PARAM_STR(buf, 0, CWMP_STUN_SERVER_ADDR_LEN+1);
			CONFIG_SET(MIB_CWMP_STUN_SERVER_ADDR, buf);
			gRestartStun = 1;
		}
		break;
		
	case eMS_STUNServerPort:
		if(stunState != STUN_BINDING_CHANGE)
		{
			pNum = (unsigned int *)data;
			if ((*pNum < 0) || (*pNum > 65535)) return ERR_9007;		
			CONFIG_SET(MIB_CWMP_STUN_SERVER_PORT, pNum);
			gRestartStun = 1;
		}
		break;
		
	case eMS_STUNUsername:
		CHECK_PARAM_STR(buf, 0, CWMP_STUN_USERNAME_LEN+1);
		CONFIG_SET(MIB_CWMP_STUN_USERNAME, buf);
		pthread_mutex_lock(&mutex);
		stunState = STUN_CHANGE_OTHER_PARAM;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		break;
		
	case eMS_STUNPassword:
		CHECK_PARAM_STR(buf, 0, CWMP_STUN_PASSWORD_LEN+1);
		CONFIG_SET(MIB_CWMP_STUN_PASSWORD, buf);
		pthread_mutex_lock(&mutex);
		stunState = STUN_CHANGE_OTHER_PARAM;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		break;
		
	case eMS_STUNMaximumKeepAlivePeriod:
		pInt = (int *)data;
		if (*pInt < -1) return ERR_9007;	
		CONFIG_SET(MIB_CWMP_STUN_MAX_KEEP_ALIVE_PERIOD, pInt);
		pthread_mutex_lock(&mutex);
		stunState = STUN_CHANGE_OTHER_PARAM;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		break;
		
	case eMS_STUNMinimumKeepAlivePeriod:
		pNum = (unsigned int *)data;
		CONFIG_SET(MIB_CWMP_STUN_MIN_KEEP_ALIVE_PERIOD, pNum);
		pthread_mutex_lock(&mutex);
		stunState = STUN_CHANGE_OTHER_PARAM;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		break;

	case EN_MANAGEABLE_DEVICE_NORTIFICATION_LIMIT:
		pNum = (unsigned int *)data;
		iVal = (*pNum );
		CONFIG_SET(MIB_CWMP_MANAGE_DEV_NOTIF_LIMIT, &iVal);
		break;
#endif
	default:
		return ERR_9005;
				
	}

	return 0;
}


#ifdef TR069_ANNEX_G
extern unsigned int getInstNum( char *name, char *objname );
int getManageDeviceInfo( unsigned int num, char *sOUI, char *sSN, char *sClass );
int getManageDevEntity(char *name, struct CWMP_LEAF *entity, int *type, void **data)
{
	char		*lastname = entity->info->name;
	unsigned int	num=0;
	char		sOUI[7]="", sSN[65]="", sClass[65]="";
	
	if( (name==NULL) || (type==NULL) || (data==NULL) || (entity==NULL)) 
		return -1;

	num = getInstNum( name, "ManageableDevice" );
	if(num==0) return ERR_9005;
	getManageDeviceInfo( num, sOUI, sSN, sClass );

	*type = entity->info->type;
	*data = NULL;
	if( strcmp( lastname, "ManufacturerOUI" )==0 )
	{
		*data = strdup( sOUI );
	}else if( strcmp( lastname, "SerialNumber" )==0 )
	{
		*data = strdup( sSN );
	}else if( strcmp( lastname, "ProductClass" )==0 )
	{
		*data = strdup( sClass );
/*ping_zhang:20081217 START:patch from telefonica branch to support WT-107*/
#ifdef _PRMT_WT107_
	}else if( strcmp( lastname, "Host" )==0 )
	{
		extern unsigned int gDHCPTotalHosts;
		extern int updateDHCP();
		char hostStrHead[] = "InternetGatewayDevice.LANDevice.1.Hosts.Host.";
		char hostStr[1000];
		int i,len=0;
		
		updateDHCP();
		
		if(gDHCPTotalHosts>0)
		{
			for(i=0; i<gDHCPTotalHosts; i++)
			{
				if( len+strlen(hostStrHead)+16 > sizeof(hostStr) )
					break;

				if(i!=0)
					len += sprintf(hostStr+len,",");
				len += sprintf(hostStr+len,"%s%d",hostStrHead,i+1);
			}
			*data = strdup(hostStr);
		}
		else
			*data = strdup( "" );
#endif
/*ping_zhang:20081217 END*/
	}else{
		return ERR_9005;
	}
	
	return 0;
}

int objManageDevice(char *name, struct CWMP_LEAF *e, int type, void *data)
{
	struct CWMP_NODE *entity=(struct CWMP_NODE *)e;
	FILE *fp;
	char buf[160];

	//fprintf( stderr, "%s:action:%d: %s\n", __FUNCTION__, type, name);

	switch( type )
	{
	case eCWMP_tINITOBJ:
	     {
		int MaxInstNum=0,count=0;
		struct CWMP_LINKNODE **c = (struct CWMP_LINKNODE **)data;

		if( (name==NULL) || (entity==NULL) || (data==NULL) ) return -1;

		fp=fopen( TR111_DEVICEFILE, "r" );
		if(fp==NULL) return 0;
		
		while( fgets( buf,160,fp ) )
		{
			char *p;
			
			p = strtok( buf, " \n\r" );
			if( p && atoi(p)>0 )
			{
				if( MaxInstNum < atoi(p) )
					MaxInstNum = atoi(p);
				
				if( create_Object( c, tManageableDeviceObject, sizeof(tManageableDeviceObject), 1, atoi(p) ) < 0 )
					break;
				count++;
				//c = & (*c)->sibling;
			}
		}
		fclose(fp);
		gDeviceNumber = count;
		add_objectNum( name, MaxInstNum );
		return 0;
	     }
	case eCWMP_tUPDATEOBJ:	
	     {
	     	int count=0;
	     	struct CWMP_LINKNODE *old_table;
	
	     	old_table = (struct CWMP_LINKNODE *)entity->next;
	     	entity->next = NULL;

		fp=fopen( TR111_DEVICEFILE, "r" );
	     	while( fp && fgets( buf,160,fp ) )
	     	{
	     		struct CWMP_LINKNODE *remove_entity=NULL;
			char *p;
			
			p = strtok( buf, " \n\r" );
			if( p && atoi(p)>0 )
			{	
				remove_entity = remove_SiblingEntity( &old_table, atoi(p) );
				if( remove_entity!=NULL )
				{
					add_SiblingEntity( (struct CWMP_LINKNODE **)&entity->next, remove_entity );
				}else{ 
					unsigned int MaxInstNum;
					MaxInstNum = atoi(p);					
					add_Object( name, (struct CWMP_LINKNODE **)&entity->next,  tManageableDeviceObject, sizeof(tManageableDeviceObject), &MaxInstNum );
				}
				count++;
			}
	     	}
			
	     	if(fp) fclose(fp);

	     	gDeviceNumber = count;
	     	if( old_table )
	     		destroy_ParameterTable( (struct CWMP_NODE *)old_table );
	     	return 0;
	     }
	}
	
	return -1;
}



int getManageDeviceInfo( unsigned int num, char *sOUI, char *sSN, char *sClass )
{
	FILE *fp;
	int ret=-1;
	
	if( num<=0 || sOUI==NULL || sSN==NULL || sClass==NULL ) return ret;
	
	sOUI[0]=sSN[0]=sClass[0]=0;
	
	fp=fopen( TR111_DEVICEFILE, "r" );
	if(fp)
	{
		char buf[160], *p, *n;
		
		while( fgets( buf,160,fp ) )
		{
			p = strtok( buf, " \n\r" );
			n = strtok( NULL, "\n\r" );
			if( p && (atoi(p)==num) && n )
			{
				char *s1=NULL, *s2=NULL, *s3=NULL;
				
				s1 = strtok( n, "?\n\r" );
				s2 = strtok( NULL, "?\n\r" );
				s3 = strtok( NULL, "?\n\r" );
				if( s1 && s2 && s3 )
				{
					strncpy( sOUI, s1, 6 );
					sOUI[6]=0;
					strncpy( sClass, s2, 64 );
					sClass[64]=0;
					strncpy( sSN, s3, 64 );
					sSN[64]=0;
					ret = 0;
				}else if( s1 && s2 )
				{
					strncpy( sOUI, s1, 6 );
					sOUI[6]=0;
					sClass[0]=0;
					strncpy( sSN, s2, 64 );
					sSN[64]=0;
					ret = 0;
				}//else error
				break;
			}
		}
		
		fclose(fp);
	}
	return ret;
}
#endif
