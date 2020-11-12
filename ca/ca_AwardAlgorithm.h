#include "../utils/string_util.h"
#include "../utils/util.h"
#include "../utils/time_util.h"
#include "ca_device.h"
#include "ca_header.h"
#include "ca_clientinfo.h"
#include "ca_global.h"

#include <complex>
#include <iomanip>
#include <tuple>

/**
 * @brief 额外奖励算法
*/

namespace a_award {

class AwardAlgorithm {
public:
    /**
     * @brief 初始化私有变量
     */
    AwardAlgorithm();

    /**
     * @brief 组合工具函数 得出额外奖励TODO
     * @param need_verify 共识数
     * @param vec_hash 钱包地址数组
     */
    int Build(uint need_verify_count, const std::vector<std::string> &vec_hash, const std::vector<double> &vec_onlinet);

    /**
     * @brief 计算每个钱包地址分配的额外奖励
     */
    int AwardList();

    /**
     * @brief 查询总签名次数
     * @param addr 签名钱包地址
     * @param addr 签名次数
     * @return 返回错误码
     */
    int GetSignCountByAddr(const std::string addr, uint64_t & count);

    /**
     * @brief 查询签名额外奖励总数
     * @param addr 签名钱包地址
     * @return 返回签名获取的总额外奖励金额
     */
    int GetAwardAmountByAddr(std::string addr, uint64_t & awardAmount);

    int GetTimeFromBlockHash(const std::string & blockHash, uint64_t & blockTime);

    int GetBlockNumInUnitTime(uint64_t & blockSum);

    int GetBaseAward(uint64_t blockNum);

    int GetAward(const uint64_t blockNum, uint64_t & awardAmount);

    std::multimap<uint32_t, std::string> &GetDisAward();

private:
    
    uint64_t now_time;
    
    uint32_t need_verify_count;
    
    uint64_t award_pool;
    
    std::vector<std::string> vec_addr;

    std::vector<double> vec_onlinet;
    
    std::multimap<uint32_t, std::string> map_addr_award;

    
    uint sign_amount;

    
    
    
    
    
    std::tuple<std::vector<double>, std::vector<uint>, std::vector<double>, std::vector<double>> t_list;

    uint64_t unitTime = 5 * 60;  
    uint64_t minAward = 0.025 * DECIMAL_NUM;  
    uint64_t unitTimeMinBlockNum = 5;  
    uint32_t slopeCurve = 70;   
    uint64_t awardTotal = (uint64_t)80000000 * DECIMAL_NUM;  
};

}