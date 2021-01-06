/*
 * @Author: your name
 * @Date: 2020-04-18 10:55:33
 * @LastEditTime: 2020-05-07 14:41:43
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \ebpc\net\test_menu.h
 */
#ifndef _TEST_MENU_H_
#define _TEST_MENU_H_


#include "./../include/net_interface.h"
#include "../utils/singleton.h"
#include "./net_api.h"

class TestMenu
{
	friend class Singleton<TestMenu>;

public:
	void main_menu();
};

#endif//_TEST_MENU_H_