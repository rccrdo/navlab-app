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

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "cv.h"
#include "highgui.h"
#include "helpers.h"
#include "camera.h"


using namespace cv;
using namespace std;


#define DSTATE_PATROLLING     0
#define DSTATE_PT_TRANSITION  1
#define DSTATE_TRACKING       2
#define DSTATE_TP_TRANSITION  3

struct detection_status {
	int state;
	int msec_activate_tracking;
	int msec_deactivate_tracking;
	struct timeval last_edge_time;
};

static struct detection_status dstatus;


static void detection_init(void) {
	dstatus.state = DSTATE_PATROLLING;
	dstatus.msec_activate_tracking = 250;
	dstatus.msec_deactivate_tracking = 500;
}


static void detection_report(bool obj_found) {
	bool update_time;

	update_time = false;
	if (obj_found) {
	 	switch (dstatus.state) {
 		case DSTATE_PATROLLING:
 			// start PT transition
			dstatus.state = DSTATE_PT_TRANSITION;
			update_time = true;
 			break;
 		case DSTATE_PT_TRANSITION: {
 			int ret, diff;
                        struct timeval now;

                        ret = gettimeofday(&now, NULL);
                        if (ret) {
                        	std::cout << "warning, detection_report(" << obj_found << ")::DSTATE_PT_TRANSITION, failed to get 'now'\n";
                        	break;
                        }

                        diff = timerdiff_msec(now, dstatus.last_edge_time);
                        if (diff > dstatus.msec_activate_tracking) {
				std::cout << "0->1@" << now.tv_sec << "." <<  setfill('0') << setw(6) << now.tv_usec << std::endl;
                        	dstatus.state = DSTATE_TRACKING;
                        }                       
 			break;
 		}
 		case DSTATE_TRACKING:
 			// nothing to do 
 			break;
 		case DSTATE_TP_TRANSITION:
 			// stop TP transition
			dstatus.state = DSTATE_TRACKING;
 			break;
 		}
 	} else {
	 	switch (dstatus.state) {
 		case DSTATE_PATROLLING:
 			// nothing to do
 			break;
 		case DSTATE_PT_TRANSITION: {
 			// stop PT transition
			dstatus.state = DSTATE_PATROLLING;
 			break;
 		}
 		case DSTATE_TRACKING:
 			// start TP transition
			dstatus.state = DSTATE_TP_TRANSITION;
			update_time = true;
 			break;
 		case DSTATE_TP_TRANSITION: {
 			int ret, diff;
                        struct timeval now;

                        ret = gettimeofday(&now, NULL);
                        if (ret) {
                        	std::cout << "warning, detection_report(" << obj_found << ")::DSTATE_TP_TRANSITION, failed to get 'now'\n";
                        	break;	
                        }

                        diff = timerdiff_msec(now, dstatus.last_edge_time);
                        if (diff > dstatus.msec_deactivate_tracking) {
				std::cout << "1->0@" << now.tv_sec << "." <<  setfill('0') << setw(6) << now.tv_usec << std::endl;
                        	dstatus.state = DSTATE_PATROLLING;
                        }                       
 			break;
 		}
 		}
 	}
	if (update_time) {
		int ret;
 			
		ret = gettimeofday(&dstatus.last_edge_time, NULL);
		if (ret)
			std::cout << "warning, detection_report(" << obj_found << "), failed to update time\n";
 	}
 	
 	if (dstatus.state == DSTATE_PATROLLING)
 		camera::switch_to_patrolling();
 	if (dstatus.state == DSTATE_TRACKING)
 		camera::switch_to_tracking();
}


// initial values for the HSV filter
static int H_MIN = 0;
static int H_MAX = 256;
static int S_MIN = 0;
static int S_MAX = 256;
static int V_MIN = 0;
static int V_MAX = 256;

// detection box
static int Y_MIN = 0;
static int Y_MAX = __DEFAULT_HEIGHT__;
static int X_MIN = 0;
static int X_MAX = __DEFAULT_WIDTH__;

// patrolling pan limits (+90 deg)
static int LPAN = 60;
static int RPAN = 120;
static int PAN_TESTING = 90;

// minimum and maximum object area
static int MIN_OBJECT_AREA = 3000;
static int MAX_OBJECT_AREA = (__DEFAULT_HEIGHT__*__DEFAULT_WIDTH__/2);

