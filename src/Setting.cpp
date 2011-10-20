/*
 * Setting.cpp
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

#include "Setting.h"

#include <iostream>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/tokenizer.hpp>

#define GIBBON_VERSION "0.1"

namespace po = boost::program_options;

/**
 * Loads the options appropriately giving preference to options provided by user.
 * If there is error or users asks it will print help(TODO) and usage of this application to the terminal
 * */
bool Setting::loadOptions(int argc, char* argv[]) {
	try {
		//read program options
		po::options_description desc("Allowed options");
		desc.add_options()
		   ("help", "Display this message")
		   ("pgr-index", po::value<int>(&pgr_cam_index)->default_value(0), "index of pgr camera to use. Negative means do not use pgr camera")
		   ("obs-cam-index", po::value<int>(&pgr_obs_cam1_index)->default_value(1), "index of observer camera")
		   ("config-file", po::value<std::string>(&config_file_path),"Optionally provide a path to configuration file")
		   ("verbose", po::value<bool>(&verbose), "If you want me to keep talking set verbose to true")
		   ("is-daemon", po::value<bool>(&is_daemon), "In daemon mode there is no vidoe or visualization")
		   ("send-tuio", po::value<bool>(&send_tuio), "if true gestures are sent as tuio messages")
		   ("tuio-port", po::value<int>(&tuio_port), "Port to be used to deliver TUIO messages")
		   ("tuio-host", po::value<string>(&tuio_host), "Host for TUIO messages to go to")
		   ("source-recording-path", po::value<std::string>(&source_recording_path), "The path where video from camera will be saved without visualizations or annotation.")
		   ("result-recording-path", po::value<std::string>(&result_recording_path), "The path where annotated video with visualization of features and detecte gestures will be stored")
		   ("log-path", po::value<std::string>(&log_path), "The path for log file of detected gestures")
		   ("snapshot-path", po::value<std::string>(&snapshot_path), "The path to save snapshots of important images")
		   ("input-video-path", po::value<std::string>(&input_video_path),"Path to the input video to use instead of the camera")
		   ("lower-threshold", po::value<int>(&lower_threshold)->default_value(10), "Set the lower threshold")
		   ("upper-threshold", po::value<int>(&upper_threshold)->default_value(255), "set the upper threshold")
		   ("radius-threshold", po::value<int>(&radius_threshold)->default_value(20), "Set the lower threshold")
		   ("median-blur-factor", po::value<int>(&median_blur_factor)->default_value(7), "set the median blur factor for contour detection")
		   ("do-undistortion", po::value<bool>(&do_undistortion), "If true, camera image will be corrected for lens distortion")
		   ("imageOffsetX", po::value<float>(&imageOffsetX)->default_value(0), "x offset of image ROI")
		   ("imageOffsetY", po::value<float>(&imageOffsetY)->default_value(0), "y offset of image ROI")
		   ("imageSizeX", po::value<float>(&imageSizeX)->default_value(752), "width of image ROI")
		   ("imageSizeY", po::value<float>(&imageSizeY)->default_value(480), "height of image ROI")
		   ("undistortion-calibration-numChessboards", po::value<int>(&undistortion_calibration_numChessboards)->default_value(2), "number of chess boards to use for undistortion calibration")
		   ("undistortion-calibration-hCorners", po::value<int>(&undistortion_calibration_hCorners)->default_value(6), "number of horizontal inside corners in chess board image used for undistortion calibration")
		   ("undistortion-calibration-vCorners", po::value<int>(&undistortion_calibration_vCorners)->default_value(6), "number of vertical inside corners in chess board image used for undistortion calibration")
		   //("", po::value<std::string>(&),"")
		   ;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if ((!strcmp(argv[1], "-?") || !strcmp(argv[1], "--?") || !strcmp(argv[1], "/?") || !strcmp(argv[1], "/h") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--h") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "/help") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "help") ))
		{
			std::cout << "Bimanual Near Touch Tracker\n"
			<< "Version: " << GIBBON_VERSION
			<< "Authors:\n"
			<< "	Aras Balali Moghaddam\n"
			<< "	Nathan McLean\n"
			<< desc;
		   return false;
		}

		if (vm.count("config-file"))
{
			std::ifstream ifs(config_file_path.c_str());
			if (ifs) {
				store(parse_config_file(ifs, desc), vm);
				notify(vm);
			} else {
				cout << "error reading configuration file: " << config_file_path << endl;
				return false;
			}
		}

		if (verbose) {
			cout 	<< "Gibbon version " << GIBBON_VERSION
					<< "Verbose mode. :-) I am going to tell you what's happening ...\n"
					<< "\n****************** Configurations: ******************"
					<< "\nsend tuio	= " << send_tuio
					<< "\ntuio_port = " << tuio_port
					<< "\ntuio_host = " << tuio_host
					<< "\nis daemon	= " << is_daemon
					<< "\nlog path = " << log_path
					<< "\npgr camera index = " << pgr_cam_index
					<< "\nsource recording path	= " << source_recording_path
					<< "\nresult recording path	= " << result_recording_path
					<< "\nsnapshot path = " << snapshot_path
					<< "\ninput video path = " << input_video_path
					<< "\nconfig file path = " << config_file_path
					<< "\nlower threshold = " << lower_threshold
					<< "\nupper threshold = "	<< upper_threshold
					<< "\nmedian blur factor = " << median_blur_factor
					<< "\ndo undistortion = " << do_undistortion
					<< "\n*******************************************************"
					<< endl;
		}

		// if (vm.count("")) {	}

		} catch (exception& e) {
			if(verbose) {
				cout << "error in processing configuration parameters" << endl;
				cout << e.what() << endl;
			}
			return false;
		}
		return true;
}
