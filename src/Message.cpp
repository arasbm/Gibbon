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
#include <stdexcept>

#include "Message.h"
#include "TuioServer.h"
#include "TuioClient.h"
#include "TuioCursor.h"
#include "Setting.h"
#include "Log.h"

using namespace TUIO;

#define setting Setting::Instance()

Message::Message() {
	if(setting->send_tuio) {
		TuioTime::initSession();
		//tuioServer = new TuioServer(setting->tuio_host.c_str(), setting->tuio_port);
		tuioServer = new TuioServer("127.0.0.1", 3333);
	}
}

/**
 * Initialize and prepare a new message to be packed with a bunch of different events.
 * The new message will not be sent until it is committed
 */
void Message::init() {
	if(setting->send_tuio) {
		//TuioTime time = TuioTime::getSessionTime();
		tuioServer->initFrame(TuioTime::getSessionTime());
	}
}
/**
 * Every time (frame) a NEW hand is detected this method should be called to create a new message
 * for this hand and any gestures associated with it
 */

void Message::newHand(Hand hand) {
	if(setting->send_tuio) {
		//tuioTime = TuioTime::getSessionTime();
		//handList[hand.getHandNumber()] = TuioObject(tuioTime, 0, hand.handMessageID(), hand.getX(), hand.getY(), hand.getAngle());
		handList[hand.getHandSide()] = tuioServer->addTuioObject(hand.handMessageID(), hand.getX(), hand.getY(), hand.getAngle());
	}
}

/**
 * for updating an existing hand. New gestures will be retrived from the hand and sent over using
 * appropriate protocols such as TUIO
 */
void Message::updateHand(Hand hand) {
	if(setting->send_tuio) {
		tuioServer->updateTuioObject(handList[hand.getHandSide()], hand.getX(), hand.getY(), hand.getAngle());
	}
}

/**
 * send the message that this hand is not present
 */
void Message::removeHand(Hand hand) {
	if(setting->send_tuio) {
		tuioServer->removeTuioObject(handList[hand.getHandSide()]);
	}
}

/**
 * commit the frame containing all the messages that have been added since last init()
 * the message will be transmitted to the client using appropriate protocol(s) such as TUIO
 */
void Message::commit() {
	if(setting->send_tuio) {
		tuioServer->commitFrame();
	}
}

Message::~Message() {
	if(setting->send_tuio) {
		handList.clear();
		delete tuioServer;
	}
}
