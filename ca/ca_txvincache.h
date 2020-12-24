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
        uint64_t amount;
        uint64_t timestamp;
    };

    static string TxToString(const TxVinCache::Tx& tx);

public:
    TxVinCache();
    TxVinCache(TxVinCache &&) = default;
    TxVinCache(const TxVinCache &) = default;
    TxVinCache &operator=(TxVinCache &&) = default;
    TxVinCache &operator=(const TxVinCache &) = default;
    ~TxVinCache();

public:

    // 添加交易
    int Add(const TxVinCache::Tx & tx);
    int Add(const string& hash, const vector<string>& vin, const vector<string>& fromAddr, const vector<string>& toAddr, uint64_t amount);

    // 去除交易
    int Remove(const TxVinCache::Tx& tx);
    int Remove(const std::string& txHash, const vector<string>& from);

    bool IsExist(const TxVinCache::Tx& tx);
    bool IsExist(const std::string& txHash);

    bool IsConflict(const TxVinCache::Tx& tx);
    bool IsConflict(const vector<string>& fromAddr);

    void Print();

    // 全部去除
    int Clear();

    // 获得交易
    int GetAllTx(std::vector<TxVinCache::Tx> & txs);

    // 查询交易
    int Find(const std::string & txHash, TxVinCache::Tx & tx);
    int Find(const std::string & fromAddr, std::vector<TxVinCache::Tx> & txs);

    void DoClearExpire();
    
    
    // 任务处理
    int Process();

    // 定时任务
    static int CheckExpire(TxVinCache * txVinCache);


private:

    std::vector<TxVinCache::Tx> txs_;
    CTimer timer_;
    std::mutex mutex_;

    
};


#endif // !CA_TXWINCACHE_H