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

#include <jevois/Image/RawImage.H>
#include <jevois/Core/VideoMapping.H>

namespace jevois
{
  //! Base class for video output. Gadget, MovieOutput, VideoDisplay, and VideoOutputNone derive from it
  /*! This virtual base class provides the interface for video output, as needed by Engine. \ingroup core */
  class VideoOutput
  {
    public:
      //! Virtual destructor for safe inheritance
      virtual ~VideoOutput();

      //! Set the video format and frame rate
      /*! Sometimes this may be implemented as a no-op. For the USB Gadget class, we cannot decide and set the format,
          it is set as the result of USB/UVC commands being received, so that the USB host is prepared to receive images
          with the selected format and frame size. For the VideoDisplay class, this allocates the output buffers and
          must be called before streaming starts.

          Engine calls this (as well as setFormat() on the camera) from within its own setFormat(), so usually one would
          just set the format at the Engine level and not directly on VideoOutput. */
      virtual void setFormat(VideoMapping const & m) = 0;

      //! Get a pre-allocated image so that we can fill the pixel data and later send out using send()
      /*! May throw if no buffer is available, i.e., all have been queued to send to the host but have not yet been
          sent. Application code must balance exactly one send() for each get(). */
      virtual void get(RawImage & img) = 0;
      
      //! Send an image out
      /*! May throw if the format is incorrect or std::overflow_error if we have not yet consumed the previous image. */
      virtual void send(RawImage const & img) = 0;

      //! Start streaming
      virtual void streamOn() = 0;

      //! Abort streaming
      /*! This only cancels future get() and send() calls, one should still call streamOff() to turn off streaming. */
      virtual void abortStream() = 0;
      
      //! Stop streaming
      virtual void streamOff() = 0;
  };
}

