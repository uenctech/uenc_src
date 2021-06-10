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
 # @description: Block related interface 
==================================================================================== */


/* ====================================================================================  
 # @description: Set the height of the data block through the block hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash string 
 # @param blockHeight : Set the height of the data block 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // // block hash -> block num
    int SetBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash, const unsigned int blockHeight);
 
/* ====================================================================================  
 # @description: Get the height of the data block through the block hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash string  
 # @param blockHeight : The height of the obtained data block (only one height) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */   
    int GetBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash, unsigned int & blockHeight);
 
/* ====================================================================================  
 # @description: Remove the block height in the database by block hashing 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash string  
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */     
    int DeleteBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: Set the hash of the data block by the block height (in concurrency, there may be multiple block hashes at the same height) 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHeight : The height of the data block 
 # @param blockHash  :  Block hash string 
 # @param is_mainblock : Is it a block on the main chain (there may be multiple blocks at the same height when concurrently) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // // block num -> block hash (mutli)
    int SetBlockHashByBlockHeight(Transaction* txn, const unsigned int blockHeight, const std::string & blockHash, bool is_mainblock = false);

/* ====================================================================================  
 # @description: Get the hash of a single block by the height of the data block 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHeight  : The height of the data block 
 # @param hash : Get the hash of the block by the block height (single hash) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetBlockHashByBlockHeight(Transaction* txn, const int blockHeight, std::string &hash);

/* ====================================================================================  
 # @description: Obtain multiple block hashes based on the height of the data block 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHeight : Set the height of the data block  
 # @param hashes  : Block hash string (multiple hashes are put into a std::vector<std::string>) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetBlockHashsByBlockHeight(Transaction* txn, const unsigned int blockHeight, std::vector<std::string> &hashes);

/* ====================================================================================  
 # @description: Remove the hash of the block in the database by the block height 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHeight : The height of the data block 
 # @param blockHash  : Block hash string 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int RemoveBlockHashByBlockHeight(Transaction* txn, const unsigned int blockHeight, const std::string & blockHash);

/* ====================================================================================  
 # @description:Set block via block hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash string 
 # @param block : Block data that needs to be set (binary is a data block that can be directly put into protobuf for transmission) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // block hash -> block 
    int SetBlockByBlockHash(Transaction* txn, const std::string & blockHash, const std::string & block);

/* ====================================================================================  
 # @description: Obtain data block information through block hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash string 
 # @param block : Obtained block data (binary data block can be directly put into protobuf for transmission) 
 # @return :If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */ 
    int GetBlockByBlockHash(Transaction* txn, const std::string & blockHash, std::string & block);

/* ====================================================================================  
 # @description:Remove the block in the data block through block hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash string 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */  
    int DeleteBlockByBlockHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: Set the highest block 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHeight  : The height of the data block that needs to be set 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */ 
    // block Top -> top
    int SetBlockTop(Transaction* txn, const unsigned int blockHeight);

/* ====================================================================================  
 # @description: Get the highest block 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHeight  : The height of the acquired data block 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */ 
    int GetBlockTop(Transaction* txn, unsigned int & blockHeight);

/* ====================================================================================  
 # @description: Set the best chain 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : The block hash of the best chain that needs to be set 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */ 
    // best chain
    int SetBestChainHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: Get the best chain 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash in the best chain obtained 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum)
 ==================================================================================== */
    int GetBestChainHash(Transaction* txn, std::string & blockHash);

/* ====================================================================================  
 # @description: Set block header information through the hash of the block 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash in the best chain obtained 
 # @param header  : The block header information that needs to be set (binary data can be directly transmitted in protobuf) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */ 
    //====
    int SetBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash, const std::string & header);

/* ====================================================================================  
 # @description: Get the block header information through the hash of the block 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash in the best chain obtained 
 # @param header  : Obtained block header information (binary data can be directly transmitted in protobuf) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash, std::string & header);
 
/* ====================================================================================  
 # @description: Remove the block header information in the database according to the block hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param blockHash  : Block hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int DeleteBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash);

/* ====================================================================================  
 # @description: Set Utxo hash by address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param utxoHash  : Need to set utxohash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // UTXO
    int SetUtxoHashsByAddress(Transaction* txn, const std::string & address, const std::string & utxoHash);

/* ====================================================================================  
 # @description: Get Utxo hash by address (there are multiple utxohash) 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param utxoHashs  : Obtained utxohash (there are multiple utxo put into std::vector<std::string>) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetUtxoHashsByAddress(Transaction* txn, const std::string & address, std::vector<std::string> & utxoHashs);

/* ====================================================================================  
 # @description: Remove Utxo hash by address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param utxoHash  : Utxohash that needs to be removed 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */   
    int RemoveUtxoHashsByAddress(Transaction* txn, const std::string & address, const std::string & utxoHash);

