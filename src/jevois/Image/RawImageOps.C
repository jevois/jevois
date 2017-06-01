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

#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/VideoBuf.H>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>
#include <jevois/Image/Jpeg.H>
#include <future>

#include <linux/videodev2.h>
#include <cmath>
#include <opencv2/imgproc/imgproc.hpp>

// ####################################################################################################
cv::Mat jevois::rawimage::cvImage(jevois::RawImage const & src)
{
  unsigned int bpp = jevois::v4l2BytesPerPix(src.fmt);

  switch (bpp)
  {
  case 3: return cv::Mat(src.height, src.width, CV_8UC3, src.buf->data());
  case 2: return cv::Mat(src.height, src.width, CV_8UC2, src.buf->data());
  case 1: return cv::Mat(src.height, src.width, CV_8UC1, src.buf->data());
  default: LFATAL("Unsupported RawImage format");
  }
}

// ####################################################################################################
namespace
{
  inline void rgb565pixrgb(unsigned short rgb565, unsigned char & r, unsigned char & g, unsigned char & b)
  {
    r = ((((rgb565 >> 11) & 0x1F) * 527) + 23) >> 6;
    g = ((((rgb565 >> 5) & 0x3F) * 259) + 33) >> 6;
    b = (((rgb565 & 0x1F) * 527) + 23) >> 6;
   }
  
