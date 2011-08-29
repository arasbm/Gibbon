/*
 * Undistortion.cpp
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

#include "Undistortion.h"
#include "Setting.h"
#include "Log.h"
#include "cv.h"
#include "cxcore.h"

using namespace cv;

static Setting* setting = Setting::Instance();

/**
 * Initialize distortion calibration data by loading the necessary files
 * from hard drive.
 * */
Undistortion::Undistortion() {
	FileStorage fsIntrinsic("Intrinsics.xml", FileStorage::READ);
	fsIntrinsic["Intrinsics"] >> intrinsic;
	FileStorage fsDistortion("Distortions.xml", FileStorage::READ);
	fsDistortion["Distortions"] >> distortion;
	fsIntrinsic.release();
	fsDistortion.release();
	//newCameraMatrix = getOptimalNewCameraMatrix(intrinsic, distortion, setting->imageSize, setting->undistortion_factor);
	newCameraMatrix = getDefaultNewCameraMatrix(intrinsic, Size(setting->imageSizeX,setting->imageSizeY), true);
}

/**
 * Process the image to remove lens distortion by remapping the image
 * based on stored calibration data
 * */
void Undistortion::undistortImage(Mat image){
	//TODO: check to see if cloning here is necessary
	Mat undistImage = image.clone();
	//cvRemap(image, undistImage, mapx, mapy);
	cv::undistort(undistImage, image, intrinsic, distortion, newCameraMatrix);
	//cvUndistort2(image, undistImage, intrinsic, distortion, newCameraMatrix);
}

