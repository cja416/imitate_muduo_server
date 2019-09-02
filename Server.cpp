#include "Server.h"
#include "Logging.h"
#include "Util.h"
#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

Server::Server(EventLoop *loop, int threadNum, int port)
	: loop_(loop),
	threadNum_(threadNum),
	eventLoopThreadPool_(new EventLoopThreadPool(loop_,threadNum)),		//新建事件循环线程池
	started_(false),
	acceptChannel_(new Channel(loop_)),		//新建channel
	port_(port),
	listenFd_(socket_bind_listen(port_))	//绑定端口，初始化listenfd
{
	acceptChannel_->setFd(listenFd_);
	handle_for_sigpipe();			//在util中实现
	if (setSocketNonBlocking(listenFd_) < 0) {	//设置非阻塞监听
		perror("set socket non block failed");
		abort();
	}
}

void Server::start() {
	eventLoopThreadPool_->start();			//开启线程池
	//acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));			//处理新连接事件   如果是普通线程池的话，是通过传进函数指针的方式指定
	acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));			//处理读写事件
	loop_->addToPoller(acceptChannel_, 0);	//将负责处理接受连接的channel 挂到epoll红黑树上去
	started_ = true;
}

void Server::handNewConn() {
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct socketaddr_in));
	socklen_t client_addr_len = sizeof(client_addr);
	int accept_fd = 0;
	//接受新连接直到没有了	因为是ET模式，所以accept要套一个while
	while ((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr) > 0)) {
		EventLoop* loop = eventLoopThreadPool_->getNextLoop();				//所谓的robin round分配法
		LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);
		// cout << "new connection" << endl;
		// cout << inet_ntoa(client_addr.sin_addr) << endl;
		// cout << ntohs(client_addr.sin_port) << endl;
		/*
		// TCP的保活机制默认是关闭的
		int optval = 0;
		socklen_t len_optval = 4;
		getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
		cout << "optval ==" << optval << endl;
		*/
		// 限制服务器的最大并发连接数
		if (accept_fd >= MAXFDS) {
			close(accept_fd);
			continue;
		}
		// 设为非阻塞模式
		if (setSocketNonBlocking(accept_fd)<0)
		{
			LOG << "Set non block failed!";
			//perror("Set non block failed!");
			return;
		}
		setSocketNodelay(accept_fd);
		//setSocketNoLinger(accept_fd);
		shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));	//HttpData来处理读写描述符的请求内容
		req_info->getChannel()->setHolder(req_info);
		loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));		//加进任务队列的函数是向IO线程epoll_add
	}
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}