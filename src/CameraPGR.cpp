/*
 *  CameraPGR.cpp
 *
 *  Created on: 2011-07-06
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

#include "CameraPGR.h"
#include "FlyCapture2.h"
#include "Setting.h"
#include "Undistortion.h"
#include "highgui.h"

using namespace FlyCapture2;

#define setting Setting::Instance()
Undistortion* undistortion;

CameraPGR::~CameraPGR() {
	pgrCam.StopCapture();
	pgrCam.Disconnect();
}

/**
 * Initialize a PGR camera with format 7 settings
 * */
void CameraPGR::init(int cam_index, bool do_undistortion, bool is_color){
	//if undistortion on, initialize undistortion
    this->do_undistortion = do_undistortion;
    this->is_color = is_color;
    if(this->do_undistortion && setting->do_undistortion) {
		undistortion = new Undistortion();
	}

	//Starts the camera.
	busManager.GetNumOfCameras(&totalCameras);
	printf("Found %d cameras on the bus.\n",totalCameras);

    if(is_color) {
        //terrible hack ... TODO: fix this!!
        fmt7ImageSettings.mode = MODE_0;
        fmt7ImageSettings.offsetX = 44;
        fmt7ImageSettings.offsetY = 52;
        fmt7ImageSettings.width = 668;
        fmt7ImageSettings.height = 376;
        fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_RAW8;
    } else {
        fmt7ImageSettings.mode = MODE_0;
        fmt7ImageSettings.offsetX = 0;
        fmt7ImageSettings.offsetY = 0;
        fmt7ImageSettings.width = setting->pgr_cam_max_width;
        fmt7ImageSettings.height = setting->pgr_cam_max_height;
        fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_MONO8;
    }
    bool valid;

    busManager.GetCameraFromIndex(cam_index, &guid);
	Error pgError;
	pgError = pgrCam.Connect(&guid);
	if (pgError != PGRERROR_OK){
		printf("Error in starting the camera.\n");
		return;
    }

    // Get the camera information
    CameraInfo camInfo;
    pgError = pgrCam.GetCameraInfo( &camInfo );
    if (pgError != PGRERROR_OK)
    {
        cout << "Errot getting camera info";
    }
    PrintCameraInfo(&camInfo);

    // Set the settings to the camera
    if(is_color) {

        //Validate format7 settings
        pgError = pgrCam.ValidateFormat7Settings(
            &fmt7ImageSettings,
            &valid,
            &fmt7PacketInfo );
        if (pgError != PGRERROR_OK)
        {
            cout << "ERROR! in validating format7 settings" << endl;
        }
        if ( !valid )
        {
            // Settings are not valid
            cout << "Format7 settings are not valid" << endl;
        }

        unsigned int recommended_size = 996;
        pgError = pgrCam.SetFormat7Configuration( &fmt7ImageSettings, recommended_size );
    } else {
        //unsigned int recommended_size = 1880;
        //pgError = pgrCam.SetFormat7Configuration(&fmt7ImageSettings, recommended_size );
            //fmt7PacketInfo.recommendedBytesPerPacket );
        pgError = pgrCam.SetVideoModeAndFrameRate(
                    VIDEOMODE_640x480Y8,
                    FRAMERATE_15 );
        if (pgError != PGRERROR_OK)
        {
            cout << "ERROR! not able to set camera framerate";
        }
    }

    if (pgError != PGRERROR_OK)
    {
        cout << "ERROR setting format7" << endl;
    } else {
		pgrCam.StartCapture();
    }
    if (pgError != PGRERROR_OK){
		cout << "Error in starting the camera capture" << endl;
		return;
	}

    // set frame rate property
    Property prop;

    if(is_color) {
        //set framerate
        prop.type = FRAME_RATE;
        prop.autoManualMode = false;
        prop.onOff = true;
        prop.absValue = 15.0;
        pgError = pgrCam.SetProperty( &prop );
        if (pgError != PGRERROR_OK)
        {
            cout << "ERROR setting camera FPS" << endl;
        }
        cout << "Color pgr camera [" << cam_index << "] successfully initialized." << endl;
    } else {
        //Main camera settings ...
        //set brightness
//        prop.type = BRIGHTNESS;
//        prop.autoManualMode = false;
//        prop.onOff = true;
//        prop.absValue = 80.0;
//        pgError = pgrCam.SetProperty( &prop );
//        if (pgError != PGRERROR_OK)
//        {
//            cout << "ERROR setting camera brightness" << endl;
//        }

//        //set shutter
//        prop.type = SHUTTER;
//        prop.autoManualMode = false;
//        prop.onOff = true;
//        prop.absValue = 68.280;
//        pgError = pgrCam.SetProperty( &prop );
//        if (pgError != PGRERROR_OK)
//        {
//            cout << "ERROR setting camera shutter" << endl;
//        }

//        //set Gain
//        prop.type = GAIN;
//        pgError = pgrCam.GetProperty( &prop );
//        prop.autoManualMode = false;
//        prop.onOff = true;
//        prop.absValue = 7.0;
//        pgError = pgrCam.SetProperty( &prop );
//        if (pgError != PGRERROR_OK)
//        {
//            cout << "ERROR setting camera gain" << endl;
//        }

        cout << "Monochrome pgr camera [" << cam_index << "] successfully initialized." << endl;
    }
}

