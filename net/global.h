
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "./define.h"
#include "./msg_queue.h"
#include "../utils/time_task.h"
#include <list>

namespace global{
    
    extern MsgQueue queue_read;
    extern MsgQueue queue_work;
    extern MsgQueue queue_write;
    extern std::string local_ip;
    extern std::string mac_md5;
    extern int   cpu_nums;
    extern atomic<int> nodelist_refresh_time;
    extern std::list<int> phone_list;
    extern std::mutex mutex_for_phone_list;
    extern Timer g_timer;
    extern std::mutex mutex_listen_thread;
    extern std::mutex mutex_set_fee;
    extern std::condition_variable_any cond_listen_thread;
    extern std::condition_variable_any cond_fee_is_set;
    extern bool listen_thread_inited;
    extern int fee_inited; 
}


#endif