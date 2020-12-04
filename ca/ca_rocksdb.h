#ifndef _CA_ROCKSDB_H_
#define _CA_ROCKSDB_H_
#include <iostream>
#include <cstring>
#include <assert.h>
#include <string>
#include <sstream>
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

using namespace std;
using namespace rocksdb;

class Rocksdb 
{
public:
    Rocksdb();
    ~Rocksdb();

/*====================================================================================  
 # @description: 区块相关的接口
==================================================================================== */


/* ====================================================================================  
 # @description: 通过块哈希设置数据块的高度
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希字符串 
 # @param blockHeight : 设置数据块的高度 
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // // block hash -> block num
    int SetBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash, const unsigned int blockHeight);
 
/* ====================================================================================  
 # @description: 通过块哈希获取到数据块的高度
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希字符串 
 # @param blockHeight : 获取到的数据块的高度 (只有一个高度)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */   
    int GetBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash, unsigned int & blockHeight);
 
/* ====================================================================================  
 # @description: 通过块哈希移除数据库当中的块高度
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希字符串 
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */     
    int DeleteBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: 通过块高度设置数据块的哈希(在并发的时候同一高度可能有多个块哈希)
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHeight : 数据块的高度 
 # @param blockHash  :  块哈希字符串 
 # @param is_mainblock : 是否是主链上的块(并发的时候同一高度可能会有多个块)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // // block num -> block hash (mutli)
    int SetBlockHashByBlockHeight(Transaction* txn, const unsigned int blockHeight, const std::string & blockHash, bool is_mainblock = false);

/* ====================================================================================  
 # @description: 通过数据块的高度获取单个块的哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHeight  : 数据块的高度
 # @param hash : 通过块高度获取到块的哈希(单个哈希) 
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBlockHashByBlockHeight(Transaction* txn, const int blockHeight, std::string &hash);

/* ====================================================================================  
 # @description: 通过数据块的高度获取到多个块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHeight : 设置数据块的高度 
 # @param hashes  : 块哈希字符串 (多个哈希放入一个std::vector<std::string>里边)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBlockHashsByBlockHeight(Transaction* txn, const unsigned int blockHeight, std::vector<std::string> &hashes);

/* ====================================================================================  
 # @description: 通过块高度移除数据库里边块的哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHeight : 数据块的高度
 # @param blockHash  : 块哈希字符串 
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int RemoveBlockHashByBlockHeight(Transaction* txn, const unsigned int blockHeight, const std::string & blockHash);

/* ====================================================================================  
 # @description: 通过块哈希设置块
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希字符串
 # @param block : 需要设置的块数据 (二进制是数据块可以直接放入protobuf里边进行传输的)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // block hash -> block 
    int SetBlockByBlockHash(Transaction* txn, const std::string & blockHash, const std::string & block);

/* ====================================================================================  
 # @description: 通过块哈希获取到数据块信息
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希字符串
 # @param block : 获取到的块数据 (二进制是数据块可以直接放入protobuf里边进行传输的)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */ 
    int GetBlockByBlockHash(Transaction* txn, const std::string & blockHash, std::string & block);

/* ====================================================================================  
 # @description: 通过块哈希移除数据块里边的块
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希字符串
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */  
    int DeleteBlockByBlockHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: 设置最高块
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHeight  : 需要设置的数据块的高度
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */ 
    // block Top -> top
    int SetBlockTop(Transaction* txn, const unsigned int blockHeight);

/* ====================================================================================  
 # @description: 获取最高块
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHeight  : 获取到的数据块的高度
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */ 
    int GetBlockTop(Transaction* txn, unsigned int & blockHeight);

/* ====================================================================================  
 # @description: 设置最佳链
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 需要设置的最佳链的块哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */ 
    // best chain
    int SetBestChainHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: 获取到最佳链
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 获取到的最佳链当中的块哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBestChainHash(Transaction* txn, std::string & blockHash);

