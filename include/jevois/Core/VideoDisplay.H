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
#include <jevois/Core/VideoBuf.H>
#include <jevois/Types/BoundedBuffer.H>
#include <jevois/Image/RawImage.H>
#include <vector>
#include <memory>
#include <atomic>

namespace jevois
{
  //! Video output to local screen
  /*! This class is useful for debugging machine vision code on a desktop computer as opposed to the JeVois
      hardware. Images are simply displayed on the local screen. Engine instantiates a VideoDisplay in place of Gadget
      if the provided Gadget device name is empty. \ingroup core */
  class VideoDisplay : public VideoOutput
  {
    public:
      //! Constructor
      VideoDisplay(char const * displayname, size_t nbufs = 2);
      
      //! Virtual destructor for safe inheritance
      virtual ~VideoDisplay();

      //! Set the video format and frame rate, allocate the buffers
      void setFormat(VideoMapping const & m) override;

      //! Get a pre-allocated image so that we can fill the pixel data and later send out using send()
      void get(RawImage & img) override;
      
      //! Send an image out to display
      void send(RawImage const & img) override;

      //! Start streaming
      void streamOn() override;

      //! Abort streaming
      /*! This only cancels future get() and done() calls, one should still call streamOff() to turn off streaming. */
      void abortStream() override;
      
      //! Stop streaming
      void streamOff() override;

    private:
      std::vector<std::shared_ptr<VideoBuf> > itsBuffers;
      BoundedBuffer<RawImage, BlockingBehavior::Block, BlockingBehavior::Block> itsImageQueue;
      std::string const itsName;
  };
}