// ruler
static int K_ANGLE_OVER_PIX1000 = 100;

static int K_VEL100 = 100;

static const string windowName = "Original Image";
static const string windowName2 = "Thresholded Image";
static const string trackbarWindowName = "Trackbars";


static void *ctrl_thread(void *arg);


static void on_trackbar(int val, void *id) {
	long int _id;
	
	_id = (long int)id;
	switch (_id) {
	case 1:
		if (val >= H_MAX)
			setTrackbarPos("H_MIN", trackbarWindowName, H_MAX-1);
		else
			camera::set_hmin(val);
		break;
	case 2:
		if (val <= H_MIN)
			setTrackbarPos("H_MAX", trackbarWindowName, H_MIN+1);
		else
			camera::set_hmax(val);
		break;
	case 3:
		if (val >= S_MAX)
			setTrackbarPos("S_MIN", trackbarWindowName, S_MAX-1);
		else
			camera::set_smin(val);			
		break;
	case 4:
		if (val <= S_MIN)
			setTrackbarPos("S_MAX", trackbarWindowName, S_MIN+1);
		else
			camera::set_smax(val);
		break;
	case 5:
		if (val >= V_MAX)
			setTrackbarPos("V_MIN", trackbarWindowName, V_MAX-1);
		else
			camera::set_vmin(val);
		break;
	case 6:
		if (val <= V_MIN)
			setTrackbarPos("V_MAX", trackbarWindowName, V_MIN+1);
		else
			camera::set_vmax(val);
		break;
	case 7:
		if (val >= MAX_OBJECT_AREA)
			setTrackbarPos("MIN_OBJECT_AREA", trackbarWindowName, MAX_OBJECT_AREA-1);
		else
			camera::set_area_min(val);
		break;
	case 8:
		if (val <= MIN_OBJECT_AREA)
			setTrackbarPos("MAX_OBJECT_AREA", trackbarWindowName, MIN_OBJECT_AREA+1);
		break;
	case 9:
		if (val >= Y_MAX)
			setTrackbarPos("Y_MIN", trackbarWindowName, Y_MAX-1);
		else
			camera::set_ymin(val);
		break;
	case 10:
		if (val <= Y_MIN)
			setTrackbarPos("Y_MAX", trackbarWindowName, Y_MIN+1);
		else
			camera::set_ymax(val);
		break;
	case 11:
		if (val >= X_MAX)
			setTrackbarPos("X_MIN", trackbarWindowName, X_MAX-1);
		else
			camera::set_xmin(val);
		break;
	case 12:
		if (val <= Y_MIN)
			setTrackbarPos("X_MAX", trackbarWindowName, X_MIN+1);
		else
			camera::set_xmax(val);
		break;
	case 13:
		if (val > RPAN) {
			setTrackbarPos("LPAN", trackbarWindowName, RPAN);
		} else {
			camera::set_lpan(val-90);
		}
		break;
	case 14:
		if (val < LPAN) {
			setTrackbarPos("RPAN", trackbarWindowName, LPAN);
		} else {
			camera::set_rpan(val-90);
		}	
		break;
	case 15: {
		ptz_t pos;
		pos.pan = PAN_TESTING - 90.;	
		pos.tilt = camera::base_tilt();
		pos.zoom = 1.;
		camera::set_pos(pos);
	
		break;
	}	
	case 16: {
		if (val < 1) {
			setTrackbarPos("K_ANGLE_OVER_PIX1000", trackbarWindowName, 1);
		} else {
			camera::set_k_angle_over_pixel(double(val)/1000.);
		}	
		break;
	}
	case 17: {
		if (val < 10) {
			setTrackbarPos("K_VEL100", trackbarWindowName, 10);
		} else {
			camera::set_k_vel(double(val)/1000.);
		}	
		break;
	}
	default:
		break;	
	}
}


