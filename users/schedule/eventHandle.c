
/*
Copyright (c) 2019, All rights reserved.

File         : timer.c
Status       : Current
Description  : 

Author       : haopeng
Contact      : 376915244@qq.com

Revision     : 2019-08 
Description  : Primary released

## Please log your description here for your modication ##

Revision     : 
Modifier     : 
Description  : 

*/

#include <stdio.h>     
#include <time.h>       /* time_t, struct tm, time, localtime */
#include "apmib.h"
#define	TIMER_CONTINUE	0
#define	TIMER_REMOVE	1
#define WEEK_NUM        7
#define WEEK_TIME_DISABLED 7
#define  SUNDAY 	 0
#define  MONDAY 	 1
#define  TUESDAY	 2
#define  WEDNESDAY	 3
#define  THURSDAY	 4
#define  FRIDAY 	 5
#define  SATDAY 	 6
#define  PARENT_CONTRL_SET_COMMAND    "sysconf firewall addParentControl %d"
#define  PARENT_CONTRL_DELETE_COMMAND "sysconf firewall deleteParentControl %d"





char* convertTimeToString(const struct tm* timeptr)
{  
static const char weekName[][4] = {    
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"  
	};  
static char result[40]={0};  
sprintf(result, "%d-%.2d-%.2d %.2d:%.2d:%.2d",    
	1900+timeptr->tm_year,    
	timeptr->tm_mon+1,    
	timeptr->tm_mday,    
	timeptr->tm_hour,    
	timeptr->tm_min,
	timeptr->tm_sec);  
return result;
}

/*
struct tm:
tm_sec :econds after the minute range 0-59
tm_min :minutes after the hour  range 0-59
tm_hour:hours since midnight    range 0-23
tm_mday:day of the month        range 1-31
tm_mon: months since January    range 0-11
tm_year:years since 1900
tm_wday: days since Sunday      range 0-6 ,0=sunday 1=Mon 2=Tues....
tm_yday days since January 1    range 0-365
*/
#if 0
int getCurrentTime(struct tm timeinfo)
{
    time_t rawtime;  
	struct tm* timeinfo;
	time (&rawtime);  
	timeinfo = localtime(&rawtime);  
	printf("current local time: %s\n", convertTimeToString(timeinfo)); 
	//strcpy(date,convertTimeToString(timeinfo));
	//return date;
	return 0;
}
#endif

int parentContrlList()
{
	int nBytesSent=0, parentEntryNum, i,j;
	PARENT_CONTRL_T entry;
	int curentTime;
	char commandBuf[64]={0};

	time_t rawtime;  
	struct tm* currentTimeInfo;
	time (&rawtime);  
	currentTimeInfo = localtime(&rawtime); 
	//printf("current local time: %s\n", convertTimeToString(currentTimeInfo)); 

	if ( !apmib_get(MIB_PARENT_CONTRL_TBL_NUM, (void *)&parentEntryNum)) {
  		fprintf(stderr, "Get table entry error!\n");
		return -1;
	}
    
	for (i=1; i<=parentEntryNum; i++) 
	{

		*((char *)&entry) = (char)i;

		//	memset(&entry, 0x00, sizeof(entry));
		if ( !apmib_get(MIB_PARENT_CONTRL_TBL, (void *)&entry))
			return -1;
		//printf("------>function_%s_line[%d]: terminalNUm=%d\n",__FUNCTION__,__LINE__,parentEntryNum);

		//printf("\n(---+++++table--%d)tmpMon=%d tmpTues=%d  tmpWed=%d  tmpThur=%d  tmpFri=%d tmpSat=%d  tmpSun=%d  tmpstart=%d  tmpend=%d terminal=%s\n", \
		i,entry.parentContrlWeekMon,entry.parentContrlWeekTues,entry.parentContrlWeekWed,\
		entry.parentContrlWeekThur,entry.parentContrlWeekFri,entry.parentContrlWeekSat, \
		entry.parentContrlWeekSun, entry.parentContrlStartTime,entry.parentContrlEndTime,entry.parentContrlTerminal);
		//getCurrentTime(currentTimeInfo);
		curentTime=(currentTimeInfo->tm_hour*60+currentTimeInfo->tm_min);
		if((currentTimeInfo->tm_wday==(entry.parentContrlWeekMon?MONDAY:WEEK_TIME_DISABLED)) \
		||(currentTimeInfo->tm_wday==(entry.parentContrlWeekTues?TUESDAY:WEEK_TIME_DISABLED)) \
		||(currentTimeInfo->tm_wday==(entry.parentContrlWeekWed?WEDNESDAY:WEEK_TIME_DISABLED)) \
		||(currentTimeInfo->tm_wday==(entry.parentContrlWeekThur?THURSDAY:WEEK_TIME_DISABLED)) \
		||(currentTimeInfo->tm_wday==(entry.parentContrlWeekFri?FRIDAY:WEEK_TIME_DISABLED)) \
		||(currentTimeInfo->tm_wday==(entry.parentContrlWeekSat?SATDAY:WEEK_TIME_DISABLED)) \
		||(currentTimeInfo->tm_wday==(entry.parentContrlWeekSun?SUNDAY:WEEK_TIME_DISABLED))) \
		{
		 	//printf("------>function_%s_line[%d]: parent week is ok \n",__FUNCTION__,__LINE__);
		 if((curentTime>=entry.parentContrlStartTime)&&(curentTime<(entry.parentContrlStartTime+1)))
		 {
		  memset(commandBuf,0,sizeof(commandBuf));
		  sprintf(commandBuf,PARENT_CONTRL_SET_COMMAND,i);
          system(commandBuf);
		 }
		 else if((curentTime>=entry.parentContrlEndTime)&&(curentTime<(entry.parentContrlEndTime+1)))
		 {	 
		    memset(commandBuf,0,sizeof(commandBuf));
		 	sprintf(commandBuf,PARENT_CONTRL_DELETE_COMMAND,i);
		 	system(commandBuf);
		 }
		}
	}
	return TIMER_CONTINUE;
}


int parentContrl(void *data, int reason )
{
    int intVal=0;
    apmib_get(MIB_PARENT_CONTRL_ENABLED,  (void *)&intVal);
	if(intVal==1){
     parentContrlList();
	}
	 
	 return TIMER_CONTINUE;  
}