/* ====================================================================================  
 # @description: Set transaction raw data through transaction hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param txHash  : Transaction hash 
 # @param txRaw  : Raw transaction data required (transaction data is binary data and can be directly put into protobuf for data transmission) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // txhash -> txRaw
    int SetTransactionByHash(Transaction* txn, const std::string & txHash, const std::string & txRaw);

/* ====================================================================================  
 # @description: Obtain transaction raw data through Transaction hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param txHash  : Transaction hash 
 # @param txRaw  : The obtained raw transaction data (transaction data is binary data and can be directly put into protobuf for data transmission) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetTransactionByHash(Transaction* txn, const std::string & txHash, std::string & txRaw);

/* ====================================================================================  
 # @description:Use Transaction hash to remove the original transaction data in the database 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param txHash  : Transaction hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */   
    int DeleteTransactionByHash(Transaction* txn, const std::string & txHash);

/* ====================================================================================  
 # @description: Set block hash through Transaction hash 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param txHash  : Transaction hash 
 # @param blockHash  : Block hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // txHash -> blockHash
    int SetBlockHashByTransactionHash(Transaction* txn, const std::string & txHash, const std::string & blockHash);

/* ====================================================================================  
 # @description: Obtain Block hash through Transaction hash  
 # @param txn  : Transaction pointer in Rocksdb 
 # @param txHash  : Transaction hash 
 # @param blockHash  : Block hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetBlockHashByTransactionHash(Transaction* txn, const std::string & txHash, std::string & blockHash);

/* ====================================================================================  
 # @description: Remove Block hash in the database through Transaction hash  
 # @param txn  : Transaction pointer in Rocksdb 
 # @param txHash  : Transaction hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int DeleteBlockHashByTransactionHash(Transaction* txn, const std::string & txHash);

//////////////////// TODO: May be deprecated start 
/* ====================================================================================  
 # @description: Transaction query related 
==================================================================================== */

/* ====================================================================================  
 # @description: Set block transaction via Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txNum  : Transaction serial number 
 # @param txRaw  : Transaction raw data (raw data can be directly put into protobuf) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
  // addr + tx num -> tx raw
    int SetTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, const std::string & txRaw);

/* ====================================================================================  
 # @description: Transaction raw data (raw data can be directly put into protobuf) 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txNum  : Transaction serial number 
 # @param txRaw  : Transaction raw data (raw data can be directly put into protobuf) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, std::string & txRaw);

/* ====================================================================================  
 # @description:Remove transaction data in the database through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txNum  : Transaction serial number 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int DeleteTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum);

/* ====================================================================================  
 # @description: Set Block hash through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txNum  : Transaction serial number 
 # @param blockHash  : Block hash value 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // // addr + tx num -> block hash
    int SetBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, const std::string & blockHash);

/* ====================================================================================  
 # @description: Obtain Block hash through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txNum  : Transaction serial number 
 # @param blockHash  : Block hash value 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum, std::string & blockHash);

/* ====================================================================================  
 # @description: Remove the Block hash in the database through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txNum  : Transaction serial number 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
   int DeleteBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum);

/* ====================================================================================  
 # @description: Get the highest transaction height through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txIndex  : Transaction serial number 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */   
    // addr Top -> addr tx top
    int SetTransactionTopByAddress(Transaction* txn, const std::string & address, const unsigned int txIndex);

/* ====================================================================================  
 # @description: Get the highest transaction height through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txIndex  : Transaction serial number 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetTransactionTopByAddress(Transaction* txn, const std::string & address, unsigned int & txIndex);

//////////////////// TODO: May be discarded end 
    
/* ====================================================================================  
 # @description: Application layer query 
==================================================================================== */

/* ====================================================================================  
 # @description: Set account balance through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param balance  : Account balance 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // addr -> balance
    int SetBalanceByAddress(Transaction* txn, const std::string & address, int64_t balance);

/* ====================================================================================  
 # @description: Obtain Account balance through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param balance  : Account balance 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetBalanceByAddress(Transaction* txn, const std::string & address, int64_t & balance);

/* ====================================================================================  
 # @description: Set up all transactions through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txHash  : Transaction hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */  
    // addr -> transcations
    int SetAllTransactionByAddress(Transaction* txn, const std::string & address, const std::string & txHash);

