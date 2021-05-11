#include <iostream>
#include "./main/main.h"
#include "./include/logging.h"
#include "./common/config.h"
//#include <signal.h>
using namespace std;

int main(int argc, char* argv[])
{
    //signal(SIGINT, keycontrol);
	if (0 != deal_command(argc))
		return 0;

	//init_menu(argc, argv);
	parse_command(argc, argv);
	
	return 0;
}
