/*
 * Copyright (c) 20xx ??
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
#include <sys/param.h>
#include <math.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include "types.h"
#include "helpers.h"
#include "camera.h"


#define MAX_LEN_STR 40		/* massima lunghezza della stringa per i comandi */
#define __SERIAL_TIME__ 70000 	/* Tempo minimo tra due scritture sulla seriale */
#define __CAMERA_SPEED__ 92
#define __NR_MAX_RETRIES__ 10


using namespace std;

// singletone camera instance
static camera *_instance = NULL;


static void add_checksum(char s[]) {
	int sum = 0;
	int i, checksum;
	char s2[MAX_LEN_STR] = "[";

	/* Calcola il checksum della stringa, aggiunge le parentesi quadre
	 * all'inizio e alla fine della stringa
	 */
	for (i = 0; i < (int) strlen(s); i++)
		sum += s[i];

	sum = sum % 26;
	checksum = sum + 65;
	s[i] = checksum;
	s[i+1] = '\0';
	strcat(s, "]");

	/* Aggiungo la quadra iniziale, s2 contiene inizialmente solo
	 * la quadra chiusa
	 */
	strcat(s2, s);
	strcpy(s, s2);
}


static int check_ulisse(char *answer, int num_uliss) {
	int posT = -1;
	int pos_first_square = -1;
	int k;
	char n[2];

	/* conferma che il numero di ulisse che ho trovato (cam_id) è
	   quello con cui comunico */

	/* trovo le posizioni di alcuni caratteri nella stringa */
	sprintf(n, "%d", num_uliss);
	for (k = 0; k < (int) strlen(answer); k++) {
		if ((answer[k] == '[') && (pos_first_square == -1))
			pos_first_square = k;

		if ((answer[k] == 'T') && (posT == -1) && (pos_first_square != -1))
			posT = k;
	}

	/* Cerco il numero dell'ulisse considerato */
	/* Al momento gestisce solamente ulisse con numero da 1 a 9 */
	if ((answer[posT + 1] == 'M') && (answer[pos_first_square + 1] == 'P')) {
		if (answer[posT + 2] == *n)
			return 1;
		else
			return 0;
	}
	return 0;
}


static int open_serial_port(int port) {
	int fd;
	char numStr[4];
	char stringa_seriale[20] = "/dev/ttyUSB";
	struct termios options;

	sprintf(numStr, "%d", port);
	strcat(stringa_seriale, numStr);
	fd = open(stringa_seriale,  O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd < 0) {
		std::cout << "warning, cannot open \"" << stringa_seriale << "\"!\n";
		return -1;
	}

	// Get the current options for the port...
	tcgetattr(fd, &options);

	// Set the baud rates to 38400...
	cfsetispeed(&options, B38400);
	cfsetospeed(&options, B38400);

	// Enable the receiver and set local mode...
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO);

	// Set the new options for the port...
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);

	return fd;
}


static int what_usb(int cam_id) {
  	int i, ser;
	int find[10];
	for (i = 0; i<10; i++) find[i] = -1;
	int cnt = 0;
	for (i = 0; i<10; i++)
	{
		ser = open_serial_port(i);
		if (ser!=-1)
		{
			find[cnt] = i;
			cnt++;
		}
		close(ser);
	}

	if (cnt == 0)
		std::cout << "error: no ulisse linked\n";

	int tmp = 0;
	int usb_giusta = -1;
	char num_ul_str[6];
	while (find[tmp] != -1)
	{
		ser = open_serial_port(find[tmp]);
		char answer[200] ="";
		char command[200] = "TM";
		sprintf(num_ul_str, "%d", cam_id);
		strcat(command, num_ul_str);
		strcat(command, "PC1EXP?");
		add_checksum(command);
		write(ser, command, strlen(command));
		usleep(__SERIAL_TIME__);
		read(ser, answer, 200);
		if (check_ulisse(answer,cam_id)) {
			usb_giusta = find[tmp];
			find[tmp+1] = -1;
		}
		close(ser);
		tmp++;
	}

	return usb_giusta;
}


camera *camera::instance() {
	if (_instance)
		return _instance;
		
	_instance = new camera();
	return _instance;
}


