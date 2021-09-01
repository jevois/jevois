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

// This is only available on JeVoisPro
#ifdef JEVOIS_PRO

#include <jevois/Core/VideoOutput.H>
#include <jevois/Core/VideoBuf.H>
#include <jevois/Types/BoundedBuffer.H>
#include <jevois/GPU/GPUimage.H>
#include <jevois/GPU/VideoDisplayBackendX11.H>
#include <jevois/GPU/VideoDisplayBackendMALI.H>
#include <vector>
#include <atomic>

namespace jevois
{
  //! Video output to local screen
  /*! This class provides accelerated OpenGL-ES display on JeVois-Pro platform. It only works on JeVois-Pro and creates
      fatal errors on other host or platform configs. Images are simply displayed on the local screen using
      OpenGL-ES.

      Here is the basic theory of operation:

      - When running on host, we open an X11 window; on platform, we instead use fbdev.
      - When the first video frame arrives, we then initialize OpenGL and create 2 triangles that will be rendered in
        the whole window. The reason for this is to avoid threading issues (so OpenGL is initialized in the same thread
        as rendering calls will be made). We also initialize a texture of the same size as the incoming image. We
        finally create a surface of same size as the window or fbdev. We then load a compile shaders to render our
        texture into our surface.
      - On every frame, we just update the texture's pixel location and render the scene.

      \ingroup core */
  class VideoDisplayGL : public VideoOutput
  {
    public:
      //! Constructor
      VideoDisplayGL(size_t nbufs = 2);
      
      //! Virtual destructor for safe inheritance
      virtual ~VideoDisplayGL();

      //! Set the video format and frame rate, allocate the buffers
      virtual void setFormat(VideoMapping const & m) override;

      //! Get a pre-allocated image so that we can fill the pixel data and later send out using send()
      virtual void get(RawImage & img) override;
      
      //! Send an image out to display
      virtual void send(RawImage const & img) override;

      //! Start streaming
      virtual void streamOn() override;

      //! Abort streaming
      /*! This only cancels future get() and done() calls, one should still call streamOff() to turn off streaming. */
      virtual void abortStream() override;
      
      //! Stop streaming
      virtual void streamOff() override;

    protected:
      std::vector<std::shared_ptr<VideoBuf> > itsBuffers;
      BoundedBuffer<RawImage, BlockingBehavior::Block, BlockingBehavior::Block> itsImageQueue;
      GPUimage itsImage;
      
#ifdef JEVOIS_PLATFORM_PRO
      VideoDisplayBackendMALI itsBackend;
#else
      VideoDisplayBackendX11 itsBackend;
#endif
       std::atomic<bool> itsStreaming;
 };
}

#endif //  JEVOIS_PRO
