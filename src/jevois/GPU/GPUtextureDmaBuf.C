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

#ifdef JEVOIS_PLATFORM_PRO

#include <jevois/GPU/GPUtextureDmaBuf.H>
#include <jevois/Util/Utils.H>

#include <linux/videodev2.h>
#include <drm/drm_fourcc.h>

// ####################################################################################################
jevois::GPUtextureDmaBuf::GPUtextureDmaBuf(EGLDisplay display, GLsizei width, GLsizei height,
                                           unsigned int fmt, int dmafd) :
    Width(width), Height(height), Display(display)
{
  GL_CHECK(glGenTextures(1, &Id));

  // Convert V4L2 format to DRM format:
  EGLint drm_fmt;
  switch (fmt)
  {
  case V4L2_PIX_FMT_YUYV: drm_fmt = DRM_FORMAT_YUYV; break;
  case V4L2_PIX_FMT_RGB32: drm_fmt = DRM_FORMAT_XBGR8888; break; // FIXME nasty byte swapping?
  case V4L2_PIX_FMT_RGB565: drm_fmt = DRM_FORMAT_RGB565; break;
  case V4L2_PIX_FMT_BGR24: drm_fmt = DRM_FORMAT_RGB888; break;  // FIXME is ISP sending BGR?
  case V4L2_PIX_FMT_RGB24: drm_fmt = DRM_FORMAT_BGR888; break; // FIXME is ISP sending BGR?
  case V4L2_PIX_FMT_UYVY: drm_fmt = DRM_FORMAT_UYVY; break;

    // Darn, greyscale does not seem to be supported by MALI, eglCreateImageKHR() fails
    //case V4L2_PIX_FMT_GREY: drm_fmt = DRM_FORMAT_R8; break; // FIXME should be Y8
    //case V4L2_PIX_FMT_GREY: drm_fmt = fourcc_code('G','R','E','Y'); break; // missing from drm_fourcc.h, fails too

    // NV12 and YUV444 could be supported but we need 3 dmafd, 3 textures, etc
  case V4L2_PIX_FMT_NV12: //drm_fmt = DRM_FORMAT_NV12; break;
  case V4L2_PIX_FMT_YUV444: //drm_fmt = DRM_FORMAT_YUV444; break;

    // These are not supported: FIXME could add a debayer shader or is one already in MALI?
  case V4L2_PIX_FMT_SRGGB8:
  case V4L2_PIX_FMT_SBGGR16:
  case V4L2_PIX_FMT_MJPEG:
  case 0:
  default: LFATAL("Unsupported pixel format " << jevois::fccstr(fmt));
  }

  // Create an image:
  EGLint attrib_list[] =
    {
     EGL_WIDTH, width,
     EGL_HEIGHT, height,
     EGL_LINUX_DRM_FOURCC_EXT, drm_fmt,
     EGL_DMA_BUF_PLANE0_FD_EXT, dmafd,
     EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
     EGL_DMA_BUF_PLANE0_PITCH_EXT, EGLint(width * jevois::v4l2BytesPerPix(fmt)),
     //EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC601_EXT, // EGL_ITU_REC601_EXT, EGL_ITU_REC709_EXT, EGL_ITU_REC2020_EXT
     //EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_FULL_RANGE_EXT, // EGL_YUV_FULL_RANGE_EXT, EGL_YUV_NARROW_RANGE_EXT
     EGL_NONE,
    };

  Image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attrib_list);
  if (Image == EGL_NO_IMAGE_KHR) LFATAL("eglCreateImageKHR() failed");

  glBindTexture(GL_TEXTURE_EXTERNAL_OES, Id);
  GL_CHECK(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  GL_CHECK(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CHECK(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, Image);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
  
  LDEBUG("Created " << width << 'x' << height << " texture (id=" << Id << ", dmafd=" << dmafd << ')');
}

// ####################################################################################################
jevois::GPUtextureDmaBuf::~GPUtextureDmaBuf()
{
  glDeleteTextures(1, &Id);
  eglDestroyImageKHR(Display, Image);
}

#endif // JEVOIS_PLATFORM_PRO
