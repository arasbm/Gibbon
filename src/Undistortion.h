/*
 * Undistortion.h
 *
 *  Created on: 2011-07-07
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

#ifndef UNDISTORTION_H_
#define UNDISTORTION_H_

#include "CameraPGR.h"

class Undistortion {

public:
	Undistortion();
	void undistortImage(cv::Mat* image);
	void calibrateUndistortion(CameraPGR* cam);

private:
	cv::Mat mapx;
	cv::Mat mapy;
	cv::Mat intrinsic;
	cv::Mat distortion;
	cv::Mat newCameraMatrix;

	void saveCalibrationData(cv::Mat intrinsic, cv::Mat distortion, float offsetX, float offsetY, float sizeX, float sizeY);
};

#endif /* UNDISTORTION_H_ */
