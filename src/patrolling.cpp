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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#include "camera.h"


static void _sweeper_wain_until_pos(ptz_t pos) {
	const double wait_slice = 0.33;

	while (true) {
		double wait, ttr;

		ttr = camera::time_to_reach(pos);
		wait = std::min(wait_slice, ttr);
		if (wait < 0.01)
			break;

		usleep(wait*1000000.);
	}
}


static void *sweeper(void *arg) {
	ptz_t pos;
		
	assert (arg == NULL);

	pos.tilt = camera::base_tilt();
	pos.zoom = 1.;

	while(true) {
		/* move to the right */
		pos.pan = camera::rpan();
		camera::move(pos);
		_sweeper_wain_until_pos(pos);
		
		/* move to the left */
		pos.pan = camera::lpan();
		camera::move(pos);
		_sweeper_wain_until_pos(pos);
	}

	return NULL;
}


void *patrolling(void *arg) {
	pthread_t sweeper_id;
	bool sweeper_running;

	assert(arg == NULL);

	// nothing to do if cam is fixed
	if (camera::is_fixed()) {
		std::cout << "warning: exiting patrolling thread since camera is fixed\n";
		return NULL;
	}

	sweeper_running = false;
	while (true) {
	 	// sleep 50ms
                usleep(50000); 
		
		if (camera::is_tracking()) {
			if (sweeper_running == true) {
				int ret;
				void *status;

				// cancel thread
				ret = pthread_cancel(sweeper_id);
				if (ret)
					std::cout <<"warning: patrolling(), error cancelling thread\n";

				pthread_join(sweeper_id, &status);
				if (status != PTHREAD_CANCELED)
					std::cout << "warning: patrolling(), error joining thread\n";
				
				sweeper_running = false;
			}
		} else if (camera::is_patrolling()) {
			if (sweeper_running == false) {
				int ret;

				// start patrolling in new thread
				ret = pthread_create(&sweeper_id, NULL, sweeper, NULL);
				if (ret) {
					std::cout << "warning: patrolling(), failed to create sweeper thread\n";
					// well, if this happens we are hosed anyway: just die
					exit(1);
				}
				sweeper_running = true;
			}
		}
	}

	return NULL;
}


