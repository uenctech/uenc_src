#include <string>
#include <random>
#include <chrono>
#include "./pack.h"
#include "../../include/logging.h"
#include "net.pb.h"
#include "../../utils/util.h"


using namespace std;

const net_pack Pack::pack_to_pack(const char* data, const int data_len)
{
	net_pack pack;

	pack.len = data_len;
	pack.data = string(data, data_len);
	pack.len = sizeof(int)*2 + pack.data.size() + 32;

	return pack;
}

const net_pack Pack::pack_to_pack(const std::string& data)
{
	net_pack pack;

	pack.len = (int)data.size();
	pack.data = data;
	pack.len = sizeof(int)*2 + pack.data.size() + 32;

	return pack;
}

void Pack::packag_to_buff(const net_pack & pack, char* buff, int buff_len)
{
	memcpy(buff, &pack.len, 4);
	memcpy(buff + 4, pack.data.data(), pack.data.size());
	memcpy(buff + 4 + pack.data.size(), &pack.checksum, 4);
	memcpy(buff + 4 + pack.data.size() + 4, &pack.end_flag, 4);
}

string Pack::packag_to_str(const net_pack& pack)
{
	int buff_size = pack.len + sizeof(int);
	char* buff = new char[buff_size]{0};
	ExitCaller ec([=] { delete[] buff; });
	Pack::packag_to_buff(pack, buff, buff_size);
	string msg(buff, buff_size);
	return msg;
}




bool Pack::apart_pack( net_pack& pk, const char* pack, int pack_len)
{
	if (NULL == pack)
	{
		error("apart_pack is NULL");
		return false;
	}
	if(pack_len < 8)
	{	
		error("apart_pack len < 8\n");
		return false;
	}

	pk.len = pack_len;
	pk.data = std::string(pack , pack_len - sizeof(uint32_t) * 2); 

	memcpy(&pk.checksum, pack + pk.data.size(),     4);
	memcpy(&pk.end_flag, pack + pk.data.size() + 4, 4);

	return true;
}


bool Pack::common_msg_to_pack(const CommonMsg& msg, net_pack& pack)
{
	pack.data = msg.SerializeAsString();
	pack.len = pack.data.size() + sizeof(int) + sizeof(int);
	pack.checksum = Util::adler32(pack.data.data(), pack.data.size());

	return true;
}
