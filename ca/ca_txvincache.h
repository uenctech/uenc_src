/*
 * @Author: your name
 * @Date: 2020-12-10 09:08:01
 * @LastEditTime: 2020-12-10 14:29:11
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_txvincache.h
 */
//


#ifndef CA_TXWINCACHE_H
#define CA_TXWINCACHE_H

#include "../utils/CTimer.hpp"
#include "proto/transaction.pb.h"

#include <string>
#include <vector>
#include <mutex>

using namespace std;


class TxVinCache
{
// define
public:
    class Tx
    {
    public:
        std::string txHash;
        std::vector<std::string> vins;
        std::vector<std::string> from;
        std::vector<std::string> to;
        std::string amount;
        uint64_t timestamp;
        string gas;
        std::vector<std::string> toAmount;
        uint32_t type;
        int isBroadcast = 0;
        uint64_t txBroadcastTime = 0;
    };

    static string TxToString(const TxVinCache::Tx& tx);
    static string TxToString(const CTransaction& tx);

public:
    TxVinCache();
    TxVinCache(TxVinCache &&) = default;
    TxVinCache(const TxVinCache &) = default;
    TxVinCache &operator=(TxVinCache &&) = default;
    TxVinCache &operator=(const TxVinCache &) = default;
    ~TxVinCache();

public:
    void SetBroadcastFlag(const string& txhash);
    bool IsBroadcast(const string& txhash);

    void UpdateTransactionBroadcastTime(const string& txhash);

    // Add transaction 
    int Add(const CTransaction& tx, bool broadcast = true);

    // Remove transaction 
    int Remove(const TxVinCache::Tx& tx);
    int Remove(const std::string& txHash, const vector<string>& from);

    bool IsExist(const TxVinCache::Tx& tx);
    bool IsExist(const std::string& txHash);

    bool IsConflict(const TxVinCache::Tx& tx);
    bool IsConflict(const vector<string>& fromAddr);

    void Print();

    // Remove all 
    int Clear();

    // Get a deal 
    int GetAllTx(std::vector<TxVinCache::Tx> & txs);

    // Query transaction 
    int Find(const std::string & txHash, TxVinCache::Tx & tx);
    int Find(const std::string & fromAddr, std::vector<TxVinCache::Tx> & txs);

    void DoClearExpire();
    
    
    // Task processing 
    int Process();

    // Timed task 
    static int CheckExpire(TxVinCache * txVinCache);

private:
    int Add(const TxVinCache::Tx & tx);
    int Add(const string& hash, const vector<string>& vin, const vector<string>& fromAddr, const vector<string>& toAddr, uint64_t amount);

    bool VerifySign(const CTransaction& tx);
    void ConvertTx(const CTransaction& tx, TxVinCache::Tx& newTx);
    void BroadcastTxPending(const CTransaction& tx);


private:

    std::vector<TxVinCache::Tx> txs_;
    CTimer timer_;
    std::mutex mutex_;

    
};


#endif // !CA_TXWINCACHE_H