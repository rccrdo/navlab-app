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

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <vector>
#include <cmath>
#include "cv.h"
#include "types.h"
#include "grabber.h"


#define CAMERA_CTRL_POS 0
#define CAMERA_CTRL_VEL 1


class camera {
public:
	static inline cv::Mat *capture(void) {
		cv::Mat *vstream_ptr;

		camera *instance = camera::instance();
		if (!instance->_grabber)
			return NULL;

		vstream_ptr = instance->_grabber->capture();
		if (instance->_intrinsic && instance->_distcoeff) {
			cv::Mat *vstream_undistorted_ptr;
			
			vstream_undistorted_ptr = new cv::Mat();

			undistort(*vstream_ptr, *vstream_undistorted_ptr, *instance->_intrinsic, *instance->_distcoeff);
			//delete vstream_ptr;
			return vstream_undistorted_ptr;
		}

		return vstream_ptr;
	}


	static inline int grabber_width(void) {
		camera *instance = camera::instance();
		if (!instance->_grabber)
			return -1;

		return instance->_grabber->width();
	}
	

	static inline int grabber_height(void) {
		camera *instance = camera::instance();
		if (!instance->_grabber)
			return -1;

		return instance->_grabber->height();
	}


        static camera *instance();


	static void velocity(double vpan, double vtilt);
	static void velocity(vel_t vel);


	static std::vector<double> bangles() {
		camera *instance = camera::instance();
		return instance->_bangles;
	};


	static void stop() {
		camera *instance = camera::instance();
		instance->_patrolling = false;
		instance->_tracking = false;

		camera::velocity(0,0);
	};


	static void switch_to_patrolling() {
		camera *instance = camera::instance();
		instance->_patrolling = true;
		instance->_tracking = false;
	};


	static void switch_to_tracking() {
		camera *instance = camera::instance();
		instance->_patrolling = false;
		instance->_tracking = true;
	};


	static bool is_patrolling() {
		camera *instance = camera::instance();
		return instance->_patrolling;
	}


	static bool is_tracking() {
		camera *instance = camera::instance();
		return instance->_tracking;
	};


	static float lpan() {
		camera *instance = camera::instance();
		return instance->_lpan;
	};


	static float fov() {
		camera *instance = camera::instance();
		return instance->_fov;
	};


	static float rpan() {
		camera *instance = camera::instance();
		return instance->_rpan;
	};
	

	static int id() {
		camera *instance = camera::instance();
		return instance->_cam_id;
	};
	

	static bool is_ptz() {
		camera *instance = camera::instance();
		return instance->_ptz;
	};
	

	static bool is_fixed() {
		return !camera::is_ptz();
	};
	

	static int hmax() {
		camera *instance = camera::instance();
		return instance->_hmax;
	};


	static int hmin() {
		camera *instance = camera::instance();
		return instance->_hmin;
	};


	static int smax() {
		camera *instance = camera::instance();
		return instance->_smax;
	};


	static int smin() {
		camera *instance = camera::instance();
		return instance->_smin;
	};


	static int vmax() {
		camera *instance = camera::instance();
		return instance->_vmax;
	};


	static int vmin() {
		camera *instance = camera::instance();
		return instance->_vmin;
	};


	static int ymax() {
		camera *instance = camera::instance();
		return instance->_ymax;
	};


	static int ymin() {
		camera *instance = camera::instance();
		return instance->_ymin;
	};
	

	static int xmax() {
		camera *instance = camera::instance();
		return instance->_xmax;
	};


	static int xmin() {
		camera *instance = camera::instance();
		return instance->_xmin;
	};

	
	static int base_tilt() {
		camera *instance = camera::instance();
		return instance->_base_tilt;
	};

	
	static int area_min() {
		camera *instance = camera::instance();
		return instance->_area_min;
	};

	
	static void set_lpan(int val) {
		camera *instance = camera::instance();
		instance->_lpan = val;
	};
	

	static void set_rpan(int val) {
		camera *instance = camera::instance();
		instance->_rpan = val;
	};

	
	static void set_hmin(int val) {
		camera *instance = camera::instance();
		instance->_hmin = val;
	};
	

	static void set_hmax(int val) {
		camera *instance = camera::instance();
		instance->_hmax = val;
	};

	
	static void set_smin(int val) {
		camera *instance = camera::instance();
		instance->_smin = val;
	};

	
	static void set_smax(int val) {
		camera *instance = camera::instance();
		instance->_smax = val;
	};
	

	static void set_vmin(int val) {
		camera *instance = camera::instance();
		instance->_vmin = val;
	};
	

