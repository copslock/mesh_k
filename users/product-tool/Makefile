ifeq ($(wildcard $(DIR_USERS)/.config),)
include ../.config
else
include $(DIR_USERS)/.config
endif

COMMON_CFLAGS =

ifeq ($(CONFIG_APP_MULTI_AP),y)
COMMON_CFLAGS += -DRTK_MULTI_AP
COMMON_CFLAGS += -DUNIVERSAL_REPEATER
COMMON_CFLAGS += -DWIFI_SIMPLE_CONFIG
endif
COMMON_CFLAGS += -DCONFIG_RTL_8197F

CC = msdk-linux-gcc

APMIB_DIR = ../boa/apmib

CFLAGS := -s -Wall -Os
LDFLAGS := -s -Wall
IFLAGS := -I. -I$(APMIB_DIR)

TARGET = product_tool
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)


all: $(TARGET)


$(TARGET): $(OBJS) version.o
	$(CC) $(LDFLAGS) $(COMMON_CFLAGS) -L$(APMIB_DIR) -lapmib -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(IFLAGS) $(COMMON_CFLAGS) -c -o $@ $<

version.o: 	../boa/src/version.c
	$(CC) $(CFLAGS) $(IFLAGS) $(COMMON_CFLAGS) -c -o $@ $<
	
romfs:
	$(ROMFSINST) $(TARGET) /bin/$(TARGET)
	
.PHONY: clean

clean:
	rm -f *o
	rm -f $(TARGET)