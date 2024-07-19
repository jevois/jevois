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

#include <sstream>
#include <jevois/Component/ParameterStringConversion.H>

// ######################################################################
// ######################################################################
// ######################################################################
template <typename T> inline
jevois::ValidValuesSpecBase<T>::ValidValuesSpecBase()
{ }

template <typename T> inline
jevois::ValidValuesSpecBase<T>::~ValidValuesSpecBase()
{ }

// ######################################################################
// ######################################################################
// ######################################################################
template <typename T> inline
jevois::ValidValuesSpecNone<T>::ValidValuesSpecNone() : ValidValuesSpecBase<T>()
{ }

template <typename T> inline
jevois::ValidValuesSpecNone<T>::~ValidValuesSpecNone()
{ }

template <typename T> inline
bool jevois::ValidValuesSpecNone<T>::checkValueValidity(T const &) const
{ return true; }

template <typename T> inline
std::string const jevois::ValidValuesSpecNone<T>::str() const
{ return "None:[]"; }

// ######################################################################
// ######################################################################
// ######################################################################
template <typename T> inline
jevois::ValidValuesSpecList<T>::ValidValuesSpecList(std::vector<T> const & valid_list) :
    ValidValuesSpecBase<T>(), itsValidList(valid_list)
{ }

template <typename T> inline
jevois::ValidValuesSpecList<T>::~ValidValuesSpecList()
{ }

template <typename T> inline
bool jevois::ValidValuesSpecList<T>::checkValueValidity(T const & val) const
{
  for (T const & v : itsValidList) if (val == v) return true;
  return false;
}

template <typename T> inline
std::string const jevois::ValidValuesSpecList<T>::str() const
{
  std::ostringstream os;
  os << "List:[";
  for (T const & v : itsValidList) os << v << '|';
  os.seekp(-1, std::ios_base::end); os << ']';
  return os.str();
}

// ######################################################################
// ######################################################################
// ######################################################################
template <typename T> inline
jevois::ValidValuesSpecRange<T>::ValidValuesSpecRange(jevois::Range<T> const & valid_range) :
    ValidValuesSpecBase<T>(), itsValidRange(valid_range)
{ }

template <typename T> inline
jevois::ValidValuesSpecRange<T>::~ValidValuesSpecRange()
{ }

template <typename T> inline
bool jevois::ValidValuesSpecRange<T>::checkValueValidity(T const & val) const
{
  return (val >= itsValidRange.min() && val <= itsValidRange.max());
}

template <typename T> inline
std::string const jevois::ValidValuesSpecRange<T>::str() const
{
  std::ostringstream os;
  os << "Range:" << itsValidRange;
  return os.str();
}

// ######################################################################
// ######################################################################
// ######################################################################
template <typename T> inline
jevois::ValidValuesSpecStepRange<T>::ValidValuesSpecStepRange(jevois::StepRange<T> const & valid_range) :
    ValidValuesSpecBase<T>(), itsValidStepRange(valid_range)
{ }

template <typename T> inline
jevois::ValidValuesSpecStepRange<T>::~ValidValuesSpecStepRange()
{ }

template <typename T> inline
bool jevois::ValidValuesSpecStepRange<T>::checkValueValidity(T const & val) const
{
  return itsValidStepRange.isValueValid(val);
}

template <typename T> inline
std::string const jevois::ValidValuesSpecStepRange<T>::str() const
{
  std::ostringstream os;
  os << "StepRange:" << itsValidStepRange;
  return os.str();
}

// ######################################################################
// ######################################################################
// ######################################################################
template <typename T> inline
jevois::ValidValuesSpecRegex<T>::ValidValuesSpecRegex(boost::regex const & valid_regex) :
    ValidValuesSpecBase<T>(), itsValidRegex(valid_regex)
{ }

template <typename T> inline
jevois::ValidValuesSpecRegex<T>::~ValidValuesSpecRegex()
{ }

template <typename T> inline
bool jevois::ValidValuesSpecRegex<T>::checkValueValidity(T const & val) const
{
  std::string v; jevois::paramValToString(val, v);
  return boost::regex_match(v, itsValidRegex);
}

template <typename T> inline
std::string const jevois::ValidValuesSpecRegex<T>::str() const
{
  std::ostringstream os;
  os << "Regex:[" << itsValidRegex.str() << ']';
  return os.str();
}
