#include "ca_txconfirmtimer.h"
#include <iostream>
#include <cassert>
#include "../utils/singleton.h"
#include "../utils/time_util.h"
#include "MagicSingleton.h"
#include "ca_txfailurecache.h"
#include "ca_txconfirmtimer.h"
#include "ca_blockpool.h"

TransactionConfirmTimer::TransactionConfirmTimer()
{
    tx_confirmation_.reserve(128);
}

void TransactionConfirmTimer::add(TxVinCache::Tx& tx)
{
    TxConfirmation confirmation;
    confirmation.tx = tx;
    confirmation.startstamp = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
    confirmation.count = 0;

    std::lock_guard<std::mutex> lck(mutex_);
    tx_confirmation_.push_back(confirmation);
}

void TransactionConfirmTimer::add(const string& tx_hash)
{
    TxVinCache::Tx tx;
    tx.txHash = tx_hash;
    add(tx);
}

bool TransactionConfirmTimer::remove(const string& tx_hash)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = tx_confirmation_.begin(); iter != tx_confirmation_.end(); ++iter)
    {
        if (iter->tx.txHash == tx_hash)
        {
            tx_confirmation_.erase(iter);
            return true;
        }
    }
    return false;
}

int TransactionConfirmTimer::get_count(const string& tx_hash)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto& txconfirm : tx_confirmation_)
    {
        if (txconfirm.tx.txHash == tx_hash)
        {
            return txconfirm.count;
        }
    }

    return 0;
}

void TransactionConfirmTimer::update_count(const string& tx_hash, CBlock& block)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto& txconfirm : tx_confirmation_)
    {
        if (txconfirm.tx.txHash == tx_hash)
        {
            ++txconfirm.count;
            txconfirm.block = block;
            break;
        }
    }
}

void TransactionConfirmTimer::confirm()
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = tx_confirmation_.begin(); iter != tx_confirmation_.end();)
    {
        uint64_t nowTime = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
        static const uint64_t CONFIRM_WAIT_TIME = 1000000UL * 6UL;
        if ((nowTime - iter->startstamp) >= CONFIRM_WAIT_TIME)
        {
            static const int MIN_CONFIRM_COUNT = 60;
            if (iter->count >= MIN_CONFIRM_COUNT)
            {
                MagicSingleton<BlockPoll>::GetInstance()->Add(Block(iter->block));
            }
            else
            {
                MagicSingleton<TxFailureCache>::GetInstance()->Add(iter->tx);
            }
            std::cout << "Handle confirm: " << iter->count << ", hash=" << iter->tx.txHash << std::endl;

            iter = tx_confirmation_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void TransactionConfirmTimer::timer_start()
{
    this->timer_.AsyncLoop(1000 * 2, TransactionConfirmTimer::timer_process, this);
}

void TransactionConfirmTimer::timer_process(TransactionConfirmTimer* timer)
{
    assert(timer != nullptr);
    
    timer->confirm();
}

void TransactionConfirmTimer::update_id(const string& tx_hash,std::string&id)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto& txconfirm : tx_confirmation_)
    {
        if (txconfirm.tx.txHash == tx_hash)
        {
            txconfirm.ids.push_back(id);
            break;
        }
    }
}


void TransactionConfirmTimer::get_ids(const string& tx_hash,std::vector<std::string>&ids)
 {
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto& txconfirm : tx_confirmation_)
    {
        if (txconfirm.tx.txHash == tx_hash)
        {
           ids = txconfirm.ids;
           break;
        }
    }
 }