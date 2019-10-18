/*
 * Copyright (c) 2019, All rights reserved.
 *
 * File         : remoteUpgrade.c
 * Status       : Current
 * Description  :
 *
 * Author       : haopeng
 * Contact      : 376915244@qq.com
 *
 * Revision     : 2019-08
 * Description  : Primary released
 *
 * ## Please log your description here for your modication ##
 *
 * Revision     :
 * Modifier     :
 * Description  :
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/shm.h>
#include "remoteUpgrade.h"
#include "cJSON.h"
#include "md5.h"
#include "boa.h"
#include "apform.h"
#include "utility.h"
#include "apmib.h"

	
#define CHECK_OK_STRING "Firmware upgrade check OK!\n"
#include <fcntl.h> 
#include <sys/stat.h>
#include <linux/limits.h>
#include <linux/ioctl.h>
	
#include <sys/types.h>
	
remoteUpgrade_t remoteUpgradeInfo;

static unsigned long parseDns(const char* host_name)
{
	struct hostent* host;
	struct in_addr addr;
	char ** pp;

	host = gethostbyname(host_name);
	if(NULL == host)
	{
		printf("autoupgradeDns: gethostbyname %s failed,try again\n", host_name);

		sleep(2) ;
		
		host = gethostbyname(host_name);
		
		if(NULL == host)
		{
			sleep(2) ;
			
			host = gethostbyname(host_name);
			
			if(NULL == host)
			{
				return -1;
			}
		}
	}
	pp = host->h_addr_list;
	
	if(*pp != NULL)
	{
		addr.s_addr = *((unsigned int *)*pp);
		pp++;
		
		printf("autoupgradeDns: gethostbyname %s sucess.\n", host_name);

		return addr.s_addr;
	}

	printf("autoupgradeDns: gethostbyname %s fail.\n", host_name);

	return 0;
}

static int httpTcpClientCreate(const char *host, int port){
    struct hostent *he;
    struct sockaddr_in serverAddr; 
    int socketFd;
 
    if((he = gethostbyname(host))==NULL){
        return -1;
    }
 
   serverAddr.sin_family = AF_INET;
   serverAddr.sin_port = htons(port);
   serverAddr.sin_addr = *((struct in_addr *)he->h_addr);
 
    if((socketFd = socket(AF_INET,SOCK_STREAM,0))==-1){
        return -1;
    }
 
    if(connect(socketFd, (struct sockaddr *)&serverAddr,sizeof(struct sockaddr)) == -1){
        return -1;
    }
 
    return socketFd;
}
 
static void httpTcpClientClose(int socket){
    close(socket);
}
 
static int httpParseUrl(const char *url,char *host,char *file,int *port)
{
    char *ptr1,*ptr2;
    int len = 0;
    if(!url || !host || !file || !port){
        return -1;
    }
 
    ptr1 = (char *)url;
 
    if(!strncmp(ptr1,"http://",strlen("http://"))){
        ptr1 += strlen("http://");
    }else{
        return -1;
    }
 
    ptr2 = strchr(ptr1,'/');
    if(ptr2){
        len = strlen(ptr1) - strlen(ptr2);
        memcpy(host,ptr1,len);
        host[len] = '\0';
        if(*(ptr2 + 1)){
            memcpy(file,ptr2 + 1,strlen(ptr2) - 1 );
            file[strlen(ptr2) - 1] = '\0';
        }
    }else{
        memcpy(host,ptr1,strlen(ptr1));
        host[strlen(ptr1)] = '\0';
    }
    /*get host and ip*/
    ptr1 = strchr(host,':');
    if(ptr1){
        *ptr1++ = '\0';
        *port = atoi(ptr1);
    }else{
        *port = MY_HTTP_DEFAULT_PORT;
    }
 
    return 0;
}
 
 
static int httpTcpClientRecv(int socket,char *lpbuff){
    int recvnum = 0;
 
    recvnum = recv(socket, lpbuff,BUFFER_SIZE*4,0);
    printf("received response:\n%s \n",lpbuff);
 
    return recvnum;
}
 
