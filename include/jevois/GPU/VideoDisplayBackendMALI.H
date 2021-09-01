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

#pragma once

#ifdef JEVOIS_PLATFORM_PRO

#include <jevois/GPU/VideoDisplayBackend.H>

namespace jevois
{
  //! Backend for VideoDisplay on JeVois-Pro host using MALI
  /*! This class provides an interface used by VideoDisplayGL using Mali framebuffer + OpenGL-ES 3.2 (to be used on
      JeVois-Pro platform).  \ingroup core */
  class VideoDisplayBackendMALI : public VideoDisplayBackend
  {
    public:
      //! Constructor 
      VideoDisplayBackendMALI();

      //! Virtual destructor for safe inheritance, free resources
      virtual ~VideoDisplayBackendMALI();

      //! Initialize the underlying engine that will process events, create windows, etc
      /*! The init starts with the given initial framebuffer size. Note that on MALI, size and fullscreen parameters are
          ignored and we always run full-screen at the native resolution of the framebuffer. */
      virtual void init(unsigned short w, unsigned short h, bool fullscreen = false) override;

      //! Poll events such as mouse movements, buttons, keyboard, joystick, and pass to VideoDisplay
      /*! Returns true if some events were received, which can be used to turn on mouse pointer and GUI. On MALI, there
          is no support for events at this level, and hence no support for a test to close. */
      virtual bool pollEvents(bool & shouldclose) override;

    protected:
      // On platform, use an fbdev_window to draw to the framebuffer directly:
      fbdev_window itsWindow;
  };

} // namespace jevois

#endif // JEVOIS_PLATFORM_PRO