camera::camera() {
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
    
	this->_cam_id = cam_id;    
	cout << "hostname=" << hostname << ", camid=" << cam_id << endl;

	this->_patrolling = false;
	this->_tracking = false;

	// setup defaults
	this->_base_tilt = 0.;
	this->_area_min = 3000;
	this->_ymin = 0;
	this->_ymax = 300;
	this->_xmin = 0;
	this->_xmax = 720;
	this->_lpan = 60;
	this->_rpan = 120;
	this->_k_angle_over_pixel = 7.2/100.; // per tutte le ptz
	this->_k_vel = 0.1;

	this->_intrinsic = NULL;
	this->_distcoeff = NULL;

	this->_fov = 30;
    	// setup per camera options
	switch (this->_cam_id) {
       	case 0:
    	  	this->_ptz = false;
		this->_hmin = 0;
		this->_hmax = 256;
	        this->_smin = 176;
		this->_smax = 256;
		this->_vmin = 111;
		this->_vmax = 193;
		this->_ymax = 600;
		if (1) {
			this->_bangles.push_back(-22.);
			this->_bangles.push_back(-4.5);
			this->_bangles.push_back(+14.5);
		}
 		break;
       	case 1:
    	  	this->_lpan = -36;
    	  	this->_rpan = 50;

    	  	this->_ptz = true;

		this->_hmin = 141;
		this->_hmax = 256;
	        this->_smin = 131;
		this->_smax = 204;
		this->_vmin = 48;
		this->_vmax = 189;

		this->_xmin = 296;
		this->_xmax = 441;
		this->_ymin = 253;
		this->_ymax = 568;

		this->_intrinsic = new cv::Mat(3,3, CV_32F);
		this->_intrinsic->at<float>(0,0) = 827.7959078769385;
		this->_intrinsic->at<float>(0,1) = 0;
		this->_intrinsic->at<float>(0,2) = 382.3062743711746;
		this->_intrinsic->at<float>(1,0) = 0;
		this->_intrinsic->at<float>(1,1) = 907.6253795817457;		
		this->_intrinsic->at<float>(1,2) = 294.2857741679972;
		this->_intrinsic->at<float>(2,0) = 0;
		this->_intrinsic->at<float>(2,1) = 0;
		this->_intrinsic->at<float>(2,2) = 1;

		this->_distcoeff = new cv::Mat(1,5, CV_32F);
		this->_distcoeff->at<float>(0,0) = -0.1826903297339333;
		this->_distcoeff->at<float>(0,1) = -0.9169841776310198;
		this->_distcoeff->at<float>(0,2) = -0.0003983547124312067;
		this->_distcoeff->at<float>(0,3) = 0.001383279120331801;
		this->_distcoeff->at<float>(0,4) = 5.155916821851194;

 		break;
       	case 2:
    	  	this->_lpan = -72;
    	  	this->_rpan = 36;
    	  	
    	  	this->_ptz = true;

		this->_hmin = 0;
		this->_hmax = 256;
	        this->_smin = 132;
		this->_smax = 256;
		this->_vmin = 29;
		this->_vmax = 256;

		this->_xmin = 280;
		this->_xmax = 437;
		this->_ymin = 255;
		this->_ymax = 534;

		this->_intrinsic = new cv::Mat(3,3, CV_32F);
		this->_intrinsic->at<float>(0,0) = 775.8468734878796;
		this->_intrinsic->at<float>(0,1) = 0;
		this->_intrinsic->at<float>(0,2) = 390.6988340600847;
		this->_intrinsic->at<float>(1,0) = 0;
		this->_intrinsic->at<float>(1,1) = 851.0116418835253;		
		this->_intrinsic->at<float>(1,2) = 268.7829019504324;
		this->_intrinsic->at<float>(2,0) = 0;
		this->_intrinsic->at<float>(2,1) = 0;
		this->_intrinsic->at<float>(2,2) = 1;

		this->_distcoeff = new cv::Mat(1,5, CV_32F);
		this->_distcoeff->at<float>(0,0) = -0.2488196362478963;
		this->_distcoeff->at<float>(0,1) = 0.2202732516656443;
		this->_distcoeff->at<float>(0,2) = -0.008203912830744352;
		this->_distcoeff->at<float>(0,3) = -0.005608598356846471;
		this->_distcoeff->at<float>(0,4) = 0.05651586291455744;

		// difficili da rivelare
		//this->_bangles.push_back(-12.47); 
		this->_bangles.push_back(-20); 
 		break;
    	case 3:
    	  	this->_lpan = -75;
    	  	this->_rpan = 52;
    	  	
    	  	this->_ptz = true;

		this->_hmin = 148;
		this->_hmax = 256;
	        this->_smin = 130;
		this->_smax = 256;
		this->_vmin = 25;
		this->_vmax = 79;
		this->_ymax = 600;

		this->_xmin = 275;
		this->_xmax = 444;
		this->_ymin = 245;
		this->_ymax = 568;

		this->_k_angle_over_pixel = 7.2/100.;

		this->_bangles.push_back(-28.91);

		this->_intrinsic = new cv::Mat(3,3, CV_32F);
		this->_intrinsic->at<float>(0,0) = 822.6185132910256;
		this->_intrinsic->at<float>(0,1) = 0;
		this->_intrinsic->at<float>(0,2) = 331.1484651846446;
		this->_intrinsic->at<float>(1,0) = 0;
		this->_intrinsic->at<float>(1,1) = 885.7557491804558;		
		this->_intrinsic->at<float>(1,2) = 311.9952305702794;
		this->_intrinsic->at<float>(2,0) = 0;
		this->_intrinsic->at<float>(2,1) = 0;
		this->_intrinsic->at<float>(2,2) = 1;

		this->_distcoeff = new cv::Mat(1,5, CV_32F);
		this->_distcoeff->at<float>(0,0) = -0.2746485174687344;
		this->_distcoeff->at<float>(0,1) = 0.4076778420608688;
		this->_distcoeff->at<float>(0,2) = 0.00380916502139352;
		this->_distcoeff->at<float>(0,3) = -0.004269559257388246;
		this->_distcoeff->at<float>(0,4) = -0.1896001840051721;
    		break;
       	case 4:
    	  	this->_lpan = 23;
    	  	this->_rpan = 75;
    	  	
    	  	this->_ptz = true;

		this->_hmin = 0;
		this->_hmax = 256;
	        this->_smin = 97;
		this->_smax = 256;
		this->_vmin = 212;
		this->_vmax = 256;
		this->_base_tilt = -10;

		this->_xmin = 298;
		this->_xmax = 442;
		this->_ymin = 48;
		this->_ymax = 264;

		this->_k_angle_over_pixel = 7.2/100.;

		this->_bangles.push_back(62.5);

		this->_intrinsic = new cv::Mat(3,3, CV_32F);
		this->_intrinsic->at<float>(0,0) = 797.0694372036658;
		this->_intrinsic->at<float>(0,1) = 0;
		this->_intrinsic->at<float>(0,2) = 342.8282857045096;
		this->_intrinsic->at<float>(1,0) = 0;
		this->_intrinsic->at<float>(1,1) = 852.6154703631233;		
		this->_intrinsic->at<float>(1,2) = 307.8974867462443;
		this->_intrinsic->at<float>(2,0) = 0;
		this->_intrinsic->at<float>(2,1) = 0;
		this->_intrinsic->at<float>(2,2) = 1;

		this->_distcoeff = new cv::Mat(1,5, CV_32F);
		this->_distcoeff->at<float>(0,0) = -0.4606575120361817;
		this->_distcoeff->at<float>(0,1) = 0.8726487040395485;
		this->_distcoeff->at<float>(0,2) = -0.001048658539378076;
		this->_distcoeff->at<float>(0,3) = 0.0001268885677444621;
		this->_distcoeff->at<float>(0,4) = -1.755941144016009;
 		break;
       	case 5:
		this->_hmin = 0;
		this->_hmax = 14;
	        this->_smin = 162;
		this->_smax = 256;
		this->_vmin = 81;
		this->_vmax = 214;
		this->_base_tilt = 0;
    	  	this->_lpan = 0;
    	  	this->_rpan = 0;
    	  	this->_ptz = false;
		this->_area_min = 216;

		this->_xmin = 164;
		this->_xmax = 620;
		this->_ymin = 129;
		this->_ymax = 255;

		this->_k_angle_over_pixel = 8.8/100.;

		this->_intrinsic = new cv::Mat(3,3, CV_32F);
		this->_intrinsic->at<float>(0,0) = 545.612;
		this->_intrinsic->at<float>(0,1) = 0;
		this->_intrinsic->at<float>(0,2) = 304.7398;
		this->_intrinsic->at<float>(1,0) = 0;
		this->_intrinsic->at<float>(1,1) = 544.430;		
		this->_intrinsic->at<float>(1,2) = 229.717;
		this->_intrinsic->at<float>(2,0) = 0;
		this->_intrinsic->at<float>(2,1) = 0;
		this->_intrinsic->at<float>(2,2) = 1;

		this->_distcoeff = new cv::Mat(1,5, CV_32F);
		this->_distcoeff->at<float>(0,0) = 0.058;
		this->_distcoeff->at<float>(0,1) = -0.099;
		this->_distcoeff->at<float>(0,2) = -0.003;
		this->_distcoeff->at<float>(0,3) = -0.005;
		this->_distcoeff->at<float>(0,4) = -0.02009;
		break;
       	case 6:
		this->_xmin = 168;
		this->_xmax = 563;
		this->_ymin = 56;
		this->_ymax = 285;

		this->_k_angle_over_pixel = 11.2/100.;
		
		// difficili da rivelare
		//this->_bangles.push_back(+4.57);

		this->_hmin = 124;
		this->_hmax = 256;
	        this->_smin = 58;
		this->_smax = 256;
		this->_vmin = 79;
		this->_vmax = 256;
		this->_base_tilt = 0;
    	  	this->_lpan = 0;
    	  	this->_rpan = 0;
    	  	this->_ptz = false;

		this->_intrinsic = new cv::Mat(3,3, CV_32F);
		this->_intrinsic->at<float>(0,0) =531.2789775901376 ;
		this->_intrinsic->at<float>(0,1) = 0;
		this->_intrinsic->at<float>(0,2) = 317.2443886798362;
		this->_intrinsic->at<float>(1,0) = 0;
		this->_intrinsic->at<float>(1,1) = 530.3613816869509;		
		this->_intrinsic->at<float>(1,2) = 215.746101908899;
		this->_intrinsic->at<float>(2,0) = 0;
		this->_intrinsic->at<float>(2,1) = 0;
		this->_intrinsic->at<float>(2,2) = 1;

		this->_distcoeff = new cv::Mat(1,5, CV_32F);
		this->_distcoeff->at<float>(0,0) = 0.05864142884587226;
		this->_distcoeff->at<float>(0,1) = -0.1828565128541732;
		this->_distcoeff->at<float>(0,2) = -0.00482827830070716;
		this->_distcoeff->at<float>(0,3) = 0.001085729258876486;
		this->_distcoeff->at<float>(0,4) = 0.08728331313744789;

		break;
       	default:
    	  	this->_lpan = 0;
    	  	this->_rpan = 0;
    	  	this->_ptz = false;
 		break;
	}

	// find out the serial port and open it
	pthread_mutex_init(&this->_serial_lock, NULL);
	this->_serial_port = -1;
	this->_serial_fd = -1;
	if (this->_ptz) {
		this->_serial_port = what_usb(this->_cam_id);
		this->_serial_fd = open_serial_port(this->_serial_port);
	}

	// init the camera 'state estimator'
	this->_ctrl = CAMERA_CTRL_VEL;
	this->_ctrl_ref_vel.vpan = 0.;
	this->_ctrl_ref_vel.vtilt = 0.;
	this->_ctrl_last_pos.pan = 0.;
	this->_ctrl_last_pos.tilt = 0.;
	this->_ctrl_last_pos.zoom = 0.;
	gettimeofday(&this->_ctrl_last_pos_time, NULL);
   
	// init the grabber
	if (this->_ptz)
		this->_grabber = new ulisse_grabber(this->_serial_port);
	else
		this->_grabber = new webcam_grabber(0);
   
	cout << "lpan       : " << this->_lpan << "\n";
	cout << "rpan       : " << this->_rpan << "\n";
	cout << "ptz        : " << this->_ptz  << "\n";
	cout << "hmin       : " << this->_hmin << "\n";
	cout << "hmax       : " << this->_hmax << "\n";
	cout << "smin       : " << this->_smin << "\n";
	cout << "smax       : " << this->_smax << "\n";
	cout << "vmin       : " << this->_vmin << "\n";
	cout << "vmax       : " << this->_vmax << "\n";
	cout << "xmin       : " << this->_xmin << "\n";
	cout << "xmax       : " << this->_xmax << "\n";
	cout << "ymin       : " << this->_ymin << "\n";
	cout << "ymax       : " << this->_ymax << "\n";
	cout << "base-tilt  : " << this->_base_tilt << "\n";
	cout << "serial port: " << this->_serial_port << "\n";
	cout << "serial fd  : " << this->_serial_fd << "\n";

	cout << "bangles    : (";
	for (unsigned int i=0; i < this->_bangles.size(); i++) {
		cout << this->_bangles[i] << ", ";
	}
	cout << ")\n";
   
	this->_grabber->dump();

	if (this->_intrinsic) {
		std::cout << "Mat intrinsic:\n";
		std::cout << *this->_intrinsic << "\n";
	}
	if (this->_distcoeff) {
		std::cout << "Mat distcoeff:\n";
		std::cout << *this->_distcoeff << "\n";
	}

	// load saved parameters
	this->load_config();
}


