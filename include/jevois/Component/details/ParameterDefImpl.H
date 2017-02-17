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

#include <jevois/Component/ParameterStringConversion.H>
#include <jevois/Util/Demangle.H>
#include <sstream>

namespace jevois
{
  namespace detail
  {
    // ######################################################################
    //! Conversions to/from string with exception wrapping
    template <class T> inline
    T const paramValFromString(std::string const & valstring, jevois::ParameterBase const * param)
    {
      T val;
      try
      {
        jevois::paramStringToVal(valstring, val);
      }
      catch (std::exception & e)
      {
        throw std::range_error(param->descriptor() + ": Failed to convert [" + valstring + "] to type [" +
                               jevois::demangledName<T>() + "]: " + e.what());
      }
      return val;
    }

    // ######################################################################
    template <class T> inline
    std::string const paramStringFromVal(T const & val, jevois::ParameterBase const * param)
    {
      std::string valstring;
      try
      {
        jevois::paramValToString(val, valstring);
      }
      catch (std::exception & e)
      {
        throw std::range_error(param->descriptor() + ": Failed to convert value of type [" +
                               jevois::demangledName<T>() + "] to string: " + e.what());
      }
      return valstring;
    }
  } // namespace detail
} // namespace jevois

// ######################################################################
// ######################################################################
// ######################################################################
template <class T> inline
jevois::ParameterDef<T>::ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                                      jevois::ParameterCategory const & category) :
    ParameterDefBase(name, description, category), itsDefaultValue(defaultValue),
    itsValidValuesSpec(new jevois::ValidValuesSpecNone<T>()) 
{ }

// ######################################################################
template <class T> template <template <typename> class ValidValuesSpec> inline
jevois::ParameterDef<T>::ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                                      ValidValuesSpec<T> const & validValuesSpec,
                                      jevois::ParameterCategory const & category) :
    ParameterDefBase(name, description, category), itsDefaultValue(defaultValue),
    itsValidValuesSpec(new ValidValuesSpec<T>(validValuesSpec))
{
  checkDefaultValue();
}

// ######################################################################
template <class T> inline
jevois::ParameterDef<T>::ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                                      std::vector<T> const & validvalues, jevois::ParameterCategory const & category) :
    ParameterDefBase(name, description, category), itsDefaultValue(defaultValue),
    itsValidValuesSpec(new jevois::ValidValuesSpecList<T>(validvalues))
{
  checkDefaultValue();
}

// ######################################################################
template <class T> inline
jevois::ParameterDef<T>::ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                                      jevois::Range<T> const & validrange, jevois::ParameterCategory const & category) :
    ParameterDefBase(name, description, category), itsDefaultValue(defaultValue),
    itsValidValuesSpec(new jevois::ValidValuesSpecRange<T>(validrange))
{
  checkDefaultValue();
}

// ######################################################################
template <class T> inline
jevois::ParameterDef<T>::ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                                      jevois::StepRange<T> const & validrange,
                                      jevois::ParameterCategory const & category) :
    ParameterDefBase(name, description, category), itsDefaultValue(defaultValue),
    itsValidValuesSpec(new jevois::ValidValuesSpecStepRange<T>(validrange))
{
  checkDefaultValue();
}

// ######################################################################
template <class T> inline
jevois::ParameterDef<T>::ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                                      boost::regex const & validregex, jevois::ParameterCategory const & category) :
    ParameterDefBase(name, description, category), itsDefaultValue(defaultValue),
    itsValidValuesSpec(new jevois::ValidValuesSpecRegex<T>(validregex))
{
  checkDefaultValue();
}

// ######################################################################
template <class T> inline
void jevois::ParameterDef<T>::checkDefaultValue()
{
  // Enforce that the default value is one of the valid ones:
  if (itsValidValuesSpec->checkValueValidity(itsDefaultValue) == false)
  {
    // ooops, given default value is not valid, let's throw:
    std::ostringstream os;
    os << "Provided default value [" << itsDefaultValue << "] for ParameterDef [" << itsName
       << "] not valid according to specification " << itsValidValuesSpec->str();
    throw std::range_error(os.str());
  }
}

// ######################################################################
template <class T> inline
T const jevois::ParameterDef<T>::defaultValue() const
{ return itsDefaultValue; }

// ######################################################################
template <class T> inline
std::string const jevois::ParameterDef<T>::defaultValueString() const
{ return jevois::detail::paramStringFromVal(itsDefaultValue, nullptr); }

// ######################################################################
template <class T> inline
std::string const jevois::ParameterDef<T>::validValuesString() const
{ return itsValidValuesSpec->str(); }


