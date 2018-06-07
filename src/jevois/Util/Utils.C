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

#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>

#include <linux/videodev2.h>
#include <string>
#include <vector>
#include <regex>

#include <string.h> // for strncmp
#include <fstream>
#include <cstdarg> // for va_start, etc
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <cctype> // for std::isspace()

#include <opencv2/core/hal/interface.h> // for CV_MAT_DEPTH_MASK

// ####################################################################################################
std::string jevois::fccstr(unsigned int fcc)
{
  if (fcc == 0) return "NONE"; // for no video over USB output
  
  std::string ret("    ");
  ret[0] = static_cast<char>(fcc & 0xff);
  ret[1] = static_cast<char>((fcc >> 8) & 0xff);
  ret[2] = static_cast<char>((fcc >> 16) & 0xff);
  ret[3] = static_cast<char>((fcc >> 24) & 0xff);
  return ret;
}

// ####################################################################################################
std::string jevois::cvtypestr(unsigned int cvtype)
{
  // From: https://gist.github.com/rummanwaqar/cdaddd5a175f617c0b107d353fd33695
  std::string r;

  uchar depth = cvtype & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (cvtype >> CV_CN_SHIFT);

  switch (depth)
  {
  case CV_8U:  r = "8U"; break;
  case CV_8S:  r = "8S"; break;
  case CV_16U: r = "16U"; break;
  case CV_16S: r = "16S"; break;
  case CV_32S: r = "32S"; break;
  case CV_32F: r = "32F"; break;
  case CV_64F: r = "64F"; break;
  default:     r = "User"; break;
  }

  r += 'C' + (chans + '0');

  return r;
}

// ####################################################################################################
unsigned int jevois::strfcc(std::string const & str)
{
  if (str == "BAYER") return V4L2_PIX_FMT_SRGGB8;
  else if (str == "YUYV") return V4L2_PIX_FMT_YUYV;
  else if (str == "GREY" || str == "GRAY") return V4L2_PIX_FMT_GREY;
  else if (str == "MJPG") return V4L2_PIX_FMT_MJPEG;
  else if (str == "RGB565") return V4L2_PIX_FMT_RGB565;
  else if (str == "BGR24") return V4L2_PIX_FMT_BGR24;
  else if (str == "NONE") return 0;
  else throw std::runtime_error("Invalid pixel format " + str);
}

// ####################################################################################################
unsigned int jevois::v4l2BytesPerPix(unsigned int fcc)
{
  switch (fcc)
  {
  case V4L2_PIX_FMT_YUYV: return 2U;
  case V4L2_PIX_FMT_GREY: return 1U;
  case V4L2_PIX_FMT_SRGGB8: return 1U;
  case V4L2_PIX_FMT_RGB565: return 2U;
  case V4L2_PIX_FMT_MJPEG: return 2U; // at most??
  case V4L2_PIX_FMT_BGR24: return 3U;
  case 0: return 0U; // for NONE output to USB mode
  default: LFATAL("Unsupported pixel format " << jevois::fccstr(fcc));
  }
}

// ####################################################################################################
unsigned int jevois::v4l2ImageSize(unsigned int fcc, unsigned int width, unsigned int height)
{ return width * height * jevois::v4l2BytesPerPix(fcc); }

// ####################################################################################################
unsigned int jevois::blackColor(unsigned int fcc)
{
  switch (fcc)
  {
  case V4L2_PIX_FMT_YUYV: return 0x8000;
  case V4L2_PIX_FMT_GREY: return 0;
  case V4L2_PIX_FMT_SRGGB8: return 0;
  case V4L2_PIX_FMT_RGB565: return 0;
  case V4L2_PIX_FMT_MJPEG: return 0;
  case V4L2_PIX_FMT_BGR24: return 0;
  default: LFATAL("Unsupported pixel format " << jevois::fccstr(fcc));
  }
}

