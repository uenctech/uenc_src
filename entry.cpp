#include <iostream>
#include "./main/main.h"
#include "./include/logging.h"
#include "./common/config.h"

using namespace std;







int main(int argc, char* argv[])
{
    
	if (0 != deal_command(argc))
		return 0;

	init_menu(argc, argv);
	
	return 0;
}
