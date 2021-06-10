/*
 * @Author: your name
 * @Date: 2020-09-18 18:01:27
 * @LastEditTime: 2021-01-28 10:39:35
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_txhelper.h
 */
#ifndef _TXHELPER_H_
#define _TXHELPER_H_

#include <map>
#include <list>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <vector>
#include <bitset>
#include <iostream>
#include "../proto/ca_protomsg.pb.h"



class TxHelper
{
public:
    TxHelper() = default;
    ~TxHelper() = default;

    static std::vector<std::string> GetTxOwner(const std::string tx_hash);
    static std::vector<std::string> GetTxOwner(const CTransaction & tx);
    static std::string GetTxOwnerStr(const std::string tx_hash);
    static std::string GetTxOwnerStr(const CTransaction & tx);


    /**
     * @description: Create transaction information 
     * @param fromAddr Transaction initiator 
     * @param fromAddr Transaction recipient 
     * @param needVerifyPreHashCount Consensus number 
     * @param minerFees Mining fee 
     * @param outTx Returned transaction information structure 
     * @return 
     * 0，Successfully created transaction 
     * -1，Parameter error 
     * -2，The transaction address is wrong (the address is incorrect or there is a transaction party address in the receiver) 
     * -3，Previous transaction pending 
     * -4，Open database error 
     * -5，Failed to get packing fee 
     * -6，Failed to obtain transaction information 
     * -7，Insufficient balance 
     * -8, The handling fee is too high or too low 
     * 
     */
    static int CreateTxMessage(const std::vector<std::string> & fromAddr, const std::map<std::string, int64_t> toAddr, uint32_t needVerifyPreHashCount, uint64_t minerFees, CTransaction & outTx, bool is_local = true);
    static void DoCreateTx(const std::vector<std::string> & fromAddr, const std::map<std::string, int64_t> toAddr, uint32_t needVerifyPreHashCount, uint64_t minerFees);
    static uint64_t GetUtxoAmount(std::string tx_hash, std::string address);
    static std::vector<std::string> GetUtxosByAddresses(std::vector<std::string> addresses);
    static std::vector<std::string> GetUtxosByTx(const CTransaction & tx);

};




#endif