  class rgb565ToGray : public cv::ParallelLoopBody
  {
    public:
      rgb565ToGray(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 2; // 2 bytes/pix for RGB565
        outlinesize = outw * 1; // 1 byte/pix for Gray
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; ++i)
          {
            int const in = inoff + i * 2;
            int const out = outoff + i;
            unsigned short const rgb565 = ((unsigned short)(inImg.data[in + 0]) << 8) | inImg.data[in + 1];
            unsigned char r, g, b;
            rgb565pixrgb(rgb565, r, g, b);
            int lum = int(r + g + b) / 3;
            outImg[out] = lum;
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  class rgb565ToBGR : public cv::ParallelLoopBody
  {
    public:
      rgb565ToBGR(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 2; // 2 bytes/pix for RGB565
        outlinesize = outw * 3; // 3 bytes/pix for BGR
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; ++i)
          {
            int const in = inoff + i * 2;
            int const out = outoff + i * 3;
            unsigned short const rgb565 = ((unsigned short)(inImg.data[in + 0]) << 8) | inImg.data[in + 1];

            unsigned char r, g, b; rgb565pixrgb(rgb565, r, g, b);
            outImg[out + 0] = b;
            outImg[out + 1] = g;
            outImg[out + 2] = r;
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  class rgb565ToRGB : public cv::ParallelLoopBody
  {
    public:
      rgb565ToRGB(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 2; // 2 bytes/pix for RGB565
        outlinesize = outw * 3; // 3 bytes/pix for RGB
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; ++i)
          {
            int const in = inoff + i * 2;
            int const out = outoff + i * 3;
            unsigned short const rgb565 = ((unsigned short)(inImg.data[in + 0]) << 8) | inImg.data[in + 1];

            unsigned char r, g, b; rgb565pixrgb(rgb565, r, g, b);
            outImg[out + 0] = r;
            outImg[out + 1] = g;
            outImg[out + 2] = b;
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };
  
  class rgb565ToRGBA : public cv::ParallelLoopBody
  {
    public:
      rgb565ToRGBA(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 2; // 2 bytes/pix for RGB565
        outlinesize = outw * 4; // 4 bytes/pix for RGBA
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; ++i)
          {
            int const in = inoff + i * 2;
            int const out = outoff + i * 4;
            unsigned short const rgb565 = ((unsigned short)(inImg.data[in + 0]) << 8) | inImg.data[in + 1];

            unsigned char r, g, b; rgb565pixrgb(rgb565, r, g, b);
            outImg[out + 0] = r;
            outImg[out + 1] = g;
            outImg[out + 2] = b;
            outImg[out + 3] = (unsigned char)(255);
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };
} // anonymous namespace

// ####################################################################################################
cv::Mat jevois::rawimage::convertToCvGray(jevois::RawImage const & src)
{
  cv::Mat rawimgcv = jevois::rawimage::cvImage(src);
  cv::Mat result;
  
  switch (src.fmt)
  {
  case V4L2_PIX_FMT_YUYV: cv::cvtColor(rawimgcv, result, CV_YUV2GRAY_YUYV); return result;
  case V4L2_PIX_FMT_GREY: return rawimgcv;
  case V4L2_PIX_FMT_SRGGB8: cv::cvtColor(rawimgcv, result, CV_BayerBG2GRAY); return result;

  case V4L2_PIX_FMT_RGB565: // camera outputs big-endian pixels, cv::cvtColor() assumes little-endian
    result = cv::Mat(cv::Size(src.width, src.height), CV_8UC1);
    cv::parallel_for_(cv::Range(0, src.height), rgb565ToGray(rawimgcv, result.data, result.cols));
    return result;

  case V4L2_PIX_FMT_MJPEG: LFATAL("MJPEG not supported");
  case V4L2_PIX_FMT_BGR24: cv::cvtColor(rawimgcv, result, CV_BGR2GRAY); return result;
  }
   LFATAL("Unknown RawImage pixel format");
}

// ####################################################################################################
cv::Mat jevois::rawimage::convertToCvBGR(jevois::RawImage const & src)
{
  cv::Mat rawimgcv = jevois::rawimage::cvImage(src);
  cv::Mat result;
  
  switch (src.fmt)
  {
  case V4L2_PIX_FMT_YUYV: cv::cvtColor(rawimgcv, result, CV_YUV2BGR_YUYV); return result;
  case V4L2_PIX_FMT_GREY: cv::cvtColor(rawimgcv, result, CV_GRAY2BGR); return result;
  case V4L2_PIX_FMT_SRGGB8: cv::cvtColor(rawimgcv, result, CV_BayerBG2BGR); return result;

  case V4L2_PIX_FMT_RGB565: // camera outputs big-endian pixels, cv::cvtColor() assumes little-endian
    result = cv::Mat(cv::Size(src.width, src.height), CV_8UC3);
    cv::parallel_for_(cv::Range(0, src.height), rgb565ToBGR(rawimgcv, result.data, result.cols));
    return result;

  case V4L2_PIX_FMT_MJPEG: LFATAL("MJPEG not supported");
  case V4L2_PIX_FMT_BGR24: return rawimgcv;
  }
  LFATAL("Unknown RawImage pixel format");
}

// ####################################################################################################
cv::Mat jevois::rawimage::convertToCvRGB(jevois::RawImage const & src)
{
  cv::Mat rawimgcv = jevois::rawimage::cvImage(src);
  cv::Mat result;
  
  switch (src.fmt)
  {
  case V4L2_PIX_FMT_YUYV: cv::cvtColor(rawimgcv, result, CV_YUV2RGB_YUYV); return result;
  case V4L2_PIX_FMT_GREY: cv::cvtColor(rawimgcv, result, CV_GRAY2RGB); return result;
  case V4L2_PIX_FMT_SRGGB8: cv::cvtColor(rawimgcv, result, CV_BayerBG2RGB); return result;

  case V4L2_PIX_FMT_RGB565: // camera outputs big-endian pixels, cv::cvtColor() assumes little-endian
    result = cv::Mat(cv::Size(src.width, src.height), CV_8UC3);
    cv::parallel_for_(cv::Range(0, src.height), rgb565ToRGB(rawimgcv, result.data, result.cols));
    return result;

  case V4L2_PIX_FMT_MJPEG: LFATAL("MJPEG not supported");
  case V4L2_PIX_FMT_BGR24: cv::cvtColor(rawimgcv, result, CV_BGR2RGB); return result;
  }
  LFATAL("Unknown RawImage pixel format");
}

// ####################################################################################################
cv::Mat jevois::rawimage::convertToCvRGBA(jevois::RawImage const & src)
{
  cv::Mat rawimgcv = jevois::rawimage::cvImage(src);
  cv::Mat result;
  
  switch (src.fmt)
  {
  case V4L2_PIX_FMT_YUYV: cv::cvtColor(rawimgcv, result, CV_YUV2RGBA_YUYV); return result;

  case V4L2_PIX_FMT_GREY: cv::cvtColor(rawimgcv, result, CV_GRAY2RGBA); return result;

  case V4L2_PIX_FMT_SRGGB8:
  {
    // FIXME: we do two conversions, should get a hold of the opencv source for bayer conversions and make an RGBA
    // version of it:
    cv::Mat fixme;
    cv::cvtColor(rawimgcv, fixme, CV_BayerBG2RGB);
    cv::cvtColor(fixme, result, CV_RGB2RGBA);
    return result;
  }
  
  case V4L2_PIX_FMT_RGB565: // camera outputs big-endian pixels, cv::cvtColor() assumes little-endian
    result = cv::Mat(cv::Size(src.width, src.height), CV_8UC4);
    cv::parallel_for_(cv::Range(0, src.height), rgb565ToRGBA(rawimgcv, result.data, result.cols));
    return result;

  case V4L2_PIX_FMT_MJPEG: LFATAL("MJPEG not supported");

  case V4L2_PIX_FMT_BGR24: cv::cvtColor(rawimgcv, result, CV_BGR2RGBA); return result;
  }
  LFATAL("Unknown RawImage pixel format");
}

// ####################################################################################################
void jevois::rawimage::byteSwap(jevois::RawImage & img)
{
  if (img.bytesperpix() != 2) LFATAL("Can only byteswap images with 2 bytes/pixel");

#ifdef JEVOIS_PLATFORM
  // Use neon acceleration, in parallel threads:
  unsigned int ncores = std::min(4U, std::thread::hardware_concurrency());
  size_t const nbc = img.bytesize() / ncores;
  unsigned char * ptr = img.pixelsw<unsigned char>();

  // FIXME check for possible size rounding problems
  
  // Launch ncores-1 threads and we will do the last chunk in the current thread:
  std::vector<std::future<void> > fut;
  for (unsigned int core = 0; core < ncores-1; ++core)
    fut.push_back(std::async(std::launch::async, [&ptr, &nbc](int core) -> void {
          unsigned char * cptr = ptr + core * nbc;
          for (size_t i = 0; i < nbc; i += 16) vst1q_u8(cptr + i, vrev16q_u8(vld1q_u8(cptr + i)));
        }, core));
  
  // Last chunk:
  size_t const sz = img.bytesize();
  for (size_t i = (ncores-1) * nbc; i < sz; i += 16) vst1q_u8(ptr + i, vrev16q_u8(vld1q_u8(ptr + i)));

  // Wait for all the threads to complete:
  for (auto & f : fut) f.get();
#else
  // Use CPU:
  size_t const sz = img.width * img.height; // size in shorts
  unsigned short * ptr = img.pixelsw<unsigned short>();
  for (size_t i = 0; i < sz; ++i) ptr[i] = __builtin_bswap16(ptr[i]);
#endif
}

// ####################################################################################################
void jevois::rawimage::paste(jevois::RawImage const & src, jevois::RawImage & dest, int x, int y)
{
  if (src.fmt != dest.fmt) LFATAL("src and dest must have the same pixel format");
  if (x < 0 || y < 0 || x + src.width > dest.width || y + src.height > dest.height)
    LFATAL("src does not fit within dest");

  unsigned int const bpp = src.bytesperpix();

  unsigned char const * sptr = src.pixels<unsigned char>();
  unsigned char * dptr = dest.pixelsw<unsigned char>() + (x + y * dest.width) * bpp;
  size_t const srclinelen = src.width * bpp;
  size_t const dstlinelen = dest.width * bpp;
  
  for (unsigned int j = 0; j < src.height; ++j)
  {
    memcpy(dptr, sptr, srclinelen);
    sptr += srclinelen;
    dptr += dstlinelen;
  }
}

// ####################################################################################################
void jevois::rawimage::roipaste(jevois::RawImage const & src, int x, int y, unsigned int w, unsigned int h,
                                jevois::RawImage & dest, int dx, int dy)
{
  if (src.fmt != dest.fmt) LFATAL("src and dest must have the same pixel format");
  if (x < 0 || y < 0 || x + w > src.width || y + h > src.height) LFATAL("roi not within source image");
  if (dx < 0 || dy < 0 || dx + w > dest.width || dy + h > dest.height) LFATAL("roi not within dest image");

  unsigned int const bpp = src.bytesperpix();

  unsigned char const * sptr = src.pixels<unsigned char>() + (x + y * src.width) * bpp;
  unsigned char * dptr = dest.pixelsw<unsigned char>() + (dx + dy * dest.width) * bpp;
  size_t const srclinelen = src.width * bpp;
  size_t const dstlinelen = dest.width * bpp;
  
  for (unsigned int j = 0; j < h; ++j)
  {
    memcpy(dptr, sptr, w * bpp);
    sptr += srclinelen;
    dptr += dstlinelen;
  }
}

// ####################################################################################################
void jevois::rawimage::pasteGreyToYUYV(cv::Mat const & src, jevois::RawImage & dest, int x, int y)
{
  if (x + src.cols > int(dest.width) || y + src.rows > int(dest.height)) LFATAL("src does not fit within dest");
  unsigned int const bpp = dest.bytesperpix();

  unsigned char const * sptr = src.data;
  unsigned char * dptr = dest.pixelsw<unsigned char>() + (x + y * dest.width) * bpp;
  size_t const dststride = (dest.width - src.cols) * bpp;
  
  for (int j = 0; j < src.rows; ++j)
  {
    for (int i = 0; i < src.cols; ++i) { *dptr++ = *sptr++; *dptr++ = 0x80; }
    dptr += dststride;
  }
}

// ####################################################################################################
void jevois::rawimage::drawDisk(jevois::RawImage & img, int cx, int cy, unsigned int rad, unsigned int col)
{
  // From the iLab Neuromorphic Vision C++ Toolkit
  unsigned short * const dptr = img.pixelsw<unsigned short>();
  int const w = int(img.width);

  if (rad == 0) { if (img.coordsOk(cx, cy)) dptr[cx + w * cy] = col; return; }

  int const intrad = rad;
  for (int y = -intrad; y <= intrad; ++y)
  {
    int bound = int(std::sqrt(float(intrad * intrad - y * y)));
    for (int x = -bound; x <= bound; ++x)
      if (img.coordsOk(x + cx, y + cy)) dptr[x + cx + w * (y + cy)] = col;
  }
}

// ####################################################################################################
void jevois::rawimage::drawCircle(jevois::RawImage & img, int cx, int cy, unsigned int rad,
                                  unsigned int thick, unsigned int col)
{
  // From the iLab Neuromorphic Vision C++ Toolkit
  if (rad == 0) { jevois::rawimage::drawDisk(img, cx, cy, thick, col); return; }

  jevois::rawimage::drawDisk(img, cx - rad, cy, thick, col);
  jevois::rawimage::drawDisk(img, cx + rad, cy, thick, col);
  int bound1 = rad, bound2;

  for (unsigned int dy = 1; dy <= rad; ++dy)
  {
    bound2 = bound1;
    bound1 = int(0.4999F + sqrtf(rad*rad - dy*dy));
    for (int dx = bound1; dx <= bound2; ++dx)
    {
      jevois::rawimage::drawDisk(img, cx - dx, cy - dy, thick, col);
      jevois::rawimage::drawDisk(img, cx + dx, cy - dy, thick, col);
      jevois::rawimage::drawDisk(img, cx + dx, cy + dy, thick, col);
      jevois::rawimage::drawDisk(img, cx - dx, cy + dy, thick, col);
    }
  }
}

// ####################################################################################################
void jevois::rawimage::drawLine(jevois::RawImage & img, int x1, int y1, int x2, int y2, unsigned int thick,
                                unsigned int col)
{
  // From the iLab Neuromorphic Vision C++ Toolkit
  // from Graphics Gems / Paul Heckbert
  int const dx = x2 - x1; int const ax = std::abs(dx) << 1; int const sx = dx < 0 ? -1 : 1;
  int const dy = y2 - y1; int const ay = std::abs(dy) << 1; int const sy = dy < 0 ? -1 : 1;
  int const w = img.width; int const h = img.height;
  int x = x1, y = y1;

  if (ax > ay)
  {
    int d = ay - (ax >> 1);
    for (;;)
    {
      if (x >= 0 && x < w && y >= 0 && y < h) jevois::rawimage::drawDisk(img, x, y, thick, col);

      if (x == x2) return;
      if (d >= 0) { y += sy; d -= ax; }
      x += sx; d += ay;
    }
  }
  else
  {
    int d = ax - (ay >> 1);
    for (;;)
    {
      if (x >= 0 && x < w && y >= 0 && y < h) jevois::rawimage::drawDisk(img, x, y, thick, col);
      if (y == y2) return;
      if (d >= 0) { x += sx; d -= ay; }
      y += sy; d += ax;
    }
  }
}

// ####################################################################################################
void jevois::rawimage::drawRect(jevois::RawImage & img, int x, int y, unsigned int w, unsigned int h,
                                unsigned int thick, unsigned int col)
{
  if (thick == 0)
    jevois::rawimage::drawRect(img, x, y, w, h, col);
  else
  {
    jevois::rawimage::drawLine(img, x, y, x+w, y, thick, col);
    jevois::rawimage::drawLine(img, x, y+h, x+w, y+h, thick, col);
    jevois::rawimage::drawLine(img, x, y, x, y+h, thick, col);
    jevois::rawimage::drawLine(img, x+w, y, x+w, y+h, thick, col);
  }
}
// ####################################################################################################
void jevois::rawimage::drawRect(jevois::RawImage & img, int x, int y, unsigned int w, unsigned int h,
                                unsigned int col)
{
  if (x > int(img.width)) x = img.width;
  if (y > int(img.height)) y = img.height;
  if (x + w > img.width) w = img.width - x;
  if (y + h > img.height) h = img.height - y;

  unsigned int const imgw = img.width;
  unsigned short * b = img.pixelsw<unsigned short>() + x + y * imgw;

  // Two horizontal lines:
  unsigned int const offy = (h-1) * imgw;
  for (unsigned int xx = 0; xx < w; ++xx) { b[xx] = col; b[xx + offy] = col; }

  // Two vertical lines:
  unsigned int const offx = w-1;
  for (unsigned int yy = 0; yy < h * imgw; yy += imgw) { b[yy] = col; b[yy + offx] = col; }
}
// ####################################################################################################
void jevois::rawimage::drawFilledRect(jevois::RawImage & img, int x, int y, unsigned int w, unsigned int h,
                                      unsigned int col)
{
  if (x > int(img.width)) x = img.width;
  if (y > int(img.height)) y = img.height;
  if (x + w > img.width) w = img.width - x;
  if (y + h > img.height) h = img.height - y;
  
  unsigned int const stride = img.width - w;
  unsigned short * b = img.pixelsw<unsigned short>() + x + y * img.width;

  for (unsigned int yy = 0; yy < h; ++yy)
  {
    for (unsigned int xx = 0; xx < w; ++xx) *b++ = col;
    b += stride;
  }
}

// ####################################################################################################
// Font pattern definitions:
namespace jevois
{
  namespace font
  {
    extern const unsigned char font10x20[95][200];
    extern const unsigned char font11x22[95][242];
    extern const unsigned char font12x22[95][264];
    extern const unsigned char font14x26[95][364];
    extern const unsigned char font15x28[95][420];
    extern const unsigned char font16x29[95][464];
    extern const unsigned char font20x38[95][760];
    extern const unsigned char font5x7[95][35];
    extern const unsigned char font6x10[95][60];
    extern const unsigned char font7x13[95][91];
    extern const unsigned char font8x13bold[95][104];
    extern const unsigned char font9x15bold[95][135];
  } // namespace font
} // namespace jevois


// ####################################################################################################
void jevois::rawimage::writeText(jevois::RawImage & img, std::string const & txt, int x, int y, unsigned int col,
                                 jevois::rawimage::Font font)
{
  jevois::rawimage::writeText(img, txt.c_str(), x, y, col, font);
}

// ####################################################################################################
void jevois::rawimage::writeText(jevois::RawImage & img, char const * txt, int x, int y, unsigned int col,
                                 jevois::rawimage::Font font)
{
  int len = int(strlen(txt));
  unsigned int const imgw = img.width;

  int fontw, fonth; unsigned char const * fontptr;
  switch (font)
  {
  case Font5x7:      fontw =  5; fonth =  7; fontptr = &jevois::font::font5x7[0][0]; break;
  case Font6x10:     fontw =  6; fonth = 10; fontptr = &jevois::font::font6x10[0][0]; break;
  case Font7x13:     fontw =  7; fonth = 13; fontptr = &jevois::font::font7x13[0][0]; break;
  case Font8x13bold: fontw =  8; fonth = 13; fontptr = &jevois::font::font8x13bold[0][0]; break;
  case Font9x15bold: fontw =  9; fonth = 15; fontptr = &jevois::font::font9x15bold[0][0]; break;
  case Font10x20:    fontw = 10; fonth = 20; fontptr = &jevois::font::font10x20[0][0]; break;
  case Font11x22:    fontw = 11; fonth = 22; fontptr = &jevois::font::font11x22[0][0]; break;
  case Font12x22:    fontw = 12; fonth = 22; fontptr = &jevois::font::font12x22[0][0]; break;
  case Font14x26:    fontw = 14; fonth = 26; fontptr = &jevois::font::font14x26[0][0]; break;
  case Font15x28:    fontw = 15; fonth = 28; fontptr = &jevois::font::font15x28[0][0]; break;
  case Font16x29:    fontw = 16; fonth = 29; fontptr = &jevois::font::font16x29[0][0]; break;
  case Font20x38:    fontw = 20; fonth = 38; fontptr = &jevois::font::font20x38[0][0]; break;
  default: LFATAL("Invalid font");
  }
  
  // Clip the text so that it does not go outside the image:
  while (x + len * fontw > int(imgw)) { --len; if (len <= 0) return; }

  // Be nice and handle various pixel formats:
  switch (img.bytesperpix())
  {
  case 2:
  {
    unsigned short * b = img.pixelsw<unsigned short>() + x + y * imgw;

    for (int i = 0; i < len; ++i)
    {
      int idx = txt[i] - 32; if (idx >= 95) idx = 0;
      unsigned char const * ptr = fontptr + fontw * fonth * idx;
      unsigned short * bb = b;
      for (int yy = 0; yy < fonth; ++yy)
      {
        // Draw one line of this letter, note the transparent background:
        for (int xx = 0; xx < fontw; ++xx) if (*ptr++) ++bb; else *bb++ = col;
        bb += imgw - fontw;
      }
      b += fontw;
    }
  }
  break;

  case 1:
  {
    unsigned char * b = img.pixelsw<unsigned char>() + x + y * imgw;
    
    for (int i = 0; i < len; ++i)
    {
      int idx = txt[i] - 32; if (idx >= 95) idx = 0;
      unsigned char const * ptr = fontptr + fontw * fonth * idx;
      unsigned char * bb = b;
      for (int yy = 0; yy < fonth; ++yy)
      {
        // Draw one line of this letter, note the transparent background:
        for (int xx = 0; xx < fontw; ++xx) if (*ptr++) ++bb; else *bb++ = col;
        bb += imgw - fontw;
      }
      b += fontw;
    }
  }
  break;
  
  default:
    LFATAL("Sorry, only 1 and 2 bytes/pixel images are supported for now");
  }
}

// ####################################################################################################
namespace
{
  class bgrToBayer : public cv::ParallelLoopBody
  {
    public:
      bgrToBayer(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 3; // 3 bytes/pix for BGR
        outlinesize = outw * 1; // 1 byte/pix for Bayer
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; i += 2)
          {
            int const in = inoff + i * 3;
            int const out = outoff + i;

            if ( (j & 1) == 0) { outImg[out + 0] = inImg.data[in + 2]; outImg[out + 1] = inImg.data[in + 4]; }
            else { outImg[out + 0] = inImg.data[in + 1]; outImg[out + 1] = inImg.data[in + 3]; }
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  // ####################################################################################################
  void convertCvBGRtoBayer(cv::Mat const & src, jevois::RawImage & dst)
  {
    if (src.type() != CV_8UC3) LFATAL("src must have type CV_8UC3 and BGR pixels");
    if (dst.fmt != V4L2_PIX_FMT_SRGGB8) LFATAL("dst format must be V4L2_PIX_FMT_SRGGB8");
    if (int(dst.width) != src.cols || int(dst.height) != src.rows) LFATAL("src and dst dims must match");
    
    cv::parallel_for_(cv::Range(0, src.rows), bgrToBayer(src, dst.pixelsw<unsigned char>(), dst.width));
  }
} // anonymous namespace

// ####################################################################################################
namespace
{
  class grayToBayer : public cv::ParallelLoopBody
  {
    public:
      grayToBayer(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 1; // 1 bytes/pix for GRAY
        outlinesize = outw * 1; // 1 byte/pix for Bayer
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          memcpy(&outImg[outoff], &inImg.data[inoff], inlinesize);
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  // ####################################################################################################
  void convertCvGRAYtoBayer(cv::Mat const & src, jevois::RawImage & dst)
  {
    if (src.type() != CV_8UC1) LFATAL("src must have type CV_8UC1 and GRAY pixels");
    if (dst.fmt != V4L2_PIX_FMT_SRGGB8) LFATAL("dst format must be V4L2_PIX_FMT_SRGGB8");
    if (int(dst.width) != src.cols || int(dst.height) != src.rows) LFATAL("src and dst dims must match");
    
    cv::parallel_for_(cv::Range(0, src.rows), grayToBayer(src, dst.pixelsw<unsigned char>(), dst.width));
  }
} // anonymous namespace

// ####################################################################################################
namespace
{
  class rgbaToBayer : public cv::ParallelLoopBody
  {
    public:
      rgbaToBayer(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 4; // 4 bytes/pix for RGBA
        outlinesize = outw * 1; // 1 byte/pix for Bayer
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; i += 2)
          {
            int const in = inoff + i * 4;
            int const out = outoff + i;

            if ( (j & 1) == 0) { outImg[out + 0] = inImg.data[in + 0]; outImg[out + 1] = inImg.data[in + 5]; }
            else { outImg[out + 0] = inImg.data[in + 1]; outImg[out + 1] = inImg.data[in + 6]; }
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  // ####################################################################################################
  void convertCvRGBAtoBayer(cv::Mat const & src, jevois::RawImage & dst)
  {
    if (src.type() != CV_8UC4) LFATAL("src must have type CV_8UC4 and RGBA pixels");
    if (dst.fmt != V4L2_PIX_FMT_SRGGB8) LFATAL("dst format must be V4L2_PIX_FMT_SRGGB8");
    if (int(dst.width) != src.cols || int(dst.height) != src.rows) LFATAL("src and dst dims must match");
    
    cv::parallel_for_(cv::Range(0, src.rows), rgbaToBayer(src, dst.pixelsw<unsigned char>(), dst.width));
  }
} // anonymous namespace

// ####################################################################################################
namespace
{
  class bgrToYUYV : public cv::ParallelLoopBody
  {
    public:
      bgrToYUYV(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 3; // 3 bytes/pix for BGR
        outlinesize = outw * 2; // 2 bytes/pix for YUYV
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; i += 2)
          {
            int mc = inoff + i * 3;
            unsigned char const B1 = inImg.data[mc + 0];
            unsigned char const G1 = inImg.data[mc + 1];
            unsigned char const R1 = inImg.data[mc + 2];
            unsigned char const B2 = inImg.data[mc + 3];
            unsigned char const G2 = inImg.data[mc + 4];
            unsigned char const R2 = inImg.data[mc + 5];

            float const Y1 = (0.257F * R1) + (0.504F * G1) + (0.098F * B1) + 16.0F;
            //float const V1 = (0.439F * R1) - (0.368F * G1) - (0.071F * B1) + 128.0F;
            float const U1 = -(0.148F * R1) - (0.291F * G1) + (0.439F * B1) + 128.0F;
            float const Y2 = (0.257F * R2) + (0.504F * G2) + (0.098F * B2) + 16.0F;
            float const V2 = (0.439F * R2) - (0.368F * G2) - (0.071F * B2) + 128.0F;
            //float const U2 = -(0.148F * R2) - (0.291F * G2) + (0.439F * B2) + 128.0F;
           
            mc = outoff + i * 2;
            outImg[mc + 0] = Y1;
            outImg[mc + 1] = U1;
            outImg[mc + 2] = Y2;
            outImg[mc + 3] = V2;
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  // ####################################################################################################
  void convertCvBGRtoYUYV(cv::Mat const & src, jevois::RawImage & dst)
  {
    if (src.type() != CV_8UC3) LFATAL("src must have type CV_8UC3 and BGR pixels");
    if (dst.fmt != V4L2_PIX_FMT_YUYV) LFATAL("dst format must be V4L2_PIX_FMT_YUYV");
    if (int(dst.width) != src.cols || int(dst.height) < src.rows) LFATAL("src and dst dims must match");

    cv::parallel_for_(cv::Range(0, src.rows), bgrToYUYV(src, dst.pixelsw<unsigned char>(), dst.width));
  }
} // anonymous namespace

// ####################################################################################################
namespace
{
  class grayToYUYV : public cv::ParallelLoopBody
  {
    public:
      grayToYUYV(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 1; // 1 bytes/pix for GRAY
        outlinesize = outw * 2; // 2 bytes/pix for YUYV
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; ++i)
          {
            int mc = inoff + i;
            unsigned char const G = inImg.data[mc + 0];

            mc = outoff + i * 2;
            outImg[mc + 0] = G;
            outImg[mc + 1] = 0x80;
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  // ####################################################################################################
  void convertCvGRAYtoYUYV(cv::Mat const & src, jevois::RawImage & dst)
  {
    if (src.type() != CV_8UC3) LFATAL("src must have type CV_8UC3 and GRAY pixels");
    if (dst.fmt != V4L2_PIX_FMT_YUYV) LFATAL("dst format must be V4L2_PIX_FMT_YUYV");
    if (int(dst.width) != src.cols || int(dst.height) < src.rows) LFATAL("src and dst dims must match");

    cv::parallel_for_(cv::Range(0, src.rows), grayToYUYV(src, dst.pixelsw<unsigned char>(), dst.width));
  }
} // anonymous namespace

// ####################################################################################################
namespace
{
  class rgbaToYUYV : public cv::ParallelLoopBody
  {
    public:
      rgbaToYUYV(cv::Mat const & inputImage,  unsigned char * outImage, size_t outw) :
          inImg(inputImage), outImg(outImage)
      {
        inlinesize = inputImage.cols * 4; // 4 bytes/pix for RGBA
        outlinesize = outw * 2; // 2 bytes/pix for YUYV
      }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const inoff = j * inlinesize;
          int const outoff = j * outlinesize;

          for (int i = 0; i < inImg.cols; i += 2)
          {
            int mc = inoff + i * 4;
            unsigned char const R1 = inImg.data[mc + 0];
            unsigned char const G1 = inImg.data[mc + 1];
            unsigned char const B1 = inImg.data[mc + 2];
            unsigned char const R2 = inImg.data[mc + 4];
            unsigned char const G2 = inImg.data[mc + 5];
            unsigned char const B2 = inImg.data[mc + 6];

            float const Y1 = (0.257F * R1) + (0.504F * G1) + (0.098F * B1) + 16.0F;
            float const U1 = -(0.148F * R1) - (0.291F * G1) + (0.439F * B1) + 128.0F;
            float const Y2 = (0.257F * R2) + (0.504F * G2) + (0.098F * B2) + 16.0F;
            float const V2 = (0.439F * R2) - (0.368F * G2) - (0.071F * B2) + 128.0F;
           
            mc = outoff + i * 2;
            outImg[mc + 0] = Y1;
            outImg[mc + 1] = U1;
            outImg[mc + 2] = Y2;
            outImg[mc + 3] = V2;
          }
        }
      }

    private:
      cv::Mat const & inImg;
      unsigned char * outImg;
      int inlinesize, outlinesize;
  };

  // ####################################################################################################
  void convertCvRGBAtoYUYV(cv::Mat const & src, jevois::RawImage & dst)
  {
    if (src.type() != CV_8UC4) LFATAL("src must have type CV_8UC4 and RGBA pixels");
    if (dst.fmt != V4L2_PIX_FMT_YUYV) LFATAL("dst format must be V4L2_PIX_FMT_YUYV");
    if (int(dst.width) != src.cols || int(dst.height) != src.rows) LFATAL("src and dst dims must match");

    cv::parallel_for_(cv::Range(0, src.rows), rgbaToYUYV(src, dst.pixelsw<unsigned char>(), dst.width));
  }
} // anonymous namespace

// ####################################################################################################
void jevois::rawimage::convertCvBGRtoRawImage(cv::Mat const & src, RawImage & dst, int quality)
{
  if (src.type() != CV_8UC3) LFATAL("src must have type CV_8UC3 and BGR pixels");
  if (int(dst.width) != src.cols || int(dst.height) < src.rows) LFATAL("src and dst dims must match");

  // Note how the destination opencv image dstcv here is just a shell, the actual pixel data is in dst:
  cv::Mat dstcv = jevois::rawimage::cvImage(dst);

  switch (dst.fmt)
  {
  case V4L2_PIX_FMT_SRGGB8: convertCvBGRtoBayer(src, dst); break;
  case V4L2_PIX_FMT_YUYV: convertCvBGRtoYUYV(src, dst); break;
  case V4L2_PIX_FMT_GREY: cv::cvtColor(src, dstcv, CV_BGR2GRAY); break;
  case V4L2_PIX_FMT_RGB565: cv::cvtColor(src, dstcv, CV_BGR2BGR565); break;
  case V4L2_PIX_FMT_MJPEG: jevois::compressBGRtoJpeg(src, dst, quality); break;
  case V4L2_PIX_FMT_BGR24: memcpy(dst.pixelsw<void>(), src.data, dst.width * dst.height * dst.bytesperpix()); break;
  default: LFATAL("Unsupported output pixel format " << jevois::fccstr(dst.fmt) << std::hex <<' '<< dst.fmt);
  }
}

// ####################################################################################################
void jevois::rawimage::unpackCvRGBAtoGrayRawImage(cv::Mat const & src, RawImage & dst)
{
  if (src.type() != CV_8UC4) LFATAL("src must have type CV_8UC4 and RGBA pixels");
  if (dst.fmt != V4L2_PIX_FMT_GREY) LFATAL("dst must have pixel type V4L2_PIX_FMT_GREY");
  int const w = src.cols, h = src.rows;
  if (int(dst.width) < w || int(dst.height) < 4 * h) LFATAL("dst must be at least as wide and 4x as tall as src");

  unsigned char const * sptr = src.data; unsigned char * dptr = dst.pixelsw<unsigned char>();
  int const stride = int(dst.width) - w;
  
  // Do R, G, B in 3 threads then A in the current thread:
  std::vector<std::future<void> > fut;
  for (int i = 0; i < 3; ++i) fut.push_back(std::async(std::launch::async, [&](int offset) {
          unsigned char const * s = sptr + offset; unsigned char * d = dptr + offset * w * h;
          for (int y = 0; y < h; ++y) { for (int x = 0; x < w; ++x) { *d++ = *s; s += 4; } d += stride; } }, i));

  unsigned char const * s = sptr + 3; unsigned char * d = dptr + 3 * w * h;
  for (int y = 0; y < h; ++y) { for (int x = 0; x < w; ++x) { *d++ = *s; s += 4; } d += stride; }

  // Wait for all threads to complete (those should never throw):
  for (auto & f : fut) f.get();
}

// ####################################################################################################
void jevois::rawimage::convertCvRGBAtoRawImage(cv::Mat const & src, RawImage & dst, int quality)
{
  if (src.type() != CV_8UC4) LFATAL("src must have type CV_8UC4 and RGBA pixels");
  if (int(dst.width) != src.cols || int(dst.height) != src.rows) LFATAL("src and dst dims must match");

  cv::Mat dstcv = jevois::rawimage::cvImage(dst);

  switch (dst.fmt)
  {
  case V4L2_PIX_FMT_SRGGB8: convertCvRGBAtoBayer(src, dst); break;
  case V4L2_PIX_FMT_YUYV: convertCvRGBAtoYUYV(src, dst); break;
  case V4L2_PIX_FMT_GREY: cv::cvtColor(src, dstcv, CV_RGBA2GRAY); break;
  case V4L2_PIX_FMT_RGB565: cv::cvtColor(src, dstcv, CV_BGRA2BGR565); break;
  case V4L2_PIX_FMT_MJPEG: jevois::compressRGBAtoJpeg(src, dst, quality); break;
  case V4L2_PIX_FMT_BGR24: cv::cvtColor(src, dstcv, CV_RGBA2BGR); break;
  default: LFATAL("Unsupported output pixel format " << jevois::fccstr(dst.fmt) << std::hex <<' '<< dst.fmt);
  }
}

// ####################################################################################################
void jevois::rawimage::convertCvGRAYtoRawImage(cv::Mat const & src, RawImage & dst, int quality)
{
  if (src.type() != CV_8UC1) LFATAL("src must have type CV_8UC1 and Gray pixels");
  if (int(dst.width) != src.cols || int(dst.height) != src.rows) LFATAL("src and dst dims must match");

  cv::Mat dstcv = jevois::rawimage::cvImage(dst);

  switch (dst.fmt)
  {
  case V4L2_PIX_FMT_SRGGB8: convertCvGRAYtoBayer(src, dst); break;
  case V4L2_PIX_FMT_YUYV: convertCvGRAYtoYUYV(src, dst); break;
  case V4L2_PIX_FMT_GREY: memcpy(dst.pixelsw<void>(), src.data, dst.width * dst.height * dst.bytesperpix()); break;
  case V4L2_PIX_FMT_RGB565: cv::cvtColor(src, dstcv, CV_GRAY2BGR565); break;
  case V4L2_PIX_FMT_MJPEG: jevois::compressGRAYtoJpeg(src, dst, quality); break;
  case V4L2_PIX_FMT_BGR24: cv::cvtColor(src, dstcv, CV_GRAY2BGR); break;
  default: LFATAL("Unsupported output pixel format " << jevois::fccstr(dst.fmt) << std::hex <<' '<< dst.fmt);
  }
}

// ####################################################################################################
namespace
{
  class hflipYUYV : public cv::ParallelLoopBody
  {
    public:
      hflipYUYV(unsigned char * outImage, size_t outw) :
          outImg(outImage), linesize(outw * 2) // 2 bytes/pix for YUYV
      { }

      virtual void operator()(const cv::Range & range) const
      {
        for (int j = range.start; j < range.end; ++j)
        {
          int const off = j * linesize;

          for (int i = 0; i < linesize / 2; i += 4)
          {
            unsigned char * ptr1 = outImg + off + i;
            unsigned char * ptr2 = outImg + off + linesize - 4 - i;
            std::swap(ptr1[0], ptr2[2]);
            std::swap(ptr1[1], ptr2[1]);
            std::swap(ptr1[2], ptr2[0]);
            std::swap(ptr1[3], ptr2[3]);
          }
        }
      }
      
    private:
      unsigned char * outImg;
      int linesize;
  };

} // anonymous namespace

// ####################################################################################################
void jevois::rawimage::hFlipYUYV(RawImage & img)
{
  if (img.fmt != V4L2_PIX_FMT_YUYV) LFATAL("img format must be V4L2_PIX_FMT_YUYV");
  cv::parallel_for_(cv::Range(0, img.height), hflipYUYV(img.pixelsw<unsigned char>(), img.width));
}

