/*
 * =====================================================================================
 *
 *       Filename:  server.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 09时53分42秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include"server.h"
#include"subscriber.h"
//#include"cometserver_db.h"    //对mysql的数据操作进行封装
#include<ctime>
#include<iostream>
#include<string>

extern http::CONFIGURE _conf;
extern std::pair < unsigned int, unsigned int >discdata;
extern bool _verbose;		//是否显示详细信息
extern void err_sys(const char *fmt, ...);
extern void err_quit(const char *fmt, ...);
extern void err_msg(const char *fmt, ...);

extern pthread_mutex_t _global_mutex;
extern log4cplus::Logger _logger;

namespace http {
	Server::Server() {
		init();		//初始化
	} Server::~Server() {
	}

	void Server::init() {
		_clientNum = 0;
		for (int i = 0; i < CHAN; ++i)	//初始化创建1024个通道
		{
			auto channel = make_shared < Channel > ();	//初始化创建时默认_msgcount为0
			channel->_id = i;	//通道ID
			operation::comet_query_seqmax_sql(i, channel->_msgcount);	//初始化通道内消息计数器
			_channel.push_back(channel);
		}
	}

	void Server::read_handler(struct evhttp_request *req, void *arg)	//用户订阅消息,不管是否有消息，即可断开连接，无需保存用户信息
    {
		struct evkeyvalq params;	//键值对结构队列
		struct evbuffer *buf;	//创建一个缓冲区
        std::string filename;
		struct evkeyval *kv;	//键值对结构体
		const char *uri;

		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
			//log_debug("reply to subscriber: 405 Method Not Allowed");
            log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed"); //记录的是调试信息
			if (_verbose)
				err_msg
				    ("reply to subscriber: 405 Method Not Allowed");
			return;
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
			//log_error("evhttp_request_get_uri error");
            log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error: "<<uri<<std::endl);  //记录信息用于调试
			//err_sys("evhttp_request_get_uri error");
            return ;    //直接返回，不要终止程序的运行，return只是在当前线程直接返回
		}

        //printf("uri: %s\n",uri);

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
			//log_error("evhttp_parse_query error");
            log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error: "<<uri<<std::endl);
			//err_sys("evhttp_parse_query error");
            return ;
		}

		for (kv = params.tqh_first; kv; kv = kv->next.tqe_next) {	//遍历请求报文中的键值对，找出ID的值
			if (strcmp(kv->key, "file") == 0) {	//读取的文件名
				filename = kv->value;
            }
		}


		if (_conf._nthreads > 1) {
			if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
			{
				//log_error("pthread_mutex_lock error for global_mutex");
                log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
				err_sys("pthread_mutex_lock error for global_mutex");
                //return ;
			}
		}


		if ((buf = evbuffer_new()) == nullptr) {
			//log_error("evbuffer_new error !");	//将错误信息记录日志
            log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evbuffer_new error !");	//将错误信息记录日志
			//err_sys("evbuffer_new error !");	//终止程序运行,输出错误消息
            return ;
		}
		evhttp_send_reply_start(req, HTTP_OK, "OK");	//通过chunk 编码传输向客户响应报文，初始化作用
		//evhttp_add_header(req->output_headers, "Connection", "Keep-Alive");   //保持HTTP长连接
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");

        if(_verbose)    //显示更多的详细信息
        {
		    //log_info("reply to subscriber: HTTP_OK OK");
		    //log_info("reply to subscriber: Connection Keep-Alive");
		    //log_info("reply to subscriber: Content-Type,text/html; charset=utf-8");
            log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
			LOG4CPLUS_TRACE(_logger,"reply to subscriber: HTTP_OK OK");	//记录详细信息
			LOG4CPLUS_TRACE(_logger,"reply to subscriber: Connection Keep-Alive");
			LOG4CPLUS_TRACE(_logger,"reply to subscriber: Content-Type,text/html; charset=utf-8");
        }

        //读取文件内容,将文件数据发送给用户,逐行读取文件内容，逐行发送至缓冲区
        std::string line;
        //std::string file="/unp/graduation/project/doc_root/";     //所有文件存放的目录
        std::string file=_conf._generic_file;
        file+=filename;
        std::ifstream infile(file);

        if(infile.is_open())
        {
            while(getline(infile,line))     //逐行读取文件并逐行发送文件内容
            {
			    evbuffer_add_printf(buf,"%s\n",line.c_str());
			    evhttp_send_reply_chunk(req, buf);	//块传输
                //std::cout<<line<<std::endl;
            }
        }else{  //发送错误响应报文
			    evbuffer_add_printf(buf,"file is not exists!\n");
			    evhttp_send_reply_chunk(req, buf);	//块传输
        }


		evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
		evbuffer_free(buf);

		//log_info("reply to subscriber[%d]: %s",_clientNum,file.c_str());
        log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
        LOG4CPLUS_TRACE(_logger, "reply to subscriber["<<_clientNum<<"]: "<<file<<std::endl);

		if (++_clientNum > 50000)	//如果用户ID超过20000，从0开始标识
			_clientNum = 0;


		if (_conf._nthreads > 1) {
			if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
			{
				//log_error("pthread_mutex_unlock error for global_mutex");
                log4cplus::NDCContextCreator _context("read_handler");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
				err_sys("pthread_mutex_unlock error for global_mutex");//解锁失败是致命错误，需要设置为FATAL级别，并且程序退出
			}
		}
    
    }

	void Server::sub_handler(struct evhttp_request *req, void *arg)	//用户订阅消息,不管是否有消息，即可断开连接，无需保存用户信息
	{
		struct evkeyvalq params;	//键值对结构队列
		struct evbuffer *buf;	//创建一个缓冲区
		int cid = -1;
		int seq = -1;
		std::string time_sql;
		struct evkeyval *kv;	//键值对结构体
		const char *uri;

		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
			//log_debug("reply to subscriber: 405 Method Not Allowed");
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed");
			if (_verbose)
				err_msg
				    ("reply to subscriber: 405 Method Not Allowed");
			return;     //直接返回，不终止程序的运行
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
			//log_error("evhttp_request_get_uri error");
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error");
			//err_sys("evhttp_request_get_uri error");
            return ;
		}

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
			//log_error("evhttp_parse_query error");
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error: "<<uri<<std::endl);
			//err_sys("evhttp_parse_query error");
            return ;
		}

		for (kv = params.tqh_first; kv; kv = kv->next.tqe_next) {	//遍历请求报文中的键值对，找出ID的值
			if (strcmp(kv->key, "cid") == 0) {	//通道ID
				cid = atoi(kv->value);
			} else if (strcmp(kv->key, "seq") == 0) {
				seq = atoi(kv->value);
			} else if (strcmp(kv->key, "time") == 0) {
				time_sql = kv->value;
			}
		}

		if (cid < 0 || cid >= CHAN) {	//ID值无效，向客户发送响应报文
			buf = evbuffer_new();
			evhttp_send_reply_start(req, HTTP_NOTFOUND,
						"Not Found");
			evbuffer_free(buf);
			//log_error("subscriber request cid invalide !");
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"subscriber request cid invalide: "<<cid<<std::endl);
			return;
		}

		if (_conf._nthreads > 1) {
			if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
			{
				//log_error("pthread_mutex_lock error for global_mutex");
                log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
				err_sys("pthread_mutex_lock error for global_mutex");   //上锁失败是致命错误,程序终止
			}
		}

		auto channel = _channel[cid];	//返回通道的shared_ptr
		if (channel == nullptr) {
			buf = evbuffer_new();
			evhttp_send_reply_start(req, HTTP_NOTFOUND,
						"Not Found");
			evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
			evbuffer_free(buf);
			//log_error("subscriber request cid invalide !");
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"subscriber request cid invalide: "<<cid<<std::endl);

			discdata = {
			_clientNum, cid};
			evhttp_connection_set_closecb(req->evcon, operation::cleanup, &discdata);	//设置关闭连接的回调函数

			if (++_clientNum > 50000)	//如果用户ID超过20000，从0开始标识
				_clientNum = 0;

			if (_conf._nthreads > 1) {
				if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
				{
					//log_error("pthread_mutex_unlock error for global_mutex");
                    log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			        LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
					err_sys("pthread_mutex_unlock error for global_mutex");
				}
			}
			return;
		}

		if ((buf = evbuffer_new()) == nullptr) {
			log_error("evbuffer_new error !");	//将错误信息记录日志
			err_sys("evbuffer_new error !");	//终止程序运行,输出错误消息
		}
		evhttp_send_reply_start(req, HTTP_OK, "OK");	//通过chunk 编码传输向客户响应报文，初始化作用
		//evhttp_add_header(req->output_headers, "Connection", "Keep-Alive");   //保持HTTP长连接
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");

        if(_verbose)
        {
		    //log_info("reply to subscriber: HTTP_OK OK");
		    //log_info("reply to subscriber: Content-Type,text/html; charset=utf-8");
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_TRACE(_logger,"reply to subscriber: HTTP_OK OK");
			LOG4CPLUS_TRACE(_logger,"reply to subscriber: Content-Type,text/html; charset=utf-8");
        }

		/*
		   if (channel->_msg.empty())   //该通道消息队列为空
		   {
		   evbuffer_add_printf(buf,
		   "{type: \"text\", cid: \"%d\", content: \"not message read\"}\n",
		   cid);
		   evhttp_send_reply_chunk(req, buf);   //块传输

		   } else {     //该通道消息队列不为空
		   time_t tm;
		   time(&tm);
		   //从消息队列中取得消息发送给用户
		   for (auto & x:channel->_msg) {
		   if ((tm - x.time) < _conf._timeout.message) {
		   evbuffer_add_printf(buf,
		   "{type: \"text\", cid: \"%d\", content: \"%s\"}\n",
		   cid,
		   x.content.c_str());
		   evhttp_send_reply_chunk(req, buf);   //块传输
		   }
		   }
		   //channel->_msg.clear();  //清空消息队列
		   //删除超时的消息
		   auto it = channel->_msg.begin();
		   for (; it != channel->_msg.end();) {
		   if ((tm - it->time) > _conf._timeout.message) {
		   it = channel->_msg.erase(it);        //返回删除的后一个位置
		   continue;
		   }
		   ++it;
		   }
		   }
		 */

		//从数据库中查询消息发送给用户
		std::vector < std::string > reply_msg;

		char sql[255];
		if (seq > 0 && time_sql.empty())
			sprintf(sql,
				"select content from message where channel=%d and seq>=%d;",
				cid, seq);
		else if (seq < 0 && !time_sql.empty())
			sprintf(sql,
				"select content from message where channel=%d and time=%s;",
				cid, time_sql.c_str());
		else if (seq > 0 && time_sql.empty()) {
			sprintf(sql,
				"select content from message where channel=%d and seq>=%d and time=%s;",
				cid, seq, time_sql.c_str());
		} else if (seq <= 0 && time_sql.empty()) {
			sprintf(sql,
				"select content from message where channel=%d and seq>=1",
				cid);
		}

		operation::comet_query_sql(sql, reply_msg);	//将查询结果存储在reply_msg容器中
	      for (auto & x:reply_msg) {
			evbuffer_add_printf(buf,
					    "{type:\"welcom\",cid=%d,content:\"%s\"}\n",
					    cid, x.c_str());
			evhttp_send_reply_chunk(req, buf);	//块传输
			//log_info("reply to subscriber[%d]: {type: \"text\", cid: \"%d\", content: \"%s\"}",_clientNum,cid,x.c_str());
            log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
			LOG4CPLUS_TRACE(_logger,"reply to subscriber["<<_clientNum<<"]: {type: \"text\", cid: "<<cid<<", content: \""<<x<<"\"}"<<std::endl);
		}
		reply_msg.clear();

		evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
		evbuffer_free(buf);

		discdata = {
		_clientNum, cid};
		evhttp_connection_set_closecb(req->evcon, operation::cleanup, &discdata);	//设置关闭连接的回调函数

		if (++_clientNum > 50000)	//如果用户ID超过20000，从0开始标识
			_clientNum = 0;


		if (_conf._nthreads > 1) {
			if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
			{
				//log_error("pthread_mutex_unlock error for global_mutex");
                log4cplus::NDCContextCreator _context("sub_handler");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");     //解锁失败是致命错误
				err_sys("pthread_mutex_unlock error for global_mutex");
			}
		}
	}

	//有消息直接返回，无消息保持HTTP长连接
	void Server::pool_handler(struct evhttp_request *req, void *arg)	//订阅消息(保持HTTP长连接,需保存用户信息)
	{
		struct evkeyvalq params;	//键值对结构队列
		struct evbuffer *buf;	//创建一个缓冲区
		int cid = -1;
		int seq = -1;
		std::string time_sql;
		struct evkeyval *kv;	//键值对结构体
		const char *uri;

		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
			//log_debug("reply to subscriber: 405 Method Not Allowed");
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed");
			if (_verbose)
				err_msg
				    ("reply to subscriber: 405 Method Not Allowed");
			return;
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
			//log_error("evhttp_request_get_uri error");
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error");
		}

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
			log_error("evhttp_parse_query error");
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error");
		}

		for (kv = params.tqh_first; kv; kv = kv->next.tqe_next) {	//遍历请求报文中的键值对，找出ID的值
			if (strcmp(kv->key, "cid") == 0) {	//通道ID
				cid = atoi(kv->value);
			} else if (strcmp(kv->key, "seq") == 0) {
				seq = atoi(kv->value);
			} else if (strcmp(kv->key, "time") == 0) {
				time_sql = kv->value;
			}
		}

		if (cid < 0 || cid >= CHAN) {	//ID值无效，向客户发送响应报文
			buf = evbuffer_new();
			evhttp_send_reply_start(req, HTTP_NOTFOUND,
						"Not Found");
			evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
			evbuffer_free(buf);
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"subscriber request cid invalide !");

			discdata = {
			_clientNum, cid};
			evhttp_connection_set_closecb(req->evcon, operation::cleanup, &discdata);	//设置关闭连接的回调函数

			if (++_clientNum > 50000)	//如果用户ID超过20000，从0开始标识
				_clientNum = 0;

			if (_conf._nthreads > 1) {
				if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
				{
					//log_error("pthread_mutex_unlock error for global_mutex");
                    log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
					LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
					err_sys("pthread_mutex_unlock error for global_mutex");
				}
			}
			return;
		}

		if (_conf._nthreads > 1) {
			if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
			{
				//log_error("pthread_mutex_lock error for global_mutex");
                log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
				err_sys("pthread_mutex_lock error for global_mutex");
			}
		}

		auto channel = _channel[cid];	//返回通道的shared_ptr
		if (channel == nullptr) {
			buf = evbuffer_new();
			evhttp_send_reply_start(req, HTTP_NOTFOUND,
						"Not Found");
			evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
			evbuffer_free(buf);
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"subscriber request cid invalide !");

			discdata = {
			_clientNum, cid};
			evhttp_connection_set_closecb(req->evcon, operation::cleanup, &discdata);	//设置关闭连接的回调函数

			if (++_clientNum > 50000)	//如果用户ID超过20000，从0开始标识
				_clientNum = 0;

			if (_conf._nthreads > 1) {
				if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
				{
                    log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
					LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
					err_sys("pthread_mutex_unlock error for global_mutex");
				}
			}
			return;
		}

		//if (++_clientNum > 500000)	//如果用户ID超过500000，从0开始标识
		//	_clientNum = 0;

		//创建用户，加入到通道中
		auto subscriber = make_shared < Subscriber > ();
		subscriber->_id = _clientNum;	//用户ID
		subscriber->_req = req;	//http连接结构体
		channel->_subscriber.push_back(subscriber);	//加入通道中

		if ((buf = evbuffer_new()) == nullptr) {
			//log_error("evbuffer_new error !");	//将错误信息记录日志
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evbuffer_new error !");	//记录出错的信息，用于调试
		}
		evhttp_send_reply_start(req, HTTP_OK, "OK");	//通过chunk 编码传输向客户响应报文，初始化作用
		evhttp_add_header(req->output_headers, "Connection", "Keep-Alive");	//保持HTTP长连接
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");

        if(_verbose)
        {
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
		    LOG4CPLUS_TRACE(_logger,"reply to subscriber: HTTP_OK OK");
		    LOG4CPLUS_TRACE(_logger,"reply to subscriber: Connection Keep-Alive");
		    LOG4CPLUS_TRACE(_logger,"reply to subscriber: Content-Type,text/html; charset=utf-8");
        }

		/*  
		   if (channel->_msg.empty())   //该通道消息队列为空
		   {
		   time_t tm;
		   time(&tm);
		   subscriber->_freeStart=tm;      //设置用户空闲开始时间
		   if(channel->_freeStart==0)      //第一次设置
		   channel->_freeStart=tm;     //设置通道空闲开始时间,不应该每次都更新空闲开始时间,保证只设置一次

		   evbuffer_add_printf(buf,
		   "{type: \"text\", cid: \"%d\", content: \"not message read\"}\n",
		   cid);
		   evhttp_send_reply_chunk(req, buf);   //块传输

		   if (_conf._nthreads > 1) {
		   if (pthread_mutex_unlock(&_global_mutex) != 0)       //解锁
		   {
		   log_error
		   ("pthread_mutex_unlock error for global_mutex");
		   err_sys
		   ("pthread_mutex_unlock error for global_mutex");
		   }
		   }

		   evbuffer_free(buf);
		   ++_clientNum;
		   return ;    //直接返回，不要断开连接

		   }    

		   //该通道消息队列不为空
		   time_t tm;
		   time(&tm);
		   //从消息队列中取得消息发送给用户
		   for (auto & x:channel->_msg) {
		   if ((tm - x.time) < _conf._timeout.message) {
		   evbuffer_add_printf(buf,
		   "{type: \"text\", cid: \"%d\", content: \"%s\"}\n",
		   cid,
		   x.content.c_str());
		   evhttp_send_reply_chunk(req, buf);   //块传输
		   }
		   }
		   //channel->_msg.clear();  //清空消息队列
		   //删除超时的消息
		   auto it = channel->_msg.begin();
		   for (; it != channel->_msg.end();) {
		   if ((tm - it->time) >= _conf._timeout.message) {
		   it = channel->_msg.erase(it);        //返回删除的后一个位置
		   continue;
		   }
		   ++it;
		   }

		   //evhttp_send_reply_chunk(req, buf);  //块传输
		   evhttp_send_reply_end(req);  //此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
		   evbuffer_free(buf);

		   channel->delSubscriber(_clientNum);  //删除用户

		   //auto data=make_pair(_clientNum,cid);    //不能这样传，临时变量
		   discdata = {_clientNum, cid};
		   evhttp_connection_set_closecb(req->evcon, operation::cleanup, &discdata);    //设置关闭连接的回调函数
		 */

		//从数据库中查询消息发送给用户
		std::vector < std::string > reply_msg;

		char sql[255];
		if (seq >= 0 && time_sql.empty())
			sprintf(sql,
				"select content from message where channel=%d and seq>=%d;",
				cid, seq);
		else if (seq < 0 && !time_sql.empty())
			sprintf(sql,
				"select content from message where channel=%d and time=%s;",
				cid, time_sql.c_str());
		else if (seq >= 0 && !time_sql.empty()) {
			sprintf(sql,
				"select content from message where channel=%d and seq>=%d and time=%s;",
				cid, seq, time_sql.c_str());
		}
		operation::comet_query_sql(sql, reply_msg);	//将查询结果存储在reply_msg容器中

		time_t tm;
		if (reply_msg.empty())	//通道内没有消息
		{
			time(&tm);
			subscriber->_freeStart = tm;	//设置用户空闲开始时间
			if (channel->_freeStart == 0)	//第一次设置
				channel->_freeStart = tm;	//设置通道空闲开始时间,不应该每次都更新空闲开始时间,保证只设置一次

			evbuffer_add_printf(buf,
					    "{type: \"text\", cid: %d, content: \"not message read\"}\n",
					    cid);
			evhttp_send_reply_chunk(req, buf);	//块传输

            /*
			log_info
			    ("reply to subscriber[%d]: {type: \"text\", cid: \"%d\", content: \"not message read\"}",
			     _clientNum,cid);
                 */
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_TRACE(_logger,"reply to subscriber["<<_clientNum<<"]: {type: \"text\", cid: "<<cid<<", content: \"not message read\"}"<<std::endl);


			if (_conf._nthreads > 1) {
				if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
				{
                    log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
					LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
					err_sys("pthread_mutex_unlock error for global_mutex");
				}
			}

			evbuffer_free(buf);
			if (++_clientNum > 500000)	//如果用户ID超过500000，从0开始标识
				_clientNum = 0;
			return;	//直接返回，不要断开连接

		}
		//从数据库中查询到了数据
		time(&tm);
	      for (auto & x:reply_msg) {
			evbuffer_add_printf(buf,
					    "{type: \"text\", cid: \"%d\", content: \"%s\"}\n",
					    cid, x.c_str());
			evhttp_send_reply_chunk(req, buf);	//块传输
			//log_info("reply to subscriber[%d]: {type: \"text\", cid: \"%d\", content: \"%s\"}",_clientNum,cid,x.c_str());
            log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
			LOG4CPLUS_TRACE(_logger,"reply to subscriber["<<_clientNum<<"]: {type: \"text\", cid: \""<<cid<<"\", content: \""<<x<<"\"}"<<std::endl);

		}

		evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
		evbuffer_free(buf);

		channel->delSubscriber(_clientNum);	//删除用户

		//auto data=make_pair(_clientNum,cid);    //不能这样传，临时变量
		discdata = {
		_clientNum, cid};
		evhttp_connection_set_closecb(req->evcon, operation::cleanup, &discdata);	//设置关闭连接的回调函数

		if (++_clientNum > 500000)	//如果用户ID超过500000，从0开始标识
			_clientNum = 0;

		if (_conf._nthreads > 1) {
			if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
			{
				//log_error("pthread_mutex_unlock error for global_mutex");
                log4cplus::NDCContextCreator _context("pool_handler");       //设置上下文信息
				LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
				err_sys("pthread_mutex_unlock error for global_mutex");
			}
		}
	}

	void Server::pub_handler(struct evhttp_request *req, void *arg)	//推送消息
	{
		struct evkeyvalq params;
		const char *uri;
		struct evbuffer *buf;
		int cid = -1;
		const char *content = "";
		struct evkeyval *kv;

		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
            log4cplus::NDCContextCreator _context("pub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed");
			if (_verbose)
				err_msg("reply to subscriber: 405 Method Not Allowed");
			return;
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
            log4cplus::NDCContextCreator _context("pub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error");
            return ;
		}

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
            log4cplus::NDCContextCreator _context("pub_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error");
            return ;
		}

		for (kv = params.tqh_first; kv; kv = kv->next.tqe_next) {
			if (strcmp(kv->key, "cid") == 0) {
				cid = atoi(kv->value);
			} else if (strcmp(kv->key, "content") == 0) {
				content = kv->value;
			}
		}

		shared_ptr < Channel > channel;	//获取通道对象
		if (cid < 0 || cid >= CHAN) {
			channel = nullptr;
		} else {
			channel = _channel[cid];	//获取通道
		}

		time_t t;
		time(&t);
		MSG msg;
		msg.content = content;	//消息内容
		msg.time = t;	//记录消息创建的时间
		//将消息加入通道消息队列

		if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
		{
            log4cplus::NDCContextCreator _context("pub_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
			err_sys("pthread_mutex_lock error for global_mutex");
		}
		channel->_msg.push_back(msg); 

		//将消息写入mysql数据库
		char datebuf[11];
		struct tm *tmp;
		time(&t);
		tmp = localtime(&t);
		strftime(datebuf, 11, "%F", tmp);
		++channel->_msgcount;	//该通道内总消息数
		char sql_exe[255];
		sprintf(sql_exe,
			"insert into message values(%d,%d,\'%s\',\'%s\');", cid,
			channel->_msgcount, datebuf, content);
		operation::exe_sql(sql_exe);	//将消息写入数据库

		//log_info("push message \"%s\" to channel[%d]", content, cid);
        log4cplus::NDCContextCreator _context("pub_handler");       //设置上下文信息
		LOG4CPLUS_TRACE(_logger,"push message \""<<content<<"\" to channel["<<cid<<"]"<<std::endl);

		if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
		{
            log4cplus::NDCContextCreator _context("pub_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
			err_sys("pthread_mutex_unlock error for global_mutex");
		}

		if (_verbose)
			std::cout << "pub: channel:" << cid << " content: " <<
			    content << std::endl;

		// response to publisher
		buf = evbuffer_new();
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");
		evbuffer_add_printf(buf, "ok\n");
		evhttp_send_reply(req, 200, "OK", buf);	//发送响应报文给发布者
		evbuffer_free(buf);
	}

	void Server::broadcast_handler(struct evhttp_request *req, void *arg)	//管理员推送消息,pool用户能够立刻接受到消息
	{

		struct evkeyvalq params;
		const char *uri;
		struct evbuffer *buf;
		const char *content = "";
		struct evkeyval *kv;


		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
            log4cplus::NDCContextCreator _context("broadcast_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed");
			if (_verbose)
				err_msg
				    ("reply to subscriber: 405 Method Not Allowed");
			return;
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
            log4cplus::NDCContextCreator _context("broadcast_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error");
            return ;
		}

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
            log4cplus::NDCContextCreator _context("broadcast_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error");
            return ;
		}

		if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
		{
            log4cplus::NDCContextCreator _context("broadcast_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
			err_sys("pthread_mutex_lock error for global_mutex");
		}

		for (kv = params.tqh_first; kv; kv = kv->next.tqe_next) {
			if (strcmp(kv->key, "content") == 0) {
				content = kv->value;
			}
		}

		char datebuf[11];
		time_t t;
		MSG msg;
		msg.content = content;
		time(&t);
		msg.time = t;	//记录消息创建的时间

	      for (auto & x:_channel)
			//将消息推送给所有通道
		{
			//x->_msg.push_back(msg); 

			//将消息写入mysql数据库
			struct tm *tmp;
			tmp = localtime(&t);
			strftime(datebuf, 11, "%F", tmp);
			++x->_msgcount;	//该通道内总消息数
			char sql_exe[255];
			sprintf(sql_exe,
				"insert into message values(%d,%d,\'%s\',\'%s\');",
				x->_id, x->_msgcount, datebuf, content);
			operation::exe_sql(sql_exe);	//将消息写入数据库
		}

		//log_info("broadcast message: \"%s\"", content);
        log4cplus::NDCContextCreator _context("broadcast_handler");       //设置上下文信息
		LOG4CPLUS_TRACE(_logger,"broadcast message \""<<content<<"\""<<std::endl);

		if (_verbose)
			std::
			    cout << "broadcast: content: " << content <<
			    std::endl;

		if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
		{
            log4cplus::NDCContextCreator _context("broadcast_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
			err_sys("pthread_mutex_unlock error for global_mutex");
		}
		// response to publisher
		buf = evbuffer_new();
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");
		evbuffer_add_printf(buf, "ok\n");
		evhttp_send_reply(req, 200, "OK", buf);	//发送响应报文给发布者
		evbuffer_free(buf);
	}

	void Server::clear_handler(struct evhttp_request *req, void *arg)	//清空指定通道内的所有消息
	{
		struct evkeyvalq params;
		const char *uri;
		struct evbuffer *buf;
		int cid = -1;
		struct evkeyval *kv;

		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
            log4cplus::NDCContextCreator _context("clear_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed");
			if (_verbose)
				err_msg
				    ("reply to subscriber: 405 Method Not Allowed");
			return;
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
            log4cplus::NDCContextCreator _context("clear_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error");
            return ;
		}

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
            log4cplus::NDCContextCreator _context("clear_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error");
            return ;
		}

		if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
		{
            log4cplus::NDCContextCreator _context("clear_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
			err_sys("pthread_mutex_lock error for global_mutex");
		}

		for (kv = params.tqh_first; kv; kv = kv->next.tqe_next) {
			if (strcmp(kv->key, "cid") == 0) {
				cid = atoi(kv->value);
			}
		}

		shared_ptr < Channel > channel;	//获取通道对象
		if (cid < 0 || cid >= CHAN) {
			channel = nullptr;
		} else {
			channel = _channel[cid];	//获取通道
		}

		char sql_exe[255];
		sprintf(sql_exe, "delete from message where channel=%d;", cid);
		operation::exe_sql(sql_exe);	//将消息写入数据库

		//log_info("clear message in channel[%d]", cid);
        log4cplus::NDCContextCreator _context("clear_handler");       //设置上下文信息
		LOG4CPLUS_TRACE(_logger,"clear message in channel["<<cid<<"]"<<std::endl);

		channel->_msgcount = 0;	//通道内的消息计数器重置为0

		if (_verbose)
			std::cout << "clear: cid: " << cid << std::endl;

		if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
		{
            log4cplus::NDCContextCreator _context("clear_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
			err_sys("pthread_mutex_unlock error for global_mutex");
		}
		// response to publisher
		buf = evbuffer_new();
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");
		evbuffer_add_printf(buf, "ok\n");
		evhttp_send_reply(req, 200, "OK", buf);	//发送响应报文给发布者
		evbuffer_free(buf);

	}

	void Server::generic_handler(struct evhttp_request *req, void *arg)	//通用回调函数
    {
		struct evkeyvalq params;
		const char *uri;
		struct evbuffer *buf;

		if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
			evhttp_send_reply(req, 405, "Method Not Allowed", NULL);	//发送错误的响应报文给用户
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"reply to subscriber: 405 Method Not Allowed");
			if (_verbose)
				err_msg
				    ("reply to subscriber: 405 Method Not Allowed");
			return;
		}

		if ((uri = evhttp_request_get_uri(req)) == NULL)	//解析URI
		{
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_request_get_uri error");
            return ;
		}

		if (evhttp_parse_query(uri, &params) != 0)	//将URI请求报文中的消息头（键值对）提取出来，保存在params中
		{
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evhttp_parse_query error");
            return ;
		}

		if (pthread_mutex_lock(&_global_mutex) != 0)	//上锁
		{
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_lock error for global_mutex");
			err_sys("pthread_mutex_lock error for global_mutex");
		}

        //解析URI，获取客户请求的文件
        /*
        char *file; 
        if((file=strrchr(uri,'/'))==NULL)
        {
			evhttp_send_reply(req, 404, "FILE NOT FOUND", NULL);	//发送错误的响应报文给用户
			log_error("FILE NOT FOUND");	//发送错误的响应报文给用户
            if(_verbose)
              err_msg("FILE NOT FOUND");
        }else
          ++file;
          */
        std::string tmp=uri;
        auto pos=tmp.find('/',0);
        std::string file=tmp.substr(++pos);
        //std::cout<<"file: "<<file<<std::endl;

		if ((buf = evbuffer_new()) == nullptr) {
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
			LOG4CPLUS_DEBUG(_logger,"evbuffer_new error !");	//将错误信息记录日志
            return ;
		}

		evhttp_send_reply_start(req, HTTP_OK, "OK");	//通过chunk 编码传输向客户响应报文，初始化作用
		//evhttp_add_header(req->output_headers, "Connection", "Keep-Alive");   //保持HTTP长连接
		evhttp_add_header(req->output_headers, "Content-Type",
				  "text/html; charset=utf-8");

        if(_verbose)
        {
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
		    LOG4CPLUS_TRACE(_logger,"reply to subscriber: HTTP_OK OK");
		    LOG4CPLUS_TRACE(_logger,"reply to subscriber: Connection Keep-Alive");
		    LOG4CPLUS_TRACE(_logger,"reply to subscriber: Content-Type,text/html; charset=utf-8");
        }

        std::string comfile;
        std::string line;
        comfile=_conf._generic_file+file;   //请求文件名
        std::ifstream infile(comfile);


        if(infile.is_open())
        {
            while(getline(infile,line)) 
            {
			    evbuffer_add_printf(buf,"%s\n",line.c_str());
			    evhttp_send_reply_chunk(req, buf);	//块传输
            }
        }else{
			    evbuffer_add_printf(buf,"file is not exists!\n");
			    evhttp_send_reply_chunk(req, buf);	//块传输
        }


		evhttp_send_reply_end(req);	//此语句会关闭HTTP连接（如果不调用此函数，将保持HTTP长连接）
		evbuffer_free(buf);

        //log_info("reply to subscriber[%d]: %s",_clientNum,file.c_str());
        log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
		LOG4CPLUS_TRACE(_logger,"reply to subscriber["<<_clientNum<<"]: file["<<file<<"]"<<std::endl);

		if (++_clientNum > 500000)	//如果用户ID超过500000，从0开始标识
			_clientNum = 0;

		if (pthread_mutex_unlock(&_global_mutex) != 0)	//解锁
		{
            log4cplus::NDCContextCreator _context("generic_handler");       //设置上下文信息
			LOG4CPLUS_FATAL(_logger,"pthread_mutex_unlock error for global_mutex");
			err_sys("pthread_mutex_unlock error for global_mutex");
		}
    
    }
}