static void createTrackbars(){
	namedWindow(trackbarWindowName, 0);

	// Filter HSV settings
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, 256, on_trackbar, (void *)1);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, 256, on_trackbar, (void *)2);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, 256, on_trackbar, (void *)3);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, 256, on_trackbar, (void *)4);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, 256, on_trackbar, (void *)5);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, 256, on_trackbar, (void *)6);

	// Filter area settings
	createTrackbar("MIN_OBJ_AREA", trackbarWindowName, &MIN_OBJECT_AREA,
		       10000, on_trackbar, (void *)7);
	createTrackbar("MAX_OBJ_AREA", trackbarWindowName, &MAX_OBJECT_AREA,
		       10000, on_trackbar, (void *)8);

	// Patrolling<->Tracking transition settings
	createTrackbar("MSEC_ACTIVATE_TRACKING", trackbarWindowName,
		       &dstatus.msec_activate_tracking, 2000, on_trackbar);
	createTrackbar("MSEC_DEACTIVATE_TRACKING", trackbarWindowName,
		       &dstatus.msec_deactivate_tracking, 2000, on_trackbar);

	// Detection rectangle
	createTrackbar("Y_MIN", trackbarWindowName, &Y_MIN, __DEFAULT_HEIGHT__, on_trackbar, (void *)9);
	createTrackbar("Y_MAX", trackbarWindowName, &Y_MAX, __DEFAULT_HEIGHT__, on_trackbar, (void *)10);
	createTrackbar("X_MIN", trackbarWindowName, &X_MIN, __DEFAULT_WIDTH__, on_trackbar, (void *)11);
	createTrackbar("X_MAX", trackbarWindowName, &X_MAX, __DEFAULT_WIDTH__, on_trackbar, (void *)12);
	
	createTrackbar("LPAN", trackbarWindowName, &LPAN, 180, on_trackbar, (void *)13);
	createTrackbar("RPAN", trackbarWindowName, &RPAN, 180, on_trackbar, (void *)14);
	createTrackbar("PAN - TESTING", trackbarWindowName, &PAN_TESTING, 180, on_trackbar, (void *)15);
	createTrackbar("K_ANGLE_OVER_PIX1000", trackbarWindowName, &K_ANGLE_OVER_PIX1000, 500, on_trackbar, (void *)16);
	createTrackbar("K_VEL100", trackbarWindowName, &K_VEL100, 1000, on_trackbar, (void *)17);
}


static void draw_pointer(int x, int y, Mat &frame){
	Scalar color = Scalar(0,255,0);

	circle(frame, Point(x,y), 20, color,2);

	if (y-25>0)
		line(frame, Point(x,y), Point(x,y-25), color, 2);
	else
		line(frame, Point(x,y), Point(x,0), color, 2);

	if (y+25<frame.rows)
		line(frame, Point(x,y), Point(x,y+25), color, 2);
	else
		line(frame, Point(x,y), Point(x, frame.rows), color, 2);

	if (x-25>0)
		line(frame, Point(x,y), Point(x-25,y), color, 2);
	else
		line(frame, Point(x,y), Point(0,y), color, 2);

	if (x+25 < frame.cols)
		line(frame, Point(x,y), Point(x+25,y), color, 2);
	else
		line(frame, Point(x,y), Point(frame.cols, y), color, 2);
	

	{
		ptz_t pos;
		
		pos = camera::get_pos_estimate(x - frame.cols/2);
		putText(frame, int2str(x) + "," + int2str(y) + " - " + float2str(pos.pan), Point(x, y + 30), 1, 1, color, 2);
	}
}


static void filter_image(Mat &thresh){
	Scalar color_black = Scalar(0,0,0);

	// use a 3px by 3px rectangle to erode and 8by8 to dilate
	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3,3));
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8,8));
	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);
	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);
	
	// black out pixels out of the detection box
	rectangle(thresh, Point(0,0), Point(thresh.cols, Y_MIN), color_black, CV_FILLED);
	rectangle(thresh, Point(0,Y_MAX), Point(thresh.cols, thresh.rows), color_black, CV_FILLED);
	rectangle(thresh, Point(0,0), Point(X_MIN, thresh.rows), color_black, CV_FILLED);
	rectangle(thresh, Point(X_MAX,0), Point(thresh.cols, thresh.rows), color_black, CV_FILLED);
}


