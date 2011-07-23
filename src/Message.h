/*
 * Message.h
 *
 *  Created on: 2011-07-05
 *      Author: Aras Balali Moghaddam
 *
 *  This file is part of Gibbon (Bimanual Near Touch Tracker).
 *
 *  Gibbon is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 3.
 *
 *  Gibbon is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Gibbon.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_
#include "TuioServer.h"
#include "TuioClient.h"
#include "TuioCursor.h"
#include <map>
#include "Hand.h"

using namespace TUIO;
using namespace osc;

class Message {
public:
	Message();
	void init();
	void addHandGesture(Hand hand);
	void updateHand(Hand hand);
	void removeHand(Hand hand);
	void commit();
	~Message();

private:
	TuioServer tuioServer;
	TuioTime tuioTime;
	//TuioCursor* touch;
	std::map<int, TuioObject> handList; //One tuio object for each hand object
	//TODO if above does not work properly try using typedef

};

#endif /* MESSAGE_H_ */
