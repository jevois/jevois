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

#ifdef JEVOIS_HOST_PRO
#include <jevois/GPU/ImGuiBackend.H>
#include <SDL.h>

namespace jevois
{
  //! BackendSDL for ImGui on JeVois-Pro
  /*! This abstract base class provides an interface used by VideoDisplayGUI. Derived implementations are available for
      SDL2 and Mali framebuffer + evdev. \ingroup core */
  class ImGuiBackendSDL : public ImGuiBackend
  {
    public:
      //! Constructor 
      ImGuiBackendSDL();

      //! Virtual destructor for safe inheritance, free resources
      virtual ~ImGuiBackendSDL();

      //! Initialize the underlying engine that will process events, create windows, etc
      /*! The init starts with the given initial window/framebuffer size. */
      void init(unsigned short w, unsigned short h, bool fullscreen = false, float scale = 1.0f,
                bool conslock = false) override;

      //! Poll events such as mouse movements, buttons, keyboard, joystick, and pass to ImGui
      /*! Returns true if some events were received, which can be used to turn on mouse pointer and GUI. */
      bool pollEvents(bool & shouldclose) override;

      //! Start a new frame and clear the window/framebuffer
       void newFrame() override;

      //! Render the ImGui graphics
      void render() override;

      // Returns current window size
      /*! Returns 0,0 if display has not been initialized. */
      void getWindowSize(unsigned short & w, unsigned short & h) const override;

    protected:
      void init(unsigned short w, unsigned short h, bool fullscreen = false);
      SDL_GLContext itsSDLctx;
      SDL_Window * itsSDLwin;
  };

} // namespace jevois

#endif // JEVOIS_HOST_PRO
