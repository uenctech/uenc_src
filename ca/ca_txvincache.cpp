/*
 * @Author: biz
 * @Date: 2020-12-10 09:08:25
 * @LastEditTime: 2021-03-01 11:33:07
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_txvincache.cpp
 */

#include "ca_txvincache.h"
#include "ca_txfailurecache.h"
#include "ca_txconfirmtimer.h"
#include "ca_MultipleApi.h"
#include "ca_transaction.h"
#include "MagicSingleton.h"
#include "../utils/singleton.h"
#include "../utils/time_util.h"
#include "proto/ca_protomsg.pb.h"
#include "../include/net_interface.h"
#include "../common/config.h"
#include "ca_rocksdb.h"
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

int TxVinCache::Add(const CTransaction& tx, bool broadcast/* = true*/)
{
    if (!VerifySign(tx))
    {
        cout << "In add, verify sign failed!^^^^^VVVVVV" << endl;
        return -1;
    }

    TxVinCache::Tx newTx;
    ConvertTx(tx, newTx);

    int result = Add(newTx);
    if (result == 0 && broadcast)
    {
        BroadcastTxPending(tx);
    }

    return result;
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
    tx.timestamp = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();

    return Add(tx);
}

bool TxVinCache::VerifySign(const CTransaction& tx)
{
    int verifyPreHashCount = 0;
	std::string txBlockHash;
    std::string txHashStr;

    // Calc hash
    CTransaction copyTx = tx;
    for (int i = 0; i != copyTx.vin_size(); ++i)
    {
        CTxin * txin = copyTx.mutable_vin(i);
        txin->clear_scriptsig();
    }
    copyTx.clear_signprehash();
    copyTx.clear_hash();

    std::string serCopyTx = copyTx.SerializeAsString();
    size_t encodeLen = serCopyTx.size() * 2 + 1;
    unsigned char encode[encodeLen] = {0};
    memset(encode, 0, encodeLen);
    long codeLen = base64_encode((unsigned char *)serCopyTx.data(), serCopyTx.size(), encode);
    std::string encodeStr( (char *)encode, codeLen );
    txHashStr = getsha256hash(encodeStr);

    // Verify
    bool result = VerifyTransactionSign(tx, verifyPreHashCount, txBlockHash, txHashStr);
    return result;
}

void TxVinCache::ConvertTx(const CTransaction& tx, TxVinCache::Tx& newTx)
{
    newTx.txHash = tx.hash();
    for (int i = 0; i < tx.vin_size(); i++)
    {
        CTxin txin = tx.vin(i);
        std::string hash = txin.prevout().hash();
        newTx.vins.push_back(hash);
    }

    std::vector<std::string> txOwnerVec;
    SplitString(tx.txowner(), txOwnerVec, "_");
    newTx.from = txOwnerVec;

    uint64_t amount = 0;
    std::vector<std::string> toId;
    std::vector<std::string> toAmount;
    for (int i = 0; i < tx.vout_size(); i++)
    {
        CTxout out = tx.vout(i);
        auto found = std::find(txOwnerVec.begin(), txOwnerVec.end(), out.scriptpubkey());
        if (found == txOwnerVec.end())
        {
            toId.push_back(out.scriptpubkey());
            toAmount.push_back(to_string(((double)out.value()) / DECIMAL_NUM));
            amount += out.value();
        }
    }					
    newTx.amount = to_string(((double)amount) / DECIMAL_NUM);
    newTx.to = toId;
    newTx.toAmount = toAmount;
    newTx.timestamp = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();

    nlohmann::json extra = nlohmann::json::parse(tx.extra());
    uint64_t signFee = extra["SignFee"].get<uint64_t>();               
    uint64_t needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<uint64_t>();
    uint64_t packageFee = extra["PackageFee"].get<uint64_t>();
    uint64_t gas = signFee * (needVerifyPreHashCount - 1);
    bool flag = IsNeedPackage(tx);
    if(flag)
    {
       gas += packageFee; 
    }
    
    newTx.gas = to_string(((double)gas)/DECIMAL_NUM);

    uint32_t type = 0;
    string txType = extra["TransactionType"].get<std::string>();
    if (txType == TXTYPE_TX)
        type = 0;
    else if (txType == TXTYPE_PLEDGE)
        type = 1;
    else if (txType == TXTYPE_REDEEM)
        type = 2;
    newTx.type = type;

    if (txType == TXTYPE_REDEEM)
    {
        nlohmann::json txInfo = extra["TransactionInfo"].get<nlohmann::json>();
        std::string utxoStr = txInfo["RedeemptionUTXO"].get<std::string>();
        std::string strTxRaw;

        auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
        Transaction* txn = pRocksDb->TransactionInit();
        if (txn != nullptr)
        {
            if (pRocksDb->GetTransactionByHash(txn, utxoStr, strTxRaw) == 0)
            {
                CTransaction utxoTx;
                utxoTx.ParseFromString(strTxRaw);
                for (int i = 0; i < utxoTx.vout_size(); i++)
                {
                    CTxout txout = utxoTx.vout(i);
                    if (txout.scriptpubkey() == VIRTUAL_ACCOUNT_PLEDGE)
                    {
                        uint64_t outAmount = txout.value();
                        newTx.amount = to_string(((double)outAmount) / DECIMAL_NUM);
                        newTx.to = newTx.from;
                        std::vector<std::string> currentToAmount = { newTx.amount };
                        newTx.toAmount = currentToAmount;
                        break ;
                    }
                }
                
            }
            pRocksDb->TransactionDelete(txn, true);
        }
    }

}

