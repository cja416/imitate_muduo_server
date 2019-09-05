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
size_t convert(char buf[], T value) {// ��T(ʮ��������)����ת����string
	T i = value;
	char* p = buf;

	do {
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = zero[lsd];
	} while (i != 0);

	if (value < 0) {
		*p++ = '-';		//Ϊ����
	}
	*p = '\0';
	std::reverse(buf, p);		//��ת����������������֮�������

	return p - buf;
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

template<typename T>
void LogStream::formatInteger(T v) {		// �����͵�����ʽ�����ַ�����ӵ�buff��
	// buffer�ݲ���kMaxNumericSize���ַ��Ļ��ᱻֱ�Ӷ���
	if (buffer_.avail() >= kMaxNumericSize) {
		size_t len = convert(buffer_.current(), v);
		buffer_.add(len);				//typedef FixedBuffer<kSmallBuffer> Buffer;   buffer_��һ��Buffer����
										//add,avail���Զ��庯��
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
	*this << static_cast<int>(v);			//��vת����int������������У�
									//�������ģ���vת����int��Ȼ��������غõ� "operator<<(bool v)",��v ��λ��append����Աbuffer_��
									//�����ǵ��á�operator<<(int v)��,  ��ʵӦ���ǵ��������... �������append����Աbuffer_
	return *this;			//return *this���ص��ǵ�ǰ����Ŀ�¡���߱�������������ΪA�� ���ǿ�¡�� ����������ΪA&�� ���Ǳ��� ����
							//return this���ص�ǰ����ĵ�ַ��ָ��ǰ�����ָ�룩
}

LogStream& LogStream::operator<<(unsigned short v)
{
	*this << static_cast<unsigned int>(v);		//ʵ�����ǵ���*this��<<�����ˣ� ���Ҵ���<<�����Ĳ���Ϊstatic_cast<unsigned int>(v)
												//�ɲο����õȺ�������������*this=another;
	return *this;
}


LogStream& LogStream::operator<<(int v)
{
	formatInteger(v);			//formatInteger�Ǹ����ͺ��������ս�vת����string׷�ӵ�buffer_��
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
LogStream& LogStream::operator<<(double v) {			//ǰ�涼�ǿ�תΪ������double�͵�Ҫ��д
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