/* ====================================================================================  
 # @description: 通过块的哈希设置块头信息
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 获取到的最佳链当中的块哈希
 # @param header  : 需要设置的块头信息(二进制数据可以直接进行protobuf里边进行传输的)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */ 
    //====
    int SetBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash, const std::string & header);

/* ====================================================================================  
 # @description: 通过块的哈希获取到块头信息
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 获取到的最佳链当中的块哈希
 # @param header  : 获取到的块头信息(二进制数据可以直接进行protobuf里边进行传输的)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash, std::string & header);
 
/* ====================================================================================  
 # @description: 根据块哈希移除数据库里边的块头信息
 # @param txn  : Rocksdb当中的事务指针
 # @param blockHash  : 块哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int DeleteBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: 通过地址设置Utxo哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param utxoHash  : 需要设置的utxohash
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // UTXO
    int SetUtxoHashsByAddress(Transaction* txn, const std::string & address, const std::string & utxoHash);

/* ====================================================================================  
 # @description: 通过地址获取Utxo哈希(utxohash有多个)
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param utxoHashs  : 获取到的utxohash(有多个utxo放入std::vector<std::string>当中)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetUtxoHashsByAddress(Transaction* txn, const std::string & address, std::vector<std::string> & utxoHashs);

/* ====================================================================================  
 # @description: 通过地址移除Utxo哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param utxoHash  : 需要移除的utxohash
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */   
    int RemoveUtxoHashsByAddress(Transaction* txn, const std::string & address, const std::string & utxoHash);

/* ====================================================================================  
 # @description: 通过交易哈希设置交易原始数据
 # @param txn  : Rocksdb当中的事务指针
 # @param txHash  : 交易哈希
 # @param txRaw  : 需要的交易原始数据(交易数据是二进制数据可以直接放入protobuf里边进行数据传输)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // txhash -> txRaw
    int SetTransactionByHash(Transaction* txn, const std::string & txHash, const std::string & txRaw);

/* ====================================================================================  
 # @description: 通过交易哈希获取交易原始数据
 # @param txn  : Rocksdb当中的事务指针
 # @param txHash  : 交易哈希
 # @param txRaw  : 获取到的交易原始数据(交易数据是二进制数据可以直接放入protobuf里边进行数据传输)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetTransactionByHash(Transaction* txn, const std::string & txHash, std::string & txRaw);

/* ====================================================================================  
 # @description: 通过交易哈希移除数据库里边的交易原始数据
 # @param txn  : Rocksdb当中的事务指针
 # @param txHash  : 交易哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */   
    int DeleteTransactionByHash(Transaction* txn, const std::string & txHash);

/* ====================================================================================  
 # @description: 通过交易哈希设置块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param txHash  : 交易哈希
 # @param blockHash  : 块哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // txHash -> blockHash
    int SetBlockHashByTransactionHash(Transaction* txn, const std::string & txHash, const std::string & blockHash);

/* ====================================================================================  
 # @description: 通过交易哈希获取块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param txHash  : 交易哈希
 # @param blockHash  : 块哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBlockHashByTransactionHash(Transaction* txn, const std::string & txHash, std::string & blockHash);

/* ====================================================================================  
 # @description: 通过交易哈希移除数据库里边的块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param txHash  : 交易哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int DeleteBlockHashByTransactionHash(Transaction* txn, const std::string & txHash);

//////////////////// TODO: 可能被废弃 start
/* ====================================================================================  
 # @description: 交易查询相关
==================================================================================== */

