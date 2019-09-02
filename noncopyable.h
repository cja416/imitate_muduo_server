#pragma once

class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;
private:
	noncopyable(const noncopyable&);
	const noncopyable& operator=(const noncopyable&);
};