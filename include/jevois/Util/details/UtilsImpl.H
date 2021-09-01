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

#include <jevois/Debug/Log.H>
#include <jevois/Util/Demangle.H>
#include <errno.h>
#include <sys/ioctl.h>
#include <type_traits>

// ####################################################################################################
template <typename T> inline
typename std::enable_if< ! std::is_integral<T>::value, T>::type jevois::from_string(std::string const & str)
{
  T val; std::istringstream iss(str); iss >> val;

  if (iss.fail())
    throw std::invalid_argument("Cannot convert [" + str + "] to value of type " + jevois::demangledName<T>());

  return val;
}

// ####################################################################################################
template <typename T> inline
typename std::enable_if<std::is_integral<T>::value, T>::type jevois::from_string(std::string const & str)
{
  // We need to be careful with unsigned parameters to throw if the string value is negative:
  if (std::is_unsigned<T>::value)
    if (str.find('-') != str.npos)
      throw std::range_error("Cannot assign negative value [" + str +
			     "] to unsigned type " + jevois::demangledName<T>());

  // Make sure people are aware of the octal conversion if it will trigger here:
  if (str.length() > 1 && str[0] == '0' && str[1] >= '0' && str[1] <= '9')
    LINFO("Value [" << str << "] interpreted as Octal (base 8) because of leading 0");

  // Convert. stoll() will throw if malformed
  return clamped_convert<T>(std::stoll(str, nullptr, 0));
}

// ####################################################################################################
template <typename T> inline
std::string jevois::to_string(T const & val)
{
  std::ostringstream oss; oss << val; return oss.str();
}

// ####################################################################################################
template <typename dest_type, typename source_type> inline
dest_type jevois::clamped_convert(source_type source)
{




  //FIXME




  return dest_type(source);
}

// ####################################################################################################
#define XIOCTL(dev, req, mem)                                           \
  do {                                                                  \
    int result = 0;                                                     \
    do { result = ioctl(dev, req, mem); } while ((result < 0) && (errno == EINTR)); \
    if (result < 0) PLFATAL(#req " failed");                            \
  } while (false)

// ####################################################################################################
#define XIOCTL_QUIET(dev, req, mem)                                     \
  do {                                                                  \
    int result = 0;                                                     \
    do { result = ioctl(dev, req, mem); } while ((result < 0) && (errno == EINTR)); \
    if (result < 0) throw std::runtime_error(#req " failed");           \
  } while (false)

// ####################################################################################################
#define XIOCTL_QUIET_ONCE(dev, req, mem)                                \
  do {                                                                  \
    int result = ioctl(dev, req, mem);                                  \
    if (result < 0) throw std::runtime_error(#req " failed");           \
  } while (false)
