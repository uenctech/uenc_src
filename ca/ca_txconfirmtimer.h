// Create: timer for confirmed the transaction, 20210310   LiuMing

#ifndef __CA_TX_CONFIRM_TIMER_H__
#define __CA_TX_CONFIRM_TIMER_H__

#include "../utils/CTimer.hpp"
#include "ca_txvincache.h"
#include "proto/block.pb.h"
#include <vector>
#include <mutex>
#include <string>

using namespace std;

struct TxConfirmation
{
    TxVinCache::Tx tx;
    uint64_t startstamp;
    std::vector<std::string>ids;
    int count = 0;
    CBlock block;
};

class TransactionConfirmTimer
{
public:
    TransactionConfirmTimer();
    ~TransactionConfirmTimer() = default;
    
    void add(TxVinCache::Tx& tx);
    void add(const string& tx_hash);
    bool remove(const string& tx_hash);
    int get_count(const string& tx_hash);
    void update_count(const string& tx_hash, CBlock& block);

    void confirm();
    void timer_start();

    void get_ids(const string& tx_hash,std::vector<std::string>&ids);
    void update_id(const string& tx_hash,std::string&ids );
    static void timer_process(TransactionConfirmTimer* timer);

private:
    std::vector<TxConfirmation> tx_confirmation_;
    std::mutex mutex_;
    
    CTimer timer_;
};

#endif // __CA_TX_CONFIRM_TIMER_H__