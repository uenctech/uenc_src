#include <string>
#include <iostream>
#include <fcntl.h>
#include <errno.h>
#include <sys/uio.h>  
#include <stdint.h>
#include <endian.h>
#include <unistd.h>
#include <map>
#include <fstream>

#include "HttpRequest.h"

const std::map<std::string, int>::value_type init_value[] =
{
    std::map<std::string, int>::value_type( "GET", HttpRequest::GET),

    std::map<std::string, int>::value_type( "POST", HttpRequest::POST)
};

const static std::map<std::string, int> kRequestMethodMap(init_value, init_value + (sizeof init_value / sizeof init_value[0]));

static inline uint16_t hostToNetwork16(uint16_t host16)
{
  return htobe16(host16);
}


int sockets::createSocket(sa_family_t family){
  
  

    int sock;

    
    
    
#ifdef SOCK_CLOEXEC
  sock = socket(family, SOCK_STREAM|SOCK_CLOEXEC, 0);
  if (sock != -1 || errno != EINVAL) return sock;
  
#endif

  sock = socket(family, SOCK_STREAM, 0);

#ifdef FD_CLOEXEC
  if (sock != -1) fcntl(sock, F_SETFD, FD_CLOEXEC);
#endif
  return sock;
}

int sockets::connect(int sockfd, const struct sockaddr* addr)
{
  return ::connect(sockfd, addr, sizeof(struct sockaddr));
}

void sockets::fromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in* addr)
{
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
  {
    
  }
}

ssize_t sockets::read(int sockfd, void *buf, size_t count)
{
  return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
  return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count)
{
  return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd)
{
  if (::close(sockfd) < 0)
  {
    
  }
}

void sockets::delaySecond(int sec)
{
  struct  timeval tv;
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  select(0, NULL, NULL, NULL, &tv);
}



InetAddress::InetAddress(std::string ip, uint16_t port)
{
  ::bzero(&m_addr, sizeof m_addr);
  sockets::fromIpPort(ip.c_str(), port, &m_addr);
}

uint32_t InetAddress::ipNetEndian() const
{
  assert(family() == AF_INET);
  return m_addr.sin_addr.s_addr;
}


HttpRequest::HttpRequest(std::string httpUrl)
  :m_httpUrl(httpUrl)
{

}

HttpRequest::~HttpRequest()
{

}

void HttpRequest::connect()
{
  char ip[32] = {0};
  while(true)
  {
    struct hostent* phost = NULL;

    phost = gethostbyname(m_httpUrl.domain().c_str());
    if (NULL == phost)
    {
    
      sockets::delaySecond(1);
      continue;
    }

    inet_ntop(phost->h_addrtype,  phost->h_addr, ip, sizeof ip);

    

    InetAddress serverAddr = InetAddress(ip, 80);

    m_sockfd = sockets::createSocket(serverAddr.family());
    
    int ret = sockets::connect(m_sockfd, serverAddr.getSockAddr());
    
    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno)
    {
      case 0:
      case EINPROGRESS:
      case EINTR:
      case EISCONN:
        
        break;
      default :
        
        sockets::delaySecond(1);
        continue;
    }

    break;
  }


}

void HttpRequest::setRequestMethod(const std::string &method)
{
    switch(kRequestMethodMap.at(method))
    {
        case HttpRequest::GET :
            m_stream << "GET " << "/" << m_httpUrl.getHttpUrlSubSeg(HttpUrl::URI) << " HTTP/1.1\r\n";
            
            break;
        case HttpRequest::POST :
            m_stream << "POST "  << "/" << m_httpUrl.getHttpUrlSubSeg(HttpUrl::URI) << " HTTP/1.1\r\n";
            
            break;
        default :
            
            break;
    }

    m_stream << "Host: " << m_httpUrl.getHttpUrlSubSeg(HttpUrl::HOST) << "\r\n";
}


void HttpRequest::setRequestProperty(const std::string &key, const std::string &value)
{
    m_stream << key << ": " << value << "\r\n";
}

void HttpRequest::setRequestBody(const std::string &content)
{
    m_stream << content;
}

