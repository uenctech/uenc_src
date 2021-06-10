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

//Get md5 header file 
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

// Get the default base58 address 
std::string GetDefault58Addr();
// std::string ser_reqblkinfo(int64_t height);
unsigned get_extra_award_height(); //Get extra rewards based on height 

int new_add_ouput_by_signer(CTransaction &tx, bool isExtra = false, const std::shared_ptr<TxMsg>& msg = nullptr);


/**
    consensus: Consensus number 
    award_list: Node reward list 
**/
int getNodeAwardList(int consensus, std::vector<int> &award_list, int amount = 1000, float coe = 1);

uint64_t CheckBalanceFromRocksDb(const std::string & address);


/**
 * @description: Query whether UTXO is sufficient from the database and create a transaction body 
 * @param fromAddr   The wallet address of the transaction initiator 
 * @param toAddr     The wallet address of the transaction recipient 
 * @param amount Transaction amount 
 * @param needVerifyPreHashCount Consensus number 
 * @param minerFees  Single signature fee 
 * @param outTx  Out of the parameters, the generated transaction body 
 * @param isRedeem  Unpledged sign 
 * @return {bool} Return true on success, false on failure 
 */
bool FindUtxosFromRocksDb(const std::string & fromAddr, const std::string & toAddr, uint64_t amount, uint32_t needVerifyPreHashCount, uint64_t minerFees, CTransaction & outTx, std::string utxoStr = "");


bool VerifyTransactionSign(const CTransaction & tx, int & VerifyPreHashCount, std::string & txBlockHash, std::string txHash);
void CalcTransactionHash(CTransaction & tx);
int GetSignString(const std::string & message, std::string & signature, std::string & strPub);
std::string CalcBlockHeaderMerkle(const CBlock & cblock);
std::vector<std::string> randomNode(unsigned int n);


CBlock CreateBlock(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg = nullptr);
CTransaction CreateWorkTx(const CTransaction & tx, bool bIsAward = false, const std::shared_ptr<TxMsg>& msg = nullptr); //extra_award Extra reward 0No 1Yes 

bool VerifyBlockHeader(const CBlock & cblock);
bool AddBlock(const CBlock & cblock, bool isSync = false);

typedef enum emTransactionType{
	kTransactionType_Unknown = -1,		// unknown 
	kTransactionType_Tx = 0,			// Normal transaction 
	kTransactionType_Fee,			// Transaction fee 
	kTransactionType_Award,	// Reward transaction 
} TransactionType;

TransactionType CheckTransactionType(const CTransaction & tx);

/* ====================================================================================  
 # @description: Split string with split identifier 
 # @param dst  : The data after the split is stored in the array 
 # @param src  : The string to be split  
 # @param separator : Delimiter  
 # @return separator: Returns the number of divisions 
 ==================================================================================== */
int StringSplit(std::vector<std::string>& dst, const std::string& src, const std::string& separator);

/* ==================================================================================== 
 # @description: Exit the daemon process 
 # @param no 
 # @return: Return true on success, false on failure 
 ==================================================================================== */
bool ExitGuardian();

/**
 * @description: Building block 
 * @param recvTxString The serialized string of the transaction 
 * @param SendTxMsg Received transaction body 
 * @return: Return 0 on success; return a value less than 0 on failure ；
 */
int BuildBlock(std::string &recvTxString, const std::shared_ptr<TxMsg>& msg);


void HandleGetMacReq(const std::shared_ptr<GetMacReq>& getMacReq, const MsgData& from);

int get_mac_info(std::vector<std::string> &vec);


