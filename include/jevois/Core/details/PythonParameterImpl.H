// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 20122by Laurent Itti, the University of Southern
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

#pragma once

#include <jevois/Core/PythonSupport.H>

// ####################################################################################################
template <typename T> inline
std::shared_ptr<jevois::python::PyParHelperBase>
jevois::python::PyParHelper<T>::create(jevois::Component * comp, std::string const & name,
                                       std::string const & description, boost::python::object const & defaultValue,
                                       jevois::ParameterCategory const & category)
{
  return std::make_shared<jevois::python::PyParHelper<T>>(comp, name, description, defaultValue, category);
}

// ####################################################################################################
template <typename T> inline
jevois::python::PyParHelper<T>::PyParHelper(jevois::Component * comp, std::string const & name,
                                            std::string const & description,
                                            boost::python::object const & defaultValue,
                                            jevois::ParameterCategory const & category) :
    jevois::python::PyParHelperBase(comp)
{
  itsParam = comp->addDynamicParameter<T>(name, description, jevois::python::pyextract<T>(defaultValue), category);
}

// ####################################################################################################
template <typename T> inline
jevois::python::PyParHelper<T>::~PyParHelper()
{
  comp()->removeDynamicParameter(itsParam->name());
}

// ####################################################################################################
template <typename T> inline
boost::python::object jevois::python::PyParHelper<T>::get()
{
  return jevois::python::topyobj(itsParam->get());
}

// ####################################################################################################
template <typename T> inline
void jevois::python::PyParHelper<T>::set(boost::python::object const & val)
{
  itsParam->set(jevois::python::pyextract<T>(val));
}

// ####################################################################################################
template <typename T> inline
std::shared_ptr<jevois::ParameterBase> jevois::python::PyParHelper<T>::par() const
{
  return itsParam;
}

// ####################################################################################################
template <typename T> inline
void jevois::python::PyParHelper<T>::setCallback(boost::python::object const & cb)
{
  itsPyCallback = cb;
  itsParam->setCallback([this](T const & newval) { itsPyCallback(boost::python::object(newval)); } );

  try
  {
    // Call the callback one first time, using the current value of the parameter:
    itsParam->set(itsParam->get());
  }
  catch (...) { jevois::warnAndRethrowException(); }
}