/* ====================================================================================  
 # @description: Get all transactions through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txHash  : Transaction hash (Multiple Transaction hashes are put into std::vector<std::string>) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetAllTransactionByAddreess(Transaction* txn, const std::string & address, std::vector<std::string> & txHashs);

/* ====================================================================================  
 # @description: Move all transactions in the database through Transaction address 
 # @param txn  : Transaction pointer in Rocksdb 
 # @param address  : Transaction address 
 # @param txHash  : Transaction hash 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int RemoveAllTransactionByAddress(Transaction* txn, const std::string & address, const std::string & txHash);

/* ====================================================================================  
 # @description:Set up equipment packaging fee 
 # @param publicNodePackageFee  : Equipment packing fee 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    //  publicnodepackagefee
    int SetDevicePackageFee(const uint64_t publicNodePackageFee);

/* ====================================================================================  
 # @description: Obtain Equipment packing fee 
 # @param publicNodePackageFee  : Equipment packing fee 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetDevicePackageFee(uint64_t & publicNodePackageFee);

/* ====================================================================================  
 # @description: Set device signature fee 
 # @param mineSignatureFee  : Equipment signature fee 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    //  minesignaturefee
    int SetDeviceSignatureFee(const uint64_t mineSignatureFee);

/* ====================================================================================  
 # @description: Obtain the Equipment signature fee  
 # @param mineSignatureFee  : Equipment signature fee 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetDeviceSignatureFee(uint64_t & mineSignatureFee);

/* ====================================================================================  
 # @description: Set device online duration 
 # @param minerOnLineTime  : Device online time 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
     // Mining machine online time 
    int SetDeviceOnlineTime(const double minerOnLineTime);

/* ====================================================================================  
 # @description: Get device online time 
 # @param minerOnLineTime  : Device online time 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetDeviceOnLineTime(double & minerOnLineTime);


/* ====================================================================================  
 # @description: Set up a pledge address 
 # @param txn  : Transactions in Rocksdb 
 # @param address  : Transaction address 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    //Set up, get, and remove the account address of the stagnant assets 
    int SetPledgeAddresses(Transaction* txn,  const std::string &address); 

/* ====================================================================================  
 # @description: Get the pledge address 
 # @param txn  : Transactions in Rocksdb 
 # @param address  : Transaction address (There are multiple addresses stored in std::vector<string>) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetPledgeAddress(Transaction* txn,  std::vector<string> &addresses);

/* ====================================================================================  
 # @description: (There are multiple addresses stored in std::vector<string>) 
 # @param txn  : Transactions in Rocksdb 
 # @param address  : Transaction address 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int RemovePledgeAddresses(Transaction* txn,  const std::string &address);

/* ====================================================================================  
 # @description: Utxo to set the staking address 
 # @param txn  : Transactions in Rocksdb 
 # @param address  : Transaction address 
 # @param utxo  : utxo
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */   
    //Set up, get, and remove the utxo of the stagnant asset account 
    int SetPledgeAddressUtxo(Transaction* txn, const std::string &address, const std::string &utxo); 

/* ====================================================================================  
 # @description: Utxo to get the pledge address 
 # @param txn  : Transactions in Rocksdb 
 # @param address  : Transaction address 
 # @param utxo  : utxo(Utxo has multiple storage with std::vector<string>) 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetPledgeAddressUtxo(Transaction* txn, const std::string &address, std::vector<string> &utxos);

/* ====================================================================================  
 # @description:   Remove Utxo from the data 
 # @param txn  : Transactions in Rocksdb 
 # @param address  : Transaction address 
 # @param utxo  : utxo
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
   int RemovePledgeAddressUtxo(Transaction* txn, const std::string &address,const std::string &utxos);

/* ====================================================================================  
 # @description: Set the total number of transactions 
 # @param txn  : Transactions in Rocksdb 
 # @param count  : Number of transactions 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    /* Transaction statistics */
    // Get and set the total number of transactions 
    int SetTxCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: Get the total number of transactions 
 # @param txn  : Transactions in Rocksdb 
 # @param count  : Number of transactions 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetTxCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: Set total fuel cost 
 # @param txn  : Transactions in Rocksdb 
 # @param count  : Total fuel 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // Get and set the total fuel cost 
    int SetGasCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: Get the total fuel cost 
 # @param txn  : Transactions in Rocksdb 
 # @param count  : Total fuel 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetGasCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: Set the total bonus 
 # @param txn  : Transactions in Rocksdb 
 # @param count  : Total bonus 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    // Get and set the total additional reward fee 
    int SetAwardCount(Transaction* txn, uint64_t &count);