	static void set_vmax(int val) {
		camera *instance = camera::instance();
		instance->_vmax = val;
	};

	
	static void set_area_min(int val) {
		camera *instance = camera::instance();
		instance->_area_min = val;
	};

	
	static void set_ymin(int val) {
		camera *instance = camera::instance();
		instance->_ymin = val;
	};
	

	static void set_ymax(int val) {
		camera *instance = camera::instance();
		instance->_ymax = val;
	};


	static void set_xmin(int val) {
		camera *instance = camera::instance();
		instance->_xmin = val;
	};

	
	static void set_xmax(int val) {
		camera *instance = camera::instance();
		instance->_xmax = val;
	};


	static int serial_port(void) {
		camera *instance = camera::instance();
		return instance->_serial_port;
	};
	

	static ptz_t get_pos_estimate(double dx=0, struct timeval *tv=NULL) {
		ptz_t pos;

		camera *instance = camera::instance();
		instance->_ctrl_step_estimate();
		pos = instance->_ctrl_last_pos;
		if (tv != NULL)
			*tv = instance->_ctrl_last_pos_time;

		pos.pan += dx * instance->_k_angle_over_pixel;

		return pos;
	};

	
	static double k_angle_over_pixel(void) {
		camera *instance = camera::instance();
		return instance->_k_angle_over_pixel;		
	}


	static void set_k_angle_over_pixel(double k) {
		camera *instance = camera::instance();
		k = fabs(k);
		if (k > 0.)
			instance->_k_angle_over_pixel = k;
	}	
	

	static float k_vel() {
		camera *instance = camera::instance();
		return instance->_k_vel;
	};

	
	static void set_k_vel(double k) {
		camera *instance = camera::instance();
		k = fabs(k);
		if (k > 0.)
			instance->_k_vel = k;
	}	

	
	static int pan2dx(double pan) {
		double dpan;
		camera *instance = camera::instance();

		instance->_ctrl_step_estimate();

		dpan = pan - instance->_ctrl_last_pos.pan;
		return int(dpan/instance->_k_angle_over_pixel);
	};	
	

	// the returned vector ptr must be freed by the caller
	static std::vector<double> *pan2dx(std::vector<double> &pan_angles) {
		double cur_pan;
		double k_angle_over_pixel;
		camera *instance;
		std::vector<double> *dx = new std::vector<double>();
		
		instance = camera::instance();
		cur_pan = instance->_ctrl_last_pos.pan;
		k_angle_over_pixel = instance->_k_angle_over_pixel;
		assert(k_angle_over_pixel > 0.0001);
		for (unsigned i = 0; i < pan_angles.size(); i++) {
			double dpan;
			dpan = pan_angles[i] - cur_pan;
			dx->push_back(dpan/k_angle_over_pixel);
		}
		
		return dx;
	};


	static void set_pos(ptz_t pos) {
		camera *instance = camera::instance();

		instance->_ctrl_step_estimate();
		instance->_ctrl_last_pos = pos;
		pos = instance->_ctrl_last_pos;
	};
	

        static void actual_pan_tilt(double *alfa, double *theta, double *zoom);


	static void move(double pan, double tilt, double zoom);
	static void move(ptz_t pos);


	static double time_to_reach(double target_pan, double target_tilt, double target_zoom);
	static double time_to_reach(ptz_t pos);


	static double time_to_reach_from(ptz_t from, ptz_t to);


	static void ctrl_estimator_measure_pos(void);


	static void save_config(void);

protected:

	camera();
	~camera();

	void _ctrl_step_estimate(void);

        void serial_lock(void);
        void serial_unlock(void);
        
	void load_config(void);

	// spec	
	int _cam_id;
	bool _ptz;
	float _fov;
	
	// status
	bool _patrolling;
	bool _tracking;

	// config: ptz limits
	float _lpan, _rpan;
	float _base_tilt;

	// config: tracking filter
	int _hmin, _hmax;
	int _smin, _smax;
	int _vmin, _vmax;
	int _area_min;

	// config: detection box
	int _ymin, _ymax;
	int _xmin, _xmax;

	// config: state estimator
	double _k_angle_over_pixel;
	double _k_vel;

	// serial
	int _serial_port;
	int _serial_fd;

	// cam ctrl
	int _ctrl;
	struct timeval _ctrl_last_pos_time;
	ptz_t _ctrl_last_pos;
	vel_t _ctrl_ref_vel;
	ptz_t _ctrl_ref_ptz;

	std::vector<double> _bangles;
	pthread_mutex_t _serial_lock;
	grabber *_grabber;

	// unidstort filter
	cv::Mat *_intrinsic;
	cv::Mat *_distcoeff;
};


#endif