static int httpTcpClientSend(int socket,char *buff,int size){
    int sent=0,tmpres=0;
 
    while(sent < size){
        tmpres = send(socket,buff+sent,size-sent,0);
        if(tmpres == -1){
            return -1;
        }
        sent += tmpres;
    }
    return sent;
}
 
static char *httpParseResult(const char*recvBuf)
{
    char *ptmp = NULL; 
    char *response = NULL;
	char *recvBufPtr=NULL;
	char *dataHead,*dataTail;
	int count = 0;
	char httpHead[512];
	    printf("---->recvBuf:\n%s\n",recvBuf);
    ptmp = (char*)strstr(recvBuf,"HTTP/1.1");
    if(!ptmp){
        printf("http/1.1 not faind\n");
        return NULL;
    }
    if(atoi(ptmp + 9)!=200){
        printf("result:\n%s\n",recvBuf);
        return NULL;
    }
 
    ptmp = (char*)strstr(recvBuf,"\r\n\r\n");
    if(!ptmp){
        printf("ptmp is NULL\n");
        return NULL;
    }
    response = (char *)malloc(strlen(ptmp)+1);
    if(!response){
        printf("malloc failed \n");
        return NULL;
    }
    //strcpy(response,ptmp+4);
	response = strchr(ptmp+4,'{');
    return response;
}

 
char * httpGet(const char *url)
{
    char post[BUFFER_SIZE] = {0};
    int socketFd = -1;
    char recvBuf[BUFFER_SIZE*4] = {0};
    char *ptmp;
    char hostAddr[BUFFER_SIZE] = {'\0'};
    char file[BUFFER_SIZE] = {'\0'};
    int port = 0;
    int len=0;
	
    printf("--->httpGeturl=%s\n",url);
    if(!url){
        printf("      failed!\n");
        return NULL;
    }

    if(httpParseUrl(url,hostAddr,file,&port)){
        printf("httpParseUrl failed!\n");
        return NULL;
    }
    printf("hostAddr : %s\tfile:%s\t,%d\n",hostAddr,file,port);
 
    socketFd =  httpTcpClientCreate(hostAddr,port);
    if(socketFd < 0){
        printf("httpTcpclientCreate failed\n");
        return NULL;
    }
 
    sprintf(recvBuf,HTTP_GET,file,hostAddr,port);
 
    if(httpTcpClientSend(socketFd,recvBuf,strlen(recvBuf)) < 0){
        printf("httpCcpclientSend failed..\n");
        return NULL;
    }
	printf("send request:\n%s\n",recvBuf);
 
    if(httpTcpClientRecv(socketFd,recvBuf) <= 0){
        printf("httpTcpclientRecv failed\n");
        return NULL;
    }
    httpTcpClientClose(socketFd);
 
    return httpParseResult(recvBuf);
}

/*
 *parse Data as fellow:
 {"flag":true,"code":200,"message":"查询成功",
 "data":{"fid":"3e37bade62a44a618753e0081e756e75",
 "name":"测试2","type":0,"version":"1.0.0",
 "createtime":1566898148000,"content":"656564656",
 "downurl":"http://120.79.61.154:9001/firmware/download?fid=3e37bade62a44a618753e0081e756e75",
 "md5":"80AE29AFD0C97B2DF8F970567F09BE4B",
 "binurl":"/usr/uploads/bin/20192927012907.png","ssid":null}}
*/
remoteUpgrade_t getUpgradeFileInfo(remoteUpgrade_t fileInfo,char *recvData)
{
  cJSON *jsonData=NULL;
  cJSON *jasonObj=NULL;
  jsonData = cJSON_Parse(recvData);
  if (!jsonData)
  {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
  }
   if(jasonObj = cJSON_GetObjectItem(jsonData,"data"))
   {
 	strcpy(fileInfo.downloadUrl,cJSON_GetObjectItem(jasonObj,"downurl")->valuestring);
	strcpy(fileInfo.md5,cJSON_GetObjectItem(jasonObj,"md5")->valuestring);
	strcpy(fileInfo.remoteFwVersion,cJSON_GetObjectItem(jasonObj,"version")->valuestring);
	strcpy(fileInfo.versionUpdateLog,cJSON_GetObjectItem(jasonObj,"content")->valuestring);
//	printf("--->downurl=%s remoteFwVersion=%s md5=%s\n",fileInfo.downloadUrl,fileInfo.remoteFwVersion,fileInfo.md5);
   }
   if(jsonData != NULL)
     free(jsonData);
   return fileInfo;
}
int upgradeInit(remoteUpgrade_t fileInfo)
{
 memset(&fileInfo,0,sizeof(remoteUpgrade_t));
 return 0;
}

