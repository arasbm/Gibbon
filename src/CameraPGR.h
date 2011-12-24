/*
 * CameraPGR.h
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

#ifndef CAMERAPGR_H_
#define CAMERAPGR_H_

#include "FlyCapture2.h"
#include "cv.h"

#include "ImageProvider.h"

using namespace FlyCapture2;


class CameraPGR : public ImageProvider {

public:
	cv::Mat grabImage();
    void init(int cam_index, bool do_undistortion, bool is_color);
	void calibrateUndistortionROI();
	~CameraPGR();

private:
    void getOpenCVFromPGR();
    void PrintCameraInfo( CameraInfo* pCamInfo );
    //unsigned char pMemBuffers;
    PixelFormat pixFormat;
    IplImage* cvImage;
    Error pgError;
    Image pImage;
    Image colorImage; //new image to be referenced by cvImage for converting to color
    cv::Mat image;
    cv::Mat big_image; //bigger (752x480) container for image for undistortion
    cv::Mat roi_center;
    cv::Mat flippedImage;
    cv::Mat croppedImage;
	Camera pgrCam;
	PGRGuid guid;
    Format7ImageSettings fmt7ImageSettings;
    Format7PacketInfo fmt7PacketInfo;
	BusManager busManager;
	unsigned int totalCameras;
    //do_undistortion: if undistortion should be applied to this particular camera.
    //note that there is also a global setting->do_undistortion that applies to all cameras
    bool do_undistortion;
    bool is_color;
};

#endif /* CAMERAPGR_H_ */