/**
 * Retrieve an image from the camera and return it
 * */
cv::Mat CameraPGR::grabImage(){
    getOpenCVFromPGR();
    big_image = cv::Mat(cv::Size(setting->pgr_cam_max_width, setting->pgr_cam_max_height), CV_8UC1);
    roi_center = big_image(cv::Rect(56, 0, 640, 480));
    image.copyTo(roi_center);
    if(this->do_undistortion && setting->do_undistortion) {
        undistortion->undistortImage(&big_image);
    }
    //TODO: fix this glocal crop. for now cropping all images to the ROI of source image

    if(is_color) {
        cv::flip(image, flippedImage, 1);
        croppedImage = flippedImage(cv::Rect(0, 0, setting->imageSizeX, setting->imageSizeY));
        return croppedImage;
    } else {
        cv::flip(big_image, flippedImage, 1);
        croppedImage = flippedImage(cv::Rect(setting->imageOffsetX, setting->imageOffsetY, setting->imageSizeX, setting->imageSizeY));
        return croppedImage;
    }
}

void CameraPGR::calibrateUndistortionROI() {
	undistortion->settingsLoop(this);
	delete undistortion;
	undistortion = new Undistortion();
}

/**
 * Convert PGR image to appropriate OpenCV image format
 * */
void CameraPGR::getOpenCVFromPGR(){
    pImage.ReleaseBuffer();
    //image.release();

    pgError = pgrCam.RetrieveBuffer(&pImage);
    if (pgError != PGRERROR_OK){
        cout << "Error in grabbing frame." << endl;
    }

    unsigned int rows, cols, stride;
    pImage.GetDimensions( &rows, &cols, &stride, &pixFormat );

    switch ( pixFormat )
	{
		case PIXEL_FORMAT_MONO8:
            //good old single chanel 8bit monochrome
            image = cv::Mat(rows, cols, CV_8UC1 , pImage.GetData());
			break;
		case PIXEL_FORMAT_411YUV8:
		case PIXEL_FORMAT_422YUV8:
		case PIXEL_FORMAT_444YUV8:
		case PIXEL_FORMAT_RGB8:
        case PIXEL_FORMAT_RAW8:
        case PIXEL_FORMAT_BGR:
            //8bit color image
            pImage.Convert(PIXEL_FORMAT_BGR, &colorImage);
            image = cv::Mat(rows, cols, CV_8UC3 , colorImage.GetData());
			break;
		case PIXEL_FORMAT_MONO16:
        case PIXEL_FORMAT_S_MONO16:
            //16bit monochrome image single chanel
            image = cv::Mat(rows, cols, CV_16UC1 , pImage.GetData());
			break;
		case PIXEL_FORMAT_RGB16:
        case PIXEL_FORMAT_RAW16:
		case PIXEL_FORMAT_S_RGB16:
            //16bit color
            pImage.Convert(PIXEL_FORMAT_BGR, &colorImage);
            image = cv::Mat(rows, cols, CV_16UC3 , colorImage.GetData());
			break;
		case PIXEL_FORMAT_MONO12:
        case PIXEL_FORMAT_RAW12:
            cout << "Error: Image format is not supported by OpenCV";
			break;
		case PIXEL_FORMAT_BGRU:
		case PIXEL_FORMAT_RGBU:
            //8bit  four chanel color image
            pImage.Convert(PIXEL_FORMAT_BGRU, &colorImage);
            image = cv::Mat(rows, cols, CV_8UC4 , colorImage.GetData());
			break;
		default:
			cout << "ERROR in detecting image format" << endl;
			break;
	}
}

void CameraPGR::PrintCameraInfo( CameraInfo* pCamInfo ) {
    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n",
        pCamInfo->serialNumber,
        pCamInfo->modelName,
        pCamInfo->vendorName,
        pCamInfo->sensorInfo,
        pCamInfo->sensorResolution,
        pCamInfo->firmwareVersion,
        pCamInfo->firmwareBuildTime );
}
