#ifndef EBPC_MAIN_H
#define EBPC_MAIN_H
 
#define EBPC_HELP\
                "NAME:\n"\
                "    ebpc - the kademlia network interface\n"\
                "\n"\
                "    Copyright 2018-2019 The ebpc Authors\n"\
                "\n"\
                "\n"\
                "VERSION:\n"\
                "    1.8-stable\n"\
                "\n"\
                "\n"\
                "COMMANDS:\n"\
                "\n"\
                "\n"\
                "\n"\
                "-------------------使用前需要初始化一些存放日志的文件---------------------\n"\
                "\n"\
                "命令:\n"\
                "\n"\
                "--serlog init : 初始化系统日志文件\n"\
                "\n"\
                "--idlog init : 初始化id日志文件\n"\
                "\n"\
                "--kblog init : 初始化k桶的日志文件\n"\
                "\n"\
                "初始化成功以上文件都默认存放在./log文件夹下\n"\
                "\n"\
                "\n"\
                "--------------------------------------------------------------------------\n"\
                "\n"\
                "EBPC的网络协议:\n"\
                "\n"\
                "协议号为四个字节 如0x0000\n"\
                "\n"\
                "协议号0x0...开头的为net模块下使用的网络协议\n"\
                "协议号0x1...开头的为ca模块下使用的网络协议\n"\
                "\n"\
                "\n"\
                "net 模块使用的网络协议----------------------------------------------------\n"\
                "\n"\
                "PRINT_MESS 0x0000  把接收到的信息直接打印出来\n"\
                "\n"\
                "FIND_NODE 0x0001  处理find node指令\n"\
                "\n"\
                "ID_EXIST 0x0002   收到id 已存在的提示 将重新生成id\n"\
                "\n"\
                "HANDLE_USR_INFO 0x0003  处理接收到的用户信息 为在线用户设置id\n"\
                "\n"\
                "HANDLE_NEIGHBOR_NODE 0x0004  处理接收到的邻居节点 根据节点号来加入k桶\n"\
                "\n"\
                "RECEIVE_PING 0x0005 接收到的ping信息\n"\
                "\n"\
                "\n"\
                "ca 模块使用的网络协议-----------------------------------------------------\n"\
                "\n"\
                "CA_TEST 0x1000 把接收到信息16进制打出 并触发回调\n"\
                "\n"\
                "\n"\

int deal_command(int argc);

//加上-m或者-M参数才会调用菜单函数
int init_menu(int argc, char *argv[]);

int mkdir_log();
int serlog_file_init();
int kblog_file_init();
int idlog_file_init();

bool init();
bool InitDevicePwd();

void menu();

int parse_command(int argc, char *argv[]);
void menu_command();

#endif
