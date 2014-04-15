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

#ifndef __GRABBER_H__
#define __GRABBER_H__

#include "cv.h"
#include "highgui.h"

#include "v4l2_input.h"
#include "image-helpers.h"


#define __DEFAULT_WIDTH__    720
#define __DEFAULT_HEIGHT__   576
#define __ULISSE_WIDTH__   __DEFAULT_WIDTH__
#define __ULISSE_HEIGHT__  __DEFAULT_HEIGHT__
#define __WEBCAM_WIDTH__   640
#define __WEBCAM_HEIGHT__  480


class grabber {
public:
	grabber();
	~grabber();

	// capture a new frame into a Mat objec
	// ! the returned ptr to Mat must be freed by caller
	virtual cv::Mat *capture(void) = 0;
	
	void dump(void);

	inline bool initialized(void) {
		return this->_initialized;
	}
	
	inline int width(void) {
		return this->_width;
	}
	
	inline int height(void) {
		return this->_height;
	}

protected:
	bool _initialized;
	int _width, _height;
};


class webcam_grabber : public grabber {
public:
	webcam_grabber(int id);
	~webcam_grabber();

	cv::Mat *capture(void);

private:
	cv::VideoCapture _vcapture;
};


class ulisse_grabber : public grabber {
public:
	ulisse_grabber(int port);
	~ulisse_grabber();

	cv::Mat *capture(void);

private:
	frame_t _frame;
	v4l2_input_layer_t _v4l2_layer;
	IplImage *_imgbuf;
	unsigned char _pixelbuf[__ULISSE_WIDTH__*__ULISSE_HEIGHT__*3];
};

#endif

