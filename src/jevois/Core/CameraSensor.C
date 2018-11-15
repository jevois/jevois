// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern
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

#include <jevois/Core/CameraSensor.H>
#include <linux/videodev2.h>

// ####################################################################################################
bool jevois::sensorSupportsFormat(jevois::CameraSensor s, unsigned int fmt, unsigned int w, unsigned int h, float fps)
{
  switch (s)
  {
    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::any:
    return true;
    
    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ov9650:
    // This sensor supports: YUYV, BAYER, RGB565
    //  SXGA (1280 x 1024): up to 15 fps
    //   VGA ( 640 x  480): up to 30 fps
    //   CIF ( 352 x  288): up to 60 fps
    //  QVGA ( 320 x  240): up to 60 fps
    //  QCIF ( 176 x  144): up to 120 fps
    // QQVGA ( 160 x  120): up to 60 fps
    // QQCIF (  88 x   72): up to 120 fps

    if (fmt != V4L2_PIX_FMT_YUYV && fmt != V4L2_PIX_FMT_SRGGB8 && fmt != V4L2_PIX_FMT_RGB565) return false;

    if (w == 1280 && h == 1024) { if (fps <=  15.0F) return true; else return false; }
    if (w ==  640 && h ==  480) { if (fps <=  30.0F) return true; else return false; }
    if (w ==  352 && h ==  288) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  320 && h ==  240) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  176 && h ==  144) { if (fps <= 120.0F) return true; else return false; }
    if (w ==  160 && h ==  120) { if (fps <=  60.0F) return true; else return false; }
    if (w ==   88 && h ==   72) { if (fps <= 120.0F) return true; else return false; }

    return false;

    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ov2640:
    // This sensor supports: YUYV, BAYER, RGB565
    //  UXGA (1600 x 1200): up to 15 fps
    //  SXGA (1280 x 1024): up to 15 fps
    //  720p (1280 x  720): up to 15 fps
    //   XGA (1024 x  768): up to 15 fps
    //  SVGA ( 800 x  600): up to 30 fps
    //   VGA ( 640 x  480): up to 30 fps
    //   CIF ( 352 x  288): up to 60 fps
    //  QVGA ( 320 x  240): up to 60 fps
    //  QCIF ( 176 x  144): up to 60 fps
    // QQVGA ( 160 x  120): up to 60 fps
    // QQCIF (  88 x   72): up to 60 fps

    if (fmt != V4L2_PIX_FMT_YUYV && fmt != V4L2_PIX_FMT_SRGGB8 && fmt != V4L2_PIX_FMT_RGB565) return false;

    if (w == 1600 && h == 1200) { if (fps <=  15.0F) return true; else return false; }
    if (w == 1280 && h == 1024) { if (fps <=  15.0F) return true; else return false; }
    if (w == 1280 && h ==  720) { if (fps <=  15.0F) return true; else return false; }
    if (w == 1024 && h ==  768) { if (fps <=  15.0F) return true; else return false; }
    if (w ==  800 && h ==  600) { if (fps <=  30.0F) return true; else return false; }
    if (w ==  640 && h ==  480) { if (fps <=  30.0F) return true; else return false; }
    if (w ==  352 && h ==  288) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  320 && h ==  240) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  176 && h ==  144) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  160 && h ==  120) { if (fps <=  60.0F) return true; else return false; }
    if (w ==   88 && h ==   72) { if (fps <=  60.0F) return true; else return false; }

    return false;

    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ov7725:
    // This sensor supports: YUYV, BAYER, RGB565
    //   VGA ( 640 x  480): up to 60 fps
    //   CIF ( 352 x  288): up to 60 fps
    //  QVGA ( 320 x  240): up to 60 fps
    //  QCIF ( 176 x  144): up to 60 fps
    // QQVGA ( 160 x  120): up to 60 fps
    // QQCIF (  88 x   72): up to 60 fps

    if (fmt != V4L2_PIX_FMT_YUYV && fmt != V4L2_PIX_FMT_SRGGB8 && fmt != V4L2_PIX_FMT_RGB565) return false;

    if (w ==  640 && h ==  480) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  352 && h ==  288) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  320 && h ==  240) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  176 && h ==  144) { if (fps <=  60.0F) return true; else return false; }
    if (w ==  160 && h ==  120) { if (fps <=  60.0F) return true; else return false; }
    if (w ==   88 && h ==   72) { if (fps <=  60.0F) return true; else return false; }

    return false;
    // ----------------------------------------------------------------------------------------------------

  case jevois::CameraSensor::ar0135:
    // This sensor supports: BAYER or MONO
    //  SXGA (1280 x  960): up to 54 fps
    //  720p (1280 x  720): up to 60 fps
    //   VGA ( 640 x  480): up to 54 fps (binned version of SXGA)
    //  360p ( 640 x  360): up to 60 fps
    //  QVGA ( 320 x  240): up to 54 fps (central crop of binned version of SXGA)
    //  180p ( 320 x  180): up to 60 fps
    // QQVGA ( 160 x  120): up to 54 fps (central crop of binned version of SXGA)
    //   90p ( 160 x   90): up to 60 fps

    // We support native Bayer or Mono, and YUYV through Bayer/mono to YUYV conversion in the Camera class:
    if (fmt != V4L2_PIX_FMT_SRGGB8 && fmt != V4L2_PIX_FMT_GREY && fmt != V4L2_PIX_FMT_YUYV) return false;

    if (w == 1280 && h ==  960) { if (fps <=  54.0F) return true; else return false; }
    if (w == 1280 && h ==  720) { if (fps <=  60.0F) return true; else return false; }

    if (w == 640 && h ==  480) { if (fps <=  54.0F) return true; else return false; }
    if (w == 640 && h ==  360) { if (fps <=  60.0F) return true; else return false; }

    if (w == 320 && h ==  240) { if (fps <=  60.0F) return true; else return false; }
    if (w == 320 && h ==  180) { if (fps <=  60.0F) return true; else return false; }

    if (w == 160 && h ==  120) { if (fps <=  60.0F) return true; else return false; }
    if (w == 160 && h ==  90) { if (fps <=  60.0F) return true; else return false; }

    return false;
  }

  return false;
}

