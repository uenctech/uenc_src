#pragma once
#include <iostream>
using namespace std;




static const int TIMEOUT = 10;
class TcpSocket
{
public:
	enum ErrorType {ParamError = 3001, TimeoutError, PeerCloseError, MallocError};
	TcpSocket();
	
	TcpSocket(int connfd);
	~TcpSocket();

	
	int connectToHost(string ip, unsigned short port, int timeout = TIMEOUT);

	
	int sendMsg(const string sendData, int timeout = TIMEOUT);

	
	int sendMsgn(string sendData, int timeout = TIMEOUT);

	
	string recvMsg(int timeout = TIMEOUT);

	int getSocket();

	
	void disConnect();

private:
	
	int setNonBlock(int fd);
	
	int setBlock(int fd);
	
	int writeTimeout(unsigned int wait_seconds);
	
	int readTimeout(unsigned int wait_seconds);
	
	int connectTimeout(struct sockaddr_in *addr, unsigned int wait_seconds);
	
	int readn(void *buf, int count);
	
	int writen(const void *buf, int count);

private:
	int m_socket = -1;		
};