static int ulisse_write_pos(int ul_number, int fd) {
	int ret;
	char command[MAX_LEN_STR] = "TM";
	char numUl[4];

	sprintf(numUl, "%d", ul_number);
	strcat(command, numUl);
	strcat(command, "PC1EXP?");
	add_checksum(command);

	ret = write(fd, command, strlen(command));
	if (ret < 0) {
		std::cout << "warning, ulisse_write_pos(), write failed\n";
		return -1;
	}
	return 0;
}


static double ulisse_zoom_s2x(double val, int module) {
	if (module == 10) {
		/* caso modulo 10x */
		double valz = (val*val*val)*3.7218*pow(10,-12) + (val*val)*(-4.7020)*pow(10,-8) + val*3.0992*pow(10,-4) + 0.99064;

		return  valz;
	} else if (module == 36) {
		long double valz = pow(val,7) * pow(10,-25) *( 1.13976659179002 )+
			pow(val,6) * pow(10,-21) *( -7.71094220949187) +
			pow(val,5) * pow(10,-16) *(  2.13354979609449) +
			pow(val,4) * pow(10,-12) *( -3.08232277405313) +
			pow(val,3) * pow(10,-8) *( 2.44668148972473 ) +
			pow(val,2) * pow(10,-4) *( -1.00884865838561) +
			pow(val,1) * pow(10,-1) *( 1.68427100689602 ) +
			pow(10,-1) *( 9.99829457833220);
		return valz;
	}
	
	std::cout << "warning, ulisse_zoom_s2x: wrong module value\n";
	return -1;
}


