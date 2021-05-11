//#include <iostream>
#include "../include/all.h"
#include "main.h"
#include "../utils/singleton.h"
#include "../net/net_api.h"
#include "./net/test_menu.h"
#include "../ca/ca_interface.h"
#include "../ca/ca_hexcode.h"
#include "../ca/ca.h"
#include "../ca/ca_global.h"
#include "../include/logging.h"
#include "../common/devicepwd.h"

/**
 * @description: 初始化log
 */
bool InitConfig();




int deal_command(int argc)
{
	if (argc <= 6)
	{
		return 0;
	}
	else
	{
		cout << "command error!!!" << endl;
	 	return -1;
	}

}

int init_menu(int argc, char *argv[])
{
	if (argc == 2)
	{
		if (strcmp(argv[1], "--help") == 0)
	 	{
	 		cout << EBPC_HELP << endl;
	 		return 0;
	 	}
		//加上-m或者-M参数才会调用菜单函数
		else if (strcmp(argv[1], "-m") == 0|| strcmp(argv[1], "-M") == 0)
		{
			if (true != init())
				return 1;
			menu();

			return 0;
		}
		else
		{
			cout << "invalid command!!!" << endl;
			return 1;
		}
	}
	else if (argc == 3)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[2], "--help") == 0)
	 	{
	 		cout << EBPC_HELP << endl;
	 		return 0;
	 	}
		//加上-m或者-M参数才会调用菜单函数
		if (strcmp(argv[1], "-m") == 0|| strcmp(argv[1], "-M") == 0 || strcmp(argv[2], "-m") == 0|| strcmp(argv[2], "-M") == 0)
		{
			if (true != init())
				return 1;
			menu();

			return 0;
		}

		cout << "invalid command!!!" << endl;
		return 1;
	}
	else if (argc == 1)
	{
		if (true != init())
			return 1;
		while (1)
		{
			sleep(9999);
		}
		return 0;
	}
	else
	{
		cout << "unknown error occured" << endl;
		return 1;
	}
}

bool init()
{
	if (! InitConfig() )
	{
		return false;
	}

	if(!InitDevicePwd())
	{
		return false;
	}
	
	return net_com::net_init() && ca_init();
}

void menu()
{
	char key;

    interface_Init("./cert");
	
	while (1)
	{
		std::cout << "1.ca" << std::endl;
		std::cout << "2.net" << std::endl;
		std::cout << "3.gen key" << std::endl;
		std::cout << "4.transation" << std::endl;
		std::cout << "5.test get amount" << std::endl;
		std::cout << "7.gen usr" << std::endl;
		std::cout << "8.out private key" << std::endl;
		std::cout << "9.in private key" << std::endl;
		std::cout << "x.test gen tx" << std::endl;
		std::cout << "0.quit" << std::endl;
		std::cout << "please input your choice:";
		std::cin >> key;

		switch (key)
		{
		case '1':
			ca_print();
			break;

		case '2':
			Singleton<TestMenu>::get_instance()->main_menu();
			break;

        case '3':
                interface_Init("/home/cert");
                interface_GenerateKey();
                break;
        case '4':
                {
                    char **phone_buf = NULL;
                    int phone_len = 0;
                    char from[1024] = {0};
                    char to[1024] = {0};
                    char ip[32] = {0};

                    cout << "please input from:";
                    cin >> from;

                    cout << "please input to:";
                    cin >> to;

                    cout << "please input ip:";
                    cin >> ip;

                    CreateTx(from, to, "1", NULL, 3, "0.1");

                    printf("hash len:%d\n",phone_len);
                    hex_print((unsigned char *)phone_buf,phone_len);

                    if(phone_buf)
                    free(phone_buf);
                    break;
                }

        case '5':
                {
                    char out_data[1024] = {0};
                    size_t data_len = 1024;
                    int ret = interface_ShowMeTheMoney("1BViAvtJ3XU1kyHXp1F5ebFBDWM2jxQYP2", 20, out_data, &data_len);
                    if(ret)
                    {
                        cout << "get suc len:" << data_len << endl;
                    }
                    break;
                }

        case '7':
			{
				interface_GenerateKey();
				break;
			}

		case '8':
			{
				char out_data[1024] = {0};
				int data_len = 1024;

                interface_Init("./cert");

				int ret = interface_GetPrivateKeyHex(nullptr, out_data, data_len);
				if(ret)
					cout << "get private key success." << endl;
				else
					cout << "get private key error." << endl;

				cout << "private key:" << out_data << endl;

				memset(out_data,0,1024);

				ret = interface_GetMnemonic(nullptr, out_data, data_len);
				if(ret)
					cout << "get mem success." << endl;
				else
					cout << "get mem error." << endl;

				cout << "mem data:" << out_data << endl;
				break;
			}

		case '9':
			{
                interface_Init("./cert");
				char data[1024] = {0};
				cout << "please input private key:";
				cin >> data;
				int ret = interface_ImportPrivateKeyHex(data);

				if(ret)
					cout << "import private key suc." << endl;
				else
					cout << "import private key fai." << endl;
				break;
			}

        case 'x':
            {
            }

		case '0':
		{
			//ca_cleanup();
			//std::cout << "Bye!!!" << std::endl;
			//exit(0);
			return;
		}
		default:
			std::cout << "input error!!!please try again..." << std::endl;
		}

		//解决守护进程启动后刷屏问题
		sleep(1);
	}
}

