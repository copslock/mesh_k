

/*
Copyright (c) 2019, All rights reserved.

File         : task.c
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


#include "task.h"

static struct taskList *s_taskHead = NULL;

#define taskStateStandby(task) (task->state == TASK_STATE_STANDBY)
#define taskStateFailed(task) (task->state == TASK_STATE_FAILED)
#define taskStateDone(task) ((task->state == TASK_STATE_FAILED) || (task->state == TASK_STATE_SUCCESS))


const char *taskTargetString(taskTarget_t *target)
{
    static char buffer[128];
    
    switch(target->type)
    {
        case TASK_ADDR_ETHER:
            return etherAddrToString(&target->ethAddr, 0);
            
        case TASK_ADDR_IPV4:
            return inet_ntoa(target->ipAddr);
            
        case TASK_ADDR_INTEGER:
            sprintf(buffer, "0X%08lX", target->index);
            return buffer;
            
        case TASK_ADDR_OCTET:
            if (target->data)
            {
                #define DIGITS_HEX 		"0123456789ABCDEF"                
                int i, c;
                for (i = 0, c = 0; (i < target->size) && (c < (sizeof(buffer)/sizeof(buffer[0])) - 1); i ++)
                {
                    if (isprint(target->data[i]))
                    {
                        buffer[c ++] = target->data[i];
                    }
                    else if (c < (sizeof(buffer)/sizeof(buffer[0])) - 3)
                    {
                        buffer[c ++] = '%';
				        buffer[c ++] = DIGITS_HEX [(target->data[i] >> 4) & 0x0F];
				        buffer[c ++] = DIGITS_HEX [(target->data[i] >> 0) & 0x0F];
                    }
                }
                buffer[c] = '\0';                
            }
            else 
            {
                buffer[0] = '\0';
            }
            return buffer;
        default:
            break;
    }

    return "Invalid Target Type";
}


static int taskTargetMatch(taskTarget_t *t1, taskTarget_t *t2)
{
    if (t1->type != t2->type)
    {
        return 0;
    }

    switch(t1->type)
    {
        case TASK_ADDR_ETHER:
            if (!memcmp(&t1->ethAddr, &t2->ethAddr, sizeof(t1->ethAddr)))
            {
                return 1;
            }
            return 0;
            
        case TASK_ADDR_IPV4:
            return (t1->ipAddr.s_addr == t2->ipAddr.s_addr) ? 1 : 0;
            
        case TASK_ADDR_INTEGER:
            return (t1->index == t2->index) ? 1 : 0;
            
        case TASK_ADDR_OCTET:
            if (!t1->size || !t2->size || (t1->size != t2->size))
            {
                return 0;
            }
            if (!memcmp(t1->data, t2->data, t1->size))
            {
                return 1;
            }
            return 0;
        default:
            break;            
    }    

    return 0;
}

void taskFreeAll(void)
{
    struct taskList *list, *listNext = NULL;
    struct task *task;
    
    list = s_taskHead;
    while(list)
    {
        struct task *next = NULL;
        task = list->head;        
        while(task)
        {
            next = task->next;
            
            printf( " Remove task(%s-%s)", taskTargetString(&task->target), task->name);                
            free(task);
            
            task = next;
        }

        listNext = list->next;

        printf( "Remove task list(%s)", taskTargetString(&list->target)); 
        free(list);

        list = listNext;        
    }

    s_taskHead = NULL;
}


void taskPolling(void *data)
{
    struct taskList *list, *prev = NULL;
    struct task *task;
    
    list = s_taskHead;
    while(list)
    {
        struct task *t_prev = NULL;
        task = list->head;        
        while(task)
        {
            if (taskStateStandby(task)
                || (taskStateFailed(task) && task->retry))
            {
                if (taskStateFailed(task) && task->retry)
                {
                    task->retry --;
                }
                
                if (task->start)
                {
                    printf( "Task(%s-%s) start", taskTargetString(&task->target), task->name);
                    task->start(task);
                }
            }
            else if (taskStateDone(task))
            {
                if (t_prev)
                {
                    t_prev->next = task->next;
                }
                else 
                {
                    list->head = task->next;
                }
                
                printf( "Task(%s-%s) done, remove", taskTargetString(&task->target), task->name);  

                if (task->freeOnExit)
                {
                    task->freeOnExit(task);
                }
                
                free(task);
                
                task = t_prev ? t_prev->next : list->head;
                continue;
            }
            
            t_prev = task;
            task = task->next;
        }
        
        if (list->head == NULL)
        {
            if (prev)
            {
                prev->next = list->next;
            }
            else 
            {
                s_taskHead = list->next;
            }

            printf( "Task list(%s) empty, remove", taskTargetString(&list->target)); 

            free(list);
            list = prev ? prev->next : s_taskHead;

            continue;
        }
        prev = list;
        list = list->next;
    }
}

static struct taskList *taskGet(taskTarget_t *target)
{
    struct taskList *list = s_taskHead;
    
    while(list)
    {
        if (taskTargetMatch(&list->target, target))
        {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

/*
    
*/
int taskPush(struct task *task)
{
    struct taskList *list;

    if (task == NULL)
    {
        return 0;
    }
    
    list = taskGet(&task->target);
    
    if (list == NULL)
    {
        list = (struct taskList *)malloc(sizeof(struct taskList));
        
        if (list == NULL)
        {
            printf( "malloc(%d)", sizeof(struct taskList));
            free(task);            
            return 0;
        }
        
        memset(list, 0, sizeof(*list));
        memcpy(&list->target, &task->target, sizeof(list->target));
        // enqueue
        list->next = s_taskHead;
        s_taskHead = list;
        
    }
    else 
    {
        // yes , we get a list head using the same address.
        // check if a similar task is already running
        struct task *p = list->head;
        while(p)
        {
            if ((p->start == task->start)
                && ((p->state == TASK_STATE_STANDBY) || (p->state == TASK_STATE_RUNNING)))
            {                
                printf( "Task(%s-%s) already queued", taskTargetString(&task->target), p->name);                
                free(task);
                return 0;
            }
            p = p->next;
        }       
    }
    
    task->next = list->head;
    list->head = task;
    
    printf( "Push task : %s-%s", taskTargetString(&task->target), task->name);

    return 1;    
}


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
    )
{
    task->name = name;
    task->state = TASK_STATE_STANDBY;
    task->error = 0;
    task->retry = retry;

    task->target.type = TASK_ADDR_NONE;
    
    switch(addrType)
    {
        case TASK_ADDR_ETHER:
            task->target.type = addrType;
            memcpy(&task->target.ethAddr, addrData, sizeof(task->target.ethAddr));
            break;
            
        case TASK_ADDR_IPV4:
            task->target.type = addrType;
            memcpy(&task->target.ipAddr, addrData, sizeof(task->target.ipAddr));
            break;
            
        case TASK_ADDR_INTEGER:
            task->target.type = addrType;
            memcpy(&task->target.index, addrData, sizeof(task->target.index));
            break;
            
        case TASK_ADDR_OCTET:
            task->target.type = addrType;
            memcpy(&task->target.data, addrData, addrSize);
            task->target.size = addrSize;
            break;            
    }
    
    task->start = startFunction;
    task->freeOnExit = freeOnExitFunction;
    task->callback = callback;
    task->data = data;
    task->next = NULL;
}

void taskInit
    (
        struct task *task, 
        const char *name, 
        int addrType, const void *addrData, int addrSize,
        int retry, 
        taskStart_t startFunction,
        taskCallback_t callback, 
        void *data
    )
{
    return taskInit2(task, name, addrType, addrData, addrSize, retry, startFunction, NULL, callback, data);
}


const char *taskErrorString(int error)
{
    const char *str[] = {
        "Success",
        "Failed",
        "Internal error",
        "Memory allocation failed",
        "Access system IO failed",
        "MME status error",
        "Timeout"
    };

    if (error < sizeof(str)/sizeof(str[0])){
        return str[error];
    }
    
    return "Unknown";
}

