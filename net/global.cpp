#include "global.h"

namespace global
{
    std::string local_ip;
    std::string mac_md5;
    int cpu_nums;
    atomic<int> nodelist_refresh_time;
    MsgQueue queue_read("ReadQueue");   //Read queue 
    MsgQueue queue_work("WorkQueue");   //Work queue, mainly to process the queue that calls CA code after read 
    MsgQueue queue_write("WriteQueue"); //Write queue 
    //todo
    std::list<int> phone_list; //Store the fd connected to the mobile phone 
    std::mutex mutex_for_phone_list;
    CTimer g_timer;
    CTimer broadcast_timer;
    CTimer registe_public_node_timer; //liuzg
    std::mutex mutex_listen_thread;
    std::mutex mutex_set_fee;
    std::condition_variable_any cond_listen_thread;
    std::condition_variable_any cond_fee_is_set;
    bool listen_thread_inited = false;
    int fee_inited = 0;

    std::map<std::string, std::pair<uint32_t, uint64_t>> reqCntMap; // statistics 
    //Transaction information stored in other nodes 
    std::mutex g_mutex_transinfo;
    bool g_is_utxo_empty = true;
    GetTransInfoAck g_trans_info;

}
