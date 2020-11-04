#ifndef _SOCKET_BUF_H_
#define _SOCKET_BUF_H_

#include <string>
#include <queue>
#include <utility>
#include <map>
#include <unordered_map>
#include <mutex>
#include <memory>


#include "../include/logging.h"
#include "../utils/singleton.h"
#include "./msg_queue.h"
#include "./net_api.h"
#include "./pack.h"


extern std::unordered_map<int, std::unique_ptr<std::mutex>> fds_mutex;
std::mutex& get_fd_mutex(int fd);



class SocketBuf
{
public:
    int fd;
    uint64_t port_and_ip;

private:
    std::string cache;
	std::mutex mutex_for_read_;

    std::string send_cache;
    std::mutex mutex_for_send_;
	std::atomic<bool> is_sending_;

private:
    bool send_pk_to_mess_queue(const std::string& send_data);

public:
	SocketBuf() : fd(0), port_and_ip(0), is_sending_(false) {};
    ~SocketBuf()
    {
        if (!this->cache.empty())
        {
            info("SocketBuf is not empty!!");
            info("SocketBuf.fd: %d", fd);
            info("SocketBuf.port_and_ip: %lu", port_and_ip);
            info("SocketBuf.cache: %s", cache.c_str());
        }
        if(!this->send_cache.empty())
            debug("send_cache: %s", this->send_cache.c_str());
    };

    bool add_data_to_read_buf(char *data, size_t len);
    void printf_cache();
    std::string get_send_msg();
    void pop_n_send_msg(int n);
    void push_send_msg(const std::string& data);
    bool is_send_cache_empty();
	bool is_sending_msg();
	void set_sending_msg(bool is_sending);

    void verify_cache(size_t curr_msg_len);      
    void correct_cache();                       
};


class BufferCrol
{
public:
    BufferCrol() = default;
    ~BufferCrol() = default;

private:
	std::mutex mutex_;
    std::map<uint64_t,std::shared_ptr<SocketBuf>> BufferMap;

public:
  
    bool add_read_buffer_queue(std::string ip, uint16_t port, char* buf, socklen_t len);
    bool add_read_buffer_queue(uint32_t ip, uint16_t port, char* buf, socklen_t len);
    bool add_read_buffer_queue(uint64_t port_and_ip, char* buf, socklen_t len);
    bool add_buffer(std::string ip, uint16_t port,const int fd);
    bool add_buffer(uint32_t ip, uint16_t port,const int fd);
    bool add_buffer(uint64_t port_and_ip,const int fd);
    bool delete_buffer(std::string ip, uint16_t port);
    bool delete_buffer(uint32_t ip, uint16_t port);
    bool delete_buffer(uint64_t port_and_ip);
    std::string get_write_buffer_queue(uint64_t port_and_ip);
    std::string get_write_buffer_queue(std::string ip, uint16_t port);
    std::string get_write_buffer_queue(uint32_t ip, uint16_t port);
    bool add_write_buffer_queue(uint64_t port_and_ip, const std::string& data);
    bool add_write_buffer_queue(std::string ip, uint16_t port, const std::string& data);
    bool add_write_buffer_queue(uint32_t ip, uint16_t port, const std::string& data);
    void pop_n_write_buffer_queue(uint64_t port_and_ip, int n);
    void pop_n_write_buffer_queue(std::string ip, uint16_t port, int n);
    void pop_n_write_buffer_queue(uint32_t ip, uint16_t port, int n);

	bool add_write_pack(uint64_t port_and_ip, const net_pack& pack);
	bool add_write_pack(std::string ip, uint16_t port, const net_pack& pack);
	bool add_write_pack(uint32_t ip, uint16_t port, const net_pack& pack);

    bool add_write_pack(uint64_t port_and_ip, const std::string ios_msg);
	bool add_write_pack(std::string ip, uint16_t port, const std::string ios_msg);
	bool add_write_pack(uint32_t ip, uint16_t port, const std::string ios_msg);


    bool is_exists(uint64_t port_and_ip);
	bool is_exists(std::string ip, uint16_t port);
	bool is_exists(uint32_t ip, uint16_t port);

    bool is_cache_empty(uint32_t ip, uint16_t port);

    
    void print_bufferes();
    
};


#endif
