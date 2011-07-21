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
	CameraPGR();
	cv::Mat grabImage();
	void init();
	~CameraPGR();

private:
	IplImage* convertImageToOpenCV(Image* pImage);

	Camera pgrCam;
	PGRGuid guid;
    Format7ImageSettings fmt7ImageSettings;
    Format7PacketInfo fmt7PacketInfo;
	BusManager busManager;
	unsigned int totalCameras;

};

#endif /* CAMERAPGR_H_ */
