/*
 * ImageProvider.cpp
 *
 *  Created on: Jul 5, 2011
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

#include "ImageProvider.h"

void ImageProvider::init() {

}

cv::Rect ImageProvider::getROI() {
	return roi;
}

/**
 * Set the rectangular region of interest obtained from the original image
 * from a camera or video stream
 * */
void ImageProvider::setROI(cv::Rect roiRect) {
	roi = roiRect;
}
