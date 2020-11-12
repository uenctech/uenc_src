
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
#include "../ca/ca_http_api.h"
#include "../ca/ca_MultipleApi.h"

int deal_command(int argc)
{
	if (argc == 1 || argc == 2 || argc == 3)
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
		cout << "invalid command!!!" << endl;
		return 1;
	}
	else if (argc == 3)
	{
		if(strcmp(argv[1], "-fee") == 0)
		{	
			uint64_t service_fee = atoi(argv[2]);

			auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
			auto status = rdb_ptr->SetDeviceSignatureFee(service_fee);
			if (!status) {
				net_update_fee_and_broadcast(service_fee);
			}
			cout << "set fee success!" << endl;
		}
		if (true != init())
			return 1;
		while (1)
		{
			sleep(9999);
		}
		return 0;
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

	
	if (false == Singleton<Config>::get_instance()->InitFile())
	{
		debug("配置文件初始化失败");
		return false;
	}
	ca_initConfig();
	ca_register_http_callbacks();
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
			ca_cleanup();
			std::cout << "Bye!!!" << std::endl;
			exit(0);
			return;
		}
		default:
			std::cout << "input error!!!please try again..." << std::endl;
		}

		
		sleep(1);
	}
}