/* ====================================================================================  
 # @description: 通过交易地址设置块交易
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txNum  : 交易序列号
 # @param txRaw  : 交易原始数据(原始数据都是可以直接放入protobuf当中的)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
  // addr + tx num -> tx raw
    int SetTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, const std::string & txRaw);

/* ====================================================================================  
 # @description: 通过交易地址获取块交易
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txNum  : 交易序列号
 # @param txRaw  : 交易原始数据(原始数据都是可以直接放入protobuf当中的)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, std::string & txRaw);

/* ====================================================================================  
 # @description: 通过交易地址移除数据库里边的交易数据
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txNum  : 交易序列号
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int DeleteTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum);

/* ====================================================================================  
 # @description: 通过交易地址设置块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txNum  : 交易序列号
 # @param blockHash  : 块哈希值
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // // addr + tx num -> block hash
    int SetBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, const std::string & blockHash);

/* ====================================================================================  
 # @description: 通过交易地址获取块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txNum  : 交易序列号
 # @param blockHash  : 块哈希值
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, std::string & blockHash);

/* ====================================================================================  
 # @description: 通过交易地址移除数据库里边的块哈希
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txNum  : 交易序列号
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
   int DeleteBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum);

/* ====================================================================================  
 # @description: 通过交易地址设置交易最高高度
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txIndex  : 交易序列号
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */   
    // addr Top -> addr tx top
    int SetTransactionTopByAddress(Transaction* txn, const std::string & address, const unsigned int txIndex);

/* ====================================================================================  
 # @description: 通过交易地址获取交易最高高度
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txIndex  : 交易序列号
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetTransactionTopByAddress(Transaction* txn, const std::string & address, unsigned int & txIndex);

//////////////////// TODO: 可能被废弃 end
    
/* ====================================================================================  
 # @description:  应用层查询
==================================================================================== */

/* ====================================================================================  
 # @description: 通过交易地址设置账号余额
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param balance  : 账号余额
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // addr -> balance
    int SetBalanceByAddress(Transaction* txn, const std::string & address, int64_t balance);

/* ====================================================================================  
 # @description: 通过交易地址获取账号余额
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param balance  : 账号余额
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetBalanceByAddress(Transaction* txn, const std::string & address, int64_t & balance);

/* ====================================================================================  
 # @description: 通过交易地址设置所有交易
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txHash  : 交易哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */  
    // addr -> transcations
    int SetAllTransactionByAddress(Transaction* txn, const std::string & address, const std::string & txHash);

/* ====================================================================================  
 # @description: 通过交易地址获取所有交易
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txHash  : 交易哈希(交易哈希有多个放入std::vector<std::string>当中)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetAllTransactionByAddreess(Transaction* txn, const std::string & address, std::vector<std::string> & txHashs);

/* ====================================================================================  
 # @description: 通过交易地址移数据库里边的所有交易
 # @param txn  : Rocksdb当中的事务指针
 # @param address  : 交易地址
 # @param txHash  : 交易哈希
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int RemoveAllTransactionByAddress(Transaction* txn, const std::string & address, const std::string & txHash);

/* ====================================================================================  
 # @description: 设置设备打包费
 # @param publicNodePackageFee  : 设备打包费
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    //  publicnodepackagefee
    int SetDevicePackageFee(const uint64_t publicNodePackageFee);

/* ====================================================================================  
 # @description: 获取设备打包费
 # @param publicNodePackageFee  : 设备打包费
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetDevicePackageFee(uint64_t & publicNodePackageFee);

/* ====================================================================================  
 # @description: 设置设备签名费
 # @param mineSignatureFee  : 设备签名费
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    //  minesignaturefee
    int SetDeviceSignatureFee(const uint64_t mineSignatureFee);

/* ====================================================================================  
 # @description: 获取设备签名费
 # @param mineSignatureFee  : 设备签名费
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetDeviceSignatureFee(uint64_t & mineSignatureFee);

/* ====================================================================================  
 # @description: 设置设备在线时长
 # @param minerOnLineTime  : 设备在线时长
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
     // 矿机在线时长
    int SetDeviceOnlineTime(const double minerOnLineTime);

/* ====================================================================================  
 # @description: 获取设备在线时长
 # @param minerOnLineTime  : 设备在线时长
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetDeviceOnLineTime(double & minerOnLineTime);


/* ====================================================================================  
 # @description: 设置质押地址
 # @param txn  : Rocksdb当中的事务
 # @param address  : 交易地址
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    //设置、获取、移除滞押资产账户地址
    int SetPledgeAddresses(Transaction* txn,  const std::string &address); 

/* ====================================================================================  
 # @description: 获取质押地址
 # @param txn  : Rocksdb当中的事务
 # @param address  : 交易地址(地址有多个用std::vector<string>存放)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetPledgeAddress(Transaction* txn,  std::vector<string> &addresses);

/* ====================================================================================  
 # @description: 移除数据库当中的质押地址
 # @param txn  : Rocksdb当中的事务
 # @param address  : 交易地址
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int RemovePledgeAddresses(Transaction* txn,  const std::string &address);

/* ====================================================================================  
 # @description: 设置质押地址的Utxo
 # @param txn  : Rocksdb当中的事务
 # @param address  : 交易地址
 # @param utxo  : utxo
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */   
    //设置、获取、移除滞押资产账户的utxo
    int SetPledgeAddressUtxo(Transaction* txn, const std::string &address, const std::string &utxo); 