static double ulisse_zoom_x2s(double val, int module) {
	if (module == 10) {
		/* caso modulo 10x */
		double valz = (val*val*val*val*val)*2.1419 - 69.776*(val*val*val*val)+879.62*(val*val*val) -
			5436.5*(val*val) + 17705*val - 13049;
		return valz;
	} else if (module == 36) {
		double valz =
			(pow(val,14) * pow(10,-13)) *( -4.72019456884080) +
			(pow(val,13) * pow(10,-10)) *( 1.28968776132135) +
			(pow(val,12) * pow(10,-8)) *( -1.59170454355467) +
			(pow(val,11) * pow(10,-6)) *( 1.17328913705008) +
			(pow(val,10) * pow(10,-5)) *( -5.75530016345087) +
			(pow(val,9) * pow(10,-3)) *( 1.98143404143399 )+
			(pow(val,8) * pow(10,-2)) *(  -4.92140137126225) +
			(pow(val,7) * pow(10,-1)) *( 8.92758769219052)+
			( pow(val,6) * pow(10,1)) *(  -1.18449997231627 )+
			(pow(val,5) * pow(10,2)) *(   1.14051848957031 )+
			(pow(val,4) * pow(10,2)) *( -7.83549391651709 )+
			(pow(val,3) * pow(10,3)) *( 3.73766357784957)+
			(pow(val,2) * pow(10,4)) *( -1.19020870947909 )+
			(pow(val,1) * pow(10,4)) *( 2.41371036984136 )+
			pow(10,4)*( -1.52907322061481);

		return valz;
	}
	
	std::cout << "warning, ulisse_zoom_x2s: wrong module value\n";
	return -1;
}


