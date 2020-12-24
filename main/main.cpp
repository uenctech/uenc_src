
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
			if (!status) 
			{
				net_update_fee_and_broadcast(service_fee);
				cout << "set fee success!" << endl;
			}
			else
			{
				cout << "set fee failed!" << endl;
			}
		}
		else if (strcmp(argv[1], "-pack") == 0)
		{
			uint64_t service_package = atoi(argv[2]);

			auto rdb_ptr = MagicSingleton<Rocksdb>::GetInstance();
			auto status = rdb_ptr->SetDevicePackageFee(service_package);
			if (!status) 
			{
				net_set_self_package_fee(service_package);
				cout << "set package success!" << endl;
			}
			else
			{
				cout << "set package failed!" << endl;
			}
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
	
}
