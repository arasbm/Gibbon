/*
 * ImageUtils.cpp
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

#include "ImageUtils.h"
#include "Setting.h"
#include "cv.h"

using namespace cv;

static Setting* setting = Setting::Instance();

/**
 * Experimental
 * Precondition: ksize (sobel kernel size) must be odd and not larger than 31
 */
void depthFromDiffusion(Mat sourceImg, Mat depthImg) {
	// blockSize define the window to consider around each pixel, so higher number produces larger blocks in the image
	cv::cornerMinEigenVal(sourceImg, depthImg, 10, 9);
}

/**
 * Rotate specified image by specified angle in degrees
 */
void rotateImage(Mat* src, Mat* dst, float degrees)
{
	   /// Compute a rotation matrix with respect to the center of the image
	   Point center = Point(src->cols/2, src->rows/2);

	   /// Get the rotation matrix with the specifications above
	   Mat rot_mat = getRotationMatrix2D(center, degrees, 1.0f);

	   /// Rotate the warped image
	   warpAffine(*src, *dst, rot_mat, src->size());
}
