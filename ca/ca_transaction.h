#ifndef __CA_TRANSACTION__
#define __CA_TRANSACTION__

#include "ca_cstr.h"
#include "ca_base58.h"
#include "ca_global.h"
#include "Crypto_ECDSA.h"
#include "proto/block.pb.h"
#include "proto/transaction.pb.h"
#include "proto/block.pb.h"
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



void blk_print(struct blkdb *db);

typedef void (*RECVFUN)(const char *ip, const char *message, char **out, int *outlen);

//void Init();
void InitAccount(accountinfo *acc, const char *path);

// 获得默认的base58地址
std::string GetDefault58Addr();
// std::string ser_reqblkinfo(int64_t height);
unsigned get_extra_award_height(); //根据高度获取额外奖励

int new_add_ouput_by_signer(CTransaction &tx, bool isExtra = false, const std::shared_ptr<TxMsg>& msg = nullptr);


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


void HandleGetMacReq(const std::shared_ptr<GetMacReq>& getMacReq, const MsgData& from);

int get_mac_info(std::vector<std::string> &vec);


/* ====================================================================================  
 # @description:  处理接收到的交易
 # @param  msg :  其他节点发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleTx( const std::shared_ptr<TxMsg>& msg, const MsgData& msgdata );


/**
 * @description: 交易处理
 * @param {const std::shared_ptr<TxMsg>& } msg 交易流转信息
 * @param {std::string &}  成功后返回的交易哈希
 * @return {int} 	0 成功
 * 					-1 版本不兼容
 * 					-2 高度不符合
 * 					-3 本节点无该交易的父区块哈希
 * 					-4 交易共识数小于最小共识数
 * 					-5 交易体检查错误
 * 					-6 打开数据库错误
 * 					-7 已签名节点中在线时长错误
 *                  -8 校验流转交易体失败
 * 					-9 验证签名错误（有失败后尝试重发机制）
 * 					-10 交易流转中已经对该交易签过名了（有失败后尝试重发机制）
 * 					-11 非质押交易非默认账户的交易质押金额不足（有失败后尝试重发机制）
 * 					-12 交易接收方不能签名（有失败后尝试重发机制）
 * 					-13 本节点未设置矿费（有失败后尝试重发机制）
 * 					-14 交易发起方所支付的手续费低于本节点设定的签名费（有失败后尝试重发机制）
 * 					-15 交易的签名费超出范围
 * 					-16 交易流转过程中，添加交易流转信息失败，如无足够签名节点等原因
 * 					-17 发送交易失败
 * 					-18 最后一个交易签名时，添加交易流转信息失败，如无足够签名节点等原因
 * 					-19 该交易已在本节点建块
 * 					-20 交易流转中的签名节点和共识数不符
 * 					-21 建块失败
 * 			
 */
int DoHandleTx( const std::shared_ptr<TxMsg>& msg, std::string & tx_hash);


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
 # @description:  交易挂起广播处理接口
 # @param  msg :  其他节点发送过来的消息体
 # @param  msgdata： 网络通信所必须的信息
 ==================================================================================== */
void HandleTxPendingBroadcastMsg(const std::shared_ptr<TxPendingBroadcastMsg>& msg, const MsgData& msgdata);


/**
 * @description: 随机查找签名价格合适的节点作为下一个签名节点
 * @param tx: 交易体
 * @param nextNodeNumber: 需要获取的节点个数
 * @param signedNodes: 需要排重的已签名节点
 * @param nextNodes: 返回签名节点
 * @return 成功返回0，失败返回如下
 */ 
int FindSignNode(const CTransaction & tx, const int nodeNumber, const std::vector<std::string> & signedNodes, std::vector<std::string> & nextNodes);


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


/* ==================================================================================== 
 # @description: 检查块是否存在
 # @param  blkHash  块hash
 # @return 存在返回0, 失败返回小于0的值
 ==================================================================================== */
int IsBlockExist(const std::string & blkHash);

/**
 * @description: 计算交易尝试次数
 * @param {int} needVerifyPreHashCount 共识数
 * @return {int} 返回共识数所对应的失败尝试次数
 */
