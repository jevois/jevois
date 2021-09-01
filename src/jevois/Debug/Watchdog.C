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

#include <jevois/Debug/Watchdog.H>
#include <jevois/Util/Async.H>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>

#include <unistd.h>

// ####################################################################################################
jevois::Watchdog::Watchdog(double timeout)
{
  // Launch run thread:
  itsRunFut = jevois::async_little(std::bind(&jevois::Watchdog::run, this, timeout));

  // Wait until we are running:
  while (itsRunning.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

// ####################################################################################################
jevois::Watchdog::~Watchdog()
{
  itsReset = true;

  // Stop the run() thread:
  itsRunning = false;
  JEVOIS_WAIT_GET_FUTURE(itsRunFut);
}

// ####################################################################################################
void jevois::Watchdog::reset()
{ itsReset.store(true); }

// ####################################################################################################
void jevois::Watchdog::run(double timeout)
{
  itsRunning.store(true);
  if (timeout <= 0.0F) return;
  auto const dur = std::chrono::microseconds(long(timeout * 1.0e6));
  
  while (itsRunning.load())
  {
    // Sleep...
    itsReset.store(false);
    std::this_thread::sleep_for(dur);

    // If reset() was not called, kill our process:
    if (itsReset.load() == false)
    {
      LERROR("Watchdog timed out -- KILLING PROCESS");
      jevois::system("/usr/bin/kill -9 " + std::to_string(getpid()));
    }
  }
}
