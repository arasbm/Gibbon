/*
 * Hand.cpp
 *
 *  Created on: 2011-07-06
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

#include "Hand.h"

Hand::Hand(handSide s) {
	static int handCount;
	side = s;
	present = false;
	handNumber = handCount;
	handCount++;
}

handSide Hand::getHandSide() {
	return side;
}

bool Hand::isPresent() {
	return present;
}

void Hand::setPresent(bool p) {
	present = p;
}

Mat Hand::getContour() {
	return contour;
}

void Hand::setContour(Mat handContour) {
	contour = handContour;
}

/**
 * rect should be the enclosing rectangle with minimum area
 * as calculated with cv::minAreaRect function
 */
void Hand::setMinRect(RotatedRect rect) {
	minRect = rect;
}

RotatedRect Hand::getMinRect() {
	return minRect;
}

void Hand::setMinCircleCenter(Point2f center) {
	circleCenter = center;
}

Point2f Hand::getMinCircleCenter() {
	return circleCenter;
}

void Hand::setMinCircleRadius(int radius) {
	circleRadius = radius;
}

int Hand::getMinCircleRadius() {
	return circleRadius;
}

/**
 * Returns the unique number assigned to each hand object during construction of that object.
 * Note that multiple hand objects may be used to track a physical hand within a temporal window
 */
int Hand::getHandNumber() {
	return handNumber;
}

/**
 * Set this hand presence to false and do any other additional cleanup if necessary
 */
void Hand::clear() {
	setPresent(false);
	//TODO: Check for anything else I need to do here to prevent error or release memory
}
