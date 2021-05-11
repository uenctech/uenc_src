#ifndef _HTTP_REQUEST_HH
#define _HTTP_REQUEST_HH

#include <netdb.h>
#include <arpa/inet.h>
#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <map>

const size_t kBufferSize = 4096;

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
public:

  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 4096;
//public:

    explicit Buffer(size_t initialSize = kInitialSize)
    : m_buffer(kCheapPrepend + initialSize),
      m_readerIndex(kCheapPrepend),
      m_writerIndex(kCheapPrepend)
      {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
      }

  size_t readableBytes() const
  { return m_writerIndex - m_readerIndex; }

  size_t writableBytes() const
  { return m_buffer.size() - m_writerIndex; }

  size_t prependableBytes() const
  { return m_readerIndex; }

  const char* peek() const
  { return begin() + m_readerIndex; }

  char* beginWrite()
  { return begin() + m_writerIndex; }

  void hasWritten(size_t len)
  {
    assert(len <= writableBytes());
    m_writerIndex += len;
  }

  void unwrite(size_t len)
  {
    assert(len <= readableBytes());
    m_writerIndex -= len;
  }

  // retrieve returns void, to prevent
  // string str(retrieve(readableBytes()), readableBytes());
  // the evaluation of two functions are unspecified
  void retrieve(size_t len)
  {
    assert(len <= readableBytes());
    if (len < readableBytes())
    {
      m_readerIndex += len;
    }
    else
    {
      retrieveAll();
    }
  }

  void retrieveAll()
  {
    m_readerIndex = kCheapPrepend;
    m_writerIndex = kCheapPrepend;
  }

private:
  char* begin()
  {return &*m_buffer.begin(); }

  const char* begin() const
  {return &*m_buffer.begin(); }

private:
    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
};


namespace sockets
{

/// Creates a non-blocking socket file descriptor,
/// abort if any error.
int createSocket(sa_family_t family);

int  connect(int sockfd, const struct sockaddr* addr);
ssize_t read(int sockfd, void *buf, size_t count);
ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t write(int sockfd, const void *buf, size_t count);
void close(int sockfd);

void fromIpPort(const char* ip, uint16_t port,
                struct sockaddr_in* addr);

int getSocketError(int sockfd);
void delaySecond(int sec);
//const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr)
//const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);

}

class InetAddress
{
 public:
  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(std::string ip, uint16_t port);

  /// Constructs an endpoint with given struct @c sockaddr_in
  /// Mostly used when accepting new connections
  explicit InetAddress(const struct sockaddr_in& addr)
    : m_addr(addr)
  { }

  sa_family_t family() const { return m_addr.sin_family; }
  //std::string toIp() const;
  //std::string toIpPort() const;

  const struct sockaddr* getSockAddr() const { return (struct sockaddr*)(&m_addr); }

  uint32_t ipNetEndian() const;

  // resolve hostname to IP address, not changing port or sin_family
  // return true on success.
  // thread safe
  // static bool resolve(StringArg hostname, StringArg* ip);
  // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

 private:
    struct sockaddr_in m_addr;

};


class HttpUrl
{
public:
    HttpUrl(std::string& httpUrl)
    :m_httpUrl(httpUrl),
     m_smatch(detachHttpUrl())
    {
        //LOG_DEBUG << "URL : " << m_httpUrl;
    }
    ~HttpUrl(){};

    enum HttpUrlMatch
    {
        URL = 0,
        HOST = 1,
        URI = 2
    };

    std::vector<std::string> detachHttpUrl() const
    {
      std::vector<std::string> v;
      std::string::size_type pos1, pos2;
      pos2 = m_httpUrl.find('/');
      assert(std::string::npos != pos2);
      pos1 = pos2 + 2;
      pos2 = m_httpUrl.find('/', pos1);
      assert(std::string::npos != pos2);
      v.push_back(m_httpUrl);
      v.push_back(m_httpUrl.substr(pos1, pos2 - pos1));
      v.push_back(m_httpUrl.substr(pos2 + 1));
    //   LOG_DEBUG << "detachHttpUrl() url :" << v[0];
    //   LOG_DEBUG << "detachHttpUrl() host :" << v[1];
    //   LOG_DEBUG << "detachHttpUrl() uri :" << v[2];
      return v;
    }

    bool HttpUrlToIp(const std::string& host, char* ip) const
    {
        struct hostent* phost = NULL;

        phost = gethostbyname(host.c_str());
        if (NULL == phost)
        {
            // LOG_ERROR << "HttpUrlToIp(): gethostbyname error : " << errno << " : "<< strerror(errno);
            return false;
            //LOG_SYSERR << "urlToIp(): gethostbyname error";
        }

        inet_ntop(phost->h_addrtype,  phost->h_addr, ip, 17);

        return true;
    }

    std::string domain() const
    {
        return getHttpUrlSubSeg(HOST);
    }

    std::string getHttpUrlSubSeg(HttpUrlMatch sub = HOST) const{ return m_smatch[sub]; }

private:
    std::string m_httpUrl;
    std::vector<std::string> m_smatch;
};


class HttpRequest
{
public:
    enum HttpRequestMethod{
        GET = 0,
        POST
    };
    HttpRequest(std::string httpUrl);
    ~HttpRequest();

    void connect();
    //void TEST(const std::string path,const std::string content);
    void setRequestMethod(const std::string &method);
    void setRequestProperty(const std::string &key, const std::string &value);
    void setRequestBody(const std::string &content);

    //void clear() { clearStream(); m_buffer.retrieveAll(); }
    void clearStream() {m_stream.str("");}
    std::string strStream() const { return m_stream.str(); };

    int getResponseCode() const {
        assert(m_haveHandleHead);
        return m_code;
    }

    std::string getResponseProperty(const std::string& key) const {
        assert(m_haveHandleHead);
        return m_ackProperty.at(key);
    }

    std::string getResponseContent() {
        assert(m_haveHandleHead);
        return std::string(m_buffer.peek(), m_buffer.readableBytes());
    }

    void handleRead();
    void uploadFile(const std::string& file, const std::string& contentEnd);
    void downloadFile(const std::string& file);

    void send(){
        sockets::write(m_sockfd, strStream().c_str(), strStream().size());
    }

    void close(){ sockets::close(m_sockfd); }

private:
    void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);

    Buffer m_buffer;
    HttpUrl m_httpUrl;
    std::stringstream m_stream;
    int m_code;
    int m_sockfd;
    bool m_haveHandleHead;
    std::map<std::string, std::string> m_ackProperty;
};

#endif