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

#include "cv.h"
#include "Hand.h"
#include "Setting.h"
#include <utility>

using namespace std;

Setting setting = Setting::Instance();

Hand::Hand(handSide s) {
	static int handCount;
	side = s;
	present = false;
	handNumber = handCount;
	handGesture = GESTURE_NONE;
	handCount++;
}

/**
 * Return LEFT_HAND or RIGHT_HAND
 */
handSide Hand::getHandSide() {
	return side;
}

/**
 * If the hand is still being tracked returns true
 * if it is out of view and so Hand::clear() has been called returns false
 */
bool Hand::isPresent() {
	return present;
}

/**
 * set whether this hand is being tracked or has gone out of view
 */
void Hand::setPresent(bool p) {
	present = p;
}

/**
 * return the contour of this hand
 */
vector<cv::Point> Hand::getContour() {
	return contour;
}

/**
 * This function will receive a vector<cv::Point>  as input parameter
 */
void Hand::setContour(vector<cv::Point> handContour) {
	contour = handContour;
}

/**
 * rect should be the enclosing rectangle with minimum area
 * as calculated with cv::minAreaRect function
 */
void Hand::setMinRect(RotatedRect rect) {
	minRect = RotatedRect(rect.center, rect.size, rect.angle + 90);
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

void Hand::setGesture(gesture g) {
	handGesture = g;
}

gesture Hand::getGesture() {
	return handGesture;
}

/**
 * The message combines hand ID with the current gesture into one int.
 * Assuming 32 bit integer the most left right 8 bits are used for gesture ID and
 * the second 8 bits are used for Hand ID. So a 32 bit integer can be divided into four bytes:
 *        [unused][unused][HandID][GestureID]
 */
int Hand::handMessageID() {
	return (handNumber << 8) | getGesture();
}

/**
 * Return the X value of position of hand gesture as a number in the range [0 1]
 * The position depends on the type of gesture and it means the location in which the gesture
 * should be applied to. For example in the case of Grab gesture this may be the intersection
 * of trajectory of most features of the hands.
 */
float Hand::getX() {
	//return gestureX;
	//return (setting.imageSize.width - this->getMinCircleCenter().x) / setting.imageSize.width;
	float alpha = 0.5f;
	static float oldX = -1;

	float x = (setting.imageSize.width - this->getFeatureMean().x) / setting.imageSize.width;

	if(oldX != -1)
	{
		alpha = fabs(x - oldX) / setting.imageSize.width * 2500.f;
		alpha = min(1.0f, alpha);
		x =  alpha*x + (1-alpha)*oldX;
	}

	oldX = x;
	return x;
}

/**
 * Return the Y value of position of hand gesture as a number in the range [0 1]
 * The position depends on the type of gesture and it means the location in which the gesture
 * should be applied to. For example in the case of Grab gesture this may be the intersection
 * of trajectory of most features of the hands.
 */
float Hand::getY() {
	//return gestureY;
	//return this->getMinCircleCenter().y / setting.imageSize.height;
	float alpha = 0.5f;
	static float oldY = -1;

	float y = this->getFeatureMean().y / setting.imageSize.height;

	if(oldY != -1)
	{
		alpha = fabs(y - oldY) / setting.imageSize.height * 2500.f;
		alpha = min(1.0f, alpha);
		y = alpha*y + (1-alpha)*oldY;
	}

	oldY = y;
	return y;
}

/**
 * The angle of this hand as a float in the range [0 2PI]
 */
float Hand::getAngle() {
	//return angle;
	return this->minRect.angle;
}


/**
 * Set standard deviation and location of mean for features belonging to this hand
 */
void Hand::setFeatureMeanStdDev(Point2f mean, float stdDev) {
	featureMean = mean;
	featureStdDev = stdDev;
}

/**
 * return the mean location of this hand
 * @Precondition: setFeatureMeanStdDev has been called
 */
Point2f Hand::getFeatureMean() {
	return featureMean;
}

/**
 * return the standard deviation of features
 */
float Hand::getFeatureStdDev() {
	return featureStdDev;
}

/**
 * set the number of features that belong to this hand
 */
void Hand::setNumOfFeatures(int nof) {
	numOfFeatures = nof;
}

/**
 * if the point is inside the hand contour returns true, otherwise return false
 */
bool Hand::hasPointInside(Point2f point) {
	return cv::pointPolygonTest(Mat(getContour()).reshape(2), point, false) > -1;
}

/**
 * add location of a features of this hand and the vector associated with it
 */
void Hand::addFeatureAndVector(Point2f feature, Point2f vector) {
	features.push_back(feature);
	vectors.push_back(vector);
}

/**
 * return the number of features associated with this hand
 * @Precondition: addFeatureAndVector has been called for all the features of this hand
 */
int Hand::getNumOfFeatures() {
	return features.size();
}

/**
 * return a list of features and their associated vectors
 */
vector<Point2f> Hand::getFeatures() {
	return features;
}

/**
 * Return a vector of points representing "Vectors" associated with features of this hand
 */
vector<Point2f> Hand::getVectors() {
	return vectors;
}

/**
 * return true if there is any gesture assigned to this hand
 */
bool Hand::hasGesture() {
	return (this->getGesture() != GESTURE_NONE);
}

/**
 * This function needs to be called one time after all the features have been added.
 * After calling this function you can use getFeatureMean() and getFeatureStdDev()
 */
void Hand::calcMeanStdDev() {
	cv::Scalar mean;
	cv::Scalar stdDev;
	cv::meanStdDev(Mat(getFeatures()).reshape(2), mean, stdDev);
	featureMean.x = (float)mean.val[0];
	featureMean.y = (float)mean.val[1];
	featureStdDev = (float)(stdDev.val[0] + stdDev.val[1]);
}

/**
 * Set this hand presence to false and do any other additional cleanup if necessary
 */
void Hand::clear() {
	setPresent(false);
	handGesture = GESTURE_NONE;
	features.clear();
	vectors.clear();
	//TODO: Check for anything else I need to do here to prevent error or release memory
}
