#ifndef _EPOLLMODE_H_
#define _EPOLLMODE_H_

#include <iostream>
#include <thread>
#include <map>
#include <sys/epoll.h>
#include <sys/resource.h>
#include "../include/logging.h"
#include "./net_api.h"
#include "../utils/singleton.h"
#include "./peer_node.h"
#include "./net_api.h"
#include "./socket_buf.h"

class EpollMode
{
    
    friend class Singleton<EpollMode>;
private:
    std::thread* listen_thread;
    
public:
    
    int epoll_loop();
    
public:
    
    bool add_epoll_event(int fd, int state);
    bool delete_epoll_event(int fd);
    bool modify_epoll_event(int fd, int state);

    bool init_listen();
    static void work(EpollMode *epmd);
    bool start();

    EpollMode();
    ~EpollMode();

public:
    int epoll_fd;

private:
    
    int fd_ser_main;
    int fd_ser_hold;
};

#endif
