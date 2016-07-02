/*
 * =====================================================================================
 *
 *       Filename:  subscriber.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 10时16分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __SUBSCRIBER_H
#define __SUBSCRIBER_H

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <event.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace http {
	class Subscriber {
	      public:
		unsigned int _id;	//用户ID 
		struct evhttp_request *_req;	//连接的http结构体
        time_t          _freeStart;

	      public:
		 Subscriber();
		~Subscriber();

	      public:
		 bool operator==(Subscriber &);
		bool operator<(Subscriber &);
	};
}
#endif
