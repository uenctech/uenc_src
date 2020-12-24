#include "ca_timefuc.h"
#include <iostream>
#include <thread>

using namespace std;

int outtime::PasswordCount = 0;
 int outtime::count = 300;
 bool outtime::flag = false ;

int outtime::PasswordCountFuc()
{
    countlock.lock();
    PasswordCount++;
    std::cout<<"PasswordCount = "<<PasswordCount<<std::endl;
    countlock.unlock();
    return PasswordCount;
}

int outtime::SetPasswordCountZero()
{
    PasswordCount = 0;
    return PasswordCount;
}

int outtime::GetPasswordCount()
{
    return PasswordCount;
}

bool outtime::GetFlag()
{
    return flag;
}

int outtime::GetCount()
{
    return count;
}


int counttimefunc()
{
    outtime timercout;
    for(int i = 0; i < 300; i++)
    {
        timercout.timeCountLock.lock();
        timercout.count --;
        timercout.timeCountLock.unlock();
        sleep(1);
    }
    timercout.flag = false;
    timercout.count = 300;
    return  timercout.SetPasswordCountZero();
}

int outtime::alarmfunc()
{
    flag = true;
    std::thread counter(counttimefunc);
    counter.detach();
    return 0;
} 
