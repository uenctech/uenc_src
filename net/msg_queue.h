
#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include <memory>
#include <list>
#include <string>
#include <utility>
#include <condition_variable>
#include "./ip_port.h"
#include "../include/logging.h"
#include "./peer_node.h"


using namespace std;
enum DataType
{
    E_READ,
    E_WORK,
    E_WRITE
};

typedef struct msg_data
{
    DataType type;
    int fd;
    uint16_t port;
    uint32_t ip;
    std::string data;
    net_pack pack;
    id_type id;
}MsgData;

class MsgQueue
{
public:
  
    std::string str_info_;
    std::atomic<i64> msg_cnt_;
	std::list<MsgData> msg_list_;
	std::mutex mutex_for_list_;

	std::condition_variable_any m_notEmpty;	
	std::condition_variable_any m_notFull;	
	size_t max_size_;						

private:
	bool IsFull() const
	{
		return msg_list_.size() == max_size_;
	}
	bool IsEmpty() const
	{
		return msg_list_.empty();
	}

public:
    bool push(MsgData& data)
    {
		std::lock_guard<std::mutex> lck(mutex_for_list_);
		while (IsFull())
		{
			debug(" %s the blocking queue is full,waiting...",str_info_.c_str());
			m_notFull.wait(mutex_for_list_);
		}
		msg_list_.push_back(std::move(data));
		m_notEmpty.notify_one();
		++msg_cnt_;
		
		
		return true;

    };
	


    bool try_wait_pop(MsgData & out)
    {
		std::lock_guard<std::mutex> lck(mutex_for_list_);
		while (IsEmpty())
		{
			
			m_notEmpty.wait(mutex_for_list_);
		}

		out = std::move(msg_list_.front());
		msg_list_.pop_front();
		m_notFull.notify_one();
		--msg_cnt_;
		
		

		return true;
    };
    
    bool pop(MsgData *ret)
    {
	    return ret;
    };
    MsgQueue() :str_info_(""),msg_cnt_(0), max_size_(10000*5){}
	MsgQueue(std::string strInfo) :msg_cnt_(0), max_size_(10000*5){
         str_info_ = strInfo;
    }
    ~MsgQueue() = default;
};


#endif