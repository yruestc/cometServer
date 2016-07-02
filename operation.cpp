/*
 * =====================================================================================
 *
 *       Filename:  operation.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 16时23分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include<strings.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include"operation.h"
#include"server.h"
#include<boost/lexical_cast.hpp>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<mysql/mysql.h>
#include<iostream>
#include<memory>

//#include"daemonize.hpp"

/*
#define HOST	"localhost"
#define USER	"allen"
#define PASSWORD	""
#define DATABASE	"cometserver"
*/

//using namespace log4cplus;
//using namespace log4cplus::helpers;
//using namespace log4cplus::thread;

//连接指定的mysql服务器
static const char *HOST = "localhost";
static const char *USER = "allen";
static const char *PASSWORD = "123456";
static const char *DATABASE = "cometserver";

//extern shared_ptr < http::Server > server;    //声明一个服务器只能指针
extern void err_sys(const char *fmt, ...);
extern void err_quit(const char *fmt, ...);
extern void err_msg(const char *fmt, ...);

extern http::THREAD_LIBEVENT * tptr;	//定义一个线程数组
extern http::DISPATCH_THREAD dispatcher_thread;	//主线程结构
extern pthread_mutex_t _global_mutex;

extern shared_ptr < http::Server > server;	//声明一个服务器智能指针
extern http::CONFIGURE _conf;
extern http::SETUP _setup;	//启动方式
extern bool _verbose;		//是否显示详细信息
extern bool _daemon;		//是否以守护进程方式运行
extern std::pair < unsigned int, unsigned int >discdata;
extern boost::timer::cpu_timer server_run_timer;	//记录服务器运行时间

extern log4cplus::Logger _rootlog;
extern log4cplus::Logger _logger;

namespace http {
    void operation::initlog4cplus()    //初始化日志组件的配置
    {
        log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(false);   //屏蔽输出信息中的调试信息
	    //_rootlog = log4cplus::Logger::getRoot();
        //_logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("log"));      //实例化日志记录器对象

        try{
            log4cplus::ConfigureAndWatchThread confthread("urconfig.properties",5*1000);    //使用线程监控脚本的更新
        }catch(std::exception &e){
	        LOG4CPLUS_FATAL(_logger, "This is a FATAL message: "<<e.what()<<std::endl );
        }
    }

	void operation::cleanup(struct evhttp_connection *evcon, void *arg)	//断开连接回调函数
	{
		auto data = (std::pair < unsigned int, unsigned int >*)arg;
		if (_verbose)
			std::cout << "subscriber(" << data->first <<
			    ") disconnect from channel(" << data->second << ")"
			    << std::endl;
		//server->_channel[data->second]->delSubscriber(data->first); //从通道中删除该用户
		//log_info("subscriber(%d) disconnect from channel(%d) !",data->first, data->second);
	}
	void operation::startinfo(string cmd)	//显示程序启动信息
	{
		std::cout << cmd << " runnning as daemon" << std::endl;
		std::cout << "Copyrih=ght (c) 2015 allen" << std::endl;
		std::cout << "email: 1135628277@qq.com" << std::endl;
	}

	std::vector < std::string > operation::split(const std::string & s,
						     const std::string &
						     seperator) {
		std::vector < std::string > result;
		typedef std::string::size_type string_size;

		string_size i = 0;

		while (i != s.size()) {

			//找到字符串中首个不等于分隔符的字母；    
			int flag = 0;
			while (i != s.size() && flag == 0) {
				flag = 1;
				for (string_size x = 0; x < seperator.size(); ++x)	//如果当前下标所对元素与分隔符相等，继续下一次while循环
					if (s[i] == seperator[x]) {
						++i;
						flag = 0;
						break;
					}
			}
			//找到又一个分隔符，将两个分隔符之间的字符串取出；    
			flag = 0;
			string_size j = i;
			while (j != s.size() && flag == 0) {
				for (string_size x = 0;
				     x < seperator.size(); ++x)
					if (s[j] == seperator[x]) {	//如果当前下标所对元素与分隔符相等，跳出循环
						flag = 1;
						break;
					}
				if (flag == 0)	//如果与分隔符不匹配，下标后移
					++j;
			}

			if (i != j) {
				result.push_back(s.substr(i, j - i));	//将分隔符之间的字符串提取出来,存在容器里
				i = j;
			}
		}

		return result;

	}

