#ifndef __CA_TIMEFUC_H__
#define __CA_TIMEFUC_H__
#include<unistd.h>
#include <pthread.h>
#include <shared_mutex>


class outtime
{
public:  
    outtime() = default;
    ~outtime() = default;

    int PasswordCountFuc();
    int SetPasswordCountZero();

    int  GetPasswordCount();
    int  alarmfunc();
    bool GetFlag();
    int  GetCount();
    std::shared_mutex countlock;
    std::shared_mutex timeCountLock;
    static bool flag ;
    static int count ;
private:
    static int PasswordCount;
    
};


# endif