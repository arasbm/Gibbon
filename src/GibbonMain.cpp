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

//OpenCV include files.
#include "highgui.h"
#include "cv.h"
#include "cxcore.h"
#include "ml.h"
#include "cxtypes.h"

//Standard includes
#include <cmath>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>

#include "Setting.h"
#include "CameraPGR.h"
#include "Log.h"
#include "Hand.h"
#include "Message.h"
#include "Undistortion.h"
#include "ImageUtils.h"

using namespace std;
using namespace cv;

void processKey(char key);
void addFeatureMeanStdDev(bool left, Point2f mean, float stdDev);
void findHands(vector<vector<cv::Point> > contours);
void opencvConnectedComponent(Mat* src, Mat* dst);
void init();
void start();
void checkGrab();
void checkRelease();
void meanAndStdDevExtract();
void findGoodFeatures(Mat frame1, Mat frame2);
void assignFeaturesToHands();
void drawFeatures(Mat img);
void drawMeanAndStdDev(Mat img);
float getDistance(const Point2f a, const Point2f b);
int numberOfHands();
int index();
void updateMessage();
int previousIndex();

/** OpenCV variables **/
CvPoint mouseLocation;
VideoWriter sourceWriter;
VideoWriter resultWriter;

/** Hand tracking structures (temporal tracking window) **/
const uint hand_window_size = 5; //Number of frames to keep track of hand. Minimum of two is needed
vector<Hand> leftHand(hand_window_size, Hand(LEFT_HAND));
vector<Hand> rightHand(hand_window_size, Hand(RIGHT_HAND));

/** goodFeaturesToTrack structure and settings **/
vector<Point2f> previousCorners;
vector<Point2f> currentCorners; //Center point of feature or corner rectangles
vector<uchar> flowStatus;
vector<float> featureDepth;
vector<uchar> leftRightStatus; // 0=None, 1=Left, 2=Right
vector<float> flowError;
TermCriteria termCriteria = TermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 );
double derivLambda = 0.5; //proportion for impact of "image intensity" as opposed to "derivatives"
int maxCorners = 16;
double qualityLevel = 0.01;
double minDistance = 10;
int blockSize = 16;
bool useHarrisDetector = false; //its either harris or cornerMinEigenVal

CameraPGR pgrCamera;
Undistortion undistortion;
Message message; //used by updateMessage() and inside the main loop
int frameCount = 0;
static Setting setting = Setting::Instance();

