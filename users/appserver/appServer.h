/*

File         : appServer.h
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
/* define exit() codes if not provided */
#ifndef EXIT_FAILURE		
#define EXIT_FAILURE               1
#endif
#ifndef EXIT_SUCCESS		
#define EXIT_SUCCESS               0
#endif

#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif


#define FALSE                      0
#define TRUE                       1
#define LISTEN_PORT                5000
#define MAX_CLIENT                 400



/*use for listen() func*/
#define PENDING_CONNECTION_NUM     3   

/*use for struct timeval,time unit is second*/
#define TIME_INTERVAL              60

extern int app_event_handler(int fd,char *buffer);