// Create: parse paramter, 20201103   LiuMingLiang
int parse_command(int argc, char *argv[])
{
	bool showMenu = false;
	float signFee = 0;
	float packFee = 0;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--help") == 0)
		{
			cout << argv[0] << ": --help version:" << getEbpcVersion() << " \n -m show menu\n -s value, signature fee\n -p value, package fee" << endl;
			exit(0);
			return 0;
		}
		else if (strcmp(argv[i], "-s") == 0) // strcasecmp()
		{
			if (i + 1 < argc)
			{
				i++;
				signFee = atof(argv[i]);
				cout << "Set signature fee: " << signFee << endl;
			}
		}
		else if (strcmp(argv[i], "-p") == 0)
		{
			if (i + 1 < argc)
			{
				i++;
				packFee = atof(argv[i]);
				cout << "Set package fee: " << packFee << endl;
			}
		}
		else if (strcmp(argv[i], "-m") == 0)
		{
			showMenu = true;
			cout << "Show menu." << endl;
		}
	}

	if (!init())
	{
		return 1;
	}
	
	if (signFee > 0.0001)
	{
		set_device_signature_fee(signFee);
	}
	if (packFee > 0.0001)
	{
		set_device_package_fee(packFee);
	}
	Singleton<Config>::get_instance()->Removefile();
	Singleton<Config>::get_instance()->RenameConfig();
	if (showMenu)
	{
		menu_command();

		return 0;
	}
	else
	{
		while (1)
		{
			sleep(9999);
		}
		return 0;
	}
	
	return 0;
}

// Create: show menu, 20201103   LiuMingLiang
void menu_command()
{
	ca_print_basic_info();

	interface_Init("./cert");
	
	
	// Show menu.
	while (true)
	{
		cout << "1.Transaction" << endl;
		cout << "2.Pledge" << endl;
		cout << "3.Redeem pledge" << endl;
		cout << "0.exit" << endl;
		cout << "Please input your choice: "<< endl;

		char key;
		cin >> key;
		switch (key)
		{
			case '1':
				cout << "Selected transaction." << endl;
				handle_transaction();
				break ;
			case '2':
				cout << "Selected pledge." << endl;
				handle_pledge();
				break ;
			case '3':
				cout << "Selected redeem pledge." << endl;
				handle_redeem_pledge();
				break ;
			case '9':
				cout << "Selected advance menu." << endl;
				menu();
				break ;
			case '0':
				cout << "Exiting, bye!" << endl;
				ca_cleanup();
				exit(0);
				return ;
				break ;
			default:
				cout << "Input error! please try again." << endl;
				break ;
		}

		sleep(1);
	}
	
}

bool InitConfig()
{
	//配置文件初始化
	if (false == Singleton<Config>::get_instance()->InitFile())
	{
		std::cout << "init Config failed" << std::endl;
		return false;
	}

	std::string logLevel = Singleton<Config>::get_instance()->GetLogLevel();
	if (logLevel.length() == 0)
	{
		logLevel = "TRACE";
	}
	setloglevel(logLevel);

	std::string logFilename = Singleton<Config>::get_instance()->GetLogFilename();
	if (logFilename.length() == 0)
	{
		logFilename = "log.txt";
	}
	setlogfile(logFilename);


	return true;
}


bool InitDevicePwd()
{
	//配置文件初始化
	if (false == Singleton<DevicePwd>::get_instance()->InitPWDFile())
	{
		std::cout << "init Config failed" << std::endl;
		cout<<"init Config failed get_instance()->InitPWDFile())"<<endl;
		return false;
	}
	if (false == Singleton<DevicePwd>::get_instance()->UpdateDevPwdConfig())
	{
		std::cout << "update devpwd config failed" << std::endl;
		cout<<"update devpwd config get_instance()->UpdateDevPwdConfig())"<<endl;
		return false;
	}
	return true;
}