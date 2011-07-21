/*
 * Camera.h
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

#ifndef IMAGE_PROVIDER_H_
#define IMAGE_PROVIDER_H_

#include "cv.h"
// TODO: find the right include above and remove the unnecessary ones.

class ImageProvider {
	public:
		virtual cv::Mat grabImage() = 0;
		virtual void init();
		virtual void setROI(cv::Rect roi);
		virtual cv::Rect getROI();

	protected:
		float fps; //TODO: figure out if fps should be set and accessible from here
		cv::Rect roi; //region of interest

};

#endif /* CAMERA_H_ */
