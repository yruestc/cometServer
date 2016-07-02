/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "log.h"

static Logger logger;

int log_open(FILE * fp, int level, bool is_threadsafe)
{				//参数1:文件描述符，参数2:日志级别，参数3:是否线程安全
	return logger.open(fp, level, is_threadsafe);
}

int log_open(const char *filename, int level, bool is_threadsafe,
	     uint64_t rotate_size)
{				//文件名，日志级别，是否线程安全，单个日志文件大小
	return logger.open(filename, level, is_threadsafe, rotate_size);
}

int log_level()
{
	return logger.level();	//返回日志级别
}

void set_log_level(int level)
{
	logger.set_level(level);	//设置日志级别
}

int get_level_by_name(const char *level_name)	//根据级别名字返回级别的值
{
	return logger.get_level(level_name);
}

int log_write(int level, const char *fmt, ...)
{				//间接调用logv函数写入日志文件
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(level, fmt, ap);
	va_end(ap);
	return ret;
}

/*****/

Logger::Logger()
{				//Logger类的默认构造函数
	fp = stdout;		//默认是将信息写往标准输出
	level_ = LEVEL_DEBUG;	//默认日志级别是调试级别
	mutex = NULL;		//默认是不加互斥锁的，即非线程安全的

	filename[0] = '\0';	//初始化文件名为空
	rotate_size = 0;	//初始化单个日志文件大小为0
	stats.w_curr = 0;	//初始化当前偏移量为0
	stats.w_total = 0;	//初始化总的偏移量为0
}

Logger::~Logger()
{				//析构函数
	if (mutex) {		//如果加了互斥锁，那么释放互斥锁
		pthread_mutex_destroy(mutex);
		free(mutex);
	}
	this->close();		//并且调用close函数
}

void Logger::threadsafe()
{				//保证线程安全函数
	if (mutex) {		//释放之前存在的互斥锁
		pthread_mutex_destroy(mutex);
		free(mutex);
		mutex = NULL;
	}
	mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);	//重新分配并初始化一把互斥锁
}

int Logger::open(FILE * fp, int level, bool is_threadsafe)
{				//以文件描述符方式打开日志文件
	this->fp = fp;		//设置Logger类的成员：日志文件描述符
	this->level_ = level;	//设置日志的级别
	if (is_threadsafe) {
		this->threadsafe();	//如果是线程安全的，那么分配一把锁，这主要是用于在多线程的环境下，单线程的环境下不需要
	}
	return 0;
}

int Logger::open(const char *filename, int level, bool is_threadsafe,
		 uint64_t rotate_size)
{				//文件名，日志级别，是否线程安全，单个日志文件大小
	if (strlen(filename) > PATH_MAX - 20) {	//判断日志文件名是否过长
		fprintf(stderr, "log filename too long!");
		return -1;
	}
	strcpy(this->filename, filename);	//将日志文件名保存到Logger类的私有成员中：日志文件名

	FILE *fp;
	if (strcmp(filename, "stdout") == 0) {	//将信息写入标准输出
		fp = stdout;
	} else if (strcmp(filename, "stderr") == 0) {	//将信息写入标准错误输出
		fp = stderr;
	} else {
		fp = fopen(filename, "a");	//以追加方式打开一个文件
		if (fp == NULL) {
			return -1;
		}

		struct stat st;
		int ret = fstat(fileno(fp), &st);	//获取文件结构信息
		if (ret == -1) {
			fprintf(stderr, "fstat log file %s error!", filename);
			return -1;
		} else {
			this->rotate_size = rotate_size;	//设置单个日志文件的大小
			stats.w_curr = st.st_size;	//设置当前写入文件的偏移量
		}
	}
	return this->open(fp, level, is_threadsafe);	//设置其他参数（日志级别，是否线程安全而分配一把互斥锁）
}

void Logger::close()
{
	if (fp != stdin && fp != stdout) {	//如果写入的对象就是一个文件，那么关闭该文件描述符，并不会删除该文件
		fclose(fp);
	}
}

void Logger::rotate()
{				//交替更新日志
	fclose(fp);		//关闭上一个写满的日志文件
	char newpath[PATH_MAX];
	time_t time;
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);	//得到当前时间
	time = tv.tv_sec;
	tm = localtime(&time);	//转换时间格式
	sprintf(newpath, "%s.%04d%02d%02d-%02d%02d%02d", this->filename, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);	//构造当前时间的表达式格式

	//printf("rename %s => %s\n", this->filename, newpath);
	int ret = rename(this->filename, newpath);	//以时间格式重新命名文件
	if (ret == -1) {
		return;
	}
	fp = fopen(this->filename, "a");	//以追加方式打开一个文件
	if (fp == NULL) {
		return;
	}
	stats.w_curr = 0;	//设置在当前文件中的偏移量为0(初始化)
}

