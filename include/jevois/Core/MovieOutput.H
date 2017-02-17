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

#include <jevois/Core/VideoOutput.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Types/BoundedBuffer.H>
#include <opencv2/core/version.hpp>
#include <opencv2/videoio.hpp> // for cv::VideoCapture
#include <future>

namespace jevois
{
  //! Video output to a movie file, using OpenCV video encoding
  /*! This video output mode saved output frames to a file (or series of files). It is useful when developing new
      algorithms to check the correctness of generated outputs offline, or to save some documentation/demo movies of a
      module. \ingroup core */
  class MovieOutput : public VideoOutput
  {
    public:
      //! Constructor
      MovieOutput(std::string const & fn);
      
      //! Virtual destructor for safe inheritance
      virtual ~MovieOutput();

      //! Set the video format and frame rate
      virtual void setFormat(VideoMapping const & m) override;

      //! Get a pre-allocated image so that we can fill the pixel data and later send out using send()
      /*! May throw if not buffer is available, i.e., all have been queued to send but have not yet been
          sent. Application code must balance exactly one send() for each get(). */
      virtual void get(RawImage & img) override;
      
      //! Send an image out
      /*! May throw if the format is incorrect or std::overflow_error if we have not yet consumed the previous image. */
      virtual void send(RawImage const & img) override;

      //! Start streaming
      virtual void streamOn() override;

      //! Abort streaming
      /*! This only cancels future get() and send() calls, one should still call streamOff() to turn off streaming. */
      virtual void abortStream() override;
      
      //! Stop streaming
      virtual void streamOff() override;

    protected:
      std::shared_ptr<VideoBuf> itsBuffer; //!< Our single video buffer
      VideoMapping itsMapping; //!< Our current video mapping, we resize the input to the mapping's camera dims

      void run(); //!< Use a thread to encode and save frames
      std::future<void> itsRunFut; //!< Future for our run() thread
      jevois::BoundedBuffer<cv::Mat, jevois::BlockingBehavior::Block,
                            jevois::BlockingBehavior::Block> itsBuf; //!< Buffer of frames to encode and write to file
      std::atomic<bool> itsSaving; //!< True when we are saving to file
      int itsFileNum; //!< File number, gets incremented on each streamOff() to avoid overwriting previous files
      std::atomic<bool> itsRunning; //!< True when our run() thread should keep running
      std::string itsFilename; //!< Current file name to save video to
      std::string itsFilebase; //!< Current file base to save video to
  };
}