	void operation::parserinfo(std::string file)	//解析配置文件
	{
		using namespace boost::property_tree;
		try {
			ptree pt;
			read_info(file, pt);	//解析配置文件

			_conf._pidfile =
			    pt.get < std::string > ("conf.pidfile");

			auto child = pt.get_child("conf.log");
			_conf._log.file = child.get < std::string > ("file");	//日志文件名
			_conf._log.level_name = child.get < std::string > ("level_name");	//日志级别名
			_conf._log.rotate_size = child.get < long >("rotate_size");	//日志轮替大小

			child = pt.get_child("conf.admin");
			_conf._admin.ip = child.get < std::string > ("ip");	//服务器访问IP
			_conf._admin.port = child.get < unsigned int >("port");	//服务器访问端口

			child = pt.get_child("conf.front");
			_conf._front.ip = child.get < std::string > ("ip");	//前端访问IP
			_conf._front.port = child.get < unsigned int >("port");	//前端访问端口

			child = pt.get_child("conf.timeout");
			_conf._timeout.channel = child.get < int >("channel");	//通道超时时间
			_conf._timeout.subscriber = child.get < int >("subscriber");	//用户超时时间
			_conf._timeout.polling = child.get < int >("polling");	//轮询时间
			_conf._timeout.message = child.get < int >("message");	//消息报文存活时间

			child = pt.get_child("conf.max_per_channel");
			_conf._max_per_channel.message = child.get < int >("message");	//单通道最大消息数量
			_conf._max_per_channel.subscriber = child.get < int >("subscriber");	//单通道最大用户数量

			_conf._max_channel = pt.get < int >("conf.max_channel");	//获取最大通道数

			std::string str = pt.get < std::string > ("conf.admin_allow_ip");	//获得的是未解析的字符串
			_conf._admin_allow_ip = split(str, "/");	//将分词结果保存在容器中

			_conf._backlog = pt.get < int >("conf.backlog");	//获取监听描述符数量
			_conf._nthreads = pt.get < int >("conf.nthreads");	//获取初始化创建线程数量
			_conf._generic_file = pt.get < std::string > ("conf.generic_file");	//获取通用文件数量

		} catch(info_parser_error & e) {
			std::cout << "Exception: " << e.what() << std::endl;
		} catch(std::exception & e) {
			std::cout << "Exception: " << e.what() << std::endl;
		}
	}

	void operation::sigint_cb(evutil_socket_t, short, void *)	//中断信号处理回调函数
	{
		if (_verbose)
			std::cout << "reveive SIGINT ..." << std::endl;
		log_info("reveive SIGINT ...");
		event_base_free(dispatcher_thread.base);
		for (int i = 0; i < _conf._nthreads; i++) {
			if (_verbose)
				std::cout << "thread " << tptr[i].thread_tid <<
				    " used: " << tptr[i].count << std::endl;
			log_info("thread[%d] run %d times", tptr[i].thread_tid,
				 tptr[i].count);
			if (tptr[i].base != NULL)
				event_base_free(tptr[i].base);
			if (tptr[i].timer_event != NULL)
				event_free(tptr[i].timer_event);
			if (tptr[i].sigint_event != NULL)
				event_free(tptr[i].sigint_event);	//释放线程数组事件
			if (tptr[i].sigterm_event != NULL)
				event_free(tptr[i].sigterm_event);	//释放线程数组事件
			if (tptr[i].adminhttp != NULL)
				evhttp_free(tptr[i].adminhttp);	//释放线程数组事件
			if (tptr[i].fronthttp != NULL)
				evhttp_free(tptr[i].fronthttp);	//释放线程数组事件
		}
		record_time(server_run_timer);
		exit(0);
	}

	void operation::sigterm_cb(evutil_socket_t, short, void *)	//终止信号处理回调函数
	{
		if (_verbose)
			std::cout << "reveive SIGTERM ..." << std::endl;
		log_info("reveive SIGTERM ...");
		event_base_free(dispatcher_thread.base);
		for (int i = 0; i < _conf._nthreads; i++) {
			if (_verbose)
				std::cout << "thread " << tptr[i].thread_tid <<
				    " used: " << tptr[i].count << std::endl;
			log_info("thread[%d] run %d times", tptr[i].thread_tid,
				 tptr[i].count);
			if (tptr[i].base != NULL)
				event_base_free(tptr[i].base);
			if (tptr[i].timer_event != NULL)
				event_free(tptr[i].timer_event);
			if (tptr[i].sigint_event != NULL)
				event_free(tptr[i].sigint_event);	//释放线程数组事件
			if (tptr[i].sigterm_event != NULL)
				event_free(tptr[i].sigterm_event);	//释放线程数组事件
			if (tptr[i].adminhttp != NULL)
				evhttp_free(tptr[i].adminhttp);	//释放线程数组事件
			if (tptr[i].fronthttp != NULL)
				evhttp_free(tptr[i].fronthttp);	//释放线程数组事件
		}
		record_time(server_run_timer);
		exit(0);
	}

