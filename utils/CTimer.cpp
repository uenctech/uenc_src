/*
 * @Author: your name
 * @Date: 2020-11-17 14:14:33
 * @LastEditTime: 2020-11-17 14:14:33
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\utils\CTimer.cpp
 */
//
//  CTimer.cpp
//
//  Created by lzj<lizhijian_21@163.com> on 2018/7/20.
//  Copyright © 2018年 ZJ. All rights reserved.
//

#include "CTimer.hpp"
#include <future>

CTimer::CTimer(const std::string sTimerName):m_bExpired(true), m_bTryExpired(false), m_bLoop(false)
{
    m_sName = sTimerName;
}

CTimer::~CTimer()
{
    m_bTryExpired = true;   //尝试使任务过期
    DeleteThread();
}

bool CTimer::Start(unsigned int msTime, std::function<void()> task, bool bLoop, bool async)
{
    if (!m_bExpired || m_bTryExpired) return false;  //任务未过期(即内部仍在存在或正在运行任务)
    m_bExpired = false;
    m_bLoop = bLoop;
    m_nCount = 0;

    if (async) {
        DeleteThread();
        m_Thread = new std::thread([this, msTime, task]() {
            if (!m_sName.empty()) {
#if (defined(__ANDROID__) || defined(ANDROID))      //兼容Android
                pthread_setname_np(pthread_self(), m_sName.c_str());
#elif defined(__APPLE__)                            //兼容苹果系统
                pthread_setname_np(m_sName.c_str());    //设置线程(定时器)名称
#endif
            }
            
            while (!m_bTryExpired) {
                m_ThreadCon.wait_for(m_ThreadLock, std::chrono::milliseconds(msTime));  //休眠
                if (!m_bTryExpired) {
                    task();     //执行任务

                    m_nCount ++;
                    if (!m_bLoop) {
                        break;
                    }
                }
            }
            
            m_bExpired = true;      //任务执行完成(表示已有任务已过期)
            m_bTryExpired = false;  //为了下次再次装载任务
        });
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(msTime));
        if (!m_bTryExpired) {
            task();
        }
        m_bExpired = true;
        m_bTryExpired = false;
    }
    
    return true;
}

void CTimer::Cancel()
{
    if (m_bExpired || m_bTryExpired || !m_Thread) {
        return;
    }
    
    m_bTryExpired = true;
}

void CTimer::DeleteThread()
{
    if (m_Thread) {
        m_ThreadCon.notify_all();   //休眠唤醒
        m_Thread->join();           //等待线程退出
        delete m_Thread;
        m_Thread = nullptr;
    }
}

