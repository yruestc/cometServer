/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef UTIL_LOG_H
#define UTIL_LOG_H

#include <inttypes.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

class Logger {			//日志类(维护日志文件信息)
      public:
	static const int LEVEL_NONE = (-1);
	static const int LEVEL_MIN = 0;
	static const int LEVEL_FATAL = 0;
	static const int LEVEL_ERROR = 1;
	static const int LEVEL_WARN = 2;
	static const int LEVEL_INFO = 3;
	static const int LEVEL_DEBUG = 4;
	static const int LEVEL_TRACE = 5;
	static const int LEVEL_MAX = 5;

	static int get_level(const char *levelname);	//根据级别名字返回级别的值
      private:
	 FILE * fp;		//日志文件描述符
	char filename[PATH_MAX];	//日志文件名
	int level_;		//日志级别
	pthread_mutex_t *mutex;	//互斥锁

	uint64_t rotate_size;	//单个日志文件的大小(一旦超过这个文件大小就新创建一个同名的文件)
	struct {
		uint64_t w_curr;
		uint64_t w_total;
	} stats;		//文件状态

	void rotate();		//日志交替更新
	void threadsafe();	//线程安全函数，即分配一把互斥锁,Mutex就不为空,在需要加锁处就能成功加锁
      public:
	 Logger();		//默认构造函数
	~Logger();		//析构函数

	int level() {		//返回level_的值
		return level_;
	} void set_level(int level) {	//设置日志文件的级别
		this->level_ = level;
	}

	int open(FILE * fp, int level = LEVEL_DEBUG, bool is_threadsafe = false);	//以文件描述符的方式打开日志文件(3个参数)
	int open(const char *filename, int level = LEVEL_DEBUG, bool is_threadsafe = false, uint64_t rotate_size = 0);	//以文件路径的方式打开文件(4个参数)
	void close();

	int logv(int level, const char *fmt, va_list ap);

	int trace(const char *fmt, ...);
	int debug(const char *fmt, ...);
	int info(const char *fmt, ...);
	int warn(const char *fmt, ...);
	int error(const char *fmt, ...);
	int fatal(const char *fmt, ...);
};

int log_open(FILE * fp, int level = Logger::LEVEL_DEBUG, bool is_threadsafe = false);	//以下函数间接调用的是logger类的成员函数
int log_open(const char *filename, int level = Logger::LEVEL_DEBUG,
	     bool is_threadsafe = false, uint64_t rotate_size = 0);
int log_level();		//返回日志文件级别
void set_log_level(int level);	//设置日志文件级别
int log_write(int level, const char *fmt, ...);	//写入日志文件
int get_level_by_name(const char *);	//根据级别名字返回级别的值

//以下的宏定义以不同的日志级别调用log_write
#ifdef NDEBUG
#define log_trace(fmt, args...) do{}while(0)
#else
#define log_trace(fmt, args...)	\
		log_write(Logger::LEVEL_TRACE, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#endif

#define log_debug(fmt, args...)	\
	log_write(Logger::LEVEL_DEBUG, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_info(fmt, args...)	\
	log_write(Logger::LEVEL_INFO,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_warn(fmt, args...)	\
	log_write(Logger::LEVEL_WARN,  "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_error(fmt, args...)	\
	log_write(Logger::LEVEL_ERROR, "%s(%d): " fmt, __FILE__, __LINE__, ##args)
#define log_fatal(fmt, args...)	\
	log_write(Logger::LEVEL_FATAL, "%s(%d): " fmt, __FILE__, __LINE__, ##args)

#endif