void TxVinCache::BroadcastTxPending(const CTransaction& tx)
{
    string transactionRaw = tx.SerializeAsString();
    TxPendingBroadcastMsg txPendingMsg;
    txPendingMsg.set_version(getVersion());
    txPendingMsg.set_txraw(transactionRaw);

    std::vector<Node> vnode = net_get_public_node();
    for (auto & item : vnode)
    {
        net_send_message<TxPendingBroadcastMsg>(item.id, txPendingMsg, net_com::Priority::kPriority_Middle_1);
    }
    
    // net_broadcast_message<TxPendingBroadcastMsg>(txPendingMsg);
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

void TxVinCache::SetBroadcastFlag(const string& txhash)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == txhash)
        {
            iter->isBroadcast = 1;
            break ;
        }
    }
}
bool TxVinCache::IsBroadcast(const string& txhash)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == txhash)
        {
            if (iter->isBroadcast == 0)
            {
                return false;
            }
            else if (iter->isBroadcast == 1)
            {
                return true;
            }
            else
            {
                return true;
            }
        }
    }

    return false;
}

void TxVinCache::UpdateTransactionBroadcastTime(const string& txhash)
{
    std::lock_guard<std::mutex> lck(mutex_);

    for (auto iter = txs_.begin(); iter != txs_.end(); ++iter)
    {
        if (iter->txHash == txhash)
        {
            uint64_t nowTime = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
            iter->txBroadcastTime = nowTime;
            break ;
        }
    }
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

string TxVinCache::TxToString(const CTransaction& tx)
{
    std::vector<std::string> txOwnerVec;
    SplitString(tx.txowner(), txOwnerVec, "_");

    stringstream info;
    info << "hash: " << tx.hash();
    info << " from: ";
    for (const auto& id : txOwnerVec)
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
        uint64_t nowTime = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
        //static const uint64_t MINUTES5 = 1000000UL * 60UL * 3UL;
        //uint64_t syncTime = Singleton<Config>::get_instance()->GetSyncDataPollTime();
        //const uint64_t WAIT_TIME = syncTime * 4 * 1000000UL;
        static const uint64_t WAIT_TIME = 1000000UL * 60UL * 10UL;
        if (nowTime - iter->timestamp >= WAIT_TIME)
        {
            SendTransactionConfirm(iter->txHash, ConfirmTxFlag);
            MagicSingleton<TransactionConfirmTimer>::GetInstance()->add(*iter);
            std::cout << "Start confirm " << iter->txHash << std::endl;
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