int main(int argc, char* argv[]) {
	if(!setting.loadOptions(argc, argv)) {
		cout << "Error in loading options. Exiting the program." << endl;
		return -1;
	}
	verbosePrint("Starting ... ");

	if(!setting.is_daemon) {
		cvNamedWindow( "Source", CV_WINDOW_AUTOSIZE ); 		//monochrome source
		//cvNamedWindow( "Processed", CV_WINDOW_AUTOSIZE ); 	//monochrome image after pre-processing
		cvNamedWindow( "Tracked", CV_WINDOW_AUTOSIZE ); 	//Color with pretty drawings showing tracking results
	}
	// Mat colorImg; //color image for connected component labelling

	init();
	start();

	if(!setting.is_daemon) {
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
	if(setting.do_undistortion) {
		undistortion = Undistortion();
	}

}

/**
 * Check for grab gesture
 */
void checkGrab() {

}

/**
 * Check for release gesture
 */
void checkRelease() {

}

/**
 * Check if there are any hands and add current hand gestures to global object
 * "message"
 */
void updateMessage() {
	if(leftHand.at(index()).isPresent() && (!leftHand.at(previousIndex()).isPresent())) {
		//New hand!
		message.newHand(leftHand.at(index()));
	} else if(leftHand.at(index()).isPresent()) {
		//Update existing hand
		message.updateHand(leftHand.at(index()));
	} else if((!leftHand.at(index()).isPresent()) && leftHand.at(previousIndex()).isPresent()) {
		//ask for remove
		message.removeHand(leftHand.at(index()));
	} else {
		//Peace and quiet here. Nothing to do.
	}

	if(rightHand.at(index()).isPresent() && (!rightHand.at(previousIndex()).isPresent())) {
		//New hand!
		message.newHand(rightHand.at(index()));
	} else if(rightHand.at(index()).isPresent()) {
		//Update existing hand
		message.updateHand(rightHand.at(index()));
	} else if((!rightHand.at(index()).isPresent()) && rightHand.at(previousIndex()).isPresent()){
		//no hand, so ask for remove
		message.removeHand(rightHand.at(index()));
	} else {
		//nothing to do.
	}
}

/**
 * Draw the circles around the hands and the trace of them moving
 * during the temporal window that they are tracked
 * */
void drawHandTrace(Mat img) {
	//left hand
	if(leftHand.at(index()).isPresent()) {
		ellipse(img, leftHand[index()].getMinRect(), ORANGE, 2, 8);
		for(uint i = index(); i < index() + hand_window_size; i++) {
			int current = (i + 1) % hand_window_size;
			int previous = i % hand_window_size;
			if(leftHand.at(current).isPresent() && leftHand.at(previous).isPresent()) {
				if(setting.left_grab_mode) {
					line(img, leftHand.at(previous).getMinCircleCenter(), leftHand.at(current).getMinCircleCenter(), ORANGE, 5, 4, 0);
				} else {
					line(img, leftHand.at(previous).getMinCircleCenter(), leftHand.at(current).getMinCircleCenter(), ORANGE, 2, 4, 0);
				}
			}
		}
	}

	//right hands
	if(rightHand.at(index()).isPresent()) {
		//polylines(img, rightHand[index()].getMinRect()., 4, 1, true, BLUE, 2, 8, 1);
		ellipse(img, rightHand[index()].getMinRect(), BLUE, 2, 8);
		for(uint i = index(); i < index() + hand_window_size; i++) {
			int current = (i + 1) % hand_window_size;
			int previous = i % hand_window_size;
			if(rightHand.at(current).isPresent() && rightHand.at(previous).isPresent()) {
				if(setting.left_grab_mode) {
					line(img, rightHand.at(previous).getMinCircleCenter(), rightHand.at(current).getMinCircleCenter(), BLUE, 5, 4, 0);
				} else {
					line(img, rightHand.at(previous).getMinCircleCenter(), rightHand.at(current).getMinCircleCenter(), BLUE, 2, 4, 0);
				}
			}
		}
	}
}

/**
 * This functions process the input key and set application mode accordingly
 * */
void processKey(char key) {
	switch(key) {
		case '1':
			break;
		case '2':
			break;
		case 'a':
			break;
		case 's':
			setting.save_input_video = !setting.save_input_video;
			break;
		case 'r':
			setting.save_output_video = !setting.save_output_video;
			break;
		case 'c':
			setting.capture_snapshot = true;
			break;
		default:
			break;
	}
}

/**
 * find features in two frame and track them using optical flow.
 * result is stored in global data structure
 **/
void findGoodFeatures(Mat frame1, Mat frame2) {
	goodFeaturesToTrack(frame1, previousCorners, maxCorners, qualityLevel, minDistance, frame1, blockSize, useHarrisDetector);
	//cornerSubPix(previousFrame, previousCorners, Size(10,10), Size(-1,-1), termCriteria);
	calcOpticalFlowPyrLK(frame1, frame2, previousCorners, currentCorners, flowStatus, flowError, Size(blockSize, blockSize), 1, termCriteria, derivLambda, OPTFLOW_FARNEBACK_GAUSSIAN);
}

/**
 * This is the main loop function that loads and process images one by one
 * The function attempts to either connect to a PGR camera or loads
 * a video file from predefined path
 * */
void start(){
	//Contour detection structures
	vector<vector<cv::Point> > contours;
	vector<Vec4i> hiearchy;

	VideoCapture video(setting.input_video_path);
	video.set(CV_CAP_PROP_FPS, 30);

	if(setting.pgr_cam_index >= 0) {
		pgrCamera.init();
	} else {
		if(!video.isOpened()) { // check if we succeeded
			cout << "Failed to open video file: " << setting.input_video_path << endl;
			return;
		}
		if (setting.verbose)  {
			cout << "Video path = " << setting.input_video_path << endl;
			cout << "Video fps = " << video.get(CV_CAP_PROP_FPS);
		}
	}

	//Preparing for main video loop
	Mat previousFrame;
	Mat currentFrame;
	Mat trackingResults;
	Mat binaryImg; //binary image for finding contours of the hand
	Mat tmpColor;
	Mat depthImage;

	Mat watershed_markers = cvCreateImage( setting.imageSize, IPL_DEPTH_32S, 1 );
	Mat watershed_image;

	char key = 'a';
	timeval first_time, second_time; //for fps calculation
	std::stringstream fps_str;
	gettimeofday(&first_time, 0);
	int fps = 0;
	while(key != 'q') {
		if(setting.pgr_cam_index >= 0){
			currentFrame = pgrCamera.grabImage();
			if(setting.do_undistortion) {
				undistortion.undistortImage(currentFrame);
			}
			if(setting.save_input_video) {
				if (sourceWriter.isOpened()) {
					cvtColor(currentFrame, tmpColor, CV_GRAY2RGB);
					sourceWriter << tmpColor;
				} else {
					sourceWriter = VideoWriter(setting.source_recording_path, CV_FOURCC('D', 'I', 'V', '5'), fps, setting.imageSize);
				}
			}
		} else{
			//This is a video file source, no need to save
			video >> currentFrame;

			cvtColor(currentFrame, tmpColor, CV_RGB2GRAY);
			currentFrame = tmpColor;
		}
		message.init();

		if(!setting.is_daemon) {
			imshow("Source", currentFrame);
		}

		//do all the pre-processing
		//process(currentFrame);
		//imshow("Processed", currentFrame);

		//need at least one previous frame to process
		if(frameCount == 0) {
			previousFrame = currentFrame;
		}

		if(!setting.is_daemon) {
			key = cvWaitKey(20);
			processKey(key);
		}

		/**
		 * Prepare the binary image for tracking hands as the two largest blobs in the scene
		 * */
		binaryImg = currentFrame.clone();
		threshold(currentFrame, binaryImg, setting.lower_threshold, 255, THRESH_BINARY);

		if(setting.capture_snapshot) {
			imwrite(setting.snapshot_path + "binary.png", binaryImg);
		}

		//clean up the current frame from noise using median blur filter
		medianBlur(binaryImg, binaryImg, setting.median_blur_factor);
		if(setting.capture_snapshot) {
			imwrite(setting.snapshot_path + "median.png", binaryImg);
		}

		//adaptiveThreshold(binaryImg, binaryImg, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 10); //adaptive thresholding not works so well here
		if(!setting.is_daemon) {
			imshow("Binary", binaryImg);
			depthImage = Mat(currentFrame.size(), CV_32SC1);
			//TODO: init depthImage with CV_32FC1
			depthFromDiffusion(currentFrame, depthImage);
			imshow("depth", depthImage);
		}

		findContours(binaryImg, contours, hiearchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_L1);
		//findContours( binaryImg, contours, RETR_TREE, CV_CHAIN_APPROX_SIMPLE );

		//Canny(previousFrame, previousFrame, 0, 30, 3);
		if(!setting.is_daemon) {
			trackingResults = cvCreateMat(currentFrame.rows, currentFrame.cols, CV_8UC3 );
			cvtColor(currentFrame, trackingResults, CV_GRAY2BGR);

			if (contours.size() > 0) {
				int index = 0;
				for (; index >= 0; index = hiearchy[index][0]) {
					drawContours(trackingResults, contours, index, OLIVE, 1, 4, hiearchy, 0);

				}
			}
		}
		//drawContours(watershed_markers, contours, -1, CV_RGB(0, 0, 0)); //TODO watershed testing
		//TODO: watershed testing
		//imshow("Watershed before", watershed_markers);
		//watershed_image = cvCreateMat(currentFrame.rows, currentFrame.cols, CV_8UC3 );
		//cvtColor(currentFrame, watershed_image, CV_GRAY2BGR);
		//watershed(watershed_image, depthImage);
		//imshow("Watershed", depthImage);

		findHands(contours);

		if(numberOfHands() > 0) {
			findGoodFeatures(previousFrame, currentFrame);
			drawHandTrace(trackingResults);
			assignFeaturesToHands();
			//featureDepthExtract(trackingResults);

			drawFeatures(trackingResults);
			//drawFeatureDepth(trackingResults);

			//TODO: Fix: meanAndStdDevExtract();
			//TODO: Fix: drawMeanAndStdDev(trackingResults);
			checkGrab();
			checkRelease();
		} else {
			//TODO: is any cleanup necessary here?
		}
		updateMessage();

		if(setting.capture_snapshot) {
			imwrite(setting.snapshot_path + "source.png", currentFrame);
			depthImage.convertTo(depthImage, CV_8SC3);
			//cvtColor(depthImage, depthImage, CV_GRAY2BGR, 1);
			imwrite(setting.snapshot_path + "depth.png", depthImage);
			imwrite(setting.snapshot_path + "result.png", trackingResults);
			putText(trackingResults, "Snapshot OK!", Point(40,120), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
			setting.capture_snapshot = false;
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

		if(!setting.is_daemon) {
			if(setting.save_output_video){
				if (resultWriter.isOpened()) {
					resultWriter << trackingResults;
					putText(trackingResults, "Recording Results ... ", Point(40,120), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
				} else {
					resultWriter = VideoWriter(setting.result_recording_path, CV_FOURCC('D', 'I', 'V', '5'), fps, setting.imageSize);
				}
			}

			if(setting.save_input_video){
				//actually saving is done before pre processing above
				putText(trackingResults, "Recording Source ... ", Point(40,40), FONT_HERSHEY_COMPLEX, 1, YELLOW, 3, 8, false);
			}
			imshow("Tracked", trackingResults);
		}

		message.commit();
		previousFrame = currentFrame;
		currentCorners = previousCorners;
		frameCount++;
	}

	//Clean up before leaving
	previousFrame.release();
	currentFrame.release();
	trackingResults.release();
	tmpColor.release();
	if(setting.pgr_cam_index >= 0){
		pgrCamera.~CameraPGR();
	}
}

/**
 * Find two largest blobs which hopefully represent the two hands
 * */
void findHands(vector<vector<cv::Point> > contours) {
	Point2f tmpCenter, max1Center, max2Center;
	float tmpRadius = 0, max1Radius = 0, max2Radius = 0;
	int max1ContourIndex = 0, max2ContourIndex = 0;

	for (uint i = 0; i < contours.size(); i++) {
		if(contours[i].size() > 0) {
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
	int radius_threshold = 10;
	if(max1Radius > radius_threshold) {
		if(max1Center.x > max2Center.x) {
			//max1 is on the right
			rightHand[index()].setMinCircleCenter(max1Center);
			rightHand[index()].setMinCircleRadius(max1Radius);
			rightHand[index()].setContour(Mat(contours[max1ContourIndex]));
			rightHand[index()].setMinRect(minAreaRect(rightHand[index()].getContour()));
			rightHand[index()].setPresent(true);
			if(max2Radius > radius_threshold) {
				//max2 is on the left
				leftHand[index()].setMinCircleCenter(max2Center);
				leftHand[index()].setMinCircleRadius(max2Radius);
				leftHand[index()].setContour(Mat(contours[max2ContourIndex]));
				leftHand[index()].setMinRect(minAreaRect(leftHand[index()].getContour()));
				leftHand[index()].setPresent(true);
			} else {
				//Clear left hand
				leftHand[index()].clear();
			}
		} else {
			//max1 is on the left
			leftHand[index()].setMinCircleCenter(max1Center);
			leftHand[index()].setMinCircleRadius(max1Radius);
			leftHand[index()].setContour(Mat(contours[max1ContourIndex]));
			leftHand[index()].setMinRect(minAreaRect(leftHand[index()].getContour()));
			leftHand[index()].setPresent(true);
			if(max2Radius > radius_threshold) {
				//max2 is on the right
				rightHand[index()].setMinCircleCenter(max2Center);
				rightHand[index()].setMinCircleRadius(max2Radius);
				rightHand[index()].setContour(Mat(contours[max2ContourIndex]));
				rightHand[index()].setMinRect(minAreaRect(rightHand[index()].getContour()));
				rightHand[index()].setPresent(true);
			} else {
				rightHand[index()].clear();
			}
		}
	} else {
		rightHand[index()].clear();
	}
}

/**
 * Calculate and return the number of detected hands in the current frame
 * */
int numberOfHands() {
	int numberOfHands = 0;
	if (leftHand.at(index()).isPresent()) {
		numberOfHands++;
	}
	if (rightHand.at(index()).isPresent()) {
		numberOfHands++;
	}
	return numberOfHands;
}

/**
 * Returns the current index based on the frame count that is used to identify which hand in the
 * leftHand and rightHand arrays are corresponding to current frame
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
 * Draw features based on the hand they belong to
 * @Precondition: assignFeaturedToHand() is executed and leftRightStatus[i] is filled
 */
void drawFeatures(Mat img) {
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1) {
			// Left hand
			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), ORANGE);
			//circle(img, Point(currentCorners[i].x, currentCorners[i].y), featureDepth[i] + 1, ORANGE);
		} else if(leftRightStatus[i] == 2) {
			// Right hand
			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), BLUE);
			//circle(img, Point(currentCorners[i].x, currentCorners[i].y), featureDepth[i] + 1, BLUE);
		} else {
			// None
			rectangle(img, Point(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2), Point(currentCorners[i].x + blockSize/2, currentCorners[i].y + blockSize/2), PINK);
		}

		if((uchar)flowStatus[i] == 1) {
			line(img, currentCorners[i], previousCorners[i], GREEN, 2, 8, 0);
		}
	}
}

