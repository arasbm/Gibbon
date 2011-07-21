/*
 * Setting.h
 * Handle options, settings and modes.
 *
 *  Created on: 2011-07-05
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

#ifndef SETTING_H_
#define SETTING_H_

#include <iostream>
#include <fstream>
#include <string>

#include "cv.h"
#include <boost/program_options.hpp>

using namespace std;

/** Define some nice RGB colors for dark background **/
const cv::Scalar YELLOW = CV_RGB(255, 255, 51);
const cv::Scalar ORANGE = CV_RGB(255, 153, 51); //use for left hand
const cv::Scalar RED = CV_RGB(255, 51, 51);
const cv::Scalar PINK = CV_RGB(255, 51, 153);
const cv::Scalar GREEN = CV_RGB(153, 255, 51);
const cv::Scalar BLUE = CV_RGB(51, 153, 255); // use for right hand
const cv::Scalar OLIVE = CV_RGB(184, 184, 0);
const cv::Scalar RANDOM_COLOR = CV_RGB( rand()&255, rand()&255, rand()&255 ); //a random color


class Setting {

public:
	static Setting& Instance(){
	    static Setting singleton;
	    return singleton;
	}

    bool loadOptions(int argc, char* argv[]);

	/*** Global settings ***/
	int lower_threshold;
	int upper_threshold;
	int median_blur_factor;
	bool save_input_video;
	bool save_output_video;
	bool subtract_background;
	bool send_tuio;
	bool verbose; //when in verbose mode, process info is printed to terminal in various places
	bool is_daemon; //when in daemon mode no video is displayed and no drawing happens
	string source_recording_path;
	string result_recording_path;
	string snapshot_path;
	string log_path;
	string input_video_path;
	string config_file_path;
	int grab_std_dev_factor; // the rate at which stdDev is expected to change during grab and release gesture
	int tuio_port;
	string tuio_host;
	float undistortion_factor; //alpha factor for correcting image distortion. Should be in the range: [0 1] inclusive.
	bool do_undistortion;

	/*** modes ***/
	bool left_grab_mode;
	bool right_grab_mode;
	bool capture_snapshot; //capture and save an snapshot of all the important images

	/*** pgr camera serrings ***/
	int pgr_cam_index;
	cv::Size imageSize; //size of the image after RIO selection

private:
	Setting() {
		/** Setting Default Values **/
		/*** Global settings ***/
		lower_threshold = 10;
		upper_threshold = 200;
		median_blur_factor = 7;
		save_input_video = false;
		save_output_video = false;
		subtract_background = false;
		send_tuio = false;
		verbose = false; //when in verbose mode, process info is printed to terminal in various places
		is_daemon = false; //when in daemon mode no video is displayed and no drawing happens
		source_recording_path = "/home/arasbm/Desktop/grab_and_release_data/source.avi";
		result_recording_path = "/home/arasbm/Desktop/grab_and_release_data/result.avi";
		snapshot_path = "/home/arasbm/Desktop/grab_and_release_data/";
		log_path = "/home/arasbm/Desktop/grab_and_release_data/log.txt";
		input_video_path = "/mnt/arasbm_server/near_touch_data/test.avi";
		config_file_path = "config.ini";
		grab_std_dev_factor = 14; // the rate at which stdDev is expected to change during grab and release gesture
		tuio_port = 3333;
		tuio_host = "localhost";
		do_undistortion = true;
		undistortion_factor = 0.1;


		/*** modes ***/
		left_grab_mode = false;
		right_grab_mode = false;

		/*** pgr camera serrings ***/
		pgr_cam_index = 0;
		imageSize = cv::Size(580, 380); //size of the image after RIO selection
	};// Private constructor

	//Setting(const Setting&);                 // Prevent copy-construction
	Setting& operator=(const Setting&);        // Prevent assignment

};

#endif /* SETTING_H_ */
