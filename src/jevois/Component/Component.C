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

#include <jevois/Debug/Log.H>
#include <jevois/Component/Component.H>
#include <jevois/Component/Manager.H>
#include <jevois/Component/Parameter.H>
#include <jevois/Util/Utils.H>

#include <fstream>
#include <algorithm> // for std::all_of

// ######################################################################
jevois::Component::Component(std::string const & instanceName) :
    itsInstanceName(instanceName), itsInitialized(false), itsParent(nullptr), itsPath()
{
  JEVOIS_TRACE(5);
}

// ######################################################################
std::string const & jevois::Component::className() const
{
  boost::shared_lock<boost::shared_mutex> lck(itsMetaMtx);

  // We need the (derived!) component to be fully constructed for demangle to work, hence the const_cast here:
  if (itsClassName.empty()) *(const_cast<std::string *>(&itsClassName)) = jevois::demangle(typeid(*this).name());

  return itsClassName;
}

// ######################################################################
std::string const & jevois::Component::instanceName() const
{ return itsInstanceName; }

// ######################################################################
jevois::Component::~Component()
{
  JEVOIS_TRACE(5);

  LDEBUG("Deleting Component");

  // Recursively un-init us and our subs; call base class version as derived classes are destroyed:
  if (itsInitialized) jevois::Component::uninit();

  // All right, we need to nuke our subs BEFORE we get destroyed, since they will backflow to us (for param
  // notices, recursive descriptor access, etc):
  boost::upgrade_lock<boost::shared_mutex> uplck(itsSubMtx);

  while (itsSubComponents.empty() == false)
  {
    auto itr = itsSubComponents.begin();
    doRemoveSubComponent(itr, uplck, "SubComponent");
  }
}

// ######################################################################
void jevois::Component::removeSubComponent(std::string const & instanceName, bool warnIfNotFound)
{
  JEVOIS_TRACE(5);

  boost::upgrade_lock<boost::shared_mutex> uplck(itsSubMtx);

  for (auto itr = itsSubComponents.begin(); itr != itsSubComponents.end(); ++itr)
    if ((*itr)->instanceName() == instanceName)
    {
      // All checks out, get doRemoveSubComponent() to do the work:
      doRemoveSubComponent(itr, uplck, "SubComponent");
      return;
    }

  if (warnIfNotFound) LERROR("SubComponent [" << instanceName << "] not found. Ignored.");
}

// ######################################################################
void jevois::Component::doRemoveSubComponent(std::vector<std::shared_ptr<jevois::Component> >::iterator & itr,
                                          boost::upgrade_lock<boost::shared_mutex> & uplck,
                                          std::string const & displayname)
{
  JEVOIS_TRACE(5);

  // Try to delete and let's check that it will actually be deleted:
  std::shared_ptr<jevois::Component> component = *itr;

  LDEBUG("Removing " << displayname << " [" << component->descriptor() << ']');

  // Un-init the component:
  if (component->initialized()) component->uninit();

  // Remove it from our list of subs:
  boost::upgrade_to_unique_lock<boost::shared_mutex> ulck(uplck);
  itsSubComponents.erase(itr);

  if (component.use_count() > 1)
    LERROR(component.use_count() - 1 << " additional external shared_ptr reference(s) exist to "
                << displayname << " [" << component->descriptor() << "]. It was removed but NOT deleted.");

  component.reset(); // nuke the shared_ptr, this should yield a delete unless use_count was > 1
}

// ######################################################################
bool jevois::Component::isTopLevel() const
{
  JEVOIS_TRACE(6);

  boost::shared_lock<boost::shared_mutex> lck(itsMtx);

  if (dynamic_cast<jevois::Manager *>(itsParent) != nullptr) return true;

  return false;
}

// ######################################################################
void jevois::Component::init()
{
  JEVOIS_TRACE(5);

  if (itsInitialized) { LERROR("Already initialized. Ignored."); return; }

  LDEBUG("Initializing...");

  runPreInit();
  setInitialized();
  runPostInit();

  LDEBUG("Initialized.");
}

// ######################################################################
void jevois::Component::runPreInit()
{
  JEVOIS_TRACE(6);

  // Pre-init all subComponents:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->runPreInit();
  }
  
  // Then us. So the last one here will be the manager, and its preInit() will parse the command line:
  preInit();

  // If we have some parameters with callbacks that have not been set expicitly by the command-line, call the callback a
  // first time here. This may add some new parameters
  ParameterRegistry::callbackInitCall();
}