// ####################################################################################################
unsigned int jevois::whiteColor(unsigned int fcc)
{
  switch (fcc)
  {
  case V4L2_PIX_FMT_YUYV: return 0x80ff;
  case V4L2_PIX_FMT_GREY: return 0xff;
  case V4L2_PIX_FMT_SRGGB8: return 0xff;
  case V4L2_PIX_FMT_RGB565: return 0xffff;
  case V4L2_PIX_FMT_MJPEG: return 0xff;
  case V4L2_PIX_FMT_BGR24: return 0xffffff;
  default: LFATAL("Unsupported pixel format " << jevois::fccstr(fcc));
  }
}

// ####################################################################################################
std::vector<std::string> jevois::split(std::string const & input, std::string const & regex)
{
  // This code is from: http://stackoverflow.com/questions/9435385/split-a-string-using-c11
  // passing -1 as the submatch index parameter performs splitting
  std::regex re(regex);
  std::sregex_token_iterator first{input.begin(), input.end(), re, -1}, last;
  return { first, last };
}

// ####################################################################################################
std::string jevois::join(std::vector<std::string> const & strings, std::string const & delimiter)
{
  if (strings.empty()) return "";
  if (strings.size() == 1) return strings[0];

  std::string ret; size_t const szm1 = strings.size() - 1;

  for (size_t i = 0; i < szm1; ++i) ret += strings[i] + delimiter;
  ret += strings[szm1];

  return ret;
}

// ####################################################################################################
bool jevois::stringStartsWith(std::string const & str, std::string const & prefix)
{
  return (strncmp(str.c_str(), prefix.c_str(), prefix.length()) == 0);
}

// ####################################################################################################
std::string jevois::replaceWhitespace(std::string const & str, char rep)
{
  std::string ret = str;
  for (char & c : ret) if (std::isspace(c)) c = rep;
  return ret;
}

// ####################################################################################################
std::string jevois::strip(std::string const & str)
{
  int idx = str.length() - 1;
  while (idx >= 0 && std::isspace(str[idx])) --idx;
  return str.substr(0, idx + 1);
}

// ####################################################################################################
namespace
{
  // This code is from NRT, and before that from the iLab C++ neuromorphic vision toolkit
  std::string vsformat(char const * fmt, va_list ap)
  {
    // if we have a null pointer or an empty string, then just return an empty std::string
    if (fmt == nullptr || fmt[0] == '\0') return std::string();

    int bufsize = 1024;
    while (true)
    {
      char buf[bufsize];
      
      int const nchars = vsnprintf(buf, bufsize, fmt, ap);
      
      if (nchars < 0)
      {
        // Better leave this as LFATAL() rather than LERROR(), otherwise we have to return a bogus std::string (e.g. an
        // empty string, or "none", or...), which might be dangerous if it is later used as a filename, for example.
        LFATAL("vsnprintf failed for format '" << fmt << "' with bufsize = " << bufsize);
      }
      else if (nchars >= bufsize)
      {
        // buffer was too small, so let's double the bufsize and try again:
        bufsize *= 2;
        continue;
      }
      else
      {
        // OK, the vsnprintf() succeeded:
        return std::string(&buf[0], nchars);
      }
    }
    return std::string(); // can't happen, but placate the compiler
  }
}

// ####################################################################################################
std::string jevois::sformat(char const * fmt, ...)
{
  va_list a;
  va_start(a, fmt);
  std::string result = vsformat(fmt, a);
  va_end(a);
  return result;
}

// ####################################################################################################
void jevois::flushcache()
{
#ifdef JEVOIS_PLATFORM
  std::ofstream ofs("/proc/sys/vm/drop_caches");
  if (ofs.is_open()) ofs << "3" << std::endl;
  else LERROR("Failed to flush cache -- ignored");
#endif
}

// ####################################################################################################
// This code modified from here: https://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-
// output-of-command-within-c-using-posix
std::string jevois::system(std::string const & cmd, bool errtoo)
{
  std::array<char, 128> buffer; std::string result;
  std::shared_ptr<FILE> pip;
  if (errtoo) pip.reset(popen((cmd + " 2>&1").c_str(), "r"), pclose);
  else pip.reset(popen(cmd.c_str(), "r"), pclose);
  if (!pip) LFATAL("popen() failed for command [" << cmd << ']');
  while (!feof(pip.get())) if (fgets(buffer.data(), 128, pip.get()) != NULL) result += buffer.data();
  return result;
}
