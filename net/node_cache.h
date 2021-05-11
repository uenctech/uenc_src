// Create:The node was cached from server, 20210304   LiuMing
// node_cache.h

#include "peer_node.h"
#include <map>
#include <mutex>
#include <vector>
#include <string>
#include <time.h>

#include "../utils/CTimer.hpp"

using namespace std;

class NodeCache
{
public:
    NodeCache();
    ~NodeCache() = default;

    bool add(const Node& node);
    bool add(const vector<Node>& nodes);
    void reset_node(const vector<Node>& nodes);

    std::vector<Node> get_nodelist();

    int timer_start();

    void fetch_newest_node();

protected:
    bool is_exist(const Node& node);

    int find(const Node& node);

    void init_start_time();

private:
    static int timer_process(NodeCache* cache);

private:
    static const size_t MAX_SIZE = 100;
    std::mutex mutex_for_nodes_;
	//std::map<string, Node> node_map_;
    std::vector<Node> nodes_;
    time_t starttime_;

    CTimer timer_;
};