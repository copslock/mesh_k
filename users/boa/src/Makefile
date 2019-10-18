# $Id: Makefile.in,v 1.59.2.15 2005/02/22 03:02:40 jnelson Exp $

ifeq ($(wildcard $(DIR_LINUX)/.config),)
include ../../../linux-2.6.30/.config
else
include $(DIR_LINUX)/.config
endif

ifeq ($(wildcard $(DIR_USERS)/.config),)
include ../../.config
else
include $(DIR_USERS)/.config
endif

ifdef CONFIG_RTL_8198_AP_ROOT
CONFIG_RTL_AP = CONFIG_RTL_8198_AP_ROOT
endif

ifdef CONFIG_RTL_8197D_AP
CONFIG_RTL_AP = CONFIG_RTL_8197D_AP
endif

ifdef CONFIG_RTL_AP_PACKAGE 
CONFIG_RTL_AP = CONFIG_RTL_AP_PACKAGE
endif

ifdef CONFIG_RTL_AP
AP=1
SKUTAG=ap
else
GW=1
SKUTAG=gw
endif

#------------------------------------------------------------------------------
#ifndef COMMON_CFLAGS
#$(error COMMON_CFLAGS is not defined)
#endif

#ifndef APMIB_SHARED
#$(error APMIB_SHARED is not defined)
#endif

#ifndef APMIB_LIB
#$(error APMIB_LIB is not defined)
#endif

#------------------------------------------------------------------------------
.SUFFIXES:
.SUFFIXES: .o .c
.PHONY: clean mrclean distclean all dist depend

# The following gcc warning switches are too noisy to be useful checking
# Boa for lint:
#  -Wtraditional -Wconversion -Wredundant-decls -Wunreachable-code
# The following gcc warning switches should generate no warnings:
GCC_FLAGS = -Wstrict-prototypes -Wpointer-arith -Wcast-align -Wcast-qual \
  -Wshadow -Waggregate-return -Wmissing-prototypes -Wnested-externs \
  -Wall -W -Wno-unused -Winline -Wwrite-strings -Wundef -pedantic


#srcdir = .
#VPATH = ./../extras
LDFLAGS =  -g
#LIBS = 
CFLAGS = -Os -pipe
CPPFLAGS =  -I. -I.
DEPEND = ./.depend

#------------------------------------------------------------------------------
APMIB_DIR=../apmib
MTDAPI_DIR=$(DIR_ROOT)/users/lib/lib

#------------------------------------------------------------------------------
# EXTRA_CFLAGS
#------------------------------------------------------------------------------
EXTRA_CFLAGS =

#------------------------------------------------------------------------------
# Files
#------------------------------------------------------------------------------
REALTEK_FILES = fmtcpip.c fmfwall.c fmlinuxvlan.c utility.c fmget.c fmwlan.c fmmgmt.c version.c
KING_SIGNAL_FILES = deviceProcIf.c remoteUpgrade.c
ifeq ($(CONFIG_APP_RTK_INBAND_CTL), y)
SHOW_WAN_STATUS_FILES = showwanstatus.c utility.c 
SHOW_WAN_STATUS_OBJS = $(SHOW_WAN_STATUS_FILES:.c=.o)
endif

ifeq ($(GW),1)
REALTEK_FILES += fmddns.c fmroute.c fmipv6.c fmbt.c fmpocketap.c
endif
ifeq ($(CONFIG_APP_OPENVPN),y)
REALTEK_FILES += fmopenvpn.c
endif

ifeq ($(CONFIG_HTTP_FILE_SERVER_SUPPORT),y)
REALTEK_FILES += http_files.c
#REALTEK_FILES += fmhttp_files.c
REALTEK_FILES += http_file_dir.c
endif

ifeq ($(CONFIG_RTL_HTTP_REDIRECT), y)
REALTEK_FILES	+= fmredirect.c 
endif

FLAGS_FOR_DOMAIN_QUERY=0

ifeq ($(CONFIG_DOMAIN_NAME_QUERY_SUPPORT),y)
FLAGS_FOR_DOMAIN_QUERY=1
endif

