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
 * @brief Extra reward algorithm 
*/

namespace a_award {

class AwardAlgorithm {
public:
    /**
     * @brief Initialize private variables 
     */
    AwardAlgorithm();

    /**
     * @brief test 
     */
    void TestPrint(bool lable = false);

    /**
     * @brief Combine tool functions to get extra bonus TODO 
     * @param need_verify Consensus number 
     * @param vec_hash Wallet address array 
     */
    int Build(uint need_verify_count, 
                        const std::vector<std::string> &vec_addr,
                        const std::vector<double> &vec_online, 
                        const std::vector<uint64_t> &vec_award_total, 
                        const std::vector<uint64_t> &vec_sign_num);

    /**
     * @brief Calculate the additional rewards allocated by each wallet address 
     */
    int AwardList();

    /**
     * @brief Query the total number of signatures 
     * @param addr Signature wallet address 
     * @param addr Number of signatures 
     * @return Return error code 
     */
    int GetSignCountByAddr(const std::string addr, uint64_t & count);

    /**
     * @brief Query the total number of signature bonuses 
     * @param addr Signature wallet address 
     * @return Return the total bonus amount obtained by signing 
     */
    int GetAwardAmountByAddr(std::string addr, uint64_t & awardAmount);

    int GetTimeFromBlockHash(const std::string & blockHash, uint64_t & blockTime);

    int GetBlockNumInUnitTime(uint64_t & blockSum);

    int GetBaseAward(uint64_t blockNum);

    int GetAward(const uint64_t blockNum, uint64_t & awardAmount);

    std::multimap<uint32_t, std::string> &GetDisAward();

private:
    /* ntp timestamp  */
    uint64_t now_time;
    /* 共识数 */
    uint32_t need_verify_count;
    /* Calculated prize pool amount  */
    uint64_t award_pool;
    /* Signature wallet address array  */
    std::vector<std::string> vec_addr;
    /* Sign the wallet address to get the total reward value  */
    std::vector<uint64_t> vec_award_total;
    /* Total number of signatures in signed wallet address  */
    std::vector<uint64_t> vec_sign_sum;
    /* Signing node online time  */
    std::vector<double> vec_online;
    /* Reward distribution map  */
    std::multimap<uint32_t, std::string> map_addr_award;

    /* Total number of signatures  */
    uint sign_amount;

    /* Test parameters  */
    // vector<double> &total_online =  get<0>(t_list);
    // vector<uint> &total_sign =  get<1>(t_list);
    // vector<double> &total_award =  get<2>(t_list);
    // vector<double> &rate = get<3>(t_list); //Ratio of all addresses Total reward amount/total number of signatures/duration (days) 
    std::tuple<std::vector<double>, std::vector<uint>, std::vector<double>, std::vector<double>> t_list;

    uint64_t unitTime = 5 * 60;  // Time interval, in seconds, used to calculate the number of blocks generated in the time interval 
    uint64_t minAward = 0.025 * DECIMAL_NUM;  // Minimum reward value 
    uint64_t unitTimeMinBlockNum = 5;  // The minimum number of blocks within unitTime, if it is less than 5, it is calculated as 5 
    uint32_t slopeCurve = 70;   // Curve slope 
    uint64_t awardTotal = (uint64_t)80000000 * DECIMAL_NUM;  // Total reward value 
    uint64_t firstBlockTime = 1604222598187783;
};

}