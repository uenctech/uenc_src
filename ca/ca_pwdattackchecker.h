/*
 * @Author: your name
 * @Date: 2020-12-15 14:12:36
 * @LastEditTime: 2020-12-16 09:40:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_Cpwdattackchecker.h
 */


#ifndef _CA_CPWDATTACKCHECKER_H
#define _CA_CPWDATTACKCHECKER_H

#include "../utils/CTimer.hpp"

#include <mutex>


class CPwdAttackChecker
{
public:
    CPwdAttackChecker();
    CPwdAttackChecker(CPwdAttackChecker &&) = delete;
    CPwdAttackChecker(const CPwdAttackChecker &) = delete;
    CPwdAttackChecker &operator=(CPwdAttackChecker &&) = delete;
    CPwdAttackChecker &operator=(const CPwdAttackChecker &) = delete;
    ~CPwdAttackChecker();

public:
    
    /**
     * @description: Determine whether you can enter a password 
     * @param limitSecond: If the limit is exceeded, how many seconds are left to enter again 
     * @return true: No prohibition can be entered 
     *          false: No input is forbidden 
     */
    bool IsOk(uint32_t & limitSecond); 

    /**
     * @description: input error 
     * @param no
     * @return -1: The number of attempts has not exceeded the limit 
     *          -2: The number of attempts has exceeded the limit 
     *          0: The number of attempts exceeds the limit, and the countdown starts 
     */
    int Wrong();

    /**
     * @description: Entered correctly 
     * @param no 
     * @return 0 Set successfully 
     */
    int Right();

private:

    /**
     * @description: Countdown 
     * @param check CPwdAttackChecker Class pointer 
     * @return 
     */
    static int Count(CPwdAttackChecker * check);

    /**
     * @description: Reset 
     * @param no
     * @return 0 Reset successfully 
     */
    int Reset();

private:
    CTimer timer_;
    std::mutex mutex_;

    uint32_t tryCount_; // Number of attempts 
    uint32_t limitSecond_; // Countdown seconds 

    static uint32_t kMaxTryCount; // Maximum number of attempts 
    static uint32_t kMaxlimitSecond; // Maximum countdown seconds 
    
};



#endif // !_CA_CPWDATTACKCHECKER_H