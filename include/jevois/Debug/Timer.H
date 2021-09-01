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

#pragma once

#include <chrono>
#include <sys/syslog.h>
#include <string>
#include <sys/time.h>
#include <sys/resource.h>

namespace jevois
{
  //! Simple timer class
  /*! This class reports the time spent between start() and stop(), at specified intervals.  Because JeVois modules
      typically work at video rates, this class only reports the average time after some number of iterations through
      start() and stop(). Thus, even if the time of an operation between start() and stop() is only a few microseconds,
      by reporting it only every 100 frames one will not slow down the overall framerate too much. See Profiler for a
      class that provides additional checkpoints between start() and stop(). \ingroup debugging */
  class Timer
  {
    public:
      //! Constructor
      Timer(char const * prefix, size_t interval = 100, int loglevel = LOG_INFO);

      //! Start a time measurement period
      void start();

      //! End a time measurement period, report time spent if reporting interval is reached
      /*! The fps and cpu load are returned, in case users want to show this info, eg, in an overlay display. Note that
          the values are only updated when the reporting interval is reached, and remain the same in between. If seconds
          is not null, it will be set to the instantaneous number of seconds between start() and stop(). */
      std::string const & stop(double * seconds);

      //! Same as the other signature of stop() except does not provide seconds, for python bindings
      std::string const & stop();

    private:
      std::string const itsPrefix;
      size_t const itsInterval;
      int const itsLogLevel;
      
      size_t itsCount;
      std::chrono::time_point<std::chrono::steady_clock> itsStartTime;
      double itsSecs, itsMinSecs, itsMaxSecs;
      std::string itsStr;

      rusage itsStartRusage;
      std::chrono::time_point<std::chrono::steady_clock> itsStartTimeForCpu;
  };
  
  //! Simple one-shot timer class
  /*! This class reports the time spent between start() and stop(). Typically, this may be useful to report processing
      time of a neural network, by rendering the string returned by stop() into the video output.  \ingroup debugging */
  class TimerOne
  {
    public:
      //! Constructor
      TimerOne(char const * prefix);

      //! Start a time measurement period
      void start();

      //! End a time measurement period, report time spent as: 'prefix: %ms (%fps)' where % is replaced by values
      /*! If seconds is not null, it will be set to the instantaneous number of seconds between start() and stop(). */
      std::string stop(double * seconds);

      //! Same as the other signature of stop() except does not provide seconds, for python bindings
      std::string stop();

    private:
      std::string const itsPrefix;
      std::chrono::time_point<std::chrono::steady_clock> itsStartTime;
  };
}
