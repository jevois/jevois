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
  //! A generic range class
  /*! The Range class is used to represent a range from [min .. max]

      Note that the operators of Range do not use type promotions. This is to minimize template burden and also possible
      programmer confusion. For example, while in principle one could define Range<int> + float and return a
      Range<float>, here we do not define such an operator. The same result can be achieved by first converting the
      range and then adding the constant:

      @code
      jevois::Range<int> r1(0, 100);
      // NOT ALLOWED: jevois::Range<float> r2 = r1 + 1.23F;
      jevois::Range<float> r2 = jevois::Range<float>(r1) + 1.23F; // OK, no hidden conversions, easy to read, explicit
      @endcode

      Operators on Range use range checking and clamping internally. Thus, be careful if using Range<byte>. For
      example:
      @code
      jevois::Range<jevois::byte> r(jevois::byte(0), jevois::byte(250));
      r += jevois::byte(100); // new range is 100-255
      @endcode

      \ingroup types */
  template <typename T>
  class Range
  {
    public:
      //! Default constructor, range is [0 .. 0]
      explicit Range();

      //! Constructor
      explicit Range(T const mini, T const maxi);

      //! Copy constructor
      Range(Range<T> const & other) = default;

      //! Move constructor
      Range(Range<T> && other) = default;

      //! Copy-conversion constructor, uses jevois::clamped_convert<T,U> internally
      /*! Note that this constructor is \e explicit, i.e., you need to explicitly mention it. This is to avoid
          conversions to happen without a programmer being aware of it. For example:
          @code
          void myFunc(jevois::Range<float> const & rng) { ... }
          jevois::Range<int> r1(0, 100);
          jevois::Range<float> r2(r1); // ok, explicit constructor call
          myFunc(r2);  // ok, no conversion necessary
          //myFunc(r1) // NO: passing Range<int> to function that takes a Range<float> arg fails to compile
          myFunc(jevois::Range<float>(r1)); // ok, nice and explicit; and you can think about whether r1 should 
                                         // have been a Range<float> in the first place so you don't waste CPU
                                         // doing this conversion (as opposed to just not being aware of the wasting)
          @endcode */
      template <typename U>
      explicit Range(Range<U> const & other);

      //! Assignment
      Range<T> & operator=(Range<T> const & other) = default;

      //! Move assignment
      Range<T> & operator=(Range<T> && other) = default;

      //! Return the minimum value
      T const & min() const;

      //! Return the maximum value
      T const & max() const;

      //! Return whether min() == max()
      bool empty() const;

      //! Extend the range, if needed, so that it includes val
      void extend(T const val);

      //! Return true if val is within [min ... max]
      bool contains(T const & val) const;
      
    private:
      T itsMin;
      T itsMax;
  };

  // ######################################################################
  // Free functions for Range<T>
  // ######################################################################

  //! Merge two ranges
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> merge(Range<T> const & r1, Range<T> const & r2);

  //! Stream out as "[min ... max]"
  /*! \relates jevois::Range */
  template <typename T>
  std::ostream & operator<<(std::ostream & out, Range<T> const & r);

  //! Stream in as "[min ... max]"
  /*! \relates jevois::Range */
  template <typename T>
  std::istream & operator>>(std::istream & in, Range<T> & r);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs \c min...max (e.g., 0...100)
  /*! \relates jevois::Range */
  template <class T>
  void paramValToString(Range<T> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads \c min...max (e.g., 0...100)
  /*! \relates jevois::Range */
  template <class T>
  void paramStringToVal(std::string const & valstring, Range<T> & result);

  //! Equality test: Range<T> == Range<T>
  /*! \relates jevois::Range */
  template <typename T>
  bool operator==(Range<T> const & range1, Range<T> const & range2);

  //! Inequality test: Range<T> != Range<T>
  /*! \relates jevois::Range */
  template <typename T>
  bool operator!=(Range<T> const & range1, Range<T> const & range2);

  //! Add constant to both ends of a range: Range<T> + T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator+(Range<T> const & range, T const & scalar);

  //! Add constant to both ends of a range: T + Range<T>
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator+(T const & scalar, Range<T> const & range);

  //! Subtract constant from both ends of a range: Range<T> - T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator-(Range<T> const & range, T const & scalar);

  //! Subtract constant from both ends of a range: T - Range<T>
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator-(T const & scalar, Range<T> const & range);

  //! Divide both ends of a range by a factor: Range<T> / T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator/(Range<T> const & range, T const & scalar);

  //! Divide a factor by both ends of a range: T / Range<T>
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator/(T const & scalar, Range<T> const & range);

  //! Multiply both ends of a range by a factor: Range<T> * T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator*(Range<T> const & range, T const & scalar);

  //! Multiply a factor by both ends of a range: T * Range<T>
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> operator*(T const & scalar, Range<T> const & range);

  //! Add constant to both ends of a range: Range<T> += T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> & operator+=(Range<T> & range, T const & scalar);

  //! Subtract constant from both ends of a range: Range<T> -= T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> & operator-=(Range<T> & range, T const & scalar);

  //! Multiply both ends of a range by a factor: Range<T> *= T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> & operator*=(Range<T> & range, T const & scalar);

  //! Divide both ends of a range by a factor: Range<T> /= T
  /*! \relates jevois::Range */
  template <typename T>
  Range<T> & operator/=(Range<T> & range, T const & scalar);

} // namespace jevois

// Include inlined implementation details that are of no interest to the end user
#include <jevois/Types/details/RangeImpl.H>
