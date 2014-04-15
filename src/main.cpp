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
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "camera.h"


using namespace std;


// the two main threads entry points
void *tracking(void *arg);
void *patrolling(void * arg);


void sig_kill(int param) {
	// - save config
	// - if ptz, try to stop the camera
	// - kill us

	camera::save_config();

	if (camera::is_ptz()) {
		cout << "Stopping camera ...\n";
		camera::velocity(0,0);
		cout << ("Exiting ...\n");
	}

	exit(1);
}


int main(int argc, const char *argv[]) {
	time_t rawtime;
	struct tm * timeinfo;
        struct timeval now;
	pthread_t thread_ids[2];

	// setup a signal handler to exit excution gracefully
	signal(SIGINT, sig_kill);

	// !avoid multiple instantiation
	// camera::instance() should be protected with a mutex ...
	camera::id();

        // Welcome !
	time(&rawtime);
	timeinfo = localtime (&rawtime);
	gettimeofday(&now, NULL);
	cout << "Hi! Now is " << now.tv_sec << "." <<  setfill('0') << setw(6) << now.tv_usec << ", aka "  << asctime(timeinfo);

	// start the tracking and patrolling threads
	if (pthread_create(&thread_ids[0], NULL, tracking,  NULL) != 0) {
		perror("main: cannot spawn tracking thread");
		// wow, that was short !
		exit(1);
	}
	if (pthread_create(&thread_ids[1], NULL, patrolling, NULL) != 0) {
		perror("main: cannot spawn patrolling thread");
		exit(1);
	};

	// start patrolling
	camera::switch_to_patrolling();

	pthread_join(thread_ids[0], NULL);
	pthread_join(thread_ids[1], NULL);

	return 0;
}
