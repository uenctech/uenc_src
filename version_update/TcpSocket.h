#pragma once
#include <iostream>
using namespace std;


/* 用于通信的套接字类 */
// 超时的时间
static const int TIMEOUT = 10;
class TcpSocket
{
public:
	enum ErrorType {ParamError = 3001, TimeoutError, PeerCloseError, MallocError};
	TcpSocket();
	// 使用一个可以用于通信的套接字实例化套接字对象
	TcpSocket(int connfd);
	~TcpSocket();

	// 连接服务器
	int connectToHost(string ip, unsigned short port, int timeout = TIMEOUT);

	// 发送数据，不加包头
	int sendMsg(const string sendData, int timeout = TIMEOUT);

	// 发送数据，加包头
	int sendMsgn(string sendData, int timeout = TIMEOUT);

	// 接收数据
	string recvMsg(int timeout = TIMEOUT);

	int getSocket();

	// 断开连接
	void disConnect();

private:
	// 设置I/O为非阻塞模式
	int setNonBlock(int fd);
	// 设置I/O为阻塞模式
	int setBlock(int fd);
	// 写超时检测函数, 不包含写操作
	int writeTimeout(unsigned int wait_seconds);
	// 读超时检测函数，不含读操作
	int readTimeout(unsigned int wait_seconds);
	// 带连接超时的connect函数
	int connectTimeout(struct sockaddr_in *addr, unsigned int wait_seconds);
	// 每次从缓冲区中读取n个字符
	int readn(void *buf, int count);
	// 每次往缓冲区写入n个字符
	int writen(const void *buf, int count);

private:
	int m_socket = -1;		// 用于通信的套接字
};

