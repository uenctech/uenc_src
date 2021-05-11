#include "node_cache.h"
#include <iostream>
#include <cassert>
#include "../utils/singleton.h"
#include "net_api.h"

NodeCache::NodeCache()
{
    nodes_.reserve(MAX_SIZE);
    init_start_time();
}

bool NodeCache::add(const Node& node)
{
    std::lock_guard<std::mutex> lck(mutex_for_nodes_);
    
    int i = -1;
    if ((i = find(node)) >= 0)
    {
        nodes_[i] = node;
        return true;
    }

    if (nodes_.size() >= MAX_SIZE)
    {
        nodes_.erase(nodes_.begin());
    }
    nodes_.push_back(node);

    return true;
}

bool NodeCache::add(const vector<Node>& nodes)
{
    for (auto& node : nodes)
    {
        add(node);
    }

    return true;
}

void NodeCache::reset_node(const vector<Node>& nodes)
{

}

bool NodeCache::is_exist(const Node& node)
{
    for (auto& current : nodes_)
    {
        if (current.id == node.id)
        {
            return true;
        }
    }

    return false;
}

int NodeCache::find(const Node& node)
{
    for (size_t i = 0; i < nodes_.size(); ++i)
    {
        if (nodes_[i].id == node.id)
        {
            return i;
        }
    }

    return -1;
}

std::vector<Node> NodeCache::get_nodelist()
{
    return nodes_;
}

void NodeCache::init_start_time()
{
    this->starttime_ = time(NULL);
}

void NodeCache::fetch_newest_node()
{
    bool is_fetch_public = false;
    time_t now = time(NULL);
    static const time_t FETCH_PUBLIC_INTERVAL = 1 * 60 * 60;
    if ((now - this->starttime_) >= FETCH_PUBLIC_INTERVAL)
    {
        is_fetch_public = true;
        init_start_time();
        std::cout << "Fetch the public node! ^^^VVV" << std::endl;
    }

    // vector<Node> nodelist = Singleton<PeerNode>::get_instance()->get_public_node();
    // if (nodelist.empty())
    // {
    //     std::cout << "In Node cache, public node is empty." << std::endl;
    //     return ;
    // }

    // std::random_device device;
	// std::mt19937 engine(device());
    // while (!nodelist.empty())
    // {
    //     size_t index = engine() % nodelist.size();
    //     if (nodelist[index].fd > 0)
    //     {
    //         std::cout << "Send request for getting height." << nodelist[index].id << std::endl;
    //         net_com::SendGetHeightReq(nodelist[index], is_fetch_public);
    //         break ;
    //     }
    //     nodelist.erase(nodelist.begin() + index);
    // }
    auto publicId = Singleton<PeerNode>::get_instance()->get_self_node().public_node_id;
    if (!publicId.empty())
    {
        Node node;
        auto find = Singleton<PeerNode>::get_instance()->find_node(publicId, node);
        if (find)
        {
            net_com::SendGetHeightReq(node, is_fetch_public);
        }
        else
        {
           std::cout << "In node cache: find the public node failed!" << std::endl;
        }
    }
    else
    {
        std::cout << "In node cache: public id is empty!" << std::endl;
    }
}

int NodeCache::timer_start()
{
    // if (Singleton<PeerNode>::get_instance()->get_self_node().is_public_node)
    // {
    //     return 1;
    // }
    
    this->timer_.AsyncLoop(1000 * 15, NodeCache::timer_process, this);

    return 0;
}

int NodeCache::timer_process(NodeCache* cache)
{
    assert(cache != nullptr);
    if (cache == nullptr)
    {
        std::cout << "Node cache timer is null!" << std::endl;
        return -1;
    }

    cache->fetch_newest_node();

    return 0;
}