/**
 * Draw a circle for mean and stdDev of features and a trace of their changes over
 * the hand temporal window
 * */
void drawMeanAndStdDev(Mat img) {
//	int lastLeftIndex = leftHandFeatureMean.size() - 1;
//	int lastRightIndex = rightHandFeatureMean.size() - 1;
//	if(lastLeftIndex >= 0 && leftHandCenter.size() > 0) {
//		circle(img, leftHandFeatureMean.at(lastLeftIndex), leftHandFeatureStdDev.at(lastLeftIndex), YELLOW, 1, 4, 0);
//		line(img, leftHandFeatureMean.at(lastLeftIndex), leftHandCenter.at(leftHandCenter.size() - 1), YELLOW, 1, 4, 0);
//	}
//	if(lastRightIndex >= 0 && rightHandCenter.size() > 0) {
//		circle(img, rightHandFeatureMean.at(lastRightIndex), rightHandFeatureStdDev.at(lastRightIndex), YELLOW, 1, 4, 0);
//		line(img, rightHandFeatureMean.at(lastRightIndex), rightHandCenter.at(rightHandCenter.size() - 1), YELLOW, 1, 4, 0);
//	}
}

/**
 * assign features to hand(s)if at least one hand exist
 * */
void assignFeaturesToHands() {
	leftRightStatus.clear();
//	for(int i = 0; i < maxCorners; i++) {
//		if(leftHand.at(index()).isPresent() &&
//				(pointPolygonTest(leftHand.at(index()).getContour(), currentCorners[i], false) > -0.1)) {
//			//point is inside contour of the left hand
//			leftRightStatus.push_back(1);
//		} else if(rightHand.at(index()).isPresent() &&
//				(pointPolygonTest(rightHand.at(index()).getContour(), currentCorners[i], false) > -0.1)) {
//			leftRightStatus.push_back(2);
//		} else {
//			//this is noise or some other object
//			leftRightStatus.push_back(0); //Neither hand
//		}
//	}
	for(int i = 0; i < maxCorners; i++) {
		if(leftHand.at(index()).isPresent() &&
				getDistance(currentCorners[i], leftHand.at(index()).getMinCircleCenter()) < leftHand.at(index()).getMinCircleRadius()) {
			//point is inside contour of the left hand
			leftRightStatus.push_back(1);
		} else if(rightHand.at(index()).isPresent() &&
				getDistance(currentCorners[i], rightHand.at(index()).getMinCircleCenter()) < rightHand.at(index()).getMinCircleRadius()) {
			leftRightStatus.push_back(2);
		} else {
			//this is noise or some other object
			leftRightStatus.push_back(0); //Neither hand
		}
	}
}

