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

#pragma once

// This code from:

/* author Shane Grant <wgrant@usc.edu>
// ////////////////////////////////////////////////////////////////////////
//              The iLab Neuromorphic Robotics Toolkit (NRT)             //
// Copyright 2010-2012 by the University of Southern California (USC)    //
//                          and the iLab at USC.                         //
//                                                                       //
//                iLab - University of Southern California               //
//                Hedco Neurociences Building, Room HNB-10               //
//                    Los Angeles, Ca 90089-2520 - USA                   //
//                                                                       //
//      See http://ilab.usc.edu for information about this project.      //
// ////////////////////////////////////////////////////////////////////////
// This file is part of The iLab Neuromorphic Robotics Toolkit.          //
//                                                                       //
// The iLab Neuromorphic Robotics Toolkit is free software: you can      //
// redistribute it and/or modify it under the terms of the GNU General   //
// Public License as published by the Free Software Foundation, either   //
// version 3 of the License, or (at your option) any later version.      //
//                                                                       //
// The iLab Neuromorphic Robotics Toolkit is distributed in the hope     //
// that it will be useful, but WITHOUT ANY WARRANTY; without even the    //
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       //
// PURPOSE.  See the GNU General Public License for more details.        //
//                                                                       //
// You should have received a copy of the GNU General Public License     //
// along with The iLab Neuromorphic Robotics Toolkit.  If not, see       //
// <http://www.gnu.org/licenses/>.                                       //
// ////////////////////////////////////////////////////////////////////////
*/

#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/if.hpp>

/*! \defgroup compilerutil Miscellaneous preprocessor utilities related to controlling compilation
    \ingroup utils */

// Get the compiler name: (see http://stackoverflow.com/questions/19204792/c-if-elif-endif-dont-seem-to-work for why we
// use 2 variables; JEVOIS_COMPILER_NAME is used for macro expansion with the name, JEVOIS_COMPILER used for #if
// testing):
#define JEVOIS_COMPILER_GCC 1
#define JEVOIS_COMPILER_CLANG 2
#define JEVOIS_COMPILER_OTHER 3

#if defined __clang__
#define JEVOIS_COMPILER JEVOIS_COMPILER_CLANG
#define JEVOIS_COMPILER_NAME clang
#elif defined __GNUC__
#define JEVOIS_COMPILER JEVOIS_COMPILER_GCC
#define JEVOIS_COMPILER_NAME GCC
#else
#define JEVOIS_COMPILER JEVOIS_COMPILER_OTHER
#define JEVOIS_COMPILER_NAME other
#endif

#define JEVOIS_PRAGMA(x) _Pragma(BOOST_PP_STRINGIZE(JEVOIS_COMPILER_NAME x))

/*! @def JEVOIS_BEGIN_UNCHECKED_INCLUDES
    A series of preprocessor directives that turns off specific warnings as
    errors until the matching JEVOIS_END_UNCHECKED_INCLUDES.  This is useful for
    libraries that are written with code that won't compile under -Wall.

    Must be followed by a matching JEVOIS_END_UNCHECKED_INCLUDES
    @ingroup compilerutil */
#define JEVOIS_BEGIN_UNCHECKED_INCLUDES JEVOIS_PRAGMA(diagnostic push) \
  JEVOIS_PRAGMA(diagnostic ignored "-Wpragmas") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wunknown-pragmas") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wunused-local-typedefs") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wattributes") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wconstant-conversion") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wenum-compare") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wunknown-attributes") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wunused-but-set-variable") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wreorder") \
  JEVOIS_PRAGMA(diagnostic ignored "-Wall")

/*! @def JEVOIS_END_UNCHECKED_INCLUDES
    Turns back on all warnings that were in effect prior to an JEVOIS_BEGIN_UNCHECKED_INCLUDES.
    @ingroup compilerutil */
#define JEVOIS_END_UNCHECKED_INCLUDES JEVOIS_PRAGMA(diagnostic pop)
