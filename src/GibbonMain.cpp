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
#include <math.h>
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
const uint hand_window_size = 12; //Number of frames to keep track of hand. Minimum of two is needed
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
double qualityLevel = 0.01;//0.01;
double minDistance = 10;
int blockSize = 26;
bool useHarrisDetector = false; //its either harris or cornerMinEigenVal

CameraPGR pgrCamera;
CameraPGR pgrObsCam1; //external camera for observing user

Message* message; //used by updateMessage() and inside the main loop
FileStorage logFile;
ofstream logFile2; //second log file is a CSV file with values from potential models
Mat logMatrixOne; //matrix containing data to log at each frame for hand one. cols = 24 + 6 * maxCorners rows = hand_window_size
Mat logMatrixTwo; //log matrix for hand two

bool wiz_grab = false;
bool wiz_release = false;
int frameCount = 0;
int fps = 0;
int record_number = 0; //used for logging. Incremented after each record
int log_num_cols = 24 + 6 * maxCorners; //number of columns in the log matrices
bool show_grid = false; //grid is used to visually inspect calibration
#define setting Setting::Instance()

int main(int argc, char* argv[]) {
	if(!setting->loadOptions(argc, argv)) {
		cout << "Error in loading options. Exiting the program." << endl;
		return -1;
	}
	verbosePrint("Starting ... ");

	if(!setting->is_daemon) {
		//cvNamedWindow( "Source", CV_WINDOW_AUTOSIZE ); 		//monochrome source
		//cvNamedWindow( "Processed", CV_WINDOW_AUTOSIZE ); 	//monochrome image after pre-processing
		cvNamedWindow( "Gibbon", CV_WINDOW_NORMAL); 	//Color with pretty drawings showing tracking results
		//cvSetWindowProperty("Tracked", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN); //Set tracked window to fullscreen

	}

	//print out key functions
	printKeys();

	init();
	start();

	if(!setting->is_daemon) {
		//cvDestroyWindow( "Source" );
		cvDestroyWindow( "Gibbon" );

		//cvDestroyWindow( "Processed" );
		verbosePrint("Bye bye!");
	}
	return 0;
}

/**
 * Initialize global variables
 */
void init() {
    logFile = FileStorage( setting->participant_number + "_log.yml", FileStorage::WRITE );
    string log2name = setting->participant_number + "_log.csv";
    logFile2.open ( log2name.c_str(), ios_base::app );
    logMatrixOne = ( Mat_<float>( hand_window_size, log_num_cols ));
    logMatrixTwo = ( Mat_<float>( hand_window_size, log_num_cols ));
    setLog2Headers();
	message = new Message();
}

/**
 * add headers to the second log file
 * @precondition: logFile2 ofstream exist and is initialized
 */