/* ====================================================================================  
 # @description: Get the total number of signatures 
 # @param txn  : Transactions in Rocksdb 
 # @param count  : Total bonus 
 # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
 ==================================================================================== */
    int GetAwardCount(Transaction* txn, uint64_t &count);

    


    // Added an interface for obtaining the total value of mining fees and rewards, to be improved 
    int GetGasTotal(Transaction* txn, uint64_t & gasTotal);   // Not called 
    int SetGasTotal(Transaction* txn, const uint64_t & gasTotal);
    int GetAwardTotal(Transaction* txn, uint64_t &awardTotal);
    int SetAwardTotal(Transaction* txn, const uint64_t &awardTotal);
    
    /* ==================================================================================== 
     # @description: Get the total handling fee obtained through base58 address 
     # @param txn  : Create a pointer generated by the transaction 
     # @param addr : base58 address 
     # @param gasTotal : Participation, total handling fee 
     # @return Return 0 on success, and return a number greater than zero on error. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
     ==================================================================================== */
    int GetGasTotalByAddress(Transaction* txn, const std::string & addr, uint64_t & gasTotal);

    /* ==================================================================================== 
     # @description: Set the total commission received through base58 address 
     # @param txn  : Create a pointer generated by the transaction 
     # @param addr : base58 address 
     # @param gasTotal : Total fee 
     # @return Return 0 on success, and return a number greater than zero on error. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
     ==================================================================================== */
    int SetGasTotalByAddress(Transaction* txn, const std::string & addr, const uint64_t & gasTotal);

    /* ==================================================================================== 
     # @description: Get the total rewards obtained through base58 address 

     # @param txn  : Create a pointer generated by the transaction 
     # @param addr : base58 address 
     # @param awardTotal : Participation, total reward 
     # @return Return 0 on success, and return a number greater than zero on error. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
     ==================================================================================== */
    int GetAwardTotalByAddress(Transaction* txn, const std::string & addr, uint64_t &awardTotal);

    /* ==================================================================================== 
     # @description: Set the total rewards obtained through base58 address 
     # @param txn  : Create a pointer generated by the transaction 
     # @param addr : base58 address 
     # @param awardTotal : Total reward 
     # @return Return 0 on success, and return a number greater than zero on error. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
     ==================================================================================== */
    int SetAwardTotalByAddress(Transaction* txn, const std::string & addr, const uint64_t &awardTotal);

    /* ==================================================================================== 
     # @description: Get the total number of signatures obtained through base58 address 
     # @param txn  : Create a pointer generated by the transaction 
     # @param addr : base58 address 
     # @param awardTotal : Participation, the total number of signatures 
     # @return Return 0 on success, and return a number greater than zero on error. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
     ==================================================================================== */
    int GetSignNumByAddress(Transaction* txn, const std::string & addr, uint64_t &SignNum);

    /* ==================================================================================== 
     # @description: Set the total number of signatures obtained through base58 address 
     # @param txn  : Create a pointer generated by the transaction 
     # @param addr : base58 address 
     # @param awardTotal : Total number of signatures 
     # @returnReturn 0 on success, and return a number greater than zero on error. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
     ==================================================================================== */
    int SetSignNumByAddress(Transaction* txn, const std::string & addr, const uint64_t &SignNum);

    /* ====================================================================================  
    # @description: Set the database Program Version 
    # @param txn  : Transactions in Rocksdb 
    # @param version  : Program Version 
    # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
    ==================================================================================== */
    // Record the Program Version of the initialization database 
    int SetInitVer(Transaction* txn, const std::string & version);

    /* ====================================================================================  
    # @description: Set the database Program Version 
    # @param txn  : Transactions in Rocksdb 
    # @param version  : Program Version 
    # @return : If the setting succeeds, it returns 0. If it fails, it returns a number greater than zero. ROCKSDB_ERR = 1, //Error ROCKSDB_PARAM_NULL = 2, //The parameter is illegal (you can view it in the enum) 
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

/** Prefix  
 * hs = hash
 * ht = height
 * hd = header
 * bal = balance
 * bh = block hash
 * blk = block
*/

/* ====================================================================================  
 # @description: Block related interface 
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
 # @description: Transaction query related 
==================================================================================== */
    const std::string kAddress2TransactionRawKey = "addr2txraw"; // addr2txraw 
    const std::string kAddress2BlcokHashKey = "addr2blkhs"; // addr2blkhs
    const std::string kAddress2TransactionTopKey = "addr2txtop"; // addr2txtop
/* ====================================================================================  
 # @description:  Application layer query 
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
    const std::string kAddrToGasTotalKey = "addr2gastotal"; // addr2gastotal
    const std::string kAddrToAwardTotalKey = "addr2awardtotal"; // addr2awardtotal
    const std::string kAddrToSignNumKey = "addr2signnum"; // addr2signnum

    const std::string kInitVersionKey = "initVer"; // initVer
    
public:
    enum STATUS
    {
        ROCKSDB_SUC = 0, //success 
        ROCKSDB_ERR = 1, //error 
        ROCKSDB_PARAM_NULL = 2, //Invalid parameter 
        ROCKSDB_NOT_FOUND = 3, //did not find 
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