/**
 * Calculate the distance between two points
 * */
float getDistance(const Point2f a, const Point2f b) {
	return sqrt(pow((a.x - b.x), 2) + pow((a.y - b.y), 2));
}

/**
 * Find mean point and standard deviation of features for each hand
 * @Precondition: assignFeatureToHands is executed
 * */
void meanAndStdDevExtract() {
	//first calculate the mean
	Point2f leftMean, rightMean;
	uint leftCount = 0, rightCount = 0;
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1) {
			//Left hand
			leftMean += currentCorners[i];
			leftCount++;
		} else if(leftRightStatus[i] == 2) {
			//Right hand
			rightMean += currentCorners[i];
			rightCount++;
		} else {
			//well, nothing if the feature is not assigned to a hand
		}
	}

	leftMean = Point2f(leftMean.x / leftCount, leftMean.y / leftCount);
	rightMean = Point2f(rightMean.x / rightCount, rightMean.y / rightCount);

	//Now that we have the mean, calculate stdDev
	float leftStdDev = 0, rightStdDev = 0;
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1) {
			//Left hand
			leftStdDev += getDistance(leftMean, currentCorners[i]);
		} else if(leftRightStatus[i] == 2) {
			//Right hand
			rightStdDev += getDistance(rightMean, currentCorners[i]);
		} else {
			//feature is not assigned to a hand
		}
	}
	leftStdDev = leftStdDev / leftCount;
	rightStdDev = rightStdDev / rightCount;

	//TODO: finish this function
	if(leftCount > 0) {
		//addFeatureMeanStdDev(true, leftMean, leftStdDev);
	}
	if(rightCount > 0) {
		//addFeatureMeanStdDev(false, rightMean, rightStdDev);
	}
}

/**
 * Calculate the depth of each feature based on the blurriness of its window
 * */
void featureDepthExtract(const Mat img) {
	Mat feature;
	Rect rect;
	Scalar stdDev;
	Scalar mean;
	featureDepth.clear();
	for(int i = 0; i < maxCorners; i++) {
		if(leftRightStatus[i] == 1 || leftRightStatus[i] == 2) {
			//left or right hand feature
			rect = Rect(currentCorners[i].x - blockSize/2, currentCorners[i].y - blockSize/2, blockSize, blockSize);
			feature = Mat(img, rect);

			//meanStdDev(feature, mean, stdDev);
			//featureDepth.push_back(stdDev.val[0]);
		} else {
			//Doesnt matter if the feature is not assigned to a hand, so set to -1
			featureDepth.push_back(-1);
		}
	}
}
