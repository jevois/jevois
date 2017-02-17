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

#include <jevois/Util/Demangle.H>

// ######################################################################
inline jevois::ParameterBase::ParameterBase() :
    itsFrozen(false), itsVirgin(true)
{ }

// ######################################################################
inline jevois::ParameterBase::~ParameterBase()
{
  // unlock the mutex that was locked by Parameter<T> destructor as destroying a locked mutex is evil
  itsMutex.unlock();
}

// ######################################################################
inline void jevois::ParameterBase::freeze()
{ itsFrozen = true; }

// ######################################################################
inline void jevois::ParameterBase::unFreeze()
{ itsFrozen = false; }

// ######################################################################
template <typename T> inline
jevois::ParameterCore<T>::ParameterCore(jevois::ParameterDef<T> const & def) :
    jevois::ParameterBase(), itsCallback(), itsVal(def.defaultValue()), itsDef(def)
{ }

// ######################################################################
template <typename T> inline
jevois::ParameterCore<T>::~ParameterCore()
{
  // Get a unique lock into our mutex, to ensure all get(), callbacks, etc are done before we die:
  itsMutex.lock(); // keep us locked up into oblivion, base destructor will unlock at the last moment
}

// ######################################################################
template <typename T> inline
std::string const & jevois::ParameterCore<T>::name() const
{ return itsDef.name(); }

// ######################################################################
template <typename T> inline
std::string jevois::ParameterCore<T>::descriptor() const
{
  jevois::Component const * comp = owner();
  if (comp) return comp->descriptor() + ':' + itsDef.name(); else return itsDef.name();
}

// ######################################################################
template <typename T> inline
T jevois::ParameterCore<T>::get() const
{
  boost::shared_lock<boost::shared_mutex> lck(itsMutex);
  return itsVal;
}

// ######################################################################
template <typename T> inline
void jevois::ParameterCore<T>::setCallback(std::function<void(T const &)> cb)
{
  boost::unique_lock<boost::shared_mutex> lck(itsMutex);
  itsCallback = cb;
}

// ######################################################################
template <typename T> inline
void jevois::ParameterCore<T>::set(T const & newVal)
{
  boost::upgrade_lock<boost::shared_mutex> lck(itsMutex);

  // If we are frozen, reject the change:
  if (itsFrozen)
  {
    std::ostringstream os;
    os << descriptor() << ": Provided value [" << newVal << "] rejected because Parameter is frozen";
    throw std::range_error(os.str());
  }

  
  // Is the value authorized, depending on the type of valid values we have:
  if (itsDef.itsValidValuesSpec->checkValueValidity(newVal) == false)
  {
    std::ostringstream os;
    os << descriptor() << ": Provided value [" << newVal << "] not valid according to specification "
       << itsDef.itsValidValuesSpec->str();
    throw std::range_error(os.str());
  }

  // If we have a callback, see whether it likes this change:
  if (itsCallback)
  {
    try { itsCallback(newVal); }
    catch (std::exception const & e)
    {
      std::ostringstream os;
      os << descriptor() << ": Provided value [" << newVal << "] rejected by callback with reason [" << e.what() << ']';
      throw std::range_error(os.str());
    }
    catch (...)
    {
      std::ostringstream os;
      os << descriptor() << ": Provided value [" << newVal << "] rejected by callback with unknown reason";
      throw std::range_error(os.str());
    }

    // We have been set at least once now:
    itsVirgin = false;
  }

  // If we made it through, all is ok. Change the value:
  {
    boost::upgrade_to_unique_lock<boost::shared_mutex> ulock(lck);
    itsVal = newVal;
  }
}

// ######################################################################
template <typename T> inline
void jevois::ParameterCore<T>::strset(std::string const & valstring)
{ jevois::ParameterCore<T>::set(jevois::detail::paramValFromString<T>(valstring, this)); }

// ######################################################################
template <typename T> inline
std::string const jevois::ParameterCore<T>::strget() const
{ return jevois::detail::paramStringFromVal(jevois::ParameterCore<T>::get(), this); }

// ######################################################################
template <typename T> inline
void jevois::ParameterCore<T>::changeParameterDef(jevois::ParameterDef<T> const & def)
{
  boost::unique_lock<boost::shared_mutex> ulck(itsMutex);
  itsVal = def.defaultValue();
  *(const_cast<jevois::ParameterDef<T> *>(& itsDef)) = def;
}

// ######################################################################
template <typename T> inline
void jevois::ParameterCore<T>::callbackInitCall()
{
  boost::shared_lock<boost::shared_mutex> _(itsMutex);

  // If we have a callback, call it with our current value:
  if (itsCallback && itsVirgin)
  {
    try { itsCallback(itsVal); }
    catch (std::exception const & e)
    {
      std::ostringstream os;
      os << descriptor() << ": Value [" << itsVal << "] rejected by callback with reason [" << e.what() << ']';
      throw std::range_error(os.str());
    }
    catch (...)
    {
      std::ostringstream os;
      os << descriptor() << ": Value [" << itsVal << "] rejected by callback with unknown reason";
      throw std::range_error(os.str());
    }
  }
}

// ######################################################################
template <typename T> inline
jevois::ParameterSummary const jevois::ParameterCore<T>::summary() const
{
  jevois::ParameterSummary ps;

  boost::shared_lock<boost::shared_mutex> lck(itsMutex);

  ps.descriptor = descriptor();
  ps.name = itsDef.name();
  ps.description = itsDef.description();
  ps.valuetype = jevois::demangledName<T>();
  ps.defaultvalue = itsDef.defaultValueString();
  ps.value = strget();
  ps.validvalues = itsDef.validValuesString();
  ps.category = itsDef.category().name;
  ps.categorydescription = itsDef.category().description;
  ps.frozen = itsFrozen;
  
  return ps;
}
