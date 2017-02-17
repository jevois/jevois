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

#include <jevois/Component/ParameterRegistry.H>
#include <jevois/Component/Parameter.H>
#include <jevois/Debug/Log.H>

// ######################################################################
jevois::ParameterRegistry::~ParameterRegistry()
{ }

// ######################################################################
void jevois::ParameterRegistry::addParameter(jevois::ParameterBase * const param)
{
  boost::upgrade_lock<boost::shared_mutex> uplck(itsParamMtx);
  if (itsParameterList.find(param->name()) != itsParameterList.end())
    LFATAL("Duplicate Parameter Name: " << param->name());

  boost::upgrade_to_unique_lock<boost::shared_mutex> ulck(uplck);
  itsParameterList[param->name()] = param;

  LDEBUG("Added Parameter [" << param->name() << ']');
}

// ######################################################################
void jevois::ParameterRegistry::removeParameter(jevois::ParameterBase * const param)
{
  boost::upgrade_lock<boost::shared_mutex> uplck(itsParamMtx);
  auto itr = itsParameterList.find(param->name());

  if (itr == itsParameterList.end())
    LERROR("Parameter " << param->name() << " not owned by this component -- NOT REMOVED");
  else
  {
    boost::upgrade_to_unique_lock<boost::shared_mutex> ulck(uplck);
    itsParameterList.erase(itr);
  }

  LDEBUG("Removed Parameter [" << param->name() << ']');
}

// ######################################################################
void jevois::ParameterRegistry::callbackInitCall()
{
  boost::shared_lock<boost::shared_mutex> _(itsParamMtx);

  for (auto const & pl : itsParameterList) pl.second->callbackInitCall();
}
