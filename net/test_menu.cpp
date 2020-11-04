#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "test_menu.h"
#include "./peer_node.h"
#include "../include/logging.h"
#include "./socket_buf.h"
void TestMenu::main_menu()
{
	char key;
	while (1)
	{
		cout << endl;
		cout << "1.Send Mess To User" << endl;
		cout << "2.Show My K Bucket" << endl;
		cout << "3.踢节点" << endl;
		cout << "4.test echo" << endl;
		cout << "5.Broadcast Sending Messages" << endl;
		cout << "7.Print bufferes" << endl;
		cout << "9.Big Data send to User" << endl;
		cout << "e.Show My MD5_ID" << endl;
		cout << "f.Show My PublicNode" << endl;
		cout << "g.GetSelfID" << endl;
		cout << "0.Quit" << endl;
		cout << "Please input your choice:" << endl;

		cin >> key;

		switch (key)
		{
		case '0':
			return;

		case '1':
		{
			if (net_com::input_send_one_message() == 0)
				debug("send one msg Succ.");
			else
				debug("send one msg Fail.");
			break;
		}

		case '2':
		{
			cout << endl
				 << "The K bucket is being displayed..." << endl;
			auto list = Singleton<PeerNode>::get_instance()->get_nodelist();
			Singleton<PeerNode>::get_instance()->print(list);
			break;
		}

		case '3':
		{
			cout << "input id:" << endl;
			string id;
			cin >> id;
			Singleton<PeerNode>::get_instance()->delete_node(id);
			cout << "踢节点成功！" << endl;
			break;
		}

		case '4':
		{
			EchoReq echoReq;
			echoReq.set_id(Singleton<PeerNode>::get_instance()->get_self_id());
			net_com::broadcast_message(echoReq);
			break;
		}		
		case '5':
		{
			net_com::test_broadcast_message();
			break;
		}

		case '7':
		{
			Singleton<BufferCrol>::get_instance()->print_bufferes();
			break;
		}
		case '9':
		{
			net_com::test_send_big_data();
			break;
		}
		case 'f':
		{
			auto list = Singleton<PeerNode>::get_instance()->get_nodelist(NODE_PUBLIC);
			Singleton<PeerNode>::get_instance()->print(list);
			break;
		}
		case 'g':
		{
			printf("MyID : %s\n", Singleton<Config>::get_instance()->GetKID().c_str());
			break;
		}
		default:
			cout << "No such case..." << endl;
			break;
		}
	}
}
