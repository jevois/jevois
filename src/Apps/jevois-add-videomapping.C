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
#include <jevois/Util/Utils.H>
#include <fstream>
#include <iostream>
#include <sstream>

//! Add a new mapping to videomappings.cfg skipping duplicates
/*! This little app was created so that we ensure perfect consistency between the kernel driver and the user code by
    using exactly the same config file parsing code (in VideoMapping). */
int main(int argc, char const* argv[])
{
  jevois::logLevel = LOG_INFO;//CRIT;

  if (argc != 11)
    LFATAL("USAGE: jevois-add-videomapping <USBmode> <USBwidth> <USBheight> <USBfps> <CAMmode> "
           "<CAMwidth> <CAMheight> <CAMfps> <Vendor> <Module>");

  std::string args;
  for (int i = 1; i < argc; ++i) { args += argv[i]; args += ' '; }
  args += "\n";
  std::stringstream ss(args);
  
  // Create the new mapping by parsing the command-line args. Here we are lenient and do not check for existence of the
  // .so or .py file, as it may get installed later, and we assume GUI is available:
  size_t defidx;
  std::vector<jevois::VideoMapping> vm =
    jevois::videoMappingsFromStream(jevois::CameraSensor::any, ss, defidx, false, true);
  if (vm.size() != 1)
    LFATAL("Could not parse input args into a valid video mapping: [" << ss.str() << ']');
  jevois::VideoMapping & m = vm[0];

  // Parse the videomappings.cfg file and create the mappings, do not check for .so/.py existence, assume GUI exists:
  std::ifstream ifs(JEVOIS_ENGINE_CONFIG_FILE);
  if (ifs.is_open() == false) LFATAL("Could not open [" << JEVOIS_ENGINE_CONFIG_FILE << ']');
  std::vector<jevois::VideoMapping> mappings =
    jevois::videoMappingsFromStream(jevois::CameraSensor::any, ifs, defidx, false, true);
  ifs.close();
  
  // Check for match, ignoring the python field since we did not set it:
  for (jevois::VideoMapping const & mm : mappings)
    if (m.hasSameSpecsAs(mm) && m.wdr == mm.wdr && m.vendor == mm.vendor && m.modulename == mm.modulename)
      return 0; // We found it. Nothing to add and we are done.

  // Not found, so add one line to videomappings.cfg with the new mapping:
  std::ofstream ofs(JEVOIS_ENGINE_CONFIG_FILE, std::ios_base::app);
  if (ofs.is_open() == false) LFATAL("Could not write to [" << JEVOIS_ENGINE_CONFIG_FILE << ']');
  ofs << std::endl << m << std::endl;

  LINFO("Added [" << m.str() << "] to [" << JEVOIS_ENGINE_CONFIG_FILE << ']');
  
  // Terminate logger:
  jevois::logEnd();

  return 0;
}

