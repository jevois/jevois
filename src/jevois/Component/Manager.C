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

#include <jevois/Component/Manager.H>
#include <jevois/Component/Parameter.H>
#include <jevois/Debug/Log.H>
#include <unordered_map>
#include <fstream>

// ######################################################################
jevois::Manager::Manager(std::string const & instanceID) :
    jevois::Component(instanceID), itsGotArgs(false)
{ }

// ######################################################################
jevois::Manager::Manager(int argc, char const* argv[], std::string const & instanceID) :
    jevois::Component(instanceID), itsCommandLineArgs((char const **)(argv), (char const **)(argv+argc)),
    itsGotArgs(true)
{ }

// ######################################################################
void jevois::Manager::setCommandLineArgs(int argc, char const* argv[])
{
  itsCommandLineArgs = std::vector<std::string>((char const **)(argv), (char const **)(argv+argc));
  itsGotArgs = true;
}

// ######################################################################
jevois::Manager::~Manager()
{ }

// ######################################################################
void jevois::Manager::preInit()
{
  if (itsGotArgs == false)
    LERROR("No command-line arguments given; did you forget to call jevois::Manager::setArgs()?");
  
  if (itsCommandLineArgs.size() > 0) itsRemainingArgs = parseCommandLine(itsCommandLineArgs);
}

// ######################################################################
void jevois::Manager::postInit()
{
  // Note: exit() tries to uninit which can deadlock since we are in init() here...
  //if (jevois::manager::help::get()) { printHelpMessage(); std::abort(); /*exit(0);*/ } // yes this is brutal!

  if (jevois::manager::help::get()) { printHelpMessage(); LINFO("JeVois: exit after help message"); exit(0); }

  // The --help parameter is only useful for parsing of command-line arguments. After that is done, we here hide it as
  // we will instead provide a 'help' command:
  help::freeze(true);

  // Do not confuse users with a non-working tracelevel parameter if tracing has not been compiled in:
#if !defined(JEVOIS_TRACE_ENABLE) || !defined(JEVOIS_LDEBUG_ENABLE)
  tracelevel::freeze(true);
#endif
}

// ######################################################################
void jevois::Manager::printHelpMessage() const
{
  constructHelpMessage(std::cout);
}

// ######################################################################
void jevois::Manager::constructHelpMessage(std::ostream & out) const
{
  std::unordered_map<std::string, // category:description
                     std::unordered_map<std::string, // --name (type) default=[def]
                                        std::vector<std::pair<std::string, // component name
                                                              std::string  // current param value
                                                              > > > > helplist;
  // First our own options, excluding our subs:
  this->populateHelpMessage("", helplist, false);

  // Then all our components/modules, we call them directly instead of just recursing down from us so that the manager
  // name is omitted from all descriptors:
  {
    boost::shared_lock<boost::shared_mutex> lck(itsSubMtx);
    for (std::shared_ptr<jevois::Component> c : itsSubComponents) c->populateHelpMessage("", helplist);
  }

  // Helplist should never be empty since the Manager has options, but in any case...
  if (helplist.empty()) { out << "NO PARAMETERS."; return; }

  out << "PARAMETERS:" << std::endl << std::endl;

  for (auto & c : helplist)
  {
    // Print out the category name and description
    out << c.first << std::endl;

    // Print out the parameter details
    for (auto const & n : c.second)
    {
      out << n.first << std::endl;

      // Print out the name of each component that exports this parameter definition, but strip the manager's name for
      // brevity, unless that's the only thing in the descriptor:
      out << "       Exported By: ";
      for (auto const & cp : n.second)   // pair: <component, value>
      {
        out << cp.first; // component descriptor
        if (cp.second.empty() == false) out << " value=[" << cp.second << ']';  // value
        if (cp != *(n.second.end()-1)) out << ", ";
      }

      out << std::endl;
      out << std::endl;
    }
    out << std::endl;
  }
  out << std::flush;
}

// ######################################################################
std::vector<std::string> const jevois::Manager::parseCommandLine(std::vector<std::string> const & commandLineArgs)
{
  // Start by pushing the program name into remaining args
  std::vector<std::string> remainingArgs;
  remainingArgs.push_back(commandLineArgs[0]);

  // process all the -- args, push other things into remaining args:
  std::vector<std::string>::const_iterator argIt;
  for (argIt = commandLineArgs.begin() + 1; argIt != commandLineArgs.end(); ++argIt)
  {
    // All arguments should start with "--", store as remaining arg anything that does not:
    if (argIt->length() < 2 || (*argIt)[0] != '-' || (*argIt)[1] != '-') { remainingArgs.push_back(*argIt); continue; }

    // If the argument is just a lone "--", then we are done with command line parsing:
    if (*argIt == "--") break;

    // Split the string by "=" to separate the parameter name from the value
    size_t const equalsPos = argIt->find_first_of('=');
    if (equalsPos < 3) LFATAL("Cannot parse command-line argument with no name [" << *argIt << ']');

    std::string const parameterName  = argIt->substr(2, equalsPos - 2);
    std::string const parameterValue = (equalsPos == std::string::npos) ? "true" : argIt->substr(equalsPos + 1);

    // Set the parameter recursively, will throw if not found, and here we allow multiple matches and set all matching
    // parameters to the given value:
    setParamString(parameterName, parameterValue);
  }

  // Add anything after a lone -- to the remaining args:
  while (argIt != commandLineArgs.end()) { remainingArgs.push_back(*argIt); ++argIt; }

  return remainingArgs;
}

// ######################################################################
std::vector<std::string> const & jevois::Manager::remainingArgs() const
{ return itsRemainingArgs; }

// ######################################################################
void jevois::Manager::removeComponent(std::string const & instance, bool warnIfNotFound)
{
  // Keep this code in sync with Componnet::removeSubComponent

  boost::upgrade_lock<boost::shared_mutex> uplck(itsSubMtx);

  for (auto itr = itsSubComponents.begin(); itr != itsSubComponents.end(); ++itr)
    if ((*itr)->instanceName() == instance)
    {
      doRemoveSubComponent(itr, uplck, "Component");
      return;
    }

  if (warnIfNotFound) LERROR("Component [" << instance << "] not found. Ignored.");
}
// BEGIN_JEVOIS_CODE_SNIPPET manager3.C

// ######################################################################
void jevois::Manager::onParamChange(jevois::manager::loglevel const &, jevois::manager::LogLevel const & newval)
{ 
  switch(newval)
  {
  case jevois::manager::LogLevel::fatal: jevois::logLevel = LOG_CRIT; break;
  case jevois::manager::LogLevel::error: jevois::logLevel = LOG_ERR; break;
  case jevois::manager::LogLevel::info: jevois::logLevel = LOG_INFO; break;
#ifdef JEVOIS_LDEBUG_ENABLE
  case jevois::manager::LogLevel::debug: jevois::logLevel = LOG_DEBUG; break;
#endif
  }
}

// ######################################################################
void jevois::Manager::onParamChange(jevois::manager::tracelevel const &, unsigned int const & newval)
{
  if (newval)
  {
#if !defined(JEVOIS_TRACE_ENABLE) || !defined(JEVOIS_LDEBUG_ENABLE)
    LERROR("Debug trace has been disabled at compile-time, re-compile with -DJEVOIS_LDEBUG_ENABLE=ON and "
           "-DJEVOIS_TRACE_ENABLE=ON to see trace info");
#endif
  }
  
  jevois::traceLevel = newval;
}

// END_JEVOIS_CODE_SNIPPET
