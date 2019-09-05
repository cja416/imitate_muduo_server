#include "AsyncLogging.h"
#include "LogFile.h"
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <functional>

AsyncLogging::AsyncLogging(std::string logFileName_,int flushInterval)
	:flushInterval_(flushInterval),				//默认值为2
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
	nextBuffer_->bzero();			//即memset
	buffers_.reserve(16);		//一个存放 指向Buffer的智能指针 的vector
}

void AsyncLogging::append(const char* logline, int len) {
	MutexLockGuard lock(mutex_);		//传进去会调用MutexLock的构造函数进行初始化，然后上锁
	if (currentBuffer_->avail() > len)
		currentBuffer_->append(logline, len);	//底层实际上memcpy
	else {										//否则，表示当前这个缓冲区装不下了，换备用的或者新建
		buffers_.push_back(currentBuffer_);		//先push进去
		currentBuffer_.reset();
		if (nextBuffer_)
			currentBuffer_ = std::move(nextBuffer_);	//使用备用buffer（移动语义即可清空备用buffer）
		else
			currentBuffer_.reset(new Buffer);		//到这说明备用buffer不能用了
		currentBuffer_->append(logline, len);
		cond_.notify();							//唤醒谁呢？LOG线程
	}
}

void AsyncLogging::threadFunc() {
	assert(running_ == true);
	latch_.countDown();
	LogFile output(basename_);
	BufferPtr newBuffer1(new Buffer);			//指向FixedBuffer<4000000>对象的智能指针，其底层是一个char data_[4000000]，都是new在堆上的
	BufferPtr newBuffer2(new Buffer);
	newBuffer1->bzero();
	newBuffer2->bzero();
	BufferVector buffersToWrite;
	buffersToWrite.reserve(16);
	while (running_)						//在哪里会改变这个running值？			一旦log线程创建完毕，他就会一直循环（后端往文件里写）
	{												//初始化日志文件名时，开启日志（running_=true）			（至于前端往后端写是在各个IO线程里）
		assert(newBuffer1 && newBuffer1->length() == 0);
		assert(newBuffer2 && newBuffer2->length() == 0);
		assert(buffersToWrite.empty());

		{
			MutexLockGuard lock(mutex_);
			if (buffers_.empty()) // unusual usage!			buffers_一个BufferVector的vector，	
			{
				cond_.waitForSeconds(flushInterval_);			//日志为空的话等待几秒钟，此处的条件变量就是锁
			}
			buffers_.push_back(currentBuffer_);
			currentBuffer_.reset();

			currentBuffer_ = std::move(newBuffer1);				//当前前端缓冲区push进后端的待写vector中，并清空
			buffersToWrite.swap(buffers_);							//拿到后端待写vector
			if (!nextBuffer_) {
				nextBuffer_ = std::move(newBuffer2);		//移动拷贝给备用buffer赋值
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
			buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());	//只留前面两个
		}

		for (size_t i = 0; i < buffersToWrite.size(); ++i) {
			// FIXME: use unbuffered stdio FILE ? or use ::writev ?
			output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());	//一个LogFile对象
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
			newBuffer1.reset();		//reset是将此对象的引用计数-1  ,若引用计数为0，则调用其析构函数
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
	output.flush();		//往日志中写,实际上LogFile类包含一个AppendFile对象，此对象的成员函数才是真正往文件中写的。
}
