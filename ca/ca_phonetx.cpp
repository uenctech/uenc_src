#include "ca_phonetx.h"
#include "ca_global.h"
#include "ca_base64.h"
#include "ca_rocksdb.h"
#include "ca_txhelper.h"
#include "ca_message.h"
#include "ca_transaction.h"
#include "MagicSingleton.h"
#include "ca_device.h"

#include "../net/msg_queue.h"
#include "../proto/ca_protomsg.pb.h"
#include "../proto/transaction.pb.h"
#include "../proto/interface.pb.h"
#include "../include/ScopeGuard.h"

int GetAddrsFromMsg( const std::shared_ptr<CreateMultiTxMsgReq>& msg, 
                     std::vector<std::string> &fromAddr,
                     std::map<std::string, int64_t> &toAddr)
{
    for (int i = 0; i < msg->from_size(); ++i)
    {
        std::string fromAddrStr = msg->from(i);
        fromAddr.push_back(fromAddrStr);
    }

    for (int i = 0; i < msg->to_size(); ++i)
    {
        ToAddr toAddrInfo = msg->to(i);
        if (toAddrInfo.toaddr().empty() && toAddrInfo.amt().empty())
        {
            error("parse error! toaddr or amt is empty!");
            return -1;
        }

        int64_t amount = std::stod(toAddrInfo.amt()) * DECIMAL_NUM;
        toAddr.insert( make_pair(toAddrInfo.toaddr(), amount ) );
    }

    if (fromAddr.size() == 0 || toAddr.size() == 0)
    {
        return -2;
    }

    if (toAddr.size() != (size_t)msg->to_size())
    {
        return -3;
    }

    return 0;
}

void HandleCreateMultiTxReq( const std::shared_ptr<CreateMultiTxMsgReq>& msg, const MsgData& msgdata )
{
    // 手机端回执消息体
    CreateMultiTxMsgAck createMultiTxMsgAck;
    createMultiTxMsgAck.set_version(getVersion());

    // 判断版本是否兼容
    if( 0 != IsVersionCompatible( msg->version() ) )
	{
		createMultiTxMsgAck.set_code(PHONE_TX_VERSION_ERROR);
		createMultiTxMsgAck.set_message("Version incompatible!");
		net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
		error("HandleCreateMultiTxReq: IsVersionCompatible error!!!\n");
		return ;
	}

    if (msg->from_size() > 1 && msg->to_size() > 1)
    {
        createMultiTxMsgAck.set_code(PHONE_TX_PARAM_ERROR);
		createMultiTxMsgAck.set_message("many to many tx!");
		net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
		error("HandleCreateMultiTxReq: many to many tx!!!\n");
		return ;
    }

    CTransaction outTx;
    std::vector<std::string> fromAddr;
    std::map<std::string, int64_t> toAddr;

    int ret = GetAddrsFromMsg(msg, fromAddr, toAddr);
    if (0 != ret)
    {
        int err = PHONE_TX_CREATE_ERROR;
        if (ret == -1)
        {
            err = PHONE_TX_PARAM_ERROR;
        }
        else if (ret == -2 || ret == -3)
        {
            err = PHONE_TX_ADDR_ERROR;
        }
        
        createMultiTxMsgAck.set_code(err);
        createMultiTxMsgAck.set_message("param error!");
        net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
		error("HandleCreateMultiTxReq: GetAddrsFromMsg error!!!\n");
        return ;
    }

    uint64_t minerFees = stod( msg->minerfees() ) * DECIMAL_NUM;
    uint32_t needVerifyPreHashCount = stoi( msg->needverifyprehashcount() );
    
    ret = TxHelper::CreateTxMessage(fromAddr, toAddr, needVerifyPreHashCount, minerFees, outTx);
    if(ret != 0)
	{
        int err = PHONE_TX_CREATE_ERROR;
        if (ret == -1)
        {
            err = PHONE_TX_PARAM_ERROR;
        }
        else if (ret == -2)
        {
            err = PHONE_TX_ADDR_ERROR;
        }
        else if (ret == -3 || ret == -4)
        {
            err = PHONE_TX_TRANSACTION_ERROR;
        }
        else if (ret == -5)
        {
            err = PHONE_TX_NO_AMOUNT;
        }
        
        createMultiTxMsgAck.set_code(err);
        createMultiTxMsgAck.set_message("CreateTxMessage error!");
        net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
		error("HandleCreateMultiTxReq: TxHelper::CreateTxMessage error!!!\n");
		return;
	}

    for(int i = 0;i <outTx.vin_size();i++)
    {
        CTxin *txin = outTx.mutable_vin(i);
        txin->clear_scriptsig();
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        createMultiTxMsgAck.set_code(PHONE_TX_TXN_ERROR);
        createMultiTxMsgAck.set_message("TransactionInit error!");
        net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
        error("(HandleCreateMultiTxReq) TransactionInit failed !");
        return;
    }

    ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};

    uint64_t packageFee = 0;
    if ( 0 != pRocksDb->GetDevicePackageFee(packageFee) )
    {
        createMultiTxMsgAck.set_code(PHONE_TX_GET_PACK_FEE_ERROR);
        createMultiTxMsgAck.set_message("GetDevicePackageFee error!");
        net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
        error("(HandleCreateMultiTxReq) GetDevicePackageFee failed !");
        return;
    }

    nlohmann::json extra;
    extra["TransactionType"] = TXTYPE_TX;
	extra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
	extra["SignFee"] = minerFees;
    extra["PackageFee"] = packageFee;   // 本节点代发交易需要打包费
	outTx.set_extra(extra.dump());

    std::string serTx = outTx.SerializeAsString();

	size_t encodeLen = serTx.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)serTx.data(), serTx.size(), encode);
	std::string encodeStr( (char *)encode, codeLen );

	std::string encodeStrHash = getsha256hash(encodeStr);

    createMultiTxMsgAck.set_txdata(encodeStr);
    createMultiTxMsgAck.set_txencodehash(encodeStrHash);
    createMultiTxMsgAck.set_code(PHONE_TX_SUCCESS);
    createMultiTxMsgAck.set_message("success!");

    net_send_message<CreateMultiTxMsgAck>(msgdata, createMultiTxMsgAck);
}


