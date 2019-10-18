
/*
Copyright (c) 2019, All rights reserved.

File         : etherAddr.c
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
     
#include <ctype.h>

#include "etherAddr.h"

const etherAddr_t g_broadcastAddr = {.octet = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

int stringToEtherAddr(const char *string, etherAddr_t *ether, int type)
{
    int i = 0;

#define tohex(c) ((c <= '9') ? c - '0' : toupper(c) - 'A' + 10)

    // type  ETHER_TYPE_NO_SEPARTOR   
    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 

    i ++;
    
    if (!string[i] || !isxdigit(string[i]))
    {
        return 0;
    }

    i ++; 

    if(ether)
    { 
        ether->octet[0] = tohex(string[i - 2]) << 4 | tohex(string[i - 1]); 
    } 
    
    if (type == ETHER_TYPE_FIVE_COLON)
    { 
        if (string[i] != ':')
        {
            return 0;
        }
        i ++;
    }
    else if (type == ETHER_TYPE_FIVE_DASH)
    {  
        if (string[i] != '-'){ 
            return 0;
        } 
        i ++;
    }
    else if (type == ETHER_TYPE_DEFAULT)
    { 
        if (string[i] == ':')
        { 
            type = ETHER_TYPE_FIVE_COLON;
            i ++;
        }
        else if (string[i] == '-') 
        {
            type = ETHER_TYPE_FIVE_DASH;
            i ++;
        }
    }
        
    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 

    i ++;    

    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 

    i ++; 

    if(ether)
    { 
        ether->octet[1] = tohex(string[i - 2]) << 4 | tohex(string[i - 1]); 
    } 

    if ((type == ETHER_TYPE_FIVE_COLON) || (type == ETHER_TYPE_TWO_COLON))
    { 
        if (string[i] != ':')
        {
            return 0;
        }
        i ++;
    }
    else if ((type == ETHER_TYPE_FIVE_DASH) || (type == ETHER_TYPE_TWO_DASH))
    {  
        if (string[i] != '-')
        { 
            return 0;
        } 
        i ++;
    }
    else if (type == ETHER_TYPE_DEFAULT)
    { 
        if (string[i] == ':')
        { 
            type = ETHER_TYPE_TWO_COLON;
            i ++;
        } 
        else if (string[i] == '-') 
        {
            type = ETHER_TYPE_TWO_DASH;
            i ++;
        }
    }   
    
    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 

    i ++;

    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 

    i ++;

    if(ether)
    { 
        ether->octet[2] = tohex(string[i - 2]) << 4 | tohex(string[i - 1]); 
    } 
    
    if ((type == ETHER_TYPE_FIVE_COLON) || (type == ETHER_TYPE_ONE_COLON))
    { 
        if (string[i] != ':')
        {
            return 0;
        }
        i ++;
    }
    else if ((type == ETHER_TYPE_FIVE_DASH) || (type == ETHER_TYPE_ONE_DASH))
    {  
        if (string[i] != '-')
        { 
            return 0;
        } 
        i ++;
    }
    else if (type == ETHER_TYPE_DEFAULT)
    { 
        if (string[i] == ':')
        { 
            type = ETHER_TYPE_ONE_COLON;
            i ++;
        } 
        else if (string[i] == '-') 
        {
            type = ETHER_TYPE_ONE_DASH;
            i ++;
        }
    }   
    
    if (!string[i] || !isxdigit(string[i]))
    {
        return 0;
    }

    i ++;

    if (!string[i] || !isxdigit(string[i]))
    {
        return 0;
    }

    i ++; 

    if(ether)
    {
        ether->octet[3] = tohex(string[i - 2]) << 4 | tohex(string[i - 1]); 
    } 
    
    if ((type == ETHER_TYPE_FIVE_COLON) || (type == ETHER_TYPE_TWO_COLON))
    { 
        if (string[i] != ':')
        {
            return 0;
        }
        i ++;
    }
    else if ((type == ETHER_TYPE_FIVE_DASH) || (type == ETHER_TYPE_TWO_DASH))
    {  
        if (string[i] != '-')
        { 
            return 0;
        } 
        i ++;
    } 
    
    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 
    
    i ++;
    
    if (!string[i] || !isxdigit(string[i]))
    { 
        return 0;
    } 

    i ++;  

    if(ether)
    {
        ether->octet[4] = tohex(string[i - 2]) << 4 | tohex(string[i - 1]); 
    } 
    
    if (type == ETHER_TYPE_FIVE_COLON)
    { 
        if (string[i] != ':')
        {
            return 0;
        }
        i ++;
    }
    else if (type == ETHER_TYPE_FIVE_DASH)
    {  
        if (string[i] != '-')
        { 
            return 0;
        } 
        i ++;
    }  
    
    if (!string[i] || !isxdigit(string[i]))
    {
        return 0;
    } 

    i ++;
    
    if (!string[i] || !isxdigit(string[i]))
    {
        return 0;
    } 

    i ++;   
    
    if(ether)
    {
        ether->octet[5] = tohex(string[i - 2]) << 4 | tohex(string[i - 1]); 
    } 

    if (string[i])
    {
        return 0;
    }

    return 1;
}

char *etherAddrToString(etherAddr_t *ether, int type)
{
    static char buffer[8][64];
    static int buffer_index = 0;

    if (buffer_index >= sizeof(buffer)/sizeof(buffer[0]) - 1)
    {
        buffer_index = 0;
    }
    else 
    {
        buffer_index ++;
    }

    if (type == ETHER_TYPE_DEFAULT)
    {
        type = ETHER_ADDR_TYPE_DEFAULT;
    }
    
    switch(type)
    {
        case ETHER_TYPE_NO_SEPARTOR:
            sprintf(buffer[buffer_index], "%02X%02X%02X%02X%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);
            break;
        case ETHER_TYPE_ONE_COLON:
            sprintf(buffer[buffer_index], "%02X%02X%02X:%02X%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;
        case ETHER_TYPE_ONE_DASH:
            sprintf(buffer[buffer_index], "%02X%02X%02X-%02X%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;            
        case ETHER_TYPE_TWO_COLON:
            sprintf(buffer[buffer_index], "%02X%02X:%02X%02X:%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;            
        case ETHER_TYPE_TWO_DASH:
            sprintf(buffer[buffer_index], "%02X%02X-%02X%02X-%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;            
        case ETHER_TYPE_FIVE_COLON:
            sprintf(buffer[buffer_index], "%02X:%02X:%02X:%02X:%02X:%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;            
        case ETHER_TYPE_FIVE_DASH:
            sprintf(buffer[buffer_index], "%02X%02X%02X%02X%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;
        default:
            sprintf(buffer[buffer_index], "%02X%02X%02X%02X%02X%02X", 
                ether->octet[0], ether->octet[1], ether->octet[2],
                ether->octet[3], ether->octet[4], ether->octet[5]);  
            break;            
    }
    
    return buffer[buffer_index];
}



