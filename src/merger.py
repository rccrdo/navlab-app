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
import matplotlib
from matplotlib import rc
rc('font', family='sans-serif')
import matplotlib.pyplot as plt


_FONTSIZE = 12

# ulisse 1 indietro di 5 da tutti 

class Observation():
    def __init__(self, time_skew=0., min_event_lenght=0.):
    	assert isinstance(time_skew, float)
    	assert isinstance(min_event_lenght, float)
    	assert min_event_lenght >= 0.
    	self._values = []
    	self._times = []
    	self._time_skew = time_skew
        self._intervals = None
    	self._min_event_lenght = min_event_lenght

    def accumulate(self, value, _time):
    	assert isinstance(value, bool)
    	assert isinstance(_time, float)
    	assert len(self._values) == len(self._times)
    	
    	_time -= self._time_skew
    	
    	append = False
    	if len(self._times):
    	    last_time = self._times[-1]

            last_value = self._values[-1]
            if value != last_value:
                # code in this class depends on the assumption that in the 
                # accumulated data every positive-edge is followed only by
                # a negative-edge or the other way around
                if last_time < _time:
                    append = True
                elif last_time == _time:
                    print "warning, consecutive observations with same time (%f)" % _time
                    # invalidate last observation and discard the present one
                    self._values.pop()
                    self._times.pop()
                    
                    # this also invalidates the already computed intervals
                    self._intervals = None
                else:
                    print "error, new observation is in the past!"
                    assert 0
        else:
            # code in this class depends on the assumption that
            # the first accumulated event is a positive-edge
            if value:
            	append = True

        if append:
            self._values.append(value)
            self._times.append(_time)

            # invalidate the already computed intervals
            self._intervals = None


    def intervals(self):
        # Returns the time intervals during wich the object was tracked
        if self._intervals:
            return self._intervals
        
    	assert len(self._values) == len(self._times)

        intervals = []
        start = None
        for i in xrange(len(self._values)):
            val = self._values[i]
            if start == None:
                # this is a positive edge 'patrolling -> tracking'
                assert val
                start = self._times[i]
            else:
                # this is a negative edge 'tracking -> patrolling'
                assert not val
                
                end = self._times[i]
                assert end > start
                intervals.append((start, end))
                start = None

        self._intervals = intervals
        return intervals


    def intervals_filtered(self, min_start=None, max_end=None):
        # Returns only the intervals with lenght > self._min_event_lenght
        intervals = self.intervals()
        filtered = []
        for i in xrange(len(intervals)):
            start, end = intervals[i]

            if min_start != None and start < min_start:
                continue

            if max_end != None and end > max_end:
                continue
            
            if end - start >= self._min_event_lenght:
                filtered.append((start,end))
        return filtered


    def _plot_intervals(self, intervals, color_long, color_short, axis, x_offset=0., y_offset=0.):

        for entry in intervals:
            start, end = entry
            height = 0.5
            assert end > start
            width = end - start
            color = color_long
            if width < self._min_event_lenght:
                color = color_short
            axis.add_patch(matplotlib.patches.Rectangle((start - x_offset, y_offset-height/2.), width, height, facecolor=color, edgecolor=(1,1,1,0)))

    def plot_intervals(self, axis, min_start=None, max_end=None, x_offset=0., y_offset=0.):
    	assert len(self._values) == len(self._times)
    	
    	color_gray = (0.75,0.75,0.75, 1)
    	color_int = (0.25,0.25,0.9, 1)
    	color_short = (0.9,0.25,0.25, 1)
    	if min_start != None:
    	    intervals = self.intervals_filtered(None, min_start)
    	    self._plot_intervals(intervals, color_gray, color_gray, axis, x_offset, y_offset)
            if max_end != None:
                intervals = self.intervals_filtered(min_start, max_end)
                self._plot_intervals(intervals, color_int, color_short, axis, x_offset, y_offset)

    	if max_end != None:
    	    if min_start == None:
                intervals = self.intervals_filtered(None, max_end)
                self._plot_intervals(intervals, color_int, color_short, axis, x_offset, y_offset)

    	    intervals = self.intervals_filtered(max_end, None)
    	    self._plot_intervals(intervals, color_gray, color_gray, axis, x_offset, y_offset)
    	
    	if min_start == None and max_end == None:
            intervals = self.intervals_filtered(None, None)
            self._plot_intervals(intervals, color_int, color_short, axis, x_offset, y_offset)
             
        #for entry in self.intervals():
        #    start, end = entry
        #    height = 0.5
        #    assert end > start
        #    width = end - start
        #    color = (0.25,0.25,0.9, 1)
        #    if width < self._min_event_lenght:
        #        color = (0.9,0.25,0.25, 1)
        #    axis.add_patch(matplotlib.patches.Rectangle((start - x_offset, y_offset-height/2.), width, height, facecolor=color, edgecolor=(1,1,1,0)))


    def xlim(self):
        if not len(self._times):
            return (None, None)
        #if len(self._times) < 2:
        #   return (self.,None)
        return (self._times[0], self._times[-1])


