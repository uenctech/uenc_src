#ifndef __CA_TRANSACTION__
#define __CA_TRANSACTION__

#include "ca_cstr.h"
#include "ca_base58.h"
#include "ca_global.h"
#include "Crypto_ECDSA.h"
#include "../include/cJSON.h"
#include "proto/block.pb.h"
#include "proto/transaction.pb.h"
#include "proto/ca_protomsg.pb.h"
#include "proto/interface.pb.h"
#include "net/msg_queue.h"

//获取md5头文件
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <map>
#include <memory>
#include "getmac.pb.h"
#include "../include/net_interface.h"
#ifdef __cplusplus
extern "C" {
#endif



void blk_print(struct blkdb *db);

typedef void (*RECVFUN)(const char *ip, const char *message, char **out, int *outlen);

//void Init();
void InitAccount(accountinfo *acc, const char *path);
void GetDefault58Addr(char *buf, size_t len);
// std::string ser_reqblkinfo(int64_t height);
unsigned get_extra_award_height(); //根据高度获取额外奖励

void new_add_ouput_by_signer(CTransaction &tx, bool isExtra = false, const std::shared_ptr<TxMsg>& msg = nullptr);


/**
    consensus: 共识数
    award_list: 节点奖励列表

**/
int getNodeAwardList(int consensus, std::vector<int> &award_list, int amount = 1000, float coe = 1);

uint64_t CheckBalanceFromRocksDb(const std::string & address);


/**
 * @description: 从数据库查询UTXO是否足够，并创建交易体
 * @param fromAddr   交易发起方钱包地址
 * @param toAddr     交易接收方钱包地址
 * @param amount 交易金额
 * @param needVerifyPreHashCount 共识数
 * @param minerFees  单笔签名手续费
 * @param outTx  出参，生成的交易体
 * @param isRedeem  解质押标志
 * @return {bool} 成功返回true，失败返回false
 */
bool FindUtxosFromRocksDb(const std::string & fromAddr, const std::string & toAddr, uint64_t amount, uint32_t needVerifyPreHashCount, uint64_t minerFees, CTransaction & outTx, std::string utxoStr = "");


bool VerifyTransactionSign(const CTransaction & tx, int & VerifyPreHashCount, std::string & txBlockHash, std::string txHash);
void CalcTransactionHash(CTransaction & tx);
int GetSignString(const std::string & message, std::string & signature, std::string & strPub);
std::string CalcBlockHeaderMerkle(const CBlock & cblock);
std::vector<std::string> randomNode(unsigned int n);

#ifdef __cplusplus
}
#endif

CBlock CreateBlock(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg = nullptr);
CTransaction CreateWorkTx(const CTransaction & tx, bool bIsAward = false, const std::shared_ptr<TxMsg>& msg = nullptr); //extra_award额外奖励 0否 1是

bool VerifyBlockHeader(const CBlock & cblock);
bool AddBlock(const CBlock & cblock, bool isSync = false);

typedef enum emTransactionType{
	kTransactionType_Unknown = -1,		// 未知
	kTransactionType_Tx = 0,			// 正常交易
	kTransactionType_Fee,			// 手续费交易
	kTransactionType_Award,	// 奖励交易
} TransactionType;
TransactionType CheckTransactionType(const CTransaction & tx);



/* ====================================================================================  
 # @description: 纠正数据块
 # @param height :  纠正起始块的高度
 # @return 成功返回0，失败返回小于0的数
 ==================================================================================== */
int RollbackToHeight(unsigned int height);


/* ====================================================================================  
 # @description: 将字符串以分割标识符分割
 # @param dst  : 分割完之后的数据，存放到数组
 # @param src  : 要分割的字符串 
 # @param separator : 分割符 
 # @return separator: 返回分割的数量
 ==================================================================================== */
int StringSplit(std::vector<std::string>& dst, const std::string& src, const std::string& separator);

/* ==================================================================================== 
 # @description: 退出守护者进程
 # @param 无
 # @return: 成功返回true, 失败返回false
 ==================================================================================== */
