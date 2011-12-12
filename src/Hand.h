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
#include <utility>

using namespace cv;

typedef enum _handSide {
	LEFT_HAND,
	RIGHT_HAND
} handSide;

typedef enum _gesture {
	GESTURE_NONE = 0,
	GESTURE_GRAB = 1,
	GESTURE_RELEASE = 2,
	GESTURE_TWIST = 3
} gesture;

class Hand {

public:
	Hand(handSide s);
	handSide getHandSide();
	bool isPresent();
	void setPresent(bool present);
	void setMinRect(RotatedRect minRect);
	RotatedRect getMinRect();
	Point2f getMinRectCenter();
	void setMinCircleCenter(Point2f center);
	Point2f getMinCircleCenter();
	void setMinCircleRadius(int radius);
	int getMinCircleRadius();
	vector<cv::Point> getContour();
	void setContour(vector<cv::Point> contour);
	int getHandNumber();
	void clear();
	gesture getGesture();
	void setGesture(gesture g);
	int handMessageID();
	float getX();
	float getY();
	float getAngle();
	void setFeatureMeanStdDev(Point2f mean, float stdDev);
	Point2f getFeatureMean();
	float getFeatureStdDev();
	void setNumOfFeatures(int numOfFeatures);
	int getNumOfFeatures();
	bool hasPointInside(Point2f point);
	void addFeatureAndVector(Point2f feature, Point2f vector, float depth, Point2f orientation, uchar flowStatus);
	vector<Point2f> getFeatures();
	vector<Point2f> getVectors();
	vector<float> getFeaturesDepth();
	vector<Point2f> getFeatureOrientation();
	Point2f getFeatureAt(int i);
	uchar isFeatureTracked(int i);
	void calcMeanStdDev();
	bool hasGesture();
	Mat getFeatureMatrix();
        void setFeatureMatrix(Mat featureMatrix);
        Moments getMoments();

private:
	handSide side;
	gesture handGesture;
	Point2f circleCenter; //center of enclosing circle
	vector<cv::Point> contour;
	vector<Point2f> features; //location of features of this hand
	vector<uchar> flowStatus; //set to 1 if the flow for the corresponding features has been found, 0 otherwise
	vector<Point2f> vectors; //vector associated with features of this hand that represent the direction in which the feature is moving
	vector<float> featureDepth; //relative depth of current feature based on sharpness of its region. Higher means closer to screen
	vector<Point2f> featureOrientation; //Orientation of feature. In case of finger tip it is the normalized 2D projection of the vector in direction the finger is pointing at
	Point2f featureMean; //mean location of features
	float featureStdDev; //standard deviation of features

	int circleRadius; //radius of enclosing circle
	RotatedRect minRect;
	bool present;
	int handNumber;
	float gestureX; //X position of the gesture in the range [0 1]
	float gestureY; //Y position of the gesture in the range [0 1]
	float angle; //the angle in the range [0 2PI]
	//int numOfFeatures; //number of features detected that belong to this hand

        Moments moments; //object containing moments of the contour of this hand
        Mat featureMatrix; //the matrix containing features of this hand and any other previous hand inside the temporal window
};

#endif /* HAND_H_ */