// ######################################################################
void jevois::Component::setInitialized()
{
  JEVOIS_TRACE(6);

  // First all subComponents
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->setInitialized();
  }
  
  // Then us:
  itsInitialized = true;
}

// ######################################################################
void jevois::Component::runPostInit()
{
  JEVOIS_TRACE(6);

  // First all subComponents:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->runPostInit();
  }
  
  // Then us:
  postInit();
}

// ######################################################################
bool jevois::Component::initialized() const
{
  JEVOIS_TRACE(6);

  return itsInitialized;
}

// ######################################################################
void jevois::Component::uninit()
{
  JEVOIS_TRACE(5);

  if (itsInitialized)
  {
    LDEBUG("Uninitializing...");

    runPreUninit();
    setUninitialized();
    runPostUninit();

    LDEBUG("Uninitialized.");
  }
}

// ######################################################################
void jevois::Component::runPreUninit()
{
  JEVOIS_TRACE(6);

  // First all subComponents:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->runPreUninit();
  }
  
  // Then us:
  preUninit();
}

// ######################################################################
void jevois::Component::setUninitialized()
{
  JEVOIS_TRACE(6);

  // First us:
  itsInitialized = false;

  // Then all subComponents
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->setUninitialized();
  }
}

// ######################################################################
void jevois::Component::runPostUninit()
{
  JEVOIS_TRACE(6);

  // First us:
  postUninit();

  // Then all subComponents:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->runPostUninit();
  }
}

// ######################################################################
std::string jevois::Component::descriptor() const
{
  JEVOIS_TRACE(8);

  // Top-level components or those with no parent just return their instance name. Sub-components return a chain of
  // component instance names up to the top level:

  boost::shared_lock<boost::shared_mutex> lck(itsMtx);

  if (itsParent && dynamic_cast<jevois::Manager *>(itsParent) == nullptr)
    return itsParent->descriptor() + ':' + itsInstanceName;

  return itsInstanceName;
}

// ######################################################################
void jevois::Component::findParamAndActOnIt(std::string const & descrip,
                                            std::function<void(jevois::ParameterBase *, std::string const &)> doit,
                                            std::function<bool()> empty) const
{
  JEVOIS_TRACE(9);

  // Split this parameter descriptor by single ":" (skipping over all "::")
  std::vector<std::string> desc = jevois::split(descrip, ":" /*"FIXME "(?<!:):(?!:)" */);

  if (desc.empty()) std::range_error(descriptor() + ": Cannot parse empty parameter name");

  // Recursive call with the vector of tokens:
  findParamAndActOnIt(desc, true, 0, "", doit);

  if (empty()) throw std::range_error(descriptor() + ": No Parameter named [" + descrip + ']');
}

// ######################################################################
void jevois::Component::findParamAndActOnIt(std::vector<std::string> const & descrip,
                                         bool recur, size_t idx, std::string const & unrolled,
                                         std::function<void(jevois::ParameterBase *, std::string const &)> doit) const
{
  JEVOIS_TRACE(9);

  // Have we not yet reached the bottom (still have some component names before the param)?
  if (descrip.size() > idx + 1)
  {
    // We have some token before the param, is it a '*', in which case we turn on recursion?
    if (descrip[idx] == "*") { recur = true; ++idx; }
    else {
      // We have some Instance specification(s) of component(s) before the param. Let's see if we match against the
      // first one.  If it's a match, eat up the first token and send the rest of the tokens to our subcomponents,
      // otherwise keep the token and recurse the entire list to the subs:
      if (itsInstanceName == descrip[idx]) { recur = false; ++idx; }
    }
  }

  // Have we reached the end of the list (the param name)?
  if (descrip.size() == idx + 1)
  {
    // We have just a paramname, let's see if we have that param:
    boost::shared_lock<boost::shared_mutex> lck(itsParamMtx);

    for (auto const & p : itsParameterList)
      if (p.second->name() == descrip[idx])
      {
        // param name is a match, act on it:
        std::string ur = itsInstanceName + ':' + p.second->name();
        if (unrolled.empty() == false) ur = unrolled + ':' + ur;
        doit(p.second, ur);
      }
  }

  // Recurse through our subcomponents if recur is on or we have not yet reached the bottom:  
  if (recur || descrip.size() > idx + 1)
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);

    std::string ur;
    if (unrolled.empty()) ur = itsInstanceName; else ur = unrolled + ':' + itsInstanceName;

    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->findParamAndActOnIt(descrip, recur, idx, ur, doit);
  }
}

