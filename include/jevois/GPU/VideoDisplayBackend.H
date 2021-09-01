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

#ifdef JEVOIS_PRO

#include <jevois/GPU/OpenGL.H>

namespace jevois
{
  //! Backend for VideoDisplay on JeVois-Pro
  /*! This abstract base class provides an interface used by VideoDisplayGL. Derived implementations are available for
      X11+OpenGL-ES 3.2 (to be used on JeVois-Pro host) and Mali framebuffer + OpenGL-ES 3.2 (to be used on JeVois-Pro
      platform).  \ingroup core */
  class VideoDisplayBackend
  {
    public:
      //! Constructor 
      VideoDisplayBackend();

      //! Virtual destructor for safe inheritance, free resources
      virtual ~VideoDisplayBackend();

      //! Initialize the underlying engine that will process events, create windows, etc
      /*! The init starts with the given initial framebuffer size. Note that on MALI, fullscreen parameter is ignored
          and we always run full-screen. */
      virtual void init(unsigned short w, unsigned short h, bool fullscreen = false) = 0;

      //! Poll events such as mouse movements, buttons, keyboard, joystick, and pass to VideoDisplay
      /*! Returns true if some events were received, which can be used to turn on mouse pointer and GUI. */
      virtual bool pollEvents(bool & shouldclose) = 0;

      //! Start a new frame and clear the window/framebuffer
      virtual void newFrame();

      //! Render the VideoDisplay graphics
      virtual void render();

      // Returns current window size
      /*! Returns 0,0 if display has not been initialized. */
      virtual void getWindowSize(unsigned short & w, unsigned short & h) const;

      //! Access our display
      /*! May return nullptr if init() has not yet been called. */
      EGLDisplay getDisplay() const;

    protected:
      //! Initialize the underlying engine that will process events, create windows, etc
      /*! The init starts with the given initial window/framebuffer size. In the base class, we only initialize
          OpenGL. Derived classes should first create a window or framebuffer as needed. */
      virtual void init(unsigned short w, unsigned short h, EGLNativeWindowType win);

      //! Un-initialize the underlying engine, close windows, etc
      /*! In the base class, we only close OpenGL. */
      virtual void uninit();
      
      std::string const itsName;
      EGLDisplay itsDisplay = 0;
      EGLConfig itsConfig = 0;
      EGLContext itsContext = 0;
      EGLSurface itsSurface = 0;
  };

} // namespace jevois

#endif // JEVOIS_PRO
