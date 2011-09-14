/***********************************************************************
 *  Application : Gibbon (A Bimanual Near Touch Tracker)
 *  Author      : Aras Balali Moghaddam
 *  Version     : 0.1
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
 ************************************************************************/
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <sstream>
#include <exception>

//Standard includes
#include <cmath>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>

#include "GibbonMain.h"
#include "GestureTracker.h"
#include "Setting.h"
#include "CameraPGR.h"
#include "Log.h"
#include "Hand.h"
#include "Message.h"
#include "ImageUtils.h"

using namespace std;
using namespace cv;

/** OpenCV variables **/
CvPoint mouseLocation;
VideoWriter sourceWriter;
VideoWriter resultWriter;

/** Hand tracking structures [temporal tracking window] **/
const uint hand_window_size = 5; //Number of frames to keep track of hand. Minimum of two is needed
vector<Hand> handOne(hand_window_size, Hand(LEFT_HAND)); //circular: see index() function
vector<Hand> handTwo(hand_window_size, Hand(RIGHT_HAND)); //circular: see index() function

/** goodFeaturesToTrack structure and settings **/
vector<Point2f> previousCorners;
bool noPreviousCorners = true;
vector<Point2f> currentCorners; //Centre point of feature or corner rectangles
vector<float> featureDepth; //depth of current corners as calculated by featureDepthExtract function
vector<uchar> flowStatus; //set to 1 if the flow for the corresponding features has been found, 0 otherwise
vector<float> flowCount; //number of times the flow of this feature has been detected
//vector<uchar> leftRightStatus; // 0=None, 1=Left, 2=Right
vector<float> flowError;
//TermCriteria termCriteria = TermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 );
TermCriteria termCriteria = TermCriteria( CV_TERMCRIT_NUMBER | CV_TERMCRIT_EPS, 10, 0.3);
double derivLambda = 0; //proportion for impact of "image intensity" as opposed to "derivatives"
int maxCorners = 5;
double qualityLevel = 0.001;//0.01;
double minDistance = 10;
int blockSize = 26;
bool useHarrisDetector = false; //its either harris or cornerMinEigenVal

CameraPGR pgrCamera;
Message message; //used by updateMessage() and inside the main loop
int frameCount = 0;
static Setting* setting = Setting::Instance();