ifeq ($(CONFIG_RTL_P2P_SUPPORT),y)
FLAGS_FOR_DOMAIN_QUERY=1
endif

ifeq ($(CONFIG_RTL_ULINKER),y)
FLAGS_FOR_DOMAIN_QUERY=1
REALTEK_FILES += fmulinker.c
endif

ifeq ($(FLAGS_FOR_DOMAIN_QUERY),1)
REALTEK_FILES += fmdomain_query.c
endif 

ifeq ($(CONFIG_APP_TR069),y)
REALTEK_FILES += fmtr069.c
endif

ifeq ($(CONFIG_IPV6_CE_ROUTER_SUPPORT),y)
REALTEK_FILES += sha1.c
endif

ifeq ($(CONFIG_APP_RTK_VOIP),y)
include $(VOIP_APP)/web/voip_var.mak
REALTEK_FILES += $(VOIP_FILES)
CFLAGS += $(VOIP_CFLAGS) -DCONFIG_APP_RTK_VOIP
LDFLAGS += $(VOIP_LDFLAGS)
CFLAGS += -DCONFIG_APP_BOA
endif

ifeq ($(CONFIG_RTL_8812AR_VN_SUPPORT),y)
        COMMON_CFLAGS += -DCONFIG_RTL_8812AR_VN_SUPPORT
endif

ifeq ($(CONFIG_RTL_ETH_802DOT1X_SUPPORT),y)
REALTEK_FILES += fmethdot1x.c
endif

#ifeq ($(CONFIG_APP_SAMBA_WEB),y)
REALTEK_FILES += fmdiskmanage.c
#endif

ifeq ($(CONFIG_APP_MULTI_AP),y)
REALTEK_FILES += fmmultiap.c
endif

ifeq ($(CONFIG_DIGEST_AUTHENTICATION),y)
#REALTEK_FILES += md5.c
endif
REALTEK_FILES += md5.c

REALTEK_FILES += cJSON.c

ifeq ($(CONFIG_ISP_IGMPPROXY_MULTIWAN),y)
CFLAGS += -DCONFIG_ISP_IGMPPROXY_MULTIWAN
endif

#------------------------------------------------------------------------------
# CFLAGS
#------------------------------------------------------------------------------
DEBUG	= -g -Wall
IFLAGS 	= -I. -I$(APMIB_DIR)
CFLAGS += $(COMMON_CFLAGS) $(EXTRA_CFLAGS)
#include $(DIR_USERS)/.config
ifdef	CONFIG_APP_MULTIPPPOE
		CFLAGS += -D MULTI_PPPOE
endif

ifdef	CONFIG_APP_BOA_IPV6_SUPPORT
CFLAGS +=-DINET6
endif

ifeq ($(CONFIG_APP_OPENSSL-0.9.8I), y)
OPENSSL_DIR=$(DIR_ROOT)/users/openssl-0.9.8i
endif
ifeq ($(CONFIG_APP_OPENSSL-0.9.8Y), y)
OPENSSL_DIR=$(DIR_ROOT)/users/openssl-0.9.8y
endif
ifeq ($(CONFIG_APP_OPENSSL-1.0.2A), y)
OPENSSL_DIR=$(DIR_ROOT)/users/openssl-1.0.2a
endif
ifeq ($(CONFIG_APP_OPENSSL-1.0.2D), y)
OPENSSL_DIR=$(DIR_ROOT)/users/openssl-1.0.2d
endif
ifeq  ($(CONFIG_APP_BOA_WITH_OPENSSL), y)
SSL_PATH = $(OPENSSL_DIR)
CFLAGS += -DBOA_WITH_OPENSSL -DBOA_WITH_SSL -I$(SSL_PATH)/include/
SSL_LIBS = -lssl -lcrypto
endif
ifeq ($(CONFIG_APP_BOA_WITH_MBEDTLS), y)
SSL_PATH = ./../../mbedtls-2.2.1
CFLAGS += -DBOA_WITH_MBEDTLS -DBOA_WITH_SSL -I$(SSL_PATH)/include/
SSL_LIBS = -L$(SSL_PATH)/library/ -lmbedcrypto -lmbedtls -lmbedx509
endif

