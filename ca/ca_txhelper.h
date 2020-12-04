/*
 * @Author: your name
 * @Date: 2020-09-18 18:01:27
 * @LastEditTime: 2020-10-14 10:08:56
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
     * @description: 创建交易信息
     * @param fromAddr 交易发起方 
     * @param fromAddr 交易接收方
     * @param needVerifyPreHashCount 共识数
     * @param minerFees 矿费
     * @param outTx 返回的交易信息结构体
     * @return 
     * 0，创建交易成功
     * -1，参数错误
     * -2，交易地址错误（地址不正确或是接收方中有交易方地址）
     * -3，获取交易信息失败
     * -4，获取交易信息失败
     * -5，余额不足
     */
    static int CreateTxMessage(const std::vector<std::string> & fromAddr, const std::map<std::string, int64_t> toAddr, uint32_t needVerifyPreHashCount, uint64_t minerFees, CTransaction & outTx, bool is_local = true);
    static void DoCreateTx(const std::vector<std::string> & fromAddr, const std::map<std::string, int64_t> toAddr, uint32_t needVerifyPreHashCount, uint64_t minerFees);
    static uint64_t GetUtxoAmount(std::string tx_hash, std::string address);
    static std::vector<std::string> GetUtxosByAddresses(std::vector<std::string> addresses);
    static std::vector<std::string> GetUtxosByTx(const CTransaction & tx);

};




#endif







