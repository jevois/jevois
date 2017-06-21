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

#include <memory>

// Although not strictly required here, we include videodev.h to bring in the V4L2_PIX_FMT_... definitions and make them
// available to all users of RawImage:
#include <linux/videodev2.h>

namespace jevois
{
  class Engine;
  class VideoBuf;
  /*! \defgroup image Minimalistic support for images in the core JeVois library

      The main purpose of the classes and functions that support images is to allow handling of image buffers whose
      memory is mapped onto hardware (camera or USB drivers) without having to copy the data over. Many other libraries
      and frameworks are available that define more complete image classes and associated image processing and machine
      vision functions. Functions are here provided to allow you to reinterpret the raw image buffers as, e.g., OpenCV
      cv::Mat images without copying the image pixel data. For the purposes of creating demo displays, simple image
      copy, paste, drawing, text, etc into raw YUYV image buffers (which would typically be obtained from the USB driver
      and need to be filled with some pixel data to send over the USB link) are also provided.

      The class that represents an image with a pixel buffer that the camera or USB hardware has direct memory access to
      (see VideoBuf) is called RawImage. To avoid defining pixel types yet again, as most image-related libraries
      already do, in RawImage we just use the definitions provided by Video4Linux2.

      The functions that operate on RawImage are thus mostly intented for two purposes: 1) get pixel data out of
      RawImage and into another format like OpenCV, or from some other format into RawImage; 2) Allow simple drawings of
      lines, circles, rectangles, etc to make simple demo displays directly in the RawImage buffer that will be sent
      over the USB link. */

  //! Helper YUYV colors
  /*! Here we assume little endian (so the chroma is the high byte, luminance is low byte) and we assume that we will
      write the same value to the YU and the YV shorts. This means that all the colors are on the green to magenta
      (purple) axis, with some limited opportunity for variations using the luminance value. \ingroup image */
  namespace yuyv
  {
    static unsigned short const Black = 0x8000;
    static unsigned short const DarkGrey = 0x8050;
    static unsigned short const MedGrey = 0x8080;
    static unsigned short const LightGrey = 0x80a0;
    static unsigned short const White = 0x80ff;

    static unsigned short const DarkGreen = 0x0000;
    static unsigned short const MedGreen = 0x0040;
    static unsigned short const LightGreen = 0x00ff;

    static unsigned short const DarkTeal = 0x7070;
    static unsigned short const MedTeal = 0x7090;
    static unsigned short const LightTeal = 0x70b0;

    static unsigned short const DarkPurple = 0xa030;
    static unsigned short const MedPurple = 0xa050;
    static unsigned short const LightPurple = 0xa080;

    static unsigned short const DarkPink = 0xff00;
    static unsigned short const MedPink = 0xff80;
    static unsigned short const LightPink = 0xffff;
  } // namespace yuyv

  //! Helper RGB565 colors
  /*! We also assume little endian encoding here. These colors are from
      http://stackoverflow.com/questions/13720937/c-defined-16bit-high-color \ingroup image */
  namespace rgb565
  {
    static unsigned short const Black       = 0x0000; //   0,   0,   0
    static unsigned short const Navy        = 0x000F; //   0,   0, 128
    static unsigned short const DarkGreen   = 0x03E0; //   0, 128,   0
    static unsigned short const DarkCyan    = 0x03EF; //   0, 128, 128
    static unsigned short const Maroon      = 0x7800; // 128,   0,   0
    static unsigned short const Purple      = 0x780F; // 128,   0, 128
    static unsigned short const Olive       = 0x7BE0; // 128, 128,   0
    static unsigned short const LightGrey   = 0xC618; // 192, 192, 192
    static unsigned short const DarkGrey    = 0x7BEF; // 128, 128, 128
    static unsigned short const Blue        = 0x001F; //   0,   0, 255
    static unsigned short const Green       = 0x07E0; //   0, 255,   0
    static unsigned short const Cyan        = 0x07FF; //   0, 255, 255
    static unsigned short const Red         = 0xF800; // 255,   0,   0
    static unsigned short const Magenta     = 0xF81F; // 255,   0, 255
    static unsigned short const Yellow      = 0xFFE0; // 255, 255,   0
    static unsigned short const White       = 0xFFFF; // 255, 255, 255
    static unsigned short const Orange      = 0xFD20; // 255, 165,   0
    static unsigned short const GreenYellow = 0xAFE5; // 173, 255,  47
    static unsigned short const Pink        = 0xF81F;
  } // namespace rgb565
  
  //! A raw image as coming from a V4L2 Camera and/or being sent out to a USB Gadget
  /*! The pixel data is allocated and memory mapped by the respective camera or gadget drivers. Because the pixel buffer
      is allocated and managed by the hardware driver, we cannot make deep copies of RawImage, and thus the copy
      constructor and assignment operators will yield images that share the same pixel data. To copy pixels from one
      RawImage to another (e.g., from camera image to USB image), see jevois::rawimage::paste() and other RawImage
      functions. \ingroup image */
  class RawImage
  {
    public:
      //! Default constructor, uninitialized
      RawImage();

      //! Default move constructor
      RawImage(RawImage && other) = default;

      //! Default copy constructor
      RawImage(RawImage const & other) = default;

      //! Default assignment
      RawImage & operator=(RawImage const & other) = default;
        
      //! Construct from an existing VideoBuf and associated params
      RawImage(unsigned int w, unsigned int h, unsigned int f, float fs, std::shared_ptr<VideoBuf> b, size_t bindex);

      //! Invalidate the image by zero'ing out the pointer to pixel buffer and the dims and format
      void invalidate();

      //! Check whether the image has a valid pixel buffer
      bool valid() const;

      //! Clear the pixels to all black
      /*! Black value depends on format. Does not work with MJPEG. Throws if the raw image is not valid() and silently
          does nothing if the raw image has MJPEG pixels (since the raw image buffer will be overwritten by the MJPEG
          compressor anyway). */
      void clear();
      
      //! Require a particular image size and format, issue a fatal error message and throw if no match
      /*! The info string is included in the fatal error message to help identifying which image failed the
          requirement. Typically, you would pass "input" or "output" as info. */
      void require(char const * info, unsigned int w, unsigned int h, unsigned int f) const;
      
      unsigned int width;      //!< Image width in pixels
      unsigned int height;     //!< Image height in pixels
      unsigned int fmt;        //!< Pixel format as a V4L2_PIX_FMT_XXX
      float fps;               //!< Programmed frames/s as given by current video mapping, may not be actual
      std::shared_ptr<VideoBuf> buf; //!< The pixel data buffer
      size_t bufindex; //!< The index of the data buffer in the kernel driver

      //! Helper function to get the number of bytes/pixel given the RawImage pixel format
      unsigned int bytesperpix() const;

      //! Helper function to get the total number of bytes in the RawImage, i.e., width * height * bytesperpix()
      unsigned int bytesize() const;
      
      //! Helper function to check that coords are within image bounds
      bool coordsOk(int x, int y) const;

      //! Shortcut access to pixels, read-write
      template <typename T>
      T * pixelsw();

      //! Shortcut access to pixels, read-only
      template <typename T>
      T const * pixels() const;
  };
} // namespace jevois

// Include implementation details
#include <jevois/Image/details/RawImageImpl.H>
