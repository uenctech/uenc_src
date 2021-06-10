/*
 * @Author: your name
 * @Date: 2020-11-17 14:14:11
 * @LastEditTime: 2020-11-17 14:14:12
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\utils\CTimer.hpp
 */
//
//  CTimer.hpp
//
//  Created by lzj<lizhijian_21@163.com> on 2018/7/20.
//  Copyright © 2018 ZJ. All rights reserved. 
//

#ifndef CTimer_hpp
#define CTimer_hpp

#include <stdio.h>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <condition_variable>

class CTimer
{
public:
    CTimer(const std::string sTimerName = "");   //Construct a timer with a name 
    ~CTimer();
    
    /**
     Start running timer 
     @param msTime Delay operation (unit: ms) 
     @param task Task function interface 
     @param bLoop Whether to loop (execute 1 time by default) 
     @param async Whether it is asynchronous (default asynchronous) 
     @return true:Ready to execute ，Otherwise fail 
     */
    bool Start(unsigned int msTime, std::function<void()> task, bool bLoop = false, bool async = true);
    
    /**
     Cancel the timer, the synchronous timer cannot be cancelled (if the task code has been executed, the cancellation is invalid) 
     */
    void Cancel();
    
    /**
     Execute once simultaneously 
     #This interface doesn’t feel very useful, but the reality is here for the time being 
     @param msTime Delay time (ms) 
     @param fun Function interface or lambda code block 
     @param args parameter 
     @return true:Ready to execute ，Otherwise fail 
     */
    template<typename callable, typename... arguments>
    bool SyncOnce(int msTime, callable&& fun, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(fun), std::forward<arguments>(args)...)); //Bind task function or lambda to function 
        return Start(msTime, task, false, false);
    }
    
    /**
     Execute a task asynchronously 
     
     @param msTime Delay and interval time 
     @param fun Function interface or lambda code block 
     @param args parameter 
     @return true:Ready to execute ，Otherwise fail 
     */
    template<typename callable, typename... arguments>
    bool AsyncOnce(int msTime, callable&& fun, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(fun), std::forward<arguments>(args)...));
        
        return Start(msTime, task, false);
    }
    
    /**
     Execute a task asynchronously (Execute after 1 millisecond delay by default) 
     
     @param fun Function interface or lambda code block 
     @param args parameter 
     @return true:Ready to execute ，Otherwise fail 
     */
    template<typename callable, typename... arguments>
    bool AsyncOnce(callable&& fun, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(fun), std::forward<arguments>(args)...));
        
        return Start(1, task, false);
    }
    
    
    /**
     Asynchronous loop execution of tasks 
     @param msTime Delay and interval time 
     @param fun Function interface or lambda code block 
     @param args parameter 
     @return true:Ready to execute ，Otherwise fail 
     */
    template<typename callable, typename... arguments>
    bool AsyncLoop(int msTime, callable&& fun, arguments&&... args) {
        std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(fun), std::forward<arguments>(args)...));
        
        return Start(msTime, task, true);
    }
    
    
private:
    void DeleteThread();    //Delete task thread 

public:
    int m_nCount = 0;   //Cycles 
    
private:
    std::string m_sName;   //Timer name 
    
    std::atomic_bool m_bExpired;       //Whether the loaded task has expired 
    std::atomic_bool m_bTryExpired;    //Equipment expires loaded tasks (marking) 
    std::atomic_bool m_bLoop;          //Whether to loop 
    
    std::thread *m_Thread = nullptr;
    std::mutex m_ThreadLock;
    std::condition_variable_any m_ThreadCon;
};

#endif /* CTimer_hpp */
