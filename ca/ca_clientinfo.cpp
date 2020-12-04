#include "ca_clientinfo.h"
#include "../include/cJSON.h"
#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <memory.h>
#include "../include/ScopeGuard.h"


const char * kClientInfoFile = "clientinfo.json";

const char * kLanguageZHCN = "zh_CN";
const char * kLanguageENUS = "en_US";

std::string ca_create_clientInfo_JSon_string()
{
    cJSON * root = cJSON_CreateObject();

    // chinese
    std::map<std::string, std::string> miOS_zh_CN;
    miOS_zh_CN["ver"] = "3.0.6";
    miOS_zh_CN["desc"] = "此次版本更新了iOS XXXX";
    miOS_zh_CN["dl"] = "https://www.baidu.com/img/bd_logo1.png";

    std::map<std::string, std::string> mAndroid_zh_CN;
    mAndroid_zh_CN["ver"] = "3.0.6";
    mAndroid_zh_CN["desc"] = "此次版本更新了Android YYYY";
    mAndroid_zh_CN["dl"] = "https://www.baidu.com/img/bd_logo1.png";

    // english
    std::map<std::string, std::string> miOS_en_US;
    miOS_en_US["ver"] = "3.0.6";
    miOS_en_US["desc"] = "This release updates iOS Bala bala...";
    miOS_en_US["dl"] = "https://www.baidu.com/img/bd_logo1.png";

    std::map<std::string, std::string> mAndroid_en_US;
    mAndroid_en_US["ver"] = "3.0.6";
    mAndroid_en_US["desc"] = "This release updates Android Bala bala...";
    mAndroid_en_US["dl"] = "https://www.baidu.com/img/bd_logo1.png";

    
    std::vector<std::string> models = {"iOS", "Android"};
    std::vector<std::string> languages = {kLanguageZHCN, kLanguageENUS};

    std::vector<decltype(miOS_zh_CN)> infos_iOS = {miOS_zh_CN, miOS_en_US};
    std::vector<decltype(miOS_en_US)> infos_Android = {mAndroid_zh_CN, mAndroid_en_US};
    std::vector<decltype(infos_Android)> infos = {infos_iOS, infos_Android};

    for (size_t i = 0; i < models.size(); i++)
    {
        cJSON * jModel = cJSON_CreateObject();
        cJSON * jUpdate = cJSON_CreateObject();
        auto info = infos[i];
        for (unsigned int i = 0; i != info.size(); ++i)
        {
            auto m = info[i];
            cJSON * jLanguage = cJSON_CreateObject();
            for(auto && item : m)
            {
                cJSON * jValue = cJSON_CreateString(item.second.c_str());
                cJSON_AddItemToObject(jLanguage, item.first.c_str(), jValue);
            }
            cJSON_AddItemToObject(jUpdate, languages[i].c_str(), jLanguage);    
        }
        cJSON_AddItemToObject(jModel, "update", jUpdate);
        cJSON_AddItemToObject(root, models[i].c_str(), jModel);
    }

    std::string retString = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);

    return retString;
}

std::string ca_clientInfo_read()
{
    FILE * fp = fopen(kClientInfoFile, "a+");

   // if (fp < 0)
	if(fp == NULL)    
    {
        return "";
    }
    
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string out;
    if (len == 0)
    {
        out = ca_create_clientInfo_JSon_string();
        fwrite(out.c_str(), sizeof(char), out.size(), fp);
        fflush(fp);
        fclose(fp);
        return out;
    }

    fseek(fp, 0, SEEK_SET);
    char * buf = new char[len + 1];
    memset(buf, 0, len + 1);
    fread(buf, sizeof(char), len + 1, fp);
    fclose(fp);

    out = std::string(buf, len);
    delete [] buf;

    return out;
}

int ca_getUpdateInfo(const std::string & s, ClientType clientType, ClientLanguage language, std::string & sVersion, std::string & sDesc, std::string & sDownload)
{
    if (s.length() == 0)
    {
        return -1;
    }

    const char * pDevice = nullptr;
    if (clientType == kClientType_iOS)
    {
        pDevice = "iOS";
    }
    else if (clientType == kClientType_Android)
    {
        pDevice = "Android";
    }
    else
    {
        return -2;
    }

    cJSON * root = cJSON_Parse(s.c_str());
    if (root == nullptr)
    {
        return -3;
    }

    ON_SCOPE_EXIT{
        cJSON_Delete(root);
    };
    
    cJSON * device = cJSON_GetObjectItem(root, pDevice);
    if (device == nullptr)
    {
        return -4;
    }

    cJSON * update = cJSON_GetObjectItem(device, "update");
    if (update == nullptr)
    {
        return -5;
    }


    std::string strLang = "zh_CN";
    if (language == kClientLanguage_zh_CN)
    {
        strLang = "zh_CN";
    }
    else if (language == kkClientLanguage_en_US)
    {
        strLang = "en_US";
    }

    cJSON * jLanguage = cJSON_GetObjectItem(update, strLang.c_str());

    cJSON * ver = cJSON_GetObjectItem(jLanguage, "ver");
    cJSON * desc = cJSON_GetObjectItem(jLanguage, "desc");
    cJSON * dl = cJSON_GetObjectItem(jLanguage, "dl");
    if (ver == nullptr || desc == nullptr || dl == nullptr)
    {
        return -6;
    }

    sVersion = ver->valuestring;
    sDesc = desc->valuestring;
    sDownload = dl->valuestring;

    return 0;
}

