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

#include <jevois/Component/Component.H>
#include <jevois/Types/Enum.H>

#include <vector>

// BEGIN_JEVOIS_CODE_SNIPPET manager1.C
namespace jevois
{
  namespace manager
  {
    //! Parameter category \relates jevois::Manager
    static ParameterCategory const ParamCateg("General Options");

    //! Parameter \relates jevois::Manager
    JEVOIS_DECLARE_PARAMETER(help, bool, "Print this help message", false, ParamCateg);

#ifdef JEVOIS_LDEBUG_ENABLE
    //! Enum for Parameter \relates jevois::Manager
    JEVOIS_DEFINE_ENUM_CLASS(LogLevel, (fatal) (error) (info) (debug));
#else
    //! Enum for Parameter \relates jevois::Manager
    JEVOIS_DEFINE_ENUM_CLASS(LogLevel, (fatal) (error) (info));
#endif

    //! Parameter \relates jevois::Manager
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(loglevel, LogLevel, "Set the minimum log level to display",
                                           LogLevel::info, LogLevel_Values, ParamCateg);

    //! Parameter \relates jevois::Manager
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(tracelevel, unsigned int, "Set the minimum trace level to display",
                                           0, ParamCateg);
  }

  // ######################################################################
  //! Manager of a hierarchy of Component objects
  /*! A Manager should be the top-level Component of any hierarchy of Components.  It is primarily responsible for
      handling the setting of Parameter values via the command-line or otherwise.

      Users should only need to construct a Manager (including Engine, which derives from Manager), add any Component to
      it, and then call init() on the Manager, which will parse all command line options, bind them to the relevant
      Parameters, and call init() on all subComponents (which in turn calls init() on all of their subComponents,
      etc.). See the documentation of Component for more information about the init() flow.

      \ingroup component */
  class Manager : public Component,
                  public Parameter<manager::help, manager::loglevel, manager::tracelevel>
  {
    public:
      // END_JEVOIS_CODE_SNIPPET
      
      //! @name Manager construction, destruction, parsing command-line arguments
      //! @{

      //! Constructor without command-line args
      /*! The command-line args should be passed later using setArgs(), before you init(), otherwise the manager will
          issue a non-fatal warning. */
      Manager(std::string const & instance = "TheManager");

      //! Constructor
      /*! Creates the Manager, and optionally takes in the command line arguments. */
      Manager(int argc, char const* argv[], std::string const & instance = "TheManager");

      //! Set the command-line arguments, call this before start() if args were not passed at construction
      void setCommandLineArgs(int argc, char const* argv[]);

      //! Destructor
      virtual ~Manager();

      //! @}
      
      //! @name Component hierarchies unser the Manager
      //! @{

      //! Pseudo-constructor: construct a top-level Component
      /*! A component of type Comp (which must derive from jevois::Component) will be created and added as a
          sub-component of the manager (making it a so-called top-level component). The child logically "belongs" to the
          Manager, and will automatically be deleted when the Manager is deleted. In addition to construction, the
          component will be brought to the same initialized state as the Manager. */
      template <class Comp, typename... Args>
      std::shared_ptr<Comp> addComponent(std::string const & instanceName, Args && ...args);

      //! Use addComponent() on the Manager as opposed to jevois::Component::addSubComponent()
      template <class Comp, typename... Args>
      std::shared_ptr<Comp> addSubComponent(std::string const & instanceName, Args && ...args) = delete;

      //! Remove a top-level Component from the Manager, by shared_ptr
      /*! \note Beware that the passed shared_ptr is invalidated in the process. A warning is issued if the use_count is
          not down to zero after that (i.e., there are additional shared_ptr pointers to this Component floating around,
          which prevent it from actually being deleted. */
      template <class Comp>
      void removeComponent(std::shared_ptr<Comp> & component);

      //! Use removeComponent() on the Manager as opposed to jevois::Component::removeSubComponent()
      template <class Comp>
      void removeSubComponent(std::shared_ptr<Comp> & component) = delete;

      //! Remove a top-level Component from the Manager, by instance name
      void removeComponent(std::string const & instanceName, bool warnIfNotFound = true);

      //! Use removeComponent() on the Manager as opposed to jevois::Component::removeSubComponent()
      void removeSubComponent(std::string const & instanceName, bool warnIfNotFound) = delete;

      //! Get a top-level component by instance name
      /*! This method does a dynamic_pointer_cast to Comp if it is not the default (jevois::Component). Throws if
          component is not found by instance name, or it is found but not of type Comp (if Comp is specified). Note that
          once you hold a shared_ptr to a Component, it is guaranteed that the component will not be destroyed until
          that shared_ptr is released. If the JeVois system tries to destroy the component, e.g., someone calls
          removeComponent(), the component will be un-initialized and its parent will be unset, so it will not be
          fully operational, and will be actually deleted when the last shared_ptr to it runs out of scope. */
      template <class Comp = jevois::Component>
      std::shared_ptr<Comp> getComponent(std::string const & instanceName) const;

      //! Use getComponent() on the Manager as opposed to jevois::Component::getSubComponent()
      template <class Comp>
      std::shared_ptr<Comp> getSubComponent(std::string const & instanceName) const = delete;

      //! @}

      //! @name Manager runtime
      //! @{

      //! Users can call init() on the Manager to initialize the whole Component tree
      /*! This will, in particular, trigger parsing of command-line arguments. */
      using Component::init;

      //! Users can call uninit() on the Manager to un-initialize the whole Component tree
      /*! If started(), this will first call stop() */
      using Component::uninit;

      //! Get the remaining arguments that were not parsed by the command line
      /*! Any command line arguments after a lone '--' will be available in remainingArgs() */
      std::vector<std::string> const & remainingArgs() const;

      //! @}
      
    protected:
      // BEGIN_JEVOIS_CODE_SNIPPET manager2.C

      //! Parameter callback
      void onParamChange(manager::loglevel const & param, manager::LogLevel const & newval);

      //! Parameter callback
      void onParamChange(manager::tracelevel const & param, unsigned int const & newval);
      // END_JEVOIS_CODE_SNIPPET
      
      //! Calls parseCommandLine()
      void preInit() override;

      //! Checks for the --help flag
      void postInit() override;

      //! Constructs a help message from all parameters in the model, and outputs it to 'out'
      void constructHelpMessage(std::ostream & out) const;

      //! Constructs a help message and tries to send it to /usr/bin/less
      void printHelpMessage() const;

    private:
      //! Parse the command line and internally store the resulting remaining args
      void doParseCommandLine();

      //! Parses the command line, and applies settings to all Parameters
      /*! See the definition of Parameter for details on how to specify them from the command line. */
      std::vector<std::string> const parseCommandLine(std::vector<std::string> const & commandLineArgs);

      //! The original command line arguments
      std::vector<std::string> itsCommandLineArgs;

      //! Did we get command-line args (even though there may be none, just to detect if forgot)
      bool itsGotArgs;

      //! Any command line arguments not used by the model
      std::vector<std::string> itsRemainingArgs;
  };
} // namespace jevois

// Include implementation details
#include <jevois/Component/details/ManagerImpl.H>