// ######################################################################
std::vector<std::string> jevois::Component::setParamString(std::string const & descriptor, std::string const & val)
{
  JEVOIS_TRACE(7);

  std::vector<std::string> ret;
  findParamAndActOnIt(descriptor,

                      [&ret,&val](jevois::ParameterBase * param, std::string const & unrolled)
                      { param->strset(val); ret.push_back(unrolled); },

                      [&ret]() { return ret.empty(); }
                      );
  return ret;
}

// ######################################################################
void jevois::Component::setParamStringUnique(std::string const & descriptor, std::string const & val)
{
  JEVOIS_TRACE(7);

  std::vector<std::string> ret = setParamString(descriptor, val);
  if (ret.size() > 1)
    throw std::range_error("Multiple matches for descriptor [" + descriptor + "] while only one is allowed");
}

// ######################################################################
std::vector<std::pair<std::string, std::string> >
jevois::Component::getParamString(std::string const & descriptor) const
{
  JEVOIS_TRACE(8);

  std::vector<std::pair<std::string, std::string> > ret;
  findParamAndActOnIt(descriptor,

                      [&ret](jevois::ParameterBase * param, std::string const & unrolled)
                      { ret.push_back(std::make_pair(unrolled, param->strget())); },

                      [&ret]() { return ret.empty(); }
                      );
  return ret;
}

// ######################################################################
std::string jevois::Component::getParamStringUnique(std::string const & descriptor) const
{
  JEVOIS_TRACE(8);

  std::vector<std::pair<std::string, std::string> > ret = getParamString(descriptor);
  if (ret.size() > 1)
    throw std::range_error("Multiple matches for descriptor [" + descriptor + "] while only one is allowed");
  return ret[0].second;
}

// ######################################################################
void jevois::Component::freezeParam(std::string const & paramdescriptor)
{
  int n = 0;
  findParamAndActOnIt(paramdescriptor,
                      [&n](jevois::ParameterBase * param, std::string const & JEVOIS_UNUSED_PARAM(unrolled))
                      { param->freeze(); ++n; },

                      [&n]() { return (n == 0); }
                      );
}

// ######################################################################
void jevois::Component::unFreezeParam(std::string const & paramdescriptor)
{
  int n = 0;
  findParamAndActOnIt(paramdescriptor,
                      [&n](jevois::ParameterBase * param, std::string const & JEVOIS_UNUSED_PARAM(unrolled))
                      { param->unFreeze(); ++n; },

                      [&n]() { return (n == 0); }
                      );
}

// ######################################################################
void jevois::Component::freezeAllParams()
{
  boost::shared_lock<boost::shared_mutex> lck(itsParamMtx);

  for (auto const & p : itsParameterList) p.second->freeze();
}

// ######################################################################
void jevois::Component::unFreezeAllParams()
{
  boost::shared_lock<boost::shared_mutex> lck(itsParamMtx);

  for (auto const & p : itsParameterList) p.second->unFreeze();
}

// ######################################################################
void jevois::Component::setParamsFromFile(std::string const & filename)
{
  std::string const absfile = absolutePath(filename);
  std::ifstream ifs(absfile);
  if (!ifs) LFATAL("Could not open file " << absfile);
  setParamsFromStream(ifs, absfile);
}

// ######################################################################
std::istream & jevois::Component::setParamsFromStream(std::istream & is,std::string const & absfile)
{
  size_t linenum = 1;
  for (std::string line; std::getline(is, line); /* */)
  {
    // Skip over comments:
    if (line.length() && line[0] == '#') { ++linenum; continue; }
    
    // Skip over empty lines:
    if (std::all_of(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); })) { ++linenum; continue; }
    
    // Parse descriptor=value:
    size_t idx = line.find('=');
    if (idx == line.npos) LFATAL("No '=' symbol found at line " << linenum << " in " << absfile);
    if (idx == 0) LFATAL("No parameter descriptor found at line " << linenum << " in " << absfile);
    if (idx == line.length() - 1) LFATAL("No parameter value found at line " << linenum << " in " << absfile);

    std::string desc = line.substr(0, idx);
    std::string val = line.substr(idx + 1);

    // Be nice and clean whitespace at start and end (not in the middle):
    while (desc.length() > 0 && std::isspace(desc[0])) desc.erase(0, 1);
    while (desc.length() > 0 && std::isspace(desc[desc.length()-1])) desc.erase(desc.length()-1, 1);
    if (desc.empty()) LFATAL("Invalid blank parameter descriptor at line " << linenum << " in " << absfile);

    while (val.length() > 0 && std::isspace(val[0])) val.erase(0, 1);
    while (val.length() > 0 && std::isspace(val[val.length()-1])) val.erase(val.length()-1, 1);
    if (val.empty()) LFATAL("Invalid blank parameter value at line " << linenum << " in " << absfile);

    // Ok, set that param:
    setParamString(desc, val);
    
    ++linenum;
  }
  return is;
}

