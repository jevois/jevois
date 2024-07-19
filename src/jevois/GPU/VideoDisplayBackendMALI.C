// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

#ifdef JEVOIS_PLATFORM_PRO

#include <jevois/GPU/VideoDisplayBackendMALI.H>

// ##############################################################################################################
jevois::VideoDisplayBackendMALI::VideoDisplayBackendMALI() :
    jevois::VideoDisplayBackend()
{ }

// ##############################################################################################################
jevois::VideoDisplayBackendMALI::~VideoDisplayBackendMALI()
{
  // It is better to call uninit() explicitly from the same thread that called init() and others, but just in case:
  uninit();
}

// ##############################################################################################################
void jevois::VideoDisplayBackendMALI::init(unsigned short w, unsigned short h, bool)
{
  // On platform, use an fbdev_window to draw to the framebuffer directly. No other initialization is needed:
  itsWindow.width = w; itsWindow.height = h;
  LINFO("Display surface size: " << w << 'x' << h);

  // Initialize OpenGL. Will turn itsDisplayReady to true:
  jevois::VideoDisplayBackend::init(w, h, (EGLNativeWindowType)(&itsWindow));
}

// ##############################################################################################################
bool jevois::VideoDisplayBackendMALI::pollEvents(bool & shouldclose)
{
  // MALI framebuffer does not provide any event handling:
  shouldclose = false;
  return false;
}

#endif // JEVOIS_PLATFORM_PRO
