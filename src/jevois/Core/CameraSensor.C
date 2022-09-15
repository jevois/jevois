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
    //  SVGA ( 800 x  600): up to 40 fps
    //   VGA ( 640 x  480): up to 40 fps
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
    if (w ==  800 && h ==  600) { if (fps <=  40.0F) return true; else return false; }
    if (w ==  640 && h ==  480) { if (fps <=  40.0F) return true; else return false; }
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
    
    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::imx290:
    // This sensor supports: BAYER, GREY, YUYV, RGB24, ARGB32, and more. Native size is 1920x1080 at up to
    // 120fps. Sensor also supports 1280x720 and cropping from 1920x1080, but the A311D ISP has issues with these (frame
    // collision as soon as capture height is not 1080). So we always capture at 1920x1080 and use the ISP to
    // crop/rescale to any other resolution. Any size (multiple of 4) is supported through cropping & rescaling.
    
    /* Supported formats as reported by the Amlogic camera ISP kernel driver:
       
       Supported format 0 is [32-bit A/XRGB 8-8-8-8] fcc 0x34424752 [RGB4]
       Supported format 1 is [24-bit RGB 8-8-8] fcc 0x33424752 [RGB3]
       Supported format 2 is [Y/CbCr 4:2:0] fcc 0x3231564e [NV12]
       Supported format 3 is [16-bit A/XYUV 4-4-4-4] fcc 0x34343459 [Y444]
       Supported format 4 is [YUYV 4:2:2] fcc 0x56595559 [YUYV]
       Supported format 5 is [UYVY 4:2:2] fcc 0x59565955 [UYVY]
       Supported format 6 is [8-bit Greyscale] fcc 0x59455247 [GREY]
       Supported format 7 is [16-bit Bayer BGBG/GRGR (Exp.)] fcc 0x32525942 [BYR2] */
    switch (fmt)
    {
      // Here is what we support:
    case V4L2_PIX_FMT_SBGGR16:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_GREY:
      //case V4L2_PIX_FMT_NV12: not supported yet, uses 3 planes: 12-bit (8 for Y, 8 for UV half size) Y/CbCr 4:2:0
      //case V4L2_PIX_FMT_YUV444: not supported: 16-bit xxxxyyyy uuuuvvvv
      //case V4L2_PIX_FMT_UYVY: not supported to avoid confusions with YUYV
      break;
      // Everything else is unsupported:
    default: return false;
    }
    
    // Any size up to 1080P is supported. Beware that some image processing algorithms may require image width to be a
    // multiple of 8 or 16. Here, we do not impose this constraint as some neural nets do not have it (some even use odd
    // input image sizes). The sensor supports up to 120fps:
    if (w <= 1920 && h <= 1080 && fps <= 120.0) return true;
    
    return false;
    
    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::os08a10:
    // This sensor supports: BAYER, GREY, YUYV, RGB24, ARGB32, and more. Native size is 3840x2160 at up to 60fps. Sensor
    // also supports 1920x1080, 1280x720 and cropping, but the A311D ISP may have issues with these (frame collision
    // problem as with the imx290 when not at native resolution). Any size (multiple of 4) is supported through cropping
    // & rescaling.
    
    /* Supported formats as reported by the Amlogic camera ISP kernel driver:
       
       Supported format 0 is [32-bit A/XRGB 8-8-8-8] fcc 0x34424752 [RGB4]
       Supported format 1 is [24-bit RGB 8-8-8] fcc 0x33424752 [RGB3]
       Supported format 2 is [Y/CbCr 4:2:0] fcc 0x3231564e [NV12]
       Supported format 3 is [16-bit A/XYUV 4-4-4-4] fcc 0x34343459 [Y444]
       Supported format 4 is [YUYV 4:2:2] fcc 0x56595559 [YUYV]
       Supported format 5 is [UYVY 4:2:2] fcc 0x59565955 [UYVY]
       Supported format 6 is [8-bit Greyscale] fcc 0x59455247 [GREY]
       Supported format 7 is [16-bit Bayer BGBG/GRGR (Exp.)] fcc 0x32525942 [BYR2] */
    switch (fmt)
    {
      // Here is what we support:
    case V4L2_PIX_FMT_SBGGR16:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_GREY:
      //case V4L2_PIX_FMT_NV12: not supported yet, uses 3 planes: 12-bit (8 for Y, 8 for UV half size) Y/CbCr 4:2:0
      //case V4L2_PIX_FMT_YUV444: not supported: 16-bit xxxxyyyy uuuuvvvv
      //case V4L2_PIX_FMT_UYVY: not supported to avoid confusions with YUYV
      break;
      // Everything else is unsupported:
    default: return false;
    }
    
    // Any size up to 4k is supported. Beware that some image processing algorithms may require image width to be a
    // multiple of 8 or 16. Here, we do not impose this constraint as some neural nets do not have it (some even use odd
    // input image sizes). The sensor supports up to 60fps:
    if (w <= 3840 && h <= 2160 && fps <= 60.0) return true;
    
    return false;
  }

  return false;
}

// ####################################################################################################
bool jevois::sensorHasIMU(CameraSensor s)
{
  switch (s)
  {
    // These sensors have an ICM20948 IMU:
  case jevois::CameraSensor::ar0135:
  case jevois::CameraSensor::imx290:
  case jevois::CameraSensor::os08a10:
    return true;

    // All other sensors do not have an IMU:
  default:
    return false;
  }
}
