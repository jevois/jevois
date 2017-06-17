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

#include <jevois/Types/Singleton.H>
#include <jevois/Image/RawImage.H>
#include <opencv2/core/core.hpp>

namespace jevois
{
  /*! \defgroup jpeg Jpeg-related classes and functions
    
      Jpeg classes and functions, mainly used to support sending MJPEG video output over USB from raw uncompressed
      images captured by a camera.

      \ingroup image */
  
  /*! @{ */ // **********************************************************************

  //! Helper to convert from packed YUYV to planar YUV422
  /*! Memory must have been allocated by caller. */
  void convertYUYVtoYUV422(unsigned char const * src, int width, int height, unsigned char * dst);

  //! Simple singleton wrapper over a turbojpeg compressor
  /*! Most users should not need to use this class, compressBRGtoJpeg() uses it internally to avoid re-creating the
      turbojpeg compressor object on each video frame. */
  class JpegCompressor : public Singleton<JpegCompressor>
  {
    public:
      //! Constructor, create the turbojpeg object
      JpegCompressor();
      
      //! Destructor, frees the turbojpeg object
      virtual ~JpegCompressor();

      //! Access the compressor handle
      void * compressor();
      
    private:
      void * itsCompressor;
  };
  
  //! Compress raw pixel buffer to jpeg
  /*! The compressed size is returned. The dst buffer should have been allocated by caller, with size at least width *
      height * 2 bytes. quality should be between 1 (worst) and 100 (best). */
  unsigned long compressBGRtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                  int quality = 75);

  //! Compress a BGR cv::Mat into an output JPEG jevois::RawImage
  /*! The dst RawImage should have an allocated buffer, typically this is intended for use with a RawImage that was
      obtained from the UVC gadget. */
  void compressBGRtoJpeg(cv::Mat const & src, RawImage & dst, int quality = 75);

  //! Compress raw pixel buffer to jpeg
  /*! The compressed size is returned. The dst buffer should have been allocated by caller, with size at least width *
      height * 2 bytes. quality should be between 1 (worst) and 100 (best). */
  unsigned long compressRGBtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                  int quality = 75);

  //! Compress a RGB cv::Mat into an output JPEG jevois::RawImage
  /*! The dst RawImage should have an allocated buffer, typically this is intended for use with a RawImage that was
      obtained from the UVC gadget. */
  void compressRGBtoJpeg(cv::Mat const & src, RawImage & dst, int quality = 75);

  //! Compress raw pixel buffer to jpeg
  /*! The compressed size is returned. The dst buffer should have been allocated by caller, with size at least width *
      height * 2 bytes. quality should be between 1 (worst) and 100 (best). */
  unsigned long compressRGBAtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                   int quality = 75);

  //! Compress an RGBA cv::Mat into an output JPEG jevois::RawImage
  /*! The dst RawImage should have an allocated buffer, typically this is intended for use with a RawImage that was
      obtained from the UVC gadget. */
  void compressRGBAtoJpeg(cv::Mat const & src, RawImage & dst, int quality = 75);

  //! Compress raw pixel buffer to jpeg
  /*! The compressed size is returned. The dst buffer should have been allocated by caller, with size at least width *
      height * 2 bytes. quality should be between 1 (worst) and 100 (best). */
  unsigned long compressGRAYtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                   int quality = 75);
  //! Compress a Gray cv::Mat into an output JPEG jevois::RawImage
  /*! The dst RawImage should have an allocated buffer, typically this is intended for use with a RawImage that was
      obtained from the UVC gadget. */
  void compressGRAYtoJpeg(cv::Mat const & src, RawImage & dst, int quality = 75);

  /*! @} */ // **********************************************************************

} // namespace jevois
