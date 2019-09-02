#include "LogFile.h"
#include "FileUtil.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace std;

LogFile::LogFile(const string& basename, int flushEveryN)
	:basename_(basename),
	flushEveryN_(flushEveryN),
	count_(0),
	mutex_(new MutexLock)
{
	//assert(basename.find('/') >= 0);
	file_.reset(new AppendFile(basename));		//file_指向AppendFile对象从而指向打开的文件
}

LogFile::~LogFile()
{ }

void LogFile::flush() {
	MutexLockGuard lock(*mutex_);
	file_->flush();
}

void LogFile::append(const char* logline, int len)
{
	MutexLockGuard lock(*mutex_);
	append_unlocked(logline, len);
}

void LogFile::append_unlocked(const char* logline, int len) {
	file_->append(logline, len);	//往缓冲区写N次，再flush进文件中
	++count_;
	if (count_ >= flushEveryN_) {
		count_ = 0;
		file_->flush();
	}
}