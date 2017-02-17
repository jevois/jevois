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

namespace jevois
{
  //! No-op VideoOutput derivative for when there is no video output
  /*! This class derived from VideoOutput does (almost) nothing and can be used by Engine when no video output is
      desired. \ingroup core */
  class VideoOutputNone : public VideoOutput
  {
    public:
      //! Virtual destructor for safe inheritance
      virtual ~VideoOutputNone();

      //! Set the video format and frame rate
      /*! In VideoOutputNone, this is a no-op. */
      void setFormat(VideoMapping const & m) override;

      //! Get a pre-allocated image so that we can fill the pixel data and later send out using send()
      /*! In VideoOutputNone, this returns a blank buffer. */
      void get(RawImage & img) override;
      
      //! Send an image out
      /*! In VideoOutputNone, this is a no-op. */
      void send(RawImage const & img) override;

      //! Start streaming
      /*! In VideoOutputNone, this is a no-op. */
      void streamOn() override;

      //! Abort streaming
      /*! In VideoOutputNone, this is a no-op. */
      void abortStream() override;
      
      //! Stop streaming
      /*! In VideoOutputNone, this is a no-op. */
      void streamOff() override;

    protected:
      std::shared_ptr<VideoBuf> itsBuffer;
      jevois::RawImage itsImage;
  };
}

