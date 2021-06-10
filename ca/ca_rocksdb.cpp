#include <string>
#include <algorithm>

#include "ca_console.h"
#include "ca_rocksdb.h"
#include "ca_header.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/utilities/merge_operators.h"
#include "../utils/string_util.h"


/* ====================================================================================  
 # @description: Initialization, destruction, writing, reading, and deletion of Rocksdb. String cutting function 
 ==================================================================================== */
Rocksdb::Rocksdb()
{
    Options options;
    TransactionDBOptions txn_db_options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.wal_bytes_per_sync = 0;
    char delim = '_';
    options.merge_operator = MergeOperators::CreateStringAppendOperator(delim);
    auto status = TransactionDB::Open(options, txn_db_options, PATH, &db);
    info("RocksDb open error: %s", status.ToString().c_str());
}

Rocksdb::~Rocksdb()
{
    db->Close();
    delete db;
}

Transaction* Rocksdb::TransactionInit()
{
    Transaction* txn = db->BeginTransaction(write_options);
    if(txn == nullptr)
    {
        return nullptr;
    }
    txn->SetSavePoint();
    return txn;
}

int Rocksdb::TransactionCommit(Transaction* txn)
{
    if(txn == nullptr)
    {
        return ROCKSDB_ERR;
    }
    auto status = txn->Commit();
    return status.ok() ? 0 : -1;
}

void Rocksdb::TransactionDelete(Transaction* txn, bool bRollback)
{
    if(txn == nullptr)
    {
        return ;
    }
    if(bRollback)
    {
        txn->RollbackToSavePoint();
    }
    delete txn;
}

std::string Rocksdb::getKey(const std::string &key)
{
    std::string value;
    readdata(key, value);
    return value;
}

int Rocksdb::readdata(const std::string &blockkey, std::string &blockvalue)
{
    if (blockkey.empty()) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    auto status = db->Get(read_options, blockkey, &blockvalue);
    if(status.ok())
    {
        return ROCKSDB_SUC;
    }
    return ROCKSDB_ERR;
}

int Rocksdb::writedata(const std::string &blockkey, const std::string &blockvalue)
{
    if (blockkey.empty() || blockvalue.empty()) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    auto status = db->Put(write_options, blockkey, blockvalue);
    if(status.ok())
    {
        return ROCKSDB_SUC;
    }
    return ROCKSDB_ERR;
}

int Rocksdb::deletedata(Transaction* txn, const std::string &blockkey)
{
    if(txn == nullptr)
    {
        return ROCKSDB_ERR;
    }
    auto status = txn->Delete(blockkey);
    if (status.ok()) 
    {
        return ROCKSDB_SUC;
    } 
    else
    {
        return ROCKSDB_ERR;
    }
}

int Rocksdb::StringSplit(std::vector<std::string> &dst, const std::string &src, const std::string &separator)
{
    if (src.empty() || separator.empty()) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    int count = 0;
    size_t pos = 0, pre_pos = 0, pos_end = 0;
    string sub_str;
    while (pos != std::string::npos)
    {
        pos = src.find(separator, pos);
        if (pos == std::string::npos) 
        {
            pos_end = src.size();
        } 
        else 
        {
            pos_end = pos - pre_pos;
        }
        sub_str = src.substr(pre_pos, pos_end);
        if (!sub_str.empty()) 
        {
            dst.push_back(sub_str);
            ++count;
        }
        if (pos == std::string::npos) break;
        pos += 1;
        pre_pos = pos;
    }
    return count;
}
/* ====================================================================================  
 # @description: Block related interface 
 ==================================================================================== */

