/*
 * =====================================================================================
 *
 *       Filename:  channel.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 09时34分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __CHANNEL_H
#define __CHANNEL_H

#include "subscriber.h"
#include "operation.h"
#include <memory>
#include <string>
#include <vector>
#include <deque>
//#include <stdio.h>

using namespace std;

namespace http {
	class Channel {
	      public:
		unsigned int _id;	//通道ID
		 std::vector < shared_ptr < Subscriber > >_subscriber;	//用户链表(只有保持HTTP长连接的客户才需加入通道链表)
		 std::vector < MSG > _msg;	//通道内消息链表
         time_t             _freeStart;
         int                _msgcount;

	      public:
		 Channel();
		~Channel();

		void init();
		void delSubscriber(unsigned int);	//删除指定ID的用户
    

	      public:
		bool operator==(Channel & ch);
		bool operator<(Channel & ch);
	};
}
#endif
