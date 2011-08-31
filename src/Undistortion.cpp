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

#include "stdio.h"
#include "boost/regex.hpp"
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "ml.h"

using namespace std;
using namespace cv;

static Setting* setting = Setting::Instance();

//used for calibration GUI interaction
Point windowTopLeft = Point(-1,-1);
Point windowBottomRight = Point(-1,-1);

//Mouse Callback for GUI interaction
void on_mouse(int event, int x, int y, int flags, void* param) {

	static bool selecting = false;
	static Point clickPos = Point(-1,-1);

	if (event == CV_EVENT_LBUTTONDOWN)
	{
		windowTopLeft.x = x;
		windowTopLeft.y = y;
		windowBottomRight.x = x;
		windowBottomRight.y = y;
		clickPos.x = x;
		clickPos.y = y;
		selecting = true;
	}

	if(selecting) {

		if(x > clickPos.x) {
			windowBottomRight.x = x;
		} else {
			windowTopLeft.x = x;
		}

		if(y > clickPos.y) {
			windowBottomRight.y = y;
		} else {
			windowTopLeft.y = y;
		}
	}

	if (event == CV_EVENT_LBUTTONUP)
	{
		selecting = false;
	}
}

void load_file(string& s, istream& is)
{
   s.erase();
   if(is.bad()) return;
   s.reserve(is.rdbuf()->in_avail());
   char c;
   while(is.get(c))
   {
      if(s.capacity() == s.size())
         s.reserve(s.capacity() * 3);
      s.append(1, c);
   }
}

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
	newCameraMatrix = getOptimalNewCameraMatrix(intrinsic, distortion, Size(setting->pgr_cam_max_width,setting->pgr_cam_max_height), setting->undistortion_factor);
	//newCameraMatrix = getDefaultNewCameraMatrix(intrinsic, Size(setting->pgr_cam_max_width,setting->pgr_cam_max_height), true);
}

/**
 * Process the image to remove lens distortion by remapping the image
 * based on stored calibration data
 * */
void Undistortion::undistortImage(Mat* image){
	//TODO: check to see if cloning here is necessary
	Mat distorted = image->clone();
	//cvRemap(image, undistImage, mapx, mapy);
	undistort(distorted, *image, intrinsic, distortion, newCameraMatrix);
	//cvUndistort2(image, undistImage, intrinsic, distortion, newCameraMatrix);
}

void Undistortion::saveCalibrationData(Mat intrinsic, Mat distortion, float offsetX, float offsetY, float sizeX, float sizeY) {

	FileStorage fsIntrinsic("Intrinsics.xml", FileStorage::WRITE);
	fsIntrinsic << "Intrinsics" << intrinsic;
	fsIntrinsic.release();

	FileStorage fsDistortion("Distortions.xml", FileStorage::WRITE);
	fsDistortion << "Distortions" << distortion;
	fsDistortion.release();

	setting->imageOffsetX = offsetX;
	setting->imageOffsetY = offsetY;
	setting->imageSizeX = sizeX;
	setting->imageSizeY= sizeY;
	setting->do_undistortion = true;

	string configFilepath = setting->config_file_path;
	string tempConfigFilepath = configFilepath + string(".tmp");

	FILE* fpIN = fopen(configFilepath.c_str(), "r");
	FILE* fpOUT = fopen(tempConfigFilepath.c_str(), "w");

	if(fpIN == NULL || fpOUT == NULL) {
		cout << "Error reading/writing to config file" << endl;
		exit(-1);
	}

	const int maxLineSize = 256;
	char buff [maxLineSize];

	boost::regex expOffsetX("imageOffsetX[[:space:]]*=[[:space:]]*[[:digit:]]*\.?[[:digit:]]*\n");
	string replaceOffsetX(string("imageOffsetX = ") + boost::lexical_cast<string>(offsetX) + string("\n"));

	boost::regex expOffsetY("imageOffsetY[[:space:]]*=[[:space:]]*[[:digit:]]*\.?[[:digit:]]*\n");
	string replaceOffsetY(string("imageOffsetY = ") + boost::lexical_cast<string>(offsetY) + string("\n"));

	boost::regex expSizeX("imageSizeX[[:space:]]*=[[:space:]]*[[:digit:]]*\.?[[:digit:]]*\n");
	string replaceSizeX(string("imageSizeX = ") + boost::lexical_cast<string>(sizeX) + string("\n"));

	boost::regex expSizeY("imageSizeY[[:space:]]*=[[:space:]]*[[:digit:]]*\.?[[:digit:]]*\n");
	string replaceSizeY(string("imageSizeY = ") + boost::lexical_cast<string>(sizeY) + string("\n"));

	boost::regex expUndistortion("do-undistortion[[:space:]]*=[[:space:]]*[[:digit:]]*\n");
	string replaceUndistortion("do-undistortion = 1\n");


	while(fgets(buff, maxLineSize, fpIN)) {

		string line = string(buff);
		line = boost::regex_replace(line, expOffsetX, replaceOffsetX, boost::match_default);
		line = boost::regex_replace(line, expOffsetY, replaceOffsetY, boost::match_default);
		line = boost::regex_replace(line, expSizeX, replaceSizeX, boost::match_default);
		line = boost::regex_replace(line, expSizeY, replaceSizeY, boost::match_default);
		line = boost::regex_replace(line, expUndistortion, replaceUndistortion, boost::match_default);

		fputs(line.c_str(), fpOUT);
	}

	int success = 0;

	success += remove(configFilepath.c_str());
	success += rename(tempConfigFilepath.c_str(), configFilepath.c_str());
	success += fclose(fpIN);
	success += fclose(fpOUT);

	if(success != 0) {
		cout << "Error reading/writing to config file" << endl;
		exit(-1);
	}
}