//Set block height through block hash 
int Rocksdb::SetBlockHeightByBlockHash(Transaction* txn, const std::string &blockHash, const unsigned int blockHeight) 
{
    if (blockHash.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockHash2BlockHeightKey + "_" + blockHash;
    return writedata(txn, db_key, to_string(blockHeight));
}
//Set block height through block hash 
int Rocksdb::GetBlockHeightByBlockHash(Transaction* txn, const std::string &blockHash, unsigned int &blockHeight) 
{
    if (blockHash.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockHash2BlockHeightKey + "_" + blockHash;
    std::string strHeight {"0"};
    int ret = readdata(txn, db_key, strHeight);
    blockHeight = stoul(strHeight);
    return ret;
}
//Remove block height by block hash 
int Rocksdb::DeleteBlockHeightByBlockHash(Transaction* txn, const std::string & blockHash)
{
    if (blockHash.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kBlockHash2BlockHeightKey + "_" + blockHash;
    return deletedata(txn, db_key);
}
//Set block hash by block height 
int Rocksdb::SetBlockHashByBlockHeight(Transaction* txn, const unsigned int blockHeight, const std::string &blockHash, bool is_mainblock) 
{
    if (blockHash.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::vector<std::string> out;
    GetBlockHashsByBlockHeight(txn, blockHeight, out);
    std::string strHeight = to_string(blockHeight);
    string db_key = this->kBlockHeight2BlockHashKey + "_" + strHeight;
    if(!out.size()) 
    {
        return writedata(txn, db_key, blockHash);
    } 
    else 
    {
        std::string retString;
        if (std::find(out.begin(), out.end(), blockHash) != out.end())
        {
            return ROCKSDB_NOT_FOUND;
        } 
        else 
        {
            if(is_mainblock)
            {
                std::sort(out.begin(), out.end());
                retString = StringUtil::concat(out, "_");
                retString = blockHash + "_" + retString;
            }
            else
            {
                out.push_back(blockHash);
                std::sort(out.begin() + 1, out.end());
                retString = StringUtil::concat(out, "_");
            }
        }
        return writedata(txn, db_key, retString);
    }
}
//Set block hash by block height 
int Rocksdb::GetBlockHashsByBlockHeight(Transaction* txn, const unsigned int blockHeight, std::vector<std::string> &hashes) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string strHeight = to_string(blockHeight);
    string db_key = this->kBlockHeight2BlockHashKey + "_" + strHeight;
    string value;
    int ret = readdata(txn, db_key, value);
    StringSplit(hashes, value, "_");
    return ret;
}

int Rocksdb::GetBlockHashByBlockHeight(Transaction* txn, const int blockHeight, std::string &hash) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::vector<std::string> hashs;
    GetBlockHashsByBlockHeight(txn, blockHeight, hashs);
    if(hashs.size() == 0)
    {
        return 2;
    }
    hash = hashs[0];
    return 0;
}
//Remove the block hash by setting the block height 
int Rocksdb::RemoveBlockHashByBlockHeight(Transaction* txn, const unsigned int blockHeight, const std::string &blockHash)
{ 
    if (blockHash.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::vector<std::string> BlockHashs;
    GetBlockHashsByBlockHeight(txn, blockHeight, BlockHashs);
    for (auto iter = BlockHashs.begin(); iter != BlockHashs.end();) 
    {
        if (blockHash == *iter)
        {
            iter = BlockHashs.erase(iter);
        } 
        else   
        {
            ++iter;
        }
    }
    std::string strHeight = to_string(blockHeight);
    std::string db_key = this->kBlockHeight2BlockHashKey + "_" + strHeight;
    std::string retString;
    for (auto &item : BlockHashs) 
    {
        retString += item;
        retString += "_";
    }
    return writedata(txn, db_key, retString);
}

//Set block according to block hash 
int Rocksdb::SetBlockByBlockHash(Transaction* txn, const std::string &blockHash, const std::string &block) 
{
    if (txn == nullptr || blockHash.empty() || block.empty()) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockHash2BlcokRawKey + "_" + blockHash;
    return writedata(txn, db_key, block);
}
//Get the block according to the block hash 
int Rocksdb::GetBlockByBlockHash(Transaction* txn, const std::string &blockHash, std::string &block) 
{
    if (blockHash.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockHash2BlcokRawKey + "_" + blockHash;
    return readdata(txn, db_key, block);
}
//Remove blocks based on block hash 
int Rocksdb::DeleteBlockByBlockHash(Transaction* txn, const std::string & blockHash)
{
    if (blockHash.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = kBlockHash2BlcokRawKey + "_" + blockHash;
    return deletedata(txn, db_key);
}

//Set block height by address 
int Rocksdb::SetBlockTop(Transaction* txn, const unsigned int blockHeight)
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockTopKey + "_";
    return writedata(txn, db_key, to_string(blockHeight));
}
//Get block height by address 
int Rocksdb::GetBlockTop(Transaction* txn, unsigned int &blockHeight) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockTopKey + "_";
    string height {"0"};
    int ret = readdata(txn, db_key, height);
    blockHeight = stoul(height);
    return ret;
}
//Set the best chain 
int Rocksdb::SetBestChainHash(Transaction* txn, const std::string &blockHash) 
{
    if (blockHash.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kBestChainHashKey + "_";
    return writedata(txn, db_key, blockHash);
}
//Set the best chain 
int Rocksdb::GetBestChainHash(Transaction* txn, std::string &blockHash) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kBestChainHashKey + "_";
    return readdata(txn, db_key, blockHash);
}
//Set block header through block hash 
int Rocksdb::SetBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash, const std::string & header) 
{
    if (blockHash.empty()||header.empty() ||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockHash2BlockHeadRawKey+"_" + blockHash;
    return writedata(txn, db_key, header);
}
//Get block header through block hash 
int Rocksdb::GetBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash, std::string & header)
{
    if (blockHash.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kBlockHash2BlockHeadRawKey+"_" + blockHash;
    return readdata(txn, db_key, header);
}
//Remove block header through block hash 
int Rocksdb::DeleteBlockHeaderByBlockHash(Transaction* txn, const std::string & blockHash)
{
    if (blockHash.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kBlockHash2BlockHeadRawKey+"_" + blockHash;
    return deletedata(txn, db_key);
}

//Set Utxo according to the transaction address 
int Rocksdb::SetUtxoHashsByAddress(Transaction* txn, const std::string &address, const std::string &utxoHash) 
{
    if (address.empty()||utxoHash.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kAddress2UtxoKey + "_" + address;
    std::string valueString;
    readdata(txn, db_key, valueString);
    std::string utxoHash_ = utxoHash + "_";
    if(valueString.size() == 0)
    {
        return writedata(txn, db_key, utxoHash_);
    }
    else
    {
        auto pos = valueString.find(utxoHash);
        if(pos != valueString.npos)
        {
            return ROCKSDB_IS_EXIST;
        }
        else
        {
            std::string retString;
            retString += valueString;
            retString += utxoHash_;
            return writedata(txn, db_key, retString);
        }
    }
}
//Obtain Utxo according to the transaction address 
int Rocksdb::GetUtxoHashsByAddress(Transaction* txn, const std::string &address, std::vector<std::string> &utxoHashs) 
{
    if (address.empty()||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kAddress2UtxoKey + "_" + address;
    std::string valueString;
    int ret = readdata(txn, db_key, valueString);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    StringSplit(utxoHashs, valueString, "_");
    return ret;
}
//Remove Utxo based on transaction address 
int Rocksdb::RemoveUtxoHashsByAddress(Transaction* txn, const std::string &address, const std::string &utxoHash) 
{
    if (address.empty()||utxoHash.empty()||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kAddress2UtxoKey + "_" + address;
    std::string valueString;
    int ret = readdata(txn, db_key, valueString);
    if(ret == ROCKSDB_ERR)
    {
       
        return ROCKSDB_ERR;
    }
    std::string utxoHash_ = utxoHash + "_";
    auto pos = valueString.find(utxoHash_);
    if(pos == valueString.npos)
    {
        return ROCKSDB_NOT_FOUND;
    }

    valueString.erase(pos, utxoHash_.size());

    return writedata(txn, db_key, valueString);
}


//Set transaction raw data according to transaction hash 
int Rocksdb::SetTransactionByHash(Transaction* txn, const std::string &txHash, const std::string &txRaw) 
{
    if (txHash.empty() || txRaw.empty()||txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kTransactionHash2TransactionRawKey + "_" + txHash;
    return writedata(txn, db_key, txRaw);
}
//Obtain the original transaction data according to the transaction hash 
int Rocksdb::GetTransactionByHash(Transaction* txn, const std::string &txHash, std::string &txRaw) 
{
    if (txHash.empty() ||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kTransactionHash2TransactionRawKey + "_" + txHash;
    return readdata(txn, db_key, txRaw);
}
//Remove the original transaction data according to the transaction hash 
int Rocksdb::DeleteTransactionByHash(Transaction* txn, const std::string & txHash)
{
    if (txHash.empty()||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kTransactionHash2TransactionRawKey + "_" + txHash;
    return deletedata(txn, db_key);
}
//Set block hash according to transaction hash 
int Rocksdb::SetBlockHashByTransactionHash(Transaction* txn, const std::string & txHash, const std::string & blockHash) 
{
    if (txHash.empty() || blockHash.empty() ||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kTransactionHash2BlockHashKey + "_" + txHash;
    return writedata(txn, db_key, blockHash);
}
//Obtain block hash according to transaction hash 
int Rocksdb::GetBlockHashByTransactionHash(Transaction* txn, const std::string & txHash, std::string & blockHash) 
{
    if (txHash.empty()||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kTransactionHash2BlockHashKey + "_" + txHash;
    return readdata(txn, db_key, blockHash);
}
//Remove block hash based on transaction hash 
int Rocksdb::DeleteBlockHashByTransactionHash(Transaction* txn, const std::string & txHash)
{
    if (txHash.empty()||txn ==nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kTransactionHash2BlockHashKey + "_" + txHash;
    return deletedata(txn, db_key);
}


/* ====================================================================================  
 # @description: Transaction query related 
 ==================================================================================== */

//Set transaction raw data according to the address 
int Rocksdb::SetTransactionByAddress(Transaction* txn, const std::string &address, const uint32_t txNum, const std::string &txRaw) 
{
    if (address.empty()||txRaw.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2TransactionRawKey + "_"+ address + "_" + to_string(txNum);
    return writedata(txn, db_key, txRaw);
}
//Obtain the original transaction data according to the address 
int Rocksdb::GetTransactionByAddress(Transaction* txn, const std::string &address, const uint32_t txNum, std::string &txRaw) 
{
    if (address.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2TransactionRawKey + "_" + address + "_" + to_string(txNum);
    return readdata(txn, db_key, txRaw);
}
//Remove the original transaction data based on the address 
int Rocksdb::DeleteTransactionByAddress(Transaction* txn, const std::string & address, const uint32_t txNum)
{
    if (address.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2TransactionRawKey + "_" + address + "_" + to_string(txNum);
    return deletedata(txn, db_key);
}
//Set block hash according to transaction address 
int Rocksdb::SetBlockHashByAddress(Transaction* txn, const std::string &address, const uint32_t txNum, const std::string &blockHash) 
{
    if (address.empty()||blockHash.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2BlcokHashKey +"_" + address + "_" + to_string(txNum);
    return writedata(txn, db_key, blockHash);
}
//Obtain block hash according to transaction address 
int Rocksdb::GetBlockHashByAddress(Transaction* txn, const std::string &address, const uint32_t txNum, std::string &blockHash) 
{
    if (address.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2BlcokHashKey +"_" + address + "_" + to_string(txNum);
    return readdata(txn, db_key, blockHash);
}
//Remove block hash based on transaction address 
int Rocksdb::DeleteBlockHashByAddress(Transaction* txn, const std::string & address, const uint32_t txNum)
{
    if (address.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2BlcokHashKey +"_" + address + "_" + to_string(txNum);
    return deletedata(txn, db_key);
}
//Set transaction height according to address 
int Rocksdb::SetTransactionTopByAddress(Transaction* txn, const std::string &address, const unsigned int txIndex) 
{
    if (address.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2TransactionTopKey + "_" + address;
    return writedata(txn, db_key, to_string(txIndex));
}
//Get transaction height based on address 
int Rocksdb::GetTransactionTopByAddress(Transaction* txn, const std::string &address, unsigned int &txIndex)
{
    if(address.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2TransactionTopKey + "_"  + address;
    string txs {"0"};
    int ret = readdata(txn, db_key, txs);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    txIndex = stoul(txs);
    return ret;
}

/* ====================================================================================  
 # @description: Application layer query 
 ==================================================================================== */

//Set the account amount according to the address 
int Rocksdb::SetBalanceByAddress(Transaction* txn, const std::string &address, int64_t balance) 
{
    if(address.empty() || txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2BalanceKey +"_" + address;
    return writedata(txn, db_key, to_string(balance));
}
//Obtain the account amount according to the address 
int Rocksdb::GetBalanceByAddress(Transaction* txn, const std::string &address, int64_t &balance) 
{
    if (address.empty() ||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kAddress2BalanceKey +"_" + address;
    string value {"0"};
    int ret = readdata(txn, db_key, value);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    balance = stoll(value);
    return ret;
}
//Set up all transactions based on address 
int Rocksdb::SetAllTransactionByAddress(Transaction* txn, const std::string &address, const std::string &txHash) 
{
    if (address.empty() || txHash.empty() || txn ==nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAddress2AllTransactionKey + "_" + address;
    
    auto status = txn->Merge(db_key, txHash);

    if(status.ok())
    {
        return ROCKSDB_SUC;
    }

    return ROCKSDB_ERR;
}
//Get all transactions based on address 
int Rocksdb::GetAllTransactionByAddreess(Transaction* txn, const std::string &address, std::vector<std::string> &txHashs)
{
    if(address.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kAddress2AllTransactionKey + "_" + address;
    std::string value;
    int ret = readdata(txn, db_key, value);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    StringSplit(txHashs, value, "_");
    return ret;
}
//Remove all transactions based on address 
int Rocksdb::RemoveAllTransactionByAddress(Transaction* txn, const std::string & address, const std::string & txHash)
{
    if (address.empty()||txHash.empty()|| txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::vector<std::string> vTxHashs;
    int ret = GetAllTransactionByAddreess(txn, address, vTxHashs);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    for (auto iter = vTxHashs.begin(); iter != vTxHashs.end();)
    {
        if (txHash == *iter) 
        {
            iter = vTxHashs.erase(iter);
        } 
        else 
        {
            ++iter;
        }
    }
    std::string db_key = this->kAddress2AllTransactionKey + "_" + address;
    std::string retString;
    for (auto &item : vTxHashs) 
    {
        retString += item;
        retString += "_";
    }  
    return writedata(txn, db_key, retString);
}


//Set packing fee 
int Rocksdb::SetDevicePackageFee(const uint64_t publicNodePackageFee)
{
    string db_key = this->kPackageFeeKey + "_";
    return writedata(db_key, to_string(publicNodePackageFee));
}
//Get packaging fee 
int Rocksdb::GetDevicePackageFee(uint64_t & publicNodePackageFee)
{
    string db_key = this->kPackageFeeKey + "_";
    string publicnodefee {"0"};
    int ret = readdata(db_key, publicnodefee);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    publicNodePackageFee = stoull(publicnodefee);
    return ret;
}

//Set fuel fee 
int Rocksdb::SetDeviceSignatureFee(const uint64_t mineSignatureFee)
{
    string db_key = this->kGasFeeKey + "_";
    return writedata(db_key, to_string(mineSignatureFee));
}
//Get fuel 
int Rocksdb::GetDeviceSignatureFee(uint64_t & mineSignatureFee)
{
    string db_key = this->kGasFeeKey + "_";
    string signaturefee {"0"};
    int ret = readdata(db_key, signaturefee);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    mineSignatureFee = stoul(signaturefee);
    return ret;
}

//Set online duration 
int Rocksdb::SetDeviceOnlineTime(const double minerOnLineTime)
{
    string db_key = this->kOnLineTimeKey + "_";
    return writedata(db_key, to_string(minerOnLineTime));
}
//Get online time 
int Rocksdb::GetDeviceOnLineTime(double & minerOnLineTime)
{
    string db_key = this->kOnLineTimeKey + "_";
    string mineronlinetime {"0"};
    int ret = readdata(db_key, mineronlinetime);
    minerOnLineTime = stod(mineronlinetime);
    return ret;
}

int Rocksdb::SetPledgeAddresses(Transaction* txn,  const std::string &address)
{
    if (address.empty() ||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kPledgeKey + "_";
    std::string valueString;
    readdata(txn, db_key, valueString);
    std::string accountaddress_ = address + "_";
    if(valueString.size() == 0)
    {
        return writedata(txn, db_key, accountaddress_);
    }
    else
    {
        auto position = valueString.find(address);
        if(position != valueString.npos)
        {
            cout << "position is : " << position << endl;
            
            return ROCKSDB_IS_EXIST;
        }
        else
        {
            std::string retString;
            retString += valueString;
            retString += accountaddress_;
            return writedata(txn, db_key, retString);
        }
    }
}

int Rocksdb::GetPledgeAddress(Transaction* txn,  std::vector<string> &addresses)
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kPledgeKey + "_";
    std::string value;
    int ret = readdata(txn, db_key, value);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    StringSplit(addresses, value, "_");
    return ret;
}

int Rocksdb::RemovePledgeAddresses(Transaction* txn,  const std::string &address)
{
    if (address.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::vector<std::string> accountAddress;
    int ret = GetPledgeAddress(txn,accountAddress);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    for (auto iter = accountAddress.begin(); iter != accountAddress.end();) 
    {
        if (address == *iter) 
        {
            iter = accountAddress.erase(iter);
        }
        else 
        {
            ++iter;
        }
    }
    std::string db_key = this->kPledgeKey + "_";
    std::string retString;
    for (auto &item : accountAddress) 
    {
        retString += item;
        retString += "_";
    }
    return writedata(txn, db_key, retString);
}

//Set up utxo for stagnant asset accounts 
int Rocksdb::SetPledgeAddressUtxo(Transaction* txn, const std::string &address, const std::string &utxo)
{
    if(address.empty()||utxo.empty() ||txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kPledgeKey+"_" + address;
    std::string valueString;
    readdata(txn, db_key, valueString);
    std::string utxo_ = utxo + "_";
    if(valueString.size() == 0)
    {
        return writedata(txn, db_key, utxo_);
    }
    else
    {
        auto position = valueString.find(utxo);
        if(position != valueString.npos)
        {
            cout << "position is : " << position << endl;
            return ROCKSDB_IS_EXIST;
        }
        else
        {
            std::string retString;
            retString += valueString;
            retString += utxo_;
            return writedata(txn, db_key, retString);
        }
    }
}

int Rocksdb::GetPledgeAddressUtxo(Transaction* txn, const std::string &address, std::vector<string> &utxos)
{
    if(address.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kPledgeKey+"_" + address;
    std::string value;
    int ret = readdata(txn, db_key, value);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    StringSplit(utxos, value, "_");
    return ret;
}

int Rocksdb::RemovePledgeAddressUtxo(Transaction* txn, const std::string &address,const std::string &utxo)
{
    if (address.empty()||utxo.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::vector<std::string> utxosvector;
    int ret = GetPledgeAddressUtxo(txn,address,utxosvector);
    if(ret == ROCKSDB_ERR)
    {
        return ROCKSDB_ERR;
    }
    for (auto iter = utxosvector.begin(); iter != utxosvector.end();) 
    {
        if (utxo == *iter) 
        {
            iter = utxosvector.erase(iter);
        }
        else 
        {
            ++iter;
        }
    }
    std::string db_key = this->kPledgeKey+"_" + address;
    std::string retString;

    for (auto &item : utxosvector) 
    {
        retString += item;
        retString += "_";
    }
    return writedata(txn, db_key, retString);
}



//Set the total number of transactions 
int Rocksdb::SetTxCount(Transaction* txn, uint64_t &count) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kTransactionCountKey + "_";
    return writedata(db_key, to_string(count));
}
//Get the total number of transactions 
int Rocksdb::GetTxCount(Transaction* txn, uint64_t &counts) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kTransactionCountKey + "_";
    string count {"0"};
    int ret = readdata(db_key, count);
    counts = stoul(count);
    return ret;
}

// Set total fuel cost 
int Rocksdb::SetGasCount(Transaction* txn, uint64_t &count) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kGasCountKey + "_";
    return writedata(db_key, to_string(count));
}
int Rocksdb::GetGasCount(Transaction* txn, uint64_t &counts) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kGasCountKey + "_";
    string count {"0"};
    int ret = readdata(db_key, count);
    counts = stoul(count);
    return ret;
}

// Set the total additional reward fee 
int Rocksdb::SetAwardCount(Transaction* txn, uint64_t &count) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAwardCountKey +"_";
    return writedata(db_key, to_string(count));
}
// Get the total extra bonus fee 
int Rocksdb::GetAwardCount(Transaction* txn, uint64_t &counts) 
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    string db_key = this->kAwardCountKey +"_";
    string count {"0"};
    int ret = readdata(db_key, count);
    counts = stoul(count);
    return ret;
}



int Rocksdb::GetGasTotal(Transaction* txn, uint64_t & gasTotal)
{
    if (txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }

    string db_key = this->kGasTotalKey +"_";
    string strGasTotal;
    int ret = readdata(db_key, strGasTotal);
    if (ret != ROCKSDB_SUC)
    {
        return ret;
    }

    gasTotal = strtoull(strGasTotal.c_str(), NULL, 0);
    
    return ROCKSDB_SUC;
}

int Rocksdb::SetGasTotal(Transaction* txn, const uint64_t & gasTotal)
{
    if (txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    
    string db_key = this->kGasTotalKey +"_";
    return writedata(db_key, to_string(gasTotal));
}
int Rocksdb::GetAwardTotal(Transaction* txn, uint64_t &awardTotal)
{
    if (txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }

    string db_key = this->kAwardTotalKey +"_";
    string strAwardTotal;
    int ret = readdata(db_key, strAwardTotal);
    if (ret != ROCKSDB_SUC)
    {
        return ret;
    }

    awardTotal = strtoull(strAwardTotal.c_str(), NULL, 0);
    
    return ROCKSDB_SUC;
}
int Rocksdb::SetAwardTotal(Transaction* txn, const uint64_t &awardTotal)
{
    if (txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    
    string db_key = this->kAwardTotalKey +"_";
    return writedata(db_key, to_string(awardTotal));
}


int Rocksdb::SetInitVer(Transaction* txn, const std::string & version)
{
    if (version.empty() || txn == nullptr) 
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kInitVersionKey + "_";
    return writedata(txn, db_key, version);
}
    
int Rocksdb::GetInitVer(Transaction* txn, std::string & version)
{
    if(txn == nullptr)
    {
        return ROCKSDB_PARAM_NULL;
    }
    std::string db_key = this->kInitVersionKey + "_";
    return readdata(txn, db_key, version);
}


int Rocksdb::GetGasTotalByAddress(Transaction* txn, const std::string & addr, uint64_t & gasTotal)
{
    if (txn == NULL || addr.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }

    std::string db_key = this->kAddrToGasTotalKey + "_" + addr;
    std::string gasTotalStr;

    int ret =  readdata(txn, db_key, gasTotalStr);
    if (ret != ROCKSDB_SUC)
    {
        return ret;
    }
    else
    {
        gasTotal = std::stoll(gasTotalStr);
    }
    return ROCKSDB_SUC; 
}


int Rocksdb::SetGasTotalByAddress(Transaction* txn, const std::string & addr, const uint64_t & gasTotal)
{
    if (txn == NULL || addr.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }

    std::string db_key = this->kAddrToGasTotalKey + "_" + addr;
    return writedata(txn, db_key, std::to_string(gasTotal));
}


int Rocksdb::GetAwardTotalByAddress(Transaction* txn, const std::string & addr, uint64_t &awardTotal)
{
    if (txn == NULL || addr.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }

    std::string db_key = this->kAddrToAwardTotalKey + "_" + addr;
    
    std::string awardTotalStr;
    int ret = readdata(txn, db_key, awardTotalStr);
    if (ret != ROCKSDB_SUC)
    {
        return ret;
    }
    else
    {
        awardTotal = std::stoull(awardTotalStr);
    }
    return ROCKSDB_SUC;
}


int Rocksdb::SetAwardTotalByAddress(Transaction* txn, const std::string & addr, const uint64_t &awardTotal)
{
    if (txn == NULL || addr.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }

    std::string db_key = this->kAddrToAwardTotalKey + "_" + addr;
    return writedata(txn, db_key, std::to_string(awardTotal));
}


int Rocksdb::GetSignNumByAddress(Transaction* txn, const std::string & addr, uint64_t &signNum)
{
    if (txn == NULL || addr.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }

    std::string db_key = this->kAddrToSignNumKey + "_" + addr;
    std::string signNumStr;

    int ret = readdata(txn, db_key, signNumStr);
    if (ret != ROCKSDB_SUC)
    {
        return ret;
    }
    else
    {
        signNum = std::stoll(signNumStr);
    }
    return ROCKSDB_SUC;
}


int Rocksdb::SetSignNumByAddress(Transaction* txn, const std::string & addr, const uint64_t &signNum)
{
    if (txn == NULL || addr.empty())
    {
        return ROCKSDB_PARAM_NULL;
    }

    std::string db_key = this->kAddrToSignNumKey + "_" + addr;
    return writedata(txn, db_key, std::to_string(signNum));
    return 0;
}