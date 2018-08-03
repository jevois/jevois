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
#include <iostream>
#include <sstream>
#include <vector>
#include <opencv2/core/core.hpp> // for Point_ and Size

namespace jevois
{
  /*! \defgroup parameterstrconv String conversions for Parameter
      \ingroup parameter */

  /*! @{ */ // **********************************************************************

  //! Machine-readable conversion of T to a string, for use in jevois::Parameter
  /*! A default implementation is provided which internally just uses jevois::to_string(). When implementing new types
      that can be used as parameters, users should supply the appropriate template specialization for this
      function. This function is used internally by Parameter to get a Parameter value as a string. */
  template <typename T>
  void paramValToString(T const & val, std::string & result);

  //! Machine-readable conversion of T to a string, specialization to convert unsigned char to number, not character
  template <>
  void paramValToString<unsigned char>(unsigned char const & val, std::string & result);

  //! Machine-readable conversion from string to T, for use in jevois::Parameter
  /*! @throws Implementation-dependent exception encountered during conversion

      A default implementation is provided which internally just uses jevois::from_string(). When implementing new types
      that can be used as parameters, users should supply the appropriate template specialization for this
      function. This function is used internally by Parameter to set a Parameter value from string. */
  template <typename T>
  void paramStringToVal(std::string const & valstring, T & result);

   //! Machine-readable output to a string, for use in jevois::Parameter: outputs [\c Element1, \c Element2, ...]
   /*! Specialization for std::vector<T> which supports nesting of other vectors and vectors of other types  */
  template <typename Q>
  void paramValToString(std::vector<Q> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads [\c Element1, \c Element2, ...]
  /*! Specialization for std::vector<T> which supports nesting of other vectors and vectors of other types */
  template <typename Q>
  void paramStringToVal(std::string const & valstring, std::vector<Q> & result);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs \c first \c second
  template <typename F, typename S>
  void paramValToString(std::pair<F, S> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads \c first \c second
  template <typename F, typename S>
  void paramStringToVal(std::string const & valstring, std::pair<F, S> & result);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs \c x \c y
  template <typename T>
  void paramValToString(cv::Point_<T> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads \c x \c y
  template <typename T>
  void paramStringToVal(std::string const & valstring, cv::Point_<T> & result);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs \c width \c height
  template <typename T>
  void paramValToString(cv::Size_<T> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads \c width \c height
  template <typename T>
  void paramStringToVal(std::string const & valstring, cv::Size_<T> & result);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs \c r \c g \c b \c a
  template <typename T>
  void paramValToString(cv::Scalar_<T> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads \c r \c g \c b \c a or subsets
  template <typename T>
  void paramStringToVal(std::string const & valstring, cv::Scalar_<T> & result);

  //! Machine-readable output to a string, for use in jevois::Parameter: outputs [\c Key1:Value1, \c Key2:Value2, ...]
   /*! Specialization for std::map which supports nesting of other maps/vectors */
  template <typename K, typename V>
  void paramValToString(std::map<K,V> const & val, std::string & result);

  //! Machine-readable input from a string, for use in jevois::Parameter: reads [\c Key1:Value1, \c Key2:Value2, ...]
  /*! Specialization for std::map which supports nesting of other maps/vectors */
  template <typename K, typename V>
  void paramStringToVal(std::string const & valstring, std::map<K,V> & result);

  //! Machine-readable conversion of bool to a string, for use in jevois::Parameter
  /*! @returns "true" or "false". */
  template <>
  void paramValToString<bool>(bool const & val, std::string & result);

  //! Machine-readable conversion from string to bool, for use in jevois::Parameter
  /*! @throws jevois::exception::Exception if input string is malformed.

      Specialization for bool so that we can accept nice verbose terms rather than just 0 and 1.  Acceptable ways to set
      a bool Parameter are:

      - "true" or "false"
      - "t" or "f"
      - "yes" or "no"
      - "y" or "n"
      - 1 or 0
      - An empty string will set the Parameter to true, so you may use boolean parameters as switches, e.g.,
        <code>"--myFlag"</code> is equivalent to <code>"--myFlag=true"</code> */
  template <>
  void paramStringToVal<bool>(std::string const & valstring, bool & result);

  //! Machine-readable conversion of string to a string, for use in jevois::Parameter
  /*! This implementation just returns the original string. */
  template <>
  void paramValToString<std::string>(std::string const & val, std::string & result);

  //! Machine-readable conversion from string to string, for use in jevois::Parameter
  /*! This implementation just returns the original string. */
  template <>
  void paramStringToVal<std::string>(std::string const & valstring, std::string & result);

  /*! @} */ // **********************************************************************

} // namespace jevois

//! Include implementation details of no interest to the user
#include <jevois/Component/details/ParameterStringConversionImpl.H>