/* ====================================================================================  
 # @description:  Process the received transaction 
 # @param  msg :  Message body sent by other nodes 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandleTx( const std::shared_ptr<TxMsg>& msg, const MsgData& msgdata );


/**
 * @description: Transaction processing 
 * @param {const std::shared_ptr<TxMsg>& } msg Transaction flow information 
 * @param {std::string &}  Transaction hash returned after success 
 * @return {int} 	0 success
 * 					-1 Incompatible version 
 * 					-2 Height does not meet 
 * 					-3 This node does not have the parent block hash of the transaction 
 * 					-4 The transaction consensus number is less than the minimum consensus number 
 * 					-5 Transaction body check error 
 * 					-6 Open database error 
 * 					-7 The online duration of the signed node is incorrect 
 *                  -8 Failed to verify the circulation transaction body 
 * 					-9 Verify the signature error (there is a mechanism to try to resend after failure) 
 * 					-10 The transaction has been signed during the transaction (there is a mechanism to try to resend after failure) 
 * 					-11 Insufficient amount of transaction pledge deposit of non-default account for non-pledged transactions (there is a mechanism to retry after failure) 
 * 					-12 The recipient of the transaction cannot sign (there is a mechanism to try to resend after failure) 
 * 					-13 There is no mining fee set on this node (there is a mechanism to try to resend after failure) 
 * 					-14 The transaction fee paid by the initiator of the transaction is lower than the signature fee set by this node (there is a mechanism to try to resend after failure) 
 * 					-15 The transaction signature fee is out of range 
 * 					-16 During the transaction flow process, adding transaction flow information failed due to reasons such as insufficient signature nodes, etc. 
 * 					-17 Failed to send transaction 
 * 					-18 Failed to add transaction flow information when signing the last transaction, for example, there are not enough signature nodes, etc. 
 * 					-19 The transaction has been built on this node 
 * 					-20 The signature node in the transaction flow does not match the consensus number 
 * 					-21 Failed to build block 
 * 			
 */
int DoHandleTx( const std::shared_ptr<TxMsg>& msg, std::string & tx_hash);


/* ====================================================================================  
 # @description:  PC terminal processing mobile phone transaction body and signature information 
 # @param  msg :  The body of the message sent from the mobile phone 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandlePreTxRaw( const std::shared_ptr<TxMsgReq>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description:  The PC terminal receives and processes the transaction initiated by the mobile terminal 
 # @param  msg :  The body of the message sent from the mobile phone 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandleCreateTxInfoReq( const std::shared_ptr<CreateTxMsgReq>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description: Initiate a transaction on the mobile phone to generate a transaction body 
 # @param msg   Transaction-related information sent from the mobile phone 
 # @param serTx Return parameters, return the generated transaction body 
 # @return: success : 0;
 # 			fail: -1, Parameter error ; 		-2, Failed to find UTXO ;
 ==================================================================================== */
int CreateTransactionFromRocksDb( const std::shared_ptr<CreateTxMsgReq>& msg, std::string &serTx);


/* ====================================================================================  
 # @description:  Building block broadcast processing interface 
 # @param  msg :  Message body sent by other nodes 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandleBuileBlockBroadcastMsg( const std::shared_ptr<BuileBlockBroadcastMsg>& msg, const MsgData& msgdata );

/* ====================================================================================  
 # @description:  Transaction pending broadcast processing interface 
 # @param  msg :  Message body sent by other nodes 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandleTxPendingBroadcastMsg(const std::shared_ptr<TxPendingBroadcastMsg>& msg, const MsgData& msgdata);


/**
 * @description: Randomly find the node with the right signature price as the next signature node 
 * @param tx: Transaction body 
 * @param nextNodeNumber: The number of nodes that need to be obtained 
 * @param signedNodes: Signed nodes that need to be deduplicated 
 * @param nextNodes: Return signature node 
 * @return Success returns 0 ，The failure return is as follows 
 */ 
int FindSignNode(const CTransaction & tx, const int nodeNumber, const std::vector<std::string> & signedNodes, std::vector<std::string> & nextNodes);


void CalcBlockMerkle(CBlock & cblock);


/* ====================================================================================  
 # @description:  Verify the password of the mining machine before connecting to the mining machine on the mobile phone 
 # @param  msg :  The body of the message sent from the mobile phone 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandleVerifyDevicePassword( const std::shared_ptr<VerifyDevicePasswordReq>& msg, const MsgData& msgdata );


/* ====================================================================================  
 # @description:  Connect the mobile phone to the miner to initiate a transaction 
 # @param  msg :  The body of the message sent from the mobile phone 
 # @param  msgdata： Information necessary for network communication 
 ==================================================================================== */
void HandleCreateDeviceTxMsgReq( const std::shared_ptr<CreateDeviceTxMsgReq>& msg, const MsgData& msgdata );
//Get online time 
void GetOnLineTime();
//Print display online time 
int PrintOnLineTime();
int TestSetOnLineTime();

void test_rocksdb();

/* ==================================================================================== 
 # @description:  Query account pledge funds 
 # @param address The account to be queried 
 # @param pledgeamount Participation, pledged funds 
 # @return Success returns 0 
 ==================================================================================== */
int SearchPledge(const std::string &address, uint64_t &pledgeamount, std::string pledgeType = PLEDGE_NET_LICENCE);


bool IsNeedPackage(const CTransaction & tx);


