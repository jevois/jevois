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

#include <istream>
#include <ostream>

namespace jevois
{
  //! A generic range class with a step
  /*! The StepRange class is used to represent a range from [min .. (step) .. max]

      Valid values are min, min+step, min+2*step, min+3*step, ... max. Note that max is a valid value even if it is not
      equal to min+N*step for some integer N. Note that T must be a numeric type (can be cast to double). Throws if step
      is not a positive or zero number.

      Because StepRange is typically only used for limited specific purposes (e.g., defining paremeters for controls in
      video4linux framegrabbers), the interface provided is minimal (much more limited than that of jevois::Range).

      \ingroup types */
  template <typename T>
  class StepRange
  {
    public:
      //! Default constructor, range is [0 .. (0) .. 0]
      explicit StepRange();

      //! Constructor
      explicit StepRange(T const mini, T const stepi, T const maxi);

      //! Copy constructor
      StepRange(StepRange<T> const & other) = default;

      //! Move constructor
      StepRange(StepRange<T> && other) = default;

      //! Copy-conversion constructor, uses jevois::clamped_convert<T,U> internally
      /*! Note that this constructor is \e explicit, i.e., you need to explicitly mention it. This is to avoid
          conversions to happen without a programmer being aware of it. For example:
          @code
          void myFunc(jevois::StepRange<float> const & rng) { ... }
          jevois::StepRange<int> r1(0, 5, 100);
          jevois::StepRange<float> r2(r1); // ok, explicit constructor call
          myFunc(r2);  // ok, no conversion necessary
          //myFunc(r1) // NO: passing StepRange<int> to function that takes a StepRange<float> arg fails to compile
          myFunc(jevois::StepRange<float>(r1)); // ok, nice and explicit; and you can think about whether r1 should 
                                         // have been a StepRange<float> in the first place so you don't waste CPU
                                         // doing this conversion (as opposed to just not being aware of the wasting)
          @endcode */
      template <typename U>
      explicit StepRange(StepRange<U> const & other);

      //! Assignment
      StepRange<T> & operator=(StepRange<T> const & other) = default;

      //! Move assignment
      StepRange<T> & operator=(StepRange<T> && other) = default;

      //! Return the minimum value
      T const & min() const;

      //! Return the step value
      T const & step() const;

      //! Return the maximum value
      T const & max() const;

      //! Return whether min() == max()
      bool empty() const;

      //! Return true if a value is valid (i.e., it is min, min+step, min+step*2, ... or max)
      bool isValueValid(T const val) const;

    private:
      T itsMin, itsStep, itsMax;
  };

  // ######################################################################
  // Free functions for StepRange<T>
  // ######################################################################

  //! Stream out as "min...(step)...max"
  /*! \relates jevois::StepRange */
  template <typename T>
  std::ostream & operator<<(std::ostream & out, StepRange<T> const & r);

  //! Stream in as "min...(step)...max"
  /*! \relates jevois::StepRange */
  template <typename T>
  std::istream & operator>>(std::istream & in, StepRange<T> & r);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs \c min...step...max
  /*! \relates jevois::StepRange */
  template <class T>
  void paramValToString(StepRange<T> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads \c min...step...max
  /*! \relates jevois::StepRange */
  template <class T>
  void paramStringToVal(std::string const & valstring, StepRange<T> & result);

  //! Equality test: StepRange<T> == StepRange<T>
  /*! \relates jevois::StepRange */
  template <typename T>
  bool operator==(StepRange<T> const & range1, StepRange<T> const & range2);

  //! Inequality test: StepRange<T> != StepRange<T>
  /*! \relates jevois::StepRange */
  template <typename T>
  bool operator!=(StepRange<T> const & range1, StepRange<T> const & range2);

} // namespace jevois

// Include inlined implementation details that are of no interest to the end user
#include <jevois/Types/details/StepRangeImpl.H>

