
/*
Copyright (c) 2019, All rights reserved.

File         : etherAddr.h
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


#ifndef __ETHERADDR_H__
#define __ETHERADDR_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ctype.h>
#include <net/ethernet.h>

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif 

typedef struct etherAddr 
{
    unsigned char octet[ETHER_ADDR_LEN];
}etherAddr_t;


/*
 type 0: match any of belows
 type 1: match 001122334455
 type 2: match 00:11:22:33:44:55
 type 3: match 00-11-22-33-44-55
 type 4: match 0011:2233:4455
 type 5: match 0011-2233-4455
 type 6: match 001122:334455
 type 7: match 001122-334455
*/

enum{
    ETHER_TYPE_DEFAULT = 0,
    ETHER_TYPE_NO_SEPARTOR,
    ETHER_TYPE_ONE_COLON,
    ETHER_TYPE_ONE_DASH, 
    ETHER_TYPE_TWO_COLON,
    ETHER_TYPE_TWO_DASH, 
    ETHER_TYPE_FIVE_COLON,
    ETHER_TYPE_FIVE_DASH    
};

#define ETHER_ADDR_TYPE_DEFAULT  ETHER_TYPE_ONE_DASH 


int stringToEtherAddr(const char *string, etherAddr_t *ether, int type);
char *etherAddrToString(etherAddr_t *ether, int type);



/* 
 Test the addr if corret format
*/

#define multicastEtherAddr(eth) ((eth)->octet[0] & 0x01)
#define BroadcastEtherAddr(eth) (((eth)->octet[0] == 0xff) \
                                && ((eth)->octet[1] == 0xff) \
                                && ((eth)->octet[2] == 0xff) \
                                && ((eth)->octet[3] == 0xff) \
                                && ((eth)->octet[4] == 0xff) \
                                && ((eth)->octet[5] == 0xff))

#define zeroEtherAddr(eth) (((eth)->octet[0] == 0x00) \
                                && ((eth)->octet[1] == 0x00) \
                                && ((eth)->octet[2] == 0x00) \
                                && ((eth)->octet[3] == 0x00) \
                                && ((eth)->octet[4] == 0x00) \
                                && ((eth)->octet[5] == 0x00))

#define unicastEtherAddr(eth) (!BroadcastEtherAddr(eth) \
                                 && !zeroEtherAddr(eth) \
                                 && !multicastEtherAddr(eth)) 


extern const etherAddr_t g_broadcastAddr;


#endif /* #ifndef __ETHERADDR_H__ */

