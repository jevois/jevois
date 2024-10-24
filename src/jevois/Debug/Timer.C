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
#include <jevois/Util/Utils.H>
#include <sstream>
#include <iomanip>
#include <fstream>

#define _BSD_SOURCE         /* See feature_test_macros(7) */
#include <stdlib.h> // for getloadavg()

// ####################################################################################################
jevois::Timer::Timer(char const * prefix, size_t interval, int loglevel) :
    itsPrefix(prefix), itsInterval(interval), itsLogLevel(loglevel), itsCount(0),
    itsStartTime(std::chrono::steady_clock::now()), itsSecs(0.0),
    itsMinSecs(1.0e30), itsMaxSecs(-1.0e30), itsStr("-- fps, --% CPU"),
    itsStartTimeForCpu(std::chrono::steady_clock::now())
{
  if (interval == 0) LFATAL("Interval must be > 0");
  getrusage(RUSAGE_SELF, &itsStartRusage);
}

// ####################################################################################################
void jevois::Timer::start()
{
  itsStartTime = std::chrono::steady_clock::now();
  if (itsCount == 0) { getrusage(RUSAGE_SELF, &itsStartRusage); itsStartTimeForCpu = itsStartTime; }
}

// ####################################################################################################
std::string const & jevois::Timer::stop(double * seconds)
{
  std::chrono::duration<double> const dur = std::chrono::steady_clock::now() - itsStartTime;
  double secs = dur.count();
  if (seconds) *seconds = secs;
  
  // Update average duration computation:
  itsSecs += secs; ++itsCount;
  
  // Update min and max:
  if (secs < itsMinSecs) itsMinSecs = secs;
  if (secs > itsMaxSecs) itsMaxSecs = secs;
  
  if (itsCount >= itsInterval)
  {
    double avgsecs = itsSecs / itsInterval;
    std::ostringstream ss;
    ss << itsPrefix;
    if (itsLogLevel != -1) ss << " average (" << itsInterval << ") duration ";
    jevois::secs2str(ss, avgsecs);
    ss << " ["; jevois::secs2str(ss, itsMinSecs); ss << " .. "; jevois::secs2str(ss, itsMaxSecs); ss<< ']'; 
    
    float fps = 0.0F;
    if (avgsecs > 0.0) { fps = 1.0F / float(avgsecs); ss << " (" << fps << " fps)"; }
    
    switch (itsLogLevel)
    {
    case LOG_INFO: LINFO(ss.str()); break;
    case LOG_ERR: LERROR(ss.str()); break;
    case LOG_CRIT: LFATAL(ss.str()); break;
    case -1: itsStr = ss.str(); break; // We want to get the elapsed time instead of fps/cpu/temps/etc:
    default: LDEBUG(ss.str());
    }

    if (itsLogLevel != -1)
    {
      // Compute percent CPU used: since often we only run a chunk of code between start() and stop() that we want to
      // evaluate, we cannot use itsSecs is as a denominator, we need the total elapsed time since the first call to
      // start() over the current interval, hence our use of itsStartTimeForCpu here. Hence, note that the CPU load
      // reported is for the whole process that is running, not just for the critical chunk of code that is executed
      // between start() and stop(). Indeed the goal is to get an idea of the overall machine utilization:
      rusage stoprusage; getrusage(RUSAGE_SELF, &stoprusage);
      
      double const user_secs = double(stoprusage.ru_utime.tv_sec) - double(itsStartRusage.ru_utime.tv_sec) +
        (double(stoprusage.ru_utime.tv_usec) - double(itsStartRusage.ru_utime.tv_usec)) / 1000000.0;
      
      std::chrono::duration<double> const cpudur = std::chrono::steady_clock::now() - itsStartTimeForCpu;
      
      double const cpu = 100.0 * user_secs / cpudur.count();
      
#ifdef JEVOIS_PLATFORM_PRO
      // One JeVois Pro, use cpu 2 (big core) and thermal zone 1:
      static char const tempname[] = "/sys/class/thermal/thermal_zone1/temp";
      static char const freqname[] = "/sys/devices/system/cpu/cpu2/cpufreq/cpuinfo_cur_freq";
      int freq = 2208;
#else
      static char const tempname[] = "/sys/class/thermal/thermal_zone0/temp";
      static char const freqname[] = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq";
      int freq = 1344;
#endif
      
      // Get the CPU temperature:
      int temp = 30;
      std::ifstream ifs(tempname);
      if (ifs.is_open())
      {
        try { std::string t; std::getline(ifs, t); temp = std::stoi(t); }
        catch (...) { } // silently ignore any exception and keep default temp if any
        ifs.close();
      }
      
      // Most hosts report milli-degrees, JeVois-A33 platform reports straight degrees:
#ifndef JEVOIS_PLATFORM_A33
      temp /= 1000;
#endif
      
      // Finally get the CPU frequency:
      std::ifstream ifs2(freqname);
      if (ifs2.is_open())
      {
        try { std::string f; std::getline(ifs2, f); freq = std::stoi(f) / 1000; }
        catch (...) { }  // silently ignore any exception and keep default freq if any
        ifs2.close();
      }
      
      // Ready to return all that info:
      std::ostringstream os; os << std::fixed << std::setprecision(1) << fps << " fps, " << cpu << "% CPU, "
                                << temp << "C, " << freq << " MHz";
      itsStr = os.str();
    }
    
    // Get ready for the next cycle:
    itsSecs = 0.0; itsMinSecs = 1.0e30; itsMaxSecs = -1.0e30; itsCount = 0;
  }
  
  return itsStr;
}

// ####################################################################################################
std::string const & jevois::Timer::stop()
{ return stop(nullptr); }

// ####################################################################################################
// ####################################################################################################
jevois::TimerOne::TimerOne(char const * prefix) :
    itsPrefix(prefix)
{ }

// ####################################################################################################
void jevois::TimerOne::start()
{
  itsStartTime = std::chrono::steady_clock::now();
}

// ####################################################################################################
std::string jevois::TimerOne::stop(double * seconds)
{
  std::chrono::duration<double> const dur = std::chrono::steady_clock::now() - itsStartTime;
  double secs = dur.count();
  if (seconds) *seconds = secs;

  std::ostringstream ss;
  ss << itsPrefix << ": " << std::fixed << std::setprecision(1); jevois::secs2str(ss, secs);
  if (secs == 0.0) ss << " (INF fps)"; else ss << " (" << 1.0 / secs << "fps)";

  return ss.str();
}

// ####################################################################################################
std::string jevois::TimerOne::stop()
{ return stop(nullptr); }
