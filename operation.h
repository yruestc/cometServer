/*
 * =====================================================================================
 *
 *       Filename:  operation.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 10时07分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __OPERATION_H
#define __OPERATION_H

#include <boost/property_tree/ptree.hpp>     //解析info格式的配置文件所需的头文件
#include <boost/property_tree/info_parser.hpp>
#include <boost/timer/timer.hpp>
#include <log4cplus/logger.h>           //使用log4cplus日志库所需的头文件
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/ndc.h>
#include <iostream>
#include <vector>
#include <string>
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
#include <event2/listener.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"

//extern CONFIGURE _conf;


namespace http {

	typedef struct {	//===================维护每个线程的结构
		pthread_t thread_tid;
		int no;		//数组中的下标
		struct event_base *base;
        struct event *timer_event;  //定时器
		struct event *sigint_event;     //中断信号处理事件
		struct event *sigterm_event;    //终止信号处理事件
		struct evhttp *adminhttp;
		struct evhttp *fronthttp;
		int count;	//线程连接次数，用来调试使用
	} THREAD_LIBEVENT;

	typedef struct {	//维护主线程的结构
		pthread_t thread_tid;
		struct event_base *base;
        struct event *timer_event;  //定时器
		struct event *sigint_event;
		struct event *sigterm_event;
	} DISPATCH_THREAD;

	enum ADDRATE {
		SUB = 1024,
		CHAN = 1024
	};

	enum SETUP {
		OPE_START,	//开始
		OPE_STOP,	//结束
		OPE_RESTART	//重启
	};

	typedef struct {
		unsigned int seq;	//消息序列号
		time_t time;	//消息创建时间
		 std::string content;	//消息实体
	} MSG;

	typedef struct {
		std::string _pidfile;	//保存进程ID文件
		struct log {
			std::string file;	//日志文件名
			std::string level_name;	//日志级别名字
			long rotate_size;	//日志轮替大小
		};
		log _log;	//日志结构

		struct admin {
			std::string ip;	//后端服务器IP
			unsigned int port;	//后端服务器端口
		};
		admin _admin;	//后端服务器结构

		struct front {
			std::string ip;	//前端服务器IP
			unsigned int port;	//前端服务器端口
		};
		front _front;	//前端服务器结构

		struct timeout {
			int channel;	//通道超时值
			int subscriber;	//用户超时值
			int polling;	//轮询超时值
			int message;	//消息报文存活时间
		};
		timeout _timeout;	//超时结构

		struct max_per_channel {
			int message;	//单通道最大消息数量
			int subscriber;	//单通道最大用户数量
		};
		max_per_channel _max_per_channel;	//单通道结构

		int _max_channel;	//最大通道数量

		 std::vector < std::string > _admin_allow_ip;	//允许管理员登陆的IP地址

		int _backlog;	//监听描述符数量

		int _nthreads;	//创建线程数量

		std::string _generic_file;	//文件存放路径

	} CONFIGURE;

	class operation {
	      public:
        static void initlog4cplus();    //初始化日志组件的配置

		static void cleanup(struct evhttp_connection *evcon, void *arg);	//断开连接回调函数

		static void startinfo(std::string cmd);

		static std::vector < std::string > split(const std::string & s,
							 const std::
							 string & seperator);
		static void parserinfo(std::string file);	//解析配置文件

		static void sigint_cb(evutil_socket_t, short, void *);	//中断信号处理回调函数

		static void sigterm_cb(evutil_socket_t, short, void *);	//终止信号处理回调函数
		
        static void timer_cb(evutil_socket_t, short, void *);	//定时器处理回调函数

		static void sub_handler(struct evhttp_request *req, void *arg);	//订阅事件触发时的回调函数
        
		static void read_handler(struct evhttp_request *req, void *arg);	//获取文件内容订阅函数
	
        static void pool_handler(struct evhttp_request *req, void *arg);//订阅事件触发时的回调函数

		static void pub_handler(struct evhttp_request *req, void *arg);     //将消息推送至服务器，再由服务器推送至指定通道
		
        static void broadcast_handler(struct evhttp_request *req, void *arg);   //将消息广播至服务器，再由服务器广播至所有通道

        static void clear_handler(struct evhttp_request *req, void *arg);   //将消息广播至服务器，再由服务器广播至所有通道

		static void httpserver_GenericHandler(struct evhttp_request *req, void *arg);	//通用请求回调函数

		static void run();	//HTTP Server 主循环

		static unsigned int read_pid(std::string);	//读取保存进程ID的文件

		static bool write_pid(std::string);	//将进程ID写入cometServer.pid文件

		static bool file_exists(std::string);	//判断文件是否存在

		static void kill_process();	//杀死进程

		static void remove_file(std::string);	//从文件系统中删除此文件

		static void versioninfo();      //显示版本信息

		static void *httpserver_Dispatch(void *arg);

		static int http_server_bind_socket(std::string ip, int port,    //绑定在指定的套接字上监听
						   int backlog);

		static int tcp_listen(const char *host, const char *serv,   //在指定主机名和服务名上监听
				      socklen_t * addrlenp, int backlog);

		static void read_file(char *filedata);	//读取文件

        static void exe_sql(const char *sql);   //执行写入数据库操作
        
        static void query_sql(const char *sql);     //查询并显示结果
        
        static void comet_query_sql(const char* sql,std::vector<std::string> &result_store);    //只查询一个字段content
        
        static void comet_query_seqmax_sql(int channel,int &seqmax);    //获取指定通道内的消息序列最大值

        static void record_time(boost::timer::cpu_timer& t);    //记录程序的运行时间,用于衡量服务器的性能
	};
}
#endif
