ifeq ($(wildcard $(DIR_LINUX)/.config),)
ifeq ($(KERNEL_VER),linux-2.6.30)
include ../../linux-2.6.30/.config
else
include ../../linux-3.10/.config
endif
else
include $(DIR_LINUX)/.config
endif

ifeq ($(wildcard $(DIR_USERS)/.config),)
include ../.config
else
include $(DIR_USERS)/.config
endif

#------------------------------------------------------------------------------
.SUFFIXES:
.SUFFIXES: .o .c
.PHONY: clean all depend

LDFLAGS =  -g
CFLAGS = -Os -pipe
DEPEND = ./.depend

#------------------------------------------------------------------------------
APMIB_DIR=$(DIR_USERS)/boa/apmib

LIB_DIR=$(DIR_USERS)/lib/include
UBUS_DIR=$(DIR_USERS)/ubus-2014-09-17
JSONC_DIR=$(DIR_USERS)/lib/include/json-c
MTDAPI_DIR=$(DIR_ROOT)/users/lib/lib/

#------------------------------------------------------------------------------
# EXTRA_CFLAGS
#------------------------------------------------------------------------------
EXTRA_CFLAGS =

#------------------------------------------------------------------------------
# CFLAGS
#------------------------------------------------------------------------------
DEBUG	= -g -Wall
IFLAGS 	= -I. -I$(APMIB_DIR) -I$(JSONC_DIR) -I$(LIB_DIR)  -I$(UBUS_DIR) 
CFLAGS += $(COMMON_CFLAGS) $(EXTRA_CFLAGS)
CFLAGS += -DCONFIG_RTL_92D_SUPPORT

LDFLAGS += -L$(MTDAPI_DIR) -lmtdapi

SOURCES_SER = appServer.c appEventHandler.c cJSON.c appFun.c appUpload.c  ../boa/src/deviceProcIf.c  ../boa/src/version.c ../boa/src/remoteUpgrade.c ../boa/src/utility.c  ../boa/src/md5.c 


OBJS_SER = $(SOURCES_SER:.c=.o)
#------------------------------------------------------------------------------
# targets
#------------------------------------------------------------------------------
all: appServer


appServer: Makefile $(OBJS_SER)
	$(CC) -o $@  $(OBJS_SER) $(CFLAGS) $(LDFLAGS)  -L$(APMIB_DIR) -lapmib -lm
	$(STRIP) $@


clean:
	rm -f $(DEPEND)
	rm -f *.o appServer

depend: $(SOURCES_SER) $(SOURCES_CLI)
	if [ ! -e $(DEPEND) ]; then \
		$(CPP) $(DEBUG) $(CFLAGS) $(IFLAGS) -MM $^ > $(DEPEND); \
	fi
	
-include $(DEPEND)

.c.o:
	${CC} -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<

romfs:
	$(ROMFSINST) appServer /bin/appServer