int main(int argc, char* argv[]) {
	if(!setting->loadOptions(argc, argv)) {
		cout << "Error in loading options. Exiting the program." << endl;
		return -1;
	}
	verbosePrint("Starting ... ");

	if(!setting->is_daemon) {
		cvNamedWindow( "Source", CV_WINDOW_AUTOSIZE ); 		//monochrome source
		//cvNamedWindow( "Processed", CV_WINDOW_AUTOSIZE ); 	//monochrome image after pre-processing
		cvNamedWindow( "Tracked", CV_WINDOW_NORMAL); 	//Color with pretty drawings showing tracking results
		cvSetWindowProperty("Tracked", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

	}
	// Mat colorImg; //color image for connected component labelling

	//print out key functions
	printKeys();

	init();
	start();

	if(!setting->is_daemon) {
		cvDestroyWindow( "Source" );
		cvDestroyWindow( "Tracked" );

		//cvDestroyWindow( "Processed" );
		verbosePrint("Bye bye!");
	}
	return 0;
}

/**
 * Initialize global variables
 */
void init() {
	//Do nothing for now
}

/**
 * Check if there are any hands and add current hand gestures to global object
 * "message"
 */
void updateMessage() {
	int stepsBack = 1;
	if(handOne.at(index()).isPresent() && (!handOne.at(previousIndex()).isPresent())) {
		//New hand!
		if(handOne.at(previousIndex()).hasGesture()) {
			//go back 4 step to get closer to initial location gesture started at
			handOne.at(previousIndex(stepsBack)).setGesture(handOne.at(previousIndex()).getGesture());
			message.newHand(handOne.at(previousIndex(stepsBack)));
		} else {
			message.newHand(handOne.at(index()));
		}
	} else if(handOne.at(index()).isPresent()) {
		//Update existing hand
		if(handOne.at(index()).hasGesture()) {
			message.removeHand(handOne.at(index()));
			handOne.at(index()).setPresent(false);
		} else {
			message.updateHand(handOne.at(index()));
		}
	} else if((!handOne.at(index()).isPresent()) && handOne.at(previousIndex()).isPresent()) {
		//ask for remove
		message.removeHand(handOne.at(index()));
	} else {
		//Peace and quiet here. Nothing to do.
	}

	if(handTwo.at(index()).isPresent() && (!handTwo.at(previousIndex()).isPresent())) {
		//New hand!
		if(handTwo.at(previousIndex()).hasGesture()) {
			//go back 4 step to get closer to initial location gesture started at
			handTwo.at(previousIndex(stepsBack)).setGesture(handTwo.at(previousIndex()).getGesture());
			message.newHand(handTwo.at(previousIndex(stepsBack)));
		} else {
			message.newHand(handTwo.at(index()));
		}
	} else if(handTwo.at(index()).isPresent()) {
		//Update existing hand
		if(handTwo.at(index()).hasGesture()) {
			message.removeHand(handTwo.at(index()));
			handTwo.at(index()).setPresent(false);
		} else {
			message.updateHand(handTwo.at(index()));
		}
	} else if((!handTwo.at(index()).isPresent()) && handTwo.at(previousIndex()).isPresent()){
		//no hand, so ask for remove
		message.removeHand(handTwo.at(index()));
	} else {
		//nothing to do.
	}
}

/**
 * Draw the circles around the hands and the trace of them moving
 * during the temporal window that they are tracked
 */
void drawHandTrace(Mat img) {
	//left hand
	if(handOne.at(index()).isPresent()) {
		ellipse(img, handOne[index()].getMinRect(), ORANGE, 2, 8);
		for(uint i = 0; i+1 < hand_window_size; i++) {
			int current = previousIndex(i);
			int previous = previousIndex(i + 1);
			if(handOne.at(current).isPresent() && handOne.at(previous).isPresent()) {
				if(setting->left_grab_mode) {
					line(img, handOne.at(previous).getMinRectCenter(), handOne.at(current).getMinRectCenter(), ORANGE, 5, 4, 0);
				} else {
					line(img, handOne.at(previous).getMinRectCenter(), handOne.at(current).getMinRectCenter(), ORANGE, 2, 4, 0);
				}
			}
		}
	}

	//right hands
	if(handTwo.at(index()).isPresent()) {
		//polylines(img, handTwo[index()].getMinRect()., 4, 1, true, BLUE, 2, 8, 1);
		ellipse(img, handTwo[index()].getMinRect(), BLUE, 2, 8);
		for(uint i = 0; i+1 < hand_window_size; i++) {
			int current = previousIndex(i);
			int previous = previousIndex(i + 1);
			if(handTwo.at(current).isPresent() && handTwo.at(previous).isPresent()) {
				if(setting->left_grab_mode) {
					line(img, handTwo.at(previous).getMinRectCenter(), handTwo.at(current).getMinRectCenter(), BLUE, 5, 4, 0);
				} else {
					line(img, handTwo.at(previous).getMinRectCenter(), handTwo.at(current).getMinRectCenter(), BLUE, 2, 4, 0);
				}
			}
		}
	}
}

/**
 * This functions process the input key and set application mode accordingly
 */
void processKey(char key) {
	switch(key) {
		case '1':
			break;
		case '2':
			break;
		case 'a':
			break;
		case 's':
			setting->save_input_video = !setting->save_input_video;
			break;
		case 'r':
			setting->save_output_video = !setting->save_output_video;
			break;
		case 'c':
			setting->capture_snapshot = true;
			break;
		case 'u':
			cvDestroyWindow("Source");
			cvDestroyWindow("Tracked");
			cvDestroyWindow("Binary");
			cvDestroyWindow("Touch");
			pgrCamera.calibrateUndistortionROI();
			printKeys();
			break;
		case 'h':
			printKeys();
			break;
		default:
			break;
	}
}

/**
 * find features in two frame and track them using optical flow.
 * result is stored in global data structure
 */
void findGoodFeatures(Mat frame1, Mat frame2) {
	if(frame1.cols == frame2.cols && frame1.rows == frame2.rows) { //ensure frames were not resized
		previousCorners.clear();
		goodFeaturesToTrack(frame1, previousCorners, maxCorners, qualityLevel, minDistance, Mat(), blockSize, useHarrisDetector);
		//cornerSubPix(previousFrame, previousCorners, Size(10,10), Size(-1,-1), termCriteria);

		int maxLevel = 1; // 0-based maximal pyramid level number. If 0, pyramids are not used (single level), if 1, two levels are used etc.
		calcOpticalFlowPyrLK(frame1, frame2, previousCorners, currentCorners, flowStatus, flowError, Size(blockSize, blockSize), maxLevel, termCriteria, derivLambda, OPTFLOW_FARNEBACK_GAUSSIAN);
	}
}

/**
 * This is the main loop function that loads and process images one by one
 * The function attempts to either connect to a PGR camera or loads
 * a video file from predefined path
 */
void start(){
	//Contour detection structures
	vector<vector<cv::Point> > contours;
	vector<Vec4i> hiearchy;

	VideoCapture video(setting->input_video_path);

	if(setting->pgr_cam_index >= 0) {
		pgrCamera.init();
	} else {
		video.set(CV_CAP_PROP_FPS, 30);
		if(!video.isOpened()) { // check if we succeeded
			cout << "Failed to open video file: " << setting->input_video_path << endl;
			return;
		}
		if (setting->verbose)  {
			cout << "Video path = " << setting->input_video_path << endl;
			cout << "Video fps = " << video.get(CV_CAP_PROP_FPS);
		}
	}

	//Preparing for main video loop
	Mat previousFrame;
	Mat currentFrame;
	Mat trackingResults;
	Mat binaryImg; //binary image for finding contours of the hand
	Mat tmpColor;
	Mat touchImage;
	Mat previousTouchImage;

	//Mat watershed_markers = cvCreateImage( setting->imageSize, IPL_DEPTH_32S, 1 );
	//Mat watershed_image;

	flowCount = vector<float>(maxCorners);
	char key = 'a';
	timeval first_time, second_time; //for fps calculation
	std::stringstream fps_str;
	gettimeofday(&first_time, 0);
	int fps = 0;
	while(key != 'q') {
		if(setting->pgr_cam_index >= 0){
			currentFrame = pgrCamera.grabImage();
//			Mat rotatedFrame;
//			rotateImage(&currentFrame, &rotatedFrame, 180);
//			currentFrame = rotatedFrame;

			if(setting->save_input_video) {
				if (sourceWriter.isOpened()) {
					cvtColor(currentFrame, tmpColor, CV_GRAY2RGB);
					sourceWriter << tmpColor;
				} else {
					sourceWriter = VideoWriter(setting->source_recording_path, CV_FOURCC('D', 'I', 'V', '5'), fps,
							Size(setting->imageSizeX,setting->imageSizeY));
				}
			}
		} else{
			//This is a video file source, no need to save
			video >> currentFrame;

			cvtColor(currentFrame, tmpColor, CV_RGB2GRAY);
			currentFrame = tmpColor;
		}
		message.init();

		if(!setting->is_daemon) {
			imshow("Source", currentFrame);
		}

		//do all the pre-processing
		//process(currentFrame);
		//imshow("Processed", currentFrame);

		//need at least one previous frame to process
		if(frameCount == 0) {
			previousFrame = currentFrame;
		}

		if(!setting->is_daemon) {
			key = cvWaitKey(10);
			processKey(key);
		}

		/**
		 * Prepare the binary image for tracking hands as the two largest blobs in the scene
		 * */
		binaryImg = currentFrame.clone();
		threshold(currentFrame, binaryImg, setting->lower_threshold, setting->upper_threshold, THRESH_BINARY);

		if(setting->capture_snapshot) {
			imwrite(setting->snapshot_path + "binary.png", binaryImg);
		}

		//clean up the current frame from noise using median blur filter
		medianBlur(binaryImg, binaryImg, setting->median_blur_factor);
		if(setting->capture_snapshot) {
			imwrite(setting->snapshot_path + "median.png", binaryImg);
		}

		//adaptiveThreshold(binaryImg, binaryImg, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 10); //adaptive thresholding not works so well here
		touchImage = Mat(currentFrame.size(), CV_32FC1);
		sharpnessImage(currentFrame, touchImage);
		if(!setting->is_daemon) {
			imshow("Binary", binaryImg);
			imshow("Touch", touchImage);
		}
		if(frameCount == 0) {
			previousFrame = currentFrame;
		}

		//findContours(binaryImg, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_L1);
		//findContours(binaryImg, contours, hiearchy,  RETR_TREE, CHAIN_APPROX_SIMPLE);
		//findContours(binaryImg, contours, hiearchy,  RETR_EXTERNAL|RETR_CCOMP, CHAIN_APPROX_NONE);
		findContours( binaryImg, contours, RETR_EXTERNAL, CV_CHAIN_APPROX_NONE );

		//Canny(previousFrame, previousFrame, 0, 30, 3);
		if(!setting->is_daemon) {
			cvtColor(currentFrame, trackingResults, CV_GRAY2BGR);

			if (contours.size() > 0) {
//				int index = 0;
//				for (; index >= 0; index = hiearchy[index][0]) {
//					drawContours(trackingResults, contours, index, OLIVE, 1, 4, hiearchy, 0);
//				}
				drawContours(trackingResults, contours, -1, OLIVE, 1, 4);
			}
		}

		//drawContours(watershed_markers, contours, -1, CV_RGB(0, 0, 0)); //TODO watershed testing
		//TODO: watershed testing
		//imshow("Watershed before", watershed_markers);
		//watershed_image = cvCreateMat(currentFrame.rows, currentFrame.cols, CV_8UC3 );
		//cvtColor(currentFrame, watershed_image, CV_GRAY2BGR);
		//watershed(watershed_image, touchImage);
		//imshow("Watershed", touchImage);

		findHands(contours);

		if(numberOfHands() > 0) {
			//findGoodFeatures(previousFrame, currentFrame);
			findGoodFeatures(previousTouchImage, touchImage);
			featureDepthExtract(touchImage);
			assignFeaturesToHands();
			meanAndStdDevExtract();
			GestureTracker::checkGestures(&handOne);
			GestureTracker::checkGestures(&handTwo);
			if(!setting->is_daemon) {
				//only draw things if there are going to be displayed
				drawHandTrace(trackingResults);
				drawFeatures(trackingResults);
				drawMeanAndStdDev(trackingResults);
			}
		} else {
			//TODO: is any cleanup necessary here?
		}
		updateMessage();

		if(setting->capture_snapshot) {
			imwrite(setting->snapshot_path + "source.png", currentFrame);
			touchImage.convertTo(touchImage, CV_8SC3);
			//cvtColor(touchImage, touchImage, CV_GRAY2BGR, 1);
			imwrite(setting->snapshot_path + "touch.png", touchImage);
			imwrite(setting->snapshot_path + "result.png", trackingResults);
			putText(trackingResults, "Snapshot OK!", Point(40,120), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
			setting->capture_snapshot = false;
		}

		//calculate and display FPS every 100 frames
		if((frameCount + 1) % 100 == 0) {
			gettimeofday(&second_time, 0);
			fps = (int)(100/(second_time.tv_sec - first_time.tv_sec + 0.01));
			fps_str.str("");
			fps_str << "FPS = [" << fps << "]";
			first_time = second_time;
		}
		if(frameCount % 1000 == 0) verbosePrint(fps_str.str()); //report fps every 1000 frame on the terminal
		putText(trackingResults, fps_str.str(), Point(5,20), FONT_HERSHEY_COMPLEX_SMALL, 1, GREEN, 1, 8, false);

		if(!setting->is_daemon) {
			if(setting->save_output_video){
				if (resultWriter.isOpened()) {
					resultWriter << trackingResults;
					putText(trackingResults, "Recording Results ... ", Point(40,120), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
				} else {
					resultWriter = VideoWriter(setting->result_recording_path, CV_FOURCC('D', 'I', 'V', '5'), fps,
							Size(setting->imageSizeX,setting->imageSizeY));
				}
			}

			if(setting->save_input_video){
				//actually saving is done before pre processing above
				putText(trackingResults, "Recording Source ... ", Point(40,40), FONT_HERSHEY_COMPLEX, 1, YELLOW, 3, 8, false);
			}
			imshow("Tracked", trackingResults);
		}

		message.commit();
		previousFrame = currentFrame;
		currentCorners = previousCorners;
		previousTouchImage = touchImage;
		frameCount++;
	}

	//Clean up before leaving
	previousFrame.release();
	currentFrame.release();
	trackingResults.release();
	tmpColor.release();
	if(setting->pgr_cam_index >= 0){
		pgrCamera.~CameraPGR();
	}
}

/**
 * Find two largest blobs which hopefully represent the two hands
 */
void findHands(vector<vector<cv::Point> > contours) {

	bool handOnePresent = handOne.at(previousIndex()).isPresent();
	bool handTwoPresent = handTwo.at(previousIndex()).isPresent();
	Point handOneCenter = handOne.at(previousIndex()).getMinCircleCenter();
	Point handTwoCenter = handTwo.at(previousIndex()).getMinCircleCenter();

	handOne[index()].clear();
	handTwo[index()].clear();
	Point2f tmpCenter, max1Center, max2Center;
	float tmpRadius = 0, max1Radius = 0, max2Radius = 0;
	int max1ContourIndex = 0, max2ContourIndex = 0;
	int contour_side_threshold = 50;

	for (uint i = 0; i < contours.size(); i++) {

		Size2f tmpSize = minAreaRect(Mat(contours[i])).size;
		float tmpSide = min(tmpSize.height, tmpSize.width);

		if(tmpSide > contour_side_threshold) {

			minEnclosingCircle(Mat(contours[i]), tmpCenter, tmpRadius);

			if (tmpRadius > max1Radius) {
				if (max1Radius > max2Radius) {
					max2Radius = max1Radius;
					max2Center = max1Center;
					max2ContourIndex = max1ContourIndex;
				}
				max1Radius = tmpRadius;
				max1Center = tmpCenter;
				max1ContourIndex = i;
			} else if (tmpRadius > max2Radius) {
				//max1Radius is bigger than max2Radius
				max2Radius = tmpRadius;
				max2Center = tmpCenter;
				max2ContourIndex = i;
			}
		}
	}

	//Detect the two largest circles that represent hands, if they exist
	if(max1Radius > setting->radius_threshold && max2Radius > setting->radius_threshold) {
		//Two hands Present
		if(!handOnePresent && !handTwoPresent) {
			//default: max1 is hand one
			handOne[index()].setMinCircleCenter(max1Center);
			handOne[index()].setMinCircleRadius(max1Radius);
			handOne[index()].setContour(contours[max1ContourIndex]);
			handOne[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
			handOne[index()].setPresent(true);
			//and max2 is hand two
			handTwo[index()].setMinCircleCenter(max2Center);
			handTwo[index()].setMinCircleRadius(max2Radius);
			handTwo[index()].setContour(contours[max2ContourIndex]);
			handTwo[index()].setMinRect(minAreaRect(Mat(contours[max2ContourIndex])));
			handTwo[index()].setPresent(true);
		} else if(handOnePresent && !handTwoPresent) {
			if(getDistance(handOneCenter, max1Center) < getDistance(handOneCenter, max2Center)) {
				//max1 is on the left
				handOne[index()].setMinCircleCenter(max1Center);
				handOne[index()].setMinCircleRadius(max1Radius);
				handOne[index()].setContour(contours[max1ContourIndex]);
				handOne[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handOne[index()].setPresent(true);
				//and max2 is on the right
				handTwo[index()].setMinCircleCenter(max2Center);
				handTwo[index()].setMinCircleRadius(max2Radius);
				handTwo[index()].setContour(contours[max2ContourIndex]);
				handTwo[index()].setMinRect(minAreaRect(Mat(contours[max2ContourIndex])));
				handTwo[index()].setPresent(true);
			} else {
				//max1 is on the right
				handTwo[index()].setMinCircleCenter(max1Center);
				handTwo[index()].setMinCircleRadius(max1Radius);
				handTwo[index()].setContour(contours[max1ContourIndex]);
				handTwo[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handTwo[index()].setPresent(true);
				//max2 is therefore on the left
				handOne[index()].setMinCircleCenter(max2Center);
				handOne[index()].setMinCircleRadius(max2Radius);
				handOne[index()].setContour(contours[max2ContourIndex]);
				handOne[index()].setMinRect(minAreaRect(Mat(contours[max2ContourIndex])));
				handOne[index()].setPresent(true);
			}
		} else {
			if(getDistance(handTwoCenter, max1Center) > getDistance(handTwoCenter, max2Center)) {
				//max1 is on the left
				handOne[index()].setMinCircleCenter(max1Center);
				handOne[index()].setMinCircleRadius(max1Radius);
				handOne[index()].setContour(contours[max1ContourIndex]);
				handOne[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handOne[index()].setPresent(true);
				//and max2 is on the right
				handTwo[index()].setMinCircleCenter(max2Center);
				handTwo[index()].setMinCircleRadius(max2Radius);
				handTwo[index()].setContour(contours[max2ContourIndex]);
				handTwo[index()].setMinRect(minAreaRect(Mat(contours[max2ContourIndex])));
				handTwo[index()].setPresent(true);
			} else {
				//max1 is on the right
				handTwo[index()].setMinCircleCenter(max1Center);
				handTwo[index()].setMinCircleRadius(max1Radius);
				handTwo[index()].setContour(contours[max1ContourIndex]);
				handTwo[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handTwo[index()].setPresent(true);
				//max2 is therefore on the left
				handOne[index()].setMinCircleCenter(max2Center);
				handOne[index()].setMinCircleRadius(max2Radius);
				handOne[index()].setContour(contours[max2ContourIndex]);
				handOne[index()].setMinRect(minAreaRect(Mat(contours[max2ContourIndex])));
				handOne[index()].setPresent(true);
			}
		}
	} else if(max1Radius > setting->radius_threshold ){
		if(!handOnePresent && !handTwoPresent) {
			handOne[index()].setMinCircleCenter(max1Center);
			handOne[index()].setMinCircleRadius(max1Radius);
			handOne[index()].setContour(contours[max1ContourIndex]);
			handOne[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
			handOne[index()].setPresent(true);

		} else if(handOnePresent && !handTwoPresent) {
			if(getDistance(handOneCenter, max1Center) < setting->radius_threshold*4) {
				handOne[index()].setMinCircleCenter(max1Center);
				handOne[index()].setMinCircleRadius(max1Radius);
				handOne[index()].setContour(contours[max1ContourIndex]);
				handOne[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handOne[index()].setPresent(true);
				//clear right hand
				handTwo[index()].clear();
			} else {
				handTwo[index()].setMinCircleCenter(max1Center);
				handTwo[index()].setMinCircleRadius(max1Radius);
				handTwo[index()].setContour(contours[max1ContourIndex]);
				handTwo[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handTwo[index()].setPresent(true);
				//clear right hand
				handOne[index()].clear();
			}
		} else {
			if(getDistance(handTwoCenter, max1Center) > setting->radius_threshold*4) {
				handOne[index()].setMinCircleCenter(max1Center);
				handOne[index()].setMinCircleRadius(max1Radius);
				handOne[index()].setContour(contours[max1ContourIndex]);
				handOne[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handOne[index()].setPresent(true);
				//clear right hand
				handTwo[index()].clear();
			} else {
				handTwo[index()].setMinCircleCenter(max1Center);
				handTwo[index()].setMinCircleRadius(max1Radius);
				handTwo[index()].setContour(contours[max1ContourIndex]);
				handTwo[index()].setMinRect(minAreaRect(Mat(contours[max1ContourIndex])));
				handTwo[index()].setPresent(true);
				//clear right hand
				handOne[index()].clear();
			}
		}
	} else {
		handTwo[index()].clear();
		handOne[index()].clear();
	}
}

/**
 * Calculate and return the number of detected hands in the current frame
 */
int numberOfHands() {
	int numberOfHands = 0;
	if (handOne.at(index()).isPresent()) {
		numberOfHands++;
	}
	if (handTwo.at(index()).isPresent()) {
		numberOfHands++;
	}
	return numberOfHands;
}

/**
 * Returns the current index based on the frame count that is used to identify which hand in the
 * handOne and handTwo arrays are corresponding to current frame
 */
int index() {
	return frameCount % hand_window_size;
}

/**
 * return the index of previous hand in the hand temporal window
 */
int previousIndex() {
	return (frameCount - 1) % hand_window_size;
}

/**
 * return ith previous hand from the history.
 * @Precondition: i is smaller than hand_window_size
 */
int previousIndex(int i) {
	if(i >= hand_window_size) {
		verbosePrint("Incorrect index given to previousIndex(int i) function");
	}
	return (frameCount - i) % hand_window_size;
}

/**
 * Draw features based on the hand they belong to
 * @Precondition: assignFeaturedToHand() is executed and leftRightStatus[i] is filled
 */
void drawFeatures(Mat img) {
	//First hand
	if(handOne.at(index()).isPresent()) {
		vector<Point2f> points = handOne.at(index()).getFeatures();
		vector<Point2f> vectors = handOne.at(index()).getVectors();
		vector<float> featureDepth = handOne.at(index()).getFeaturesDepth();
		vector<Point2f> orientation = handOne.at(index()).getFeatureOrientation();

		for (uint i = 0; i < points.size(); i++) {
			//draw feature box
			rectangle(img, Point(points[i].x - blockSize/2, points[i].y - blockSize/2), Point(points[i].x + blockSize/2, points[i].y + blockSize/2), ORANGE);
			//TODO: draw feature trace
//			for(uint j = 0; j+1 < hand_window_size; j++) {
//				int current = previousIndex(j);
//				int previous = previousIndex(j + 1);
//				if(!handOne.at(current).isFeatureTracked(i)) {
//					continue; //only draw lines if feature is successfully tracked
//				}
//				if(handOne.at(current).isPresent() && handOne.at(previous).isPresent()) {
//					line(img, handOne.at(previous).getFeatureAt(i), handOne.at(current).getFeatureAt(i), ORANGE, 2, 4, 0);
//				}
//			}

			//draw feature direction vector
			line(img, points[i], (points[i] + vectors[i]), ORANGE, 2, 8, 0);

			//visualize feature depth and touch with a circle
			int base_radius = 50;
			int scaled_depth = 1 + base_radius * 2 * featureDepth[i];

			if(scaled_depth > setting->touch_depth_threshold) {
				//visualize touch with a filled red circle
				circle(img, points[i], blockSize, RED, -1, CV_AA);
			} else if(scaled_depth < base_radius) {
				circle(img, points[i], base_radius - scaled_depth, RED, 1, CV_AA);
			}

			//visualize feature orientation with a line
			line(img, points[i], points[i] + orientation[i], GREEN, 3, CV_AA);
		}
	}

	//Second hand
	if(handTwo.at(index()).isPresent()) {
		vector<Point2f> points = handTwo.at(index()).getFeatures();
		vector<Point2f> vectors = handTwo.at(index()).getVectors();
		for (uint i = 0; i < points.size(); i++) {
			rectangle(img, Point(points[i].x - blockSize/2, points[i].y - blockSize/2), Point(points[i].x + blockSize/2, points[i].y + blockSize/2), BLUE);
			line(img, points[i], (points[i] + vectors[i]), BLUE, 2, 8, 0);
		}
	}
//	for(int i = 0; i < maxCorners; i++) {
//		if(leftRightStatus[i] == 1) {
//			// Left hand
//			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), ORANGE);
//			//circle(img, Point(currentCorners[i].x, currentCorners[i].y), featureDepth[i] + 1, ORANGE);
//		} else if(leftRightStatus[i] == 2) {
//			// Right hand
//			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), BLUE);
//			//circle(img, Point(currentCorners[i].x, currentCorners[i].y), featureDepth[i] + 1, BLUE);
//		} else {
//			// None
//			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), PINK);
//		}
//
//		if((uchar)flowStatus[i] == 1) {
//			line(img, currentCorners[i], previousCorners[i], GREEN, 2, 8, 0);
//		}
//	}
}

/**
 * Draw a circle for mean and stdDev of features and a trace of their changes over
 * the hand temporal window
 */
void drawMeanAndStdDev(Mat img) {
	if(handOne.at(index()).isPresent()) {
		circle(img, handOne.at(index()).getFeatureMean(), handOne.at(index()).getFeatureStdDev(), YELLOW, 1, 4, 0);
		line(img, handOne.at(index()).getFeatureMean(), handOne.at(index()).getMinRectCenter(), YELLOW, 1, 4, 0);
	}
	if(handTwo.at(index()).isPresent()) {
		circle(img, handTwo.at(index()).getFeatureMean(), handTwo.at(index()).getFeatureStdDev(), YELLOW, 1, 4, 0);
		line(img, handTwo.at(index()).getFeatureMean(), handTwo.at(index()).getMinRectCenter(), YELLOW, 1, 4, 0);
	}
}

/**
 * assign features and their corresponding vector to hand(s) if the feature
 * has been successfully tracked and a hand contain it
 */
void assignFeaturesToHands() {
	for(int i = 0; i < maxCorners; i++) {
		if(flowStatus[i] == 1) {
			flowCount[i] += 1;
			if(flowCount[i] > 2) {
				if(handOne.at(index()).isPresent() && handOne.at(index()).hasPointInside(currentCorners[i])) {
					//point is inside contour of the left hand
					Point2f vector = currentCorners[i] - previousCorners[i];

					Point2f orientation = currentCorners[i] - handOne.at(index()).getMinRectCenter();
//					Point2f orientation = Point2f(currentCorners[i].x - handOne.at(index()).getMinRectCenter().x,
//							currentCorners[i].y - handOne.at(index()).getMinRectCenter().y);
					handOne.at(index()).addFeatureAndVector(currentCorners[i], vector, featureDepth[i], orientation, flowStatus[i]);
				} else if(handTwo.at(index()).isPresent() && handTwo.at(index()).hasPointInside(currentCorners[i])) {
					Point2f vector = currentCorners[i] - previousCorners[i];
					Point2f orientation = currentCorners[i] - handOne.at(index()).getMinRectCenter();
					handTwo.at(index()).addFeatureAndVector(currentCorners[i], vector, featureDepth[i], orientation, flowStatus[i]);
				} else {
					//this is noise or some other object
					//Don't worry about it!
				}
			}
		} else {
			flowCount[i] = 0;
		}
	}
}

/**
 * Calculate the distance between two points
 */
float getDistance(const Point2f a, const Point2f b) {
	return sqrt(pow((a.x - b.x), 2) + pow((a.y - b.y), 2));
}

/**
 * Find mean point and standard deviation of features for each hand
 * @Precondition: assignFeatureToHands is executed
 */
void meanAndStdDevExtract() {
	if(handOne.at(index()).isPresent()) {
		handOne.at(index()).calcMeanStdDev();
	}
	if(handTwo.at(index()).isPresent()){
		handTwo.at(index()).calcMeanStdDev();
	}
}

/**
 * Calculate the depth of each feature based on the blurriness of its window
 * @Precondition: img is the image showing minEigenValue calculation. Bright pixels in this image represent highly sharp regions
 */
void featureDepthExtract(Mat img) {
	Mat featureRegion;
	Rect rect;
	//Scalar stdDev;
	Scalar mean;
	featureDepth.clear();

	for(int i = 0; i < maxCorners; i++) {
//		if(leftRightStatus[i] == 1 || leftRightStatus[i] == 2) {
			//left or right hand feature
			rect = Rect(int(currentCorners[i].x - blockSize/2), int(currentCorners[i].y - blockSize/2), blockSize, blockSize);
			if(rect.x < 0 || rect.y < 0 || rect.x + blockSize > img.cols || rect.y + blockSize > img.rows) {
				//we dont care about the features that are right on the edge
				featureDepth.push_back(-1);
				continue;
			}
			featureRegion = img(rect);
			//meanStdDev(featureRegion, mean, stdDev);
			mean = cv::mean(featureRegion);
			featureDepth.push_back(mean.val[0]);
//		} else {
//			//Doesnt matter if the feature is not assigned to a hand, so set to -1
//			featureDepth.push_back(-1);
//		}
	}
}

void printKeys() {
	cout << "TRACKING MODE" << endl
		<< "'s' - toggle save input video" << endl
		<< "'r' - toggle save output video" << endl
		<< "'c' - capture snapshot" << endl
		<< "'u' - enter undistortion/roi settings mode" << endl
		<< "'q' - quit application" << endl
		<< "'h' - print this message" << endl << endl;
}
