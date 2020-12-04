#ifndef _BLOCKPOOL_H_
#define _BLOCKPOOL_H_

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
#include "./proto/block.pb.h"

const int PROCESS_TIME = 10;
const int SYNC_ADD_FAIL_TIME = 5;
const int SYNC_ADD_FAIL_LIMIT = 10;
const int ROLLBACK_HEIGHT = 10;
const int PENDING_EXPIRE_TIME = 120;

class Block
{
public:
    Block(const CBlock& block, bool isSync = false):blockheader_(block),time_(time(NULL)),isSync_(isSync){

    };
    ~Block() = default;

public:
    CBlock blockheader_;
    int time_;
    bool isSync_;
};


class BlockPoll
{
public:
	BlockPoll() = default;
	~BlockPoll() = default;


    bool Add(const Block& block);
    void Process();
    bool CheckConflict(const CBlock& block1, const CBlock& block2);
    bool CheckConflict(const CBlock& block);
    std::string GetBlock();
    bool VerifyHeight(const CBlock& block);
    std::vector<Block> & GetSyncBlocks()
    {
        return sync_blocks_;
    }

private:
    int sync_add_fail_times_ = 0;
	std::mutex mu_block_;
	std::mutex mu_pending_block_;

	std::vector<Block> blocks_;
	std::vector<Block> sync_blocks_;
	std::vector<Block> pending_block_;
    bool processing_ = false;

};


#endif







