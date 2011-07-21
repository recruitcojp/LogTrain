#########################
#コマンド定義
#########################
CC=/usr/bin/gcc
ARC=/usr/ccs/bin/ar
RM=/bin/rm
CP=/bin/cp

#########################
#フラグ
#########################
CFLAGS=-c -g -I/usr/include -w
LFLAGS=

#########################
#ファイル定義
#########################
OBJS1=common.o tcp.o std.o cache.o wrap.o LogTrainClient.o
OBJS2=common.o tcp.o std.o cache.o wrap.o LogTrainRetry.o
SRCS1=$(OBJS1:.o=.c)
SRCS2=$(OBJS2:.o=.c)
LTARGET1=./LogTrainClient
LTARGET2=./LogTrainRetry
TARGET1=/usr/local/logtrain/bin/LogTrainClient
TARGET2=/usr/local/logtrain/bin/LogTrainRetry

####################################################################
all:$(LTARGET1) $(LTARGET2)

.c.o: 
	$(CC) $(CFLAGS) $*.c

$(LTARGET1): $(OBJS1)
	$(CC) -o $(LTARGET1) $(OBJS1) $(LFLAGS)

$(LTARGET2): $(OBJS2)
	$(CC) -o $(LTARGET2) $(OBJS2) $(LFLAGS)

install:
	$(CP) -p $(LTARGET1) $(TARGET1)
	$(CP) -p $(LTARGET2) $(TARGET2)

clean:
	$(RM) -f *.o $(LTARGET1) $(LTARGET2) core

