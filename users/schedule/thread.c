
/*
Copyright (c) 2019, All rights reserved.

File         : thread.c
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


#include "thread.h"
#include<stdio.h>
#include<time.h>
#include<sys/time.h>
#define NULL 0

struct thread
{
    const char *name;
    int fd;
    void *data;
    void (*function)(void *);
    struct thread *next;
};
 
 
static struct thread *s_threadListeningHead = NULL;
static struct thread *s_threadPollingHead = NULL;
static int s_threadExit = 0;

threadHandle_t threadAddListeningFile(const char *name, int fd, void *data, void (*function)(void *))
{
    struct thread *p;
    
    p = (struct thread *)malloc(sizeof(struct thread));
    if (p == NULL)
    {
        printf( "malloc(%d)", sizeof(struct thread));
        return NULL;
    }

    p->name = name;
    p->fd = fd;
    p->data = data;
    p->function = function;

    p->next = s_threadListeningHead;
    s_threadListeningHead = p;
    
  //  printf( "Listening thread(%s,fd=%d) registered", p->name, p->fd);

    return p;
}


void threadRemoveListeningFile(threadHandle_t handle)
{
    struct thread *p, *prev = NULL;
    p = s_threadListeningHead;
    while(p)
    {        
        if (p == handle)
        {
            if (prev)
            {
                prev->next = p->next;
            }
            else 
            {
                s_threadListeningHead = p->next;
            }

            printf( "Remove Listening thread(%s,fd=%d)", p->name, p->fd);

            free(p); 
            return ;
        }
        else 
        {
            prev = p;
            p = p->next;
        }
    }
}


threadHandle_t threadAddPollingFunction(const char *name, void *data, void (*function)(void *))
{
    struct thread *p;
    
    p = (struct thread *)malloc(sizeof(struct thread));
    if (p == NULL)
    {
        printf( "malloc(%d)", sizeof(struct thread));
        return NULL;
    }

    p->name = name;
    p->fd = 0;
    p->data = data;
    p->function = function;

    p->next = s_threadPollingHead;
    s_threadPollingHead = p;
    
    //printf( "Polling thread(%s) registered\n", p->name);

    return p;
}


void threadRemovePollingFunction(threadHandle_t handle)
{
    struct thread *p, *prev = NULL;
    p = s_threadPollingHead;
    while(p)
    {        
        if (p == handle)
        {
            if (prev)
            {
                prev->next = p->next;
            }
            else 
            {
                s_threadPollingHead = p->next;
            }

            printf( "Remove Polling thread(%s)", p->name);

            free(p); 
            return ;
        }
        else 
        {
            prev = p;
            p = p->next;
        }
    }
}

void threadSchedule(int slot)
{
    struct thread *thread, *next;
    struct timeval tv;
    fd_set fds;
    int maxfd, ret;
         
    while(!s_threadExit)
    {
        tv.tv_sec = slot / 1000;
        tv.tv_usec = (slot % 1000) * 1000;

        maxfd = -1;
         
        FD_ZERO(&fds);
 
        thread = s_threadListeningHead;
        while(thread)
        {
            if (thread->fd >= 0)
            {
                FD_SET(thread->fd, &fds);
                if (thread->fd > maxfd)
                {
                    maxfd = thread->fd;
                }
            }
            thread = thread->next;
        }

        ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if (ret > 0) 
        {         
            thread = s_threadListeningHead;
            while(thread)
            {    
                  printf( "call s_threadListeningHead function\n");
                next = thread->next;
                if (FD_ISSET(thread->fd, &fds) && thread->function)
                {
                    thread->function(thread->data);
                }
                thread = next;
            }
        }

        thread = s_threadPollingHead;
        while(thread)
        {
   
            next = thread->next;
            if (thread->function)
            {
 //              printf( "call s_threadPollingHead function\n");
                thread->function(thread->data);
            }
            thread = next;
        }
        
//        printf( "goes next(s_threadExit=%d)\n", s_threadExit);
    }  
}

void threadFreeAll(void)
{
    struct thread *p, *next = NULL;

    p = s_threadPollingHead;
    while(p)
    {        
        next = p->next;
        printf( "Remove Polling thread(%s)", p->name);
        free(p); 
        p = next;    
    }    
    s_threadPollingHead = NULL;

    p = s_threadListeningHead;
    while(p)
    {        
        next = p->next;
        printf( "Remove Listening thread(%s,fd=%d)", p->name, p->fd);
        free(p); 
        p = next;    
    }    
    s_threadListeningHead = NULL;

}

void threadExit(void)
{
    s_threadExit = 1;
}

