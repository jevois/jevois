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

#include <jevois/Image/Jpeg.H>
#include <turbojpeg.h>
#include <stddef.h> // for size_t

// ####################################################################################################
jevois::JpegCompressor::JpegCompressor()
{ itsCompressor = tjInitCompress(); }

// ####################################################################################################
jevois::JpegCompressor::~JpegCompressor()
{ tjDestroy(itsCompressor); }

// ####################################################################################################
void * jevois::JpegCompressor::compressor()
{ return itsCompressor; }

// ####################################################################################################
void jevois::convertYUYVtoYUV422(unsigned char const * src, int width, int height, unsigned char * dst)
{
  size_t const sz = width * height;
  unsigned char * uptr = dst + sz;
  unsigned char * vptr = uptr + sz / 4;
  size_t const sz2 = sz / 2;
  
  for (size_t i = 0; i < sz2; ++i)
  {
    *dst++ = *src++;
    *uptr++ = *src++;
    *dst++ = *src++;
    *vptr++ = *src++;
  }
}

// ####################################################################################################
unsigned long jevois::compressBGRtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                        int quality)
{
  unsigned long jpegsize = width * height * 2; // allocated output buffer size

  tjhandle compressor = jevois::JpegCompressor::instance().compressor();
  
  tjCompress2(compressor, const_cast<unsigned char *>(src), width, 0, height, TJPF_BGR,
              &dst, &jpegsize, TJSAMP_422, quality, TJFLAG_FASTDCT);

  return jpegsize;
}

// ####################################################################################################
unsigned long jevois::compressRGBAtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                         int quality)
{
  unsigned long jpegsize = width * height * 2; // allocated output buffer size

  tjhandle compressor = jevois::JpegCompressor::instance().compressor();
  
  tjCompress2(compressor, const_cast<unsigned char *>(src), width, 0, height, TJPF_RGBA,
              &dst, &jpegsize, TJSAMP_422, quality, TJFLAG_FASTDCT);

  return jpegsize;
}

// ####################################################################################################
unsigned long jevois::compressGRAYtoJpeg(unsigned char const * src, int width, int height, unsigned char * dst,
                                         int quality)
{
  unsigned long jpegsize = width * height * 2; // allocated output buffer size

  tjhandle compressor = jevois::JpegCompressor::instance().compressor();
  
  tjCompress2(compressor, const_cast<unsigned char *>(src), width, 0, height, TJPF_GRAY,
              &dst, &jpegsize, TJSAMP_422, quality, TJFLAG_FASTDCT);

  return jpegsize;
}

// ####################################################################################################
void jevois::compressBGRtoJpeg(cv::Mat const & src, RawImage & dst, int quality)
{
  dst.buf->setBytesUsed(jevois::compressBGRtoJpeg(src.data, src.cols, src.rows, dst.pixelsw<unsigned char>(), quality));
}

// ####################################################################################################
void jevois::compressRGBAtoJpeg(cv::Mat const & src, RawImage & dst, int quality)
{
  dst.buf->setBytesUsed(jevois::compressRGBAtoJpeg(src.data, src.cols,src.rows, dst.pixelsw<unsigned char>(), quality));
}

// ####################################################################################################
void jevois::compressGRAYtoJpeg(cv::Mat const & src, RawImage & dst, int quality)
{
  dst.buf->setBytesUsed(jevois::compressGRAYtoJpeg(src.data, src.cols,src.rows, dst.pixelsw<unsigned char>(), quality));
}