int getFileRequestUrl(char url[])
{
  unsigned long hostIpAddr;
  char tmpBuf[16]={0};
  unsigned int ip=0;
  hostIpAddr=parseDns(UPGRADE_GILE_SERVER_HOST_NAME);
  ip=ntohl(hostIpAddr);
  sprintf(tmpBuf,IP_DOT_FORMATE,IP_SEGMENT(ip));
  sprintf(url,UPGRADE_FILE_SERVER_PATH,tmpBuf);
  return 0;
}

/*
#parameters:
 * filePath:file path need generate md5
 * md5:md5 value from remote server
#function:
 *compare md5,equal means file is ok,otherwise file is invalid,exit upgrade 
#return value:
 *0:file is ok     1:file invalid
*/
int checkFileMd5(const char *filePath, char md5[64])
{
	int ret=1;
	char md5Value[MD5_STR_LEN + 1];
	ret = computeUpgradeFileMd5(filePath, md5Value);
	if (0 == ret)
	{
	   // printf("------>[localFile:  md5 value:%s]\n", md5Value);
	//	printf("------>[serverFile: md5 value:%s]\n", md5);
		ret=strncmp(md5Value,md5,MD5_STR_LEN);
	}
	return ret;
} 

int computeUpgradeFileMd5(const char *filePath, char *md5Value)
{
	int i;
	int fd;
	int ret;
	unsigned char data[READ_DATA_SIZE];
	unsigned char md5_value[MD5_SIZE];
	MD5_CTX md5;

	fd = open(filePath, O_RDONLY);
	if (-1 == fd)
	{
		perror("open");
		return -1;
	}

	MD5Init(&md5);
	while (1)
	{
		ret = read(fd, data, READ_DATA_SIZE);
		if (-1 == ret)
		{
			perror("read");
			return -1;
		}
		MD5Update(&md5, data, ret); 
		if (0 == ret || ret < READ_DATA_SIZE)
		{
			break;
		}
	} 
	close(fd);
	MD5Final(&md5, md5_value); 
	for(i = 0; i < MD5_SIZE; i++)
	{
		snprintf(md5Value + i*2, 2+1, "%02x", md5_value[i]);
	}
	md5Value[MD5_STR_LEN] = '\0'; // add end
	return 0;
}
static firmreCheck_t app_getUpgradeFile(const char *url,const char *md5) {
    FILE *fp = NULL;
	char *ptr = NULL;
	struct stat f_stat;
	char cmdBuf[128];
	int ret =1;
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sprintf(cmdBuf,"cd /tmp ; wget -O fw.bin %s",url);
    system(cmdBuf);
	if(access(LOCATION_FILE_PATH, R_OK)==0)
	{
		stat(LOCATION_FILE_PATH, &f_stat);
		printf("/tmp/fw.bin file size = %d\n", f_stat.st_size);
		if(f_stat.st_size < 5000000)
		{
			ret=FIRMWARE_ERROR_FILE_SIZE;
			return ret;
		}
		
		ret=checkFileMd5(LOCATION_FILE_PATH, md5);
		if(ret==0)
			printf("##firmware md5 is right\n");	
		else
			printf("--firmware md5 is error\n");	
	}      
	else
	{
		ret=FIRMWARE_ERROR_FILE_PATH_NO_FIRMRE;
	}
   
    printf("firmware md5 is right ret=%d\n",ret);
    return ret;
}
int app_performUpgrade()
{
	int ret =2;
	ret=app_getUpgradeFile(remoteUpgradeInfo.downloadUrl, remoteUpgradeInfo.md5);
	if(FIRMWARE_VALID==ret)
		return 1;

	return 0;
}