// ######################################################################
void jevois::Component::setPath(std::string const & path)
{
  JEVOIS_TRACE(5);

  // First all subComponents:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->setPath(path);
  }
 
  itsPath = path;
}

// ######################################################################
std::string jevois::Component::absolutePath(std::string const & path)
{
  JEVOIS_TRACE(6);

  // If path is empty, return itsPath (be it empty of not):
  if (path.empty()) return itsPath;

  // no-op if the given path is already absolute:
  if (path[0] == '/') return path;

  // no-op if itsPath is empty:
  if (itsPath.empty()) return path;

  // We know itsPath is not empty and path does not start with a / and is not empty; concatenate both:
  return itsPath + '/' + path;
}

// ######################################################################
void jevois::Component::populateHelpMessage(std::string const & cname,
                                         std::unordered_map<std::string,
                                         std::unordered_map<std::string,
                                         std::vector<std::pair<std::string, std::string> > > > & helplist,
                                         bool recurse) const
{
  JEVOIS_TRACE(9);

  std::string const compname = cname.empty() ? itsInstanceName : cname + ':' + itsInstanceName;

  // First add our own params:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsParamMtx);
    for (auto const & p : itsParameterList)
    {
      jevois::ParameterSummary const ps = p.second->summary();

      if (ps.frozen) continue; // skip frozen parameters
      
      std::string const key1 = ps.category + ":  "+ ps.categorydescription;
      std::string const key2 = "  --" + ps.name + " (" + ps.valuetype + ") default=[" + ps.defaultvalue + "]" +
        (ps.validvalues == "None:[]" ? "\n" : " " + ps.validvalues + "\n") + "    " + ps.description;
      std::string val = "";
      if (ps.value != ps.defaultvalue) val = ps.value;
      helplist[key1][key2].push_back(std::make_pair(compname, val));
    }
  }

  // Then recurse through our subcomponents:
  if (recurse)
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->populateHelpMessage(compname, helplist);
  }
}

// ######################################################################
std::string jevois::Component::computeInstanceName(std::string const & instance, std::string const & classname) const
{
  JEVOIS_TRACE(9);

  std::string inst = instance;

  // If empty instance, replace it by the classname:
  if (inst.empty())
  {
    // Use the class name:
    inst = classname + '#';

    // Remove any namespace:: prefix:
    size_t const idxx = inst.rfind(':'); if (idxx != inst.npos) inst = inst.substr(idxx + 1);
  }

  // Replace all # characters by some number, if necessary:
  std::vector<std::string> vec = jevois::split(inst, "#");
  if (vec.size() > 1)
  {
    // First try to have no numbers in there:
    inst = jevois::join(vec, ""); bool found = false;
    for (std::shared_ptr<Component> const & c : itsSubComponents)
      if (c->instanceName() == inst) { found = true; break; }

    if (found)
    {
      // Ok, we have some conflict, so let's add some numbers where the # signs were:
      inst = "";
      for (std::string const & v : vec)
      {
        if (v.empty()) continue;

        inst += v;
        size_t largestId = 1;

        while (true)
        {
          std::string stem = inst + std::to_string(largestId);
          bool gotit = false;

          for (std::shared_ptr<Component> const & c : itsSubComponents)
            if (c->instanceName() == stem) { gotit = true; break; }

          if (gotit == false) { inst = stem; break; }

          ++largestId;
        }
      }
    }

    LDEBUG("Using automatic instance name [" << inst << ']');
    return inst;
  }

  // If we have not returned yet, no # was given. Throw if there is a conflict:
  for (std::shared_ptr<Component> const & c : itsSubComponents)
    if (c->instanceName() == inst)
      throw std::runtime_error("Provided instance name [" + instance + "] clashes with existing sub-components.");

  return inst;
}      

