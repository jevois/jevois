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

#include <jevois/GPU/GPUtexture.H>

// ####################################################################################################
jevois::GPUtexture::GPUtexture(GLsizei width, GLsizei height, GLenum format, bool createFramebuffer) :
    Width(width), Height(height), Format(format), Id(0), FramebufferId(0), RenderBufferId(0)
{
  GL_CHECK(glGenTextures(1, &Id));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, Id));
  GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, nullptr));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLfloat(GL_NEAREST)));
  GL_CHECK(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLfloat(GL_NEAREST)));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

  if (createFramebuffer)
  {
    // Optional: create framebuffer
    GL_CHECK(glGenFramebuffers(1, &FramebufferId));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, FramebufferId));

    // Optional: create render buffer
	GL_CHECK(glGenRenderbuffers(1, &RenderBufferId));
	GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, RenderBufferId));
	GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, Width, Height));
 	GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, RenderBufferId));

    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Id, 0));

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      LERROR("Framebuffer creation failed");
  }
  
  LDEBUG("Created " << width << 'x' << height << " texture (id=" << Id << ", fb=" << FramebufferId
         << ", rb=" << RenderBufferId << ')');
}

// ####################################################################################################
jevois::GPUtexture::~GPUtexture()
{
  if (RenderBufferId) glDeleteRenderbuffers(1, &RenderBufferId);
  if (FramebufferId) glDeleteFramebuffers(1, &FramebufferId);
  glDeleteTextures(1, &Id);
}

// ####################################################################################################
void jevois::GPUtexture::setPixels(void const * data)
{
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, Id));

  // Try to use the largest possible alignment to speedup the upload. We need the row length to be a multiple of the
  // alignment since opengl will pad each row to that alignment:
  int const rowlen = Width * (Format == GL_RGBA ? 4 : 1);
  int align;
  if ((rowlen & 7) == 0) align = 8;
  else if ((rowlen & 3) == 0) align = 4;
  else if ((rowlen & 1) == 0) align = 2;
  else align = 1;
  
  glPixelStorei(GL_UNPACK_ALIGNMENT, align);
  static void * olddata = nullptr;
  
  if (data != olddata)
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, Format, GL_UNSIGNED_BYTE, data));
  //GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, data));
  GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}

// ####################################################################################################
void jevois::GPUtexture::getPixels(void * data) const
{
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, FramebufferId));
  GL_CHECK(glReadPixels(0, 0, Width, Height, Format, GL_UNSIGNED_BYTE, data));
  GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