static firmreCheck_t getUpgradeFile(const char *url,const char *md5) {
    FILE *fp = NULL;
	char *ptr = NULL;
    char buf[128];
	char cmdBuf[128];
	int ret =1;
	memset(cmdBuf, 0, sizeof(cmdBuf));
	sprintf(cmdBuf,"cd /tmp ; wget -O fw.bin %s",url);
    if ((fp = popen(cmdBuf, "r")) != NULL) 
	{
        memset(buf, 0, sizeof(buf));
        while(fgets(buf, sizeof(buf)-1, fp)) 

		{         
            ptr = strstr(buf, "Network is unreachable");
            if (ptr) 
			{
                printf("Network is unreachable\n");
				ret =FIRMWARE_ERROR_NETWORK_UNREACHABLE;
                break;
            }

            memset(buf, 0, sizeof(buf));
        }

        pclose(fp);
    }
    if(access(LOCATION_FILE_PATH,R_OK)==0)
    {
      ret=checkFileMd5(LOCATION_FILE_PATH, md5);
	  if(ret==0)
	    printf("##firmware md5 is right\n");	
	  else
	  	printf("--firmware md5 is error\n");	
    }      
    else
    {
     ret=FIRMWARE_ERROR_FILE_PATH_NO_FIRMRE;
    }
   
    printf("firmware md5 is right ret=%d\n",ret);
    return ret;
}


int getUpgradeFileSize(const char *filePath)
{
    int fileSize = -1; 
    FILE *fp; 

    fp = fopen(filePath, "rb");  
    if(fp == NULL)
    {    
     printf("getUpgradeFileSize open firmware fail!\n");
     return fileSize;
    }     
    fseek(fp, 0L, SEEK_END);  
    fileSize = ftell(fp);  
    fclose(fp);  
    return fileSize;   
}
void FirmwareUpgradeReboot(int shm_id, int update_cfg, int update_fw)
{
#if 1
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
#endif
	
#ifndef NO_ACTION
#if defined(CONFIG_APP_FWD)
	{			
			char tmpStr[20]={0};
			sprintf(tmpStr,"%d",shm_id);
			write_line_to_file("/var/fwd.ready", 1, tmpStr);
			sync();
			exit(0);
	}
#else	//#if defined(CONFIG_APP_FWD)
	system("reboot");
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
}

static int updateConfigToFlash(unsigned char *data, int total_len, int *pType, int *pStatus)
{
	int len=0, status=1, type=0, ver, force;
	PARAM_HEADER_Tp pHeader;
	COMPRESS_MIB_HEADER_Tp pCompHeader;
	unsigned char *expFile=NULL;
	unsigned int expandLen=0;
	int complen=0;
	char *ptr;
	unsigned char isValidfw = 0;

	do {
		if (
			memcmp(&data[complen], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) &&
			memcmp(&data[complen], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)

		) {
			if (isValidfw == 1)
				break;
		}
		
		pCompHeader =(COMPRESS_MIB_HEADER_Tp)&data[complen];
#ifdef _LITTLE_ENDIAN_
		pCompHeader->compRate = WORD_SWAP(pCompHeader->compRate);
		pCompHeader->compLen = DWORD_SWAP(pCompHeader->compLen);
#endif
		/*decompress and get the tag*/
		expFile=malloc(pCompHeader->compLen*pCompHeader->compRate);
		if (NULL==expFile) {
			printf("malloc for expFile error!!\n");
			return 0;
		}
		expandLen = Decode(data+complen+sizeof(COMPRESS_MIB_HEADER_T), pCompHeader->compLen, expFile);
		pHeader = (PARAM_HEADER_Tp)expFile;
		
#ifdef _LITTLE_ENDIAN_
		pHeader->len = WORD_SWAP(pHeader->len);
#endif
		len += sizeof(PARAM_HEADER_T);

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

			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));

			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
		apmib_updateFlash(CURRENT_SETTING, (char *)&data[complen], pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}

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

			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));

			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
			apmib_updateFlash(DEFAULT_SETTING, (char *)&data[complen], pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}	

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

			ptr = (char *)(expFile+sizeof(PARAM_HEADER_T));
			
			if ( !CHECKSUM_OK((unsigned char *)ptr, pHeader->len)) {
				status = 0;
				break;
			}
			apmib_updateFlash(HW_SETTING, (char *)&data[complen], pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T), force, ver);
			complen += pCompHeader->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			if (expFile) {
				free(expFile);
				expFile=NULL;
			}

			type |= HW_SETTING;
			continue;
		}
	}
	while (complen < total_len);

	if (expFile) {
		free(expFile);
		expFile=NULL;
	}

	*pType = type;
	*pStatus = status;
	return complen;
}


