#include "global.h"

namespace global{
    
    std::string local_ip;
    std::string mac_md5;
    int    cpu_nums;
    atomic<int> nodelist_refresh_time;
    MsgQueue queue_read("ReadQueue");   
    MsgQueue queue_work("WorkQueue");   
    MsgQueue queue_write("WriteQueue"); 
    
    std::list<int> phone_list;      
    std::mutex mutex_for_phone_list;
    Timer g_timer; 
    std::mutex mutex_listen_thread;
    std::mutex mutex_set_fee;
    std::condition_variable_any cond_listen_thread;
    std::condition_variable_any cond_fee_is_set;
    bool listen_thread_inited = false;
    int fee_inited = 0;    
}


