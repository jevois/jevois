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

#include <jevois/Component/ParameterRegistry.H>

namespace jevois
{
  //! Generic variadic template class template definition for Component Parameters
  /*! \see Parameter<Param, Tail ...> for detailed documentation. */
  template <class ... Params> class Parameter;

  //! Special case to terminate variadic template inheritance recursion
  /*! \see Parameter<Param, Tail ...> for detailed documentation. */
  template <> class Parameter<> { };

} // namespace jevois

// We do not define these macros when compiling the doxygen doc so that parameter declarations in components and modules
// will be documented as related functions. Just remember to comment each parameter declaration with a relates doxygen
// tag that relates it to the host component.

#ifndef JEVOIS_DOXYGEN

// BEGIN_JEVOIS_CODE_SNIPPET parametermacros.C

// Convenience macro to define a Parameter type. All the ... args are passed to ParameterDef
#define JEVOIS_DECLARE_PARAMETER(ParamName, ParamType, ...)             \
  struct ParamName : public virtual jevois::ParameterRegistry, public jevois::ParameterCore<ParamType> \
  {                                                                     \
    typedef ParamType type;                                             \
                                                                        \
    inline ParamName() : jevois::ParameterCore<ParamType>(jevois::ParameterDef<ParamType>(#ParamName, __VA_ARGS__)) \
    { jevois::ParameterRegistry::addParameter(this); }                  \
                                                                        \
    inline virtual ~ParamName()                                         \
    { jevois::ParameterRegistry::removeParameter(this); }               \
                                                                        \
    inline virtual jevois::Component const * owner() const override     \
    { return dynamic_cast<jevois::Component const *>(static_cast<jevois::ParameterRegistry const *>(this)); } \
  };

// Convenience macro to define a Parameter type with callback. All the ... args are passed to ParameterDef
#define JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(ParamName, ParamType, ...) \
  struct ParamName : public virtual jevois::ParameterRegistry, public jevois::ParameterCore<ParamType> \
  {                                                                     \
    typedef ParamType type;                                             \
                                                                        \
    virtual void onParamChange(ParamName const & param, ParamType const & newval) = 0; \
                                                                        \
    inline ParamName() : jevois::ParameterCore<ParamType>(jevois::ParameterDef<ParamType>(#ParamName, __VA_ARGS__)) \
    { setCallback([this](ParamType const & newval) { this->onParamChange(*this, newval); }); \
      jevois::ParameterRegistry::addParameter(this); }                  \
                                                                        \
    inline virtual ~ParamName()                                         \
    { jevois::ParameterRegistry::removeParameter(this); }               \
                                                                        \
    inline virtual jevois::Component const * owner() const override     \
    { return dynamic_cast<jevois::Component const *>(static_cast<jevois::ParameterRegistry const *>(this)); } \
  };

// END_JEVOIS_CODE_SNIPPET

#endif // JEVOIS_DOXYGEN

