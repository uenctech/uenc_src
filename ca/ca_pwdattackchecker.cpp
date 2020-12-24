/*
 * @Author: your name
 * @Date: 2020-12-15 14:12:42
 * @LastEditTime: 2020-12-16 09:26:06
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\ca\ca_pwdattackchecker.cpp
 */


#include "ca_pwdattackchecker.h"
#include <iostream>

uint32_t CPwdAttackChecker::kMaxTryCount = 3;
uint32_t CPwdAttackChecker::kMaxlimitSecond = 600;


CPwdAttackChecker::CPwdAttackChecker()
{
    this->limitSecond_ = 0;
    this->tryCount_ = 0;
}

CPwdAttackChecker::~CPwdAttackChecker()
{
}

bool CPwdAttackChecker::IsOk(uint32_t & limitSecond)
{
    if (this->limitSecond_ > 0)
    {
        limitSecond = this->limitSecond_; 
        return false;
    }
    return true;
}

int CPwdAttackChecker::Wrong()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++tryCount_;
    std::cout << "tryCount: " << tryCount_ << std::endl;

    if (tryCount_ < CPwdAttackChecker::kMaxTryCount)
    {
        return -1;
    }
    else if (tryCount_ > CPwdAttackChecker::kMaxTryCount)
    {
        return -2;
    }
    else 
    {
        limitSecond_ = CPwdAttackChecker::kMaxlimitSecond;
        timer_.AsyncLoop(1000, Count, this);
    }

    return 0;
}

int CPwdAttackChecker::Right()
{
    std::lock_guard<std::mutex> lock(mutex_);
    Reset();
    return 0;
}

int CPwdAttackChecker::Count(CPwdAttackChecker * checker)
{
    if (checker == nullptr)
    {
        return -1;
    }

    std::lock_guard<std::mutex> lock(checker->mutex_);
    checker->limitSecond_--;
   
    if (checker->limitSecond_ == 0)
    {
        std::cout<<" == 0 reset "<<std::endl;
        checker->Reset();
    }
    std::cout << "checker->limitSecond: " << checker->limitSecond_ << std::endl;
    return 0;
} 

int CPwdAttackChecker::Reset()
{
    limitSecond_ = 0;
    tryCount_ = 0;
    timer_.Cancel();
    return 0;
}