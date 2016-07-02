LIBS=-lm -levent -lpthread -lboost_system -lmysqlclient -lboost_timer -lboost_chrono \
	 -I /usr/local/log4cplus/include/ -L /usr/local/log4cplus/lib/ -llog4cplus
CXXFLAGS=-Wall -O2 -g -std=c++11
OBJS=main.o server.o channel.o subscriber.o operation.o log.o  error.o
CC=g++
TARGETS=cometServer
$(TARGETS):$(OBJS)
	$(CC) $(LIBS) $(CXXFLAGS) $(OBJS) -o $@
main.o:main.cpp log.h operation.h server.h daemon.h
server.o:server.cpp server.h channel.h subscriber.h log.h operation.h
channel.o:channel.cpp channel.h server.h subscriber.h log.h operation.h
subscriber.o:subscriber.cpp subscriber.h channel.h operation.h
operation.o:operation.cpp server.h log.h
log.o:log.cpp log.h
error.o:error.cpp
.PHONY=clean
clean:
	rm -rf $(OBJS) $(TARGETS)

