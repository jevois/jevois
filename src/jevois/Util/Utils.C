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