static int firmwareUpgrade(char *upload_data, int upload_len, int shm_id, char *errMsgBuf,
							int checkonly, int *update_cfg, int *update_fw)
{
	int head_offset=0 ;
	int isIncludeRoot=0;
	int		 len;
	int          locWrite;
	int          numLeft;
	int          numWrite;
	IMG_HEADER_Tp pHeader;
	int flag=0, startAddr=-1, startAddrWeb=-1;
	int fh;

	int fwSizeLimit = CONFIG_FLASH_SIZE;
#ifdef CONFIG_RTL_FLASH_DUAL_IMAGE_ENABLE
	int active_bank,backup_bank;
	int dual_enable =0;
#endif
	unsigned char isValidfw = 0;

#if defined(CONFIG_APP_FWD)
#define FWD_CONF "/var/fwd.conf"
	int newfile = 1;
#endif

	/* mtd name mapping to mtdblock */
#ifdef MTD_NAME_MAPPING
	char webpageMtd[16],linuxMtd[16],rootfsMtd[16];
	if(rtl_name_to_mtdblock(FLASH_WEBPAGE_NAME,webpageMtd) == 0
		|| rtl_name_to_mtdblock(FLASH_LINUX_NAME,linuxMtd) == 0
			|| rtl_name_to_mtdblock(FLASH_ROOTFS_NAME,rootfsMtd) == 0){
		strcpy(errMsgBuf, "cannot find webpage/linux/rootfs mtdblock!");
		goto ret_upload;
	}
#endif

	*update_fw=0;
	*update_cfg=0;

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
				!memcmp(&upload_data[head_offset], COMP_HS_SIGNATURE, COMP_SIGNATURE_LEN) ||
				!memcmp(&upload_data[head_offset], COMP_DS_SIGNATURE, COMP_SIGNATURE_LEN) ||
				!memcmp(&upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)	
				) {				
			int configlen = 0;
			int type=0, status=0, cfg_len;
			
			COMPRESS_MIB_HEADER_Tp pHeader_cfg;
			pHeader_cfg = (COMPRESS_MIB_HEADER_Tp)&upload_data[head_offset];
			if(!memcmp(&upload_data[head_offset], COMP_CS_SIGNATURE, COMP_SIGNATURE_LEN)) {
				configlen +=  pHeader_cfg->compLen+sizeof(COMPRESS_MIB_HEADER_T);
			}


			cfg_len = updateConfigToFlash((unsigned char *)&upload_data[head_offset],configlen , &type, &status);

			if (status == 0 || type == 0) { 
				strcpy(errMsgBuf, "Invalid configuration file!");
				goto ret_upload;
			}
			else { 
				strcpy(errMsgBuf, "Update successfully!");
				head_offset += cfg_len;
				isValidfw = 1;
				*update_cfg = 1;
			}
			continue;
		}
		else {
			if (isValidfw == 1)
				break;
			strcpy(errMsgBuf, ("Invalid file format!"));
			goto ret_upload;
		}

		if (len > fwSizeLimit) {  
			sprintf(errMsgBuf, ("Image len exceed max size 0x%x ! len=0x%x</b><br>"),fwSizeLimit, len);
			goto ret_upload;
		}
		if ( (flag == 1) || (flag == 3)) {
			if ( !fwChecksumOk(&upload_data[sizeof(IMG_HEADER_T)+head_offset], len)) {
				sprintf(errMsgBuf, ("Image checksum mismatched! len=0x%x, checksum=0x%x</b><br>"), len,
					*((unsigned short *)&upload_data[len-2]) );
				goto ret_upload;
			}
		}
		else {
			char *ptr = &upload_data[sizeof(IMG_HEADER_T)+head_offset];
			if ( !CHECKSUM_OK((unsigned char *)ptr, len) ) {
				sprintf(errMsgBuf, ("Image checksum mismatched! len=0x%x</b><br>"), len);
				goto ret_upload;
			}
		}

		if (flag == 3)	//rootfs
		{

			fh = open(rootfsMtd, O_RDWR);
#if defined(CONFIG_APP_FWD)
			if(!checkonly) write_line_to_file(FWD_CONF, (newfile==1?1:2), rootfsMtd);
			newfile = 2;
#endif			
		}
		else if(flag == 1)	// linux
		{

			fh = open(linuxMtd, O_RDWR);
#if defined(CONFIG_APP_FWD)	
			if(!checkonly) write_line_to_file(FWD_CONF, (newfile==1?1:2), linuxMtd);
			newfile = 2;
#endif			
		}
		else	// web
		{
			fh = open(webpageMtd, O_RDWR);
#if defined(CONFIG_APP_FWD)	
			if(!checkonly) write_line_to_file(FWD_CONF, (newfile==1?1:2), webpageMtd);
			newfile = 2;
#endif			
		}


		if ( fh == -1 ) {
			strcpy(errMsgBuf, ("File open failed!"));
		} else {
			if (flag == 1) {
				if (startAddr == -1) {
					startAddr = pHeader->burnAddr ;
#ifdef _LITTLE_ENDIAN_
					startAddr = DWORD_SWAP(startAddr);
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
				}
				else
					startAddr = startAddrWeb;
			}
			lseek(fh, startAddr, SEEK_SET);
			
#if defined(CONFIG_APP_FWD)			
			{
				char tmpStr[20]={0};
				sprintf(tmpStr,"\n%d",startAddr);
				if(!checkonly) write_line_to_file(FWD_CONF, (newfile==1?1:2), tmpStr);
				newfile = 2;
			}
#endif			
			
			
						
			if (flag == 3) {
				locWrite += sizeof(IMG_HEADER_T); // remove header
				numLeft -=  sizeof(IMG_HEADER_T);
			}

			{
				char tmpStr[20]={0};
				sprintf(tmpStr,"\n%d",numLeft);
				if(!checkonly) write_line_to_file(FWD_CONF, (newfile==1?1:2), tmpStr);
				sprintf(tmpStr,"\n%d\n",locWrite+head_offset);
				if(!checkonly) write_line_to_file(FWD_CONF, (newfile==1?1:2), tmpStr);					
				newfile = 2;
			}
			
			locWrite += numWrite;
			numLeft -= numWrite;
			sync();
			close(fh);

			head_offset += len + sizeof(IMG_HEADER_T) ;
			startAddr = -1 ; //by sc_yang to reset the startAddr for next image
			*update_fw = 1;
		}
	}  
    remoteUpgradeInfo.checkVersionStatus=0;
	remoteUpgradeInfo.upgradeConfirm=0;
	fprintf(stderr,"remote upgrade ok!\n");
	
	
	return 1;
