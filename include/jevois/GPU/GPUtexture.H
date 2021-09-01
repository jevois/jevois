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

#include <jevois/GPU/OpenGL.H>

namespace jevois
{
  //! Simple class to hold an OpenGL texture
  /*! OpenGL textures are used to transfer image data from CPU to GPU and then allow the GPU to perform some processing
      on the data. Processing is typically done by using "shader" GPU programs that transform the texture's pixel data
      and render it, in some user-programmable way. */
  class GPUtexture
  {
    public:
      //! Constructor
      /*! format should be as in glTexImage2D(), typically only GL_RGBA or GL_LUMINANCE are supported on embedded
          hardware platforms. */
      GPUtexture(GLsizei width, GLsizei height, GLenum format, bool createFramebuffer);
      
      //! Destructor, frees the texture in OpenGL
      ~GPUtexture();
      
      //! Copy pixel data from memory location to the texture
      /*! data should have been allocated by caller and have the correct size given the textute width, height, and
          format. */
      void setPixels(void const * data);
      
      //! Copy pixel data from the texture to already-allocated memory location
      /*! data should have been allocated by caller and have the correct size given the textute width, height, and
          format. */
      void getPixels(void * data) const;
      
      GLsizei const Width;
      GLsizei const Height;
      GLenum const Format;
      GLuint Id;
      GLuint FramebufferId;
      GLuint RenderBufferId;
  };
} // namespace jevois
