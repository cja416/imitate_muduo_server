#include "HttpData.h"
#include "time.h"
#include "Channel.h"
#include "Util.h"
#include "EventLoop.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>
using namespace std;


pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;			//多线程情况下只做一次
std::unordered_map<std::string, std::string>MimeType::mime;		//再次声明

//一些默认参数
const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000; // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms			5分钟

char favicon[555] = {
  '\x89', 'P', 'N', 'G', '\xD', '\xA', '\x1A', '\xA',
  '\x0', '\x0', '\x0', '\xD', 'I', 'H', 'D', 'R',
  '\x0', '\x0', '\x0', '\x10', '\x0', '\x0', '\x0', '\x10',
  '\x8', '\x6', '\x0', '\x0', '\x0', '\x1F', '\xF3', '\xFF',
  'a', '\x0', '\x0', '\x0', '\x19', 't', 'E', 'X',
  't', 'S', 'o', 'f', 't', 'w', 'a', 'r',
  'e', '\x0', 'A', 'd', 'o', 'b', 'e', '\x20',
  'I', 'm', 'a', 'g', 'e', 'R', 'e', 'a',
  'd', 'y', 'q', '\xC9', 'e', '\x3C', '\x0', '\x0',
  '\x1', '\xCD', 'I', 'D', 'A', 'T', 'x', '\xDA',
  '\x94', '\x93', '9', 'H', '\x3', 'A', '\x14', '\x86',
  '\xFF', '\x5D', 'b', '\xA7', '\x4', 'R', '\xC4', 'm',
  '\x22', '\x1E', '\xA0', 'F', '\x24', '\x8', '\x16', '\x16',
  'v', '\xA', '6', '\xBA', 'J', '\x9A', '\x80', '\x8',
  'A', '\xB4', 'q', '\x85', 'X', '\x89', 'G', '\xB0',
  'I', '\xA9', 'Q', '\x24', '\xCD', '\xA6', '\x8', '\xA4',
  'H', 'c', '\x91', 'B', '\xB', '\xAF', 'V', '\xC1',
  'F', '\xB4', '\x15', '\xCF', '\x22', 'X', '\x98', '\xB',
  'T', 'H', '\x8A', 'd', '\x93', '\x8D', '\xFB', 'F',
  'g', '\xC9', '\x1A', '\x14', '\x7D', '\xF0', 'f', 'v',
  'f', '\xDF', '\x7C', '\xEF', '\xE7', 'g', 'F', '\xA8',
  '\xD5', 'j', 'H', '\x24', '\x12', '\x2A', '\x0', '\x5',
  '\xBF', 'G', '\xD4', '\xEF', '\xF7', '\x2F', '6', '\xEC',
  '\x12', '\x20', '\x1E', '\x8F', '\xD7', '\xAA', '\xD5', '\xEA',
  '\xAF', 'I', '5', 'F', '\xAA', 'T', '\x5F', '\x9F',
  '\x22', 'A', '\x2A', '\x95', '\xA', '\x83', '\xE5', 'r',
  '9', 'd', '\xB3', 'Y', '\x96', '\x99', 'L', '\x6',
  '\xE9', 't', '\x9A', '\x25', '\x85', '\x2C', '\xCB', 'T',
  '\xA7', '\xC4', 'b', '1', '\xB5', '\x5E', '\x0', '\x3',
  'h', '\x9A', '\xC6', '\x16', '\x82', '\x20', 'X', 'R',
  '\x14', 'E', '6', 'S', '\x94', '\xCB', 'e', 'x',
  '\xBD', '\x5E', '\xAA', 'U', 'T', '\x23', 'L', '\xC0',
  '\xE0', '\xE2', '\xC1', '\x8F', '\x0', '\x9E', '\xBC', '\x9',
  'A', '\x7C', '\x3E', '\x1F', '\x83', 'D', '\x22', '\x11',
  '\xD5', 'T', '\x40', '\x3F', '8', '\x80', 'w', '\xE5',
  '3', '\x7', '\xB8', '\x5C', '\x2E', 'H', '\x92', '\x4',
  '\x87', '\xC3', '\x81', '\x40', '\x20', '\x40', 'g', '\x98',
  '\xE9', '6', '\x1A', '\xA6', 'g', '\x15', '\x4', '\xE3',
  '\xD7', '\xC8', '\xBD', '\x15', '\xE1', 'i', '\xB7', 'C',
  '\xAB', '\xEA', 'x', '\x2F', 'j', 'X', '\x92', '\xBB',
  '\x18', '\x20', '\x9F', '\xCF', '3', '\xC3', '\xB8', '\xE9',
  'N', '\xA7', '\xD3', 'l', 'J', '\x0', 'i', '6',
  '\x7C', '\x8E', '\xE1', '\xFE', 'V', '\x84', '\xE7', '\x3C',
  '\x9F', 'r', '\x2B', '\x3A', 'B', '\x7B', '7', 'f',
  'w', '\xAE', '\x8E', '\xE', '\xF3', '\xBD', 'R', '\xA9',
  'd', '\x2', 'B', '\xAF', '\x85', '2', 'f', 'F',
  '\xBA', '\xC', '\xD9', '\x9F', '\x1D', '\x9A', 'l', '\x22',
  '\xE6', '\xC7', '\x3A', '\x2C', '\x80', '\xEF', '\xC1', '\x15',
  '\x90', '\x7', '\x93', '\xA2', '\x28', '\xA0', 'S', 'j',
  '\xB1', '\xB8', '\xDF', '\x29', '5', 'C', '\xE', '\x3F',
  'X', '\xFC', '\x98', '\xDA', 'y', 'j', 'P', '\x40',
  '\x0', '\x87', '\xAE', '\x1B', '\x17', 'B', '\xB4', '\x3A',
  '\x3F', '\xBE', 'y', '\xC7', '\xA', '\x26', '\xB6', '\xEE',
  '\xD9', '\x9A', '\x60', '\x14', '\x93', '\xDB', '\x8F', '\xD',
  '\xA', '\x2E', '\xE9', '\x23', '\x95', '\x29', 'X', '\x0',
  '\x27', '\xEB', 'n', 'V', 'p', '\xBC', '\xD6', '\xCB',
  '\xD6', 'G', '\xAB', '\x3D', 'l', '\x7D', '\xB8', '\xD2',
  '\xDD', '\xA0', '\x60', '\x83', '\xBA', '\xEF', '\x5F', '\xA4',
  '\xEA', '\xCC', '\x2', 'N', '\xAE', '\x5E', 'p', '\x1A',
  '\xEC', '\xB3', '\x40', '9', '\xAC', '\xFE', '\xF2', '\x91',
  '\x89', 'g', '\x91', '\x85', '\x21', '\xA8', '\x87', '\xB7',
  'X', '\x7E', '\x7E', '\x85', '\xBB', '\xCD', 'N', 'N',
  'b', 't', '\x40', '\xFA', '\x93', '\x89', '\xEC', '\x1E',
  '\xEC', '\x86', '\x2', 'H', '\x26', '\x93', '\xD0', 'u',
  '\x1D', '\x7F', '\x9', '2', '\x95', '\xBF', '\x1F', '\xDB',
  '\xD7', 'c', '\x8A', '\x1A', '\xF7', '\x5C', '\xC1', '\xFF',
  '\x22', 'J', '\xC3', '\x87', '\x0', '\x3', '\x0', 'K',
  '\xBB', '\xF8', '\xD6', '\x2A', 'v', '\x98', 'I', '\x0',
  '\x0', '\x0', '\x0', 'I', 'E', 'N', 'D', '\xAE',
  'B', '\x60', '\x82',
};



