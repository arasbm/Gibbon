/*
 * Message.cpp
 *
 *  Created on: 2011-07-07
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

#include "Message.h"
#include "TuioServer.h"
#include "TuioClient.h"
#include "TuioCursor.h"
#include "Setting.h"

using namespace TUIO;
using namespace osc;

static Setting setting = Setting::Instance();

Message::Message() {
	if(setting.send_tuio) {
		tuioServer = new TuioServer();
	}
}

/**
 * Initialize and prepare a new message to be packed with a bunch of different events.
 * The new message will not be sent until it is committed
 */
void Message::init() {
	if(setting.send_tuio) {

	}
}

void Message::addHand(Hand hand) {
	if(setting.send_tuio) {

	}
}

void Message::updateHand(Hand hand) {
	if(setting.send_tuio) {
	}
}

void Message::removeHand(Hand hand) {
	if(setting.send_tuio) {
	}
}

void Message::commit() {
	if(setting.send_tuio) {
	}
}

Message::~Message() {
	if(setting.send_tuio) {
		delete tuioServer;
	}
}
