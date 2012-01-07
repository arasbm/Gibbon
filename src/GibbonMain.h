/*
 * Gibbon.h
 *
 *  Created on: Jul 26, 2011
 *      Author: visid
 */

#ifndef GIBBON_H_
#define GIBBON_H_

//OpenCV include files.
#include "highgui.h"
#include "cv.h"
#include "cxcore.h"
#include "ml.h"
#include "cxtypes.h"

void processKey(char key);
void findHands(std::vector< std::vector<cv::Point> > contours);
void opencvConnectedComponent(cv::Mat* src, cv::Mat* dst);
void init();
void setLog2Headers();
void start();
void checkGrab();
void checkRelease();
void meanAndStdDevExtract();
void featureDepthExtract(cv::Mat img);
void findGoodFeatures(cv::Mat frame1, cv::Mat frame2);
void assignFeaturesToHands();
void drawFeatures(cv::Mat img);
void drawMeanAndStdDev(cv::Mat img);
void drawGrid(cv::Mat img);
float getDistance(const cv::Point2f a, const cv::Point2f b);
int numberOfHands();
int index();
void updateMessage();
int previousIndex();
int previousIndex(int i);
void printKeys();
void setFeatureMats();
void saveRecord(std::string gst, int hand_number);

#endif /* GIBBON_H_ */
