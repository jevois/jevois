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
   copyright GNU Public License (GPL v3)
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

/*! \defgroup types JeVois-specific types and generic helper classes

    These types and classes support the core JeVois implementation. Pay particular attention to the very useful
    JEVOIS_DEFINE_ENUM_CLASS(name, SEQ) macro to define new enums that can be used in Parameter to allow menu-style
    parameters.

    JEVOIS_DEFINE_ENUM_CLASS(name, SEQ)
    -----------------------------------

    Helper macro to define new enum class types. SEQ should be a BOOST_PP_SEQ (see example syntax below). You will get:

    - an enum class declared and defined with the name and values supplied
    - a static const std::vector of your enum values, with all the values that the enum can take, in name_Values
    - a static const std::vector of std::string, with string names of all the values the enum can take, in name_Strings
    - operator<< and operator>> for your enum.

    You can in particular use name_Values when defining a Parameter definition to list all the valid values, and the
    streaming operators will allow you to set the Parameter value by string, etc.

    For example:

    @code
    JEVOIS_DEFINE_ENUM_CLASS(myEnum, (One) (Two) (Three));
    @endcode

    expands to:

    - first, the actual definition of the enum class, with the values you specified in the sequence:
    @code
    enum class myEnum { One, Two, Three };
    @endcode

    - second, a static const vector that contains all the enum values, listed in the order you specified:
    @code
    static std::vector<myEnum> const myEnum_Values { myEnum::One, myEnum::Two, myEnum::Three };
    @endcode

    - third, a static const vector of strings that contains all the enum values in string form, listed in the order you
      specified:
    @code
    static std::vector<std::string> const myEnum_Strings { "One", "Two", "Three" };
    @endcode

    - finally, two stream operators that allow you to convert between your enum values and string:
    @code
    inline std::ostream & operator<<(std::ostream & os, myEnum const & v) {
      std::vector<myEnum>::const_iterator vitr = myEnum_Values.begin(), vend = myEnum_Values.end();
      std::vector<std::string>::const_iterator sitr = myEnum_Strings.begin();
      while (vitr != vend) if (v == *vitr) { os << *sitr; return os; } else { ++vitr; ++sitr; }
      return os;
    }

    inline std::istream & operator>>(std::istream & is, myEnum & v) {
      std::string s; is >> s;
      std::vector<myEnum>::const_iterator vitr = myEnum_Values.begin(), vend = myEnum_Values.end();
      std::vector<std::string>::const_iterator sitr = myEnum_Strings.begin();
      while (vitr != vend) if (s == *sitr) { v = *vitr; return is; } else { ++vitr; ++sitr; }
      throw std::range_error("Invalid value [" + s + "] for Enum class myEnum");
    };
    @endcode

    Note how the \c operator>> will throw if the given string does not match the string representation of one of the
    enum's values. */

//! Include details of no interest to the user
#include <jevois/Types/details/EnumImpl.H>

