#ifndef _EBPC_CA_INTERFACE_H_
#define _EBPC_CA_INTERFACE_H_
#include "../include/cJSON.h"
#include "../net/msg_queue.h"

#include "ca_global.h"


using namespace std;
// 登录 退出 相关
/**
 * @description: 初始化 指定公私钥文件的路径
 * @param path 初始化路径
 * @return: 无
 */
void interface_Init(const char *path);

/**
 * @description: 生成公私钥文件 用于注册时
 * @param 无 
 * @return: 无
 */
bool interface_GenerateKey();

// 钱包公私密钥相关
/**
 * @description: 导出助记词
 * @param: bs58address 钱包地址，若传null则以interface_SetKey设置的地址导出助记词
 * @param: out 输出字符串
 * @param: outLen 输出字符串长度
 * @return: 若导出成功返回助记词个数，out中是以空格分隔的助记词字符串，若返回0表示导出失败
 */
int interface_GetMnemonic(const char *bs58address, char *out, int outLen);

/**
 * @description: 导出私钥
 * @param: bs58address 钱包地址，若传null则以interface_SetKey设置的地址导出私钥
 * @param: pridata 输出字符串
 * @param: iLen 输出字符串长度
 * @return: 成功返回true，失败返回false
 */
bool interface_GetPrivateKeyHex(const char *bs58address, char *pridata, unsigned int iLen);

/**
 * @description: 导入私钥
 * @param: pridata 私钥的十六进制字符串
 * @return: 成功返回true，失败返回false
 */
bool interface_ImportPrivateKeyHex(const char *pridata);

/**
 * @description: 导出keystore
 * @param: bs58address 钱包地址，若传null则以interface_SetKey设置的地址导出keystore
 * @param: pass 密码
 * @param: buf 输出字符串
 * @param: bufLen 输出字符串长度
 * @return: 成功返回true，失败返回false
 */
int  interface_GetKeyStore(const char *bs58address, const char *pass, char *buf, unsigned int bufLen);

/**
 * @description: 获取钱包地址
 * @param  bs58address 钱包地址
 * @param iLen 钱包地址长度
 * @mark 函数返回结果将放置到 bs58address 地址中
 * @return: 成功返回ture，失败返回false
 */
bool interface_GetBase58Address(char *bs58address, unsigned int iLen);

/**
 * @description: 获得节点主账户金额
 * @param bs58Address 用户钱包地址
 * @param amount 给予金钱数
 * @param outdata 返回数据指针
 * @param outdataLen 返回数据长度
 * @mark 返回数据指针out_data需要调用free进行释放
 * @return: 操作成功返回true，失败返回false
 */
bool interface_ShowMeTheMoney(const char * bs58Address, double amount, char *outdata, size_t *outdataLen);

// misc
typedef enum emInterfaceClientType
{
    kInterfaceClientType_PC,
    kInterfaceClientType_iOS,
    kInterfaceClientType_Android,
}InterfaceClientType;

typedef enum emInterfaceClientLanguage
{
    kInterfaceClientLanguage_zh_CN,
    kInterfaceClientLanguage_en_US,

}InterfaceClientLanguage;

/**
 * @description: 回调函数，实时获得当前进度和总数
 * @param: ip 正在扫描的ip地址
 * @param: current 当前ip的序列号
 * @param: total 扫描的ip的总数
 */
typedef void (* ScanPortProc)(const char * ip, unsigned int current, unsigned int total);

/**
 * @description: 获得设备的ip列表(json格式)
 * @param: ip 手机端的IP地址
 * @param: mask 子网掩码
 * @param: port 端口号，(默认10091)
 * @param: outdata 返回json格式的ip列表
 * @param: outdatalen 返回ip列表长度
 * @param: spp 回调函数，实时获得当前进度和总数
 * @return: 操作成功返回true，否则返回false
 */
bool interface_ScanPort(const char *ip, const char *mask, unsigned int port, char *outdata, size_t *outdatalen, ScanPortProc spp,int i,map<int64_t,string> &i_str);


void interface_NetMessage(const void *inData, void **outData, int *outDataLen);

int CreateTx(const char* From, const char * To, const char * amt, const char *ip, uint32_t needVerifyPreHashCount, std::string minerFees);

int free_buf(char **buf);

void interface_testdevice_mac();
// net_pack packTmp;






#endif