void Undistortion::calibrateUndistortion(CameraPGR* cam) {

	bool original_do_undistortion = setting->do_undistortion;
	float original_imageOffsetX = setting->imageOffsetX;
	float original_imageOffsetY = setting->imageOffsetY;
	float original_imageSizeX = setting->imageSizeX;
	float original_imageSizeY = setting->imageSizeY;

	setting->do_undistortion = false;
	setting->imageOffsetX = 0;
	setting->imageOffsetY = 0;
	setting->imageSizeX = setting->pgr_cam_max_width;
	setting->imageSizeY = setting->pgr_cam_max_height;

	while(true) {
		int numBoards = 6;
		int numCornersH = 8;
		int numCornersV = 6;

		int numSquares = numCornersH*numCornersV;
		Size board_size = Size(numCornersH, numCornersV);

		vector< vector<Point3f> > object_points;
		vector< vector<Point2f> > image_points;

		vector<Point2f> corners;
		int successes = 0;

		Mat image;
		Mat corners_image;
		Mat final_corners;
		image = cam->grabImage();
		corners_image = Mat(image.rows, image.cols, CV_8UC1);
		final_corners = Mat(image.rows, image.cols, CV_8UC1);
		final_corners = (const Scalar) 0;


		vector<Point3f> obj;
		for(int i=0;i<numSquares;i++) {
			obj.push_back(Point3f(i/numCornersH, i%numCornersH, 0.0f));
		}

		printf("\n<<Begin Calibration>>\n\n");

		while(successes < numBoards) {
			corners_image = (const Scalar) 0;
			threshold(image, image, setting->lower_threshold, setting->upper_threshold, THRESH_OTSU);

			bool found = findChessboardCorners(image, board_size, corners, 0);

			if(found) {
				cornerSubPix(corners_image, corners, Size(11,11), Size(-1,-1), TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));
				drawChessboardCorners(corners_image, board_size, Mat(corners, true), found);
			}

			imshow("Camera", image);
			imshow("Detected Corners", corners_image);

			image = (const Scalar) 0;
			image = cam->grabImage();

			char key = waitKey(20);

			switch(key) {

			case ' ':
				//capture corners
				if(found) {
					successes++;

					image_points.push_back(corners);
					object_points.push_back(obj);
					drawChessboardCorners(final_corners, board_size, Mat(corners, true), found);
					imshow("Captured Corners", final_corners);
					printf("Board %d Captured\n", successes);
				}
				break;

			case 'r':
				//restart calibration
				successes = 0;
				final_corners = (const Scalar) 0;
				image_points.clear();
				object_points.clear();
				cvDestroyWindow("Captured Corners");
				printf("\n<<Restarting Calibration>>\n\n");
				break;

			case 'q':
				//quit calibration
				cvDestroyWindow("Camera");
				cvDestroyWindow("Detected Corners");
				cvDestroyWindow("Captured Corners");
				//revert altered settings
				setting->do_undistortion = original_do_undistortion;
				setting->imageOffsetX = original_imageOffsetX;
				setting->imageOffsetY = original_imageOffsetY;
				setting->imageSizeX = original_imageSizeX;
				setting->imageSizeY = original_imageSizeY;
				return;
			}
		}

		cvDestroyWindow("Camera");
		cvDestroyWindow("Detected Corners");
		cvDestroyWindow("Captured Corners");

		Mat intrinsic = Mat(3, 3, CV_32FC1);
		Mat distortion;
		vector<Mat> rvecs;
		vector<Mat> tvecs;
		intrinsic.ptr<float>(0)[0] = 1;
		intrinsic.ptr<float>(1)[1] = 1;
		calibrateCamera(object_points, image_points, image.size(), intrinsic, distortion, rvecs, tvecs);

		printf("\n<<Calibration Complete>>\n\n");

		Mat imageUndistorted;

		bool exitLoop = false;

		cvNamedWindow("Undistorted/Cropped Image", CV_WINDOW_AUTOSIZE);
		cvSetMouseCallback("Undistorted/Cropped Image", on_mouse);

		float offsetX = 0;
		float offsetY = 0;
		float sizeX = setting->imageSizeX;
		float sizeY = setting->imageSizeY;

		newCameraMatrix = getOptimalNewCameraMatrix(intrinsic, distortion,
				Size(setting->pgr_cam_max_width, setting->pgr_cam_max_height), setting->undistortion_factor);

		while(exitLoop == false) {

			bool settings_saved = false;

			image = cam->grabImage();
			undistort(image, imageUndistorted, intrinsic, distortion, newCameraMatrix);

			Rect cropPreviewRect = Rect(offsetX, offsetY, sizeX, sizeY);
			Mat cropPreview = imageUndistorted(cropPreviewRect);
			rectangle(cropPreview, windowTopLeft, windowBottomRight, ORANGE, 3);

			imshow("Original Image", image);
			imshow("Undistorted/Cropped Image", cropPreview);

			char key = waitKey(1);

			switch(key) {

			case 'c':
				//set cropping bounds
				if((windowBottomRight.x - windowTopLeft.x) > 0 &&
						(windowBottomRight.y - windowTopLeft.y) > 0) {
					//only set if cropping window is valid
					offsetX += windowTopLeft.x;
					offsetY += windowTopLeft.y;
					sizeX = windowBottomRight.x - windowTopLeft.x;
					sizeY = windowBottomRight.y - windowTopLeft.y;

					windowTopLeft.x = -1;
					windowTopLeft.y = -1;
					windowBottomRight.x = -1;
					windowBottomRight.y = -1;
					printf("<<Window Resized>>\n\n");

					//sending this mouse event resets resizing window
					on_mouse( CV_EVENT_LBUTTONUP, -1, -1, 0, NULL);
				}
				break;

			case 'x':
				offsetX = 0;
				offsetY = 0;
				sizeX = setting->imageSizeX;
				sizeY = setting->imageSizeY;
				windowTopLeft.x = -1;
				windowTopLeft.y = -1;
				windowBottomRight.x = -1;
				windowBottomRight.y = -1;
				printf("<<Window Size Reset>>\n\n");

				//sending this mouse event resets resizing window
				on_mouse( CV_EVENT_LBUTTONUP, -1, -1, 0, NULL);
				break;

			case 's':
				//save settings and quit
				saveCalibrationData(intrinsic, distortion, offsetX, offsetY, sizeX, sizeY);
				settings_saved = true;
				printf("<<Matrix/ROI Data Saved>>\n\n");
				cvDestroyWindow("Original Image");
				cvDestroyWindow("Undistorted/Cropped Image");
				return;

			case 'r':
				//recalibrate
				exitLoop = true;
				cvDestroyWindow("Original Image");
				cvDestroyWindow("Undistorted/Cropped Image");
				printf("<<Recalibrating>>\n\n");
				break;

			case 'q':
				//quit calibration

				if(!settings_saved) {
					//revert altered settings
					setting->do_undistortion = original_do_undistortion;
					setting->imageOffsetX = original_imageOffsetX;
					setting->imageOffsetY = original_imageOffsetY;
					setting->imageSizeX = original_imageSizeX;
					setting->imageSizeY = original_imageSizeY;
				}

				cvDestroyWindow("Original Image");
				cvDestroyWindow("Undistorted/Cropped Image");
				return;
			}
		}
	}
}

