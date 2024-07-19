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
  /*! \defgroup image Raw zero-copy / memory-mapped video images and support functions

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
    unsigned short constexpr Black = 0x8000;     //!< YUYV color value
    unsigned short constexpr DarkGrey = 0x8050;  //!< YUYV color value
    unsigned short constexpr MedGrey = 0x8080;   //!< YUYV color value
    unsigned short constexpr LightGrey = 0x80a0; //!< YUYV color value
    unsigned short constexpr White = 0x80ff;     //!< YUYV color value

    unsigned short constexpr DarkGreen = 0x0000;  //!< YUYV color value
    unsigned short constexpr MedGreen = 0x0040;   //!< YUYV color value
    unsigned short constexpr LightGreen = 0x00ff; //!< YUYV color value

    unsigned short constexpr DarkTeal = 0x7070;  //!< YUYV color value
    unsigned short constexpr MedTeal = 0x7090;   //!< YUYV color value
    unsigned short constexpr LightTeal = 0x70b0; //!< YUYV color value

    unsigned short constexpr DarkPurple = 0xa030;  //!< YUYV color value
    unsigned short constexpr MedPurple = 0xa050;   //!< YUYV color value
    unsigned short constexpr LightPurple = 0xa080; //!< YUYV color value

    unsigned short constexpr DarkPink = 0xff00;  //!< YUYV color value
    unsigned short constexpr MedPink = 0xff80;   //!< YUYV color value
    unsigned short constexpr LightPink = 0xffff; //!< YUYV color value
  } // namespace yuyv

  //! Helper RGB565 colors
  /*! We assume little endian encoding here. These colors are from
      http://stackoverflow.com/questions/13720937/c-defined-16bit-high-color \ingroup image */
  namespace rgb565
  {
    unsigned short constexpr Black       = 0x0000; //!< RGB565 value for:   0,   0,   0
    unsigned short constexpr Navy        = 0x000F; //!< RGB565 value for:   0,   0, 128
    unsigned short constexpr DarkGreen   = 0x03E0; //!< RGB565 value for:   0, 128,   0
    unsigned short constexpr DarkCyan    = 0x03EF; //!< RGB565 value for:   0, 128, 128
    unsigned short constexpr Maroon      = 0x7800; //!< RGB565 value for: 128,   0,   0
    unsigned short constexpr Purple      = 0x780F; //!< RGB565 value for: 128,   0, 128
    unsigned short constexpr Olive       = 0x7BE0; //!< RGB565 value for: 128, 128,   0
    unsigned short constexpr LightGrey   = 0xC618; //!< RGB565 value for: 192, 192, 192
    unsigned short constexpr DarkGrey    = 0x7BEF; //!< RGB565 value for: 128, 128, 128
    unsigned short constexpr Blue        = 0x001F; //!< RGB565 value for:   0,   0, 255
    unsigned short constexpr Green       = 0x07E0; //!< RGB565 value for:   0, 255,   0
    unsigned short constexpr Cyan        = 0x07FF; //!< RGB565 value for:   0, 255, 255
    unsigned short constexpr Red         = 0xF800; //!< RGB565 value for: 255,   0,   0
    unsigned short constexpr Magenta     = 0xF81F; //!< RGB565 value for: 255,   0, 255
    unsigned short constexpr Yellow      = 0xFFE0; //!< RGB565 value for: 255, 255,   0
    unsigned short constexpr White       = 0xFFFF; //!< RGB565 value for: 255, 255, 255
    unsigned short constexpr Orange      = 0xFD20; //!< RGB565 value for: 255, 165,   0
    unsigned short constexpr GreenYellow = 0xAFE5; //!< RGB565 value for: 173, 255,  47
    unsigned short constexpr Pink        = 0xF618; //!< RGB565 value for: F4 C2 C2
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
