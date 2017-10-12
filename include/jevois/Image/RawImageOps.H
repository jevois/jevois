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

#include <jevois/Image/RawImage.H>

#include <opencv2/core/core.hpp>

namespace jevois
{
  namespace rawimage
  {
    //! Create an OpenCV image from the existing RawImage data, sharing the pixel memory rather than copying it
    /*! The pixel data is not copied, just pointed to, and no pixel format conversion is done. You should destroy the
        cv::Mat before the RawImage to avoid that your cv::Mat ends up with de-allocated pixel memory. \ingroup image */
    cv::Mat cvImage(RawImage const & src);

    //! Convert RawImage to OpenCV doing color conversion from any RawImage source pixel to OpenCV gray byte
    /*! Supported RawImage pixel formats:

        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_BGR24

        \ingroup image */
    cv::Mat convertToCvGray(RawImage const & src);
    
    //! Convert RawImage to OpenCV doing color conversion from any RawImage source pixel to OpenCV BGR byte
    /*! For historical reasons, BGR is the "native" color format of OpenCV, check whether your algorithm needs RGB or
        BGR and use the appropriate conversion for it.

        Supported RawImage pixel formats:

        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_BGR24

        \ingroup image */
    cv::Mat convertToCvBGR(RawImage const & src);

    //! Convert RawImage to OpenCV doing color conversion from any RawImage source pixel to OpenCV RGB byte
    /*! For historical reasons, BGR is the "native" color format of OpenCV, not RGB as created here, check whether your
        algorithm needs RGB or BGR and use the appropriate conversion for it.
        
        Supported RawImage pixel formats:
        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_BGR24

        \ingroup image */
    cv::Mat convertToCvRGB(RawImage const & src);

    //! Convert RawImage to OpenCV doing color conversion from any RawImage source pixel to OpenCV RGB-A byte
    /*! RGBA is seldom used by OpenCV itself, but is useful for many NEON and OpenGL (GPU) algorithms. For these
        algorithms, we here just use cv::Mat as a convenient container for raw pixel data.

        Supported RawImage pixel formats:
        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_BGR24

        \ingroup image */
    cv::Mat convertToCvRGBA(RawImage const & src);

    //! Swap pairs of bytes in a RawImage
    /*! This should never be needed, except maybe for RGB565 images, mainly for internal debugging, or to directly pass
        an RGB565 camera image (which is big endian) to USB (which assumes little endian when using RGBP UVC
        format). This function is NEON-accelerated when compiled for the platform hardware. \ingroup image */
    void byteSwap(RawImage & img);
    
    //! Paste an image within another of same pixel type
    /*! To keep this function fast, throws if the source image does not fully fit within the destination image, or if
        the two images do not have the same pixel format. \ingroup image */
    void paste(RawImage const & src, RawImage & dest, int dx, int dy);

    //! Paste a grey byte image into a YUYV image
    /*! \ingroup image */
    void pasteGreyToYUYV(cv::Mat const & src, RawImage & dest, int dx, int dy);

    //! Paste a BGR byte image into a YUYV image
    /*! \ingroup image */
    void pasteBGRtoYUYV(cv::Mat const & src, RawImage & dst, int dx, int dy);

    //! Paste a RGB byte image into a YUYV image
    /*! \ingroup image */
    void pasteRGBtoYUYV(cv::Mat const & src, RawImage & dst, int dx, int dy);
    
    //! Paste an ROI from an image to within another of same pixel type
    /*! To keep this function fast, throws if the source image does not fully fit within the destination image or if the
        pixel formats differ.  \ingroup image */
    void roipaste(RawImage const & src, int x, int y, unsigned int w, unsigned int h, RawImage & dest, int dx, int dy);

    //! Draw a disk in a YUYV image
    /*! \ingroup image */
    void drawDisk(RawImage & img, int x, int y, unsigned int rad, unsigned int col);

    //! Draw a circle in a YUYV image
    /*! \ingroup image */
    void drawCircle(RawImage & img, int x, int y, unsigned int rad, unsigned int thick, unsigned int col);

    //! Draw a line in a YUYV image
    /*! \ingroup image */
    void drawLine(RawImage & img, int x1, int y1, int x2, int y2, unsigned int thick, unsigned int col);
    
    //! Draw a rectangle in a YUYV image
    /*! The pixels drawn go from (x,y) included to (x+w-1,y+h-1) included. If w or h is 0, it is replaced by 1.
        \ingroup image */
    void drawRect(RawImage & img, int x, int y, unsigned int w, unsigned int h, unsigned int thick, unsigned int col);

    //! Draw a rectangle in a YUYV image
    /*! This is really not flexible and is optimized for speed only. col will be used for both YU and YV pixels. For
        more demanding drawings, use openCV functions. The pixels drawn go from (x,y) included to (x+w-1,y+h-1)
        included. If w or h is 0, it is replaced by 1. \ingroup image */
    void drawRect(RawImage & img, int x, int y, unsigned int w, unsigned int h, unsigned int col);

    //! Draw a filled rectangle in a YUYV image
    /*! This is really not flexible and is optimized for speed only. col will be used for both YU and YV pixels. For
        more demanding drawings, use openCV functions. The pixels drawn go from (x,y) included to (x+w-1,y+h-1)
        included. If w or h is 0, it is replaced by 1. \ingroup image */
    void drawFilledRect(RawImage & img, int x, int y, unsigned int w, unsigned int h, unsigned int col);