static int __move(int cam_id, ptz_t pos, int ser) {
	int pan = 0, tilt = 0, zoom = 0;
	char pan_value[8], tilt_value[8], zoom_value[8], vel_value[8];
	char comando[MAX_LEN_STR] = "TM";
	char numUl[4];
	int res = -1;

	pan = pos.pan*100;
	tilt = pos.tilt*100;
	if (pan < 0)
		pan += 65536;

	if (tilt < 0)
		tilt += 65536;

	/* Converto il valore dello zoom da x a seriale */
	{
		int module = 10; /* caso ulisse 2*/
		if (cam_id == 4) {
			module = 36;
		}

		zoom = ulisse_zoom_x2s(pos.zoom*100, module);
	}

	sprintf(vel_value, "%d", __CAMERA_SPEED__); /*max velocità assoluta*/
	sprintf(pan_value, "%d", pan);
	sprintf(tilt_value,"%d", tilt);
	sprintf(zoom_value,"%d", zoom);
	sprintf(numUl, "%d", cam_id);
	strcat(comando, numUl);
	strcat(comando, "PC1XMtA");
	strcat(comando, pan_value);
	strcat(comando, ",");
	strcat(comando, tilt_value);
	strcat(comando, ",");
	strcat(comando, vel_value);
	strcat(comando, ",");
	strcat(comando, zoom_value);

	add_checksum(comando);

	res = write(ser, comando, strlen(comando));
	if (res < 0) {
		std::cout << "warning, __move(), write failed\n";
		return -1;
	}
	return 0;
}


void camera::move(ptz_t pos) {
	int ret;
	camera *camera;
	
	if ((fabs(pos.pan) > 160)
	    || (fabs(pos.tilt) > 89)
	    || (pos.zoom < 1) || (pos.zoom > 10)) {
		std::cout << "warning, camera::move(ptz), position out of limits!\n";
		return;
	}

	camera = camera::instance();
	camera->serial_lock();
	ret = __move(camera->_cam_id, pos, camera->_serial_fd);
	camera->serial_unlock();

	if (!ret) {
		// the new target position was set successfully:
		// track this change in the state estimator
		camera->_ctrl_step_estimate();
		
		// update state estimator
		camera->_ctrl = CAMERA_CTRL_POS;
		camera->_ctrl_ref_ptz = pos;
	}
}


void camera::move(double p, double t, double z) {
	ptz_t pos;

	pos.pan = p;
	pos.tilt = t;
	pos.zoom = z;

	camera::move(pos);
}


static int __velocity(int cam_id, vel_t vel, int ser) {
	int ret;
	int velpan = 0, veltilt = 0, veltemp=0;
	char vpan_value[4], vtilt_value[4], vtemp_value[8];
	char comserial[9] = "";
	char comando[MAX_LEN_STR] = "TM";
	char numUl[4];

	velpan = (int) vel.vpan*10.;
	veltilt = (int) vel.vtilt*10.;
	sprintf(numUl, "%d", cam_id);

	if ((velpan == 0) && (veltilt == 0)) {
		// both velocities are zero: stop
		strcat(comando, numUl);
		strcat(comando, "PC1EXSt");

	} else if (((velpan!=0) && (veltilt==0)) || ((velpan==0) && (veltilt!=0))) {
		// either vpan or vtilt is zero
	
		if (velpan > 0)	{
			// right
			strcat(comserial, "PC1EXRg" );
			veltemp = velpan;

		} else if (velpan <0) {
			// left
			strcat(comserial, "PC1EXLf" );
			veltemp = velpan;

		} else if (veltilt > 0)	{
			// up
			strcat(comserial, "PC1EXUp" );
			veltemp = veltilt;
			
		} else if (veltilt<0) {
			// down
			strcat(comserial, "PC1EXDn" );
			veltemp = veltilt;

		}

		sprintf(vtemp_value, "%d", abs(veltemp));
		strcat(comando, numUl);
		strcat(comando, comserial);
		strcat(comando, vtemp_value);

	} else {
		// both velocities are non-zero

		if ((velpan > 0) && (veltilt>0)) {
			// up-right
			strcat(comserial, "PC1EXUR" );

		} else if ((velpan > 0)&&(veltilt<0)) {
			// down-right
			strcat(comserial, "PC1EXDR" );

		} else if ((velpan < 0)&&(veltilt>0)) {
			// up-left
			strcat(comserial, "PC1EXUL" );
			
		} else if ((velpan < 0)&&(veltilt<0)) {
			// down-left
			strcat(comserial, "PC1EXDL" );

		}
		sprintf(vpan_value, "%d", abs(velpan));
		sprintf(vtilt_value,"%d", abs(veltilt));
		strcat(comando, numUl);
		strcat(comando, comserial);
		strcat(comando, vpan_value);
		strcat(comando, ",");
		strcat(comando, vtilt_value);
	}

	add_checksum(comando);

	for (int i=0; i < __NR_MAX_RETRIES__; i++) {

		ret = write(ser, comando, strlen(comando));
		if (!ret)
			break;

		std::cout << "warning, __velocity(), retry nr. " << i << " failed\n";
	}

	if (ret < 0)
		std::cout << "warning, __velocity: failed\n";

	return 0;
}


