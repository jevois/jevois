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
  //! Base class for video input, which will get derived into Camera and MovieInput
  /*! Engine uses a VideoInput to capture input frames and pass them to its currently loaded machine vision Module for
      processing. The VideoInput class is abstract and simply defines the interface. For live video processing, Engine
      will implement its VideoInput as a Camera, and for processing of pre-recorded videos or image sequences (useful to
      debug or tune an algorithm on reproducible image sequences), it can also implement its VideoInout as a
      MovieInput. \ingroup core */
  class VideoInput
  {
    public:
      //! Constructor
      /*! \param devname name of the device, or movie file, or empty
          \param nbufs number of video buffers to use, or 0 for automatic. */
      VideoInput(std::string const & devname, unsigned int const nbufs = 0);

      //! Virtual destructor for save inheritance
      virtual ~VideoInput();

      //! Start streaming
      virtual void streamOn() = 0;

      //! Abort streaming
      /*! This only cancels future get() and done() calls, one should still call streamOff() to turn off streaming. */
      virtual void abortStream() = 0;
      
      //! Stop streaming
      virtual void streamOff() = 0;

      //! Get the next captured buffer
      /*! Throws if we are not streaming or blocks until an image is available (has been captured). The image img should
          have been allocated by the caller and will be filled in by what we receive from the device here. */
      virtual void get(RawImage & img) = 0;

      //! Indicate that user processing is done with an image previously obtained via get()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled.

          \note This also invalidates the image and in particular its pixel buffer! */
      virtual void done(RawImage & img) = 0;;

      //! Get information about a control, throw if unsupported by hardware
      /*! Caller should zero-out qc and then set the id field to the desired control id. See VIDIOC_QUERYCTRL for more
          information. */
      virtual void queryControl(struct v4l2_queryctrl & qc) const = 0;

      //! Get the available menu entry names for a menu-type control, throw if unsupported by hardware
      /*! Caller should zero-out qm and then set the id and index fields to the desired control id and menu item
          index. See VIDIOC_QUERYMENU for more information. */
      virtual void queryMenu(struct v4l2_querymenu & qm) const = 0;
     
      //! Get a control's current value, throw if unsupported by hardware
      /*! This is just a pass-through to VIDIOC_G_CTRL */
      virtual void getControl(struct v4l2_control & ctrl) const = 0;
      
      //! Set a control, throw if the hardware rejects the value
      /*! This is just a pass-through to VIDIOC_S_CTRL */
      virtual void setControl(struct v4l2_control const & ctrl) = 0;

      //! Set the video format and frame rate
      virtual void setFormat(VideoMapping const & m) = 0;

      //! Write a value of one of the camera's registers
      /*! This very low-level access is for development of optimal camera settings only and should not be used in normal
          operation, it can crash your system. */
      virtual void writeRegister(unsigned char reg, unsigned char val) = 0;

      //! Read a value from one of the camera's registers
      /*! This very low-level access is for development of optimal camera settings only and should not be used in normal
          operation, it can crash your system. */
      virtual unsigned char readRegister(unsigned char reg) = 0;

    protected:
      std::string const itsDevName; //!< Our device or movie file name
      unsigned int const itsNbufs;  //!< Our number of buffers
  };
} // namespace jevois
