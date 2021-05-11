#include "global.h"

namespace global
{
    std::string local_ip;
    std::string mac_md5;
    int cpu_nums;
    atomic<int> nodelist_refresh_time;
    MsgQueue queue_read("ReadQueue");   //读队列
    MsgQueue queue_work("WorkQueue");   //工作队列，主要来处理read后的调用CA代码的队列
    MsgQueue queue_write("WriteQueue"); //写队列
    //todo
    std::list<int> phone_list; //存放手机端连接的fd
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

    std::map<std::string, std::pair<uint32_t, uint64_t>> reqCntMap; // 统计

    //存储在其他节点获取的交易信息
    std::mutex g_mutex_transinfo;
    bool g_is_utxo_empty = true;
    GetTransInfoAck g_trans_info;

}
