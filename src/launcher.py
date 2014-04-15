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
from socket import gethostname
import subprocess

def _camera_id():
    hostname =  gethostname()
    try:
        hostname10 = int(hostname[10]);
    except:
        hostname10 = -1
    
    _id = 0
    if hostname10 == 4:
        _id = 3;
    elif hostname10 == 2:
        _id = 1;
    elif hostname10 == 3:
        _id = 2;
    elif hostname10 == 1:
        _id = 4;
    elif hostname10 == 5:
        _id = 5;
    elif hostname10 == 6:
        _id = 6;
    
    return _id


camid = _camera_id()
filename = '-'.join(['rawdata', time.strftime("%Y.%m.%d-%H.%M"), "cam"+ str(camid)])
filepath = os.path.join(os.getcwd(), filename) 

print "\nlauncher: saving experiment to %s" % filename
if os.path.exists(filepath):
    answer = raw_input("An experiment with the same name already exists. Do yout want to overwrite it ? [y/N] ")
    if answer.lower() == 'y':
        os.remove(filepath)
    else:
        os.sys.exit(0)

cmd = "./tracker | tee %s" % filepath
print "\n-----------8<-------------\n\n"

subprocess.call(cmd, stdout=sys.stdout, stdin=sys.stdin, shell=True)

print "\n\n----------->8-------------"

