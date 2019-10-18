
/*
Copyright (c) 2019, All rights reserved.

File         : guestWlan.h
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


#ifndef __GUEST_WLAN_H__
#define __GUEST_WLAN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ctype.h>
#include <net/ethernet.h>


extern void InitGuestWlan();
extern int doGuestWlanCtrl(void *data, int reason);

#endif /* #ifndef __GUEST_WLAN_H__ */