    //! Available fonts for writeText()
    /*! \ingroup image */
    enum Font
    {
      Font5x7,
      Font6x10,
      Font7x13,
      Font8x13bold,
      Font9x15bold,
      Font10x20,
      Font11x22,
      Font12x22,
      Font14x26,
      Font15x28,
      Font16x29,
      Font20x38,
    };
    
    //! Write some text in an image
    /*! \ingroup image */
    void writeText(RawImage & img, std::string const & txt, int x, int y, unsigned int col, Font font = Font6x10);

    //! Write some text in an image
    /*! \ingroup image */
    void writeText(RawImage & img, char const * txt, int x, int y, unsigned int col, Font font = Font6x10);

    //! Convert a BGR cv::Mat to RawImage with already-allocated pixels and pixel type
    /*! This is used to output video frames over USB from internally computed results in opencv BGR format.  dst should
        already have an allocated pixel buffer (typically, it should be obtained from the USB gadget), and its size
        must match that of the src image.
        
        Supported dst pixel formats:
        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_MJPG
        - V4L2_PIX_FMT_BGR24
        
        quality is used only when dst is MJPG and should be between 1 and 100. \ingroup image */
    void convertCvBGRtoRawImage(cv::Mat const & src, RawImage & dst, int quality);

    //! Convert a RGB cv::Mat to RawImage with already-allocated pixels and pixel type
    /*! This is used to output video frames over USB from internally computed results in opencv BGR format.  dst should
        already have an allocated pixel buffer (typically, it should be obtained from the USB gadget), and its size
        must match that of the src image.
        
        Supported dst pixel formats:
        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_MJPG
        - V4L2_PIX_FMT_BGR24
        
        quality is used only when dst is MJPG and should be between 1 and 100. \ingroup image */
    void convertCvRGBtoRawImage(cv::Mat const & src, RawImage & dst, int quality);
    
    //! Convert an RGBA cv::Mat to RawImage with already-allocated pixels and pixel type
    /*! This is used to output video frames over USB from internally computed results in opencv RGB-A format (which is
        often the format of choice for NEON or OpenGL processing).  dst should already have an allocated pixel buffer
        (typically, it should be obtained from the USB gadget), and its size must match that of the src image.
        
        Supported dst pixel formats:
        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_MJPG
        - V4L2_PIX_FMT_BGR24
        
        quality is used only when dst is MJPG and should be between 1 and 100. \ingroup image */
    void convertCvRGBAtoRawImage(cv::Mat const & src, RawImage & dst, int quality);
    
    //! Convert a Gray cv::Mat to RawImage with already-allocated pixels and pixel type
    /*! This is used to output video frames over USB from internally computed results in opencv Gray format.  dst should
        already have an allocated pixel buffer (typically, it should be obtained from the USB gadget), and its size
        must match that of the src image.
        
        Supported dst pixel formats:
        - V4L2_PIX_FMT_YUYV
        - V4L2_PIX_FMT_GREY
        - V4L2_PIX_FMT_SRGGB8 (Bayer)
        - V4L2_PIX_FMT_RGB565
        - V4L2_PIX_FMT_MJPG
        - V4L2_PIX_FMT_BGR24
        
        quality is used only when dst is MJPG and should be between 1 and 100. \ingroup image */
    void convertCvGRAYtoRawImage(cv::Mat const & src, RawImage & dst, int quality);
    
    //! Split an RGBA cv::Mat into a 4x taller grey RawImage with already-allocated pixels
    /*! This is used to run GPU shaders that apply different filters to an image and store their results separately into
        the R, G, B and A components of an RGBA output image. Here, we just unpack the 4 components and stuff them into
        the output image one above the other: R on top, then G below it, then B below it, then A below it. The
        destination RawImage must be at least as wide as src and at least 4 times as tall as src, and must have
        V4L2_PIX_FMT_GREY pixels. \ingroup image */
    void unpackCvRGBAtoGrayRawImage(cv::Mat const & src, RawImage & dst);

    //! Flip a YUYV RawImage horizontally while preserving color information
    /*! This function is to allow one to use JeVois with the PhotoBooth app on a Mac, whih flips the image horizontally
        (about the vertical axis). Only YUYV pixels are supported. You might be able to use cv::flip() from OpenCV
        instead with other pixel types. */
    void hFlipYUYV(RawImage & img);

    //! OpenCV does not provide conversion from RGB to YUYV in cvtColor(), so this function provides it
    void convertCvRGBtoCvYUYV(cv::Mat const & src, cv::Mat & dst);

    //! OpenCV does not provide conversion from BGR to YUYV in cvtColor(), so this function provides it
    void convertCvBGRtoCvYUYV(cv::Mat const & src, cv::Mat & dst);

    //! OpenCV does not provide conversion from GRAY to YUYV in cvtColor(), so this function provides it
    void convertCvGRAYtoCvYUYV(cv::Mat const & src, cv::Mat & dst);

    //! OpenCV does not provide conversion from RGBA to YUYV in cvtColor(), so this function provides it
    void convertCvRGBAtoCvYUYV(cv::Mat const & src, cv::Mat & dst);

  } // namespace rawimage
} // namespace jevois

