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

#include <jevois/Core/VideoBuffers.H>
#include <jevois/Image/RawImage.H>

#include <linux/videodev2.h>
#include <mutex>
#include <future>
#include <atomic>

namespace jevois
{
  //! Accessory class to hold all the data associated with a V4L2 camera device
  /*! This class is used internally by jevois::Camera and is of little use to end users. \ingroup core */
  class CameraDevice
  {
    public:
      //! Constructor opens the device, checks its capabilities, starts run thread
      /*! When dummy is true, we will just dequeue and requeue the buffers with no processing. */
      CameraDevice(std::string const & devname, unsigned int const nbufs, bool dummy);

      //! Destructor frees all buffers and closes the device
      ~CameraDevice();

      //! Get our fd; used by Camera to access controls
      int getFd() const;
      
      //! Start streaming
      void streamOn();

      //! Abort streaming
      void abortStream();
      
      //! Stop streaming
      void streamOff();

      //! Get the next captured buffer
      void get(RawImage & img);

      //! Indicate that user processing is done with an image previously obtained via get()
      void done(RawImage & img);

      //! Set the video format and frame rate
      void setFormat(unsigned int const fmt, unsigned int const capw, unsigned int const caph, float const fps,
                     unsigned int const cropw, unsigned int const croph, int preset = -1);

    private:
      std::string const itsDevName; //!< Our device or movie file name
      unsigned int const itsNbufs;  //!< Our number of buffers

      int itsFd = -1; // file descriptor we use to grab frames
      bool itsMplane = false; // true if device uses multiplane V4L2 API
      VideoBuffers * itsBuffers; // video buffers
      struct v4l2_format itsFormat { }; // capture format

      std::atomic<bool> itsStreaming;
      std::future<void> itsRunFuture;
      bool itsFormatOk;
      std::atomic<bool> itsRunning;

      mutable std::condition_variable_any itsOutputCondVar;
      mutable std::timed_mutex itsOutputMtx;
      RawImage itsOutputImage;
      RawImage itsConvertedOutputImage;
      std::vector<size_t> itsDoneIdx;
      float itsFps = 0.0F;

      mutable std::timed_mutex itsMtx;

      void run();
  };

} // namespace jevois
