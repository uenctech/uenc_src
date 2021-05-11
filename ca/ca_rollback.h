#ifndef __CA_ROLLBACK_H__
#define __CA_ROLLBACK_H__

#include <iostream>
#include <memory>
#include <mutex>

#include "ca_rocksdb.h"
#include "ca_blockpool.h"
#include "transaction.pb.h"

class Rollback 
{
public:
    Rollback() = default;
    ~Rollback() = default;

    int RollbackBlockByBlockHash(Transaction* txn, std::shared_ptr<Rocksdb> & pRocksDb, const std::string & blockHash);
    int RollbackToHeight(const unsigned int & height);
    int RollbackBlockBySyncBlock(const uint32_t & conflictHeight, const std::vector<Block> & syncBlocks);

private:
    int RollbackTx(std::shared_ptr<Rocksdb> pRocksDb, Transaction* txn, CTransaction tx);
    int RollbackPledgeTx(std::shared_ptr<Rocksdb> pRocksDb, Transaction* txn, CTransaction &tx);
    int RollbackRedeemTx(std::shared_ptr<Rocksdb> pRocksDb, Transaction* txn, CTransaction &tx);

public:
    bool isRollbacking = false;

private:
    std::mutex mutex_;
};

#endif // !__CA_ROLLBACK_H__