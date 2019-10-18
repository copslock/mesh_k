 /*
  * Copyright (c) 2019, All rights reserved.
  *
  * File		 : deviceProcIf.c
  * Status		 : Current
  * Description  :
  *
  * Author		 : haopeng
  * Contact 	 : 376915244@qq.com
  *
  * Revision	 : 2019-08
  * Description  : Primary released
  *
  * ## Please log your description here for your modication ##
  *
  * Revision	 :
  * Modifier	 :
  * Description  :
  *
  * */

#ifndef _REMOTE_UPGRADE_H
#define _REMOTE_UPGRADE_H
#define MY_HTTP_DEFAULT_PORT 80
#define UPGRADE_FILE_SERVER_PATH "http://%s/firmware/latest"  //http://120.79.61.154/firmware/latest
#define UPGRADE_GILE_SERVER_HOST_NAME "mesh01.kingsignal.cn"  //mesh01.kingsignal.cn  mesh01.jxngroup.com
#define LOCATION_FILE_PATH "/tmp/fw.bin"
#define BUFFER_SIZE 1024
#define HTTP_GET "GET /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n\r\n"
#define IP_DOT_FORMATE "%u.%u.%u.%u"
#define IP_SEGMENT(addr) \
 ((unsigned char *)&addr)[3], \
 ((unsigned char *)&addr)[2], \
 ((unsigned char *)&addr)[1], \
 ((unsigned char *)&addr)[0]


#define READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)
#define FUNCTION_CONTROLLER_FILE	"/var/run/REMOTE_FUNCTION_ON"
#define HEAD_OFFSET_SIZE   150
extern fwVersion;
#define LOCAL_FW_VERSION	fwVersion


char * httpGet(const char *url);

typedef struct 
{
    char checkUpgradeUrl[128];              //check where to get upgrade file
    char downloadUrl[256];                   //url to download upgrade file 
    char remoteFwVersion[16];               //url to download upgrade file 
    char md5[64];
	char versionUpdateLog[128];
	unsigned int totalSize; 
	int isRemoteUpgrade;                   // 0---skip remote upgrade  1---ready to update process
    char remoteUpgradeStatus;             //0---upgrade process done  1---ready to update process
    int checkVersionStatus;              //0---current version is newer,needn't upgrade  1---current version is older,upgrade is needed
    int upgradeConfirm;                  //0---user havn't confirm upgrade     1---get confirm request,start to upgrade
    int firmwareCheckStatus;              // see enum FIRMWARE_CHECK    
    int uploadRequest;                  //0----none request 1---get request
}remoteUpgrade_t;


 /* firmre check result */
typedef enum
{
	DEFAULT=0,
	FIRMWARE_VALID=DEFAULT,
	FIRMWARE_ERROR_NETWORK_UNREACHABLE,
	FIRMWARE_ERROR_FILE_PATH_NO_FIRMRE,
	FIRMWARE_ERROR_FILE_SIZE,
}firmreCheck_t;

int computeUpgradeFileMd5(const char *file_path, char *value);

#endif
