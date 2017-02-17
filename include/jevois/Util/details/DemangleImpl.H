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

// This code is inspired by the Neuromorphic Robotics Toolkit (http://nrtkit.org)

#pragma once

#include <string>
#include <stdlib.h>

// ######################################################################
#if (__GNUC__ && __GNUC__ >= 3)
#include <cxxabi.h>

inline std::string jevois::demangle(std::string const & mangledName)
{
  int status = 0; char *demangledName = nullptr; std::size_t len;

  demangledName = abi::__cxa_demangle(mangledName.c_str(), 0, &len, &status);

  std::string retName(demangledName);
  free(demangledName);
  return retName;
}

inline std::string jevois::demangle(char const * mangledName)
{
  int status = 0; char *demangledName = nullptr; std::size_t len;

  demangledName = abi::__cxa_demangle(mangledName, 0, &len, &status);

  std::string retName(demangledName);
  free(demangledName);
  return retName;
}

#else // (__GNUC__ && __GNUC__ >= 3)

// We don't know how to demangle on other compilers than gcc...
inline std::string jevois::demangle(std::string const & mangledName)
{ return mangledName; }

inline std::string jevois::demangle(char const * mangledName)
{ return std::string(mangledName); }

#endif // (__GNUC__ && __GNUC__ >= 3)

// ######################################################################
template <typename T> inline
std::string jevois::demangledName()
{ return jevois::demangle(typeid(T).name()); }

// ######################################################################
namespace jevois
{
  template <> inline
  std::string demangledName<std::string>() { return "string"; }
}