//设置Content-Type：用于定义用户的浏览器或相关设备如何显示将要加载的数据，或者如何处理将要加载的数据。
void MimeType::init() {
	mime[".html"] = "text/html";	//unorder_map的一系列赋值         //文件类型，息实体
	mime[".avi"] = "video/x-msvideo";
	mime[".bmp"] = "image/bmp";
	mime[".c"] = "text/plain";
	mime[".doc"] = "application/msword";
	mime[".gif"] = "image/gif";
	mime[".gz"] = "application/x-gzip";
	mime[".htm"] = "text/html";
	mime[".ico"] = "image/x-icon";
	mime[".jpg"] = "image/jpeg";
	mime[".png"] = "image/png";
	mime[".txt"] = "text/plain";
	mime[".mp3"] = "audio/mp3";
	mime["default"] = "text/html";
}

std::string MimeType::getMime(const std::string &suffix) {     //suffix后缀
	pthread_once(&once_control, MimeType::init);		//保证只初始化一次，只调用一次init函数
	if (mime.find(suffix) == mime.end())
		return mime["default"];
	else
		return mime[suffix];
}

HttpData::HttpData(EventLoop* loop, int connfd)
	:loop_(loop),
	channel_(new Channel(loop, connfd)),			//每个HttpData对象会绑定一个新的channel
	fd_(connfd),
	error_(false),
	connectionState_(H_CONNECTED),
	method_(METHOD_GET),
	HTTPVersion_(HTTP_11),
	nowReadPos_(0),
	state_(STATE_PARSE_URI),
	hState_(H_START),
	keepAlive_(false) 
{
	//loop_->queueInLoop(bind(&HttpData::setHandlers, this));
	channel_->setReadHandler(bind(&HttpData::handleRead, this));
	channel_->setWriteHandler(bind(&HttpData::handleWrite, this));
	channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

void HttpData::reset() {
	//inBuffer_.clear();
	fileName_.clear();
	path_.clear();
	nowReadPos_ = 0;
	state_ = STATE_PARSE_URI;
	hState_ = H_START;
	headers_.clear();
	//keepAlive_ = false;
	if (timer_.lock()) {		//是std::weak_ptr<TimerNode> timer_; 尝试提升  实际上这里可以调用seperateTimer
		shared_ptr<TimerNode> my_timer(timer_.lock());
		my_timer->clearReq();
		timer_.reset();
	}
}

void HttpData::seperateTimer() {		//分离定时器
	//cout << "seperateTimer" << endl;
	if (timer_.lock()) {						//因为timer是weak_ptr<TimerNode> timer_
		shared_ptr<TimerNode> my_timer(timer_.lock());
		my_timer->clearReq();
		timer_.reset();
	}
}


void HttpData::handleRead() {					//对fd的处理主要逻辑就在此函数中
	__uint32_t &events_ = channel_->getEvents();
	do {												//do-while是为了方便一系列出错时的直接break，不用再做下面的工作了
		bool zero = false;
		int read_num = readn(fd_, inBuffer_, zero);		//读完放入应用层缓冲区内
		LOG << "Request: " << inBuffer_;
		if (connectionState_ == H_DISCONNECTING) {
			inBuffer_.clear();			//一个string
			break;
		}
		//cout << inBuffer_ << endl;
		if (read_num < 0) {
			perror("1");			//参数 s 所指的字符串会先打印出，后面再加上错误原因字符串。此错误原因依照全局变量errno的值来决定要输出的字符串。
			error_ = true;
			handleError(fd_, 400, "Bad Request");
			break;
		}
		else if (zero) {		//读到0字节的话zero会被置为true		,将状态置为关闭中，recv到0表示对端关闭了连接
			// 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
			// 最可能是对端已经关闭了，统一按照对端已经关闭处理
			connectionState_ = H_DISCONNECTING;			
			if (read_num == 0)					//zero是在readn中读完最后一个数据后置为true的，是正常。
			{									//但若是read_num总共就读到0字节，说明有请求但是读不到数据。按照错误处理
				//error_ = true;
				break;
			}
			//cout << "readnum == 0" << endl;
		}

		//走到这里说明是有读到数据的
		if (state_ == STATE_PARSE_URI) {		//state一开始初值就是STATE_PARSE_URI
			URIState flag = this->parseURI();	//此函数返回一个URIState,这里进行URI合法性判断
			if (flag == PARSE_URI_AGAIN)
				break;
			else if (flag == PARSE_URI_ERROR) {
				perror("2");
				LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
				inBuffer_.clear();
				error_ = true;
				handleError(fd_, 400, "Bad Request");
			}
			else
				state_ = STATE_PARSE_HEADERS;		    //若走到这，说明是读到有用的，将state设为报文头状态STATE_PARSE_HEADERS
		}

		if (state_ == STATE_PARSE_HEADERS) {		//进行下一步head合法性判断
			HeaderState flag = this->parseHeaders();
			if (flag == PARSE_HEADER_AGAIN)
				break;
			else if (flag == PARSE_HEADER_ERROR) {
				perror("3");
				error_ = true;
				handleError(fd_, 400, "Bad Request");
				break;
			}

			if (method_ = METHOD_POST) {		//进行下一步，请求方法判断
				// POST方法准备
				state_ = STATE_RECV_BODY;
			}
			else {
				state_ = STATE_ANALYSIS;
			}
		}

		if (state_ == STATE_RECV_BODY) {		//请求方式为post情况要处理Content-length
			int content_length = -1;
			if (headers_.find("Content-length") != headers_.end()) {		//一个std::map<std::string, std::string> headers_,在别的处理函数中赋值
				content_length = stoi(headers_["Content-length"]);
			}
			else {
				//cout << "(state_ == STATE_RECV_BODY)" << endl;
				error_ = true;
				handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
				break;
			}
			if (static_cast<int>(inBuffer_.size()) < content_length)
				break;
			state_ = STATE_ANALYSIS;			//也就是说post方法的话要多一步判断content-length
		}

		if (state_ == STATE_ANALYSIS) {			//进行下一步
			AnalysisState flag = this->analysisRequest();			//填写outbuffer
			if (flag == ANALYSIS_SUCCESS) {
				state_ = STATE_FINISH;		//完成工作了
				break;
			}
			else {
				//cout << "state_ == STATE_ANALYSIS" << endl;
				error_ = true;
				break;
			}
		}
	} while (false);
	//cout << "state_=" << state_ << endl;
	if (!error_) {
		if (outBuffer_.size() > 0) {		//输出buf里有内容
			handleWrite();
			//events_ |= EPOLLOUT;
		}
		// error_ may change
		if (!error_ && state_ == STATE_FINISH) {		//处理完输出之后
			this->reset();													//就将此httpdata对象reset，包括他的各种请求信息和状态重置为初始值，定时器置为deleted
			if (inBuffer_.size() > 0) {					//如果输入buf中还有内容，再调用一次本函数，支持管线化
				if (connectionState_ != H_DISCONNECTING)
					handleRead();							
			}
			// if ((keepAlive_ || inBuffer_.size() > 0) && connectionState_ == H_CONNECTED)
			// {
			//     this->reset();
			//     events_ |= EPOLLIN;
			// }
		}
		else if (!error_ && connectionState_ != H_DISCONNECTED)		//走到这个if说明输入buf中无内容
			events_ |= EPOLLIN;
	}
}

void HttpData::handleWrite() {
	if (!error_ && connectionState_ != H_DISCONNECTED) {
		__uint32_t &events_ = channel_->getEvents();
		if (writen(fd_, outBuffer_) < 0) {		//写,将buf中内容往fd_中写
			perror("writen");
			events_ = 0;
			error_ = true;
		}
		if (outBuffer_.size() > 0)		//输出buf中还有内容
			events_ |= EPOLLOUT;
	}
}

void HttpData::handleConn() {
	seperateTimer();
	__uint32_t &events_ = channel_->getEvents();		//httpdata绑定的channel对象
	if (!error_&& connectionState_ == H_CONNECTED) {			//若是已连接状态
		if (events_ != 0) {
			int timeout = DEFAULT_EXPIRED_TIME;
			if (keepAlive_)
				timeout = DEFAULT_KEEP_ALIVE_TIME;		//长连接时间
			if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {		//同时为输入事件和输出事件？？
				events_ = __uint32_t(0);
				events_ |= EPOLLOUT;		//就只把它设置为输出事件,并修改对应channel的fd
			}
			//events_ |= (EPOLLET | EPOLLONESHOT);
			events_ |= EPOLLET;
			loop_->updatePoller(channel_, timeout);						//进来先分离定时器，处理完了要记得update一下fd和定时器
		}
		else if (keepAlive_) {						//当events==0时，走到这里
			events_ |= (EPOLLIN | EPOLLET);		//设为输入事件,et模式
			//events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
			int timeout = DEFAULT_KEEP_ALIVE_TIME;
			loop_->updatePoller(channel_, timeout);
		}
		else {							//当events==0 且 不设置长连接时，走到这里
			//cout << "close normally" << endl;
			// loop_->shutdown(channel_);
			// loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
			events_ |= (EPOLLIN | EPOLLET);
			//events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
			int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);		//默认长连接时间/2
			loop_->updatePoller(channel_, timeout);
		}
	}
	else if (!error_ && connectionState_ == H_DISCONNECTING && (events_ & EPOLLOUT))	//若是正在断开连接状态，要将剩下的数据发完，所以要events要变为out
	{
		events_ = (EPOLLOUT | EPOLLET);
	}
	else {
		//cout << "close with errors" << endl;
		loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));		//断开连接，epoll_del
	}
}



