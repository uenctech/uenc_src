#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdarg.h>
#include <array>
#include <fcntl.h>
#include <unistd.h>
#include "ca_test.h"
#include "ca_transaction.h"
#include "ca_buffer.h"
#include "ca_interface.h"
#include "ca_console.h"
#include "ca_hexcode.h"
#include "ca_serialize.h"

#include "proto/block.pb.h"
#include "proto/transaction.pb.h"
#include "ca_rocksdb.h"
#include "MagicSingleton.h"
#include "../include/logging.h"
#include "../include/ScopeGuard.h"

void time_format_info(char * ps, uint64_t time, bool ms, CaTestFormatType type)
{
	if (ps == nullptr)
	{
		return;
	}
	
	time_t s = (time_t)(ms ? (time / 1000000) : time);
    struct tm * gm_date;
    gm_date = localtime(&s);
	ca_console tmColor(kConsoleColor_White, kConsoleColor_Black, true);

	if(type == kCaTestFormatType_Print)
	{
    	sprintf(ps, "%s%d-%d-%d %d:%d:%d (%zu) %s\n", tmColor.color().c_str(), gm_date->tm_year + 1900, gm_date->tm_mon + 1, gm_date->tm_mday, gm_date->tm_hour, gm_date->tm_min, gm_date->tm_sec, time, tmColor.reset().c_str());
	}
	else if (type == kCaTestFormatType_File)
	{
		sprintf(ps, "%d-%d-%d %d:%d:%d (%zu) \n", gm_date->tm_year + 1900, gm_date->tm_mon + 1, gm_date->tm_mday, gm_date->tm_hour, gm_date->tm_min, gm_date->tm_sec, time);
    }
}

void blkprint(CaTestFormatType type, int fd, const char *format, ...)
{
    auto len = strlen(format) + 1024 * 32;
    auto buffer = new char[len]{0};
	va_list valist;

	va_start(valist, format);

	vsnprintf(buffer, len, format, valist);

	if (type == kCaTestFormatType_Print) 
	{
		printf( "%s", buffer);
	}
	else if (type == kCaTestFormatType_File)
 	{
		if(fd < 0)
		{
            close(fd);
			return;
		}

		write(fd, buffer, strlen(buffer));
	}

	va_end(valist);
    delete[] buffer;
}

