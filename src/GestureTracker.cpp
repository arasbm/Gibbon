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
#include "GestureTracker.h"
#include "GibbonMain.h"
#include "Log.h"

void GestureTracker::checkGestures(vector<Hand>* h) {

	if(checkGrab(h))
			return;

	if(checkRelease(h))
			return;

	if(checkRotate(h))
			return;
}

/**
 * Check for grab gesture
 */
bool GestureTracker::checkGrab(vector<Hand>* h) {
	//need at least 3 hands confirming the gesture
	int factor = 5;
	int min_feature = 5;

	if(h->at(index()).isPresent() &&
			h->at(previousIndex(1)).isPresent() &&
			h->at(previousIndex(2)).isPresent() &&
			h->at(previousIndex(3)).isPresent()) {
		if(h->at(index()).getNumOfFeatures() > min_feature &&
				(h->at(previousIndex(1)).getNumOfFeatures() > min_feature) &&
				(h->at(previousIndex(2)).getNumOfFeatures() > min_feature)) {
			if(h->at(index()).getFeatureStdDev() + factor < h->at(previousIndex(1)).getFeatureStdDev() &&
					(h->at(previousIndex(1)).getFeatureStdDev() + factor < h->at(previousIndex(2)).getFeatureStdDev())) {

				verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) + " >>GRAB<<");
				h->at(index()).setGesture(GESTURE_GRAB);
				return true;
			}
		}
	}
	return false;
}

/**
 * Check for release gesture
 */
bool GestureTracker::checkRelease(vector<Hand>* h) {
	return false;
}

/**
 * Check rotate gesture
 */
bool GestureTracker::checkRotate(vector<Hand>* h) {
	//need at least 5 hands confirming the gesture
	int min_feature = 10;
	static float rot_tolerance = 30;
	static float area_tolerance = 1000;

	if(h->at(index()).isPresent() && h->at(previousIndex(1)).isPresent() && h->at(previousIndex(2)).isPresent() ) {
		if(h->at(index()).getNumOfFeatures() > min_feature &&
				(h->at(previousIndex(1)).getNumOfFeatures() > min_feature) &&
				(h->at(previousIndex(2)).getNumOfFeatures() > min_feature) &&
				(h->at(previousIndex(3)).getNumOfFeatures() > min_feature) &&
				(h->at(previousIndex(4)).getNumOfFeatures() > min_feature)) {

			float angle0 = h->at(index()).getAngle();
			float angle1 = h->at(previousIndex(1)).getAngle();
			float angle2 = h->at(previousIndex(2)).getAngle();
			float angle3 = h->at(previousIndex(3)).getAngle();
			float angle4 = h->at(previousIndex(4)).getAngle();

			float area0 = h->at(index()).getMinRect().size.area();
			float area1 = h->at(previousIndex(1)).getMinRect().size.area();
			float area2 = h->at(previousIndex(2)).getMinRect().size.area();
			float area3 = h->at(previousIndex(3)).getMinRect().size.area();
			float area4 = h->at(previousIndex(4)).getMinRect().size.area();

//			verbosePrint(boost::lexical_cast<std::string>(fabs(angle0 - angle4)));
//
//			verbosePrint(boost::lexical_cast<std::string>(angle0) +" "+ boost::lexical_cast<std::string>(angle1) +" "+
//								boost::lexical_cast<std::string>(angle2) +" "+ boost::lexical_cast<std::string>(angle3) +" "+
//								boost::lexical_cast<std::string>(angle4));
//
//			verbosePrint(boost::lexical_cast<std::string>(area0) +" "+ boost::lexical_cast<std::string>(area1) +" "+
//								boost::lexical_cast<std::string>(area2) +" "+ boost::lexical_cast<std::string>(area3) +" "+
//								boost::lexical_cast<std::string>(area4));

			if(fabs(area0-area1) < area_tolerance && fabs(area1-area2) < area_tolerance &&
					fabs(area2-area3) < area_tolerance && fabs(area3-area4) < area_tolerance) {

				if(angle0 > angle1 && angle1 > angle2 && angle2 > angle3 && angle3 > angle4 &&
						fabs(angle0 - angle4) > rot_tolerance) {

					verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) +
												" >>ROTATE<< clockwise");
					return true;

				} else if(angle0 < angle1 && angle1 < angle2 && angle2 < angle3 && angle3 < angle4 &&
						fabs(angle0 - angle4) > rot_tolerance) {

					verbosePrint("hand#: " + boost::lexical_cast<string>(h->at(index()).getHandNumber()) +
							" >>ROTATE<< counter-clockwise");
					return true;
				}
			}
		}
	}
	return false;
}
