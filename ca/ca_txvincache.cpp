/*
 * @Author: biz
 * @Date: 2020-12-10 09:08:25
 * @LastEditTime: 2020-12-10 13:10:23
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_txvincache.cpp
 */

#include "ca_txvincache.h"
#include "ca_txfailurecache.h"
#include "ca_MultipleApi.h"
#include "MagicSingleton.h"
#include "../utils/time_util.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <time.h>


TxVinCache::TxVinCache()
{
}

TxVinCache::~TxVinCache()
{
    
}

// 添加交易
int TxVinCache::Add(const TxVinCache::Tx & tx)
{
    if (IsExist(tx))
    {
        return 1;
    }

    std::lock_guard<std::mutex> lck(mutex_);
    txs_.push_back(tx);
    return 0;
}

int TxVinCache::Add(const string& hash, const vector<string>& vin, const vector<string>& fromAddr, const vector<string>& toAddr, uint64_t amount)
{
    Tx tx;
    tx.txHash = hash;
    tx.vins = vin;
    tx.from = fromAddr;
    tx.to = toAddr;
    tx.amount = amount;
    tx.timestamp = time(NULL);

    return Add(tx);
}

// 去除交易
int TxVinCache::Remove(const TxVinCache::Tx & tx)
{
    std::lock_guard<std::mutex> lck(mutex_);

    if (txs_.empty())
    {
        return 1;
    }

    int result = -1;
    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == tx.txHash )
        {
            txs_.erase(iter);
            result = 0;
            break;
        }
    }
    return result;
}

int TxVinCache::Remove(const std::string& txHash, const vector<string>& from)
{
    Tx tx;
    tx.txHash = txHash;
    tx.from = from;
    return Remove(tx);
}

bool TxVinCache::IsExist(const TxVinCache::Tx& tx)
{
    std::lock_guard<std::mutex> lck(mutex_);
    
    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == tx.txHash)
        {
            return true;
        }
    }

    return false;
}

bool TxVinCache::IsExist(const std::string& txHash)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == txHash)
        {
            return true;
        }
    }

    return false;
}

bool TxVinCache::IsConflict(const TxVinCache::Tx& tx)
{
    std::lock_guard<std::mutex> lck(mutex_);

    // Check from address
    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        for (size_t i = 0; i < tx.from.size(); i++)
        {
            auto found = find(iter->from.begin(), iter->from.end(), tx.from[i]);
            if (found != iter->from.end())
            {
                return true;
            }
        }
    }

    return false;
}

bool TxVinCache::IsConflict(const vector<string>& fromAddr)
{
    TxVinCache::Tx tx;
    tx.from = fromAddr;
    return IsConflict(tx);
}

void TxVinCache::Print()
{
    cout << "Pending transcation in Cache: " << txs_.size() << endl;
    for (const auto& iter : txs_)
    {
        cout << "Transaction Hash: " << iter.txHash << " from: ";
        for (const auto& id : iter.from)
        {
            cout << id << " ";
        }
        cout << endl;
    }
}

string TxVinCache::TxToString(const TxVinCache::Tx& tx)
{
    stringstream info;
    info << "hash: " << tx.txHash;
    info << " from: ";
    for (const auto& id : tx.from)
    {
        info << id << " ";
    }
    
    return info.str();
}

// 全部去除
int TxVinCache::Clear()
{
    std::lock_guard<std::mutex> lck(mutex_);

    txs_.clear();
    return 0;
}

// 获得交易
int TxVinCache::GetAllTx(std::vector<TxVinCache::Tx> & vectTxs)
{
    std::lock_guard<std::mutex> lck(mutex_);
    
    for (const auto& iter : txs_ )
    {
        vectTxs.push_back(iter);
    }
    return 0;
}

// 查询交易
int TxVinCache::Find(const std::string & txHash, TxVinCache::Tx & tx)
{
    std::lock_guard<std::mutex> lck(mutex_);

    int result = -1;
    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == txHash)
        {
            tx = *iter;
            result = 0;
            break;            
        }
    }
    return result;
}

int TxVinCache::Find(const std::string & fromAddr, std::vector<TxVinCache::Tx> & vectTxs)
{
    std::lock_guard<std::mutex> lck(mutex_);

    int result = -1;
    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        auto found = find(iter->from.begin(), iter->from.end(), fromAddr);
        if (found != iter->from.end())
        {
            vectTxs.push_back(*iter);
            result = 0;
        }
    }
    return result;
}

void TxVinCache::DoClearExpire()
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = txs_.begin(); iter != txs_.end();)
    {
        uint64_t nowTime = Singleton<TimeUtil>::get_instance()->getTimestamp();
        static const uint64_t MINUTES5 = 1000000UL * 60UL * 5UL;
        if (nowTime - iter->timestamp >= MINUTES5)
        {
            vector<string>txhash = { iter->txHash };
            vector<CTransaction> outTx;
            bool result =  GetTxByTxHashFromRocksdb(txhash,outTx);
            if (!result)
            {
                MagicSingleton<TxFailureCache>::GetInstance()->Add(*iter);
            }
 
            iter = txs_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

int TxVinCache::Process()
{
    this->timer_.AsyncLoop(1000 * 60 * 1, TxVinCache::CheckExpire, this);

    return 0;
}

int TxVinCache::CheckExpire(TxVinCache * txVinCache)
{
    if (txVinCache == nullptr)
    {
        return -1;    
    }
    
    // 定时任务查询时间戳是否超过5分钟
    // 超过的话将从列表中取消 
    txVinCache->DoClearExpire();

    return 0;
}