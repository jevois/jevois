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

#include <jevois/Debug/Log.H>

// ######################################################################
template <class Comp, typename... Args> inline
std::shared_ptr<Comp> jevois::Manager::addComponent(std::string const & instance, Args && ...args)
{
  // Keep this code in sync with that in Engine.C!
  
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
    LDEBUG("Adding Component [" << subComp->instanceName() << ']');
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
std::shared_ptr<Comp> jevois::Manager::getComponent(std::string const & instance) const
{
  // Keep this code in sync with module version

  // Enforce that Comp derives from Component:
  static_assert(std::is_base_of<jevois::Component, Comp>::value, "Comp must derive from jevois::Component");

  boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);

  for (std::shared_ptr<jevois::Component> c : itsSubComponents)
    if (c->instanceName() == instance)
    {
      std::shared_ptr<Comp> ret = std::dynamic_pointer_cast<Comp>(c);
      if (ret) return ret; // correct type
      LFATAL("Component [" << instance << "] is not of type [" << jevois::demangledName<Comp>() << ']');
    }

  LFATAL("Component [" << instance << "] not found");
}

// ######################################################################
// Specialization for jevois::Component return: no need to cast
#ifndef JEVOIS_DOXYGEN
namespace jevois
{
  template <> inline
  std::shared_ptr<Component> Manager::getComponent<Component>(std::string const & instance) const
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);

    for (std::shared_ptr<Component> c : itsSubComponents)
      if (c->instanceName() == instance) return c;

    LFATAL("Component [" << instance << "] not found");
  }
}
#endif

// ######################################################################
template <class Comp>
void jevois::Manager::removeComponent(std::shared_ptr<Comp> & component)
{
  static_assert(std::is_base_of<jevois::Component, Comp>::value, "Comp must derive from jevois::Component");

  boost::upgrade_lock<boost::shared_mutex> uplck(itsSubMtx);

  for (auto itr = itsSubComponents.begin(); itr != itsSubComponents.end(); ++itr)
    if (itr->get() == component.get())
    {
      // First, nuke the "component" shared_ptr so it does not show up as a dangling shared_ptr in
      // doRemoveSubComponent(). itr still has one ref to the pointee:
      component.reset();

      // Then remove that sub:
      doRemoveSubComponent(itr, uplck, "Component");
      return;
    }

  LERROR("Component [" << component->instanceName() << "] not found. Ignored.");
}
