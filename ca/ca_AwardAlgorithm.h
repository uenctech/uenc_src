#include "../utils/string_util.h"
#include "../utils//util.h"
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
     * @brief 测试
     */
    void TestPrint(bool lable = false);

    /**
     * @brief 组合工具函数 得出额外奖励TODO
     * @param need_verify 共识数
     * @param vec_hash 钱包地址数组
     */
    int Build(uint need_verify_count, 
                        const std::vector<std::string> &vec_addr,
                        const std::vector<double> &vec_online, 
                        const std::vector<uint64_t> &vec_award_total, 
                        const std::vector<uint64_t> &vec_sign_num);

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
    /* ntp时间戳 */
    uint64_t now_time;
    /* 共识数 */
    uint32_t need_verify_count;
    /* 计算出的奖池金额 */
    uint64_t award_pool;
    /* 签名钱包地址数组 */
    std::vector<std::string> vec_addr;
    /* 签名钱包地址获得总奖励值 */
    std::vector<uint64_t> vec_award_total;
    /* 签名钱包地址总签名数 */
    std::vector<uint64_t> vec_sign_sum;
    /* 签名节点在线时长 */
    std::vector<double> vec_online;
    /* 奖励分配map */
    std::multimap<uint32_t, std::string> map_addr_award;

    /* 总签名数 */
    uint sign_amount;

    /* 测试用参数 */
    // vector<double> &total_online =  get<0>(t_list);
    // vector<uint> &total_sign =  get<1>(t_list);
    // vector<double> &total_award =  get<2>(t_list);
    // vector<double> &rate = get<3>(t_list); //所有地址的比率 总奖励额/签名总数/时长(天)
    std::tuple<std::vector<double>, std::vector<uint>, std::vector<double>, std::vector<double>> t_list;

    uint64_t unitTime = 5 * 60;  // 时间区间，单位为秒，用于计算时间区间内产生的块数
    uint64_t minAward = 0.025 * DECIMAL_NUM;  // 最低奖励值
    uint64_t unitTimeMinBlockNum = 5;  // unitTime时间内最少区块数，低于5，按5计算
    uint32_t slopeCurve = 70;   // 曲线斜率
    uint64_t awardTotal = (uint64_t)80000000 * DECIMAL_NUM;  // 总奖励值
    uint64_t firstBlockTime = 1604222598187783;
};

}