	void operation::timer_cb(evutil_socket_t, short, void *)	//定时器事件
	{

		if (_conf._nthreads > 1) {
			if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
			{
				log_error
				    ("pthread_mutex_lock error for global_mutex");
				err_sys
				    ("pthread_mutex_lock error for global_mutex");
			}
		}

		if (_verbose)
			std::cout << "check free channel and subscriber..." <<
			    std::endl;

		//检查通道是否超时 
		time_t tm;
		time(&tm);
		struct evbuffer *buf;	//创建一个缓冲区

		if ((buf = evbuffer_new()) == nullptr) {
			log_error("evbuffer_new error !");	//将错误信息记录日志
			err_sys("evbuffer_new error !");	//终止程序运行,输出错误消息
		}

		auto it_ch = server->_channel.begin();
		for (; it_ch != server->_channel.end();)	//遍历服务器中所有通道
		{
			//检查通道内是否有消息，如果有就推送消息，然后断开连接
			if (!(*it_ch)->_msg.empty())	//通道内有消息
			{
			      for (auto x:(*it_ch)->_subscriber)
					//遍历所有用户
				{
				      for (auto & y:(*it_ch)->_msg)
					{
						evbuffer_add_printf(buf,
								    "{type: \"welcome\", content: \"%s\"}\n",
								    y.content.
								    c_str());
						evhttp_send_reply_chunk(x->_req, buf);	//块传输
					}
					evhttp_send_reply_end(x->_req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
				}

				(*it_ch)->_subscriber.clear();	//清空通道内的用户
			}

			if ((tm - (*it_ch)->_freeStart) >= _conf._timeout.channel)	//通道超时，断开每个用户的连接(不删除通道)
			{

			      for (auto & x:(*it_ch)->_subscriber)
					//遍历所有用户
				{
					evbuffer_add_printf(buf,
							    "{type: \"welcome\", content: \"channel was destroy\"}\n");
					evhttp_send_reply_chunk(x->_req, buf);	//块传输
					evhttp_send_reply_end(x->_req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
					discdata = {
					x->_id, (*it_ch)->_id};
					evhttp_connection_set_closecb(x->_req->evcon, operation::cleanup, &discdata);	//设置关闭连接的回调函数
				}

				(*it_ch)->_subscriber.clear();	//清空通道

				//it_ch=server->_channel.erase(it_ch);  不能删除通道，通道是固定的
				//continue;
			} else {	//检查用户是否超时
				auto it_sub = (*it_ch)->_subscriber.begin();
				for (; it_sub != (*it_ch)->_subscriber.end();)	//遍历所有用户
				{
					if ((tm - (*it_sub)->_freeStart) >= _conf._timeout.subscriber)	//用户超时
					{
						evbuffer_add_printf(buf,
								    "{type: \"welcome\", content: \"channel was destroy\"}\n");
						evhttp_send_reply_chunk((*it_sub)->_req, buf);	//块传输
						evhttp_send_reply_end((*it_sub)->_req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）

						it_sub =
						    (*it_ch)->_subscriber.
						    erase(it_sub);
						continue;
					}
					++it_sub;
				}
			}

			++it_ch;
		}

		evbuffer_free(buf);

		if (_conf._nthreads > 1) {
			if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
			{
				log_error
				    ("pthread_mutex_unlock error for global_mutex");
				err_sys
				    ("pthread_mutex_unlock error for global_mutex");
			}
		}
	}

	void operation::sub_handler(struct evhttp_request *req, void *arg)	//订阅事件触发时的回调函数(不管有无消息立刻断开连接)
	{
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->sub_handler(req, arg);
	}


	void operation::read_handler(struct evhttp_request *req, void *arg)	//读取指定文件数据回调函数
	{
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->read_handler(req, arg);
	}

	void operation::pool_handler(struct evhttp_request *req, void *arg)	//订阅事件触发时的回调函数(保持HTTP长连接)
	{
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->pool_handler(req, arg);
	}

	void operation::pub_handler(struct evhttp_request *req, void *arg) {
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->pub_handler(req, arg);
	}

	void operation::broadcast_handler(struct evhttp_request *req, void *arg) {
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->broadcast_handler(req, arg);	//将消息广播给所有通道
	}

	void operation::clear_handler(struct evhttp_request *req, void *arg) {
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->clear_handler(req, arg);	//将消息广播给所有通道
	}

	void operation::run() {
		//1,根据配置文件，创建日志文件
		//int loglevel = get_level_by_name(_conf._log.level_name.c_str());	//获取日志级别
		//std::string logfile="./logs/"+_conf._log.file;  //放在logs目录下
		//if (log_open(_conf._log.file.c_str(), loglevel, true, _conf._log.rotate_size) == -1)	//创建输出日志文件,线程安全的
		//{
		//	log_error("create log file failed");
		//	err_quit("create log file failed");
		//}
        initlog4cplus();    //初始化log4cplus组件的配置
		//2,根据程序启动方式操作程序
		if (_setup == OPE_STOP)	//停止程序的运行
		{
			kill_process();
			versioninfo();	//显示版本信息
			exit(0);
		} else if (_setup == OPE_RESTART)	//重启程序
		{
			if (file_exists(_conf._pidfile) != 0)	//如果进程ID文件存在，杀死进程
				kill_process();
			else {
				//log_error("cometServer haven't running!");
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"cometServer haven't running!");
				err_quit("cometServer haven't running!");
			}
		}
		//初始化创建1024个通道，单通道容纳1024个用户
		server = make_shared < http::Server > ();

        log4cplus::NDCContextCreator _context("run");       //设置上下文信息
		LOG4CPLUS_TRACE(_logger,"create HTTP Server success !");

		if ((dispatcher_thread.base = event_base_new()) == NULL) {	//创建主线程event_base
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"event_base_new error");
			err_sys("event_base_new error");
		}
		dispatcher_thread.thread_tid = pthread_self();	//保存主线程的线程ID

		if ((dispatcher_thread.sigint_event = evsignal_new(dispatcher_thread.base, SIGINT, sigint_cb, NULL)) == NULL)	//建立中断信号处理机制
		{
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"evsignal_new error for sigint_event");
			err_sys("evsignal_new error for sigint_event");
		}

		if (event_add(dispatcher_thread.sigint_event, NULL) != 0) {
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"event_add error for sigint_event");
			err_sys("event_add error for sigint_event");
		}

