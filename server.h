/*
 * =====================================================================================
 *
 *       Filename:  server.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 09时30分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __SERVER_H
#define __SERVER_H

#include"channel.h"		//包含通道结构体
#include"operation.h"
#include <memory>
#include <list>
#include <vector>

using namespace std;

namespace http {

	class Server {
	      public:
		    unsigned int _clientNum;	//客户数量
		    std::vector < shared_ptr < Channel > >_channel;	//所有的通道

	      public:
		 Server();
		~Server();

		void init();	//服务器初始化函数

	      public:
		void pub_handler(struct evhttp_request *req, void *arg);    //管理员推送消息,pool用户能够立刻接受到消息
		void broadcast_handler(struct evhttp_request *req, void *arg);    //将消息广播到所有通道
		void clear_handler(struct evhttp_request *req, void *arg);    //清空指定通道内的所有消息
		void sub_handler(struct evhttp_request *req, void *arg);    //用户订阅消息，如果没有消息就立刻断开连接
		void read_handler(struct evhttp_request *req, void *arg);    //读取文件数据回调函数
		void pool_handler(struct evhttp_request *req, void *arg);   //用户订阅消息，如果没有消息就保持HTTP连接
		void generic_handler(struct evhttp_request *req, void *arg);   //通用回调函数

	};
}
#endif
