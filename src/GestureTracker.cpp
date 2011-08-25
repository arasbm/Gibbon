/*
 * GestureTracekr.cpp
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

#include <math.h>

#include "GestureTracker.h"
#include "GibbonMain.h"
#include "Log.h"

void GestureTracker::checkGestures(vector<Hand>* h) {

	if(checkGrabRelease(h))
		return;

	if(checkRotate(h))
		return;
}

/**
 * Check for grab gesture
 */
bool GestureTracker::checkGrabRelease(vector<Hand>* h) {
	//need at least 3 hands confirming the gesture
	int handsToTrack = 3;
	int min_feature = 5;

	float speedTolerance = 5.0f;
	float stdDevScaleFactor = 1.1f; //expected minimum change in size of std dev

	float grabVectorTolerance = 0.8f;
	//tolerance for difference between feature vector and vector to center of features to count for grab
	//tolerance should be in range [0 2] 0 being exact match and 2 being anything

	float grabPercentTolerance = 0.45f; // smaller is easier to detect but cause more false positives

	float releaseVectorTolerance = 0.7f;
	float releasePercentTolerance = 0.5f;

	for(int i = 0; i < handsToTrack; i++) {
		if(!h->at(previousIndex(i)).isPresent()) {
			return false;
		}
		if(h->at(previousIndex(i)).getNumOfFeatures() < min_feature) {
			return false;
		}
	}

	bool grab = true;
	bool release = true;

	int totalFeatures = 0;
	int movingToCenter = 0;
	int movingFromCenter = 0;

	for(int i=0; i<handsToTrack; i++) {

		Point2f center = h->at(previousIndex(i)).getFeatureMean();

		vector<Point2f> features = h->at(previousIndex(i)).getFeatures();
		vector<Point2f> featVectors = h->at(previousIndex(i)).getVectors();

		totalFeatures += features.size();

		for(int j=0; j<features.size(); j++) {

			//check if features are not diverging, indicating no grab
			if( h->at(previousIndex(i)).getFeatureStdDev() > h->at(previousIndex(i+1)).getFeatureStdDev() * stdDevScaleFactor ) {
				grab = false;
			}
			//check if features are not converging, indicating no release
			if( h->at(previousIndex(i)).getFeatureStdDev() * stdDevScaleFactor < h->at(previousIndex(i+1)).getFeatureStdDev() ) {
				release = false;
			}

			Point2f toCenter = (center - features[j]);
			Point2f direction = featVectors[j];

			float magnitude = sqrt(direction.x*direction.x + direction.y*direction.y);

			if( magnitude > speedTolerance || std::isinf(magnitude) || std::isnan(magnitude) ) {
				//normalize
				direction.x /= magnitude;
				direction.y /= magnitude;
				magnitude = sqrt(toCenter.x*toCenter.x + toCenter.y*toCenter.y);
				toCenter.x /= magnitude;
				toCenter.y /= magnitude;

				Point2f difference = toCenter - direction;
				magnitude = sqrt(difference.x*difference.x + difference.y*difference.y);

//				verbosePrint(boost::lexical_cast<string>(toCenter.x) + "," + boost::lexical_cast<string>(toCenter.y));
//				verbosePrint(boost::lexical_cast<string>(direction.x) + "," + boost::lexical_cast<string>(direction.y));

				if(magnitude < grabVectorTolerance)
					movingToCenter++;

				difference = toCenter + direction;
				magnitude = sqrt(difference.x*difference.x + difference.y*difference.y);

				if(magnitude < releaseVectorTolerance)
					movingFromCenter++;

//				if(magnitude < releaseVectorTolerance)
//					verbosePrint(boost::lexical_cast<string>(magnitude));
			}
		}
	}

	if(movingToCenter / (float) totalFeatures < grabPercentTolerance)
		grab = false;

	if(movingFromCenter / (float) totalFeatures < releasePercentTolerance)
		release = false;

	if(grab) {
		verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) + " >>GRAB<<");
		verbosePrint("grab %: " + boost::lexical_cast<string>(movingToCenter / (float) totalFeatures));
		verbosePrint("Sum Gesture Features: " + boost::lexical_cast<string>(totalFeatures) + "\n");
		h->at(index()).setGesture(GESTURE_GRAB);
		return true;
	}

	if(release) {
		verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) + " >>RELEASE<<");
		verbosePrint("release %: " + boost::lexical_cast<string>(movingFromCenter / (float) totalFeatures));
		verbosePrint("Sum Gesture Features: " + boost::lexical_cast<string>(totalFeatures) + "\n");
		h->at(index()).setGesture(GESTURE_RELEASE);
		return true;
	}

	return false;
}

