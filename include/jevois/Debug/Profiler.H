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
#include <vector>

namespace jevois
{
  //! Simple profiler class
  /*! This class reports the time spent between start() and each of the checkpoint() calls, separately computed for each
      checkpoint string, at specified intervals. Because JeVois modules typically work at video rates, this class only
      reports the average time after some number of iterations through the start(), checkpoint(), and stop(). Thus, even
      if the time between two checkpoints is only a few microseconds, by reporting it only every 100 frames one will not
      slow down the overall framerate too much. See Timer for a lighter class with only start() and stop().
      \ingroup debugging */
  class Profiler
  {
    public:
      //! Constructor
      Profiler(char const * prefix, size_t interval = 100, int loglevel = LOG_INFO);

      //! Start a time measurement period
      void start();

      //! Note the time for a particular event
      /*! The delta time between this event and the previous one (or start() for the first checkpoint) will be
          reported. Note that we create a new unique entry in our tables for each description value, so you should keep
          the number of unique descriptions passed small (do not include a frame number or some parameter value). The
          description is passed as a raw C string to encourage you to just use a string literal for it. */
      void checkpoint(char const * description);
      
      //! End a time measurement period, report time spent for each checkpoint if reporting interval is reached
      /*! The time reported is from start to each checkpoint. */
      void stop();

    private:
      std::string const itsPrefix;
      size_t const itsInterval;
      int const itsLogLevel;
      
      std::chrono::time_point<std::chrono::high_resolution_clock> itsStartTime;

      struct data
      {
          std::string desc;
          size_t count;
          double secs;
          double minsecs;
          double maxsecs;
          std::chrono::time_point<std::chrono::high_resolution_clock> lasttime;
      };
      
      data itsData; // for the stop() checkpoint
      std::vector<data> itsCheckpointData; // one entry per checkpoint string
  };
}

      
