#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(string filename)		//打开传入的文件并指定文件流缓冲区（用户）
	:fp_(fopen(filename.c_str(), "ae"))
{
	// 用户提供缓冲区
	//在打开文件流后，读取内容之前，调用setbuffer（）可用来设置文件流的缓冲区。
	//参数stream为指定的文件流，参数buf指向自定的缓冲区起始地址，参数size为缓冲区大小。
	setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
	fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len)
{
	size_t n = this->write(logline, len);		//向缓冲区内（构造函数中指定的）写入n字节
	size_t remain = len - n;
	while (remain > 0) {		//循环直到发送完成
		size_t x = this->write(logline + n, remain);
		if (x == 0) {
			int err = ferror(fp_);
			if(err)
				fprintf(stderr, "AppendFile::append() failed !\n");
			break;
		}
		n += x;
		remain = len - n;
	}
}

void AppendFile::flush() {
	fflush(fp_);	//fflush()会强迫将缓冲区内的数据写回参数stream 指定的文件中. 如果参数stream 为NULL,fflush()会将所有打开的文件数据更新.
}


//为了快速，使用unlocked(无锁)的fwrite函数。平时我们使用的C语言IO函数，都是线程安全的，
//为了做到线程安全，会在函数的内部加锁,这会拖慢速度.而对于这个类,可以保证从
//始到终只有一个线程能访问，所以无需进行加锁操作。

size_t AppendFile::write(const char* logline, size_t len) {
	return fwrite_unlocked(logline, 1, len, fp_);
}