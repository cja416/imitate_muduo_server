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
	eventLoopThreadPool_(new EventLoopThreadPool(loop_,threadNum)),		//�½��¼�ѭ���̳߳�
	started_(false),
	acceptChannel_(new Channel(loop_)),		//�½�channel
	port_(port),
	listenFd_(socket_bind_listen(port_))	//�󶨶˿ڣ���ʼ��listenfd
{
	acceptChannel_->setFd(listenFd_);
	handle_for_sigpipe();			//��util��ʵ��
	if (setSocketNonBlocking(listenFd_) < 0) {	//���÷���������
		perror("set socket non block failed");
		abort();
	}
}

void Server::start() {
	eventLoopThreadPool_->start();			//�����̳߳�
	//acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
	acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));			//�����������¼�   �������ͨ�̳߳صĻ�����ͨ����������ָ��ķ�ʽָ��
	acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));			//�����д�¼�
	loop_->addToPoller(acceptChannel_, 0);	//��������������ӵ�channel �ҵ�epoll�������ȥ
	started_ = true;
}

void Server::handNewConn() {
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct socketaddr_in));
	socklen_t client_addr_len = sizeof(client_addr);
	int accept_fd = 0;
	//����������ֱ��û����	��Ϊ��ETģʽ������acceptҪ��һ��while
	while ((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr) > 0)) {
		EventLoop* loop = eventLoopThreadPool_->getNextLoop();				//��ν��robin round���䷨
		LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);
		// cout << "new connection" << endl;
		// cout << inet_ntoa(client_addr.sin_addr) << endl;
		// cout << ntohs(client_addr.sin_port) << endl;
		/*
		// TCP�ı������Ĭ���ǹرյ�
		int optval = 0;
		socklen_t len_optval = 4;
		getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
		cout << "optval ==" << optval << endl;
		*/
		// ���Ʒ���������󲢷�������
		if (accept_fd >= MAXFDS) {
			close(accept_fd);
			continue;
		}
		// ��Ϊ������ģʽ
		if (setSocketNonBlocking(accept_fd)<0)
		{
			LOG << "Set non block failed!";
			//perror("Set non block failed!");
			return;
		}
		setSocketNodelay(accept_fd);
		//setSocketNoLinger(accept_fd);
		shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));	//HttpData�������д����������������
		req_info->getChannel()->setHolder(req_info);
		loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));		//�ӽ�������еĺ�������IO�߳�epoll_add
	}
	acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}