/* ====================================================================================  
 # @description: 获取质押地址的Utxo
 # @param txn  : Rocksdb当中的事务
 # @param address  : 交易地址
 # @param utxo  : utxo(utxo有多个用std::vector<string>存放)
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetPledgeAddressUtxo(Transaction* txn, const std::string &address, std::vector<string> &utxos);

/* ====================================================================================  
 # @description:   移除数据当中的Utxo
 # @param txn  : Rocksdb当中的事务
 # @param address  : 交易地址
 # @param utxo  : utxo
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
   int RemovePledgeAddressUtxo(Transaction* txn, const std::string &address,const std::string &utxos);

/* ====================================================================================  
 # @description: 设置交易总数
 # @param txn  : Rocksdb当中的事务
 # @param count  : 交易个数
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    /* 交易统计 */
    // 获取与设置总交易数
    int SetTxCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: 获取交易总数
 # @param txn  : Rocksdb当中的事务
 # @param count  : 交易个数
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetTxCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: 设置总燃料费
 # @param txn  : Rocksdb当中的事务
 # @param count  : 燃料总额
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // 获取与设置总燃料费
    int SetGasCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: 获取总燃料费
 # @param txn  : Rocksdb当中的事务
 # @param count  : 燃料总额
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetGasCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: 设置总的额外奖励
 # @param txn  : Rocksdb当中的事务
 # @param count  : 总的额外奖励
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // 获取与设置总额外奖励费
    int SetAwardCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: 获取总的额外奖励
 # @param txn  : Rocksdb当中的事务
 # @param count  : 总的额外奖励
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    int GetAwardCount(Transaction* txn, uint64_t &count);


    // 新增获得矿费和奖励总值接口，待完善
    int GetGasTotal(Transaction* txn, uint64_t & gasTotal);
    int SetGasTotal(Transaction* txn, const uint64_t & gasTotal);
    int GetAwardTotal(Transaction* txn, uint64_t &awardTotal);
    int SetAwardTotal(Transaction* txn, const uint64_t &awardTotal);


/* ====================================================================================  
 # @description: 设置数据库程序版本
 # @param txn  : Rocksdb当中的事务
 # @param version  : 程序版本
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
    // 记录初始化数据库的程序版本
    int SetInitVer(Transaction* txn, const std::string & version);

/* ====================================================================================  
 # @description: 设置数据库程序版本
 # @param txn  : Rocksdb当中的事务
 # @param version  : 程序版本
 # @return : 设置成功返回0，错误返回大于零的数。ROCKSDB_ERR = 1, //错误 ROCKSDB_PARAM_NULL = 2, //参数不合法(可以在enum当中查看)
 ==================================================================================== */
   int GetInitVer(Transaction* txn, std::string & version);
    
    std::string getKey(const std::string &key);

public:
    Transaction* TransactionInit();
    int TransactionCommit(Transaction* txn);
    void TransactionDelete(Transaction* txn, bool bRollback);

