# **开发者文档**
## 操作系统:
  * CentOS7及以上版本
## 开发环境：
### step 1:升级自己的gcc编译器到8.3.1：

```
 yum install centos-release-scl scl-utils-build   
 yum install -y devtoolset-8-toolchain
 scl enable devtoolset-8 bash
```
查看是否升级成功:
```
 gcc --version
```

### step	2:查看、关闭、开启系统防火墙
```
sudo systemctl status firewalld  查看
systemctl stop firewalld.service  关闭
systemctl start firewalld.service 开启
```

### step	3:升级完gcc版本为8.3.1之后(根据自身ip，例如为192.168.1.62)配置虚拟机上网。
```
vi /etc/sysconfig/network-scripts/ifcfg-ens33
添加以下几行：
IPADDR=
NETMASK=255.255.255.0
GATEWAY=192.168.1.1
DNS1=223.5.5.5
修改两行：
BOOTPROTO=static
ONBOOT=yes
重启服务
service network restart
```


### step 4: git安装：  
##### (1).删除旧版本的git  
执行:
```
yum  remove git。
```

##### (2).在线安装   
 
```
yum install git 
```

### step 5:获取UENC源码
```
 https://github.com/uenctech/uenc_src
```
### step 6:编译运行
 ##### 安装依赖库：
``` 
 yum install -y zlib zlib-devel  
 yum install -y unzip zip  
 yum install -y autoconf automake libtool
 执行make开始编译，编译时提示说libprotobuf.a: 没有那个文件或目录，删除protobuf目录重新编译。
 编译完成后生成可执行文件ebpc_xxxxx_testnet。
 运行可执行文件：  
./ebpc_xxxxx_testnet
```   
 * 按下Ctrl+C键退出。此时会生成 cert
    目录及config.json,data.db,devpwd.json,log.txt文件。

  | 文件或目录 |     描述     | 
 | :--------: | :--------------: | 
 |   cert   | 存放生成的公私密钥对，后缀为".public.key"的文件是公钥文件，后缀为".private.key"为私钥文件 | 
 |   data.db   | 数据库文件 | 
 |   devpwd.json   | 本机访问密码哈希值，当移动端连接时使用该密码进行连接 | 
 |   config.json   | 配置文件 | 
 |   log.txt   | 日志文件 | 
 
   * 修改config.json
   
   自己节点是子网节点无需配置，可按默认配置直接运行如果是公网节点按照如下配置：
 
 1. 需要将is_public_node的值由false修改为true。
 2. 将server中的IP字段的值设置为自身节点所连接的其他公网节点IP地址。
 3. 将var字段下的local_ip字段设置为自身节点的外网IP地址。
 
 例如，自身节点外网IP地址为xxx.xxx.xxx.xxx, 所连接公网地址为yyy.yyy.yyy.yyy, 则按如下配置（以测试网公网节点配置为例）

```
"is_public_node": false,
    "server": [
        {
            "IP": "yyy.yyy.yyy.yyy",
            "PORT": 11188
        },
    ],
    "var": {
    "k_bucket": "a0dbbd80eb84b9e51f3a0d69727384c651f9bdb5",
    "k_refresh_time": 100,
    "local_ip": "xxx.xxx.xxx.xxx",
    "local_port": 11188,
    "work_thread_num": 4
    },
```
### 运行时参数介绍：  
|       参数 |参数说明|
|:---:|:---:|  
|--help  |获取帮助菜单|
|-m       |显示菜单   |
|-s       |设置矿费   |
|-p       |设置打包费 |  

列如设置矿费：
```
    ./uenc_1.3_testnet -s  0.015
```

  注意：实际值 = value * 0.000001。实际值最小值：0.000001。
 
 
Q&A:
### 编译命令：
```
make 生成测试网程序

make primary 生成主网程序

当编译出现问题需要重新编译时：

1.删除protobuf、crypto、rocksdb文件夹

2.make clean

3.make 或 make primary
```
