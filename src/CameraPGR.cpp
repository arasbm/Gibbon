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

using namespace FlyCapture2;

static Setting* setting = Setting::Instance();

CameraPGR::~CameraPGR() {
	pgrCam.StopCapture();
	pgrCam.Disconnect();
}

/**
 * Initialize a PGR camera with format 7 settings
 * */
void CameraPGR::init(){
	//Starts the camera.
	busManager.GetNumOfCameras(&totalCameras);
	printf("Found %d cameras on the bus.\n",totalCameras);
	fmt7ImageSettings.mode = MODE_0;
	fmt7ImageSettings.offsetX = setting->imageOffsetX;
	fmt7ImageSettings.offsetY = setting->imageOffsetY;
	fmt7ImageSettings.width = setting->imageSizeX;
	fmt7ImageSettings.height = setting->imageSizeY;
	fmt7ImageSettings.pixelFormat = PIXEL_FORMAT_MONO8;
    bool valid;

    busManager.GetCameraFromIndex(setting->pgr_cam_index, &guid);
	Error pgError;
	pgError = pgrCam.Connect(&guid);
	if (pgError != PGRERROR_OK){
		printf("Error in starting the camera.\n");
		return;
	}

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

    // Set the settings to the camera
    pgError = pgrCam.SetFormat7Configuration(
        &fmt7ImageSettings,
        fmt7PacketInfo.recommendedBytesPerPacket );
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
	cout << "pgr camera successfully initialized." << endl;
}

/**
 * Retrieve an image from the camera and return it
 * */
cv::Mat CameraPGR::grabImage(){
	Error pgError;
	Image rawImage;
	pgError = pgrCam.RetrieveBuffer(&rawImage);
	if (pgError != PGRERROR_OK){
		cout << "Error in grabbing frame." << endl;
	}

	//convert to opencv IplImage > undistort > convert to Mat
	cv::Mat image(convertImageToOpenCV(&rawImage));
	return image;
}

/**
 * Convert PGR image to appropriate OpenCV image format
 * */
IplImage* CameraPGR::convertImageToOpenCV(Image* pImage){
	IplImage* cvImage = NULL;
	bool bColor = true;
	CvSize mySize;
	mySize.height = pImage->GetRows();
	mySize.width = pImage->GetCols();

	switch ( pImage->GetPixelFormat() )
	{
		case PIXEL_FORMAT_MONO8:
			cvImage = cvCreateImageHeader(mySize, 8, 1 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 1;
			bColor = false;
			break;
		case PIXEL_FORMAT_411YUV8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_422YUV8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_444YUV8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_RGB8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_MONO16:
			cvImage = cvCreateImageHeader(mySize, 16, 1 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 1;
			bColor = false;
			break;
		case PIXEL_FORMAT_RGB16:
			cvImage = cvCreateImageHeader(mySize, 16, 3 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_S_MONO16:
			cvImage = cvCreateImageHeader(mySize, 16, 1 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 1;
			bColor = false;
			break;
		case PIXEL_FORMAT_S_RGB16:
			cvImage = cvCreateImageHeader(mySize, 16, 3 );
			cvImage->depth = IPL_DEPTH_16U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_RAW8:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_RAW16:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_MONO12:
			//"Image format is not supported by OpenCV"
			bColor = false;
			break;
		case PIXEL_FORMAT_RAW12:
			//"Image format is not supported by OpenCV
			break;
		case PIXEL_FORMAT_BGR:
			cvImage = cvCreateImageHeader(mySize, 8, 3 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 3;
			break;
		case PIXEL_FORMAT_BGRU:
			cvImage = cvCreateImageHeader(mySize, 8, 4 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 4;
			break;
		case PIXEL_FORMAT_RGBU:
			cvImage = cvCreateImageHeader(mySize, 8, 4 );
			cvImage->depth = IPL_DEPTH_8U;
			cvImage->nChannels = 4;
			break;
		default:
			cout << "ERROR in detecting image format" << endl;
			break;
	}

	if(bColor)
	{
		Image colorImage; //new image to be referenced by cvImage
		colorImage.SetData(new unsigned char[pImage->GetCols() * pImage->GetRows()*3], pImage->GetCols() * pImage->GetRows() * 3);
		pImage->Convert(PIXEL_FORMAT_BGR, &colorImage); //needs to be as BGR to be saved
		cvImage->width = colorImage.GetCols();
		cvImage->height = colorImage.GetRows();
		cvImage->widthStep = colorImage.GetStride();
		cvImage->origin = 0; //interleaved color channels
		cvImage->imageDataOrigin = (char*)colorImage.GetData(); //DataOrigin and Data same pointer, no ROI
		cvImage->imageData         = (char*)(colorImage.GetData());
		cvImage->widthStep              = colorImage.GetStride();
		cvImage->nSize = sizeof (IplImage);
		cvImage->imageSize = cvImage->height * cvImage->widthStep;
	}
	else
	{
		cvImage->imageDataOrigin = (char*)(pImage->GetData());
		cvImage->imageData         = (char*)(pImage->GetData());
		cvImage->widthStep         = pImage->GetStride();
		cvImage->nSize             = sizeof (IplImage);
		cvImage->imageSize         = cvImage->height * cvImage->widthStep;
		//at this point cvImage contains a valid IplImage
	}

	return cvImage;
}
