#include "LogStream.h"
#include <algorithm>
#include <limits>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;

// From muduo
template<typename T>
size_t convert(char buf[], T value) {// 将T(十进制整型)类型转换成string
	T i = value;
	char* p = buf;

	do {
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = zero[lsd];
	} while (i != 0);

	if (value < 0) {
		*p++ = '-';		//为负数
	}
	*p = '\0';
	std::reverse(buf, p);		//反转容器中两个迭代器之间的内容

	return p - buf;
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

template<typename T>
void LogStream::formatInteger(T v) {		// 将整型的数格式化成字符串添加到buff中
	// buffer容不下kMaxNumericSize个字符的话会被直接丢弃
	if (buffer_.avail() >= kMaxNumericSize) {
		size_t len = convert(buffer_.current(), v);
		buffer_.add(len);				//typedef FixedBuffer<kSmallBuffer> Buffer;   buffer_是一个Buffer对象
										//add,avail是自定义函数
	}
}

/*
LogStream& operator<<(bool v)
	{
		buffer_.append(v ? "1" : "0", 1);
		return *this;
	}
*/


LogStream& LogStream::operator<<(short v) {
	*this << static_cast<int>(v);			//将v转换成int，并输出到流中？
									//是这样的，将v转换成int，然后调用重载好的 "operator<<(bool v)",将v 逐位地append到成员buffer_中
									//或者是调用“operator<<(int v)”,  其实应该是调用这个的... 结果还是append到成员buffer_
	return *this;			//return *this返回的是当前对象的克隆或者本身（若返回类型为A， 则是克隆， 若返回类型为A&， 则是本身 ）。
							//return this返回当前对象的地址（指向当前对象的指针）
}

LogStream& LogStream::operator<<(unsigned short v)
{
	*this << static_cast<unsigned int>(v);		//实际上是调用*this的<<函数了， 并且传给<<函数的参数为static_cast<unsigned int>(v)
												//可参考调用等号运算符的情况：*this=another;
	return *this;
}


LogStream& LogStream::operator<<(int v)
{
	formatInteger(v);			//formatInteger是个泛型函数，最终将v转换成string追加到buffer_中
	return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
	formatInteger(v);
	return *this;
}
/*
LogStream& LogStream::operator<<(const void* p)
{
	uintptr_t v = reinterpret_cast<uintptr_t>(p);
	if (buffer_.avail() >= kMaxNumericSize)
	    {
	          char* buf = buffer_.current();
		  buf[0] = '0';
		  buf[1] = 'x';
		  size_t len = convertHex(buf+2, v);
		  buffer_.add(len+2);
	    }
	return *this;
}
*/
LogStream& LogStream::operator<<(double v) {			//前面都是可转为整数，double型的要再写
	if (buffer_.avail() >= kMaxNumericSize) {
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
		buffer_.add(len);
	}
	return *this;
}

LogStream& LogStream::operator<<(long double v)
{
	if (buffer_.avail() >= kMaxNumericSize)
	{
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12Lg", v);
		buffer_.add(len);
	}
	return *this;
}
