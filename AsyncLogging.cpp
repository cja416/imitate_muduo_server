#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <functional>

AsyncLogging::AsyncLogging(std::string logFileName_,int flushInterval)
	:flushInterval_(flushInterval),				//Ĭ��ֵΪ2
	running_(false),
	basename_(logFileName_),
	thread_(std::bind(&AsyncLogging::threadFunc,this),"Logging"),
	mutex_(),
	cond_(mutex_),
	currentBuffer_(new Buffer),
	nextBuffer_(new Buffer),
	buffers_(),
	latch_(1)
{
	assert(logFileName_.size() > 1);
	currentBuffer_->bzero();
	nextBuffer_->bzero();			//��memset
	buffers_.reserve(16);		//һ����� ָ��Buffer������ָ�� ��vector
}

void AsyncLogging::append(const char* logline, int len) {
	MutexLockGuard lock(mutex_);		//����ȥ�����MutexLock�Ĺ��캯�����г�ʼ����Ȼ������
	if (currentBuffer_->avail() > len)
		currentBuffer_->append(logline, len);	//�ײ�ʵ����memcpy
	else {										//���򣬱�ʾ��ǰ���������װ�����ˣ������õĻ����½�
		buffers_.push_back(currentBuffer_);		//��push��ȥ
		currentBuffer_.reset();
		if (nextBuffer_)
			currentBuffer_ = std::move(nextBuffer_);	//ʹ�ñ���buffer���ƶ����弴����ձ���buffer��
		else
			currentBuffer_.reset(new Buffer);		//����˵������buffer��������
		currentBuffer_->append(logline, len);
		cond_.notify();							//����˭�أ�LOG�߳�
	}
}

void AsyncLogging::threadFunc() {
	assert(running_ == true);
	latch_.countDown();
	LogFile output(basename_);
	BufferPtr newBuffer1(new Buffer);			//ָ��FixedBuffer<4000000>���������ָ�룬��ײ���һ��char data_[4000000]������new�ڶ��ϵ�
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while (running_)						//�������ı����runningֵ��			һ��log�̴߳�����ϣ����ͻ�һֱѭ����������ļ���д��
	{												//��ʼ����־�ļ���ʱ��������־��running_=true��			������ǰ�������д���ڸ���IO�߳��
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());

		{
			MutexLockGuard lock(mutex_);
			if (buffers_.empty()) // unusual usage!			buffers_һ��BufferVector��vector��	
			{
				cond_.waitForSeconds(flushInterval_);			//��־Ϊ�յĻ��ȴ������ӣ��˴�����������������
			}
			buffers_.push_back(currentBuffer_);
			currentBuffer_.reset();

			currentBuffer_ = std::move(newBuffer1);				//��ǰǰ�˻�����push����˵Ĵ�дvector�У������
			buffersToWrite.swap(buffers_);							//�õ���˴�дvector
			if (!nextBuffer_) {
				nextBuffer_ = std::move(newBuffer2);		//�ƶ�����������buffer��ֵ
			}
		}

		assert(!buffersToWrite.empty());

		if (buffersToWrite.size() > 25) {
			//char buf[256];
			// snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
			//          Timestamp::now().toFormattedString().c_str(),
			//          buffersToWrite.size()-2);
			//fputs(buf, stderr);
			//output.append(buf, static_cast<int>(strlen(buf)));
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());	//ֻ��ǰ������
		}

		for (size_t i = 0; i < buffersToWrite.size(); ++i) {
			// FIXME: use unbuffered stdio FILE ? or use ::writev ?
			output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());	//һ��LogFile����
		}

		if (buffersToWrite.size() > 2)
		{
			// drop non-bzero-ed buffers, avoid trashing
			buffersToWrite.resize(2);
		}

		if (!newBuffer1) {
			assert(!buffersToWrite.empty());
			newBuffer1 = buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer1.reset();		//reset�ǽ��˶�������ü���-1  ,�����ü���Ϊ0�����������������
		}

		if (!newBuffer2)
		{
			assert(!buffersToWrite.empty());
			newBuffer2 = buffersToWrite.back();
			buffersToWrite.pop_back();
			newBuffer2->reset();
		}

		buffersToWrite.clear();
		output.flush();
	}
	output.flush();		//����־��д,ʵ����LogFile�����һ��AppendFile���󣬴˶���ĳ�Ա���������������ļ���д�ġ�
}