void HttpRequest::handleRead()
{
    assert(!m_haveHandleHead);
    ssize_t nread = 0;
    

    nread = sockets::read(m_sockfd, m_buffer.beginWrite(), kBufferSize);
    
    m_buffer.hasWritten(nread);
    
    size_t remain = kBufferSize - nread;
    while(remain > 0)
    {
        size_t n = sockets::read(m_sockfd, m_buffer.beginWrite(), remain);
        
        m_buffer.hasWritten(n);
        if(0 == n)
        {
            
            break;
        }
        remain = remain - n;
    }
    

    
    
    int headsize = 0;
    std::string line;
    std::stringstream ss(m_buffer.peek());
    std::vector<std::string> v;
    getline(ss, line);
    
    headsize += line.size() + 1;
    SplitString(line, v, " ");
    
    m_code = std::stoi(v[1]);
    if(v[1] != "200")
    {
    
    }

    do{
        getline(ss, line);
        headsize += line.size() + 1;  
        if(!line.empty()) line.erase(line.end()-1); 
        
        v.clear();
        SplitString(line, v, ":");
        if(v.size() == 2){
            m_ackProperty[v[0]] = v[1].erase(0,v[1].find_first_not_of(" "));
        }
    }while(!line.empty());

    
    std::string res(m_buffer.peek(), headsize);
    
    m_buffer.retrieve(headsize);

    m_haveHandleHead = true;

}

void HttpRequest::uploadFile(const std::string& file, const std::string& contentEnd)
{

    FILE* fp = fopen(file.c_str(), "rb");
    if(fp == NULL)
    {
        
    }

    bool isEnd = false;
    ssize_t writtenBytes = 0;

    assert(m_buffer.writableBytes() == Buffer::kInitialSize);

    while(!isEnd)
    {
        ssize_t nread = fread(m_buffer.beginWrite(), 1, kBufferSize, fp);
        m_buffer.hasWritten(nread);
        while(m_buffer.writableBytes() > 0)
        {
            
            size_t n = fread(m_buffer.beginWrite(), 1, m_buffer.writableBytes(), fp);
            m_buffer.hasWritten(n);
            if(0 == n)
            {   int err = ferror(fp);
                if(err)
                {
                    fprintf(stderr, "fread failed : %s\n", strerror(err));
                }
                
                isEnd = true;
                break;
            }
        }

        ssize_t nwrite = sockets::write(m_sockfd, m_buffer.peek(), m_buffer.readableBytes());
        
        writtenBytes += nwrite;
        
        m_buffer.retrieve(nwrite);
    }

    fclose(fp);

    m_buffer.retrieveAll();

    sockets::write(m_sockfd, contentEnd.c_str(), contentEnd.size());
    
    
}

void HttpRequest::downloadFile(const std::string& file)
{
    assert(m_haveHandleHead);

    bool isEnd = false;
    ssize_t nread = 0;
    ssize_t writtenBytes = 0;
    
    

    std::ofstream output(file, std::ios::binary);
    if (!output.is_open()){ 
        
    }

    output.write(m_buffer.peek(), m_buffer.readableBytes());
    writtenBytes += m_buffer.readableBytes();
    m_buffer.retrieve(m_buffer.readableBytes());

    

    while(!isEnd)
    {
        nread = sockets::read(m_sockfd, m_buffer.beginWrite(), kBufferSize);
        
        m_buffer.hasWritten(nread);
        
        size_t remain = kBufferSize - nread;
        while(remain > 0)
        {
            size_t n = sockets::read(m_sockfd, m_buffer.beginWrite(), remain);
            
            m_buffer.hasWritten(n);
            if(0 == n)
            {
                
                isEnd = true;
                break;
            }
            remain = remain - n;
        }

        output.write(m_buffer.peek(), m_buffer.readableBytes());
        writtenBytes += m_buffer.readableBytes();
        m_buffer.retrieve(m_buffer.readableBytes());
    }
    

    output.close();
    sockets::close(m_sockfd);
}

void HttpRequest::SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while(std::string::npos != pos2)
  {
    v.push_back(s.substr(pos1, pos2-pos1));

    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if(pos1 != s.length())
    v.push_back(s.substr(pos1));
}