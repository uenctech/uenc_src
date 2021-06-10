#include "ca_AwardAlgorithm.h"
#include "ca_transaction.h"
using namespace std;

namespace a_award {

void AwardAlgorithm::TestPrint(bool lable) {
    if (lable) {
        auto br = []() {
            cout << "|---------------------------------------------------------------|" << endl;
        };

        cout << "\033[1;40;32m";
        br();
        cout << " Total amount of stall reward pool " << endl;
        cout << " " << this->award_pool << endl;

        br();
        cout << " Signature address : " << endl;
        for (auto v : vec_addr) {
            cout << " "<<  v << endl;
        }

        br();
        cout << " Online days " << endl;
        for (auto v : get<0>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " Total number of signatures " << endl;
        for (auto v : get<1>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " The amount of extra rewards obtained by the total signature " << endl;
        for (auto v : get<2>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " ratio " << endl;
        for (auto v : get<3>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " Reward distribution pool " << endl;

        for (auto v : this->map_addr_award) {
            cout << " Amount  " << v.first << endl;
            cout << " address  " << v.second << endl;
        }
        br();

        cout << "\033[0m" << endl;
    }
}

//class s+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Additional rewards  Award amount (e.g 10 million )/365/24/12/in_minutes_deal  Number of transactions (e.g 0-20 21-40 2 to the Nth power) 
AwardAlgorithm::AwardAlgorithm() : need_verify_count(0), award_pool(0.0), sign_amount(0) {
}

int AwardAlgorithm::Build(uint need_verify_count, 
                        const std::vector<std::string> &vec_addr,
                        const std::vector<double> &vec_online, 
                        const std::vector<uint64_t> &vec_award_total, 
                        const std::vector<uint64_t> &vec_sign_sum) 
{
    
    uint64_t nowTime = Singleton<TimeUtil>::get_instance()->getlocalTimestamp();
    uint64_t maxOnline = nowTime > firstBlockTime ? nowTime - firstBlockTime : 0;
    if (maxOnline == 0) 
    {
        return -1;
    }

    for(auto & onlinetime : vec_online)
    {
        if (onlinetime > maxOnline)
        {
            error("onlinetime error!");
            return -2;
        }
    }

    //Consensus number Initialization of signature wallet address 
    this->need_verify_count = need_verify_count;
    this->vec_addr = vec_addr;
    this->vec_online = vec_online;
    this->vec_award_total = vec_award_total;
    this->vec_sign_sum = vec_sign_sum;

    //Start construction and get the amount of reward stalls for each transaction 
    // double award_base = AwardBaseByYear();

    uint64_t blockNum = 0;
    if (0 != GetBlockNumInUnitTime(blockNum) )
    {
        error("(Build) GetBlockNumInUnitTime failed !");
        return -2;
    }

    // If the transaction is less than unitTimeMinBlockNum within 5 minutes, it will be calculated as unitTimeMinBlockNum 
    // unitTimeMinBlockNum Minimum number of blocks per unit time 
    if (blockNum < unitTimeMinBlockNum)
    {
        blockNum = unitTimeMinBlockNum;
    }

    GetAward(blockNum, award_pool);

    if ( 0 != AwardList() )
    {
        return -3;
    }
    return 0;
}

std::multimap<uint32_t, std::string> &AwardAlgorithm::GetDisAward() {
    return this->map_addr_award;
}

int AwardAlgorithm::GetBaseAward(uint64_t blockNum)
{
    // 70*POWER((1-50%),LOG(blockNum,2))+0.025 Function to calculate the total value of the initial reward 
    return ( slopeCurve * pow( (1 - 0.5), log(blockNum)/log(2) ) + 0.025 ) * DECIMAL_NUM;
}

int AwardAlgorithm::GetAward(const uint64_t blockNum, uint64_t & awardAmount)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        error("(GetAward) TransactionInit failed !");
        return -1;
    }

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    std::string blockHahs;
    if ( 0 != pRocksDb->GetBlockHashByBlockHeight(txn, 0, blockHahs) )
    {
        error("(GetAward) GetBlockHashByBlockHeight failed !");
        return -1;
    }

    // Get the initial block time 
    uint64_t blockTime = 0;
    if ( 0 != GetTimeFromBlockHash(blockHahs, blockTime) )
    {
        error("(GetAward) GetTimeFromBlockHash failed !");
        return -1;
    }

    // Obtain basic block rewards 
    uint64_t baseAmount = GetBaseAward(blockNum);

    /// TODO
    uint64_t award = 0;
    if ( 0 != pRocksDb->GetAwardTotal(txn, award) )
    {
        awardAmount = baseAmount;
        // error("(GetAward) GetAwardTotal failed !");
        // return -3;
    }

    if (award >= awardTotal)
    {
        // If the reward exceeds the total value of the awardTotal reward, it will no longer be rewarded 
        awardAmount= 0;
    }
    else
    {
        uint64_t addend = awardTotal / 2;  // Boundary value increase 
        uint64_t nexthavleAward = addend;  // Halve the boundary value next time 

        while (award > nexthavleAward)
        {
            baseAmount /= 2;   // Reward halved 
            addend /= 2;       // Adjust the increase in boundary value 
            nexthavleAward = nexthavleAward + addend;  // Adjust the next halving boundary value 
        }     

        awardAmount = baseAmount;
    }
    
    return 0;
}

int AwardAlgorithm::GetTimeFromBlockHash(const std::string & blockHash, uint64_t & blockTime)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        error("(GetTimeFromBlockHash) TransactionInit failed !");
        return -1;
    }

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    std::string strBlock;
    if ( 0 != pRocksDb->GetBlockByBlockHash(txn, blockHash, strBlock) )
    {
        error("(GetTimeFromBlockHash) GetBlockByBlockHash failed !");
        return -2;
    }

    CBlock block;
    block.ParseFromString(strBlock);
    blockTime = block.time() / 1000000;

    return 0;
}

int AwardAlgorithm::GetBlockNumInUnitTime(uint64_t & blockSum)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(GetBlockNumInUnitTime) TransactionInit failed !" <<  __LINE__ << std::endl;
        error("(GetBlockNumInUnitTime) TransactionInit failed !");
        return -1;
    }

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    uint32_t top = 0;
    if ( 0 != pRocksDb->GetBlockTop(txn, top) )
    {
        std::cout << "(GetBlockNumInUnitTime) GetBlockTop failed !" <<  __LINE__ << std::endl;
        error("(GetBlockNumInUnitTime) GetBlockTop failed !");
        return -2;
    }

    top = top > 500 ? top - 500 : 0;

    uint64_t blockCount = 0;
    bool bIsBreak = false;  // Determine whether to break out of the loop 

    // Calculate the reward start time 
    std::vector<std::string> blockHashs;
    if ( 0 != pRocksDb->GetBlockHashsByBlockHeight(txn, top, blockHashs) )
    {
        std::cout << "(GetBlockNumInUnitTime) GetBlockHashsByBlockHeight failed !" <<  __LINE__ << std::endl;
        error("(GetBlockNumInUnitTime) GetBlockHashsByBlockHeight failed !");
        return -3;
    }

    uint64_t startTime = 0;
    for (auto & blockHash : blockHashs)
    {
        uint64_t blockTime = 0;
        if ( 0 != GetTimeFromBlockHash(blockHash, blockTime) )
        {
            std::cout << "(GetBlockNumInUnitTime) GetTimeFromBlock failed !" <<  __LINE__ << std::endl;
            return -4;
        }
        
        startTime = startTime > blockTime ? startTime : blockTime;
    }

    if (top == 0)
    {
        blockCount = 1;
    }
    else
    {
        while(top > 0)
        {
            std::vector<std::string> blockHashs;
            if ( 0 != pRocksDb->GetBlockHashsByBlockHeight(txn, top, blockHashs) )
            {
                std::cout << "(GetBlockNumInUnitTime) GetBlockHashsByBlockHeight failed !" <<  __LINE__ << std::endl;
                error("(GetBlockNumInUnitTime) GetBlockHashsByBlockHeight failed !");
                return -3;
            }

            for (auto blockHash: blockHashs)
            {
                uint64_t blockTime = 0;
                if ( 0 != GetTimeFromBlockHash(blockHash, blockTime) )
                {
                    std::cout << "(GetBlockNumInUnitTime) GetTimeFromBlock failed !" <<  __LINE__ << std::endl;
                    error("(GetBlockNumInUnitTime) GetTimeFromBlock failed !");
                    return -4;
                }

                if (blockTime > startTime - unitTime)
                {
                    ++blockCount;
                }
                else
                {
                    bIsBreak = true;
                }
            }

            if (bIsBreak)
            {
                break;
            }

            --top;
        }
    }

    blockSum = blockCount;
    return 0;
}