def _observations_to_symbols(observations, min_start, max_end):
    assert isinstance(observations, dict)
    intervals_list = {}
    for camid, observ in observations.iteritems():
        intervals_list[camid] = observ.intervals_filtered(min_start, max_end)
    
    start = None
    sym = []
    output = ['0']
    while True:
        # check if there are intervals pending
        done = True
        for camid, intervals in intervals_list.iteritems():
            if len(intervals) > 0:
                done = False
                break
        if done:
            break
        
        if start is None:
           assert len(sym) == 0
           for camid, intervals in intervals_list.iteritems():
                if len(intervals) > 0:
                    _start, _end = intervals[0]
                    if start == None:
                        start = _start
                        sym = [camid]
                    elif start == _start:
                        assert len(sym) >= 1
                        assert camid not in sym
                        sym.append(camid)
                    elif start > _start:
                        start = _start
                        sym = [camid]
           #print "found start: ", start, sym
        else:
           # start is a valid time at this point
           assert len(sym)
           end = None
           end_sym = []
           start_other = None
           start_other_sym = []
           for camid, intervals in intervals_list.iteritems():
                if len(intervals) > 0:
                    _start, _end = intervals[0]

                    if camid in sym:
                        if end == None:
                            end = _end
                            end_sym = [camid]
                        elif end == _end:
                            assert len(end_sym) >= 1
                            assert camid not in end_sym
                            end_sym.append(camid)
                        elif end > _end:
                            end = _end
                            end_sym = [camid]
                    else: # camid not in sym
                        if start_other == None:
                            start_other = _start
                            start_other_sym = [camid]
                        elif start_other == _start:
                            assert len(start_other_sym) >= 1
                            assert camid not in start_other_sym
                            start_other_sym.append(camid)
                        elif start_other > _start:
                            start_other = _start
                            start_other_sym = [camid]
                        
           #print "cur start        :", start, sym                        
           #print "found end        : ", end, end_sym
           #print "found start_other: ", start_other, start_other_sym

           output.append('.'.join([str(x) for x in sorted(sym)]))
           if (start_other is None) or (end < start_other):
               for _sym in end_sym:
                   sym.remove(_sym)
                   intervals_list[_sym].pop(0)
               start = end
               if not len(sym):
                   start = None
                   output.append('0')
           elif (start_other is not None) and (end == start_other):
               for _sym in end_sym:
                   sym.remove(_sym)
                   intervals_list[_sym].pop(0)
               for _sym in start_other_sym:
                   sym.append(_sym)
               start = start_other
               assert len(sym) > 0
           else: # end is None or end > start_other
               assert (end is None) or (end > start_other)
               for _sym in start_other_sym:
                   sym.append(_sym)
               start = start_other
               assert len(sym) > 0

    #print output
    return output


def _view_sector_index(bangles, angle):
    assert isinstance(bangles, list)
    assert isinstance(angle, float)

    nr_bangels = len(bangles)

    # there are no b-lines
    if not nr_bangles:
        return None
    
    for i in xrange(len(bangles)):
        _ang = bangles[i]
        if angle <= _ang:
            return i
    
    # angle is past the last b-line
    return nr_bangles
    
def _secid(camid, bangle_idx):
    return str(camid) + chr(ord('a') + bangle_idx)
    


