# 编译说明

操作系统：Centos 7

GCC版本：gcc8.3.1以上

可能需要额外安装的依赖库：zlib  

编译protobuf需要的工具：autoconf automake make



编译准备：

1.安装dos2unix转换工具

yum -y install dos2unix

2.将gen_version_info.sh文件转换为unix格式

dos2unix gen_version_info.sh

3.如果没有zlib库，安装zlib库

yum -y install zlib 

4.安装编译protobuf需要的工具

yum -y install gcc automake autoconf libtool make



编译命令：

make					生成测试网程序

make primary 		 生成主网程序



当编译出现问题需要重新编译时：

1.删除protobuf、crypto、rocksdb文件夹

2.make clean

3.make 或 make primary



运行程序：

参考uenc-demo中的节点部署说明

