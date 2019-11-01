// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
// California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
//
// This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
// redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
// Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.  You should have received a copy of the GNU General Public License along with this program;
// if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
// Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*! \file */

#include <jevois/Debug/Timer.H>
#include <jevois/Debug/Log.H>
#include <sstream>
#include <iomanip>
#include <fstream>

#define _BSD_SOURCE         /* See feature_test_macros(7) */
#include <stdlib.h> // for getloadavg()

namespace
{
  void secs2str(std::ostringstream & ss, double secs)
  {
    if (secs < 1.0e-6) ss << secs * 1.0e9 << "ns";
    else if (secs < 1.0e-3) ss << secs * 1.0e6 << "us";
    else if (secs < 1.0) ss << secs * 1.0e3 << "ms";
    else ss << secs << 's';
  }
}

// ####################################################################################################
jevois::Timer::Timer(char const * prefix, size_t interval, int loglevel) :
    itsPrefix(prefix), itsInterval(interval), itsLogLevel(loglevel), itsCount(0),
    itsStartTime(std::chrono::high_resolution_clock::now()), itsSecs(0.0),
    itsMinSecs(1.0e30), itsMaxSecs(-1.0e30), itsStr("-- fps, --% CPU"),
    itsStartTimeForCpu(std::chrono::high_resolution_clock::now())
{
  if (interval == 0) LFATAL("Interval must be > 0");
  getrusage(RUSAGE_SELF, &itsStartRusage);
}

// ####################################################################################################
void jevois::Timer::start()
{
  itsStartTime = std::chrono::high_resolution_clock::now();
  if (itsCount == 0) { getrusage(RUSAGE_SELF, &itsStartRusage); itsStartTimeForCpu = itsStartTime; }
}

// ####################################################################################################
std::string const & jevois::Timer::stop()
{
  std::chrono::duration<double> const dur = std::chrono::high_resolution_clock::now() - itsStartTime;
  double secs = dur.count();

  // Update average duration computation:
  itsSecs += secs; ++itsCount;
  
  // Update min and max:
  if (secs < itsMinSecs) itsMinSecs = secs;
  if (secs > itsMaxSecs) itsMaxSecs = secs;
  
  if (itsCount >= itsInterval)
  {
    double avgsecs = itsSecs / itsInterval;
    std::ostringstream ss;
    ss << itsPrefix << " average (" << itsInterval << ") duration "; secs2str(ss, avgsecs);
    ss << " ["; secs2str(ss, itsMinSecs); ss << " .. "; secs2str(ss, itsMaxSecs); ss<< ']'; 
    
    float fps = 0.0F;
    if (avgsecs > 0.0) { fps = 1.0F / float(avgsecs); ss << " (" << fps << " fps)"; }
    
    switch (itsLogLevel)
    {
    case LOG_INFO: LINFO(ss.str()); break;
    case LOG_ERR: LERROR(ss.str()); break;
    case LOG_CRIT: LFATAL(ss.str()); break;
    default: LDEBUG(ss.str());
    }
    
    // Compute percent CPU used: since often we only run a chunk of code between start() and stop() that we want to
    // evaluate, we cannot use itsSecs is as a denominator, we need the total elapsed time since the first call to
    // start() over the current interval, hence our use of itsStartTimeForCpu here. Hence, note that the CPU load
    // reported is for the whole process that is running, not just for the critical chunk of code that is executed
    // between start() and stop(). Indeed the goal is to get an idea of the overall machine utilization:
    rusage stoprusage; getrusage(RUSAGE_SELF, &stoprusage);

    double const user_secs = double(stoprusage.ru_utime.tv_sec) - double(itsStartRusage.ru_utime.tv_sec) +
      (double(stoprusage.ru_utime.tv_usec) - double(itsStartRusage.ru_utime.tv_usec)) / 1000000.0;

    std::chrono::duration<double> const cpudur = std::chrono::high_resolution_clock::now() - itsStartTimeForCpu;
    
    double const cpu = 100.0 * user_secs / cpudur.count();

    // Get the CPU temperature:
    int temp = 30;
    std::ifstream ifs("/sys/class/thermal/thermal_zone0/temp");
    if (ifs.is_open())
      try { std::string t; std::getline(ifs, t); temp = std::stoi(t); ifs.close(); }
      catch (...) { } // silently ignore any exception and keep default temp if any
    
    // Most hosts report milli-degrees, platform reports straight degrees:
#ifndef JEVOIS_PLATFORM
    temp /= 1000;
#endif

    // Finally get the CPU frequency:
    int freq = 1344;
    std::ifstream ifs2("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq");
    if (ifs2.is_open())
      try { std::string f; std::getline(ifs2, f); freq = std::stoi(f) / 1000; ifs2.close(); }
      catch (...) { }  // silently ignore any exception and keep default freq if any
    
    // Ready to return all that info:
    std::ostringstream os; os << std::fixed << std::setprecision(1) << fps << " fps, " << cpu << "% CPU, "
                              << temp << "C, " << freq << " MHz";
    itsStr = os.str();      

    // Get ready for the next cycle:
    itsSecs = 0.0; itsMinSecs = 1.0e30; itsMaxSecs = -1.0e30; itsCount = 0;
  }
  
  return itsStr;
}