void camera::velocity(vel_t vel) {
	int ret;
	camera *camera;
	
	if ((fabs(vel.vpan) > 200) || (fabs(vel.vtilt) > 200)) {
		std::cout << "warning, camera::velocity(), values are out of limits!\n";
		return;
	}

	camera = camera::instance();
	camera->serial_lock();
	ret = __velocity(camera->_cam_id, vel, camera->_serial_fd);
	camera->serial_unlock();
	
	if (!ret) {
		// the new velocity was set: track this change in our estimator
		camera->_ctrl_step_estimate();
		
		// update state estimator
		camera->_ctrl = CAMERA_CTRL_VEL;
		camera->_ctrl_ref_vel = vel;
	}
}


void camera::velocity(double vpan, double vtilt) {
	vel_t vel;
	
	vel.vpan = vpan;
	vel.vtilt = vtilt;
	camera::velocity(vel);
}


static int ulisse_get_PTZ(int tm_number, int ser, ptz_t *pos) {
	char pan[8], tilt[8], zoom[8];
	double pan_value, tilt_value, zoom_value;
	int res, i;
	char comma, last_square, X;
	int posX = -1;
	int pos_first_comma = -1, pos_second_comma = -1, pos_last_square = -1;
	int MAX_LEN_ANSW = 60;
	char answer[MAX_LEN_ANSW + 1];

	ulisse_write_pos(tm_number,ser);

	usleep(__SERIAL_TIME__);

	res = read(ser, answer, MAX_LEN_ANSW);
	if (res < 0) {
		std::cout << "warning, ulisse_get_PTZ(), read failed\n";
		return -1;
	}

	comma = ',';
	X = 'X';
	last_square = ']';

	/* scorro la stringa letta sulla seriale per trovare la posizione dei
	 * caratteri che mi interessano
	 */
	for (i = 0; i < (int) strlen(answer); i++) {
		if ((answer[i] == X) && (posX == -1))
			posX = i;

		if ((answer[i] == comma) && (pos_first_comma == -1))
			pos_first_comma = i;

		if ((answer[i] == comma) && (pos_first_comma != i) && (pos_second_comma == -1))
			pos_second_comma = i;

		if ((pos_second_comma != -1) && (answer[i] == last_square) && (pos_last_square == -1))
			pos_last_square = i;
	}

	/* trovo il pan */
	int k = 0;
	for (i = posX+3; i < pos_first_comma; i++)
		pan[k++] = answer[i];

	pan[k]='\0';

	/* trovo il tilt */
	k = 0;
	for (i = pos_first_comma+1; i < pos_second_comma; i++)
		tilt[k++] = answer[i];

	tilt[k]='\0';

	/* trovo lo zoom */
	k = 0;
	for (i = pos_second_comma+1; i<pos_last_square-1; i++)
		zoom[k++] = answer[i];

	zoom[k] = '\0';

	/* converto la stringa in interi */
	pan_value = (double)atoi(pan);
	tilt_value = (double)atoi(tilt);
	zoom_value = (double)atoi(zoom);

	/* converto il numero in gradi */
	if (pan_value < 32768)
		pan_value = pan_value/100;
	else
		pan_value = (-65536 + pan_value)/100;

	if (tilt_value < 32768)
		tilt_value = tilt_value/100;
	else
		tilt_value = (-65536 + tilt_value)/100;

	/* conversione dello zoom in ingrandimenti */
	int module = 10; /* caso ulisse 2*/
	if (tm_number == 4)
		module = 36;

	zoom_value = ulisse_zoom_s2x(zoom_value, module);
	/* aggiorno lo stato letto per la camera considerata */
	pos->pan = (float)pan_value;
	pos->tilt = (float)tilt_value;
	pos->zoom = (float)zoom_value;
	
	{
		camera *instance;

		// refresh our estimate using the exact position we just retrieved
		instance = camera::instance();
		instance->set_pos(*pos);
	}

	return 0;
}


static ptz_t old_pos;

