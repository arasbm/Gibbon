/*
 * Hand.h
 *
 *  Created on: 2011-06-28
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

#ifndef HAND_H_
#define HAND_H_

#include "cv.h"

using namespace cv;

typedef enum _handSide {
	LEFT_HAND,
	RIGHT_HAND
} handSide;

class Hand {

public:
	Hand(handSide s);
	handSide getHandSide();
	bool isPresent();
	void setPresent(bool present);
	void setMinRect(RotatedRect minRect);
	RotatedRect getMinRect();
	void setMinCircleCenter(Point2f center);
	Point2f getMinCircleCenter();
	void setMinCircleRadius(int radius);
	int getMinCircleRadius();
	Mat getContour();
	void setContour(Mat contour);
	int getHandNumber();
	void clear();

private:
	handSide side;
	Point2f circleCenter; //center of enclosing circle
	Point2f featureMean; //mean location of features
	Mat contour; //the contour of this hand
	int circleRadius; //radius of enclosing circle
	float featureStdDev; //standard deviation of features
	RotatedRect minRect;
	bool present;
	int handNumber;

};

#endif /* HAND_H_ */