ifeq ($(CONFIG_MTD_NAME_MAPPING),y)
LDFLAGS += -L$(MTDAPI_DIR) -lmtdapi
endif

ifdef CONFIG_SYS_DIAGNOSTIC
	CFLAGS += -D SYS_DIAGNOSTIC
endif

SOURCES = alias.c boa.c buffer.c cgi.c cgi_header.c config.c escape.c \
	get.c hash.c ip.c log.c mmap_cache.c pipe.c queue.c range.c \
	read.c request.c response.c signals.c util.c sublog.c \
	select.c \
	asp_page.c \
        $(KING_SIGNAL_FILES)\
	$(REALTEK_FILES)
ifeq ($(CONFIG_APP_MULTI_USERS),y)
SOURCES += auth.c 
endif

OBJS = $(SOURCES:.c=.o) timestamp.o
ifeq ($(CONFIG_HTTP_FILE_SERVER_SUPPORT),y)
OBJS += fmhttp_files.o
endif

LDFLAGS += -lm
#------------------------------------------------------------------------------
# targets
#------------------------------------------------------------------------------
ifeq ($(CONFIG_APP_RTK_INBAND_CTL), y)
all: fileserver depend boa showwanstatus
else
all: fileserver depend boa
endif

#boa:	$(OBJS) 
#	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) 
#	
#boa_indexer:	index_dir.o escape.o   
#	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

ifeq ($(CONFIG_APP_RTK_INBAND_CTL), y)
showwanstatus:Makefile $(SHOW_WAN_STATUS_OBJS)
	$(CC) -o $@ $(SHOW_WAN_STATUS_OBJS) $(LDFLAGS) -L$(APMIB_DIR) -lapmib
	$(STRIP) $@
endif

boa: Makefile $(OBJS) $(APMIB_DIR)/$(APMIB_LIB)
ifeq ($(CONFIG_APP_BOA_WITH_OPENSSL), y)
	$(CC) -o boa $(OBJS) $(LDFLAGS) -L$(APMIB_DIR) -lapmib -L $(SSL_PATH) $(SSL_LIBS) -ldl
else
ifeq ($(CONFIG_APP_BOA_WITH_MBEDTLS), y)
	$(CC) -o boa $(OBJS) $(LDFLAGS) -L$(APMIB_DIR) -lapmib -L $(SSL_PATH) $(SSL_LIBS) -ldl
else
	$(CC) -o boa $(OBJS) $(LDFLAGS) -L$(APMIB_DIR) -lapmib
endif
endif
	$(STRIP) $@

clean:
	rm -f $(DEPEND)
	rm -f $(OBJS) boa core *~ boa_indexer
ifeq ($(CONFIG_APP_RTK_INBAND_CTL), y)
	rm -f showwanstatus showwanstatus.o
endif
	
distclean:	mrclean

mrclean:	clean
	rm -f config.status config.cache config.h Makefile config.log

# timestamp

timestamp.o:	$(SOURCES)

#------------------------------------------------------------------------------
# depend stuff
#------------------------------------------------------------------------------
#depend: $(SOURCES)
#	$(CPP) $(CPPFLAGS) -MM $^ select.c poll.c access.c > $(DEPEND)

fileserver:
ifeq ($(CONFIG_HTTP_FILE_SERVER_SUPPORT),y)
ifeq ($(CONFIG_RTL_8197F),y)
	if [ ! -e fmhttp_files.o ]; then \
		cp fmhttp_files.obj fmhttp_files.o; \
	fi
endif
endif
depend: $(SOURCES)
	if [ ! -e $(DEPEND) ]; then \
		$(CPP) $(DEBUG) $(CFLAGS) $(IFLAGS) -MM $^ > $(DEPEND); \
	fi
	
-include $(DEPEND)


# tags
tags:	$(SOURCES)
	ctags -o tags $^ *.h

release:
	./mksvnver DIR_USERS=$(DIR_USERS) DIR_LINUX=$(DIR_LINUX)


# object dump
boa.objdump:    boa
	objdump --disassemble-all --source boa > $@

.c.o:
	${CC} -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<