/**
 * Check rotate gesture
 */
bool GestureTracker::checkRotate(vector<Hand>* h) {
	//need at least 5 hands confirming the gesture
//	int min_feature = 10;
//	static float rot_tolerance = 30;
//	static float area_tolerance = 1000;

//	if(h->at(index()).isPresent() && h->at(previousIndex(1)).isPresent() && h->at(previousIndex(2)).isPresent() ) {
//		if(h->at(index()).getNumOfFeatures() > min_feature &&
//				(h->at(previousIndex(1)).getNumOfFeatures() > min_feature) &&
//				(h->at(previousIndex(2)).getNumOfFeatures() > min_feature) &&
//				(h->at(previousIndex(3)).getNumOfFeatures() > min_feature) &&
//				(h->at(previousIndex(4)).getNumOfFeatures() > min_feature)) {
//
//			float angle0 = h->at(index()).getAngle();
//			float angle1 = h->at(previousIndex(1)).getAngle();
//			float angle2 = h->at(previousIndex(2)).getAngle();
//			float angle3 = h->at(previousIndex(3)).getAngle();
//			float angle4 = h->at(previousIndex(4)).getAngle();
//
//			float area0 = h->at(index()).getMinRect().size.area();
//			float area1 = h->at(previousIndex(1)).getMinRect().size.area();
//			float area2 = h->at(previousIndex(2)).getMinRect().size.area();
//			float area3 = h->at(previousIndex(3)).getMinRect().size.area();
//			float area4 = h->at(previousIndex(4)).getMinRect().size.area();
//
////			verbosePrint(boost::lexical_cast<std::string>(fabs(angle0 - angle4)));
////
////			verbosePrint(boost::lexical_cast<std::string>(angle0) +" "+ boost::lexical_cast<std::string>(angle1) +" "+
////								boost::lexical_cast<std::string>(angle2) +" "+ boost::lexical_cast<std::string>(angle3) +" "+
////								boost::lexical_cast<std::string>(angle4));
////
////			verbosePrint(boost::lexical_cast<std::string>(area0) +" "+ boost::lexical_cast<std::string>(area1) +" "+
////								boost::lexical_cast<std::string>(area2) +" "+ boost::lexical_cast<std::string>(area3) +" "+
////								boost::lexical_cast<std::string>(area4));
//
//			if(fabs(area0-area1) < area_tolerance && fabs(area1-area2) < area_tolerance &&
//					fabs(area2-area3) < area_tolerance && fabs(area3-area4) < area_tolerance) {
//
//				if(angle0 > angle1 && angle1 > angle2 && angle2 > angle3 && angle3 > angle4 &&
//						fabs(angle0 - angle4) > rot_tolerance) {
//
//					verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) +
//												" >>ROTATE<< clockwise");
//					return true;
//
//				} else if(angle0 < angle1 && angle1 < angle2 && angle2 < angle3 && angle3 < angle4 &&
//						fabs(angle0 - angle4) > rot_tolerance) {
//
//					verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) +
//							" >>ROTATE<< counter-clockwise");
//					return true;
//				}
//			}
//		}
//	}
	return false;
}
