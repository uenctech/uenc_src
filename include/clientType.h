#ifndef _CLIENTTYPE_H_
#define _CLIENTTYPE_H_

typedef enum emClientType{
    kClientType_PC, // PC 设备
    kClientType_iOS, // iOS设备
    kClientType_Android // 安卓设备
}ClientType;

typedef enum emClientLanguage{
    kClientLanguage_zh_CN,
    kkClientLanguage_en_US,
}ClientLanguage;

#endif // !_CLIENTTYPE_H_
