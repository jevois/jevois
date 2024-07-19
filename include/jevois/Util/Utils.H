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
#include <sstream>
#include <filesystem>

//! Metadata V4L2 format used by Amlogic A311D camera ISP
#define ISP_V4L2_PIX_FMT_META v4l2_fourcc( 'M', 'E', 'T', 'A' )

//! JeVois-Pro zero-copy display of camera input frame (to be used as output mode in VideoMapping)
/*! Note that the underlying mode is RGBA as used by MALI OpenGL */
#define JEVOISPRO_FMT_GUI v4l2_fourcc( 'J', 'V', 'U', 'I' )

namespace jevois
{
  /*! \defgroup utils Misc utilities
    
    Miscellaneous utility and helper functions. */
  
  /*! @{ */ // **********************************************************************

  //! Convert a V4L2 four-cc code (V4L2_PIX_FMT_...) to a 4-char string
  std::string fccstr(unsigned int fcc);

  //! Convert cv::Mat::type() code to to a string (e.g., CV_8UC1, CV_32SC3, etc)
  std::string cvtypestr(unsigned int cvtype);

  //! Return the number of bytes per pixel for a given OpenCV pixel type
  unsigned int cvBytesPerPix(unsigned int cvtype);

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

  //! Apply a letterbox resizing to fit an image into a window
  /*! Modify given image dims (imw,imh) so that the image fits inside window dims (winw,winh) while being as large as
      possible but without modifying the image's aspect ratio. If noalias is specified, the scaling factor will be
      rounded down to the nearest integer to prevent aliasing in the display. This may reduce the displayed image
      size. For example, with a 1920x1080 window, a 640x480 image would be letterboxed to 1440x1080 when noalias is
      false. But that is a scaling factor of 2.25 which may create rendering aliasing. When noalias is true, the
      letterboxed image size will be 1280x960 (scale factor of 2.0). */
  void applyLetterBox(unsigned int & imw, unsigned int & imh, unsigned int const winw, unsigned int const winh,
                      bool noalias);
  
  //! Split string into vector of tokens using a regex to specify what to split on; default regex splits by whitespace
  std::vector<std::string> split(std::string const & input, std::string const & regex = "\\s+");

  //! Concatenate a vector of tokens into a string
  std::string join(std::vector<std::string> const & strings, std::string const & delimiter);

  //! Return true if str starts with prefix (including if both strings are equal)
  /*! Note that if str is shorter than prefix, return is false (like in strncmp()). */ 
  bool stringStartsWith(std::string const & str, std::string const & prefix);

  //! Replace white space characters in a string with underscore (default) or another character
  std::string replaceWhitespace(std::string const & str, char rep = '_');

  //! Strip white space (including CR, LF, tabs, etc) from the end of a string
  std::string strip(std::string const & str);

  //! Extract a portion of a string between two delimiters
  /*! Returns an empty string if the delimiters were not found. */
  std::string extractString(std::string const & str, std::string const & startsep, std::string const & endsep);

  //! Replace first instance of 'from' with 'to'
  /*! Returns the number of replacements made (0 or 1). */
  size_t replaceStringFirst(std::string & str, std::string const & from, std::string const & to);

  //! Replace all instances of 'from' with 'to'
  /*! Returns the number of replacements made. */
  size_t replaceStringAll(std::string & str, std::string const & from, std::string const & to);
  
  //! Replace all instances of 'from' with 'to'
  /*! Returns the number of replacements made. */
  std::string replaceAll(std::string const & str, std::string const & from, std::string const & to);

  //! Convert string to lowercase
  std::string tolower(std::string const & str);
  
  //! Compute an absolute path from two paths
  /*! Parameter \p path contains a path that could be either absolute or relative; parameter \p root should contain a
      root path. If path is absolute, it is returned; otherwise, root is prepended to it and the result is returned. */
  std::filesystem::path absolutePath(std::filesystem::path const & root, std::filesystem::path const & path);
  
  //! Create a string using printf style arguments
  /*! Example:
      @code
      std::string s = jevois::sformat("MyString_%f_%d", 1.0, 2);
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
  /*! If errtoo is true, we also grab errors by appending a 2>&1 to the command. Throws std::runtime_error if the
      command cannot be run somehow, or if it exits with a non-zero exit code. */
  std::string system(std::string const & cmd, bool errtoo = true);

  //! Report a duration given in seconds with variable units (ns, us, ms, or s), with precision of 2 decimal points
  std::string secs2str(double secs);

  //! Report avg+/-std duration given in seconds with variable units (ns, us, ms, or s), with 1 decimal point
  std::string secs2str(std::vector<double> secs);

  //! Report a duration given in seconds with variable units (ns, us, ms, or s)
  /*! You should decide on precision using std::setprecision(n) in your stream before calling this function. */
  void secs2str(std::ostringstream & ss, double secs);

  //! Report a number with variable multipliers (K, M, G, T, P, E, Z, Y), with precision of 2 decimal points
  std::string num2str(double n);

  //! Report a number with variable multipliers (K, M, G, T, P, E, Z, Y)
  /*! You should decide on precision using std::setprecision(n) in your stream before calling this function. */
  void num2str(std::ostringstream & ss, double n);

  //! Read one line from a file and return it as a string
  /*! Useful to get info from virtual kernel filesystems, such as CPU temperature, etc. Parameter \p skip optionally
      specifies a number of lines to skip before returning the one of interest. */
  std::string getFileString(char const * fname, int skip = 0);

  /*! @} */ // **********************************************************************

} // namespace jevois


#ifdef JEVOIS_DOXYGEN
// ####################################################################################################
//! Helper macro to execute an ioctl, ignore interruptions, and, if error, issue a fatal message and throw
/*! \def XIOCTL(dev, req, mem)
    \hideinitializer

    This macro assumes that req is an identifier (\#define) for the corresponding ioctl number.
    \ingroup utils */
#define XIOCTL(dev, req, mem) { }

// ####################################################################################################
//! Helper macro to execute an ioctl, ignore interruptions, and, if error throw quietly
/*! \def XIOCTL_QUIET(dev, req, mem)
    \hideinitializer

    This is useful, e.g., for V4L2 camera enumeration of formats, where one is supposed to call an ioctl 
    with increasing format number until it fails. When that happens we don't want to display any fatal error 
    message as XIOCTL() does. This macro assumes that req is an identifier for the corresponding ioctl number. 
    \ingroup utils */
#define XIOCTL_QUIET(dev, req, mem) { }

// ####################################################################################################
//! Helper macro to execute an ioctl, ignore interruptions, and, if error throw quietly
/*! \def XIOCTL_QUIET_ONCE(dev, req, mem)
    \hideinitializer

    This is useful, e.g., for V4L2 camera enumeration of formats, where one is supposed to call an ioctl 
    with increasing format number until it fails. When that happens we don't want to display any fatal error 
    message as XIOCTL() does. This macro assumes that req is an identifier for the corresponding ioctl number.
    Note that in this version we throw on any error.
    \ingroup utils */
#define XIOCTL_QUIET_ONCE(dev, req, mem) { }
#endif

// Include implementation details
#include <jevois/Util/details/UtilsImpl.H>