int AwardAlgorithm::AwardList() 
{
    auto sel_vec_addr = this->vec_addr;

    tuple<vector<double>, vector<uint>, vector<double>, vector<double>> t_list;
    vector<double> &total_online =  get<0>(t_list);
    vector<uint> &total_sign =  get<1>(t_list);
    vector<double> &total_award =  get<2>(t_list);
    vector<double> &rate = get<3>(t_list); //Ratio of all addresses Total reward amount/total number of signatures/duration (days) 

    for (uint32_t i = 1; i < sel_vec_addr.size(); i++) 
    {
        //Total online time  TODO
        double online = (this->vec_online)[i];
        total_online.push_back(online);

        //Get the total bonus amount of the address 
        uint64_t addrTotalAward = 0;
        GetAwardAmountByAddr(sel_vec_addr[i], addrTotalAward);

        total_award.push_back(addrTotalAward);

        //Get the total number of signatures in the address 
        uint64_t signCount = 0;
        GetSignCountByAddr(sel_vec_addr[i], signCount);
        total_sign.push_back(signCount);
    }

    //Calculate all address ratios 
    double sum_rate {0.0}; //Total ratio 
    for (uint32_t i = 0; i < sel_vec_addr.size() - 1; i++) 
    {
        if (!total_sign[i] || !total_award[i]) 
        {
            rate.push_back(0);
            continue;
        }
        double r = total_award[i] / total_online[i] / total_sign[i];
        sum_rate += r;
        rate.push_back(r);
    }

    //For testing 
    this->t_list = t_list;

    multimap<double, string>map_nzero; //Non-zero ratio array 
    uint64_t each_award = this->award_pool / (sel_vec_addr.size() - 1);
    uint64_t temp_award_pool = this->award_pool;

    for (uint32_t i = 1; i < sel_vec_addr.size(); i++) 
    {
        if (!rate[i-1]) 
        {
            this->map_addr_award.insert({each_award, sel_vec_addr[i]});
            temp_award_pool -= each_award;
        } 
        else 
        {
            map_nzero.insert({rate[i-1], sel_vec_addr[i]});
        }
    }

    multimap<uint64_t, string>temp_sort_positive; //First temporarily deposited in positive order 
    vector<string>temp_swap;
    for (auto v : map_nzero) 
    {
        int nr = v.first/sum_rate * temp_award_pool;
        if (nr <= 0 || (uint64_t)nr >= temp_award_pool)
        {
            return -1;
        }
        temp_sort_positive.insert({nr, v.second});
    }

    for (auto it = temp_sort_positive.rbegin(); it != temp_sort_positive.rend(); it++) 
    {
        temp_swap.push_back(it->second);
    }

    uint it_i {0};
    for (auto it = temp_sort_positive.begin(); it != temp_sort_positive.end(); it++) 
    {
        it->second = temp_swap[it_i];
        it_i++;
    }

    for (auto v : temp_sort_positive) {
        this->map_addr_award.insert({v.first, v.second});
    }

    //Add the first 
    this->map_addr_award.insert({0, this->vec_addr[0]});

    return 0;
}

