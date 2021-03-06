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

#define setting Setting::Instance()

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
 * Get the mass center point of this hand
 */
Point2f Hand::getMassCenter() {
    return massCenter;
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
 * it sets the contour of the hand as well as the moments of this contour
 */
void Hand::setContour(vector<cv::Point> handContour) {
    contour = handContour;
    //now set the moment values
    moments = cv::moments(contour, false);
    //set the center of mass
    massCenter = Point2f( moments.m10/moments.m00 , moments.m01/moments.m00 );
}

/**
 * rect should be the enclosing rectangle with minimum area
 * as calculated with cv::minAreaRect function
 */
void Hand::setMinRect(RotatedRect rect) {
     minRect = rect; //RotatedRect(rect.center, rect.size, rect.angle);
}

RotatedRect Hand::getMinRect() {
	return minRect;
}

Point2f Hand::getMinRectCenter() {
	return minRect.center;
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
//	return gestureX;
	if( this->getFeatureMean().x == 0) {
		return (setting->imageSizeX - this->getMinCircleCenter().x) / setting->imageSizeX;
	} else {
		//return (setting->imageSize.width - this->getFeatureMean().x) / setting->imageSize.width;
		float alpha = 0.5f;
		static float oldX = -1;

		float x = (setting->imageSizeX - this->getFeatureMean().x) / setting->imageSizeX;

		if(oldX != -1)
		{
			//TODO: don't use magic number
			alpha = fabs(x - oldX) / setting->imageSizeX * 2500.0f;
			alpha = min(1.0f, alpha);
			x =  alpha*x + (1-alpha)*oldX;
		}

		oldX = x;
		return x;
	}


}

/**
 * Return the Y value of position of hand gesture as a number in the range [0 1]
 * The position depends on the type of gesture and it means the location in which the gesture
 * should be applied to. For example in the case of Grab gesture this may be the intersection
 * of trajectory of most features of the hands.
 */
float Hand::getY() {
//	return gestureY;
	if(this->getFeatureMean().y == 0) {
		//if there are no features use min circle center
		return this->getMinCircleCenter().y / setting->imageSizeY;
	} else {
//		return this->getFeatureMean().y / setting->imageSizeY;
		float alpha = 0.5f;
		static float oldY = -1;

		float y = this->getFeatureMean().y / setting->imageSizeY;

		if(oldY != -1)
		{
			//TODO: don't use magic number
			alpha = fabs(y - oldY) / setting->imageSizeY * 2500.f;
			alpha = min(1.0f, alpha);
			y = alpha*y + (1-alpha)*oldY;
		}

		oldY = y;
		return y;
	}

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
 * if the point is inside the hand contour returns true, otherwise return false
 */
bool Hand::hasPointInside(Point2f point) {
	return cv::pointPolygonTest(Mat(getContour()).reshape(2), point, false) > -1;
}

/**
 * Add location of a features of this hand, the vector associated with it
 * and the approximate depth based on sharpness measurements
 */
void Hand::addFeatureAndVector(Point2f feature, Point2f vector, float depth, Point2f orientation, uchar status) {
	features.push_back(feature);
	vectors.push_back(vector);
	featureDepth.push_back(depth);
	featureOrientation.push_back(orientation);
	flowStatus.push_back(status);
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
 * Return a vector of points representing "Movement Vectors" that are
 * associated with features of this hand
 */
vector<Point2f> Hand::getVectors() {
	return vectors;
}

vector<float> Hand::getFeaturesDepth() {
	return featureDepth;
}

/**
 * Returns the vector containing feature orientation. This is the anatomical orientation
 * Usually the feature represent a finger tip and this vector represents the direction that
 * the finger is pointing at. This is NOT the movement direction.
 */
vector<Point2f> Hand::getFeatureOrientation() {
	return featureOrientation;
}

/**
 * Return the position of feature at specified index i
 */
Point2f Hand::getFeatureAt(int i) {
	return features[i];
}

/**
 * return the flow status of this feature at index i
 * if feature is tracked 1 is returned
 * if feature is NOT tracked this function returns 0
 */
uchar Hand::isFeatureTracked(int i) {
	return flowStatus[i];
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
	featureStdDev = (float)(stdDev.val[0] + stdDev.val[1]) / 2;
}

/**
 * Set this hand presence to false and do any other additional cleanup if necessary
 */
void Hand::clear() {
	setPresent(false);
	handGesture = GESTURE_NONE;
	features.clear();
	featureDepth.clear();
	vectors.clear();
	featureOrientation.clear();
	flowStatus.clear();
	//TODO: Check for anything else I need to do here to prevent error or release memory
}

/**
 * return the matrix containing features of this hand and previous hands in the
 * temporal window
 */
Mat Hand::getFeatureMatrix() {
	return featureMatrix;
}

/**
 * set the featureMatrix to features of this hand and all the previous hands in
 * the temporal window
 */
void Hand::setFeatureMatrix(Mat featureMatrix) {
    this->featureMatrix = featureMatrix;
}

/**
 * return the moment object containing the 10 moment values for the contour of this hand
 */
Moments Hand::getMoments() {
    return moments;
}


