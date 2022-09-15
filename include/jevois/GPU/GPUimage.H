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
#include <jevois/Core/InputFrame.H>
#include <imgui.h>
#include <glm/fwd.hpp>
#include <opencv2/core/core.hpp>

namespace jevois
{
  class GPUtexture;
  class GPUprogram;
#ifdef JEVOIS_PLATFORM_PRO
  class GPUtextureDmaBuf;
#endif
  
  //! Class to hold a GPUtexture, GPUprogram, and other data associated with rendering an image in OpenGL
  /*! OpenGL should have been properly setup before this class is used. \ingroup gpu */
  class GPUimage
  {
    public:
      //! Constructor
      GPUimage();

      //! Destructor
      ~GPUimage();

      //! Set pixel data from a vanilla RawImage, pixel data will be copied to texture
      /*! The caller must guarantee that the pixel data will not be de-allocated until after draw() is called. */
      void set(RawImage const & img);

      //! Set pixel data from a vanilla OpenCV Mat, pixel data will be copied to texture
      /*! If image had three or four 8-bit channels, interpret as RGB[A] if rgb is true, otherwise BGR[A]. If two 8-bit
          channels, interpret at YUYV. If one, interpret at GRAY. The caller must guarantee that the pixel data will not
          be de-allocated until after draw() is called. */
      void set(cv::Mat const & img, bool rgb);

      //! Set pixel data from camera input frame, sharing data with camera kernel driver using zero-copy
      /*! The caller must guarantee that the pixel data will not be de-allocated (e.g., by calling done() on the
          InputFrame) until after draw() is called. Zero-copy requires DMABUF sharing between V4L2 and OpenGL-ES and is
          only available on JeVois-Pro platform. On other hardware configs, this falls back to set(RawImage) using the
          RawImage from the InputFrame. */
      void set(InputFrame const & frame, EGLDisplay display);

      //! Set pixel data from camera input second (scaled) frame, sharing data with camera kernel driver using zero-copy
      /*! The caller must guarantee that the pixel data will not be de-allocated (e.g., by calling done() on the
          InputFrame) until after draw() is called. Zero-copy requires DMABUF sharing between V4L2 and OpenGL-ES and is
          only available on JeVois-Pro platform. On other hardware configs, this falls back to set(RawImage) using the
          RawImage from the InputFrame. Throws unless we are JeVois-Pro Platform and the camera is set to CropScale
          mode. */
      void set2(InputFrame const & frame, EGLDisplay display);

      //! Draw to OpenGL
      /*! If w=0 or h=0 then the image will be rescaled to fill the display as much as possible without changing the
          aspect ratio, and the actually used x,y,w,h will be returned. Otherwise, x,y,w,h are not modified. Throws if
          set() has not been previously called. If noalias is specified, the scaling factor will be rounded down to the
          nearest integer to prevent aliasing in the display. This may reduce the displayed image size. For example,
          with a 1920x1080 window, a 640x480 image would be letterboxed to 1440x1080 when noalias is false. But that is
          a scaling factor of 2.25 which may create rendering aliasing. When noalias is true, the letterboxed image size
          will be 1280x960 (scale factor of 2.0). The matrix pvm is the projection-view-model matrix to use for
          rendering. */
      void draw(int & x, int & y, unsigned short & w, unsigned short & h, bool noalias, glm::mat4 const & pvm);

      //! Convert coordinates of a point from within a rendered image to on-screen
      /*! Throws if draw() has not been previously called. */
      ImVec2 i2d(ImVec2 const & p);
      
      //! Convert a 2D size from within a rendered image to on-screen
      /*! Throws if draw() has not been previously called. */
      ImVec2 i2ds(ImVec2 const & p);
      
      //! Convert coordinates of a point from on-screen to within a rendered image
      /*! Throws if draw() has not been previously called. */
      ImVec2 d2i(ImVec2 const & p);
      
      //! Convert a 2D size from on-screen to within a rendered image
      /*! Throws if draw() has not been previously called. */
      ImVec2 d2is(ImVec2 const & p);
      
    protected:
      void setInternal(unsigned int width, unsigned int height, unsigned int fmt, unsigned char const * data);
      
      EGLDisplay itsDisplay;
      std::shared_ptr<GPUtexture> itsTexture;
      std::shared_ptr<GPUprogram> itsProgram;
#ifdef JEVOIS_PLATFORM_PRO
      void setWithDmaBuf(jevois::RawImage const & img, int dmafd, EGLDisplay display);
      std::shared_ptr<GPUtextureDmaBuf> itsTextureDmaBuf;
#endif
      GLuint itsLocation = 0;
      GLuint itsVertexArray = 0;
      GLuint itsVertexBuffers[2] = { };

      int itsDrawX = -1234567, itsDrawY = -6712345;
      unsigned int itsDrawWidth = 0, itsDrawHeight = 0;
      unsigned int itsTextureWidth = 0, itsTextureHeight = 0;
      unsigned int itsFormat = 0;
      int itsGLtextureWidth = 0;
      GLenum itsGLtextureFmt = 0;
  };
  
} // namespace jevois

#endif // JEVOIS_PRO