private:
    template <typename T>
    int writedata(Transaction* txn, const std::string &blockkey, const T &blockvalue);
    int writedata(const std::string &blockkey, const std::string &blockvalue);

    template <typename T>
    int readdata(Transaction* txn, const std::string &blockkey, T &blockvalue);
    int readdata(const std::string &blockkey, std::string &blockvalue);

    int deletedata(Transaction* txn, const std::string &blockkey);
    int StringSplit(std::vector<std::string>& dst, const std::string& src, const std::string& separator);

private:
    const std::string PATH = "data.db";
    // DB *db;

    TransactionDB* db;
    WriteOptions write_options;
    ReadOptions read_options;

/** 前缀 
 * hs = hash
 * ht = height
 * hd = header
 * bal = balance
 * bh = block hash
 * blk = block
*/

/* ====================================================================================  
 # @description: 区块相关的接口
==================================================================================== */
    const std::string kBlockHash2BlockHeightKey = "blkhs2blkht"; //hash to height
    const std::string kBlockHeight2BlockHashKey = "blkht2blkhs"; // blkht2blkhs 
    const std::string kBlockHash2BlcokRawKey = "blkhs2blkraw"; // blkhs2blkhd  
    const std::string kBlockTopKey = "blktop"; // blktop
    const std::string kBestChainHashKey = "bestchainhash"; // bestchainhash
    const std::string kBlockHash2BlockHeadRawKey = "blkhs2blkhdraw"; //blkhs2blkhdraw
    const std::string kAddress2UtxoKey= "addr2utxo"; //utxo addr2utxo 
    const std::string kTransactionHash2TransactionRawKey = "txhs2txraw"; //txhs2txraw
    const std::string kTransactionHash2BlockHashKey = "txhs2blkhs"; // txhs2blkhs   
/* ====================================================================================  
 # @description: 交易查询相关
==================================================================================== */
    const std::string kAddress2TransactionRawKey = "addr2txraw"; // addr2txraw 
    const std::string kAddress2BlcokHashKey = "addr2blkhs"; // addr2blkhs
    const std::string kAddress2TransactionTopKey = "addr2txtop"; // addr2txtop
/* ====================================================================================  
 # @description:  应用层查询
==================================================================================== */
    const std::string kAddress2BalanceKey = "addr2bal"; // addr2bal
    const std::string kAddress2AllTransactionKey = "addr2atx"; // addr2atx
    const std::string kPackageFeeKey ="packagefee"; // packagefee
    const std::string kGasFeeKey ="gasfee"; // gasfee   
    const std::string kOnLineTimeKey = "onlinetime"; // onlinetime
    const std::string kPledgeKey = "pledge"; // pledge
    const std::string kTransactionCountKey = "txcount"; // txcount
    const std::string kGasCountKey = "gascount"; // gascount
    const std::string kAwardCountKey = "awardcount"; // awardcount
    const std::string kGasTotalKey = "gastotal"; // gastotal
    const std::string kAwardTotalKey = "awardtotal"; // awardtotal

    const std::string kInitVersionKey = "initVer"; // initVer
    
public:
    enum STATUS
    {
        ROCKSDB_SUC = 0, //成功
        ROCKSDB_ERR = 1, //错误
        ROCKSDB_PARAM_NULL = 2, //参数不合法
        ROCKSDB_NOT_FOUND = 3, //没找到
        ROCKSDB_IS_EXIST = 4
    };

};

template <typename T>
int Rocksdb::readdata(Transaction* txn, const std::string &blockkey, 
T &blockvalue)
{
    if (blockkey.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }
    auto status = txn->Get(read_options, blockkey, &blockvalue);
    if (status.ok())
    {
        return ROCKSDB_SUC;
    } 
    else 
    {
        return ROCKSDB_ERR;
    }
}

template <typename T>
int Rocksdb::writedata(Transaction* txn, const std::string &blockkey, 
const T &blockvalue)
{
    if (blockkey.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }
    auto status = txn->Put(blockkey, blockvalue);
    if(status.ok()) 
    {
        return ROCKSDB_SUC;
    } 
    else 
    {
        return ROCKSDB_ERR;
    } 
}

#endif 