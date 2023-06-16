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

#include <jevois/Core/VideoInput.H>
#include <jevois/Core/VideoBuffers.H>

#include <opencv2/videoio.hpp> // for cv::VideoCapture

namespace jevois
{
  //! Movie input, can be used as a replacement for Camera to debug algorithms using a fixed video sequence
  /*! The movie will be looped forever to provide continual input. This is a direct wrapper around the VideoCapture
      class of OpenCV, and it hence supports many different video file formats, and image sequences when the filename
      contains a printf-like %d placeholder for a frame number. See OpenCV documentation for VideoCapture for more
      details.

      Note that the movie frames will be resized to match the dimensions specified by setFormat() and will be converted
      to the pixel type specified in setFormat().  \ingroup core */
  class MovieInput : public VideoInput
  {
    public:
      //! Constructor, opens the movie file
      MovieInput(std::string const & filename, unsigned int const nbufs = 3);

      //! Virtual destructor for safe inheritance
      virtual ~MovieInput();

      //! Start streaming
      void streamOn() override;

      //! Abort streaming
      /*! This only cancels future get() and done() calls, one should still call streamOff() to turn off streaming. */
      void abortStream() override;
      
      //! Stop streaming
      void streamOff() override;

      //! Get the next frame from the video file, possibly looping back to start if end is reached
      void get(RawImage & img) override;

      //! Check whether a second input image scaled by the JeVoisPro Platform ISP is available
      /*! Returns false unless we are on JeVois-Pro Platform and the camera format modifier jevois::CropType::CropScale
          is currently in use. */
      virtual bool hasScaledImage() const override;

      //! Get the next captured ISP-scaled secondary buffer
      /*! On JeVois-Pro Platform only, the camera ISP can output 2 frames: 1) raw from sensor, 2) scaled by ISP. This
          function is to access the ISP scaled frame. Throws if not JeVois-Pro Platform or the camera crop type is not
          jevois::CropType::CropScale. Throws if we are not streaming or blocks until an image is available (has
          been captured). The image img should have been allocated by the caller and will be filled in by what we
          receive from the device here. Default implementation throws. */
      virtual void get2(RawImage & img) override;

      //! Indicate that user processing is done with an image previously obtained via get()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled. This also invalidates the image and in particular its pixel buffer. */
      void done(RawImage & img) override;

      //! Indicate that user processing is done with an image previously obtained via get()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled. This also invalidates the image and in particular its pixel buffer. */
      void done2(RawImage & img) override;

      //! Get information about a control, throw if unsupported by hardware
      /*! In MovieInput, this just throws an std::runtime_error */
      void queryControl(struct v4l2_queryctrl & qc) const override;

      //! Get the available menu entry names for a menu-type control, throw if unsupported by hardware
      /*! In MovieInput, this just throws an std::runtime_error */
      void queryMenu(struct v4l2_querymenu & qm) const override;
     
      //! Get a control's current value, throw if unsupported by hardware
      /*! In MovieInput, this just throws an std::runtime_error */
      void getControl(struct v4l2_control & ctrl) const override;
      
      //! Set a control, throw if the hardware rejects the value
      /*! In MovieInput, this just throws an std::runtime_error */
      void setControl(struct v4l2_control const & ctrl) override;

      //! Set the video format and frame rate
      /*! Video frames read from the input movie file will be rescaled (if necessary) to that format's resolution, and
          will be converted (if necessary) to that format's pixel type. */
      void setFormat(VideoMapping const & m) override;

    protected:
      cv::VideoCapture itsCap; //!< Our OpenCV video capture, works on movie and image files too
      cv::Mat itsRawFrame; //!< Raw OpenCV frame last captured, before any resizing or conversion
      std::shared_ptr<VideoBuf> itsBuf; //!< Our single video buffer for the main frame
      std::shared_ptr<VideoBuf> itsBuf2; //!< Our single video buffer for the second (processing) frame
      VideoMapping itsMapping; //!< Our current video mapping, we resize the input to the mapping's camera dims
  };
  
} // namespace jevois
