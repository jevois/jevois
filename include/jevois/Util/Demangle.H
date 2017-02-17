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
#include <typeinfo>

namespace jevois
{
  /*! \defgroup demangling Name demangling functions

      Name demangling functions return the original name of a type as written in the source code, as opposed to the name
      used internally by the compiler, which is mangled to avoid name conflicts and confusions across different
      namespaces, overloads, and such.

      See http://en.wikipedia.org/wiki/Name_mangling for more information.

      \ingroup utils */

  /*! @{ */ // **********************************************************************

  //! Demangle a mangled name
  std::string demangle(std::string const & mangledName);

  //! Demangle a mangled name
  std::string demangle(char const * mangledName);

  //! Get the demangled type name of type T
  template <typename T>
  std::string demangledName();

  //! Specialization for std::string to avoid getting an ugly name
  template <>
  std::string demangledName<std::string>();

  /*! @} */ // **********************************************************************

} // namespace jevois

// Include implementation details of no interest to end users
#include <jevois/Util/details/DemangleImpl.H>
