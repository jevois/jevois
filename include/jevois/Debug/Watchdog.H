// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2021 by Laurent Itti, the University of Southern
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

#include <future>

namespace jevois
{
  //! Simple watchdog class
  /*! This class will kill the current process if reset() is not called at least every timeout seconds. If timeout <=
      0.0 then the watchdog is inactive (will never time out and kill the process). Note that to be as lightweight as
      possible, this class runs a background thread with a loop that iterates every timeout seconds, so the exact kill
      time is approximate. \ingroup debugging */
  class Watchdog
  {
    public:
      //! Constructor
      Watchdog(double timeout);

      //! Virtual destructor for safe inheritance
      ~Watchdog();
      
      //! Reset our internal timer. If this does not happen at least every timeout seconds, process is killed
      void reset();

    protected:
      void run(double timeout);
      std::future<void> itsRunFut;
      std::atomic<bool> itsReset = true;
      std::atomic<bool> itsRunning = false;
  };
}
