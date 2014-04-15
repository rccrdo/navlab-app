/*
 * Copyright (c) 2013 Riccardo Lucchese, riccardo.lucchese at gmail.com
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include <iostream>
#include <unistd.h>
#include <sstream>
#include "cv.h"
#include "v4l2_input.h"
#include "image-helpers.h"
#include "grabber.h"

using namespace cv;
using namespace std;

grabber::grabber() {
	this->_initialized = false;
	this->_width = -1;
	this->_height = -1;
}


grabber::~grabber() {
}


void grabber::dump(void) {
	std::cout << "Grabber info:\n"
		  << "  intialized : " << this->_initialized << "\n"
		  << "  width      : " << this->_width << "\n"
		  << "  height     : " << this->_height << "\n";
}


ulisse_grabber::ulisse_grabber(int port) : grabber() {
	char usb_num[2];
	char name[20] = "/dev/video";

	sprintf(usb_num, "%i", port);
	strcat(name, usb_num);

	this->_v4l2_layer.device_name = name;
	this->_v4l2_layer.input_index = 0;
	this->_v4l2_layer.n_buffers = 0;
	this->_v4l2_layer.enable_debug_time = false;
	this->_v4l2_layer.lower_layer = 0;
	this->_frame.full_height = __ULISSE_HEIGHT__;
	this->_frame.full_width = __ULISSE_WIDTH__;
	this->_v4l2_layer.frame_info = &this->_frame;

	v4l2_input_start_layer(&this->_v4l2_layer);
	v4l2_input_process(&this->_v4l2_layer);

	this->_imgbuf = cvCreateImage(cvSize(__ULISSE_WIDTH__, __ULISSE_HEIGHT__), IPL_DEPTH_8U, 3);
	
	this->_width = __ULISSE_WIDTH__;
	this->_height = __ULISSE_HEIGHT__;
	this->_initialized = true;
}


ulisse_grabber::~ulisse_grabber() {
}


cv::Mat *ulisse_grabber::capture(void) {

	if (!this->_initialized)
		return NULL;

	v4l2_input_process(&this->_v4l2_layer);

	// TODO: why not converting directly into _imgbuf->imgdata ?
	UYVY2BGR(this->_v4l2_layer.frame,  __ULISSE_WIDTH__*__ULISSE_HEIGHT__*2, this->_pixelbuf);

	memcpy(this->_imgbuf->imageData, this->_pixelbuf,  __ULISSE_WIDTH__*__ULISSE_HEIGHT__*3);

	return new cv::Mat(this->_imgbuf);
}



webcam_grabber::webcam_grabber(int id) : grabber() {
	this->_initialized = this->_vcapture.open(id);
	
	if (!this->_initialized) {
		cout << "error: webcam_grabber, failed to open stream id=" << id <<"\n";
	} else {
		this->_vcapture.set(CV_CAP_PROP_FRAME_WIDTH , __WEBCAM_WIDTH__);
		this->_vcapture.set(CV_CAP_PROP_FRAME_HEIGHT, __WEBCAM_HEIGHT__);

		this->_width = this->_vcapture.get(CV_CAP_PROP_FRAME_WIDTH);
		this->_height = this->_vcapture.get(CV_CAP_PROP_FRAME_HEIGHT);
		if (this->_width == -1)
			this->_width = __WEBCAM_WIDTH__;
		if (this->_height == -1)
			this->_height = __WEBCAM_HEIGHT__;		
	}
}


webcam_grabber::~webcam_grabber() {
	this->_vcapture.release();
}


cv::Mat *webcam_grabber::capture(void) {
	cv::Mat *mat;

	if (!this->_initialized)
		return NULL;

	// stream from webcam
	mat = new cv::Mat();
	this->_vcapture >> *mat;
	
	if (mat->cols != this->_width)
		this->_width = mat->cols;
	if (mat->rows != this->_height)
		this->_height = mat->rows;
	
	return mat;
}
