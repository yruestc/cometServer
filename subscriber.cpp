/*
 * =====================================================================================
 *
 *       Filename:  subscriber.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年01月02日 16时35分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Allen comet (), 1135628277@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include"subscriber.h"

namespace http {

	Subscriber::Subscriber() {

	}
	Subscriber::~Subscriber() {

	}

	bool Subscriber::operator==(Subscriber & subscriber) {
		if (_id == subscriber._id)
			return true;
		return false;
	}

	bool Subscriber::operator<(Subscriber & subscriber) {
		if (_id < subscriber._id)
			return true;
		return false;
	}

}
