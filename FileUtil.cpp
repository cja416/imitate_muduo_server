#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

AppendFile::AppendFile(string filename)		//�򿪴�����ļ���ָ���ļ������������û���
	:fp_(fopen(filename.c_str(), "ae"))
{
	// �û��ṩ������
	//�ڴ��ļ����󣬶�ȡ����֮ǰ������setbuffer���������������ļ����Ļ�������
	//����streamΪָ�����ļ���������bufָ���Զ��Ļ�������ʼ��ַ������sizeΪ��������С��
	setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
	fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len)
{
	size_t n = this->write(logline, len);		//�򻺳����ڣ����캯����ָ���ģ�д��n�ֽ�
	size_t remain = len - n;
	while (remain > 0) {		//ѭ��ֱ���������
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
	fflush(fp_);	//fflush()��ǿ�Ƚ��������ڵ�����д�ز���stream ָ�����ļ���. �������stream ΪNULL,fflush()�Ὣ���д򿪵��ļ����ݸ���.
}


//Ϊ�˿��٣�ʹ��unlocked(����)��fwrite������ƽʱ����ʹ�õ�C����IO�����������̰߳�ȫ�ģ�
//Ϊ�������̰߳�ȫ�����ں������ڲ�����,��������ٶ�.�����������,���Ա�֤��
//ʼ����ֻ��һ���߳��ܷ��ʣ�����������м���������

size_t AppendFile::write(const char* logline, size_t len) {
	return fwrite_unlocked(logline, 1, len, fp_);
}