//下面这几个是分析http请求报文的函数
//这里放一个报文的组成：
//＜request - line＞
//＜headers＞
//＜blank line＞
//[＜request - body＞]

/*例子：用telnet 127.0.0.1 80  写请求如下：
GET /0606/01.php HTTP/1.1		请求行
Host: localhost					请求头部
								//空行分割
								请求body为空
*/


/*post方法例子：
POST /0606/02.php HTTP/1.1
Host: localhost
Content-type: application/x-www-form-urlencoded
Content-length: 23				//必须有，告诉服务器我要给你发的请求主体有多少字节

username=zhangsan&age=9			//此处服务器中的资源会被写入两行分别是zhangsan 和9 ,并在响应中返回

*/



URIState HttpData::parseURI() {
	string& str = inBuffer_;		//引用
	string cop = str;
	// 读到完整的请求行再开始解析请求
	size_t pos = str.find('\r', nowReadPos_);	//'\r' 回车，回到当前行的行首，而不会换到下一行，如果接着输出的话，本行以前的内容会被逐一覆盖；
												//windows下'\n' 换行，换到当前位置的下一行，而不会回到行首；
	if (pos < 0) {								//linux下换行\n, 苹果下换行\r,   windows下换行\r\n.   但是http标准是用的\r\n.
		return PARSE_URI_AGAIN;
	}
	// 去掉请求行所占的空间，节省空间
	string request_line = str.substr(0, pos);		//request_line保存请求内容
	if (str.size() > pos + 1)
		str = str.substr(pos + 1);		//截去请求部分
	else
		str.clear();
	//Method			//判断请求的方法
	int posGet = request_line.find("GET");
	int posPost = request_line.find("POST");
	int posHead = request_line.find("HEAD");

	if (posGet >= 0) {
		pos = posGet;
		method_ = METHOD_GET;
	}
	else if (posPost >= 0)
	{
		pos = posPost;
		method_ = METHOD_POST;
	}
	else if (posHead >= 0)
	{
		pos = posHead;
		method_ = METHOD_HEAD;
	}
	else
	{
		return PARSE_URI_ERROR;
	}

	// filename
	pos = request_line.find("/", pos);
	if (pos < 0) {
		fileName_ = "index_html";
		HTTPVersion_ = HTTP_11;
		return PARSE_URI_SUCCESS;
	}
	else {
		size_t _pos = request_line.find(' ', pos);
		if(_pos < 0)
			return PARSE_URI_ERROR;
		else {
			if (_pos - pos > 1) {	//在pos之后找到了空格
				fileName_ = request_line.substr(pos + 1, _pos - pos - 1);//手工截取fileName_,即请求的资源
				size_t __pos = fileName_.find('?');
				if (__pos >= 0)				//有问号的话，后面部分舍去  实际上?后面是的query语句，查询数据库的
				{
					fileName_ = fileName_.substr(0, __pos);
				}
			}

			else
				fileName_ = "index.html";
		}
		pos = _pos;		//能走到这说明_pos>0，是有效值
	}

	//cout << "fileName_: " << fileName_ << endl;
	// HTTP 版本号
	pos = request_line.find("/", pos);
	if (pos < 0)
		return PARSE_URI_ERROR;
	else {
		if (request_line.size() - pos <= 3)		//不够版本号长度
			return PARSE_URI_ERROR;
		else {
			string ver = request_line.substr(pos + 1, 3);
			if(ver=="1.0")
				HTTPVersion_ = HTTP_10;
			else if (ver == "1.1")
				HTTPVersion_ = HTTP_11;
			else
				return PARSE_URI_ERROR;
		}
	}
	return PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders() {
	string &str = inBuffer_;
	int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
	int now_read_line_begin = 0;
	bool notFinish = true;
	size_t i = 0;
	for (; i < str.size() && notFinish; ++i) {	//从头到尾逐个关键点查找
		switch (hState_) 
		{
			case H_START:
			{
				if (str[i] == '\n' || str[i] == '\r')
					break;
				hState_ = H_KEY;
				key_start = i;
				now_read_line_begin = i;
				break;
			}
			case H_KEY:
			{
				if (str[i] == ':')
				{
					key_end = i;
					if (key_end - key_start <= 0)
						return PARSE_HEADER_ERROR;
					hState_ = H_COLON;	//colon冒号
				}
				else if (str[i] == '\n' || str[i] == '\r')	//没找到冒号
					return PARSE_HEADER_ERROR;
				break;
			}
			case H_COLON:
			{
				if (str[i] == ' ')
				{
					hState_ = H_SPACES_AFTER_COLON;
				}
				else
					return PARSE_HEADER_ERROR;
				break;
			}
			case H_SPACES_AFTER_COLON:
			{
				hState_ = H_VALUE;
				value_start = i;
				break;
			}
			case H_VALUE:
			{
				if (str[i] == '\r') {
					hState_ = H_CR;
					value_end = i;
					if (value_end - value_start <= 0)
						return PARSE_HEADER_ERROR;
				}
				else if (i - value_start > 255)		//超过长度
					return PARSE_HEADER_ERROR;
				break;
			}
			case H_CR:
			{
				if (str[i] == '\n')
				{
					hState_ = H_LF;
					string key(str.begin() + key_start, str.begin() + key_end);
					string value(str.begin() + value_start, str.begin() + value_end);
					headers_[key] = value;				//将拿到的key和value填进headers_
					now_read_line_begin = i;
				}
				else
					return PARSE_HEADER_ERROR;
				break;
			}
			case H_LF:
			{
				if (str[i] == '\r')
				{
					hState_ = H_END_CR;
				}
				else 
				{
					key_start = i;		//没有找到回车的话，说明本行还有key
					hState_ = H_KEY;		
				}
				break;
			}
			case H_END_CR:
			{
				if (str[i] == '\n')
				{
					hState_ = H_END_LF;
				}
				else 
					return PARSE_HEADER_ERROR;
				break;
			}
			case H_END_LF:
			{
				notFinish = false;
				key_start = i;
				now_read_line_begin = i;
				break;
			}
		}
	}
	if (hState_ == H_END_LF)		
	{
		str = str.substr(i);			//将inBuffer_中一份有效的header处理完毕，
		return PARSE_HEADER_SUCCESS;
	}
	str = str.substr(now_read_line_begin);		//找不到一份有效的header，返回错误
	return PARSE_HEADER_AGAIN;						
}


/*响应信息;
响应行： 协议版本 状态码 状态文字
响应头信息
key:value
key:value
...
Content-length:          接下来响应主体的长度
Content-type:			 类型
							//空行分割
hello					 响应主体			返回资源中的内容
*/

/*例子：

HTTP/1.1 200 OK
Date: Thu. 06 Jun 2013 12:39:02 GMT
Server: Apache/2.2.21 (win32) PHP/5.3.8
X-Powered-By: PHP/5.3.8
Content-length: 5
Content-type: text/html

hello
*/




AnalysisState HttpData::analysisRequest()
{
	if (method_ == METHOD_POST)
	{}
	else if (method_ == METHOD_GET || method_ == METHOD_HEAD) {
		string header;
		header += "HTTP/1.1 200 OK\r\n";
		if (headers_.find("Connection") != headers_.end() && (headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "keep-alive")) {
			keepAlive_ = true;
			header += string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
		}
		int dot_pos = fileName_.find('.');
		string filetype;
		if (dot_pos < 0)
			filetype = MimeType::getMime("default");
		else
			filetype = MimeType::getMime(fileName_.substr(dot_pos));	//将.后面的字符串赋值给文件类型filetype


		// echo test		
		if (fileName_ == "hello")
		{
			outBuffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
			return ANALYSIS_SUCCESS;
		}
		if (fileName_ == "favicon.ico")		//图标
		{
			header += "Content-Type: image/png\r\n";
			header += "Content-Length: " + to_string(sizeof favicon) + "\r\n";
			header += "Server: Web Server\r\n";

			header += "\r\n";
			outBuffer_ += header;
			outBuffer_ += string(favicon, favicon + sizeof favicon);
			return ANALYSIS_SUCCESS;
		}

		struct stat sbuf;
		if (stat(fileName_.c_str(), &sbuf) < 0)		
		{
			header.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}
		header += "Content-Type: " + filetype + "\r\n";
		header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
		header += "Server: Web Server\r\n";
		// 头部结束
		header += "\r\n";
		outBuffer_ += header;

		if (method_ == METHOD_HEAD)		//如果请求方法仅是头部，而不带body，这里就可以成功返回了
			return ANALYSIS_SUCCESS;


		int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
		if (src_fd < 0)
		{
			outBuffer_.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}

		void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		close(src_fd);
		if (mmapRet == (void *)-1)
		{
			munmap(mmapRet, sbuf.st_size);
			outBuffer_.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}
		char *src_addr = static_cast<char*>(mmapRet);
		outBuffer_ += string(src_addr, src_addr + sbuf.st_size);;
		munmap(mmapRet, sbuf.st_size);
		return ANALYSIS_SUCCESS;			

	}
	return ANALYSIS_ERROR;
}

void HttpData::handleError(int fd, int err_num, string short_msg) {
	short_msg = " " + short_msg;
	char send_buff[4096];
	string body_buff, header_buff;
	body_buff += "<html><title>出错了</title>";
	body_buff += "<body bgcolor=\"ffffff\">";
	body_buff += to_string(err_num) + short_msg;
	body_buff += "<hr><em> My Web Server</em>\n</body></html>";

	header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
	header_buff += "Content-Type: text/html\r\n";
	header_buff += "Connection: Close\r\n";
	header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
	header_buff += "Server: My Web Server\r\n";;
	header_buff += "\r\n";

	// 错误处理不考虑writen不完的情况,就简单地把错误信息往fd一写
	sprintf(send_buff, "%s", header_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));
	sprintf(send_buff, "%s", body_buff.c_str());
	writen(fd, send_buff, strlen(send_buff));
}


void HttpData::handleClose() {
	connectionState_ = H_DISCONNECTED;
	shared_ptr<HttpData> guard(shared_from_this());
	loop_->removeFromPoller(channel_);		//将本httpdata对象所在eventloop的channel解绑
}

void HttpData::newEvent() {
	channel_->setEvents(DEFAULT_EVENT);
	loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);		//绑上定时器，并将fd挂上epoll和加入fd2chan、fd2http
}
