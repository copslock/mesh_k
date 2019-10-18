
/*
Copyright (c) 2019, All rights reserved.

File         : guestWlan.c
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
     
#include <ctype.h>

#include "guestWlan.h"
#include "apmib.h"
#include "trace.h"

#define	TIMER_CONTINUE	0
#define	TIMER_REMOVE	1

int g_guest_time_cnt_min;
int g_reboot_flg = 0;

void InitGuestWlan()
{

    apmib_init();

    g_reboot_flg = 1;
    g_guest_time_cnt_min = 0;
}

//call every 1 min
int doGuestWlanCtrl(void *data, int reason )
{ 
    unsigned int active_time=0;
    int set_flg;
    int disable_flg_2g;
    int disable_flg_5g;
    int old_wlan_idx;
    int old_vwlan_idx;

    DTRACE(DTRACE_GUEST_WLAN, "do guest wlan ctrl.\n");
    
    old_wlan_idx = wlan_idx;
    old_vwlan_idx = vwlan_idx;
    vwlan_idx = 2;
    wlan_idx = 0;
    if (apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&disable_flg_5g) == 0)
    {
        DPrintf("get guest wlan5g disable err\n");
    }
    wlan_idx = 1;
    if (apmib_get(MIB_WLAN_WLAN_DISABLED, (void *)&disable_flg_2g) == 0)
    {
        DPrintf("get guest wlan2g disable err\n");
    }
    
    if (apmib_get(MIB_GUEST_NEWORK_VALID_TIME, (void *)&active_time) == 0)
    {
        DPrintf("get guest wlan active time err\n");
    }
     
    if (apmib_get(MIB_GUEST_NEWORK_SET_FLG, (void *)&set_flg) == 0)
    {
        DPrintf("get guest net reset flg err\n");
    }
    
    if(set_flg)
    {
        DTRACE(DTRACE_GUEST_WLAN, "guest wlan set up.\n");
        set_flg = 0;
        g_guest_time_cnt_min = 0;
        if (apmib_set(MIB_GUEST_NEWORK_SET_FLG, (void *)&set_flg) == 0)
    	{
    		DPrintf("set guest net reset flg err\n");
    	}
    }

    if(!disable_flg_5g || !disable_flg_2g)
    {
        g_guest_time_cnt_min++;

        if(active_time != 0)
        {
            if(g_guest_time_cnt_min >= active_time*60 || g_reboot_flg)
            {
                disable_flg_5g = 1;
                disable_flg_2g = 1;
                wlan_idx = 0;
                if (apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&disable_flg_5g) == 0)
            	{
            		DPrintf("set guest wlan5g disable err\n");
            	}
                wlan_idx = 1;
                if (apmib_set(MIB_WLAN_WLAN_DISABLED, (void *)&disable_flg_2g) == 0)
            	{
            		DPrintf("set guest wlan2g disable err\n");
            	}
                system("ifconfig wlan0-va1 down");
                system("ifconfig wlan1-va1 down");
                apmib_update(CURRENT_SETTING);
                DTRACE(DTRACE_GUEST_WLAN, "guest wlan over, close wlan1-va1.");
            }
        }
    }
    g_reboot_flg = 0;

    wlan_idx = old_wlan_idx;
    vwlan_idx = old_vwlan_idx;
    return TIMER_CONTINUE;  
}

