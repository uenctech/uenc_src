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
     * @description: 判断是否可输入密码
     * @param limitSecond: 若超过限制，则剩余多少秒后可再次输入
     * @return true: 没有禁止可输入
     *          false: 有禁止不可输入
     */
    bool IsOk(uint32_t & limitSecond); 

    /**
     * @description: 输入错误
     * @param 无
     * @return -1: 尝试次数未超限
     *          -2: 尝试次数已超限
     *          0: 尝试次数超限，启动倒计时
     */
    int Wrong();

    /**
     * @description: 输入正确
     * @param 无
     * @return 0 设置成功
     */
    int Right();

private:

    /**
     * @description: 倒计时
     * @param check CPwdAttackChecker 类指针
     * @return 
     */
    static int Count(CPwdAttackChecker * check);

    /**
     * @description: 重置
     * @param 无
     * @return 0 重置成功
     */
    int Reset();

private:
    CTimer timer_;
    std::mutex mutex_;

    uint32_t tryCount_; // 尝试次数
    uint32_t limitSecond_; // 倒计时秒数

    static uint32_t kMaxTryCount; // 最大尝试次数
    static uint32_t kMaxlimitSecond; // 最大倒计时秒数
    
};



#endif // !_CA_CPWDATTACKCHECKER_H