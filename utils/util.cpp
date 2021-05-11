/*
 * @Author: your name
 * @Date: 2020-05-07 17:30:18
 * @LastEditTime: 2021-03-27 14:30:34
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\utils\util.cpp
 */
#include <sys/time.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include "./util.h"
#include "common/version.h"
#include "common/global.h"


uint32_t Util::adler32(const unsigned char *data, size_t len) 
{
    const uint32_t MOD_ADLER = 65521;
    uint32_t a = 1, b = 0;
    size_t index;
    
    // Process each byte of the data in order
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
}

int Util::IsLinuxVersionCompatible(const std::vector<std::string> & vRecvVersion)
{
	if (vRecvVersion.size() == 0)
	{
		std::cout << "(linux)版本错误：-1" << std::endl;
		return -1;
	}
	std::string ownerVersion = getVersion();
	std::vector<std::string> vOwnerVersion;
	StringUtil::SplitString(ownerVersion, vOwnerVersion, "_");

	if (vOwnerVersion.size() != 3)
	{
		std::cout << "(linux)版本错误：-2" << std::endl;
		return -2;
	}

	// 版本号判断
	std::vector<std::string> vOwnerVersionNum;
	StringUtil::SplitString(vOwnerVersion[1], vOwnerVersionNum, ".");
	if (vOwnerVersionNum.size() == 0)
	{
		std::cout << "(linux)版本错误：-3" << std::endl;
		return -3;
	}

	std::vector<std::string> vRecvVersionNum;
	StringUtil::SplitString(vRecvVersion[1], vRecvVersionNum, ".");
	if (vRecvVersionNum.size() != vOwnerVersionNum.size())
	{
		std::cout << "(linux)版本错误：-4" << std::endl;
		return -4;
	}

	for (size_t i = 0; i < vRecvVersionNum.size(); i++)
	{
		if (vRecvVersionNum[i] < vOwnerVersionNum[i])
		{
			std::cout << "(linux)版本错误：-5" << std::endl;
			return -5;
		}
		else if (vRecvVersionNum[i] > vOwnerVersionNum[i])
		{
			return 0;
		}
	}

	if (g_testflag == 1)
	{
		if (vRecvVersion[2] != "t")
		{
			std::cout << "(linux)版本错误：-6" << std::endl;
			return -6;
		}
	}
	else
	{
		if (vRecvVersion[2] != "p")
		{
			std::cout << "(linux)版本错误：-7" << std::endl;
			return -7;
		}
	}

	return 0;
}

int Util::IsOtherVersionCompatible(const std::string & vRecvVersion, bool bIsAndroid)
{
	if (vRecvVersion.size() == 0)
	{
		std::cout << "(other) 版本错误: -1" << std::endl;
		return -1;
	}

	std::vector<std::string> vRecvVersionNum;
	StringUtil::SplitString(vRecvVersion, vRecvVersionNum, ".");
	if (vRecvVersionNum.size() != 3)
	{
		std::cout << "(other) 版本错误: -2" << std::endl;
		return -2;
	}

	std::string ownerVersion;
	if (bIsAndroid)
	{
		ownerVersion = g_AndroidCompatible;
	}
	else
	{
		ownerVersion = g_IOSCompatible;
	}
	
	std::vector<std::string> vOwnerVersionNum;
	StringUtil::SplitString(ownerVersion, vOwnerVersionNum, ".");

	for (size_t i = 0; i < vOwnerVersionNum.size(); ++i)
	{
		if (vRecvVersionNum[i] < vOwnerVersionNum[i])
		{
			std::cout << "(other) 版本错误: -3" << std::endl;
			std::cout << "(other) 接收到的版本：" << vRecvVersion << std::endl;
			std::cout << "(other) 本地的版本：" << ownerVersion << std::endl;
			return -3;
		}
		else if (vRecvVersionNum[i] > vOwnerVersionNum[i])
		{
			return 0;
		}
	}

	return 0;
}

int Util::IsVersionCompatible( std::string recvVersion )
{
	if (recvVersion.size() == 0)
	{
		std::cout << "版本错误：-1" << std::endl;
		return -1;
	}

	std::vector<std::string> vRecvVersion;
	StringUtil::SplitString(recvVersion, vRecvVersion, "_");
	if (vRecvVersion.size() != 2 && vRecvVersion.size() != 3)
	{
		std::cout << "版本错误：-2" << std::endl;
		return -2;
	}

	int versionPrefix = std::stoi(vRecvVersion[0]);
	if (versionPrefix > 4 || versionPrefix < 1)
	{
		std::cout << "版本错误：-3" << std::endl;
		return -3;
	}
	
	switch(versionPrefix)
	{
		case 1:
		{
			// linux  (例：1_0.3_t)
			if ( 0 != IsLinuxVersionCompatible(vRecvVersion) )
			{
				return -4;
			}
			break;
		}
		case 2:
		{
			// 暂无windows
			return -5;
		}
		case 3:
		{
			// Android  (例：4_3.0.14)
			if ( 0 != IsOtherVersionCompatible(vRecvVersion[1], false) )
			{
				return -6;
			}
			break;
		}
		case 4:
		{
			// IOS  (例：3_3.0.14)
			if ( 0 != IsOtherVersionCompatible(vRecvVersion[1], true) )
			{
				return -6;
			}
			break;
		}
		default:
		{
			return -7;
		}
	}
	return 0;
}