/*
 * =====================================================================================
 *
 *       Filename:  channel.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 10时03分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include"channel.h"

namespace http {
	Channel::Channel() {
		init();		//初始化
	}
	Channel::~Channel() {
	}

	void Channel::init() {
		_freeStart = 0;
		_msgcount = 0;
	}

	void Channel::delSubscriber(unsigned int id)	//删除指定ID的用户
	{
		auto it = _subscriber.begin();
		for (; it != _subscriber.end();) {
			if ((*it)->_id == id) {
				_subscriber.erase(it);	//删除该用户
			}
		}
	}

	bool Channel::operator==(Channel & ch) {
		if (_id == ch._id)
			return true;
		return false;
	}

	bool Channel::operator<(Channel & ch) {
		if (_id < ch._id)
			return true;
		return false;
	}
}