bool ExitGuardian();




/**
 * @description: 建块
 * @param recvTxString 交易的序列化字符串
 * @param SendTxMsg 接受到的交易体
 * @return: 成功返回0；失败返回小于0的值；
 */
int BuildBlock(std::string &recvTxString, const std::shared_ptr<TxMsg>& msg);


void  GetDeviceInfo(const std::shared_ptr<GetMacReq>& getMacReq, const MsgData& from);

int get_mac_info(std::vector<std::string> &vec);



/* ====================================================================================  
 # @description: 判断版本是否兼容
 # @param recvVersion 接收到的数据的版本
 # @return: 成功: 0;
 # 			失败: -1, 接收到的数据的版本低于最低兼容版本
 ==================================================================================== */
int IsVersionCompatible( std::string recvVersion );


/* ====================================================================================  
 # @description:  处理接收到的交易
 # @param  msg :  其他节点发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleTx( const std::shared_ptr<TxMsg>& msg, const MsgData& msgdata );


int DoHandleTx( const std::shared_ptr<TxMsg>& msg, const MsgData& msgdata, std::string & tx_hash);


/* ====================================================================================  
 # @description:  PC端处理手机交易体和签名信息
 # @param  msg :  手机端发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandlePreTxRaw( const std::shared_ptr<TxMsgReq>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description:  PC端接收处理手机端发起的交易
 # @param  msg :  手机端发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleCreateTxInfoReq( const std::shared_ptr<CreateTxMsgReq>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description: 手机端发起交易生成交易体
 # @param msg   手机端发送来的交易相关信息体
 # @param serTx 返回参数，返回生成的交易体
 # @return: 成功: 0;
 # 			失败: -1, 参数错误; 		-2, 查找UTXO失败;
 ==================================================================================== */
int CreateTransactionFromRocksDb( const std::shared_ptr<CreateTxMsgReq>& msg, std::string &serTx);


/* ====================================================================================  
 # @description:  建块广播处理接口
 # @param  msg :  其他节点发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleBuileBlockBroadcastMsg( const std::shared_ptr<BuileBlockBroadcastMsg>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description: 随机查找签名价格合适的节点作为下一个签名节点
 # @param tx  交易体
 # @param nextNodeNumber   需要获取的节点个数
 # @param nextNode   出参，下一个签名节点的集合
 # @return: 成功返回0；失败返回小于0的值。
 ==================================================================================== */
int FindSignNode(const CTransaction &tx, const int nextNodeNumber, std::vector<std::string> &nextNode);


void CalcBlockMerkle(CBlock & cblock);


/* ====================================================================================  
 # @description:  手机端连接矿机交易之前，验证矿机密码
 # @param  msg :  手机端发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleVerifyDevicePassword( const std::shared_ptr<VerifyDevicePasswordReq>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description:  手机端连接矿机发起交易
 # @param  msg :  手机端发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleCreateDeviceTxMsgReq( const std::shared_ptr<CreateDeviceTxMsgReq>& msg, const MsgData& msgdata );
//获取在线时长
void GetOnLineTime();
//打印显示在线时长
int PrintOnLineTime();
int TestSetOnLineTime();

void test_rocksdb();

/* ==================================================================================== 
 # @description:  查询账号质押资金
 # @param address 要查询的账号
 # @param pledgeamount 出参，质押的资金
 # @return 成功返回0
 ==================================================================================== */
int SearchPledge(const std::string &address, uint64_t &pledgeamount, std::string pledgeType = PLEDGE_NET_LICENCE);


bool IsNeedPackage(const CTransaction & tx);


bool IsNeedPackage(const std::vector<std::string> & fromAddr);


/* ==================================================================================== 
 # @description: 获取前100块内奖励值异常的账号
 # @param addrList 出参，异常账号列表
 # @return 成功返回0，失败返回负值
 ==================================================================================== */
int GetAbnormalAwardAddrList(std::vector<std::string> & addrList);



#endif