void printRocksdb(uint64_t start, uint64_t end) {
    int fd = -1;
    if (end - start > 5) {
        fd = open("print_blk.txt", O_CREAT | O_WRONLY);
    }
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
	if( txn == NULL )
	{
		std::cout << "(printRocksdb) TransactionInit failed !" << std::endl;
		return ;
	}

	bool bRollback = true;
	ON_SCOPE_EXIT{
		pRocksDb->TransactionDelete(txn, bRollback);
	};

    CaTestFormatType formatType;
    if (fd == -1) {
        formatType = kCaTestFormatType_Print;
    } else {
        formatType = kCaTestFormatType_File;
    }

    uint64_t height = 0;
    ca_console bkColor(kConsoleColor_Blue, kConsoleColor_Black, true);
    size_t b_count {0};//交易总数
    for (auto i = end; i >= start; --i) {
        height = i;

        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, height, vBlockHashs);
        for (auto hash : vBlockHashs) 
        {
            ++b_count;
			blkprint(formatType, fd, "\nBlockInfo ---------------------- > height [%ld]\n", height);
            CBlockHeader block;
            block.set_hash(hash);
            block.set_height(height);

            string strHeader;
            pRocksDb->GetBlockByBlockHash(txn, block.hash(), strHeader);
            CBlock header;
            header.ParseFromString(strHeader);
            blkprint(formatType, fd, "HashMerkleRoot      -> %s\n",  header.merkleroot().c_str());
            blkprint(formatType, fd, "HashPrevBlock       -> %s\n", header.prevhash().c_str());
            if (formatType == kCaTestFormatType_File) 
            {
                blkprint(formatType, fd, "BlockHash           -> %s\n", block.hash().c_str());
            } 
            else if (formatType == kCaTestFormatType_Print) 
            {
                blkprint(formatType, fd, "BlockHash           -> %s%s%s\n", bkColor.color().c_str(), block.hash().c_str(), bkColor.reset().c_str());
            }

            blkprint(formatType, fd, "nTime			-> ");	
            char blockTime[256] = {};
            time_format_info(blockTime, header.time(), CA_TEST_NTIME, formatType);
            blkprint(formatType, fd, blockTime);

            for (int i = 0; i < header.txs_size(); i++) 
            {
                CTransaction tx = header.txs(i);
                blkprint(formatType, fd, "TX_INFO  -----------> index[%d] start to print \n", i);
                ca_console txColor(kConsoleColor_Red, kConsoleColor_Black, true);
                if (formatType == kCaTestFormatType_File) 
                {
                    blkprint(formatType, fd, "tx[%d] TxHash   -> %s\n", i, tx.hash().c_str());
                } 
                else if (formatType == kCaTestFormatType_Print) 
                {
                    blkprint(formatType, fd, "tx[%d] TxHash   -> %s%s%s\n", i, txColor.color().c_str(), tx.hash().c_str(), txColor.reset().c_str());
                }
                blkprint(formatType, fd, "tx[%d] SignPreHash Length[%zu]   -> \n", i, tx.signprehash_size());

                ca_console greenColor(kConsoleColor_Green, kConsoleColor_Black, true);
                for (int k = 0; k < tx.signprehash_size(); k++) 
                {
                    int signLen = tx.signprehash(k).sign().size();
                    int pubLen = tx.signprehash(k).pub().size();
                    char * hexSign = new char[signLen * 2 + 2]{0};
                    char * hexPub = new char[pubLen * 2 + 2]{0};

                    encode_hex(hexSign, tx.signprehash(k).sign().c_str(), signLen);
                    encode_hex(hexPub, tx.signprehash(k).pub().c_str(), pubLen);

                    // std::string base58Addr = GetBase58Addr(std::string(tx.signprehash(k).pub(), pubLen));
                    char buf[2048] = {0};
                    size_t buf_len = sizeof(buf);
                    GetBase58Addr(buf, &buf_len, 0x00, tx.signprehash(k).pub().c_str(), tx.signprehash(k).pub().size());

                    blkprint(formatType, fd, "SignPreHash %s : %s[%s%s%s] \n", hexSign, hexPub, greenColor.color().c_str(), buf, greenColor.reset().c_str());

                    delete[] hexSign;
                    delete[] hexPub;
                }

                for (int k = 0; k < tx.signprehash_size(); k++) 
                {
                    char buf[2048] = {0};
                    size_t buf_len = sizeof(buf);
                    GetBase58Addr(buf, &buf_len, 0x00, tx.signprehash(k).pub().c_str(), tx.signprehash(k).pub().size());
                    if (formatType == kCaTestFormatType_File) 
                    {
                        blkprint(formatType, fd, "Transaction signer [%s]\n", buf);
                    } 
                    else if (formatType == kCaTestFormatType_Print) 
                    {
                        blkprint(formatType, fd, "Transaction signer   -> [%s%s%s]\n", greenColor.color().c_str(), buf, greenColor.reset().c_str());
                    }
                }

                for (int j = 0; j < tx.vin_size(); j++) 
                {
                    CTxin vtxin= tx.vin(j);
                    blkprint(formatType, fd, "vin[%d] scriptSig Length[%zu]    -> \n", j, tx.vin_size());

                    int signLen = vtxin.scriptsig().sign().size();
                    int pubLen = vtxin.scriptsig().pub().size();
                    char * hexSign = new char[signLen * 2 + 2]{0};
                    char * hexPub = new char[pubLen * 2 + 2]{0};
                    encode_hex(hexSign, vtxin.scriptsig().sign().c_str(), signLen);
                    encode_hex(hexPub, vtxin.scriptsig().pub().c_str(), pubLen);

                    // std::string base58Addr = GetBase58Addr(std::string(vtxin.scriptsig().pub(), pubLen));
                    char buf[2048] = {0};
                    size_t buf_len = sizeof(buf);
                    GetBase58Addr(buf, &buf_len, 0x00, vtxin.scriptsig().pub().c_str(), vtxin.scriptsig().pub().size());

                    if( vtxin.scriptsig().pub().size() == 0 )
                    {
                        blkprint(formatType, fd, "scriptSig %s \n", vtxin.scriptsig().sign().c_str());
                    }
                    else
                    {
                        blkprint(formatType, fd, "scriptSig %s : %s[%s%s%s]\n", hexSign, hexPub, greenColor.color().c_str(), buf, greenColor.reset().c_str());
                    }

                    delete[] hexSign;
                    delete[] hexPub;

                    blkprint(formatType, fd, "vin[%d] PrevoutHash   -> %s\n", j, vtxin.prevout().hash().c_str());
                }	

                ca_console pubkeyColor(kConsoleColor_Green, kConsoleColor_Black, true);
                for (int i = 0; i <tx.vout_size(); i++) {
                    CTxout vtxout = tx.vout(i);
                    ca_console amount(kConsoleColor_Yellow, kConsoleColor_Black, true);
                    if (formatType == kCaTestFormatType_File) {
                        blkprint(formatType, fd, "vout[%d] scriptPubKey[%zu]  -> %s\n", i, vtxout.scriptpubkey().size(), vtxout.scriptpubkey().c_str());
						blkprint(formatType, fd, "vout[%d] mount[%ld] \n", i, vtxout.value());
                    } else if (formatType == kCaTestFormatType_Print) {
                        blkprint(formatType, fd,"vout[%d] scriptPubKey[%zu]  [%s%s%s]\n", i, vtxout.scriptpubkey().size(), pubkeyColor.color().c_str(), vtxout.scriptpubkey().c_str(), pubkeyColor.reset().c_str());
						blkprint(formatType, fd,"vout[%d] mount[%s%ld%s] \n", i, amount.color().c_str(), vtxout.value(), amount.reset().c_str());
                    }
                }
                
                blkprint(formatType, fd, "nLockTime:  ");	
                char txTime[256] {0};
                time_format_info(txTime, tx.time(), CA_TEST_NTIME, formatType);
                blkprint(formatType, fd, txTime);	
                blkprint(formatType, fd, "nVersion: %d\n", tx.version());
            }
        }
        if (i == 0) break;
    }
    std::cout << "count>>>>>>> " << b_count << std::endl;
    std::cout << std::endl;
    close(fd);
}

std::string printBlocks(int num)
{
    auto pRocksDb = MagicSingleton<Rocksdb>::GetInstance();
	Transaction* txn = pRocksDb->TransactionInit();
    unsigned int top = 0;
    pRocksDb->GetBlockTop(txn, top);
    std::string str = "top:\n";
    str += "--------------\n";
    int j = 0;
    for(int i = top; i >= 0; i--){
        str += (std::to_string(i) + "\n");
        std::vector<std::string> vBlockHashs;
        pRocksDb->GetBlockHashsByBlockHeight(txn, i, vBlockHashs);
        for (auto hash : vBlockHashs) {   
            string strHeader;
            pRocksDb->GetBlockByBlockHash(txn, hash, strHeader);
            CBlock header;
            header.ParseFromString(strHeader);
            str = str + hash.substr(0,6) + " ";
            // str = str + hash.substr(0,6) + "-" + std::to_string( header.time() ) + " ";
        } 
        str += "\n";
        j++;
        if(num > 0 && j >= num)
        {
            break;
        }
    }
    str += "--------------\n";   
    return str;
}

void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
    string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));
         
        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
}