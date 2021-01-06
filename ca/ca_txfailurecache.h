#ifndef _TX_FAILURE_CACHE_H_
#define _TX_FAILURE_CACHE_H_
/*
 * @Author: LiuMingLiang
 * @Date: 2020-12-25 14:52:00
 * @LastEditTime: 2020-12-25 14:52:00
 * @LastEditors: 
 * @Description: 
 * @FilePath: \ebpc\ca\ca_txfailurecache.h
 */
#include "ca_txvincache.h"
#include <string>
#include <vector>
#include <set>
#include <mutex>

using namespace std;


class TxFailureCache
{
public:
    TxFailureCache();
    TxFailureCache(const TxFailureCache& failure) = default;
    TxFailureCache(TxFailureCache&& failure) = default;
    TxFailureCache& operator=(const TxFailureCache& failure) = default;
    TxFailureCache& operator=(TxFailureCache&& failure) = default;

    bool Add(const TxVinCache::Tx& failure);
    void GetAll(vector<TxVinCache::Tx>& vectList);
    int Find(const std::string & fromAddr, std::vector<TxVinCache::Tx> & vectList);

    void Print();
    void test();

private:
    struct TxCompare
    {
        bool operator()(const TxVinCache::Tx& tx1, const TxVinCache::Tx& tx2) const
        {
            return ((tx1.timestamp - tx2.timestamp) > 0);
            //return (tx1.timestamp <= tx2.timestamp);
        }
    };

private:
    set<TxVinCache::Tx, TxCompare> failure_tx_;
};

#endif // _TX_FAILURE_CACHE_H_