#ifndef _CA_CLIENTINFO_H_
#define _CA_CLIENTINFO_H_

#include <string>
#include "../include/clientType.h"

/**
 * @description: 读取clientInfo.json中的数据
 * @return: 返回文件中的数据
 * @mark: 若文件不存在则创建默认文件，并返回初始数据
 */
std::string ca_clientInfo_read();

/**
 * @description: 获得更新数据
 * @param s 升级数据
 * @param eClientType 客户端类型
 * @param language 所选语言
 * @param sVersion 返回的版本信息
 * @param sDesc 返回的描述信息
 * @param sDownload 返回的下载地址
 * @return: 成功返回0 失败返回其他值
 */
int ca_getUpdateInfo(const std::string & s, ClientType eClientType, ClientLanguage language, std::string & sVersion, std::string & sDesc, std::string & sDownload);

#endif // !1