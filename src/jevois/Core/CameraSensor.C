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
#include <jevois/Core/Camera.H>
#include <jevois/Core/VideoMapping.H>
#include <jevois/Util/Utils.H>

// ####################################################################################################
bool jevois::sensorSupportsFormat(jevois::CameraSensor s, jevois::VideoMapping const & m)
{
  switch (s)
  {
    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::any:
    return true;

#ifdef JEVOIS_PRO
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
    switch (m.cfmt)
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
    if (m.cw <= 1920 && m.ch <= 1080 && m.cfps <= 120.0) return true;
    
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
    switch (m.cfmt)
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
    if (m.cw <= 3840 && m.ch <= 2160 && m.cfps <= 60.0) return true;
    
    return false;

    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ar0234:
    // This sensor supports: BAYER, GREY, YUYV, RGB24, ARGB32, and more. Native size is 1920x1200 at up to
    // 120fps. Sensor also supports 1920x1080, 1280x720 and cropping, but the A311D ISP may have issues with these
    // (frame collision problem as with the imx290 when not at native resolution). Any size (multiple of 4) is supported
    // through cropping & rescaling.
    
    /* Supported formats as reported by the Amlogic camera ISP kernel driver:
       
       Supported format 0 is [32-bit A/XRGB 8-8-8-8] fcc 0x34424752 [RGB4]
       Supported format 1 is [24-bit RGB 8-8-8] fcc 0x33424752 [RGB3]
       Supported format 2 is [Y/CbCr 4:2:0] fcc 0x3231564e [NV12]
       Supported format 3 is [16-bit A/XYUV 4-4-4-4] fcc 0x34343459 [Y444]
       Supported format 4 is [YUYV 4:2:2] fcc 0x56595559 [YUYV]
       Supported format 5 is [UYVY 4:2:2] fcc 0x59565955 [UYVY]
       Supported format 6 is [8-bit Greyscale] fcc 0x59455247 [GREY]
       Supported format 7 is [16-bit Bayer BGBG/GRGR (Exp.)] fcc 0x32525942 [BYR2] */ // FIXME this sensor is GRBG
    switch (m.cfmt)
    {
      // Here is what we support:
    case V4L2_PIX_FMT_SGRBG16:
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
    
    // Any size up to 1920x1200 is supported. Beware that some image processing algorithms may require image width to be
    // a multiple of 8 or 16. Here, we do not impose this constraint as some neural nets do not have it (some even use
    // odd input image sizes). The sensor supports up to 120fps:
    if (m.cw <= 1920 && m.ch <= 1200 && m.cfps <= 120.0) return true;
    
    return false;

#else // JEVOIS_PRO
    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ov9650:

    if (m.wdr != jevois::WDRtype::Linear) return false; // Wide dynamic range not supported by this sensor
    if (m.crop == jevois::CropType::CropScale) return false; // Dual-stream capture not supported by this sensor
    if (m.c2fmt != m.cfmt || m.c2w != m.cw || m.c2h != m.ch) return false; // Non-trivial crop/scale not supported

    // This sensor supports: YUYV, BAYER, RGB565
    //  SXGA (1280 x 1024): up to 15 fps
    //   VGA ( 640 x  480): up to 30 fps
    //   CIF ( 352 x  288): up to 60 fps
    //  QVGA ( 320 x  240): up to 60 fps
    //  QCIF ( 176 x  144): up to 120 fps
    // QQVGA ( 160 x  120): up to 60 fps
    // QQCIF (  88 x   72): up to 120 fps

    if (m.cfmt != V4L2_PIX_FMT_YUYV && m.cfmt != V4L2_PIX_FMT_SRGGB8 && m.cfmt != V4L2_PIX_FMT_RGB565) return false;

    if (m.cw == 1280 && m.ch == 1024) { if (m.cfps <=  15.0F) return true; else return false; }
    if (m.cw ==  640 && m.ch ==  480) { if (m.cfps <=  30.0F) return true; else return false; }
    if (m.cw ==  352 && m.ch ==  288) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  320 && m.ch ==  240) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  176 && m.ch ==  144) { if (m.cfps <= 120.0F) return true; else return false; }
    if (m.cw ==  160 && m.ch ==  120) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==   88 && m.ch ==   72) { if (m.cfps <= 120.0F) return true; else return false; }

    return false;

    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ov2640:

    if (m.wdr != jevois::WDRtype::Linear) return false; // Wide dynamic range not supported by this sensor
    if (m.crop == jevois::CropType::CropScale) return false; // Dual-stream capture not supported by this sensor
    if (m.c2fmt != m.cfmt || m.c2w != m.cw || m.c2h != m.ch) return false; // Non-trivial crop/scale not supported

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

    if (m.cfmt != V4L2_PIX_FMT_YUYV && m.cfmt != V4L2_PIX_FMT_SRGGB8 && m.cfmt != V4L2_PIX_FMT_RGB565) return false;

    if (m.cw == 1600 && m.ch == 1200) { if (m.cfps <=  15.0F) return true; else return false; }
    if (m.cw == 1280 && m.ch == 1024) { if (m.cfps <=  15.0F) return true; else return false; }
    if (m.cw == 1280 && m.ch ==  720) { if (m.cfps <=  15.0F) return true; else return false; }
    if (m.cw == 1024 && m.ch ==  768) { if (m.cfps <=  15.0F) return true; else return false; }
    if (m.cw ==  800 && m.ch ==  600) { if (m.cfps <=  40.0F) return true; else return false; }
    if (m.cw ==  640 && m.ch ==  480) { if (m.cfps <=  40.0F) return true; else return false; }
    if (m.cw ==  352 && m.ch ==  288) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  320 && m.ch ==  240) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  176 && m.ch ==  144) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  160 && m.ch ==  120) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==   88 && m.ch ==   72) { if (m.cfps <=  60.0F) return true; else return false; }

    return false;

    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ov7725:

    if (m.wdr != jevois::WDRtype::Linear) return false; // Wide dynamic range not supported by this sensor
    if (m.crop == jevois::CropType::CropScale) return false; // Dual-stream capture not supported by this sensor
    if (m.c2fmt != m.cfmt || m.c2w != m.cw || m.c2h != m.ch) return false; // Non-trivial crop/scale not supported

    // This sensor supports: YUYV, BAYER, RGB565
    //   VGA ( 640 x  480): up to 60 fps
    //   CIF ( 352 x  288): up to 60 fps
    //  QVGA ( 320 x  240): up to 60 fps
    //  QCIF ( 176 x  144): up to 60 fps
    // QQVGA ( 160 x  120): up to 60 fps
    // QQCIF (  88 x   72): up to 60 fps

    if (m.cfmt != V4L2_PIX_FMT_YUYV && m.cfmt != V4L2_PIX_FMT_SRGGB8 && m.cfmt != V4L2_PIX_FMT_RGB565) return false;

    if (m.cw ==  640 && m.ch ==  480) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  352 && m.ch ==  288) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  320 && m.ch ==  240) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  176 && m.ch ==  144) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==  160 && m.ch ==  120) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw ==   88 && m.ch ==   72) { if (m.cfps <=  60.0F) return true; else return false; }

    return false;

    // ----------------------------------------------------------------------------------------------------
  case jevois::CameraSensor::ar0135:

    if (m.wdr != jevois::WDRtype::Linear) return false; // Wide dynamic range not supported by this sensor
    if (m.crop == jevois::CropType::CropScale) return false; // Dual-stream capture not supported by this sensor
    if (m.c2fmt != m.cfmt || m.c2w != m.cw || m.c2h != m.ch) return false; // Non-trivial crop/scale not supported

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
    if (m.cfmt != V4L2_PIX_FMT_SRGGB8 && m.cfmt != V4L2_PIX_FMT_GREY && m.cfmt != V4L2_PIX_FMT_YUYV) return false;

    if (m.cw == 1280 && m.ch ==  960) { if (m.cfps <=  54.0F) return true; else return false; }
    if (m.cw == 1280 && m.ch ==  720) { if (m.cfps <=  60.0F) return true; else return false; }

    if (m.cw == 640 && m.ch ==  480) { if (m.cfps <=  54.0F) return true; else return false; }
    if (m.cw == 640 && m.ch ==  360) { if (m.cfps <=  60.0F) return true; else return false; }

    if (m.cw == 320 && m.ch ==  240) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw == 320 && m.ch ==  180) { if (m.cfps <=  60.0F) return true; else return false; }

    if (m.cw == 160 && m.ch ==  120) { if (m.cfps <=  60.0F) return true; else return false; }
    if (m.cw == 160 && m.ch ==  90) { if (m.cfps <=  60.0F) return true; else return false; }

    return false;
