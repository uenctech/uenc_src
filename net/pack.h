/*
 * @Author: your name
 * @Date: 2021-01-14 17:59:45
 * @LastEditTime: 2021-03-27 14:16:38
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\net\pack.h
 */
#ifndef _PACK_H_
#define _PACK_H_

#include <string>
#include "net.pb.h"
#include "common.pb.h"
#include "./peer_node.h"
#include "utils/compress.h"
#include "common/version.h"

class Pack
{
public:

	static void packag_to_buff(const net_pack & pack, char* buff, int buff_len);
	
	static std::string packag_to_str(const net_pack& pack);
	static bool apart_pack(net_pack& pk, const char* pack, int len);

	template <typename T>
	static bool InitCommonMsg(CommonMsg & msg, T& submsg, int32_t encrypt = 0, int32_t compress = 0);
	static bool common_msg_to_pack(const CommonMsg& msg, const int8_t priority, net_pack& pack);

};


template <typename T>
bool Pack::InitCommonMsg(CommonMsg& msg, T& submsg, int32_t encrypt, int32_t compress)
{
	msg.set_type(submsg.descriptor()->name());
	msg.set_version(getVersion());
	msg.set_encrypt(encrypt);
	
	const string & tmp = submsg.SerializeAsString();
	if (compress) 
	{
		Compress cpr(tmp);
		// 尝试压缩，若压缩比不佳则不使用压缩
		if (cpr.m_compress_data.size() > tmp.size())
		{
			msg.set_compress(0);
			msg.set_data(tmp);
		}
		else
		{
			msg.set_compress(compress);
			msg.set_data(cpr.m_compress_data);
		}
	}
	else 
	{
		msg.set_compress(0);
		msg.set_data(tmp);
	}
	return true;
}

#endif//_PACK_H_