//pos old is used to avoid problem when read fail to work
static ptz_t ritorna_ptzNIK(int cam_id, int fd) {
	int ret;
	ptz_t pos;

	ret = ulisse_get_PTZ(cam_id, fd, &pos);

	if (ret != 0) {
		// read fail
		pos = old_pos;
		std::cout << "  warning, ritorna_ptzNIK(), using old position\n";
	} else {
		//refresh position
		old_pos = pos;
	}

	return pos;
}


double camera::time_to_reach_from(ptz_t from, ptz_t to) {
	int dpan, dtilt;
	double dmax, kvel, time;

	assert (__CAMERA_SPEED__ > 0.);

	dpan = fabs(fmod(from.pan, 180) - fmod(to.pan, 180));
	dtilt = fabs(fmod(from.tilt, 90) - fmod(to.tilt, 90));
	dmax = std::max(dpan, dtilt);

	kvel = 9. / double(__CAMERA_SPEED__);
	time = dmax*kvel;
	
	return time;
}


//used to store the old time if the time_to reach doesnot work
static int old_time = 5;

double camera::time_to_reach(ptz_t target_pos) {
	double time;
	ptz_t cur_pos;
	camera *camera;

	camera = camera::instance();

	if (!ulisse_get_PTZ(camera->_cam_id, camera->_serial_fd, &cur_pos)) {
		time = camera::time_to_reach_from(cur_pos, target_pos);
		old_time = time;
	} else {
		std::cout << "warning, camera::time_to_reach(), returning old time\n";
		time = old_time;
	}

	return time;
}


double camera::time_to_reach(double target_pan, double target_tilt, double target_zoom) {
	ptz_t target_pos;

	target_pos.pan = target_pan;
	target_pos.tilt = target_tilt;
	target_pos.zoom = target_zoom;
	return camera::time_to_reach(target_pos);
}


void camera::actual_pan_tilt(double *alfa, double *theta, double *zoom) {
	ptz_t angle;
	camera *camera;
	
	camera = camera::instance();

	camera->serial_lock();
	angle = ritorna_ptzNIK(camera->_cam_id, camera->_serial_fd);
	camera->serial_unlock();

	*alfa = angle.pan;
	*theta = angle.tilt;
	*zoom = angle.zoom;
}

void camera::ctrl_estimator_measure_pos(void) {
	int ret;
	ptz_t pos;
	camera *camera;
	
	camera = camera::instance();

	camera->serial_lock();
	for (int i=0; i < __NR_MAX_RETRIES__; i++) {

		ret = ulisse_get_PTZ(camera->_cam_id, camera->_serial_fd, &pos);
		if (!ret)
			break;

		std::cout << "warning, ctrl_estimator_measure_pos() retry nr. " << i << " failed\n";
	}
	camera->serial_unlock();

	if (!ret) {
		// refresh our estimate using the exact position we just retrieved
		camera->set_pos(pos);	
	}
}


void camera::serial_lock(void) {
	int ret;
	camera *camera;
	
	camera = camera::instance();	
	ret = pthread_mutex_lock(&camera->_serial_lock);
	if (ret) {
		std::cout << "error, camera::serial_lock(), failed to lock mutex\n";
		exit(1);
	}
}


void camera::serial_unlock(void) {
	int ret;
	camera *camera;
	
	camera = camera::instance();
	ret = pthread_mutex_unlock(&camera->_serial_lock);
	if (ret) {
		std::cout << "warning, camera::serial_unlock(), failed to unlock mutex\n";
		exit(1);
	}
}


static std::string getcwd_str(void) {
	char temp[MAXPATHLEN];
   
	return (getcwd(temp, MAXPATHLEN) ? std::string( temp ) : std::string(""));
}


static std::string config_path(int camid) {
	std::string cwd = getcwd_str();
	
	return cwd + "/config-" + int2str(camid) + ".cfg";
}


void camera::load_config(void) {
	std::string filepath;
	std::ifstream file;
	
	filepath = config_path(this->_cam_id);
	
	std::cout << "Loading camera configuration from \"" << filepath << "\"\n";

	try {
		file.open(filepath.c_str(), ios_base::in);

		while (file.good()) {
		
			try {
				size_t pos;
				double value;
				std::string line;
				std::string ctrl_name;

				std::getline(file, line);

				pos = line.find("=");
				if (pos == std::string::npos)
					continue;
		
				try {
					std::istringstream buffer(line.substr(pos+1));
					buffer >> value;
				} catch (std::ifstream::failure e) {
					std::cout << "warning, camera::load_config(), exception converting line \"" << line <<"\"\n";
					continue;
				}
		
				ctrl_name = line.substr(0,pos);

				if (ctrl_name == "lpan") {
					this->_lpan = value;
			
				} else if (ctrl_name == "rpan") {
					this->_rpan = value;
			
				} else if (ctrl_name == "base_tilt") {
					this->_base_tilt = value;
			
				} else if (ctrl_name == "hmin") {
					this->_hmin = (int)value;
			
				} else if (ctrl_name == "hmax") {
					this->_hmax = (int)value;
			
				} else if (ctrl_name == "smin") {
					this->_smin = (int)value;
			
				} else if (ctrl_name == "smax") {
					this->_smax = (int)value;
			
				} else if (ctrl_name == "vmin") {
					this->_vmin = (int)value;
			
				} else if (ctrl_name == "vmax") {
					this->_vmax = (int)value;
			
				} else if (ctrl_name == "area_min") {
					this->_area_min = (int)value;
			
				} else if (ctrl_name == "ymin") {
					this->_ymin = (int)value;
			
				} else if (ctrl_name == "ymax") {
					this->_ymax = (int)value;
			
				} else if (ctrl_name == "xmin") {
					this->_xmin = (int)value;
			
				} else if (ctrl_name == "xmax") {
					this->_xmax = (int)value;
			
				} else if (ctrl_name == "k_angle_over_pixel") {
					this->_k_angle_over_pixel = value;
			
				} else if (ctrl_name == "k_vel") {
					this->_k_vel = value;
			
				} else {
					std::cout << "warning, camera::load_config(), cannot decode ctrl with name \"" << ctrl_name << "\"\n";
				}
			} catch (std::ifstream::failure e) {
				break;
			}
		}
		file.close();

	} catch (std::ifstream::failure e) {
		std::cout << "warning, camera::load_config(), exception loading config file, skipping ...\n";
		file.close();
	}	
}