#endif // JEVOIS_PRO
  }
  return false;
}

// ####################################################################################################
bool jevois::sensorHasIMU(CameraSensor s)
{
  switch (s)
  {
    // These sensors have an ICM20948 IMU:
#ifdef JEVOIS_PRO
  case jevois::CameraSensor::imx290:
  case jevois::CameraSensor::os08a10:
  case jevois::CameraSensor::ar0234:
#else
  case jevois::CameraSensor::ar0135:
#endif
    return true;

    // All other sensors do not have an IMU:
  default:
    return false;
  }
}

// ####################################################################################################
void jevois::sensorPrepareSetFormat(jevois::CameraSensor s, jevois::VideoMapping const & m,
                                    unsigned int & capw, unsigned int & caph, int & preset)
{
  if (jevois::sensorSupportsFormat(s, m) == false) throw std::runtime_error("Requested format not supported by sensor");

  // Keep this code in sync with the kernel drivers:
  switch(s)
  {
#ifdef JEVOIS_PRO
  case jevois::CameraSensor::imx290:
    // Capture always 1080p. Presets:
    // 0: 1080p30
    // 1: 1080p60
    // 2: 1080p120
    capw = 1920; caph = 1080;
    if (m.cfps > 60.0F) preset = 2;
    else if (m.cfps > 30.0F) preset = 1;
    else preset = 0;

    break;

  case jevois::CameraSensor::os08a10:
    // Capture 2160p or 1080p. Presets:
    // 0: 2160p30
    // 1: 2160p60
    // 2: 1080p30
    // 3: 1080p60
    // 4: 1080p30 WDR (DOL_VC)
    if (m.cw > 1920 || m.ch > 1080)
    {
      capw = 3840; caph = 2160;
      if (m.cfps > 30.0F) preset = 1;
      else preset = 0;
    }
    else
    {
      capw = 1920; caph = 1080;
      if (m.cfps > 30.0F) preset = 3;
      else if (m.wdr != jevois::WDRtype::Linear) preset = 4;
      else preset = 2;
    }
    break;

  case jevois::CameraSensor::ar0234:
    // Capture 2160p or 1080p. Presets:
    // 0: 1080p30
    // 1: 1080p60
    // 2: 1080p120
    // 3: 1920x1200p30
    // 4: 1920x1200p60
    // 5: 1920x1200p120
    if (m.ch > 1080)
    {
      capw = 1920; caph = 1200;
      if (m.cfps > 60.0F) preset = 5;
      else if (m.cfps > 30.0F) preset = 4;
      else preset = 3;
    }
    else
    {
      capw = 1920; caph = 1080;
      if (m.cfps > 60.0F) preset = 2;
      else if (m.cfps > 30.0F) preset = 1;
      else preset = 0;
    }
    break;

#endif // JEVOIS_PRO
    
  default:
    // Just capture as specified, sensorSupportsFormat() already removed invalid modes:
    capw = m.cw; caph = m.ch; preset = -1;
  }
}
