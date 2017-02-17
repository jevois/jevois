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

#include <jevois/Debug/Log.H>
#include <jevois/Core/VideoMapping.H>
#include <fstream>
#include <iostream>

//! Parse videomappings.cfg and output a string to be passed to the jevois kernel module
/*! This little app was created so that we ensure perfect consistency between the kernel driver and the user code by
    using exactly the same config file parsing and sorting code (in VideoMapping). */
int main()
{
  jevois::logLevel = LOG_CRIT;
  
  // Parse the videomappings.cfg file and create the mappings:
  size_t defidx;
  std::ifstream ifs(JEVOIS_ENGINE_CONFIG_FILE);
  if (ifs.is_open() == false) LFATAL("Could not open [" << JEVOIS_ENGINE_CONFIG_FILE << ']');
  std::vector<jevois::VideoMapping> mappings = jevois::videoMappingsFromStream(ifs, defidx);

  // First, the default index (0-based), but we need to skip over the no-usb mappings:
  size_t uvcdefidx = 0;
  for (size_t i = 0; i <= defidx; ++i) if (mappings[i].ofmt != 0) ++uvcdefidx;
  if (uvcdefidx == 0) LFATAL("No mappings with UVC output?");
  std::cout << (uvcdefidx - 1); // FIXME: the kernel is actually not using the default index

  // Then each format, grouping by video fcc and framerates:
  unsigned int ofmt = ~0U, ow = ~0U, oh = ~0u;
  for (jevois::VideoMapping const & m : mappings)
  {
    if (m.ofmt == 0) { ofmt = 0; continue; } // skip over the no-USB mappings
    if (m.ofmt != ofmt) { std::cout << '/' << m.ofmt; ofmt = m.ofmt; ow = ~0U; oh = ~0U; }
    if (m.ow != ow || m.oh != oh) { std::cout << '-' << m.ow << 'x' << m.oh; ow = m.ow; oh = m.oh; }
    std::cout << ':' << jevois::VideoMapping::fpsToUvc(m.ofps);
  }
  std::cout << std::endl;
  
  return 0;
}

