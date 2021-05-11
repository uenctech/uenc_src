// ca_block_http_callback.h
// Create: 20210128   Liu
// Add infomation of the new block to http url

#ifndef __CA_BLOCK_HTTP_CALLBACK_H__
#define __CA_BLOCK_HTTP_CALLBACK_H__

#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "block.pb.h"

using namespace std;

class CBlockHttpCallback
{
public:
    CBlockHttpCallback();

    bool AddBlock(const string& block);
    bool AddBlock(const CBlock& block);
    bool Start(const string& url, int port);
    void Stop();
    int Work();
    bool IsRunning();
    void Test();
    void Test2();

    static int Process(CBlockHttpCallback* self);

private:
    string ToJson(const CBlock& block);
    bool SendBlockHttp(const string& block);

private:
    vector<string> blocks_;
    thread work_thread_;
    mutex mutex_;
    condition_variable cv_;
    volatile atomic_bool running_;

    string url_;
    int port_;
};

#endif