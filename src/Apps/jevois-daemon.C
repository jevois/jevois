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

#include <jevois/Core/Engine.H>
#include <exception>

//! Main daemon that grabs video frames from the camera, sends them to processing, and sends the results out over USB
int main(int argc, char const* argv[])
{
  int ret = 127;

  try
  {
    // Get an engine going, using the platform camera and platform USB gadget driver:
    std::shared_ptr<jevois::Engine> engine(new jevois::Engine(argc, argv, "engine"));
    
    engine->init();
    
#ifndef JEVOIS_PLATFORM_A33
    // Start streaming now when running on host or JeVois-Pro (since in desktop mode we have no USB host that will
    // initiate streaming). Note that streamOn() could throw if the default module is buggy or uses an unsupported
    // camera format, so just ignore any streamOn() exception so we can start the engine's main loop below:
    try { engine->streamOn(); } catch (...) { }
#endif
    
    // Enter main loop, if it exits normally, it will give us a return value; or it could throw:
    ret = engine->mainLoop();
  }
  catch (std::exception const & e) { std::cerr << "Exiting on exception: " << e.what(); }
  catch (...) { std::cerr << "Exiting on unknown exception"; }

  // Terminate logger:
  jevois::logEnd();
  
  return ret;
}
