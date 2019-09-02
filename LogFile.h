#pragma once
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"
#include <memory>
#include <string>

// TODO �ṩ�Զ��鵵����
class LogFile : noncopyable
{
public:
	// ÿ��append flushEveryN�Σ�flushһ�£������ļ�д��ֻ�������ļ�Ҳ�Ǵ���������
	LogFile(const std::string& basename, int flushEveryN = 1024);
	~LogFile();

	void append(const char* logline, int len);
	void flush();
	bool rollFile();

private:
	void append_unlocked(const char* logline, int len);

	const std::string basename_;
	const int flushEveryN_;

	int count_;
	std::unique_ptr<MutexLock> mutex_;
	std::unique_ptr<AppendFile> file_;
};