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

#include <string>
#include <vector>
#include <type_traits> // for std::is_integral

namespace jevois
{
  /*! \defgroup utils Misc utilities
    
    Miscellaneous utility and helper functions. */
  
  /*! @{ */ // **********************************************************************

  //! Convert a V4L2 four-cc code (V4L2_PIX_FMT_...) to a 4-char string
  std::string fccstr(unsigned int fcc);

  //! Convert cv::Mat::type() code to to a string (e.g., CV_8UC1, CV_32SC3, etc)
  std::string cvtypestr(unsigned int cvtype);

  //! Convert a JeVois video format string to V4L2 four-cc code (V4L2_PIX_FMT_...)
  /*! Throws a runtime_error if str is not one of: BAYER, YUYV, GREY, GRAY, MJPG, RGB565, BGR24 or NONE. */
  unsigned int strfcc(std::string const & str);

  //! Return the number of bytes per pixel for a given V4L2_PIX_FMT_...
  unsigned int v4l2BytesPerPix(unsigned int fcc);

  //! Return the image size in bytes for a given V4L2_PIX_FMT_..., width, height
  unsigned int v4l2ImageSize(unsigned int fcc, unsigned int width, unsigned int height);

  //! Return a value that corresponds to black for the given video format
  /*! The returned value is appropriate to use as the color value for the image drawing functions in \ref image and may
      not always be zero. */
  unsigned int blackColor(unsigned int fcc);
  
  //! Return a value that corresponds to white for the given video format
  /*! The returned value is appropriate to use as the color value for the image drawing functions in \ref image. */
  unsigned int whiteColor(unsigned int fcc);
  
  //! Split string into vector of tokens using a regex to specify what to split on; default regex splits by whitespace
  std::vector<std::string> split(std::string const & input, std::string const & regex = "\\s+");

  //! Concatenate a vector of tokens into a string
  std::string join(std::vector<std::string> const & strings, std::string const & delimiter);

  //! Return true if str starts with prefix (including if both strings are equal)
  /*! Note that if str is shorter than prefix, return is false (like in strncmp()). */ 
  bool stringStartsWith(std::string const & str, std::string const & prefix);

  //! Replace white space characters in a string with underscore (default) or another characters
  std::string replaceWhitespace(std::string const & str, char rep = '_');

  //! Create a string using printf style arguments
  /*! Example:
      @code
      std::string s = nrt::sformat("MyString_%f_%d", 1.0, 2);
      @endcode 

      One should normally refrain from using sformat(), and instead use streaming operators of C++, one exception is
      when issuing serial messages that have float numbers in them, the printf-like sytnax of sformat is useful to
      quickly and easily specify a numerical precision. */
  std::string sformat(char const * fmt, ...)
    // NOTE: this __attribute__ tells gcc that it should issue printf-style warnings when compiling calls to sformat(),
    // treating the 1st argument (fmt) as the format string, and the 2nd and subsequent arguments as the printf-style
    // parameters
    __attribute__((format(__printf__, 1, 2)));

  // Doxygen is not too good with enable_if, it only documents one version of the function. Here is a workaround:
#ifdef JEVOIS_DOXYGEN
  //! Convert from string to a type
  /*! For integral types, internally uses std::stoll() for that type, which supports prefixes like 0 (for octal) and 0x
      (hex). Beware of that octal convention and do not pass leading zeros unless you mean it. For non-integral types,
      internally uses operator>> for that type, so it works with any type that supports it. */
  template <typename T> T from_string(std::string const & str);
#else
  //! Convert from string to a type, version for non-integral types
  /*! This internally uses operator>> for that type, so it works with any type that supports it. */
  template <typename T>
  typename std::enable_if< ! std::is_integral<T>::value, T>::type from_string(std::string const & str);

  //! Convert from string to a type, version for integral types
  /*! This internally uses std::stoll() for that type, which supports prefixes like 0 (for octal) and 0x (hex). */
  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, T>::type from_string(std::string const & str);
#endif
  
  //! Convert from type to string
  /*! This internally uses operator>> for that type, so it works with any type that supports it. */
  template <typename T>
  std::string to_string(T const & val);

  //! Clamped numerical conversion
  template <typename dest_type, typename source_type>
  dest_type clamped_convert(source_type source);

  //! Flush the caches, may sometimes be useful when running the camera in turbo mode
  void flushcache();

  //! Execute a command and grab stdout output to a string
  /*! If errtoo is true, we also grab errors by appending a 2>&1 to the command. */
  std::string system(std::string const & cmd, bool errtoo = true);
  
  /*! @} */ // **********************************************************************

} // namespace jevois


// ####################################################################################################
//! Helper macro to execute an ioctl, ignore interruptions, and, if error, issue a fatal message and throw
/*! \def XIOCTL(dev, req, mem)
    \hideinitializer

    This macro assumes that req is an identifier (\#define) for the corresponding ioctl number.
    \ingroup utils */

// ####################################################################################################
//! Helper macro to execute an ioctl, ignore interruptions, and, if error throw quietly
/*! \def XIOCTL_QUIET(dev, req, mem)
    \hideinitializer

    This is useful, e.g., for V4L2 camera enumeration of formats, where one is supposed to call an ioctl 
    with increasing format number until it fails. When that happens we don't want to display any fatal error 
    message as XIOCTL() does. This macro assumes that req is an identifier for the corresponding ioctl number. 
    \ingroup utils */

// Include implementation details
#include <jevois/Util/details/UtilsImpl.H>
