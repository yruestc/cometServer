/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 10时24分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include"operation.h"
#include"server.h"
#include"daemon.h"
//#include"daemonize.hpp"
#include<memory>
#include<iostream>

//using namespace http;

//#define MAX_PATH    256
static const int MAX_PATH =256;

extern void err_sys(const char *fmt, ...);
extern void err_quit(const char *fmt, ...);
extern void err_msg(const char *fmt, ...);
//extern void InitDaemon();

http::THREAD_LIBEVENT * tptr;	//定义一个线程数组
http::DISPATCH_THREAD dispatcher_thread;	//主线程结构

pthread_mutex_t _global_mutex = PTHREAD_MUTEX_INITIALIZER;	//初始化互斥锁

shared_ptr < http::Server > server;	//声明一个服务器智能指针
http::CONFIGURE _conf;
http::SETUP _setup;		//启动方式
bool _verbose = false;		//是否显示详细信息
bool _daemon = false;		//是否以守护进程方式运行
std::pair < unsigned int, unsigned int >discdata;
boost::timer::cpu_timer server_run_timer;	//记录服务器运行时间

log4cplus::Logger _rootlog = log4cplus::Logger::getRoot();
log4cplus::Logger _logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("log"));      //实例化日志记录器对象

int main(int argc, char **argv)
{

	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "dvs:")) != EOF) {
		switch (c) {
		case 'd':
			_daemon = true;	//以守护进程的方式运行
			http::operation::startinfo(argv[0]);	//显示程序开始运行
			break;
		case 'v':
			_verbose = true;	//显示更过详细信息
			break;
		case 's':	//后接操作行为
			if (strcmp("start", optarg) == 0)	//启动
				_setup = http::OPE_START;
			else if (strcmp("stop", optarg) == 0)	//停止
				_setup = http::OPE_STOP;
			else if (strcmp("restart", optarg) == 0)	//重启
				_setup = http::OPE_RESTART;
			else {
				err_quit("usuage: -s (start | stop | restart)");
			}
			break;
		default:
			_setup = http::OPE_START;
			break;

		}

	}
	if ((argc - 1) != optind)	//最后一个参数接配置文件
	{
		err_quit("Lack of _configure_file");
	}

	char *configure_file;
	if ((configure_file = strrchr(argv[optind], '/')) == NULL)	//获取配置文件
	{
		if ((configure_file = (char *)malloc(MAX_PATH)) == NULL)
			err_sys("malloc error for configure_file");
		strcpy(configure_file, argv[optind]);
	} else
		configure_file++;

	http::operation::parserinfo(configure_file);	//解析配置文件

	signal(SIGPIPE, SIG_IGN);	//建立信号处理机制
	if (_daemon)
		daemonize(argv[0]);	//以守护进程方式运行

	http::operation::run();	//运行服务器程序

	http::operation::record_time(server_run_timer);	//记录服务器的运行时间(只是增加这个功能，实际程序绝对不会运行到这个地方)
	exit(0);

}
