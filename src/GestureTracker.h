/*
 * GestureTracker.h
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

#ifndef GESTURETRACKER_H_
#define GESTURETRACKER_H_

#include "Hand.h"
#include <boost/lexical_cast.hpp>

class GestureTracker {

public:
	static void checkGestures(vector<Hand*> h);

private:
	static bool checkGrabRelease(vector<Hand*> h);
	static bool checkRotate(vector<Hand*> h);
};

#endif /* GESTURETRACKER_H_ */