static int find_object(int &x, int &y, Mat threshold, Mat &vstream) {
	Mat temp;
	double refArea;
	bool objectFound;
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;

	threshold.copyTo(temp);
	
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	//use moments method to find our filtered object
	refArea = 0;
	objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects;
		const int MAX_NUM_OBJECTS = 50;	//max number of objects to be detected in frame

		numObjects = hierarchy.size();

		if (numObjects < MAX_NUM_OBJECTS) {
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {
				double area;
				Moments moment = moments((cv::Mat)contours[index]);

				area = moment.m00;

				if ((area > MIN_OBJECT_AREA) && (area < MAX_OBJECT_AREA)
				    && (area>refArea)) {
					int lx, ly;
					lx = moment.m10/area;
					ly = moment.m01/area;

					if (ly > Y_MAX)
						continue;
					if (ly < Y_MIN)
						continue;
					if (lx > X_MAX)
						continue;
					if (lx < X_MIN)
						continue;
					
					objectFound = true;
					refArea = area;
					x = lx;
					y = ly;
				}
			}
		} else {
			putText(vstream,"TOO MUCH NOISE! ADJUST FILTER",Point(0,50),1,2,Scalar(255,0,0),2);
		}
	}
	return objectFound;
}


void *tracking(void *arg) {
	int ctrl_rate_limiter;
	pthread_t ctrl_id;
	Mat HSV;
	Mat threshold;

	assert(arg == NULL);

	detection_init();

	// create slider bars for parameter tweaking
	MIN_OBJECT_AREA = camera::area_min();
	H_MIN = camera::hmin();
	H_MAX = camera::hmax();
	S_MIN = camera::smin();
	S_MAX = camera::smax();
	V_MIN = camera::vmin();
	V_MAX = camera::vmax();
	
	X_MIN = camera::xmin();
	X_MAX = camera::xmax();
	Y_MIN = camera::ymin();
	Y_MAX = camera::ymax();
	LPAN = camera::lpan() + 90;
	RPAN = camera::rpan() + 90;
	K_ANGLE_OVER_PIX1000 = camera::k_angle_over_pixel()*1000;
	K_VEL100 = camera::k_vel()*1000;
	createTrackbars();


	ctrl_rate_limiter = 0;
	while (true) {
		bool obj_found;
		int obj_x, obj_y;
		int frame_width, frame_height;
		cv::Mat *vstream_ptr;
		
		vstream_ptr = camera::capture();
		if (vstream_ptr == NULL) {
			std::cout << "warning, tracking(), camera::capture() returned NULL\n";
			return NULL;
		}

		cv::Mat vstream(*vstream_ptr);
		delete vstream_ptr;
		
		frame_width = vstream.cols;
		frame_height = vstream.rows;

		// convert frame from BGR to HSV colorspace
		cvtColor(vstream, HSV, CV_BGR2HSV);

		// filter HSV image between values and store filtered image to threshold matrix
		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);

		// denoise and emphasize the filtered object
		filter_image(threshold);

		obj_found = find_object(obj_x, obj_y, threshold, vstream);

		// draw a pointer on object location
		// ! this must be done before translating coords
		if (obj_found)
			draw_pointer(obj_x, obj_y, vstream);

		// provide visual feedback on current detection state
		if (camera::is_tracking())
			putText(vstream, " tracking", Point(0,80), 2, 1, Scalar(0,0,255), 2);
		else
			putText(vstream, " patrolling", Point(0,80), 2, 1, Scalar(0,255,0), 2);

		// translate the target location		
		obj_x -=  frame_width/2;
		obj_y -= frame_height/2;
		obj_y = -obj_y;

		{
			Scalar white_color = Scalar(255,255,255);
			Scalar line_color = Scalar(200,200,200);
			Scalar bline_color = Scalar(100,100,256);
			
			// detection box
			line(vstream, Point(X_MIN, Y_MAX), Point(X_MAX, Y_MAX), line_color, 2);
			line(vstream, Point(X_MIN, Y_MIN), Point(X_MAX, Y_MIN), line_color, 2);
			line(vstream, Point(X_MIN, Y_MIN), Point(X_MIN, Y_MAX), line_color, 2);
			line(vstream, Point(X_MAX, Y_MIN), Point(X_MAX, Y_MAX), line_color, 2);

			// pan degree ruler
			std::vector<double> ruler_angles;
			for (float i=-90; i <= 90; i+=2.5)
				ruler_angles.push_back(i);
			
			std::vector<double> *ruler_pos = camera::pan2dx(ruler_angles);
			assert (ruler_pos->size() == ruler_angles.size());
			for (unsigned i=0; i < ruler_angles.size(); i++) {
				double angle, x;
				std::string angle_str;
				
				angle = ruler_angles.at(i);
				x = ruler_pos->at(i)  + frame_width/2;// frame_width;

				//std::cout << "ruler: " << angle << ", " << x << "\n";
				line(vstream, Point(x, 0), Point(x, 20), white_color, 1);
				
				angle_str = float2str(angle);
				putText(vstream, angle_str, Point(x - 5*angle_str.size(), 35 + (i&1)*15), 1, 1, white_color, 1);
			}
			delete ruler_pos;
			
			// draw bisecting-lines
			std::vector<double> bangles = camera::bangles();
			for (unsigned int i=0; i < bangles.size(); i++) {
				double dx;
					
				dx = camera::pan2dx(bangles[i]) + frame_width/2;
				line(vstream, Point(dx, 0), Point(dx, frame_height), bline_color, 1);
				putText(vstream, float2str(bangles[i]), Point(dx + 5, frame_height -10), 1, 1, bline_color, 1);
			}
			
			// draw pan limits
			{
				double x_lpan, x_rpan;

				x_lpan = camera::pan2dx(camera::lpan() - camera::fov()/2) + frame_width/2;
				x_rpan = camera::pan2dx(camera::rpan() + camera::fov()/2) + frame_width/2;
			 
				line(vstream, Point(x_lpan, 0), Point(x_lpan, frame_height), bline_color, 3);
				putText(vstream, "L", Point(x_lpan + 5, frame_height -10), 2, 1, bline_color, 2);
				line(vstream, Point(x_rpan, 0), Point(x_rpan, frame_height), bline_color, 3);
				putText(vstream, "R ", Point(x_rpan + 5, frame_height -10), 2, 1, bline_color, 2);
			}

			imshow(windowName, vstream);
			imshow(windowName2, threshold);
		}

 		detection_report(obj_found);

		if (camera::is_tracking() && obj_found) {
			ptz_t target_ptz;
			struct timeval now;

			target_ptz = camera::get_pos_estimate(obj_x, &now);

			std::cout << "angle=" << target_ptz.pan <<"@" << now.tv_sec << "." <<  setfill('0') << setw(6) << now.tv_usec << std::endl;

			if (camera::is_ptz()) {
				ctrl_rate_limiter++;

				// limit ctrl action to once every 10 frames
				if (!(ctrl_rate_limiter % 10)) {
					int ret;
					Point2D *target_pos; 
		
					target_pos = (Point2D *)malloc(sizeof(Point2D));
					target_pos->x = obj_x;
					target_pos->y = obj_y;

					ret = pthread_create(&ctrl_id, NULL, ctrl_thread, (void *)target_pos);
					if (ret) {
						std::cout << "warning, tracking(), cannot create ctrl_thread\n";
						// this should really not happen ...
						exit(1);
					}
				}
			}
		} else {
			// reset the rate limiter
			ctrl_rate_limiter = 0;
		}

		{
			// handle user input
			char c = (char) waitKey(5);
			if (c != -1) {
				if (c == 27) {
					// esc
					camera::save_config();
					if (camera::is_ptz())
						camera::velocity(0,0);
					exit(0);
				}
			}
		}
	}

	return NULL;
}


