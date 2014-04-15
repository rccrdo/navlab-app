#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <assert.h>
#include <vector>
#include <iostream>
#include <cv.h>
#include <highgui.h>
#include "grabber.h"


using namespace cv;


static int get_cam_id(void) {
	int cam_id;
	int hostname10;
	char hostname[MAXHOSTNAMELEN];

	gethostname(hostname, MAXHOSTNAMELEN);
	hostname10 = atoi(&hostname[10]);

	cam_id = 0;
	if(hostname10 == 4) {
		cam_id = 3;
	} else if(hostname10 == 2) {
		cam_id = 1;
	} else if(hostname10 == 3) {
		cam_id = 2;
	} else if(hostname10 == 1 ) {
		cam_id = 4;
	} else if(hostname10 == 5) {
		cam_id = 5;
	} else if(hostname10 == 6 ) {
		cam_id = 6;
	}
    
	std::cout << "hostname=" << hostname << ", camid=" << cam_id << "\n";
	return cam_id;
}


int main(int argc, const char *argv[]) {
	int camid;
	int nr_boards = 0;
	int nr_corners_hor;
	int nr_corners_ver;
	grabber *grabber;
	
	camid = get_cam_id();

	// internal corners in the checkered board
	nr_corners_hor = 7;
	nr_corners_ver = 4; 
	
	// nr of boards to acquire
    	nr_boards = 20;

	// open grabber
	if (camid==0 || camid==5 || camid==6)  {
		grabber = new webcam_grabber(0);
	} else {
		// ... on /dev/video0
		grabber = new ulisse_grabber(0);
	}

	grabber->dump();
    
	int numSquares = nr_corners_hor * nr_corners_ver;
	Size board_sz = Size(nr_corners_hor, nr_corners_ver);
    
	int successes = 0;
	cv::Size image_size;
	vector<Point3f> obj;
	vector<vector<Point3f> > object_points;
	vector<vector<Point2f> > image_points;
   	vector<Point2f> corners;
	for(int j=0; j < numSquares; j++)
		obj.push_back(Point3f(j/nr_corners_hor, j%nr_corners_hor, 0.0f));
    
	while (successes < nr_boards) {
		bool found;
		Mat *vstream_ptr;
		Mat gray_image;
	
		vstream_ptr = grabber->capture();
		cv::Mat image(*vstream_ptr);
		delete vstream_ptr;
		
		image_size = image.size();		
	
		cvtColor(image, gray_image, CV_BGR2GRAY);
	
		found = findChessboardCorners(image, board_sz, corners, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS);
 
		if(found) {
		 	cornerSubPix(gray_image, corners, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 30, 0.1));

		 	drawChessboardCorners(gray_image, board_sz, corners, found);
		}
    
		imshow("win1", image);
		imshow("win2", gray_image);
 
 		// hadle user input
		int key = waitKey(10);
		if (key==537919515 || key ==1048603 || key==27) {
			// esc
			return 0;
		}
 
		if (found && (key==537919520 || key==1048608 || key==32)) {
			//spacebar
			image_points.push_back(corners);
			object_points.push_back(obj);
			std::cout << "Snapshot " << successes + 1 << " acquired !\n";
 
			successes++;
 
			if (successes >= nr_boards)
				break;
		}
	}
    

	// calibrate the camera
        Mat intrinsic = Mat(3, 3, CV_32FC1);
	Mat distCoeffs;
	vector<Mat> rvecs;
	vector<Mat> tvecs;
	intrinsic.ptr<float>(0)[0] = 1;
	intrinsic.ptr<float>(1)[1] = 1;
	calibrateCamera(object_points, image_points, image_size, intrinsic, distCoeffs, rvecs, tvecs);

	// dump estimated coefficients 
 	std::cout << "Mat intrinsic:\n";
 	std::cout << intrinsic << "\n";
 	std::cout << "Mat distCoeffs:\n";
 	std::cout << distCoeffs << "\n";
 
 	// show uncalibrated and calibrated images side-to-side
	Mat imageUndistorted;
	while(1) {
		Mat *vstream_ptr;

		vstream_ptr = grabber->capture();
		cv::Mat image(*vstream_ptr);
		delete vstream_ptr;

		undistort(image, imageUndistorted, intrinsic, distCoeffs);
 
		imshow("win1", image);
		imshow("win2", imageUndistorted);
 
 		// hadle user input
		int key = waitKey(10);
		if (key==537919515 || key ==1048603 || key==27) {
			// esc
			return 0;
		}
	}
    
	return 0;
}
    
    
    
    
    
    
