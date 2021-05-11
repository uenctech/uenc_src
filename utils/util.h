/*
 * @Author: your name
 * @Date: 2021-03-17 09:27:10
 * @LastEditTime: 2021-03-27 14:06:04
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\utils\util.h
 */
#ifndef _Util_H_
#define _Util_H_

#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <functional>
#include "../utils/string_util.h"

class Util
{
    
public:
    Util();
    ~Util();
    
    static uint32_t adler32(const unsigned char *data, size_t len); 

    /* ====================================================================================  
    # @description: 判断版本是否兼容
    # @param recvVersion 接收到的数据的版本
    # @return: 成功: 0;
    # 			失败: -1, 接收到的数据的版本低于最低兼容版本
    ==================================================================================== */
   static int IsVersionCompatible( std::string recvVersion );

   static int IsLinuxVersionCompatible(const std::vector<std::string> & vRecvVersion);

   static int IsOtherVersionCompatible(const std::string & vRecvVersion, bool bIsAndroid);

};



struct ExitCaller
{
	~ExitCaller() { functor_(); }
	ExitCaller(std::function<void()>&& functor) : functor_(std::move(functor)) {}

private:
	std::function<void()> functor_;
};

#endif