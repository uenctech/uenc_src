#include <iostream>
#include <vector>
#include <algorithm>



/* ==================================================================================== 
 # @description: 随机选择下一次签名的节点
 # @param  idsSortInfo 所有可选择的节点
 # @param  nextNodeNumber 最终要选择的节点数
 # @param  nextNode 最终要选择的节点
 # @return 成功返回0
 ==================================================================================== */
int RandomSelectNode(std::vector< std::pair<std::string, uint64_t> > &idsSortInfo, int nextNodeNumber, std::vector<std::string> &nextNode );