int GetPubByVin(const std::vector<std::string> &address, const CTxin &txin, std::string &pubStr)
{
    if (address.size() == 0)
    {
        error("GetPubByVin : param error!");
        return -1;
    }

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if ( txn == NULL )
	{
        error("GetPubByVin : TransactionInit error!");
		return -2;
	}

    for (auto addr : address)
    {
        std::vector<std::string> utxoHashs;
        if ( 0 != pRocksDb->GetUtxoHashsByAddress(txn, addr, utxoHashs) )
        {
            error("GetPubByVin : GetUtxoHashsByAddress error!");
            return -2;
        }

        CTxprevout txprevout = txin.prevout();
        if (utxoHashs.end() != find(utxoHashs.begin(), utxoHashs.end(), txprevout.hash() ) )
        {
            pubStr = addr;
        }
    }

    if (pubStr.size() <= 0)
    {
        return -3;
    }

    return 0;
}

void HandlePreMultiTxRaw( const std::shared_ptr<MultiTxMsgReq>& msg, const MsgData& msgdata )
{
    TxMsgAck txMsgAck;
    txMsgAck.set_version(getVersion());

    if( 0 != IsVersionCompatible( msg->version() ) )
	{
		txMsgAck.set_code(PHONE_TX_VERSION_ERROR);
		txMsgAck.set_message("Version incompatible!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck);
		error("HandlePreMultiTxRaw: IsVersionCompatible error!!!\n");
		return ;
	}

    unsigned char serTxCstr[msg->sertx().size()] = {0};
	unsigned long serTxCstrLen = base64_decode((unsigned char *)msg->sertx().data(), msg->sertx().size(), serTxCstr);
	std::string serTxStr((char *)serTxCstr, serTxCstrLen);

    CTransaction tx;
    tx.ParseFromString(serTxStr);

    std::vector<SignInfo> vSignInfo;
    for (int i = 0; i < msg->signinfo_size(); ++i)
    {
        SignInfo signInfo = msg->signinfo(i);
        vSignInfo.push_back(signInfo);
    }

    if (vSignInfo.size() <= 0)
    {
        txMsgAck.set_code(PHONE_TX_PARAM_ERROR);
		txMsgAck.set_message("param error!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck);
		error("HandlePreMultiTxRaw: param error!\n");
    }

    // std::vector<std::string> address = TxHelper::GetTxOwner(tx);

    // 一对多交易只有一个发起方，取第0个
    SignInfo signInfo = msg->signinfo(0);
    unsigned char strsignatureCstr[signInfo.signstr().size()] = {0};
	unsigned long strsignatureCstrLen = base64_decode((unsigned char *)signInfo.signstr().data(), signInfo.signstr().size(), strsignatureCstr);

	unsigned char strpubCstr[signInfo.pubstr().size()] = {0};
	unsigned long strpubCstrLen = base64_decode((unsigned char *)signInfo.pubstr().data(), signInfo.pubstr().size(), strpubCstr);

    for (int i = 0; i < tx.vin_size(); ++i)
    {
        CTxin * txin = tx.mutable_vin(i);

        txin->mutable_scriptsig()->set_sign( strsignatureCstr, strsignatureCstrLen );
        txin->mutable_scriptsig()->set_pub( strpubCstr, strpubCstrLen );
    }

    auto extra = nlohmann::json::parse(tx.extra());
    int needVerifyPreHashCount = extra["NeedVerifyPreHashCount"].get<int>();

    std::string serTx = tx.SerializeAsString();
    // TX的头部带有签名过的网络节点的id，格式为 num [id,id,...]
	cstring *txstr = txstr_append_signid(serTx.c_str(), serTx.size(), needVerifyPreHashCount );
	std::string txstrtmp(txstr->str, txstr->len);

    TxMsg phoneToTxMsg;
    phoneToTxMsg.set_version(getVersion());

	phoneToTxMsg.set_tx(txstrtmp);
	phoneToTxMsg.set_txencodehash(msg->txencodehash());

	auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(HandlePreMultiTxRaw) TransactionInit failed !" << std::endl;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	phoneToTxMsg.set_top(top);

	auto txmsg = make_shared<TxMsg>(phoneToTxMsg);
	HandleTx( txmsg, msgdata );
}

void HandleCreateDeviceMultiTxMsgReq(const std::shared_ptr<CreateDeviceMultiTxMsgReq>& msg, const MsgData& msgdata)
{
    TxMsgAck txMsgAck;
    txMsgAck.set_version(getVersion());

    if( 0 != IsVersionCompatible( msg->version() ) )
	{
		txMsgAck.set_code(PHONE_TX_VERSION_ERROR);
		txMsgAck.set_message("Version incompatible!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck);
		error("HandleCreateDeviceMultiTxMsgReq: IsVersionCompatible error!!!\n");
		return ;
	}

    // 判断矿机密码是否正确
    std::string password = msg->password();
    std::string hashOriPass = generateDeviceHashPassword(password);
    std::string targetPassword = Singleton<Config>::get_instance()->GetDevPassword();
    if (hashOriPass != targetPassword) 
    {
        txMsgAck.set_code(PHONE_TX_PASSWORD_ERROR);
        txMsgAck.set_message("password error!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck);
        error("password error!");
        return;
    }

    if (msg->from_size() > 1 && msg->to_size() > 1)
    {
        txMsgAck.set_code(PHONE_TX_PARAM_ERROR);
		txMsgAck.set_message("many to many tx!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck);
		error("HandleCreateDeviceMultiTxMsgReq: many to many tx!\n");
		return ;
    }

    std::vector<std::string> fromAddr;
    std::map<std::string, int64_t> toAddr;

    for (int i = 0; i < msg->from_size(); ++i)
    {
        std::string fromAddrStr = msg->from(i);
        fromAddr.push_back(fromAddrStr);
    }

    for (int i = 0; i < msg->to_size(); ++i)
    {
        ToAddr toAddrInfo = msg->to(i);
        int64_t amount = std::stod(toAddrInfo.amt()) * DECIMAL_NUM;
        toAddr.insert( make_pair(toAddrInfo.toaddr(), amount ) );
    }

    if(toAddr.size() != (size_t)msg->to_size())
    {
        txMsgAck.set_code(PHONE_TX_ADDR_ERROR);
		txMsgAck.set_message("Addr repeat!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck);
		error("HandleCreateDeviceMultiTxMsgReq: CreateTxMessage failed!\n");
        return;
    }

    uint64_t gasFee = std::stod( msg->gasfees() ) * DECIMAL_NUM;
    uint32_t needVerifyPreHashCount = stoi( msg->needverifyprehashcount() );
    
    CTransaction outTx;
    int ret = TxHelper::CreateTxMessage(fromAddr,toAddr, needVerifyPreHashCount, gasFee, outTx);
	if(ret != 0)
	{
        int err = PHONE_TX_CREATE_ERROR;
        if (ret == -1)
        {
            err = PHONE_TX_PARAM_ERROR;
        }
        else if (ret == -2)
        {
            err = PHONE_TX_ADDR_ERROR;
        }
        else if (ret == -3 || ret == -4)
        {
            err = PHONE_TX_TRANSACTION_ERROR;
        }
        else if (ret == -5)
        {
            err = PHONE_TX_NO_AMOUNT;
        }

        txMsgAck.set_code(err);
		txMsgAck.set_message("CreateTxMessage failed!");
		net_send_message<TxMsgAck>(msgdata, txMsgAck);
		error("HandleCreateDeviceMultiTxMsgReq: CreateTxMessage failed!\n");
		return ;
	}

    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
    Transaction* txn = pRocksDb->TransactionInit();
    if (txn == NULL)
    {
        txMsgAck.set_code(PHONE_TX_TXN_ERROR);
        txMsgAck.set_message("TransactionInit error!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck);
        error("(HandleCreateDeviceMultiTxMsgReq) TransactionInit failed !");
        return;
    }

    ON_SCOPE_EXIT
    {
		pRocksDb->TransactionDelete(txn, false);
	};

    uint64_t packageFee = 0;
    if ( 0 != pRocksDb->GetDevicePackageFee(packageFee) )
    {
        txMsgAck.set_code(PHONE_TX_GET_PACK_FEE_ERROR);
        txMsgAck.set_message("GetDevicePackageFee error!");
        net_send_message<TxMsgAck>(msgdata, txMsgAck);
        error("(HandleCreateMultiTxReq) GetDevicePackageFee failed !");
        return ;
    }

    nlohmann::json txExtra = nlohmann::json::parse(outTx.extra());
    txExtra["TransactionType"] = TXTYPE_TX;	
    txExtra["NeedVerifyPreHashCount"] = needVerifyPreHashCount;
	txExtra["SignFee"] = gasFee;
    txExtra["PackageFee"] = packageFee;   // 本节点代发交易需要打包费

    outTx.set_extra(txExtra.dump());

    std::vector<std::string> addrs;
	for (int i = 0; i < outTx.vin_size(); i++)
	{
		CTxin * txin = outTx.mutable_vin(i);;
		std::string pub = txin->mutable_scriptsig()->pub();
		txin->clear_scriptsig();
		addrs.push_back(GetBase58Addr(pub));
	}

    std::string serTx = outTx.SerializeAsString();

    size_t encodeLen = serTx.size() * 2 + 1;
	unsigned char encode[encodeLen] = {0};
	memset(encode, 0, encodeLen);
	long codeLen = base64_encode((unsigned char *)serTx.data(), serTx.size(), encode);
	std::string encodeStr( (char *)encode, codeLen );

	std::string encodeStrHash = getsha256hash(encodeStr);

    //签名
	for (int i = 0; i < outTx.vin_size(); i++)
	{
		std::string addr = addrs[i];
		// std::cout << "DoCreateTx:签名:" << addr << std::endl;
		std::string signature;
		std::string strPub;
		g_AccountInfo.Sign(addr.c_str(), encodeStrHash, signature);
		g_AccountInfo.GetPubKeyStr(addr.c_str(), strPub);
		CTxin * txin = outTx.mutable_vin(i);
		txin->mutable_scriptsig()->set_sign(signature);
		txin->mutable_scriptsig()->set_pub(strPub);
	}

    serTx = outTx.SerializeAsString();
	// TX的头部带有签名过的网络节点的id，格式为 num [id,id,...]
	cstring *txstr = txstr_append_signid(serTx.c_str(), serTx.size(), needVerifyPreHashCount );
	std::string txstrtmp(txstr->str, txstr->len);

	TxMsg txMsg;
    txMsg.set_version(getVersion());

    txMsg.set_tx( txstrtmp );
	txMsg.set_txencodehash( encodeStrHash );
    
	unsigned int top = 0;
	pRocksDb->GetBlockTop(txn, top);	
	txMsg.set_top(top);

    auto msgTmp = make_shared<TxMsg>(txMsg);
    HandleTx( msgTmp, msgdata );

    cstr_free(txstr, true);
}