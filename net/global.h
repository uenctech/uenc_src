#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "./define.h"
#include "./msg_queue.h"
#include <list>
#include "../utils/CTimer.hpp"
#include "net.pb.h"

namespace global
{

    extern MsgQueue queue_read;
    extern MsgQueue queue_work;
    extern MsgQueue queue_write;
    extern std::string local_ip;
    extern std::string mac_md5;
    extern int cpu_nums;
    extern atomic<int> nodelist_refresh_time;
    extern std::list<int> phone_list;
    extern std::mutex mutex_for_phone_list;
    extern CTimer g_timer;
    extern CTimer broadcast_timer;
    extern CTimer registe_public_node_timer; //liuzg
    extern std::mutex mutex_listen_thread;
    extern std::mutex mutex_set_fee;
    extern std::condition_variable_any cond_listen_thread;
    extern std::condition_variable_any cond_fee_is_set;
    extern bool listen_thread_inited;
    extern int fee_inited;

    extern std::map<std::string, std::pair<uint32_t, uint64_t>> reqCntMap;

    //存储在其他节点获取的交易信息
    extern std::mutex g_mutex_transinfo;
    extern bool g_is_utxo_empty;
    extern GetTransInfoAck g_trans_info;
}

#endif
