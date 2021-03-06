#include "epoll_mode.h"
#include "./global.h"

EpollMode::EpollMode()
{
    this->listen_thread = NULL;
}
EpollMode::~EpollMode()
{
    if (NULL != this->listen_thread)
    {
        delete this->listen_thread;
        this->listen_thread = NULL;
    }
}

bool EpollMode::init_listen()
{
    this->epoll_fd = epoll_create(MAXEPOLLSIZE);
    if (this->epoll_fd < 0)
    {
        error("EpollMode error");
        return false;
    }
    this->fd_ser_main = net_tcp::listen_server_init(SERVERMAINPORT, 1000);
    this->add_epoll_event(this->fd_ser_main, EPOLLIN | EPOLLOUT | EPOLLET);
    return true;
}

void EpollMode::work(EpollMode *epmd)
{
    epmd->init_listen();
    global::listen_thread_inited = true;
    global::cond_listen_thread.notify_all();
    // std::cout << "cond_listen_thread:notify_all" << std::endl;
    epmd->epoll_loop();
}

bool EpollMode::start()
{
    this->listen_thread = new thread(EpollMode::work, this);
    this->listen_thread->detach();
    return true;
}

int EpollMode::epoll_loop()
{
    info("EpollMode working");
    int nfds, n;
    struct sockaddr_in cliaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct epoll_event events[MAXEPOLLSIZE];
    struct rlimit rt;
    u32 u32_ip;
    u16 u16_port;
    int e_fd;
    MsgData data;
    /* Set the maximum number of files allowed to be opened per process  */
    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1)
    {
        perror("setrlimit error");
    }
    info("epoll loop start success!");
    while (1)
    {

        /* Wait for something to happen  */
        nfds = epoll_wait(this->epoll_fd, events, MAXEPOLLSIZE, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            continue;
        }
        /* Handle all events  */
        for (n = 0; n < nfds; ++n)
        {
            if (events[n].events & EPOLLERR)
            {
                // error("EPOLLERR=================");
                int status, err;
                socklen_t len;
                err = 0;
                len = sizeof(err);
                e_fd = events[n].data.fd;
                status = getsockopt(e_fd, SOL_SOCKET, SO_ERROR, &err, &len);
                // Connection failed 
                if (status == 0)
                {
                    Singleton<PeerNode>::get_instance()->delete_by_fd(e_fd);
                }
            }
            // Handle the main connection monitoring 
            if (events[n].data.fd == this->fd_ser_main)
            {
                int connfd = 0;
                int lis_fd = events[n].data.fd;
       
                while ((connfd = net_tcp::Accept(lis_fd, (struct sockaddr *)&cliaddr, &socklen)) > 0)
                {
                    net_tcp::set_fd_noblocking(connfd);
                    //Turn off all signals 
                    int value = 1;
                    setsockopt(connfd, SOL_SOCKET, MSG_NOSIGNAL, &value, sizeof(value));

                    u32_ip = IpPort::ipnum(inet_ntoa(cliaddr.sin_addr));
                    u16_port = htons(cliaddr.sin_port);

                    auto self = Singleton<PeerNode>::get_instance()->get_self_node();
                    debug(YELLOW "u32_ip(%s),u16_port(%d),self.public_ip(%s),self.local_ip(%s)" RESET, IpPort::ipsz(u32_ip), u16_port, IpPort::ipsz(self.public_ip), IpPort::ipsz(self.local_ip));


                    Singleton<BufferCrol>::get_instance()->add_buffer(u32_ip, u16_port, connfd);
                    Singleton<EpollMode>::get_instance()->add_epoll_event(connfd, EPOLLIN | EPOLLOUT | EPOLLET);
            
                } //while End of loop 
                if (connfd == -1)
                {
                    if (errno != EAGAIN && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)
                        perror("accept");
                }
                continue;
            }
            if (events[n].events & EPOLLIN)
            {
                e_fd = events[n].data.fd;
                this->delete_epoll_event(e_fd);
                u32_ip = IpPort::get_peer_nip(e_fd);
                u16_port = IpPort::get_peer_port(e_fd);

                data.ip = u32_ip;
                data.port = u16_port;
                data.type = E_READ;
                data.fd = e_fd;
                global::queue_read.push(data);
            }
            if (events[n].events & EPOLLOUT)
            {
                e_fd = events[n].data.fd;
                u32_ip = IpPort::get_peer_nip(e_fd);
                u16_port = IpPort::get_peer_port(e_fd);
               
                bool isempty = Singleton<BufferCrol>::get_instance()->is_cache_empty(u32_ip, u16_port);
                if(isempty){
                    continue;
                }
                MsgData send;
                send.type = E_WRITE;
                send.fd = e_fd;
                send.ip = u32_ip;
                send.port = u16_port;
                global::queue_write.push(send);
            }
        }
    }
    return 0;
}

bool EpollMode::add_epoll_event(int fd, int state)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = state;
    ev.data.fd = fd;

    int ret = epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &ev);

    if (0 != ret)
    {
        return false;
    }
    return true;
}

bool EpollMode::delete_epoll_event(int fd)
{
    int ret = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (0 != ret)
    {
        return false;
    }
    return true;
}

bool EpollMode::modify_epoll_event(int fd, int state)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = state;
    ev.data.fd = fd;
    int ret = epoll_ctl(this->epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    if (0 != ret)
    {
        return false;
    }

    return true;
}
