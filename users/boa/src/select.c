/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1996-2003 Jon Nelson <jnelson@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* $Id: select.c,v 1.1.2.17 2005/02/22 14:11:29 jnelson Exp $*/

/* algorithm:
 * handle any signals
 * if we still want to accept new connections, add the server to the
 * list.
 * if there are any blocked requests or the we are still accepting new
 * connections, determine appropriate timeout and select, then move
 * blocked requests back into the active list.
 * handle active connections
 * repeat
 */



#include "boa.h"
#include "remoteUpgrade.h"

#ifdef BOA_WITH_OPENSSL
#include <openssl/ssl.h>
extern int server_ssl;
extern int do_sock;
extern SSL_CTX *ctx;
#define MAX(a,b)	((a)>(b))?(a):(b)
#endif

extern int firmware_len;
extern char *firmware_data;
static void fdset_update(void);
fd_set block_read_fdset;
fd_set block_write_fdset;
int max_fd = 0;
extern int isFWUPGRADE;
extern int isREBOOTASP;
extern int isCFGUPGRADE;

int req_after_upgrade=0;
int last_req_after_upgrade=0;
int confirm_last_req=0;
extern int reboot_time;
extern void remoteChekUpgrade();
extern remoteUpgrade_t;
extern remoteUpgrade_t remoteUpgradeInfo;
void loop(int server_s)
{
	//printf("<%s:%d>enter\n", __FUNCTION__,__LINE__);
	FD_ZERO(BOA_READ);
	FD_ZERO(BOA_WRITE);
    int ret;

	max_fd = -1;
#if defined(BOA_WITH_MBEDTLS)
    int tls_fd = ((mbedtls_net_context *) &mbedtls_listen_fd)->fd;
#endif

	while (1) {
		/* handle signals here */
		if (sighup_flag)
			sighup_run();
		if (sigchld_flag)
			sigchld_run();
		if (sigalrm_flag)
			sigalrm_run();

		if (sigterm_flag) {
			/* sigterm_flag:
			 * 1. caught, unprocessed.
			 * 2. caught, stage 1 processed
			 */
			if (sigterm_flag == 1) {
				sigterm_stage1_run();
				BOA_FD_CLR(req, server_s, BOA_READ);
				close(server_s);
				/* make sure the server isn't in the block list */
				server_s = -1;
			}
			if (sigterm_flag == 2 && !request_ready && !request_block) {
				sigterm_stage2_run(); /* terminal */
			}
		} else {
			if (total_connections > max_connections) {
				/* FIXME: for poll we don't subtract 20. why? */
				BOA_FD_CLR(req, server_s, BOA_READ);
			} else {
				BOA_FD_SET(req, server_s, BOA_READ); /* server always set */
			}
		}
		if (isREBOOTASP == 1) {
			if(last_req_after_upgrade != req_after_upgrade){
				last_req_after_upgrade = req_after_upgrade;
			}
		}

		pending_requests = 0;
		/* max_fd is > 0 when something is blocked */

#ifdef BOA_WITH_OPENSSL
		//printf("<%s:%d>do_sock=%d\n",__FUNCTION__,__LINE__,do_sock);
		if (do_sock < 2)
			max_fd = MAX(server_ssl, max_fd);
		fdset_update();
#elif defined(BOA_WITH_MBEDTLS)
        max_fd = MAX(tls_fd, server_s);
		fdset_update();
#endif

		if (max_fd) {
			struct timeval req_timeout; /* timeval for select */

			req_timeout.tv_sec = (request_ready ? 0 : default_timeout);
			req_timeout.tv_usec = 0l; /* reset timeout */

			if (select(max_fd + 1, BOA_READ,
						BOA_WRITE, NULL,
						(request_ready || request_block ?
						 &req_timeout : NULL)) == -1) 
				{
				//printf("<%s:%d>errno=%d\n",__FUNCTION__,__LINE__,errno);
				/* what is the appropriate thing to do here on EBADF */
				if (errno == EINTR) {
					//fprintf(stderr,"####%s:%d isFWUPGRADE=%d isREBOOTASP=%d###\n",  __FILE__, __LINE__ ,isFWUPGRADE , isREBOOTASP);
					//fprintf(stderr,"####%s:%d last_req_after_upgrade=%d req_after_upgrade=%d confirm_last_req=%d###\n",  __FILE__, __LINE__ ,last_req_after_upgrade , req_after_upgrade, confirm_last_req);
					if (isFWUPGRADE !=0 && isREBOOTASP == 1 ) {
						if (last_req_after_upgrade == req_after_upgrade)
							confirm_last_req++;
						//printf("<%s:%d>confirm_last_req=%d\n",__FUNCTION__,__LINE__,confirm_last_req);
						if (confirm_last_req >3)
							goto ToUpgrade;
					} else if(isCFGUPGRADE ==2  && isREBOOTASP == 1 ) {
						goto ToReboot;
					}
					else if (isFWUPGRADE ==0 && isREBOOTASP == 1) {
						if (last_req_after_upgrade == req_after_upgrade)
							confirm_last_req++;
						if (confirm_last_req >3) {
							isFWUPGRADE = 0;
							isREBOOTASP = 0;
							//isFAKEREBOOT = 0;
							confirm_last_req=0;
						}
					}
#if defined(CONFIG_APP_FWD)
					{
						extern int isCountDown;
						//printf("<%s:%d>isCountDown=%d\n", __FUNCTION__, __LINE__, isCountDown);
						if (isCountDown == 2) {
							goto ToUpgrade;
						}
					}
#endif
					continue;       /* while(1) */                
				}
				else if (errno != EBADF) {
					DIE("select");
				}
			}//end if select
			/* FIXME: optimize for when select returns 0 (timeout).
			 * Thus avoiding many operations in fdset_update
			 * and others.
			 */
#ifdef BOA_WITH_OPENSSL
			//printf("<%s:%d>do_sock=%d\n",__FUNCTION__,__LINE__,do_sock);
			if(do_sock){
				if (!sigterm_flag && FD_ISSET(server_s, BOA_READ)) {
					pending_requests = 1;
				}
			}
#elif defined(BOA_WITH_MBEDTLS)
			if(!sigterm_flag && FD_ISSET(tls_fd, BOA_READ))
            {
                pending_requests = 1;
            } else if (!sigterm_flag && FD_ISSET(server_s, BOA_READ)) {
                pending_requests = 1;
            }
#else
            {
			if (!sigterm_flag && FD_ISSET(server_s, BOA_READ)) {
				pending_requests = 1;
			}
            }
#endif

#ifdef BOA_WITH_OPENSSL
			//printf("<%s:%d>do_sock=%d, server_ssl=%d\n",__FUNCTION__,__LINE__,do_sock, server_ssl);
			if (do_sock < 2) {
				//printf("<%s:%d>\n",__FUNCTION__,__LINE__);
				if(FD_ISSET(server_ssl, BOA_READ)){ /*If we have the main SSL server socket*/
					//printf("SSL request received!!\n");
					get_ssl_request();
				}   
			}   
#endif /*BOA_WITH_OPENSSL*/

			time(&current_time); /* for "new" requests if we've been in
					      * select too long */
			/* if we skip this section (for example, if max_fd == 0),
			 * then we aren't listening anyway, so we can't accept
			 * new conns.  Don't worry about it.
			 */
		}//if max_fd

		/* reset max_fd */
		max_fd = -1;

		if (request_block) {
			/* move selected req's from request_block to request_ready */
			fdset_update();
		}

		/* any blocked req's move from request_ready to request_block */
		if (pending_requests || request_ready) {
			if (isFWUPGRADE !=0 && isREBOOTASP == 1 ){
				req_after_upgrade++;
			}else if(isFWUPGRADE ==0 && isREBOOTASP == 1){
				req_after_upgrade++;
			}
#ifdef BOA_WITH_MBEDTLS
            if(FD_ISSET(tls_fd, BOA_READ))
            {
                process_requests(((mbedtls_net_context *) &mbedtls_listen_fd)->fd);
            }else
#endif
            {
			process_requests(server_s);
            }
           if((remoteUpgradeInfo.uploadRequest)) 
		   {
		   // remoteChekUpgrade();
		   }
		   //printf("===================%s:%d-----version_%d confirmware_%d-----<\n",__FUNCTION__,__LINE__,remoteUpgradeInfo.checkVersionStatus,remoteUpgradeInfo.upgradeConfirm);
		   if((remoteUpgradeInfo.checkVersionStatus)&&(remoteUpgradeInfo.upgradeConfirm))   
		   {
		    performUpgrade();
            }
			continue;
		}

ToUpgrade:
		printf("ToUpgrade\n");
#ifdef RTK_MESH_CONFIG
		#include <sys/shm.h>
		#include <signal.h>
		#define RTK_CAPWAP_UI_CONFIG_FILE		"/tmp/capwap_config"
		#define MIB_CAPWAP_MODE 		2360
		typedef enum {
			CAPWAP_AUTO_CONFIG_ENABLE = (1<<2),
		} CAPWAP_MODE_T;
		FILE *file = NULL;
		char cmd[100];
		int capwapMode, size=5000000, sleep_count=0, pid=0, val=1;
		apmib_get(MIB_CAPWAP_MODE, (void *)&capwapMode);
		if(capwapMode&CAPWAP_AUTO_CONFIG_ENABLE && sizeof(int)+firmware_len<=size)
		{
			key_t shm_key = 6166529;
			int segment_id = shmget(shm_key, size, IPC_CREAT | S_IRUSR |S_IWUSR);
			char *segment_addr = (char *)shmat(segment_id, NULL, 0);
			memcpy(segment_addr, &firmware_len, sizeof(int));
			memcpy(segment_addr+sizeof(int), firmware_data, firmware_len);
			//printf("boa shared memory attached at address %p (id=%d)\n", segment_addr, segment_id);
			printf("boa write image_len = %d\n", *(int *)segment_addr);
			sprintf(cmd, "echo 1 > %s", RTK_CAPWAP_UI_CONFIG_FILE);
			system(cmd);
			pid = find_pid_by_name("WTP");	
			if(pid > 0){
				kill(pid, SIGUSR2);
				printf("Send SIGUSR2 signal to WTP...\n");
																
				while(val && sleep_count<=30)
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
			shmdt(segment_addr);
			shmctl(segment_id, IPC_RMID, 0);
			printf("<%s>%d: capwapMode=%d count=%d\n",__FUNCTION__,__LINE__,capwapMode,sleep_count);
		}		
#endif
		if (isFWUPGRADE !=0 && isREBOOTASP == 1 ) {
			char buffer[200];
			//fprintf(stderr,"\r\n [%s-%u] FirmwareUpgrade start",__FILE__,__LINE__);
			FirmwareUpgrade(firmware_data, firmware_len, 0, buffer);
			//fprintf(stderr,"\r\n [%s-%u] FirmwareUpgrade end",__FILE__,__LINE__);
			//system("echo 7 > /proc/gpio"); // disable system LED
			isFWUPGRADE=0;
			isREBOOTASP=0;
			//reboot_time = 5;
			break;
		}

ToReboot:
		printf("ToReboot\n");
		if(isCFGUPGRADE == 2 && isREBOOTASP ==1) {
			isCFGUPGRADE=0;
			isREBOOTASP=0;
			system("reboot");
			for(;;);
		}
		
		//printf("<%s:%d>end while\n",__FUNCTION__,__LINE__);
	}//end while
}

/*
 * Name: fdset_update
 *
 * Description: iterate through the blocked requests, checking whether
 * that file descriptor has been set by select.  Update the fd_set to
 * reflect current status.
 *
 * Here, we need to do some things:
 *  - keepalive timeouts simply close
 *    (this is special:: a keepalive timeout is a timeout where
 keepalive is active but nothing has been read yet)
 *  - regular timeouts close + error
 *  - stuff in buffer and fd ready?  write it out
 *  - fd ready for other actions?  do them
 */

static void fdset_update(void)
{
	//printf("%s\n",__FUNCTION__);
	request *current, *next;

	time(&current_time);
	for (current = request_block; current; current = next) {
		time_t time_since = current_time - current->time_last;
		next = current->next;

		/* hmm, what if we are in "the middle" of a request and not
		 * just waiting for a new one... perhaps check to see if anything
		 * has been read via header position, etc... */
		if (current->kacount < ka_max && /* we *are* in a keepalive */
				(time_since >= ka_timeout) && /* ka timeout */
				!current->logline) { /* haven't read anything yet */
			log_error_doc(current);
			fputs("connection timed out\n", stderr);
			current->status = TIMED_OUT; /* connection timed out */
		} else if (time_since > REQUEST_TIMEOUT) {
			log_error_doc(current);
			fputs("connection timed out\n", stderr);
			current->status = TIMED_OUT; /* connection timed out */
		}
		if (current->buffer_end && /* there is data to write */
				current->status < DONE) {
			if (FD_ISSET(current->fd, BOA_WRITE))
				ready_request(current);
			else {
				BOA_FD_SET(current, current->fd, BOA_WRITE);
			}
		} else {
			switch (current->status) {
				case IOSHUFFLE:
#ifndef HAVE_SENDFILE
					if (current->buffer_end - current->buffer_start == 0) {
						if (FD_ISSET(current->data_fd, BOA_READ))
							ready_request(current);
						break;
					}
#endif
				case WRITE:
				case PIPE_WRITE:
					if (FD_ISSET(current->fd, BOA_WRITE))
						ready_request(current);
					else {
						BOA_FD_SET(current, current->fd, BOA_WRITE);
					}
					break;
				case BODY_WRITE:
					// davidhsu ------------------------------
#ifndef NEW_POST
					if (FD_ISSET(current->post_data_fd, BOA_WRITE))
						ready_request(current);
					else {
						BOA_FD_SET(current, current->post_data_fd,
								BOA_WRITE);
					}
#else
#if defined(BOA_CGI_SUPPORT)
					if(current->cgi_type==CGI){
						if (FD_ISSET(current->post_data_fd, BOA_WRITE))
							ready_request(current);
						else {
							BOA_FD_SET(current, current->post_data_fd,
									   BOA_WRITE);
						}
					}else
#endif
					ready_request(current);
#endif
					//--------------------------------------				
					break;
				case PIPE_READ:
					if (FD_ISSET(current->data_fd, BOA_READ))
						ready_request(current);
					else {
						BOA_FD_SET(current, current->data_fd,
								BOA_READ);
					}
					break;
				case DONE:
					if (FD_ISSET(current->fd, BOA_WRITE))
						ready_request(current);
					else {
						BOA_FD_SET(current, current->fd, BOA_WRITE);
					}
					break;
				case TIMED_OUT:
				case DEAD:
					ready_request(current);
					break;
				default:
					if (FD_ISSET(current->fd, BOA_READ))
						ready_request(current);
					else {
						BOA_FD_SET(current, current->fd, BOA_READ);
					}
					break;
			}
		}
		current = next;
	}//end for

#ifdef BOA_WITH_OPENSSL
	//printf("<%s:%d>do_sock=%d\n",__FUNCTION__,__LINE__,do_sock);
	if (do_sock < 2) {
		FD_SET(server_ssl, BOA_READ);
		//printf("Added server_ssl to fdset\n");
	} 	
#elif defined(BOA_WITH_MBEDTLS)
    FD_SET(((mbedtls_net_context *) &mbedtls_listen_fd)->fd, BOA_READ);
#endif
}