void setLog2Headers() {
    logFile2    << "record_number, "
                << "frame_number, "
                << "hand_number, "
                << "action_type, "
                << "raw_time, "
                << "time_stamp, "
                << "fps, ";
    for (int i = 0; i < hand_window_size; i++) {
        logFile2 << i <<"_steps_back, "
                << "min_rect.center.x, "
                << "min_rect.center.y, "
                << "min_rect.size.width, "
                << "min_rect.size.height, "
                << "min_rect.angle, "
                << "min_circle.center.x, "
                << "min_circle.center.y, "
                << "min_circle.radius, "
                << "mass.center.x, "
                << "mass.center.y, "
                << "feature_mean.x, "
                << "feature_mean.y, "
                << "feature_StdDev, "
                << "num_of_features,";
    }
    logFile2 << endl;
    //logFile2.flush();
    verbosePrint("LogFile2 Headers Written.");
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
			message->newHand(handOne.at(previousIndex(stepsBack)));
		} else {
			message->newHand(handOne.at(index()));
		}
	} else if(handOne.at(index()).isPresent()) {
		//Update existing hand
		if(handOne.at(index()).hasGesture()) {
			message->removeHand(handOne.at(index()));
			handOne.at(index()).setPresent(false);
		} else {
			message->updateHand(handOne.at(index()));
		}
	} else if((!handOne.at(index()).isPresent()) && handOne.at(previousIndex()).isPresent()) {
		//ask for remove
		message->removeHand(handOne.at(index()));
	} else {
		//Peace and quiet here. Nothing to do.
	}

	if(handTwo.at(index()).isPresent() && (!handTwo.at(previousIndex()).isPresent())) {
		//New hand!
		if(handTwo.at(previousIndex()).hasGesture()) {
			//go back 4 step to get closer to initial location gesture started at
			handTwo.at(previousIndex(stepsBack)).setGesture(handTwo.at(previousIndex()).getGesture());
			message->newHand(handTwo.at(previousIndex(stepsBack)));
		} else {
			message->newHand(handTwo.at(index()));
		}
	} else if(handTwo.at(index()).isPresent()) {
		//Update existing hand
		if(handTwo.at(index()).hasGesture()) {
			message->removeHand(handTwo.at(index()));
			handTwo.at(index()).setPresent(false);
		} else {
			message->updateHand(handTwo.at(index()));
		}
	} else if((!handTwo.at(index()).isPresent()) && handTwo.at(previousIndex()).isPresent()){
		//no hand, so ask for remove
		message->removeHand(handTwo.at(index()));
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
            } else {
                break;
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
            } else {
                break;
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
		case 'j':
			//simulate grab
            if(handOne.at(index()).isPresent()) {
                handOne.at(index()).setGesture(GESTURE_GRAB);
                message->newHand(handOne.at(index()));
                saveRecord("GRAB", 1);
            }
            if(handTwo.at(index()).isPresent()) {
                handTwo.at(index()).setGesture(GESTURE_GRAB);
                message->newHand(handTwo.at(index()));
                saveRecord("GRAB", 2);
            }
            verbosePrint("wizard says GRAB");
			break;
		case 'k':
			//simulate release
            if(handOne.at(index()).isPresent()) {
                handOne.at(index()).setGesture(GESTURE_RELEASE);
                message->newHand(handOne.at(index()));
                saveRecord("RELEASE", 1);
            }
            if(handTwo.at(index()).isPresent()) {
                handTwo.at(index()).setGesture(GESTURE_RELEASE);
                message->newHand(handTwo.at(index()));
                saveRecord("RELEASE", 2);
            }
			verbosePrint("wizard says RELEASE");
			break;
        case 'g':
            show_grid = !show_grid;
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
    //vector<Vec4i> hiearchy;

	VideoCapture video(setting->input_video_path);

	//Get external cameras
	//VideoCapture externalCamOne(0); // open the first USB camera
	//CvCapture *externalCamOne = 0;
	//externalCamOne = cvCaptureFromCAM(0);
	//externalCamOne.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	//externalCamOne.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	//externalCamOne.set(CV_CAP_PROP_FPS, 10);
//	if(externalCamOne) {
//		verbosePrint("USB Camera 1 is detected");
//	} else {
//		verbosePrint("NO external cam detected");
//	}

	if(setting->pgr_obs_cam1_index >=0) {
        pgrObsCam1.init(setting->pgr_obs_cam1_index, false, true); //color
	}

	if(setting->pgr_cam_index >= 0) {
        pgrCamera.init(setting->pgr_cam_index, true, false); //monochrome
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
    Mat obs1Frame;
	Mat tmpEigenBGR; //temporary color version of eigen value (touch) image to display
	Mat displayResults;

    Mat roiImgResult_topLeft;
    Mat roiImgResult_topRight;
    Mat roiImgResult_lowerRight;

	//Mat watershed_markers = cvCreateImage( setting->imageSize, IPL_DEPTH_32S, 1 );
	//Mat watershed_image;

	flowCount = vector<float>(maxCorners);
	char key = 'a';
	timeval first_time, second_time; //for fps calculation
	time_t rawtime; //time to display
	std::stringstream fps_str;
    std::stringstream resolution_str;
    std::stringstream record_str;
	std::stringstream tuio_str;
    string time_str;
	gettimeofday(&first_time, 0);
    fps = 0;
	while(key != 'q') {
		if(setting->pgr_obs_cam1_index >=0){
            //obs1Frame.release();
            obs1Frame = pgrObsCam1.grabImage();
		}

		if(setting->pgr_cam_index >= 0){
            //currentFrame.release();
			currentFrame = pgrCamera.grabImage();

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

		message->init();

		if(!setting->is_daemon) {
			time(&rawtime);
			//imshow("Source", currentFrame);
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
         */
        //binaryImg.release();
		binaryImg = currentFrame.clone();
		threshold(currentFrame, binaryImg, setting->lower_threshold, setting->upper_threshold, THRESH_BINARY);

		if(setting->capture_snapshot) {
			imwrite(setting->snapshot_path + ctime(&rawtime) + "_binary.png", binaryImg);
		}

		//clean up the current frame from noise using median blur filter
		medianBlur(binaryImg, binaryImg, setting->median_blur_factor);
		if(setting->capture_snapshot) {
			imwrite(setting->snapshot_path + ctime(&rawtime) + "_median.png", binaryImg);
		}

		//adaptiveThreshold(binaryImg, binaryImg, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 3, 10); //adaptive thresholding not works so well here
        //touchImage.release();
		touchImage = Mat(currentFrame.size(), CV_32FC1);
		sharpnessImage(currentFrame, touchImage);
		touchImage.convertTo(touchImage, CV_8UC1, 50, 0);

		if(!setting->is_daemon) {
			//imshow("Binary", binaryImg);
			//imshow("Touch", touchImage);
            //imshow("test", currentFrame);
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

		//cvtColor(currentFrame, watershed_image, CV_GRAY2BGR);
		//watershed(watershed_image, touchImage);
		//imshow("Watershed", touchImage);

		findHands(contours);
        setFeatureMats();
		if(numberOfHands() > 0) {
			//findGoodFeatures(previousFrame, currentFrame);
			findGoodFeatures(previousTouchImage, touchImage);
			featureDepthExtract(touchImage);
			assignFeaturesToHands();
			meanAndStdDevExtract();
			if(setting->wiz_of_oz) {
				//No need for system gesture tracking
			} else {
				GestureTracker::checkGestures(&handOne);
				GestureTracker::checkGestures(&handTwo);
			}
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

        //show the overlay grid if requested
        if(show_grid && !setting->is_daemon) {
            cvNamedWindow("Grid", CV_WINDOW_NORMAL);
            //cvMoveWindow("Grid", 0, 0);
            //cvSetWindowProperty("Grid", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
            drawGrid(trackingResults);
            imshow("Grid", trackingResults);
        } else {
            cvDestroyWindow("Grid");
        }

		if(!setting->is_daemon) {
			//Combine images to display
			cvtColor(touchImage, tmpEigenBGR, CV_GRAY2BGR);
            //displayResults.release();
            displayResults = Mat(trackingResults.rows + tmpEigenBGR.rows, trackingResults.cols + trackingResults.cols, CV_8UC3, Scalar(30,10,10));
            roiImgResult_topLeft = displayResults(Rect(0, 0, trackingResults.cols, trackingResults.rows)); //Image will be on the top left part
            roiImgResult_topRight = displayResults(Rect(trackingResults.cols, 0, trackingResults.cols, trackingResults.rows));
            roiImgResult_lowerRight = displayResults(Rect(trackingResults.cols, trackingResults.rows, tmpEigenBGR.cols, tmpEigenBGR.rows));
			trackingResults.copyTo(roiImgResult_topLeft);
            obs1Frame.copyTo(roiImgResult_topRight);
			tmpEigenBGR.copyTo(roiImgResult_lowerRight);

            //add fps info
			putText(displayResults, fps_str.str(), Point(20, trackingResults.rows + 30), FONT_HERSHEY_COMPLEX_SMALL, 1, GREEN, 1, 8, false);

            //add timestamp to the image
            //time_str = ctime(&rawtime);
            //time_str.erase(time_str.find_last_not_of(" \n")+1); //trim "mandatory" return off
            putText(displayResults, ctime(&rawtime), Point(20, trackingResults.rows + 60), FONT_HERSHEY_COMPLEX_SMALL, 1, YELLOW, 1, 8, false);
			putText(displayResults, setting->participant_number, Point(20, trackingResults.rows + 90), FONT_HERSHEY_COMPLEX_SMALL, 1, GREEN, 1, 8, false);


			if(setting->capture_snapshot) {
				//TODO: check for existing files and increment accordingly - or use date
				imwrite(setting->snapshot_path + ctime(&rawtime) + "_source.png", currentFrame);
				imwrite(setting->snapshot_path + ctime(&rawtime) + "_touch.png", touchImage);
				imwrite(setting->snapshot_path + ctime(&rawtime) + "_tracking_result.png", trackingResults);
				imwrite(setting->snapshot_path + ctime(&rawtime) + "_display_result.png", displayResults);
				putText(displayResults, "Snapshot OK!", Point(400,420), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
				setting->capture_snapshot = false;
			}
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

		if(!setting->is_daemon) {
            //add tuio info
            if(setting->send_tuio) {
                tuio_str.str("");
                tuio_str << "TUIO -> " << setting->tuio_host << ":" << setting->tuio_port;
                putText(displayResults, tuio_str.str(), Point(20, trackingResults.rows + 120), FONT_HERSHEY_COMPLEX_SMALL, 1, YELLOW, 1, 8, false);
            }
            //add resolution info
            resolution_str.str("");
            resolution_str << "Resolution: ["  << trackingResults.cols << "X" << trackingResults.rows << "]";
            putText(displayResults, resolution_str.str(), Point(20, trackingResults.rows + 150), FONT_HERSHEY_COMPLEX_SMALL, 1, GREEN, 1, 8, false);

            //add record number
            record_str.str("");
            record_str << "Record: #" << record_number;
            putText(displayResults, record_str.str(), Point(20, trackingResults.rows + 180), FONT_HERSHEY_COMPLEX_SMALL, 1, YELLOW, 1, 8, false);

			if(setting->save_output_video){
				if (resultWriter.isOpened()) {
					resultWriter << displayResults;
					putText(displayResults, "Recording Results ... ", Point(300, trackingResults.rows + 30), FONT_HERSHEY_COMPLEX, 1, RED, 3, 8, false);
				} else {
                    resultWriter = VideoWriter(setting->result_recording_path + setting->participant_number + "_" + ctime(&rawtime) + ".avi", CV_FOURCC('D', 'X', '5', '0'), fps,
							Size(displayResults.cols, displayResults.rows));
				}
			}
			if(setting->save_input_video){
				//actually saving is done before pre processing above
				putText(displayResults, "Recording Source ... ", Point(40,40), FONT_HERSHEY_COMPLEX, 1, YELLOW, 3, 8, false);
			}
			if(!setting->is_daemon) {
				imshow("Gibbon", displayResults);
			}
		}

		message->commit();
        //previousFrame.release();
		previousFrame = currentFrame;

		currentCorners = previousCorners;

        //previousTouchImage.release();
		previousTouchImage = touchImage;
		frameCount++;

        /*//release some memory before going to next frame
        trackingResults.release();
        binaryImg.release(); //binary image for finding contours of the hand
        tmpColor.release();
        touchImage.release();
        obs1Frame.release();
        tmpEigenBGR.release(); //temporary color version of eigen value (touch) image to display
        displayResults.release();
        currentFrame.release();
        */
	}

	//Clean up before leaving
	previousFrame.release();
	currentFrame.release();
	trackingResults.release();
	tmpColor.release();
    logFile.release();
	if(setting->pgr_cam_index >= 0){
		pgrCamera.~CameraPGR();
	}
    if(setting->pgr_obs_cam1_index >=0){
        pgrObsCam1.~CameraPGR();
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
 * calculate feature matrix for each hand in the temporal window that just passed and store it in each hand matrix
 * @precondition: this method should be called after findHands() has been called for current index()
 * this method runs for every frame
 */
void setFeatureMats() {
    if(handOne.at(index()).isPresent()) {
        Hand h = handOne.at(index());
        Moments m = handOne.at(index()).getMoments();
        logMatrixOne.pop_back(1);
        Mat tmpMatrix = (Mat_<float>(1, log_num_cols) << record_number,
                         m.m00, m.m01, m.m02, m.m03, m.m10, m.m11, m.m12, m.m20, m.m21, m.m30,
                         h.getMinRect().center.x, h.getMinRect().center.y,
                         h.getMinRect().size.width, h.getMinRect().size.height, h.getMinRect().angle,
                         h.getFeatureMean().x, h.getFeatureMean().y, h.getFeatureStdDev(),
                         h.getMinCircleCenter().x, h.getMinCircleCenter().y, h.getMinCircleRadius());
        int offset = 22; //number of values added above
        for(int i = 0, j = offset; i < maxCorners; i++, j+=5) {
            if(h.getFeatures().size() > i) {
                tmpMatrix.at<float>(0, j) = h.getFeatures()[i].x;
                tmpMatrix.at<float>(0, j + 1) = h.getFeatures()[i].y;
                tmpMatrix.at<float>(0, j + 2) = h.getFeaturesDepth()[i];
                tmpMatrix.at<float>(0, j + 3) = h.getVectors()[i].x;
                tmpMatrix.at<float>(0, j + 4) = h.getVectors()[i].y;
            } else {
                tmpMatrix.at<float>(0, j) = 0;
                tmpMatrix.at<float>(0, j + 1) = 0;
                tmpMatrix.at<float>(0, j + 2) = 0;
                tmpMatrix.at<float>(0, j + 3) = 0;
                tmpMatrix.at<float>(0, j + 4) = 0;
            }
        }
        tmpMatrix.push_back(logMatrixOne);
        logMatrixOne = tmpMatrix;
    }
    if(handTwo.at(index()).isPresent()) {
        Hand h = handTwo.at(index());
        Moments m = handTwo.at(index()).getMoments();
        logMatrixTwo.pop_back(1);
        Mat tmpMatrix = (Mat_<float>(1, log_num_cols) << record_number,
                         m.m00, m.m01, m.m02, m.m03, m.m10, m.m11, m.m12, m.m20, m.m21, m.m30,
                         h.getMinRect().center.x, h.getMinRect().center.y,
                         h.getMinRect().size.width, h.getMinRect().size.height, h.getMinRect().angle,
                         h.getFeatureMean().x, h.getFeatureMean().y, h.getFeatureStdDev(),
                         h.getMinCircleCenter().x, h.getMinCircleCenter().y, h.getMinCircleRadius());
        int offset = 22; //number of values added above
        for(int i = 0, j = offset; i < maxCorners; i++, j+=5) {
            if(h.getFeatures().size() > i) {
                tmpMatrix.at<float>(j) = h.getFeatures()[i].x;
                tmpMatrix.at<float>(j + 1) = h.getFeatures()[i].y;
                tmpMatrix.at<float>(j + 2) = h.getFeaturesDepth()[i];
                tmpMatrix.at<float>(j + 3) = h.getVectors()[i].x;
                tmpMatrix.at<float>(j + 4) = h.getVectors()[i].y;
            } else {
                tmpMatrix.at<float>(0, j) = 0;
                tmpMatrix.at<float>(0, j + 1) = 0;
                tmpMatrix.at<float>(0, j + 2) = 0;
                tmpMatrix.at<float>(0, j + 3) = 0;
                tmpMatrix.at<float>(0, j + 4) = 0;
            }
        }
        tmpMatrix.push_back(logMatrixTwo);
        logMatrixTwo = tmpMatrix;
    }
}

/**
 * saves up to two records in the log file depending on how many hands are present
 * takes gst as an string argument (for gesture such as "grab" and "release")
 */
void saveRecord(string gst, int hand_number) {
    time_t rawtime;
    time(&rawtime);
    string time_str = ctime(&rawtime);
    time_str.erase(time_str.find_last_not_of(" \n\r\t")+1); //trim "mandatory" return off
    if(hand_number == 1) {
        //save data to main log file (yml format)
        logFile << "record" << record_number;
        logFile << "frame" << frameCount;
        logFile << "fps" << fps;
        logFile << "gesture" << gst;
        logFile << "raw_time" << (float)rawtime;
        logFile << "time" << time_str;
        logFile << "hand_side" << handOne.at(index()).getHandSide();
        logFile << "features" << logMatrixOne;

        //save data to second log file (csv format)
        logFile2    <<  record_number << ','
                    << frameCount << ','
                    << handOne.at(index()).getHandSide() << ','
                    << gst << ','
                    << rawtime << ','
                    << time_str << ','
                    << fps << ',';
        for (int i = 0; i < hand_window_size; i++) {
            if(handOne.at(previousIndex(i)).isPresent()) {
                logFile2 << i << ','
                << handOne.at(previousIndex(i)).getMinRect().center.x << ','
                << handOne.at(previousIndex(i)).getMinRect().center.y << ','
                << handOne.at(previousIndex(i)).getMinRect().size.width << ','
                << handOne.at(previousIndex(i)).getMinRect().size.height << ','
                << handOne.at(previousIndex(i)).getMinRect().angle << ','
                << handOne.at(previousIndex(i)).getMinCircleCenter().x << ','
                << handOne.at(previousIndex(i)).getMinCircleCenter().y << ','
                << handOne.at(previousIndex(i)).getMinCircleRadius() << ','
                << handOne.at(previousIndex(i)).getMassCenter().x << ','
                << handOne.at(previousIndex(i)).getMassCenter().y << ','
                << handOne.at(previousIndex(i)).getFeatureMean().x << ','
                << handOne.at(previousIndex(i)).getFeatureMean().y << ','
                << handOne.at(previousIndex(i)).getFeatureStdDev() << ','
                << handOne.at(previousIndex(i)).getNumOfFeatures() << ',';
            } else {
                logFile2 << i << ",0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,"; //should match the number of fields added in the if segment above
            }
            logFile2.flush();
        }
        logFile2 << endl;
    }else if(hand_number == 2) {
        logFile << "record" << record_number;
        logFile << "frame" << frameCount;
        logFile << "fps" << fps;
        logFile << "gesture" << gst;
        logFile << "raw_time" << (float)rawtime;
        logFile << "time" << time_str;
        logFile << "hand_side" << handTwo.at(index()).getHandSide();
        logFile << "features" << logMatrixTwo;

        //save data to second log file (csv format)
        logFile2    <<  record_number << ','
                    << frameCount << ','
                    << handTwo.at(index()).getHandSide() << ','
                    << gst << ','
                    << rawtime << ','
                    << time_str << ','
                    << fps << ',';
        for (int i = 0; i < hand_window_size; i++) {
            if(handTwo.at(previousIndex(i)).isPresent()) {
                logFile2 << i << ','
                << handTwo.at(previousIndex(i)).getMinRect().center.x << ','
                << handTwo.at(previousIndex(i)).getMinRect().center.y << ','
                << handTwo.at(previousIndex(i)).getMinRect().size.width << ','
                << handTwo.at(previousIndex(i)).getMinRect().size.height << ','
                << handTwo.at(previousIndex(i)).getMinRect().angle << ','
                << handTwo.at(previousIndex(i)).getMinCircleCenter().x << ','
                << handTwo.at(previousIndex(i)).getMinCircleCenter().y << ','
                << handTwo.at(previousIndex(i)).getMinCircleRadius() << ','
                << handTwo.at(previousIndex(i)).getMassCenter().x << ','
                << handTwo.at(previousIndex(i)).getMassCenter().y << ','
                << handTwo.at(previousIndex(i)).getFeatureMean().x << ','
                << handTwo.at(previousIndex(i)).getFeatureMean().y << ','
                << handTwo.at(previousIndex(i)).getFeatureStdDev() << ','
                << handTwo.at(previousIndex(i)).getNumOfFeatures() << ',';
            } else {
                logFile2 << i << ",0,0,0,0, 0,0,0,0,0, 0,0,0,0,0,"; //should match the number of fields added in the if segment above
            }
            logFile2.flush();
        }
        logFile2 << endl;
    }
    logFile2.flush();
    record_number += 1;
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
            int base_radius = 100;
            int scaled_depth = sqrt( 1 + base_radius * featureDepth[i] / 2);
            if (scaled_depth < 0) {
                //integer overflow
                scaled_depth = base_radius;
            }
            if(scaled_depth < base_radius - blockSize) {
				circle(img, points[i], base_radius - scaled_depth, RED, 1, CV_AA);
            } else {
                //visualize touch with a bold red circle
                circle(img, points[i], blockSize, RED, 6, CV_AA);
            }

			//visualize feature orientation with a line
			line(img, points[i], points[i] + orientation[i], GREEN, 3, CV_AA);
		}
	}

	//Second hand
	if(handTwo.at(index()).isPresent()) {
		vector<Point2f> points = handTwo.at(index()).getFeatures();
		vector<Point2f> vectors = handTwo.at(index()).getVectors();
		vector<float> featureDepth = handTwo.at(index()).getFeaturesDepth();
		vector<Point2f> orientation = handTwo.at(index()).getFeatureOrientation();
		for (uint i = 0; i < points.size(); i++) {
			//draw feature box
			rectangle(img, Point(points[i].x - blockSize/2, points[i].y - blockSize/2), Point(points[i].x + blockSize/2, points[i].y + blockSize/2), BLUE);
			//TODO: draw feature trace

			//draw feature direction vector
			line(img, points[i], (points[i] + vectors[i]), BLUE, 2, 8, 0);

			//visualize feature depth and touch with a circle
            int base_radius = 100;
            int scaled_depth = sqrt( 1 + base_radius * featureDepth[i] / 2);
            if (scaled_depth < 0) {
                //integer overflow
                scaled_depth = base_radius;
            }
            if(scaled_depth < base_radius - blockSize) {
				circle(img, points[i], base_radius - scaled_depth, RED, 1, CV_AA);
            } else {
                //visualize touch with a bold red circle
                circle(img, points[i], blockSize, RED, 6, CV_AA);
            }

			//visualize feature orientation with a line
            line(img, points[i], points[i] + orientation[i], GREEN, 2, CV_AA);
		}
	}

}

/**
 * Draw a grid on the image to help inspect calibration
 */
void drawGrid(Mat img) {
    int num_lines_x = 10; //vertical lines
    int num_lines_y = 6; //horizontal lines
    int x_offset = (int)img.cols / num_lines_x;
    int y_offset = (int)img.rows / num_lines_y;
    //draw the vertical lines
    for(int i = 1; i < num_lines_x; i++) {
        line(img, Point(i * x_offset, 0), Point(i*x_offset, img.rows), YELLOW, 1, 4, 0);
    }
    //draw the horizontal lines
    for(int i = 1; i < num_lines_y; i++) {
        line(img, Point(0, i * y_offset), Point(img.cols, i*y_offset), YELLOW, 1, 4, 0);
    }
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
                    Point2f orientation = currentCorners[i] - handTwo.at(index()).getMinRectCenter();
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
		<< "'j' - simulate grab (for user study)" << endl
		<< "'k' - simulate release (for user study)" << endl
		<< "'q' - quit application" << endl
        << "'g' - toggle grid (for testing calibration)" << endl
		<< "'h' - print this message" << endl << endl;
}
