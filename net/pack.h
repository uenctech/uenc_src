#ifndef _PACK_H_
#define _PACK_H_

#include <string>
#include "net.pb.h"
#include "common.pb.h"
#include "./peer_node.h"
#include "utils/compress.h"

#define VERSION "1.0" 

class Pack
{
public:

	static const net_pack pack_to_pack(const char *data, const int data_len);
	static const net_pack pack_to_pack(const std::string & data);
	static void packag_to_buff(const net_pack & pack, char* buff, int buff_len);
	
	static std::string packag_to_str(const net_pack& pack);
	static bool apart_pack(net_pack& pk, const char* pack, int len);

	template <typename T>
	static bool InitCommonMsg(CommonMsg & msg, T& submsg, int32_t encrypt = 0, int32_t compress = 0);
	static bool common_msg_to_pack(const CommonMsg& msg, net_pack& pack);

};


template <typename T>
bool Pack::InitCommonMsg(CommonMsg& msg, T& submsg, int32_t encrypt, int32_t compress)
{
	msg.set_type(submsg.descriptor()->name());
	msg.set_version(VERSION);
	msg.set_encrypt(encrypt);
	msg.set_compress(compress);
	string tmp = submsg.SerializeAsString();
	if (compress) {
		Compress cpr(tmp);
		msg.set_data(cpr.m_compress_data);
		
		
	}
	else {
		msg.set_data(tmp);
	}
	return true;
}

#endif