int CalcTxTryCountDown(int needVerifyPreHashCount);

/**
 * @description: 根据交易流转信息获得交易失败次数
 * @param {const TxMsg &} 交易流转信息
 * @return {int} 交易失败次数
 */
int GetTxTryCountDwon(const TxMsg & txMsg);


/**
 * @description: 根据质押时间获得设备在线时长
 * @param {double_t &} 在线时长  
 * @return {int} 	0 正常返回时长
 * 					1 未质押，返回默认时长
 * 					-1 打开数据库错误
 * 					-2 获得交易信息失败
 */
int GetLocalDeviceOnlineTime(double_t & onlinetime);

/**
 * @description: 尝试转发到其他节点
 * @param {const CTransaction &} tx 交易  
 * @param {std::shared_ptr<TxMsg>&} msg 交易流转信息
 * @param {uint32_t} number
 * @return {int} 	0 转发正常
 * 					-1 未找到可转发的节点
 */
int SendTxMsg(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg, uint32_t number);

/**
 * @description: 重新尝试发送交易信息
 * @param {const CTransaction &} tx 交易信息体
 * @param {const std::shared_ptr<TxMsg>&} msg 交易流转信息
 * @return {int} 0 成功流转
 * 				-1 尝试失败
 */
int RetrySendTxMsg(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg);

/**
 * @description: 添加交易流转信息的节点签名信息
 * @param {const std::shared_ptr<TxMsg>&} 
 * @return {int} 0 成功
 * 				-1 打开数据库错误
 * 				-2 获得在线时长错误
 * 				-3 获得签名费错误
 */
int AddSignNodeMsg(const std::shared_ptr<TxMsg>& msg);



/* ==================================================================================== 
 # @description: 检查流转交易体合法性
 # @param msg 流转交易体
 # @return 0, 合法;   不合法返回小于0的值
 ==================================================================================== */
int CheckTxMsg( const std::shared_ptr<TxMsg>& msg );


/**
 * @description: 手机端连接矿机发起多重交易
 * @param {const std::shared_ptr<CreateDeviceMultiTxMsgReq>& } msg 手机端发送的消息体
 * @param {const MsgData& msgdata} MsgData 网络通信所必须的信息
 * @return {*} 无
 */
void HandleCreateDeviceMultiTxMsgReq(const std::shared_ptr<CreateDeviceMultiTxMsgReq>& msg, const MsgData& msgdata);



/**
 * @description: 手机端主网账户质押交易(第一步)
 * @param {const std::shared_ptr<CreatePledgeTxMsgReq>& } msg 手机端发送的消息体
 * @param {const MsgData &} msgdata 网络通信所必须的信息
 * @return {*}
 */
void HandleCreatePledgeTxMsgReq(const std::shared_ptr<CreatePledgeTxMsgReq>& msg, const MsgData &msgdata);
/**
 * @description: 手机端主网账户质押交易(第二步)
 * @param {const std::shared_ptr<PledgeTxMsgReq>& } msg 手机端发送的消息体
 * @param {const MsgData &} msgdata 网络通信所必须的信息
 * @return {*}
 */
void HandlePledgeTxMsgReq(const std::shared_ptr<PledgeTxMsgReq>& msg, const MsgData &msgdata);

/**
 * @description: 手机端主网账户发起解除质押(第一步)
 * @param {const std::shared_ptr<CreateRedeemTxMsgReq>& } msg 手机端发送的消息体
 * @param {const MsgData &} msgdata 网络通信所必须的信息
 * @return {*}
 */
void HandleCreateRedeemTxMsgReq(const std::shared_ptr<CreateRedeemTxMsgReq>& msg,const MsgData &msgdata);
/**
 * @description: 手机端主网账户发起解除质押(第二步)
 * @param {const std::shared_ptr<RedeemTxMsgReq>& } msg
 * @param {const MsgData &} msgdata 网络通信所必须的信息
 * @return {*}
 */