int AwardAlgorithm::GetSignCountByAddr(const std::string addr, uint64_t & count) 
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        std::cout << "(GetSignCountByAddr) TransactionInit failed !" <<  __LINE__ << std::endl;
        error("(GetSignCountByAddr) TransactionInit failed !");
        return -1;
    }

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    if ( 0 != pRocksDb->GetAwardCount(txn, count) ) 
    {
        std::cout << "(GetSignCountByAddr) GetAwardCount failed !" <<  __LINE__ << std::endl;
        error("(GetSignCountByAddr) GetAwardCount failed !");
        return -2;
    }

    return 0;
}


int AwardAlgorithm::GetAwardAmountByAddr(const std::string addr, uint64_t & awardAmount) 
{
    uint64_t awardAmountTmp = 0;

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        error("(GetAwardAmountByAddr) TransactionInit failed !");
        return -1;
    }

    ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, true);
	};

    // Get all transactions by address 
    std::vector<std::string> txHashs;
    if ( 0 != pRocksDb->GetAllTransactionByAddreess(txn, addr, txHashs) )
    {
        error("(GetAwardAmountByAddr) GetAllTransactionByAddreess failed !");
        return -2;
    }

    std::vector<std::string> blockHashs;
    for (auto txHash : txHashs) 
    {
        std::string blockHash;
        pRocksDb->GetBlockHashByTransactionHash(txn, txHash, blockHash);
        blockHashs.push_back(blockHash);
    }

    // Get the sum of all rewarded assets 
    for (auto blockHash : blockHashs) 
    {
        std::string rawBlock;
        if ( 0 != pRocksDb->GetBlockByBlockHash(txn, blockHash, rawBlock) )
        {
            error("(GetAwardAmountByAddr) GetBlockByBlockHash failed !");
            return -3;
        }
        
        CBlock block;
        block.ParseFromString(rawBlock);
        for (auto i = 0; i < block.txs_size(); ++i) 
        {
            CTransaction tx = block.txs(i);
            if (CheckTransactionType(tx) != kTransactionType_Award) 
            {
                continue; //Jump out if it is a normal transaction
            }

            for (int j = 0; j < tx.vout_size(); j++) 
            {
                CTxout txout = tx.vout(j);
                if (addr == txout.scriptpubkey()) 
                {
                    awardAmountTmp += txout.value();
                }
            }
        }
    }

    awardAmount = awardAmountTmp;
    return 0;
}
//class e+++++++++++++++++++++++++++++++++++++++++++++++++++++++

} //namespace end