void camera::save_config(void) {
	camera *instance;
	std::string filepath;
	std::ofstream file;
	
	instance = camera::instance();
	filepath = config_path(instance->_cam_id);
	
	std::cout << "Saving camera configuration to \"" << filepath << "\"\n";
	
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try {
		std::string line;

		file.open(filepath.c_str(), ios_base::out|ios_base::trunc);

		file << "lpan=" << instance->_lpan << "\n";
		file << "rpan=" << instance->_rpan << "\n";
		file << "base_tilt=" << instance->_base_tilt << "\n";
		file << "hmin=" << instance->_hmin << "\n";
		file << "hmax=" << instance->_hmax << "\n";
		file << "smin=" << instance->_smin << "\n";
		file << "smax=" << instance->_smax << "\n";
		file << "vmin=" << instance->_vmin << "\n";
		file << "vmax=" << instance->_vmax << "\n";
		file << "area_min=" << instance->_area_min << "\n";
		file << "ymin=" << instance->_ymin << "\n";
		file << "ymax=" << instance->_ymax << "\n";
		file << "xmin=" << instance->_xmin << "\n";
		file << "xmax=" << instance->_xmax << "\n";
		file << "k_angle_over_pixel=" << instance->_k_angle_over_pixel << "\n";
		file << "k_vel=" << instance->_k_vel << "\n";

		file.close();

	} catch (std::ifstream::failure e) {
		std::cout << "warning, camera::save_config(), exception writing config file, skipping ...\n";
		file.clear();
		file.close();
	}
}


void camera::_ctrl_step_estimate(void) {
        double dt;
	struct timeval now;
		
	gettimeofday(&now, NULL);
	dt = timerdiff(now, this->_ctrl_last_pos_time);
	assert (dt > 0.);

	if (dt < 0.001)
		return;

	this->_ctrl_last_pos_time = now;
	if (this->_ctrl == CAMERA_CTRL_VEL) {
	        double k_vpan, k_vtilt;

		k_vpan = this->_k_vel*__CAMERA_SPEED__;
		k_vtilt = this->_k_vel*__CAMERA_SPEED__;

		// update state estimator
		this->_ctrl_last_pos.pan  += this->_ctrl_ref_vel.vpan * k_vpan * dt;
		this->_ctrl_last_pos.tilt += this->_ctrl_ref_vel.vtilt * k_vtilt * dt;

	} else if (this->_ctrl == CAMERA_CTRL_POS) {
		double ttr = this->time_to_reach_from(this->_ctrl_last_pos, this->_ctrl_ref_ptz);

		if (dt >= ttr) {
			// track the fact that the camera has stopped
			this->_ctrl = CAMERA_CTRL_VEL;
			this->_ctrl_ref_vel.vpan = 0.;
			this->_ctrl_ref_vel.vtilt = 0.;

			this->_ctrl_last_pos.pan  = this->_ctrl_ref_ptz.pan;
			this->_ctrl_last_pos.tilt = this->_ctrl_ref_ptz.tilt;
		} else {
			assert(ttr > 0);

			if (ttr > 0.001) {
				double kt;
				
				kt = dt/ttr;
				 
				this->_ctrl_last_pos.pan  += (this->_ctrl_ref_ptz.pan - this->_ctrl_last_pos.pan)*kt;
				this->_ctrl_last_pos.tilt += (this->_ctrl_ref_ptz.tilt - this->_ctrl_last_pos.tilt)*kt;

			} else {
				this->_ctrl_last_pos.pan  = this->_ctrl_ref_ptz.pan;
				this->_ctrl_last_pos.tilt = this->_ctrl_ref_ptz.tilt;

				// the camera has almost stopped (<= 1ms)
				this->_ctrl = CAMERA_CTRL_VEL;
				this->_ctrl_ref_vel.vpan = 0.;
				this->_ctrl_ref_vel.vtilt = 0.;
			}
		}
	}
}

