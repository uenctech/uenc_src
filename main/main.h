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
                "-------------------Need to initialize some files to store logs before use ---------------------\n"\
                "\n"\
                "命令:\n"\
                "\n"\
                "--serlog init : Initialize the system log file \n"\
                "\n"\
                "--idlog init : Initialize id log file \n"\
                "\n"\
                "--kblog init : Initialize the log file of k bucket \n"\
                "\n"\
                "After the initialization is successful, the above files are stored in the ./log folder by default \n"\
                "\n"\
                "\n"\
                "--------------------------------------------------------------------------\n"\
                "\n"\
                "EBPC's network protocol :\n"\
                "\n"\
                "The protocol number is four bytes, such as 0x0000 \n"\
                "\n"\
                "The protocol number starting with 0x0... is the network protocol used under the net module \n"\
                "The protocol number starting with 0x1... is the network protocol used under the ca module \n"\
                "\n"\
                "\n"\
                "net Network protocol used by the module ----------------------------------------------------\n"\
                "\n"\
                "PRINT_MESS 0x0000  Print out the received information directly \n"\
                "\n"\
                "FIND_NODE 0x0001  Process find node instructions \n"\
                "\n"\
                "ID_EXIST 0x0002   Received a prompt that id already exists, id will be regenerated \n"\
                "\n"\
                "HANDLE_USR_INFO 0x0003  Process the received user information and set up for online users id\n"\
                "\n"\
                "HANDLE_NEIGHBOR_NODE 0x0004  Process the received neighbor nodes to join k buckets according to the node number \n"\
                "\n"\
                "RECEIVE_PING 0x0005 Ping information received \n"\
                "\n"\
                "\n"\
                "ca Network protocol used by the module -----------------------------------------------------\n"\
                "\n"\
                "CA_TEST 0x1000 Type the received information in hexadecimal and trigger the callback \n"\
                "\n"\
                "\n"\

int deal_command(int argc);

//Add the -m or -M parameter to call the menu function 
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
