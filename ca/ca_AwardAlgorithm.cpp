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
        cout << " 档位奖励池总金额" << endl;
        cout << " " << this->award_pool << endl;

        br();
        cout << " 签名地址: " << endl;
        for (auto v : vec_addr) {
            cout << " "<<  v << endl;
        }

        br();
        cout << " 在线天数" << endl;
        for (auto v : get<0>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " 总签名数" << endl;
        for (auto v : get<1>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " 总签名获取的额外奖励金额" << endl;
        for (auto v : get<2>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " 比率" << endl;
        for (auto v : get<3>(this->t_list)) {
            cout << " " << v << endl;
        }

        br();
        cout << " 奖励分配池" << endl;

        for (auto v : this->map_addr_award) {
            cout << " 金额 " << v.first << endl;
            cout << " 地址 " << v.second << endl;
        }
        br();

        cout << "\033[0m" << endl;
    }
}

//class s+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//额外奖励 奖励额(e.g 1000万)/365/24/12/in_minutes_deal 交易笔数档位(e.g 0-20 21-40 2的N次方)
AwardAlgorithm::AwardAlgorithm() : need_verify_count(0), award_pool(0.0), sign_amount(0) {
    this->now_time = Singleton<TimeUtil>::get_instance()->getNtpTimestamp();
    this->now_time = now_time == 0 ? time(0) : now_time / 1000000;
}

int AwardAlgorithm::Build(uint need_verify_count, const std::vector<std::string> &vec_addr,
const std::vector<double> &vec_onlinet) {
    //共识数 签名钱包地址初始化
    this->need_verify_count = need_verify_count;
    this->vec_addr = vec_addr;
    this->vec_onlinet = vec_onlinet;

    //开始建造 得到每笔交易奖励档位金额
    // double award_base = AwardBaseByYear();

    uint64_t blockNum = 0;
    if (0 != GetBlockNumInUnitTime(blockNum) )
    {
        error("(Build) GetBlockNumInUnitTime failed !");
        return -1;
    }

    // 5分钟内若交易少于unitTimeMinBlockNum，则按unitTimeMinBlockNum计算
    // unitTimeMinBlockNum 单位时间内最少区块数
    if (blockNum < unitTimeMinBlockNum)
    {
        blockNum = unitTimeMinBlockNum;
    }

    GetAward(blockNum, award_pool);

    AwardList();
    return 0;
}

std::multimap<uint32_t, std::string> &AwardAlgorithm::GetDisAward() {
    return this->map_addr_award;
}

int AwardAlgorithm::GetBaseAward(uint64_t blockNum)
{
    // 70*POWER((1-50%),LOG(blockNum,2))+0.025 计算初始奖励总值的函数
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

    // 获取初始块时间
    uint64_t blockTime = 0;
    if ( 0 != GetTimeFromBlockHash(blockHahs, blockTime) )
    {
        error("(GetAward) GetTimeFromBlockHash failed !");
        return -1;
    }

    // 获得基础区块奖励
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
        // 奖励超过awardTotal奖励总值，则不再奖励
        awardAmount= 0;
    }
    else
    {
        uint64_t addend = awardTotal / 2;  // 边界值增长幅度
        uint64_t nexthavleAward = addend;  // 下一次减半边界值

        while (award > nexthavleAward)
        {
            baseAmount /= 2;   // 奖励减半
            addend /= 2;       // 调整边界值增长幅度
            nexthavleAward = nexthavleAward + addend;  // 调整下次减半边界值
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

    uint64_t blockCount = 0;

    bool bIsBreak = false;  // 判断是否跳出循环

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
                    return -3;
                }

                if (blockTime > this->now_time - unitTime)
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
    vector<double> &rate = get<3>(t_list); //所有地址的比率 总奖励额/签名总数/时长(天)

    for (uint32_t i = 1; i < sel_vec_addr.size(); i++) 
    {
        //总在线时长 TODO
        double online = (this->vec_onlinet)[i];
        total_online.push_back(online);

        //获取地址总额外奖励金额
        uint64_t addrTotalAward = 0;
        GetAwardAmountByAddr(sel_vec_addr[i], addrTotalAward);

        total_award.push_back(addrTotalAward);

        //获取地址总签名数
        uint64_t signCount = 0;
        GetSignCountByAddr(sel_vec_addr[i], signCount);
        total_sign.push_back(signCount);
    }

    //计算出所有地址比率
    double sum_rate {0.0}; //总比率
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

    //测试用
    this->t_list = t_list;

    multimap<double, string>map_nzero; //非0比率数组
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

    multimap<uint64_t, string>temp_sort_positive; //先临时存入正序排列
    vector<string>temp_swap;
    for (auto v : map_nzero) 
    {
        uint64_t nr = v.first/sum_rate * temp_award_pool;
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

    //添加第一个
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

    // 通过地址获取所有交易
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

    // 获取所有奖励的资产总和
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
                continue; //如果为正常交易则跳出
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