ret_upload: 
	fprintf(stderr, "%s\n", errMsgBuf);
	
	return 0;
}


void prepareFirmware(const char *localFilePath)
{
	int shm_id;
	size_t shm_size;
	char *shm_ptr;
	char err_msg_buff[4096];
	int update_cfg, update_fw;
	int checkonly = 0;

     if (access(localFilePath,R_OK)==0) {
		struct stat stat_buff;
		key_t key;
		FILE *fp;
		int left_size, offset, r;

		/*get share memory and copy firmware to memory*/
		if (stat(localFilePath, &stat_buff) == -1) {
			printf(" get the filesize failed %s.\n", localFilePath);
			return 0;
		}
		remoteUpgradeInfo.totalSize=stat_buff.st_size;
		shm_size = stat_buff.st_size;
		key = ftok(localFilePath, 0xE04);
		shm_id = shmget(key, shm_size, IPC_CREAT | 0666);	
		if (shm_id == -1) {
			printf("create shared memory for file %s fail, key=%d, errno=%d\n", localFilePath, key, errno);
			return 0;
		}
		shm_ptr = shmat(shm_id, NULL, 0);
		if (shm_ptr == NULL) {
			printf("Cannot attach shared memory for file %s, key=%d, shm_id=%d, errno=%d\n", localFilePath, key, shm_id, errno);
			return 0;
		} 
		fp = fopen(localFilePath, "rb"); 
		if (fp==NULL) {
			printf("open %s fail!\n", localFilePath);
			if (shmdt(shm_ptr) == -1) 
      				fprintf(stderr, "shmdt failed\n");
			if(shmctl(shm_id, IPC_RMID, 0) == -1)
				fprintf(stderr, "shmctl(IPC_RMID) failed\n");
			return 0;
		}
		for (offset = 0, left_size = stat_buff.st_size; left_size>0; offset+=r, left_size-=r) {
			r = fread(&shm_ptr[offset], 1, left_size, fp);
		}

		fclose(fp);
	} 

	if(!firmwareUpgrade(shm_ptr, shm_size, shm_id, err_msg_buff, checkonly, &update_cfg, &update_fw)) {
		printf("Failed to upgrade firmware! error:\n");
		printf("%s\n", err_msg_buff);

#if defined(CONFIG_APP_FWD)		
		clear_fwupload_shm(shm_ptr, shm_id);
#endif
		return 0;
	}
	if (checkonly) {		
		printf(CHECK_OK_STRING);	
		clear_fwupload_shm(shm_ptr, shm_id);
	} else {
printf("Reboot!\n");	
		// Reboot!
		FirmwareUpgradeReboot(shm_id, update_cfg, update_fw);
	}
	return 0;
}

