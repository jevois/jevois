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

#include <string>

namespace jevois
{
  //! Get CPU info: frequency, thermal, load
  /*! \ingroup debugging */
  std::string getSysInfoCPU();

  //! Get memory info
  /*! \ingroup debugging */
  std::string getSysInfoMem();

  //! Get O.S. version info
  /*! \ingroup debugging */
  std::string getSysInfoVersion();

  //! Get the number of Coral TPUs present on this system
  size_t getNumInstalledTPUs();

  //! Get the number of Myriad-X VPUs present on this system
  size_t getNumInstalledVPUs();
  
  //! Get the number of JeVois-Pro NPUs present on this system
  size_t getNumInstalledNPUs();

  //! Get the number of Hailo8 SPUs present on this system
  size_t getNumInstalledSPUs();

  //! Get fan speed in percent, only meaningful on JeVois-Pro Platform, all others return 0
  int getFanSpeed();
  
} // namespace jevois
