
#  Copyright (C), 2014-2014, C-Data Tech. Co., Ltd.
#  文件名:Makefile
#  作者:
#  版本:1.0.0          
#  日期:
#  描述: 

MODULE_DIR           ?= $(shell pwd)
PRJ_ROOT_DIR         ?= $(shell cd ../..; pwd)

include $(PRJ_ROOT_DIR)/make/Make.config

CFLAGS += -D$(CDT_PRODUCT) -D$(CDT_CUSTOMER) -D_GNU_SOURCE -I$(PRJ_ROOT_DIR)/include -I../../application/include -I../../application/common/include -I$(ZLIB_DIR)/include
all:pon_adaptor_net_if.o pon_adaptor_errno.o pon_adaptor_common_if.o libpon_adaptor.a 

pon_adaptor_net_if.o:pon_adaptor_net_if.c
	$(CC) -o pon_adaptor_net_if.o -c $(CFLAGS) pon_adaptor_net_if.c 
	
pon_adaptor_errno.o:pon_adaptor_errno.c
	$(CC) -o pon_adaptor_errno.o -c $(CFLAGS) pon_adaptor_errno.c 	
	
pon_adaptor_common_if.o:pon_adaptor_common_if.c
	$(CC) -o pon_adaptor_common_if.o -c $(CFLAGS) pon_adaptor_common_if.c
	
libpon_adaptor.a:pon_adaptor_net_if.o pon_adaptor_errno.o pon_adaptor_common_if.o
	$(AR) $(ARFLAGS) libpon_adaptor.a pon_adaptor_net_if.o pon_adaptor_errno.o pon_adaptor_common_if.o
	$(MKDIR) $(PRJ_LIB_DIR)/ponadaptor
	$(CP) libpon_adaptor.a $(PRJ_LIB_DIR)/ponadaptor/
	touch ../pondrv/pon_drv_init.c	
clean:
	-rm -f *.o libpon_adaptor.a
