#include "appUpload.h"
#include "apmib.h"
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

static int shm_id=0, shm_size;
char *shm_memory=NULL; //, *shm_name;


char *query_temp_var=NULL; 

#define ERROR -1
#define FAILED -1
#define SUCCESS 0
#define TRUE 1
#define FALSE 0

#define MACIE5_CFGSTR	"/plain\x0d\x0a\0x0d\0x0a"
#define WINIE6_STR	"/octet-stream\x0d\x0a\0x0d\0x0a"
#define MACIE5_FWSTR	"/macbinary\x0d\x0a\0x0d\0x0a"
#define OPERA_FWSTR	"/x-macbinary\x0d\x0a\0x0d\0x0a"
#define LINE_FWSTR	"\x0d\x0a\0x0d\0x0a"
#define LINUXFX36_FWSTR "/x-ns-proxy-autoconfig\x0d\x0a\0x0d\0x0a"



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
#endif
int configlen = 0;


#define MAX_QUERY_TEMP_VAL_SIZE 4096
#define MAX_BOUNDRY_LEN 64

#define MAX_INNER_REQ_BUFF 2048

typedef struct temp_mem_s
{
	char *str;
	struct temp_mem_s *next;
} temp_mem_t;

temp_mem_t root_temp;

int getcgiparam(char *dst,char *query_string,char *param,int maxlen)
{
	int len,plen;
	int y;
	char *end_str;
	
	end_str = query_string + strlen(query_string);
	plen = strlen(param);
	while (*query_string && query_string <= end_str) {
		len = strlen(query_string);
		if ((len=strlen(query_string)) > plen) {
			if (!strncmp(query_string,param,plen)) {
				if (query_string[plen] == '=') { //copy parameter
					query_string += plen+1;
					y = 0;
					while ((*query_string)&&(*query_string!='&')) {
						if ((*query_string=='%') && (strlen(query_string)>2)) {
							if ((isxdigit(query_string[1])) && (isxdigit(query_string[2]))) {
								if (y<maxlen) {
									dst[y++] = ((toupper(query_string[1])>='A'?toupper(query_string[1])-'A'+0xa:toupper(query_string[1])-'0') << 4)
									+ (toupper(query_string[2])>='A'?toupper(query_string[2])-'A'+0xa:toupper(query_string[2])-'0');
								}
								query_string += 3;
								continue;
							}
						}
						if (*query_string=='+')	{
							if (y < maxlen)
								dst[y++] = ' ';
							query_string++;
							continue;
						}
						if (y<maxlen)
							dst[y++] = *query_string;
						query_string++;
					}
					if (y<maxlen)
						dst[y] = 0;
					return y;
				}
			}
		}
		while ((*query_string)&&(*query_string!='&'))
			query_string++;
		if(!(*query_string))
			break;
		query_string++;
	}
	if (maxlen)
		dst[0] = 0;
	return -1;
}

/******************************************************************************/
int addTempStr(char *str)
{
	temp_mem_t *temp,*newtemp;

	temp = &root_temp;
	while (temp->next) {
		temp = temp->next;
	}
	newtemp = (temp_mem_t *)malloc(sizeof(temp_mem_t));
	if (newtemp==NULL)
		return FAILED;
	newtemp->str = str;
	newtemp->next = NULL;
	temp->next = newtemp;	
	return SUCCESS;
}


char *req_get_cstream_var(request *req, char *var, char *defaultGetValue)
{
	char *buf;
#ifndef NEW_POST
	struct stat statbuf;
#endif
	int ret=-1;
	
	if (req->method == M_POST) {
		int i;
// davidhsu --------------------------------
#ifndef NEW_POST
		fstat(req->post_data_fd, &statbuf);
		buf = (char *)malloc(statbuf.st_size+1);
		if (buf == NULL)
			return (char *)defaultGetValue;
		lseek(req->post_data_fd, SEEK_SET, 0);
		read(req->post_data_fd,buf,statbuf.st_size);
		buf[statbuf.st_size] = 0;
		i = statbuf.st_size - 1;
#else
		buf = (char *)malloc(req->post_data_len+1);
		if (buf == NULL)
			return (char *)defaultGetValue;
		req->post_data_idx = 0;
		memcpy(buf, req->post_data, req->post_data_len);
		buf[req->post_data_len] = 0;
		i = req->post_data_len -1;
#endif
//-------------------------------------------
		while (i > 0) {
			if ((buf[i]==0x0a)||(buf[i]==0x0d))
				buf[i]=0;
			else
				break;
			i--;
		}
	}
	else {
		buf = req->query_string;
	}

	if (buf != NULL) {
		ret = getcgiparam(query_temp_var,buf,var,MAX_QUERY_TEMP_VAL_SIZE);	
	}
	
	if (req->method == M_POST)
		free(buf);
	
	if (ret < 0)
		return (char *)defaultGetValue;
	buf = (char *)malloc(ret+1);
	memcpy(buf, query_temp_var, ret);
	buf[ret] = 0;
	addTempStr(buf);
	return (char *)buf; //this buffer will be free by freeAllTempStr().
}
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
#if 0
int fwChecksumOk(char *data, int len)
{
	unsigned short sum=0;
	int i;

	for (i=0; i<len; i+=2) {
#ifdef _LITTLE_ENDIAN_
		sum += WORD_SWAP( *((unsigned short *)&data[i]) );
#else
		sum += *((unsigned short *)&data[i]);
#endif

	}
	return( (sum==0) ? 1 : 0);
}
#endif
int get_shm_id()
{
	return shm_id;
}

int set_shm_id(int id)
{
	 shm_id=id;
	 return 0; //fix converity error: MISSING_RETURN
}
int clear_fwupload_shm(int shmid)
{
	//shm_memory = (char *)shmat(shmid, NULL, 0);
	if(shm_memory){
		if (shmdt(shm_memory) == -1) {
			fprintf(stderr, "shmdt failed\n");
		}
	}

	if (shm_id != 0) {
		if(shmctl(shm_id, IPC_RMID, 0) == -1)
			fprintf(stderr, "shmctl(IPC_RMID) failed\n");
	}
	shm_id = 0;
	shm_memory = NULL;
	return 0; //fix converity error: MISSING_RETURN
}

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
	//extern int get_shm_id();
	//extern int clear_fwupload_shm();
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
	printf("[zzl][RTL_8367R_DUAL_BAND]aa Reboot_Wait = %d\n", Reboot_Wait);

#elif defined(RTL_8367R_8881a_DUAL_BAND)
	Reboot_Wait = (wp->upload_len/69633)+57+5+25;
	printf("[zzl][RTL_8367R_DUAL_BAND]bb Reboot_Wait = %d\n", Reboot_Wait);

#elif defined(CONFIG_RTL_8198C)
	Reboot_Wait = (wp->upload_len/19710)+50+5;
printf("[zzl][RTL_8367R_DUAL_BAND]cc Reboot_Wait = %d\n", Reboot_Wait);

#else
	Reboot_Wait = (wp->upload_len/69633)+57+5;
printf("[zzl][RTL_8367R_DUAL_BAND]dd Reboot_Wait = %d\n", Reboot_Wait);

#endif
	printf("[zzl][RTL_8367R_DUAL_BAND]aa update_cfg = %d, update_fw = %d\n", update_cfg, update_fw);

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
	//OK_MSG_FW(tmpBuf, submitUrl,Reboot_Wait,lan_ip);
#endif
	return;

ret_upload:
	
#if defined(CONFIG_APP_FWD)		
	clear_fwupload_shm(shm_id);
#endif
	Reboot_Wait=0;
}