/*retuturn value:
*         0:curent version is newer,need't upgrade
*         1:curent version is old,upgrade is needed
*         2.other  error
*/
int checkVersionStatus(const char* remoteFwVersion,const char* localFwVersion)
{
  //formate ex:   VM V1.0.4
  uint32_t vInt1,vInt2;
  uint32_t remoteVersion[3];
  uint32_t localVersion[3];
  
  printf("getUpgradeFileInfo:--->remoteFwVersion=%s localFwVersion=%s\n",remoteFwVersion,localFwVersion);
  if (sscanf(remoteFwVersion, "WM V%d.%d.%d", &remoteVersion[0], &remoteVersion[1], &remoteVersion[2]) != 3)
  {
    printf("parse remoteFwVersion error\n");
    return 2;
  }
  vInt1 = ((remoteVersion[0] << 16) + (remoteVersion[1] << 8) + (remoteVersion[2]));

  if (sscanf(localFwVersion, "WM V%d.%d.%d", &localVersion[0], &localVersion[1], &localVersion[2]) != 3)
  {
    printf("parse remoteFwVersion error");
    return 2;
  }
  vInt2 = ((localVersion[0] << 16) + (localVersion[1] << 8) + (localVersion[2]));

  return  (vInt1>vInt2)?1:0;
 
}


void remoteChekUpgrade()
{
 char *recvDataPtr=NULL;
 char requestUrl[128]={0};
 int ret=0;
 upgradeInit(remoteUpgradeInfo);
 getFileRequestUrl(requestUrl);
 recvDataPtr=httpGet(requestUrl);
 if(NULL==recvDataPtr)
 {
   printf("httpGet return fail...\n");
 }
 else
 {
   printf("=====>receiveData:\n%s\n",recvDataPtr);
   remoteUpgradeInfo=getUpgradeFileInfo(remoteUpgradeInfo,recvDataPtr);
  // printf("++++>downurl=%s md5=%s\n",remoteUpgradeInfo.downloadUrl,remoteUpgradeInfo.md5);
   remoteUpgradeInfo.checkVersionStatus=checkVersionStatus(remoteUpgradeInfo.remoteFwVersion,LOCAL_FW_VERSION);

 }
 
}
void performUpgrade()
{
 printf("----->%s---%d----------<",__FUNCTION__,__LINE__);
 int ret =2;
 ret=getUpgradeFile(remoteUpgradeInfo.downloadUrl,remoteUpgradeInfo.md5);
 if(FIRMWARE_VALID==ret)
 {
 prepareFirmware(LOCATION_FILE_PATH);
 }
 else
 {
  printf("getUpgradeFile return fail ret=%d...\n",ret);
 }
}
