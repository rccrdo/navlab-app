#!/usr/bin/env python

# Copyright (c) 2013 Riccardo Lucchese, riccardo.lucchese at gmail.com
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
#    1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
#    2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
#    3. This notice may not be removed or altered from any source
#    distribution.

import os
import sys
import time
import math
import re
from optparse import OptionParser


if __name__ == "__main__":
    parser = OptionParser()
    (options, args) = parser.parse_args()

    # after parsing arguments we shold be left with the path to the
    # raw experiment data
    datapaths = args
    if not len(datapaths):
        print "error: no paths given \n"
        parser.print_help()
        sys.exit(0)

    observations = {}
    for path in datapaths:
        if not os.path.isfile(path):
            print "warning: cannot read '%s', skipping ..." % path
            continue
        
        m = re.search(r'^rawdata-([0-9-.]+)-cam([0-9])$', os.path.basename(path))
        if m is None:
            print "warning: file at path '%s' does not conform the naming scheme, skipping ..." % path
            continue

        date = m.group(1)
        camid = int(m.group(2))

        print "Reading data for camera %d:" % camid
        print "------------8<------------"

        stripped = []
        lines = open(path, 'r').read()
        lines = lines.split('\n')
        for i in xrange(len(lines)):
            l = lines[i]
            #print "\"%s\"" % l
            if l.startswith('ctrl_step_estimate'):
            	continue
            if l.startswith('  ctrl_step_estimate'):
            	continue
            if l.startswith('  CTRL_POS'):
            	continue
            if l.startswith('    dpan='):
            	continue
            if l.startswith('ulisse_get_PTZ()'):
            	continue
            if l.startswith('camera::set_pos'):
            	continue
            if l.startswith('  camera::set_pos'):
            	continue
	    if l.startswith('camera::time_to_reach'):
	        continue
	    if l.startswith('camera::get_pos_estimate'):
	        continue
	    if l.startswith('camera::move'):
	        continue
	    if l.startswith('camera::serial'):
	        continue
	    if l.startswith('__move(), comando'):
	        continue
	    if l.startswith('warning, __velocity()'):
	        continue
	    if l.startswith('__velocity'):
	        continue
	    if l.startswith('camera::velocity'):
	        continue
	    if l.startswith('  camera::velocity'):
	        continue
	    if l.startswith('ctrl_thread:'):
	        continue
	    if l.startswith('  ctrl_thread:'):
	        continue
	    if l.startswith('ctrl_estimator_measure_pos'):
	        continue
	    if l.startswith('warning, ulisse_get_PTZ'):
	        continue
	    if l.startswith('warning, camera::time_to_reach'):
	        continue

	    if l.startswith('Saving camera configuration to'):
	        continue
	    if 'time=' in l:
	        continue
	    if ')' in l:
	        continue
	        
            lstrip = l.strip(' \n')
            if not len(lstrip):
                continue


            stripped.append(l)
            
        print "------------>8------------\n\n"


        file('stripped'+path, 'w').writelines('\n'.join(stripped))
        