void HandleRedeemTxMsgReq(const std::shared_ptr<RedeemTxMsgReq>& msg, const MsgData &msgdata );


/** 
 * @description: 手机端连接矿机发起质押交易
 * @param msg    手机端发送的交易数据
 * @param phoneControlDevicePledgeTxAck  发送回手机端的回执
 */
void HandleCreateDevicePledgeTxMsgReq(const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, const MsgData &msgdata );

/**
 * @description: 手机端连接矿机发起解除质押交易
 * @param msg 手机端发送的交易数据
 * @param msgdata 发送回手机端的回执
 * @return 无
 */
void HandleCreateDeviceRedeemTxMsgReq(const std::shared_ptr<CreateDeviceRedeemTxReq> &msg, const MsgData &msgdata );


/**
 * @description: 质押资产 
 * @param fromAddr 质押资产的账号 
 * @param amount_str 质押资产的金额
 * @param needVerifyPreHashCount 共识数 
 * @param gasFeeStr 签名费  
 * @param password 矿机密码
 * @param msgdata 网络传输所必须的信息体,手机端交易时使用
 * @param pledgeType 质押类型
 * @return 成功返回0;
 *         	-1, 参数错误
 *  		-2, 密码不正确
 * 			-3, 有挂起交易	
 * 			-4, 打开数据库错误
 * 			-5, 未找到可用utxo	
 * 			-6, 设置地址错误
 * 			-7, 获得高度错误	
 * 			-8, 获得主链错误
 */
int CreatePledgeTransaction(const std::string & fromAddr,  
                            const std::string & amount_str, 
                            uint32_t needVerifyPreHashCount, 
                            std::string password,
                            std::string gasFeeStr, 
                            std::string pledgeType,
                            std::string & txHash);

/**
 * @description: 解质押资产
 * @param fromAddr 解质押资产的账号
 * @param needVerifyPreHashCount 共识数 
 * @param GasFeeStr 签名费
 * @param blockHeaderStr 要解质押的那笔交易所在块的块头
 * @param password 矿机密码
 * @param msgdata 网络传输所必须的信息体,手机端交易时使用
 * @return 成功返回0；
 * 				-1 参数不正确
 * 				-2 密码不正确
 * 				-3 有挂起交易
 * 				-4 打开数据库错误
 * 				-5 获得质押列表错误
 * 				-6 发起账户未质押
 * 				-7 获得utxo错误
 * 				-8 解质押交易不存在
 * 				-9 未到解除期限
 * 				-10 交易utxo不存在
 *				-11 设置账户错误
 * 				-12 获得高度错误
 * 				-13 获得主链错误
 */
int CreateRedeemTransaction(const std::string & fromAddr, 
                            uint32_t needVerifyPreHashCount, 
                            std::string GasFeeStr, 
                            std::string blockHeaderStr, 
                            std::string password,
                            std::string & txHash);


int IsMoreThan30DaysForRedeem(const std::string& utxo);

bool CheckAllNodeChainHeightInReasonableRange();


/* ==================================================================================== 
 # @description:  处理手机端发送的多重交易请求
 # @param msg     手机端发送的消息体
 # @param msgdata 网络通信所必须的信息
 ==================================================================================== */
void HandleCreateMultiTxReq( const std::shared_ptr<CreateMultiTxMsgReq>& msg, const MsgData& msgdata );


/* ==================================================================================== 
 # @description:  处理接受到的手机端交易
 # @param msg     手机端发送的消息体
 # @param msgdata 网络通信所必须的信息
 ==================================================================================== */
void HandleMultiTxReq( const std::shared_ptr<MultiTxMsgReq>& msg, const MsgData& msgdata );

void HandleConfirmTransactionReq(const std::shared_ptr<ConfirmTransactionReq>& msg, const MsgData& msgdata);

void HandleConfirmTransactionAck(const std::shared_ptr<ConfirmTransactionAck>& msg, const MsgData& msgdata);

void SendTransactionConfirm(const std::string& tx_hash, ConfirmCacheFlag flag, const uint32_t confirmCount = 100);

#endif
