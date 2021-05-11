/*
 * @Author: LiuMingLiang
 * @Date: 2020-12-25 14:52:00
 * @LastEditTime: 2020-12-25 14:52:00
 * @LastEditors: 
 * @Description: 
 * @FilePath: \ebpc\ca\ca_txfailurecache.cpp 
 */
#include "ca_txfailurecache.h"
#include <iostream>
#include "../utils/singleton.h"
#include "../utils/time_util.h"

TxFailureCache::TxFailureCache()
{
    //test();
}

bool TxFailureCache::Add(const TxVinCache::Tx& failureTx)
{
    static const size_t MAX_BUFF_SIZE = 1000;
    if (failure_tx_.size() >= MAX_BUFF_SIZE)
    {
        failure_tx_.clear();
    }

    failure_tx_.insert(failureTx);
    cout << "Add to failure transaction list ^^^^^VVVVV " << (failureTx.from.size() > 0 ? failureTx.from[0] : "") << endl;

    return true;
}

void TxFailureCache::GetAll(vector<TxVinCache::Tx>& vectFailureTx)
{
    vectFailureTx.reserve(failure_tx_.size());
    for (auto iter = failure_tx_.begin(); iter != failure_tx_.end(); ++iter)
    {
        vectFailureTx.push_back(*iter);
    }
}

int TxFailureCache::Find(const std::string & fromAddr, std::vector<TxVinCache::Tx> & vectList)
{
    vectList.reserve(64);
    int result = -1;
    for (auto iter = failure_tx_.begin(); iter != failure_tx_.end(); ++iter)
    {
        auto found = find(iter->from.begin(), iter->from.end(), fromAddr);
        if (found != iter->from.end())
        {
            vectList.push_back(*iter);
            result = 0;
        }
    }
    return result;
}

void TxFailureCache::Print()
{
    cout << "Failure transcation in Cache: " << failure_tx_.size() << endl;
    for (auto iter = failure_tx_.begin(); iter != failure_tx_.end(); ++iter)
    {
        cout << "-------------------------------------------------" << endl;
        cout << "Failure transaction: " << iter->txHash << endl;
        
        cout << "from: ";
        for (const auto& id : iter->from)
        {
            cout << id << " ";
        }
        
        cout << endl;
        
        cout << "to: ";
        
        for (const auto& id : iter->to)
        {
            cout << id << " ";
        }

        cout << endl;

        cout << "amount: " <<  iter->amount << endl; 
        cout << "timestamp: " << iter->timestamp << endl;
        
        cout << "-------------------------------------------------" << endl;
        cout << endl;
    }
}

void TxFailureCache::test()
{
    TxVinCache::Tx tx1;
    tx1.txHash = "45c63885c9e85cfa50d660a39cc0f5455fdb9fabffcd1758045619a85193e5cd";
    tx1.from.push_back("1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu");
    tx1.to.push_back("1HjrxHbBuuyNQDwKMh4JtqfuGiDCLodEwC");
    tx1.toAmount.push_back("10");
    tx1.amount = "10";
    tx1.timestamp = Singleton<TimeUtil>::get_instance()->getlocalTimestamp() + 10;
    tx1.gas = "0.05";
    tx1.type = 0;

    TxVinCache::Tx tx2;
    tx2.txHash = "718b78423d45e67a65675a6fdbe195f962b85be39a7cdaa31e4b7433942fdab8";
    tx2.from.push_back("1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu");
    tx2.to.push_back("1BZDmxGwik9EAjnu2BzvWrDsQj8gSbmLJc");
    tx2.toAmount.push_back("20");
    tx2.amount = "20";
    tx2.timestamp = Singleton<TimeUtil>::get_instance()->getlocalTimestamp() + 20;
    tx2.gas = "0.06";
    tx2.type = 1;

    TxVinCache::Tx tx3;
    tx3.txHash = "3aea1cad48eeccde527904a756924868c5f674824cacbbf28caa2c01e326be24";
    tx3.from.push_back("1vkS46QffeM4sDMBBjuJBiVkMQKY7Z8Tu");
    tx3.to.push_back("1MDZgtabZNDSum2tSDxDaFTp2CJnTNvBUR");
    tx3.toAmount.push_back("30");
    tx3.amount = "30";
    tx3.timestamp = Singleton<TimeUtil>::get_instance()->getlocalTimestamp() + 30;
    tx3.gas = "0.07";
    tx3.type = 2;

    Add(tx1);
    Add(tx2);
    Add(tx3);
}