bool IsNeedPackage(const std::vector<std::string> & fromAddr);


/* ==================================================================================== 
 # @description: Get accounts with abnormal rewards in the first 100 blocks 
 # @param addrList Parameter out, list of abnormal accounts 
 # @return Success returns 0 ，Return a negative value on failure 
 ==================================================================================== */
int GetAbnormalAwardAddrList(std::vector<std::string> & addrList);


/* ==================================================================================== 
 # @description: Check if the block exists 
 # @param  blkHash  Block hash 
 # @return Existence returns 0, failure returns a value less than 0 
 ==================================================================================== */
int IsBlockExist(const std::string & blkHash);

/**
 * @description: Count the number of transaction attempts 
 * @param {int} needVerifyPreHashCount Consensus number 
 * @return {int} Return the number of failed attempts corresponding to the consensus number 
 */
int CalcTxTryCountDown(int needVerifyPreHashCount);

/**
 * @description: Obtain the number of failed transactions based on transaction flow information 
 * @param {const TxMsg &} Transaction flow information 
 * @return {int} Number of failed transactions 
 */
int GetTxTryCountDwon(const TxMsg & txMsg);


/**
 * @description: Obtain the online duration of the device according to the pledge time 
 * @param {double_t &} Online Time  
 * @return {int} 	0 Normal return time 
 * 					1 Not pledged, return to default duration 
 * 					-1 Open database error 
 * 					-2 Failed to obtain transaction information 
 */
int GetLocalDeviceOnlineTime(double_t & onlinetime);

/**
 * @description: Try to forward to other nodes 
 * @param {const CTransaction &} tx transaction 
 * @param {std::shared_ptr<TxMsg>&} msg Transaction flow information 
 * @param {uint32_t} number
 * @return {int} 	0 Forwarding is normal 
 * 					-1 No forwardable node found 
 */
int SendTxMsg(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg, uint32_t number);

/**
 * @description: Retry sending transaction information 
 * @param {const CTransaction &} tx Transaction information body 
 * @param {const std::shared_ptr<TxMsg>&} msg Transaction flow information 
 * @return {int} 0 Successful circulation
 * 				-1 failed attempt 
 */
int RetrySendTxMsg(const CTransaction & tx, const std::shared_ptr<TxMsg>& msg);

/**
 * @description: Add node signature information for transaction flow information 
 * @param {const std::shared_ptr<TxMsg>&} 
 * @return {int} 0 success 
 * 				-1 Open database error 
 * 				-2 Get online time error 
 * 				-3 Error in getting the signature fee 
 */
int AddSignNodeMsg(const std::shared_ptr<TxMsg>& msg);



/* ==================================================================================== 
 # @description: Check the legality of the circulating transaction body 
 # @param msg Circulation transaction body 
 # @return 0, Legal; illegal returns a value less than 0 
 ==================================================================================== */
int CheckTxMsg( const std::shared_ptr<TxMsg>& msg );


/**
 * @description: Connect the mobile phone to the miner to initiate multiple transactions 
 * @param {const std::shared_ptr<CreateDeviceMultiTxMsgReq>& } msg Message body sent by mobile phone 
 * @param {const MsgData& msgdata} MsgData Information necessary for network communication 
 * @return {*} no 
 */
void HandleCreateDeviceMultiTxMsgReq(const std::shared_ptr<CreateDeviceMultiTxMsgReq>& msg, const MsgData& msgdata);



/**
 * @description: Pledge transaction of mainnet account on mobile terminal (first step) 
 * @param {const std::shared_ptr<CreatePledgeTxMsgReq>& } msg Message body sent by mobile phone 
 * @param {const MsgData &} msgdata Information necessary for network communication 
 * @return {*}
 */
void HandleCreatePledgeTxMsgReq(const std::shared_ptr<CreatePledgeTxMsgReq>& msg, const MsgData &msgdata);
/**
 * @description: Pledge transaction of mainnet account on mobile terminal (second step) 
 * @param {const std::shared_ptr<PledgeTxMsgReq>& } msg Message body sent by mobile phone 
 * @param {const MsgData &} msgdata Information necessary for network communication 
 * @return {*}
 */
void HandlePledgeTxMsgReq(const std::shared_ptr<PledgeTxMsgReq>& msg, const MsgData &msgdata);

/**
 * @description: Initiation of de-staking of mobile mainnet account (first step) 
 * @param {const std::shared_ptr<CreateRedeemTxMsgReq>& } msg Message body sent by mobile phone 
 * @param {const MsgData &} msgdata Information necessary for network communication 
 * @return {*}
 */