static void *ctrl_thread(void *arg) {
        double lim_dx, lim_sx;
	Point2D *target_pos;

	assert(arg != NULL);
	assert(camera::is_ptz());

	target_pos = (Point2D *)arg;

	lim_dx = camera::pan2dx(camera::rpan());
	lim_sx = camera::pan2dx(camera::lpan());
	
	// stay inside the pan limits when tracking
	if (target_pos->x > lim_dx)
		target_pos->x = 0;

	if (target_pos->x < lim_sx)
		target_pos->x = 0;

	// move camera iff the pixel is outside the rectangle 20x20	
	if ((target_pos->x > -20) && (target_pos->x < 20)) {
		//&& (target_pos->y > -20) && (target_pos->y <20)) {
		
		// stop camera and seed the state estimator
		camera::velocity(0, 0);
		camera::ctrl_estimator_measure_pos();		

	} else if (target_pos->x < 800 && target_pos->y < 800) {
		double velx, vely, Kx, Ky;

		Kx = 0.15; // guadagno per la velocità di pan
		Ky = 0.; // don't rotate around tilt axis

		velx = Kx*target_pos->x;
		vely = -Ky*target_pos->y;

		// controllo di velocita'
		camera::velocity(velx, vely);
	}

	free(target_pos);

	return NULL;
}
