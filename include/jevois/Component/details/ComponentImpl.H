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
#include <jevois/Debug/Log.H>

// ######################################################################
// ######################################################################
// ######################################################################
template <class Comp, typename... Args> inline
std::shared_ptr<Comp> jevois::Component::addSubComponent(std::string const & instance, Args && ...args)
{
  JEVOIS_TRACE(5);

  // Enforce that Comp derives from Component:
  static_assert(std::is_base_of<jevois::Component, Comp>::value, "Comp must derive from jevois::Component");

  std::shared_ptr<Comp> subComp;
  {
    // Lock up so we guarantee the instance name does not get robbed as we add the sub:
    boost::unique_lock<boost::shared_mutex> ulck(itsSubMtx);

    // Create the sub and put it into a shared_ptr. Note: computeInstanceName() will throw if name clashes with some
    // existing sub-component:
    subComp.reset(new Comp(computeInstanceName(instance, jevois::demangledName<Comp>()), std::forward<Args>(args)...));

    // Then add it as a sub-component to us, if there is not instance name clash with our other sub-components:
    LDEBUG("Adding SubComponent [" << jevois::demangledName<Comp>() << ":: " << instance << ']');
    itsSubComponents.push_back(subComp);
    subComp->itsParent = this;

    // By default, inherit the path from the parent:
    subComp->setPath(absolutePath());
  }

  // Finally bring it to our runstate:
  if (itsInitialized) subComp->init();

  return subComp;
}

// ######################################################################
template <class Comp> inline
std::shared_ptr<Comp> jevois::Component::getSubComponent(std::string const & instance) const
{
  JEVOIS_TRACE(6);

  // Enforce that Comp derives from Component:
  static_assert(std::is_base_of<jevois::Component, Comp>::value, "Comp must derive from jevois::Component");

  boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);

  for (std::shared_ptr<jevois::Component> c : itsSubComponents)
    if (c->instanceName() == instance)
    {
      std::shared_ptr<Comp> ret = std::dynamic_pointer_cast<Comp>(c);
      if (ret) return ret; // correct type
      LFATAL("SubComponent [" << instance << "] is not of type [" << jevois::demangledName<Comp>() << ']');
    }

  LFATAL("SubComponent [" << instance << "] not found");
}

// ######################################################################
// Specialization for jevois::Component return: no need to cast
namespace jevois
{
  template <> inline
  std::shared_ptr<Component> Component::getSubComponent<Component>(std::string const & instance) const
  {
    ///JEVOIS_TRACE(6);

    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);

    for (std::shared_ptr<Component> c : itsSubComponents)
      if (c->instanceName() == instance) return c;

    LFATAL("SubComponent [" << instance << "] not found");
  }
}

// ######################################################################
template <class Comp>
void jevois::Component::removeSubComponent(std::shared_ptr<Comp> & component)
{
  JEVOIS_TRACE(5);

  static_assert(std::is_base_of<jevois::Component, Comp>::value, "Comp must derive from jevois::Component");

  boost::upgrade_lock<boost::shared_mutex> uplck(itsSubMtx);

  for (auto itr = itsSubComponents.begin(); itr != itsSubComponents.end(); ++itr)
    if (itr->get() == component.get())
    {
      // First, nuke the "component" shared_ptr so it does not show up as a dangling shared_ptr in
      // doRemoveSubComponent(). itr still has one ref to the pointee:
      component.reset();

      // Then remove that sub:
      doRemoveSubComponent(itr, uplck, "SubComponent");
      return;
    }

  LERROR("SubComponent [" << component->instanceName() << "] not found. Ignored.");
}


// ######################################################################
template <typename T> inline
std::vector<std::string>
jevois::Component::setParamVal(std::string const & descriptor, T const & val)
{
  JEVOIS_TRACE(7);

  std::vector<std::string> ret;
  findParamAndActOnIt(descriptor,

                      [&ret,&val](jevois::ParameterBase * param, std::string const & unrolled)
                      {
                        jevois::ParameterCore<T> * p = dynamic_cast<jevois::ParameterCore<T> *>(param);
                        if (p == nullptr) throw std::range_error("Attempted to set Parameter [" + unrolled +
                                                                 "] with value of incorrect type");
                        p->set(val);
                        ret.push_back(unrolled);
                      },

                      [&ret]() { return ret.empty(); }
                      );
  return ret;
}

// ######################################################################
template <typename T> inline
void jevois::Component::setParamValUnique(std::string const & descriptor, T const & val)
{
  JEVOIS_TRACE(7);

  std::vector<std::string> ret = setParamVal(descriptor, val);
  if (ret.size() > 1) throw std::range_error("Multiple matches for descriptor [" + descriptor +
                                             "] while only one is allowed");
}

// ######################################################################
template <typename T> inline
std::vector<std::pair<std::string, T> >
jevois::Component::getParamVal(std::string const & descriptor) const
{
  JEVOIS_TRACE(8);

  std::vector<std::pair<std::string, T> > ret;
  findParamAndActOnIt(descriptor,

                      [&ret](jevois::ParameterBase * param, std::string const & unrolled)
                      {
                        jevois::ParameterCore<T> * p = dynamic_cast<jevois::ParameterCore<T> *>(param);
                        if (p == nullptr) throw std::range_error("Attempted to get Parameter [" + unrolled +
                                                                 "] with value of incorrect type");
                        ret.push_back(std::make_pair(unrolled, p->get()));
                      },

                      [&ret]() { return ret.empty(); }
                      );
  return ret;
}

// ######################################################################
template <typename T> inline
T jevois::Component::getParamValUnique(std::string const & descriptor) const
{
  JEVOIS_TRACE(8);

  std::vector<std::pair<std::string, T> > ret = getParamVal<T>(descriptor);
  if (ret.size() > 1) throw std::range_error("Multiple matches for descriptor [" + descriptor +
                                             "] while only one is allowed");
  return ret[0].second;
}

// Include inlined implementation details that are of no interest to the end user
#include <jevois/Component/details/ParameterImpl.H>