void HandleCreateRedeemTxMsgReq(const std::shared_ptr<CreateRedeemTxMsgReq>& msg,const MsgData &msgdata);
/**
 * @description: The mobile phone mainnet account initiates the release of pledge (the second step) 
 * @param {const std::shared_ptr<RedeemTxMsgReq>& } msg
 * @param {const MsgData &} msgdata Information necessary for network communication 
 * @return {*}
 */
void HandleRedeemTxMsgReq(const std::shared_ptr<RedeemTxMsgReq>& msg, const MsgData &msgdata );


/** 
 * @description: The mobile terminal connects to the miner to initiate a pledge transaction 
 * @param msg    Transaction data sent by mobile phone 
 * @param phoneControlDevicePledgeTxAck  Receipt sent back to the mobile phone 
 */
void HandleCreateDevicePledgeTxMsgReq(const std::shared_ptr<CreateDevicePledgeTxMsgReq>& msg, const MsgData &msgdata );

/**
 * @description: The mobile terminal connects to the miner to initiate the unstake transaction 
 * @param msg Transaction data sent by mobile phone 
 * @param msgdata Receipt sent back to the mobile phone 
 * @return no 
 */
void HandleCreateDeviceRedeemTxMsgReq(const std::shared_ptr<CreateDeviceRedeemTxReq> &msg, const MsgData &msgdata );


/**
 * @description: Pledged assets 
 * @param fromAddr Account of pledged assets 
 * @param amount_str The amount of pledged assets 
 * @param needVerifyPreHashCount Consensus number  
 * @param gasFeeStr Signature fee   
 * @param password Mining machine password 
 * @param msgdata Information body necessary for network transmission, used in mobile phone transactions 
 * @param pledgeType Pledge type 
 * @return Success returns 0 ;
 *         	-1, Parameter error 
 *  		-2, Incorrect password 
 * 			-3, There are pending transactions 	
 * 			-4, Open database error 
 * 			-5, No available utxo found 
 * 			-6, Set address error 
 * 			-7, Get height error 	
 * 			-8, Get the main chain error 
 */
int CreatePledgeTransaction(const std::string & fromAddr,  
                            const std::string & amount_str, 
                            uint32_t needVerifyPreHashCount, 
                            std::string password,
                            std::string gasFeeStr, 
                            std::string pledgeType,
                            std::string & txHash);

/**
 * @description: Depledge assets 
 * @param fromAddr The account of the pledged assets 
 * @param needVerifyPreHashCount Consensus number  
 * @param GasFeeStr Signature fee 
 * @param blockHeaderStr The exchange to be pledged is in the block head 
 * @param password Mining machine password 
 * @param msgdata Information body necessary for network transmission, used in mobile phone transactions 
 * @return Success returns 0 ；
 * 				-1 The parameter is incorrect 
 * 				-2 Incorrect password 
 * 				-3 There are pending transactions 
 * 				-4 Open database error 
 * 				-5 Error obtaining pledge list 
 * 				-6 Originating account is not pledged 
 * 				-7 Get utxo error 
 * 				-8 Depledge transaction does not exist 
 * 				-9 The cancellation deadline has not expired 
 * 				-10 Transaction utxo does not exist 
 *				-11 Setting up account error 
 * 				-12 Get height error 
 * 				-13 Get the main chain error 
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
 # @description:  Handle multiple transaction requests sent from the mobile phone 
 # @param msg     Message body sent by mobile phone 
 # @param msgdata Information necessary for network communication 
 ==================================================================================== */
void HandleCreateMultiTxReq( const std::shared_ptr<CreateMultiTxMsgReq>& msg, const MsgData& msgdata );


/* ==================================================================================== 
 # @description:  Process received mobile phone transactions 
 # @param msg     Message body sent by mobile phone 
 # @param msgdata Information necessary for network communication 
 ==================================================================================== */
void HandleMultiTxReq( const std::shared_ptr<MultiTxMsgReq>& msg, const MsgData& msgdata );

void HandleConfirmTransactionReq(const std::shared_ptr<ConfirmTransactionReq>& msg, const MsgData& msgdata);

void HandleConfirmTransactionAck(const std::shared_ptr<ConfirmTransactionAck>& msg, const MsgData& msgdata);

void SendTransactionConfirm(const std::string& tx_hash, ConfirmCacheFlag flag, const uint32_t confirmCount = 100);

#endif
