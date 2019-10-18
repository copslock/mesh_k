
/*
Copyright (c) 2019, All rights reserved.

File         : task.h
Status       : Current
Description  : 

Author       : haopeng
Contact      : 376915244@qq.com

Revision     : 2019-07 
Description  : Primary released

## Please log your description here for your modication ##

Revision     : 
Modifier     : 
Description  : 

*/



#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "etherAddr.h"

typedef void (*taskCallback_t)(void *data);
typedef int (*taskStart_t)(void *data);
typedef void (*taskFreeOnExit_t)(void *data);


#define TASK_TARGET_TYPE_STRING_SIZE 256

/*
    Task Object Type
*/
typedef enum
{
    TASK_ADDR_NONE = 0,
    TASK_ADDR_ETHER = 1,
    TASK_ADDR_IPV4 = 2,
    TASK_ADDR_INTEGER = 3,
    TASK_ADDR_OCTET = 4
}taskObjectType_t;


/*
    Task control state
*/
typedef enum
{
    TASK_STATE_STANDBY = 0,
    TASK_STATE_RUNNING = 1,
    TASK_STATE_SUCCESS = 2,
    TASK_STATE_FAILED = 3,
}taskState_t;

/*
    Task error state
*/
typedef enum
{
    TASK_OK = 0,
    TASK_FAILED = 1,
    TASK_ERR_INTERNAL = 2,
    TASK_ERR_NOMEM = 3,
    TASK_ERR_SYSIO = 4,
    TASK_ERR_MSTATUS = 5,
    TASK_ERR_TIMEOUT = 6,
    TASK_ERR_START = 7,
    TASK_ERR_DATA = 8,
}taskError_t;


typedef struct 
{
    taskObjectType_t type;
    union
    {
        unsigned long index;
        etherAddr_t ethAddr;
        struct in_addr ipAddr;
        
        struct 
        {
            unsigned long size;
            unsigned char data[128];
        };
    };    
}taskTarget_t;

struct task
{
    const char *name;
    
    taskState_t state;
    taskError_t error;
    
    int retry;

    taskTarget_t target;
    
    taskCallback_t callback;
    taskStart_t start;
    taskFreeOnExit_t freeOnExit; /* a function to free memory allocated when task push */   
    void *data;
    struct task *next;
};


struct taskList
{
    taskTarget_t target;
    struct task *head;
    struct taskList *next;
};


#define taskNew(_task) \
do { \
    _task = (typeof(_task))malloc(sizeof(*(_task))); \
    if (_task != NULL) \
    { \
        memset(_task, 0, sizeof(*(_task))); \
    } \
    else { \
        debugf("malloc", "malloc(%d)", sizeof(*(_task)));\
        return NULL; \
    } \
}while(0)

/*
    push task
    enqueue a task
    input task will be free if push failed.
    return 0 failed
    return 1 success
*/
int taskPush(struct task *task);

void taskFreeAll(void);
void taskPolling(void *data);

/* 
    init a task struct
*/

void taskInit2
    (
        struct task *task, 
        const char *name, 
        int addrType, const void *addrData, int addrSize,
        int retry, 
        taskStart_t startFunction,
        taskFreeOnExit_t freeOnExitFunction,
        taskCallback_t callback, 
        void *data
    );

void taskInit
    (
        struct task *task, 
        const char *name, 
        int addrType, const void *addrData, int addrSize,
        int retry, 
        taskStart_t startFunction,
        taskCallback_t callback, 
        void *data
    );

const char *taskErrorString(int error);

const char *taskTargetString(taskTarget_t *target);


#endif /* __TASK_H__ */