int Logger::get_level(const char *levelname)
{				//根据级别的名字获取日志级别，这主要是用于从配置文件中读取日志级别时会利用到
	if (strcmp("trace", levelname) == 0) {
		return LEVEL_TRACE;
	}
	if (strcmp("debug", levelname) == 0) {
		return LEVEL_DEBUG;
	}
	if (strcmp("info", levelname) == 0) {
		return LEVEL_INFO;
	}
	if (strcmp("warn", levelname) == 0) {
		return LEVEL_WARN;
	}
	if (strcmp("error", levelname) == 0) {
		return LEVEL_ERROR;
	}
	if (strcmp("fatal", levelname) == 0) {
		return LEVEL_FATAL;
	}
	if (strcmp("none", levelname) == 0) {
		return LEVEL_NONE;
	}
	return LEVEL_DEBUG;
}

inline static const char *level_name(int level)
{				//根据日志级别返回级别对应的名字
	switch (level) {
	case Logger::LEVEL_FATAL:
		return "[FATAL] ";
	case Logger::LEVEL_ERROR:
		return "[ERROR] ";
	case Logger::LEVEL_WARN:
		return "[WARN ] ";
	case Logger::LEVEL_INFO:
		return "[INFO ] ";
	case Logger::LEVEL_DEBUG:
		return "[DEBUG] ";
	case Logger::LEVEL_TRACE:
		return "[TRACE] ";
	}
	return "";
}

#define LEVEL_NAME_LEN	8
#define LOG_BUF_LEN		4096

int Logger::logv(int level, const char *fmt, va_list ap)
{
	if (logger.level_ < level) {	//未设置日志级别
		return 0;
	}

	char buf[LOG_BUF_LEN];	//缓冲区大小为4KB
	int len;
	char *ptr = buf;	//指向日志缓冲区的指针

	time_t time;
	struct timeval tv;
	struct tm *tm;
	gettimeofday(&tv, NULL);	//获取当前时间(自1970年1月1日以来的秒数)
	time = tv.tv_sec;
	tm = localtime(&time);	//转换时间格式
	/* %3ld 在数值位数超过3位的时候不起作用, 所以这里转成int */
	len = sprintf(ptr, "%04d-%02d-%02d %02d:%02d:%02d.%03d ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, (int)(tv.tv_usec / 1000));	//写入日志消息之前，先写入消息产生的时间
	if (len < 0) {
		return -1;
	}
	ptr += len;		//指针往后偏移

	memcpy(ptr, level_name(level), LEVEL_NAME_LEN);	//时间之后写入的是日志的级别(8字节的长度，计算过的，可以容纳最长的那个)
	ptr += LEVEL_NAME_LEN;	//指针往后偏移8字节

	int space = sizeof(buf) - (ptr - buf) - 10;	//缓冲区剩余的空间大小
	len = vsnprintf(ptr, space, fmt, ap);	//将可变参数写入日志级别之后
	if (len < 0) {
		return -1;
	}
	ptr += len > space ? space : len;	//最多偏移到文件末端
	*ptr++ = '\n';		//字符串尾字符是换行
	*ptr = '\0';		//字符串以null字节终止

	len = ptr - buf;	//计算写入消息后的偏移量
	// change to write(), without locking?
	if (this->mutex) {
		pthread_mutex_lock(this->mutex);	//如果是线程安全的，那么加锁
	}
	fwrite(buf, len, 1, this->fp);	//将之前写入缓冲区的数据写入到日志文件中去
	fflush(this->fp);	//强制冲洗数据进入日志文件

	stats.w_curr += len;	//设置对于当前文件的偏移量
	stats.w_total += len;	//设置对于所有文件总和的偏移量
	if (rotate_size > 0 && stats.w_curr > rotate_size) {	//如果当前日志文件的长度超出了单个日志文件的长度，那么再次调用此函数进行日志的交替
		this->rotate();
	}
	if (this->mutex) {
		pthread_mutex_unlock(this->mutex);	//写操作结束后解锁
	}

	return len;		//返回此次写入的消息字节数
}

int Logger::trace(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_TRACE, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::debug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_DEBUG, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_INFO, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_WARN, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_ERROR, fmt, ap);
	va_end(ap);
	return ret;
}

int Logger::fatal(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int ret = logger.logv(Logger::LEVEL_FATAL, fmt, ap);
	va_end(ap);
	return ret;
}