if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("--cut", dest="cut", default=None,			\
                      help="Min and max times to use in the cut, format =\"t0,t1\" or \",t1\" or \"t0,\"")
    (options, args) = parser.parse_args()
    
    min_start = None
    max_end = None
    if options.cut:
        try:
            min_start, max_end = options.cut.split(',')
            if not len(min_start):
                min_start = None
            else:
                min_start = float(min_start)

            if not len(max_end):
                max_end = None
            else:
                max_end = float(max_end)

            if min_start != None and max_end != None:
                assert max_end > min_start
        except:
            print "error: the --cut argument format is \"(t0,t1)\" with t0, t1 numbers and t1 > t0\n"
            parser.print_help()
            sys.exit(0)

    # after parsing arguments we shold be left with the path to the
    # raw experiment data
    datapaths = args
    if not len(datapaths):
        print "error: no paths given for either training or validation data\n"
        parser.print_help()
        sys.exit(0)

    observations = {}
    for path in datapaths:
        if not os.path.isfile(path):
            print "warning: cannot read '%s', skipping ..." % path
            continue
        
        m = re.search(r'^[a-z]*rawdata-([0-9-.]+)-cam([0-9])$', os.path.basename(path))
        if m is None:
            print "warning: file at path '%s' does not conform the naming scheme, skipping ..." % path
            continue

        date = m.group(1)
        camid = int(m.group(2))
        if camid==6:
            camid = 5
        elif camid ==5:
            camid = 6
        if camid == 0:
            # laptop webcam
            camid = 1
        if camid in observations.keys():
            print "warning: data for camid=%d is already loaded, skipping path '%s' ..." % (camid, path)
            continue        

        print "Reading data for camera %d:" % camid
        print "------------8<------------"
        
        cam_bangles = []
        lines = open(path, 'r').read()
        lines = lines.split('\n')
        while True:
            l = lines.pop(0)
            if l.startswith('bangles'):
                #print l
       		m = re.search(r'bangles    : \(([-+\.0-9,\s]+)\)$', l)
       		#print m
                if m is not None:
                    bangle_str = m.groups()[0]
                    for a in bangle_str.split(','):
                        try:
                            cam_bangles.append(float(a))
                        except:
                            pass
                    continue
            
            if l.startswith("Hi!"):
                break
        cam_bangles = sorted(cam_bangles)
        nr_bangles = len(cam_bangles)
        print "! bangles = %s" % cam_bangles

        MIN_EVENT_LENGHT = 2.

        time_skew = 0.
        if camid == 1:
            time_skew = -5.
            
        if nr_bangles:
            for i in xrange(nr_bangles+1):
		area_id = _secid(camid, i)
		#print area_id
                observations[area_id] = Observation(time_skew, MIN_EVENT_LENGHT)
        else:
            observations[camid] = Observation(time_skew, MIN_EVENT_LENGHT)


        start_time = None
        last_angle = None
        last_angle_idx = None
        last_angle_time = None
        
        for l in lines:
            l = l.strip(' \n\t')
            if not len(l):
                continue

            if start_time is None:
                if l.startswith('0->1@'):
                    m = re.search(r'^0->1@([0-9\.]+)$', l)
                    if m is not None:
                        timestr = m.groups()[0]
                        _time = float(timestr)
                        assert _time is not None
                        start_time = _time
                        #print " found start_time", start_time
                        continue
                elif l.startswith('1->0@'):
  		    m = re.search(r'^1->0@([0-9\.]+)$', l)
                    timestr = "?"
                    if m is not None:
                        timestr = m.groups()[0]
                    print "warning, event 1->0 before 0->1 @ time %s" % timestr
                    assert 0
                    continue
            else:
                # start is a valid time
                if not nr_bangles:
                    if l.startswith('1->0@'):
                        m = re.search(r'^1->0@([0-9\.]+)$', l)
                        if m is not None:
                            timestr = m.groups()[0]
                            _time = float(timestr)
                            assert _time is not None
                            #print " closing interval for camid %d, (%f, %f)" % (camid, start_time, _time)
                            observations[camid].accumulate(True, start_time)
                            observations[camid].accumulate(False, _time)
                            start_time = None
                            continue
                    elif l.startswith('0->1@'):
  		        m = re.search(r'^0->1@([0-9\.]+)$', l)
                        timestr = "?"
                        if m is not None:
                            timestr = m.groups()[0]
                        print "warning, event 0->1 before 1->0 @ time %s" % timestr
                        assert 0
                        continue
                
                else:
                    # nr_bangles > 0
                    if l.startswith('angle='):
  		        m = re.search(r'^angle=([-\.0-9]+)@([0-9\.]+)$', l)
                        if m is not None:
                            angle_str = m.groups()[0]
                            time_str = m.groups()[1]
                            angle = float(angle_str)
                            _time = float(time_str)      
                            #print l, " time:", time_str, _time                  
                            if last_angle == None:
                                last_angle = angle
                                last_angle_idx = _view_sector_index(cam_bangles, angle)
                                last_angle_time = _time
                                continue
                            else:
                                # last_angle is a valid angle
                                angle_idx = _view_sector_index(cam_bangles, angle)
                                if angle_idx == last_angle_idx:
                                    last_angle_time = _time
                                    continue
                                else:
                                    # angle_idx != last_angle_idx
                                    midtime = last_angle_time + (_time - last_angle_time)/2.

                                    #print "jumping between sections", _secid(camid, _view_sector_index(cam_bangles, last_angle)), _secid(camid, _view_sector_index(cam_bangles, angle)), last_angle, angle

                                    secid = _secid(camid, last_angle_idx)                                    
                                    observations[secid].accumulate(True, start_time)
                                    observations[secid].accumulate(False, midtime)
                                    
                                    start_time = _time
                                    last_angle = angle
                                    last_angle_idx = angle_idx
                                    last_angle_time = _time
                                    continue
                                
                    elif l.startswith('1->0@'):
                        m = re.search(r'^1->0@([0-9\.]+)$', l)
                        if m is not None:
                            timestr = m.groups()[0]
                            _time = float(timestr)
                            assert _time is not None
                            if last_angle_idx is None:
                                print "discarding even 0->1->0 without angle info"
                            else:
                                secid = _secid(camid, last_angle_idx)
                                observations[secid].accumulate(True, start_time)
                                observations[secid].accumulate(False, _time)

                            start_time = None
                            last_angle = None
                            last_angle_idx = None
                            last_angle_time = None
                            continue


        print "------------>8------------\n\n"
        

    if not len(observations):
        print "No observation data loaded. Bye!"
        sys.exit(0)
      
        
    fig = plt.figure(figsize=(15,6))
    plt.ion()
    plt.show()

      
    # focus and setup the this figure
    plt.figure(fig.number)
    fig.clf()
    fig.patch.set_facecolor((1,1,1))

    gs = matplotlib.gridspec.GridSpec(1, 1, left=0.05, right=0.975, top=0.95, bottom=0.1, wspace=0, hspace=0)
    axis = plt.subplot(gs[0,0])
    plt.axis('on')
    plt.grid('on')
    plt.hold('on')

    xmin = None
    xmax = None
    for camid, observ in observations.iteritems():
        _xmin, _xmax = observ.xlim()
        if _xmin is not None:
            if xmin is None:
                xmin = _xmin
            else:
                xmin = min(xmin, _xmin)
        if _xmax is not None:
            if xmax is None:
                xmax = _xmax
            else:
                xmax = max(xmax, _xmax)
 
    #print xmax, xmin, xmax-xmin
    print "Observation data interval: %.2fs or %.2fmin" % (xmax-xmin, (xmax-xmin)/60.)

    if min_start != None:
        min_start += xmin
    if max_end != None:
        max_end += xmin

    i = 0
    ylabels = tuple(sorted(observations.keys()))
    for camid in ylabels:
        observ = observations[camid]
        i += 1
        #print camid, i, observ.intervals()
        observ.plot_intervals(axis, min_start, max_end, y_offset=i)#y_offset=camid)

    plt.ylim(0, i+1)
    plt.yticks(xrange(1, i+1), ylabels, fontsize=_FONTSIZE)
    xticks = range(int(math.floor(xmin)), int(math.floor(xmax)), 100)
    xlabels = [str(int(x-math.floor(xmin))) for x in xticks]
    plt.xticks(xticks, xlabels, fontsize=_FONTSIZE)

    dx = 10
    plt.xlim(xmin - dx, xmax + dx)
    if 0:
        plt.xlim(xmin + 1600 , xmin + 2000)
    plt.xlabel('time [s]', fontsize=_FONTSIZE)

    fig.canvas.draw()



    output = _observations_to_symbols(observations, min_start, max_end)
    #print output
    if options.cut:
        cut_len = max_end - min_start
        print "Cut = %s,%s, interval len. %ds or %.2fmin" % (int(min_start-xmin), int(max_end-xmin), cut_len, cut_len/60.)
    
    # Save observation data to file
    filename = '-'.join(['expdata', time.strftime("%Y.%m.%d-%H.%M")])
    savepath = os.path.join(os.getcwd(), filename) 
    print "Saving formatted observations to ", savepath
    if os.path.exists(savepath):
        answer = raw_input("An experiment with the same name already exists. Do yout want to overwrite it ? [y/N] ")
        if answer.lower() == 'y':
            os.remove(savepath)
        else:
            os.sys.exit(0)

    file(savepath, 'w').write(repr(output))
    
    fig.savefig(savepath + ".png")

    raw_input()
        
