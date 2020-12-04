#include <endian.h>
#include <vector>
#include "ca_message.h"
#include "ca_util.h"
#include "ca_ios.h"
#include "../net/define.h"

cstring *message_str(const unsigned char netmagic[4],
             const char *command_,
             const void *data, uint32_t data_len)
{
    cstring *s = cstr_new_sz(P2P_HEADER_SIZE + data_len);

    /* network identifier (magic number) */
    cstr_append_buf(s, netmagic, 4);
    
    /* command string */
    char command[12 + 2] = {};
    strncpy(command, command_, 12);
    cstr_append_buf(s, command, 12);

    /* data length */
  //  uint32_t data_len_le = htole32(data_len);
    uint32_t data_len_le = H32toh(data_len);
    cstr_append_buf(s, &data_len_le, 4);
    
    /* data checksum */
    unsigned char md32[4];
    
    bu_Hash4(md32, data, data_len);

    cstr_append_buf(s, &md32[0], 4);
    
    /* data payload */
    if (data_len > 0)
        cstr_append_buf(s, data, data_len);

    return s;
}


cstring *txstr_append_signid(const void *data, uint32_t data_len,  int verify_num, const std::vector<std::string>& ids )
{
    using namespace std;
    int len = ids.size();
    int bufLen = K_ID_LEN / 4;
    cstring *s = cstr_new_sz(sizeof(int)*2 + bufLen * len + data_len );
    cstr_append_buf(s, &verify_num, sizeof(int));
    cstr_append_buf(s, &len, sizeof(int));

    for (int i = 0; i < len; i++)
    {
        cstr_append_buf(s, ids[i].data(), bufLen);
    }
    cstr_append_buf(s, data, data_len);
    return s;
}

bool txstr_parse(void *data, uint32_t data_len, CTransaction &tx, std::vector<std::string>& ids, int * verify_num )
{
    using namespace std;
    if(data == nullptr || data_len == 0)
    {
        return false;
    }
    char * tmp = (char *)data;
    *verify_num = *(int*)tmp;


    tmp += sizeof(int);
    int len = *(int*)tmp;

    tmp += sizeof(int);
    int bufLen = K_ID_LEN / 4;
    for(int i = 0; i < len; i++)
    {
        char buff[ bufLen + 1] = {0};
        memcpy(buff,tmp, bufLen );
        string s(buff);
        ids.push_back(s);
        tmp += bufLen;
    }

    string txstr(tmp, data_len - sizeof(int) * 2 - bufLen * len);
    tx.ParseFromString(txstr);
    // cout << "ip:" << tx.ip() << "\n"
    //     << "hash" << tx.hash() << endl;
    return true;
}
