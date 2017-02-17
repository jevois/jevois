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

// This file has been modified / imported from the Neuromorphic Robotics Toolkit (NRT). Original copyright is:

/* author Laurent Itti
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

#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/transform.hpp>

#ifndef JEVOIS_DOXYGEN

// ######################################################################
#define JEVOIS_MACRO_ENUMERATED_RAW_LIST(seq) BOOST_PP_SEQ_ENUM(seq)

#define JEVOIS_MACRO_ENUMERATED_STRINGIZE(val) #val

#define JEVOIS_MACRO_ENUMERATED_STRINGIZE_OP(s, data, elem) JEVOIS_MACRO_ENUMERATED_STRINGIZE(elem)

#define JEVOIS_MACRO_ENUMERATED_STRINGIZE_SEQ(seq) BOOST_PP_SEQ_TRANSFORM(JEVOIS_MACRO_ENUMERATED_STRINGIZE_OP, 0, seq)

#define JEVOIS_MACRO_ENUMERATED_QUOTED_LIST(seq) BOOST_PP_SEQ_ENUM(JEVOIS_MACRO_ENUMERATED_STRINGIZE_SEQ(seq))

#define JEVOIS_MACRO_ENUMERATED_PREFIX_OP(s, data, elem) data::elem

#define JEVOIS_MACRO_ENUMERATED_PREFIX_SEQ(prefix, seq) BOOST_PP_SEQ_TRANSFORM(JEVOIS_MACRO_ENUMERATED_PREFIX_OP, prefix, seq)

#define JEVOIS_MACRO_ENUMERATED_PREFIXED_LIST(prefix, seq) BOOST_PP_SEQ_ENUM(JEVOIS_MACRO_ENUMERATED_PREFIX_SEQ(prefix, seq))

#define JEVOIS_DEFINE_ENUM_CLASS(name, seq)                             \
  enum class name { JEVOIS_MACRO_ENUMERATED_RAW_LIST(seq) };            \
  static std::vector<name> const name##_Values { JEVOIS_MACRO_ENUMERATED_PREFIXED_LIST(name, seq) }; \
  static std::vector<std::string> const name##_Strings { JEVOIS_MACRO_ENUMERATED_QUOTED_LIST(seq) }; \
                                                                        \
  inline std::ostream & operator<<(std::ostream & os, name const & v) { \
    std::vector<name>::const_iterator vitr = name##_Values.begin(), vend = name##_Values.end(); \
    std::vector<std::string>::const_iterator sitr = name##_Strings.begin(); \
    while (vitr != vend) if (v == *vitr) { os << *sitr; return os; } else { ++vitr; ++sitr; } \
    return os;                                                          \
  }                                                                     \
                                                                        \
  inline std::istream & operator>>(std::istream & is, name & v) {       \
    std::string s; is >> s;                                             \
    std::vector<name>::const_iterator vitr = name##_Values.begin(), vend = name##_Values.end(); \
    std::vector<std::string>::const_iterator sitr = name##_Strings.begin(); \
    while (vitr != vend) if (s == *sitr) { v = *vitr; return is; } else { ++vitr; ++sitr; } \
    throw std::range_error("Invalid value [" + s + "] for Enum class " #name); \
    return is; /* cannot happen but compiler complains */               \
  }

#endif
