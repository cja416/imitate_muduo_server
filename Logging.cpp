#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>  
#include <sys/time.h> 

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging* AsyncLogger_;		//��̬ͬ������ָ��

std::string Logger::logFileName_= "/linya_WebServer.log";

void once_init() {
	AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
	AsyncLogger_->start();										//��ʼ����־�ļ���ʱ��������־��running_=true����������־�̲߳�������־�̣߳�
}

void output(const char* msg, int len) {				//ÿ�ε���LOG������Loggerʱ�������output�����д��
	pthread_once(&once_control_, once_init);		//��ֻ֤��һ��once_init���õ����������־�ļ���ַ
	AsyncLogger_->append(msg, len);					//���������д����ͨǰ�˺�˵�һ��ͬ������
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
	gettimeofday(&tv, NULL);		//��ȡ��ǰʱ��
	time = tv.tv_sec;
	struct tm* p_time = localtime(&time);
	strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
	stream_ << str_t;				//�����stream_,һ��LogStream���󡣣�������һ���<<���Ǹ��ࣩ
}

Logger::Logger(const char *fileName, int line)
	: impl_(fileName, line)
{ }

Logger::~Logger()
{
	impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';			//����impl_.basename_��impl_.line_�����ǹ���ʱ���������ļ���������
	const LogStream::Buffer& buf(stream().buffer());				//����Buffer����ʵ��������Buffer�����buffer_append��
	output(buf.data(), buf.length());					//��ͬ������AsyncLogger_��ǰ�˻�����д						һ��typedef FixedBuffer<kSmallBuffer> Buffer��ʵ���ϻ���char data_[SIZE];
}