		if ((dispatcher_thread.sigterm_event = evsignal_new(dispatcher_thread.base, SIGTERM, sigterm_cb, NULL)) == NULL)	//建立终止信号处理机制
		{
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"evsignal_new error for sigterm_event");
			err_sys("evsignal_new error for sigterm_event");
		}

		if (event_add(dispatcher_thread.sigterm_event, NULL) != 0) {
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"event_add error for sigterm_event");
			err_sys("event_add error for sigterm_event");
		}

		if (_conf._timeout.polling < 0) {
			_conf._timeout.polling = 1;	//设置每一秒检查一次通道
		}

		if ((dispatcher_thread.timer_event = event_new(dispatcher_thread.base, -1, EV_PERSIST, timer_cb, NULL)) == NULL)	//创建定时器
		{
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"evsignal_new error for timer_event");
			err_sys("evsignal_new error for timer_event");
		}

		struct timeval tv;
		tv.tv_sec = _conf._timeout.polling;
		tv.tv_usec = 0;

		if (evtimer_add(dispatcher_thread.timer_event, &tv) != 0)	//将定时器事件添加到event_base
		{
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"evtimer_add error for dispatcher_thread.timer_event");
			err_sys("evtimer_add error for dispatcher_thread.timer_event");
		}

		int adminfd;	//管理员端端口
		int frontfd;	//订阅者端端口

		adminfd = http_server_bind_socket(_conf._admin.ip, _conf._admin.port, _conf._backlog);	//后端绑定套接字地址
		if (adminfd < 0) {
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"bind socket error");
			err_sys("bind socket error");
		}
		frontfd = http_server_bind_socket(_conf._front.ip, _conf._front.port, _conf._backlog);	//前端绑定套接字地址
		if (frontfd < 0) {
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"bind socket error");
			err_sys("bind socket error");
		}

		if ((tptr = (THREAD_LIBEVENT *) calloc(_conf._nthreads, sizeof(THREAD_LIBEVENT))) == NULL)	//预分配包含线程信息的结构
		{
            log4cplus::NDCContextCreator _context("run");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"malloc for THREAD_LIBEVENT error");
			err_sys("malloc for THREAD_LIBEVENT error");
		}

		if (_verbose)
			std::cout << "dispatcher_thread prepare complete !" <<
			    std::endl;
		int result;
		for (int i = 0; i < _conf._nthreads; ++i)	//对每个线程进行设置
		{
			if ((tptr[i].base = event_base_new()) == NULL)	//为每个线程创建一个evebt_base
			{
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"event_base_new error");
				err_sys("event_base_new error");
			}

			if ((tptr[i].adminhttp = evhttp_new(tptr[i].base)) == NULL)	//创建后端HTTP服务器
			{
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"evhttp_new error");
				err_sys("evhttp_new error");
			}

			if ((tptr[i].fronthttp = evhttp_new(tptr[i].base)) == NULL)	//创建前端HTTP服务器
			{
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"evhttp_new error");
				err_sys("evhttp_new error");
			}

			result = evhttp_accept_socket(tptr[i].adminhttp, adminfd);	//将HTTP服务器绑定在指定套接字地址上，成功返回0，失败返回-1
			if (result < 0) {
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"evhttp_accept_socket error");
				err_sys("evhttp_accept_socket error");
			}

			result = evhttp_accept_socket(tptr[i].fronthttp, frontfd);	//将HTTP服务器绑定在指定套接字地址上，成功返回0，失败返回-1
			if (result < 0) {
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"evhttp_accept_socket error");
				err_sys("evhttp_accept_socket error");
			}

			if ((tptr[i].sigint_event = evsignal_new(tptr[i].base, SIGINT, sigint_cb, NULL)) == NULL)	//建立中断信号处理机制
			{
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"evsignal_new error for sigint_event");
				err_sys("evsignal_new error for sigint_event");
			}

			if (event_add(tptr[i].sigint_event, NULL) != 0) {
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"event_add error for sigint_event");
				err_sys("event_add error for sigint_event");
			}

			if ((tptr[i].sigterm_event = evsignal_new(tptr[i].base, SIGTERM, sigterm_cb, NULL)) == NULL)	//建立终止信号处理机制
			{
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"evsignal_new error for sigterm_event");
				err_sys("evsignal_new error for sigterm_event");
			}

			if (event_add(tptr[i].sigterm_event, NULL) != 0) {
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"event_add error for sigterm_event");
				err_sys("event_add error for sigterm_event");
			}

			evhttp_set_cb(tptr[i].adminhttp, "/pub", pub_handler, (void *)&tptr[i]);	//将数据发送给指定通道
			evhttp_set_cb(tptr[i].adminhttp, "/broadcast", broadcast_handler, (void *)&tptr[i]);	//将数据广播给所有通道
			evhttp_set_cb(tptr[i].adminhttp, "/clear", clear_handler, (void *)&tptr[i]);	//清除指定通道内的消息
            evhttp_set_cb(tptr[i].fronthttp, "/sub", sub_handler, (void *)&tptr[i]);	//设置对/sub回调函数
			evhttp_set_cb(tptr[i].fronthttp, "/read", read_handler, (void *)&tptr[i]);	//设置对/read回调函数
			evhttp_set_cb(tptr[i].fronthttp, "/pool", pool_handler, (void *)&tptr[i]);	//设置对/poll回调函数
			evhttp_set_gencb(tptr[i].fronthttp, httpserver_GenericHandler, (void *)&tptr[i]);	//通用回调函数

			result = pthread_create(&tptr[i].thread_tid, NULL, httpserver_Dispatch, (void *)&tptr[i]);	//创建线程，传递属于该线程的event_base
		}
		if (_verbose)
			std::cout << "init thread pool complete !" << std::endl;
		LOG4CPLUS_TRACE(_logger,"init thread pool complete !");

		if (_daemon) {
			if (write_pid(_conf._pidfile) == false)	//将进程ID写入文件
			{
                log4cplus::NDCContextCreator _context("run");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"write process pid to file error");
				err_sys("write process pid to file error");
			}
		}
		std::cout << "  cometServer start... " << std::endl;
		std::cout << "Admin Http Listenning on " << _conf._admin.
		    ip << ":" << _conf._admin.port << "..." << std::endl;
		std::cout << "Front Http Listenning on " << _conf._front.
		    ip << ":" << _conf._front.port << "..." << std::endl;

		event_base_dispatch(dispatcher_thread.base);	//主线程事件循环

		for (int i = 0; i < _conf._nthreads; i++) {	//主线程等待子线程结束
			pthread_join(tptr[i].thread_tid, NULL);
		}

		event_base_free(dispatcher_thread.base);	//释放主循环反应堆
		if (dispatcher_thread.sigint_event != NULL)
			event_free(dispatcher_thread.sigint_event);
		if (dispatcher_thread.sigterm_event != NULL)
			event_free(dispatcher_thread.sigterm_event);
		for (int i = 0; i < _conf._nthreads; i++) {
			if (_verbose)
				std::cout << "thread " << tptr[i].thread_tid <<
				    " used: " << tptr[i].count << std::endl;
			if (tptr[i].base != NULL)
				event_base_free(tptr[i].base);	//释放线程数组反应堆
			if (tptr[i].timer_event != NULL)
				event_free(tptr[i].timer_event);
			if (tptr[i].sigint_event != NULL)
				event_free(tptr[i].sigint_event);	//释放线程数组事件
			if (tptr[i].sigterm_event != NULL)
				event_free(tptr[i].sigterm_event);	//释放线程数组事件
			if (tptr[i].adminhttp != NULL)
				evhttp_free(tptr[i].adminhttp);	//释放线程数组事件
			if (tptr[i].fronthttp != NULL)
				evhttp_free(tptr[i].fronthttp);	//释放线程数组事件
		}
		free(tptr);
	}

	void *operation::httpserver_Dispatch(void *arg)	//线程回调函数
	{
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		if (_verbose)
			std::
			    cout << "thread" << th->thread_tid << " start..." <<
			    std::endl;
		event_base_dispatch(th->base);	//每个线程事件循环
		return NULL;
	}

	int operation::http_server_bind_socket(std::string ip, int port,
					       int backlog) {
		int result;
		int sockfd;
		socklen_t addrlen;

		std::string portstr =
		    boost::lexical_cast < std::string > (port);
		if ((sockfd =
		     tcp_listen(ip.c_str(), portstr.c_str(), &addrlen,
				backlog)) < 0) {
            log4cplus::NDCContextCreator _context("http_server_bind_socket");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"tcp_listen error");
			return -1;
		}

		int one;
		result = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));	//设置套接字地址可重用
		if (result < 0) {
            log4cplus::NDCContextCreator _context("http_server_bind_socket");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"setsocketopt error");
			return -1;
		}

		int flags;
		if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0 || fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0)	//设置套接字为非阻塞的
		{
            log4cplus::NDCContextCreator _context("http_server_bind_socket");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"fcntl error");
			return -1;
		}

		return sockfd;
	}

	int operation::tcp_listen(const char *host, const char *serv,
				  socklen_t * addrlenp, int backlog) {
		int listenfd, n;
		const int on = 1;
		struct addrinfo hints, *res, *ressave;

		bzero(&hints, sizeof(struct addrinfo));
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		if ((n = getaddrinfo(host, serv, &hints, &res)) != 0) {
            log4cplus::NDCContextCreator _context("tcp_listen");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"getaddrinfo error: "<<gai_strerror(n));
			err_sys("getaddrinfo error: %s", gai_strerror(n));
		}

		ressave = res;

		do {
			if ((listenfd =
			     socket(res->ai_family, res->ai_socktype,
				    res->ai_protocol)) < 0)
				continue;

			if (setsockopt
			    (listenfd, SOL_SOCKET, SO_REUSEADDR, &on,
			     sizeof(on)) < 0) {
                log4cplus::NDCContextCreator _context("tcp_listen");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"setsockopt error");
				err_sys("setsockopt error");
			}

			if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
				break;
			close(listenfd);

		} while ((res = res->ai_next) != NULL);

		if (res == NULL) {
            log4cplus::NDCContextCreator _context("tcp_listen");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"tcp_listen error for: "<<host<<","<<serv);
			err_sys("tcp_listen error for %s,%s\n", host, serv);
		}

		if (listen(listenfd, backlog) < 0) {
            log4cplus::NDCContextCreator _context("tcp_listen");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"listen error");
			err_sys("listen error");
		}

		if (addrlenp)
			*addrlenp = res->ai_addrlen;

		freeaddrinfo(ressave);

		return listenfd;
	}

	void operation::httpserver_GenericHandler(struct evhttp_request *req, void *arg)	//通用请求回调函数
	{
		THREAD_LIBEVENT *th = (THREAD_LIBEVENT *) arg;
		th->count++;	//统计该线程被使用的次数
		server->generic_handler(req, arg);

        /*
		struct evbuffer *buf = evbuffer_new();	//创建缓冲区
		if (!buf) {
			log_error("failed to create response buffer");
			err_sys("failed to create response buffer");
		}

		char *filedata = NULL;
		read_file(filedata);
		evbuffer_add_printf(buf, "%s", filedata);	//将文件的数据返回给用户
		//evhttp_send_reply_chunk(req, buf);	//块传输
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
		//evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
		evbuffer_free(buf);
        */
	}

	void operation::read_file(char *filedata)	//读取文件
	{
		unsigned long size = 0;
		struct stat buf;

		if (stat(_conf._generic_file.c_str(), &buf) < 0) {	//获取文件属性
            log4cplus::NDCContextCreator _context("read_file");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"Read file error!");
			return;
		}

		FILE *f = fopen(_conf._generic_file.c_str(), "rb");
		if (f == NULL) {
			fprintf(stderr, "Couldn't open file\n");
			return;
		}

		size = buf.st_size;
		filedata = (char *)malloc(size + 1);
		memset(filedata, 0, size + 1);
		fread(filedata, sizeof(char), size, f);
		fclose(f);

        if(_verbose)
    		fprintf(stderr, "reply file: (%d bytes)\n", (int)size);
	}

	unsigned int operation::read_pid(std::string file)	//读取保存进程ID的文件
	{
		int nread = 0;
		char buf[10];
		FILE *fp;

		if ((fp = fopen(file.c_str(), "rb")) == NULL)	//以二进制读方式打开文件
		{
            log4cplus::NDCContextCreator _context("read_pid");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"fopen error for "<<file);
			err_sys("fopen error for %s", file.c_str());
		}
		while (!feof(fp) && !ferror(fp)) {
			nread = fread(buf, 1, sizeof(buf), fp);	//读文件     
		}
		fclose(fp);	//关闭文件
		if (nread > 0)
			return atoi(buf);	//返回进程ID
		else
			return -1;
	}

	bool operation::write_pid(std::string file)	//将进程ID写入cometServer.pid文件
	{
		unsigned int pid = (unsigned int)getpid();	//获取进程ID
		//char pid_str[10]={0};
		//sprintf(pid_str,"%d",pid);
		std::string pid_str;
		pid_str = boost::lexical_cast < std::string > (pid);

		FILE *fp;
		if ((fp = fopen(file.c_str(), "wb")) == NULL)	//以二进制写方式打开或创建一个文件
		{
            log4cplus::NDCContextCreator _context("write_pid");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"fopen error for "<<file);
			err_sys("fopen error for %s", file.c_str());
		}
		size_t write_size;
		write_size = fwrite(pid_str.c_str(), 1, sizeof(pid_str.c_str()), fp);	//将进程ID写入文件
		//std::cout<<write_size<<std::endl;
		fclose(fp);	//关闭文件
		if (write_size > 0)
			return true;
		else
			return false;
	}

	bool operation::file_exists(string file)	//判断文件是否存在
	{
		struct stat st;
		return stat(file.c_str(), &st) == 0;
	}

	void operation::kill_process()	//杀死进程
	{
		int pid;

		if ((pid = read_pid(_conf._pidfile)) <= 0)	//读取进程ID
			err_sys("read process pid error");

		if ((-1 == kill(pid, 0)) && (errno == ESRCH)) {
            log4cplus::NDCContextCreator _context("kill_process");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"process: "<<pid<<" do not runnning!");
			err_msg("process: %d do not runnning!", pid);
			remove_file(_conf._pidfile);	//从文件系统中删除保存进程ID的文件
			return;
		}

		int ret;
		if ((ret = kill(pid, SIGTERM)) == -1) {
            log4cplus::NDCContextCreator _context("kill_process");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"couldn't kill process: "<<pid);
			err_quit("couldn't kill process: %s", pid);
		}
		remove_file(_conf._pidfile);	//从文件系统中删除保存进程ID的文件

        log4cplus::NDCContextCreator _context("kill_process");       //设置上下文信息
		LOG4CPLUS_FATAL(_logger,"cometServer stop,kill process "<<pid);	//记录信息
		if (_verbose)
			err_msg("cometServer stop,kill process %d", pid);	//记录信息

		while (file_exists(_conf._pidfile))
			usleep(100 * 1000);
	}

	void operation::remove_file(string file)	//从文件系统中删除此文件
	{
		if (!file.empty())
			remove(file.c_str());
	}

	void operation::versioninfo() {
		std::cout << "--------------------------------------------" <<
		    std::endl;
		std::cout << "           End of the cometServer" << std::endl;
		std::cout << "             Thanks for you used" << std::endl;
		std::cout << "             Version 0.0" << std::endl;
		std::cout << "          Copyright (c) 2015 allen" << std::endl;
		std::cout << "          email:  1135628277@qq.com" << std::endl;
		std::cout << "--------------------------------------------" <<
		    std::endl;

	}

	void operation::exe_sql(const char *sql)	//执行写入数据库操作
	{
		MYSQL connection;	//数据库连接句柄
		int result;

		mysql_init(&connection);	//初始化mysql连接connection

		if (mysql_real_connect
		    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
		     CLIENT_FOUND_ROWS)) {
			if (_verbose)
				printf("connect mysql success!\n");

			mysql_query(&connection, "set names utf8");	//设置查询编码为utf8，这样就可以支持中文了

			result = mysql_query(&connection, sql);

			if (result)	//返回非0,标识查询失败
			{
				fprintf(stderr, "mysql query failed!\n");
				fprintf(stderr, "Query error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
				mysql_close(&connection);	//关闭mysql连接
			} else {	//返回0，标识查询成功
				if (_verbose)
					printf("%d rows affected\n", (int)
					       mysql_affected_rows
					       (&connection));
				mysql_close(&connection);	//关闭连接
			}

		} else {
			fprintf(stderr, "mysql_real_connect failed!\n");
			if (mysql_error(&connection))	//显示错误原因
			{
				fprintf(stderr, "Connection error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
			}
		}
	}

	void operation::query_sql(const char *sql)	//将查询结果显示
	{
		MYSQL connection;
		int result;
		MYSQL_RES *res_ptr;
		MYSQL_FIELD *field;
		MYSQL_ROW result_row;
		int column, row;

		mysql_init(&connection);

		if (mysql_real_connect
		    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
		     CLIENT_FOUND_ROWS)) {
			if (_verbose)
				printf("mysql connnect success!\n");
			mysql_query(&connection, "set names utf8");

			result = mysql_query(&connection, sql);

			if (result)	//返回非0，失败
			{
				fprintf(stderr, "mysql query failed!\n");
				fprintf(stderr, "Query error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
				mysql_close(&connection);	//关闭mysql连接
			} else {	//返回0，成功
				res_ptr = mysql_store_result(&connection);	//将查询的结果保存在res_ptr中

				if (res_ptr) {
					column = mysql_num_fields(res_ptr);	//结果集中的列数
					row = mysql_num_rows(res_ptr) + 1;	//结果集中的行数

					if (_verbose)
						printf("query %lu column\n",
						       row);

					for (int k = 0; field = mysql_fetch_field(res_ptr); k++)	//输出结果集中的字段名
					{
						printf("%s\t", field->name);
					}
					printf("\n");
				}

				for (int i = 1; i < row; i++)	//逐行逐列输出每一个信息
				{
					result_row = mysql_fetch_row(res_ptr);	//从结果集中获取下一行
					for (int j = 0; j < column; j++)
						printf("%s\t", result_row[j]);	//显示该行每一列信息
					printf("\n");
				}

				mysql_free_result(res_ptr);	//释放结果集
				mysql_close(&connection);	//查询完毕关闭连接

			}
		} else {
			fprintf(stderr, "mysql_real_connect failed!\n");
			if (mysql_error(&connection))	//显示错误原因
			{
				fprintf(stderr, "Connection error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
			}
		}

	}

	void operation::comet_query_sql(const char *sql, std::vector < std::string > &result_store)	//只查询一个字段content
	{
		MYSQL connection;
		int result;
		MYSQL_RES *res_ptr;
		MYSQL_ROW result_row;
		int column, row;

		mysql_init(&connection);

		if (mysql_real_connect
		    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
		     CLIENT_FOUND_ROWS)) {
			mysql_query(&connection, "set names utf8");

			result = mysql_query(&connection, sql);

			if (result)	//返回非0，失败
			{
				fprintf(stderr, "mysql query failed!\n");
				fprintf(stderr, "Query error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
				mysql_close(&connection);	//关闭mysql连接
			} else {	//返回0，成功
				res_ptr = mysql_store_result(&connection);	//将查询的结果保存在res_ptr中

				if (res_ptr) {
					column = mysql_num_fields(res_ptr);	//结果集中的列数
					row = mysql_num_rows(res_ptr) + 1;	//结果集中的行数

					for (int i = 1; i < row; i++)	//逐行逐列输出每一个信息
					{
						result_row = mysql_fetch_row(res_ptr);	//从结果集中获取下一行
						for (int j = 0; j < column; j++) {
							result_store.push_back(result_row[j]);	//将content存放到容器中
						}
					}
				}

				mysql_free_result(res_ptr);	//释放结果集
				mysql_close(&connection);	//查询完毕关闭连接

			}
		} else {
			fprintf(stderr, "mysql_real_connect failed!\n");
			if (mysql_error(&connection))	//显示错误原因
			{
				fprintf(stderr, "Connection error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
			}
		}

	}

	void operation::comet_query_seqmax_sql(int channel, int &seqmax)	//查询指定通道内消息序列号的最大值
	{
		MYSQL connection;
		int result;
		MYSQL_RES *res_ptr;
		MYSQL_ROW result_row;
		int column, row;
		char sql[255] = "";
		char *str_seqmax = NULL;

		sprintf(sql, "select MAX(seq) from message where channel=%d;", channel);	//查询指定通道内的最大消息序列号

		mysql_init(&connection);

		if (mysql_real_connect
		    (&connection, HOST, USER, PASSWORD, DATABASE, 0, NULL,
		     CLIENT_FOUND_ROWS)) {
			mysql_query(&connection, "set names utf8");

			result = mysql_query(&connection, sql);

			if (result)	//返回非0，失败
			{
				fprintf(stderr, "mysql query failed!\n");
				fprintf(stderr, "Query error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
				mysql_close(&connection);	//关闭mysql连接
			} else {	//返回0，成功
				res_ptr = mysql_store_result(&connection);	//将查询的结果保存在res_ptr中

				if (res_ptr) {
					column = mysql_num_fields(res_ptr);	//结果集中的列数
					row = mysql_num_rows(res_ptr) + 1;	//结果集中的行数

					for (int i = 1; i < row; i++)	//逐行逐列输出每一个信息
					{
						result_row = mysql_fetch_row(res_ptr);	//从结果集中获取下一行
						for (int j = 0; j < column; j++) {
							//printf("%s\t", result_row[j]);    //显示该行每一列信息
							str_seqmax =
							    result_row[j];
							if (str_seqmax != NULL) {
								seqmax =
								    boost::lexical_cast
								    < int
								> (str_seqmax);
								//printf("%d\n",seqmax);
							}
						}
					}
				}

				mysql_free_result(res_ptr);	//释放结果集
				mysql_close(&connection);	//查询完毕关闭连接

			}
		} else {
			fprintf(stderr, "mysql_real_connect failed!\n");
			if (mysql_error(&connection))	//显示错误原因
			{
				fprintf(stderr, "Connection error %d: %s\n",
					mysql_errno(&connection),
					mysql_error(&connection));
			}
		}

	}

	void operation::record_time(boost::timer::cpu_timer & t)	//记录程序的运行时间,用于衡量服务器的性能
	{
		ofstream out("cpu_timer_log/cpu_timer.log");
		out << t.format();
		out.close();
		if (_verbose)
			std::cout << t.format() << std::endl;
	}
}
