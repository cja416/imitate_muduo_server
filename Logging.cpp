#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>  
#include <sys/time.h> 

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging* AsyncLogger_;		//静态同步对象指针

std::string Logger::logFileName_= "/linya_WebServer.log";

void once_init() {
	AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
	AsyncLogger_->start();										//初始化日志文件名时，开启日志（running_=true）（创建日志线程并开启日志线程）
}

void output(const char* msg, int len) {				//每次调用LOG，析构Logger时都会调用output往后端写，
	pthread_once(&once_control_, once_init);		//保证只做一次once_init，得到欲保存的日志文件地址
	AsyncLogger_->append(msg, len);					//往这个对象写（联通前端后端的一个同步对象）
}

Logger::Impl::Impl(const char*fileName, int line)
	:stream_(),
	line_(line),
	basename_(fileName)
{
	formatTime();
}

void Logger::Impl::formatTime() {
	struct timeval tv;
	time_t time;
	char str_t[26] = { 0 };
	gettimeofday(&tv, NULL);		//获取当前时间
	time = tv.tv_sec;
	struct tm* p_time = localtime(&time);
	strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
	stream_ << str_t;				//输出到stream_,一个LogStream对象。（重载了一大堆<<的那个类）
}

Logger::Logger(const char *fileName, int line)
	: impl_(fileName, line)
{ }

Logger::~Logger()
{
	impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';			//其中impl_.basename_和impl_.line_是我们构造时传进来的文件名和行数
	const LogStream::Buffer& buf(stream().buffer());				//构造Buffer对象，实际上是往Buffer对象的buffer_append，
	output(buf.data(), buf.length());					//往同步对象AsyncLogger_的前端缓冲区写						一个typedef FixedBuffer<kSmallBuffer> Buffer，实际上还是char data_[SIZE];
}

