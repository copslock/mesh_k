/*
Copyright (c) 2019, All rights reserved.

File         : trace.h
Status       : Current
Description  : 

Author       : lhw
Contact      : xxhanwen@163.com

Revision     : 2019-08 
Description  : Primary released
    
## Please log your description here for your modication ##

Revision     : 
Modifier     : 
Description  : 

*/

#ifndef    _TRACE_H_
#define    _TRACE_H_

#define DTRACE_ON     0x01
#define DTRACE_OFF    0x00

#define ENABLE_DTRACE

#ifdef ENABLE_DTRACE
#define DTRACE(obj, fmt, ...)    do{\
    if(obj & DTRACE_ON)\
    {\
        printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);\
    }\
    }while(0)
#else
#define DTRACE(obj, fmt, ...)
#endif

#define DPrintf(fmt, ...)    printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

///////////trace obj define here///////
#define DTRACE_GUEST_WLAN        DTRACE_OFF
#define DTRACE_TIMER_CHECK       DTRACE_OFF


///////////////////////////////////

#endif
