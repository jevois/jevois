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

#include <map>
#include <string>
#include <boost/thread/shared_mutex.hpp>

namespace jevois
{
  class Component;
  class ParameterBase;

  //! A simple registry of all parameters associated with a Component
  /*! This registry allows one to loop over or search for given parameters in a Component. \ingroup parameter */
  class ParameterRegistry
  {
    public:
      //! Virtual destructor for safe inheritance
      virtual ~ParameterRegistry();

    protected:
      //! The Parameter class uses this method to register itself on construction with its owning Component
      void addParameter(ParameterBase * const param);

      //! The Parameter class uses this method to un-register itself on destruction with its owning Component
      void removeParameter(ParameterBase * const param);

      //! For all parameters that have a callback which has never been called, call it with the default param value
      void callbackInitCall();

    private:
      //! Allow Component and DynamicParameter to access our registry data, everyone else is locked out
      friend class Component;
      template <typename T> friend class DynamicParameter;
      
      // A list of pointers to all Parameters registered with this Component
      std::map<std::string, ParameterBase *> itsParameterList;

      // Mutex to protect our list of parameters
      mutable boost::shared_mutex itsParamMtx;
  };

} // namespace jevois
