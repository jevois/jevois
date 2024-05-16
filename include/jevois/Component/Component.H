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

#include <jevois/Component/Parameter.H>
#include <boost/thread.hpp>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <future>
#include <filesystem>

#include <jevois/Util/Async.H> // actually not needed here, but many derived components and modules will want to use it

namespace jevois
{
  class Manager;
  class Engine;
  class UserInterface;
  
  /*! \defgroup component Model components, parameters, manager, and associated classes

      These classes define how a complex model or vision processing pipeline with user-tunable parameters can be built
      from components.

      How to explore this documentation module:

      - Start with looking at Component, which highlights the broad conceptual framework of how one may assemble complex
        vision processing pipelines in JeVois, with substantial re-use of shared algorithmic elements.

      - Then have a look a \ref parameter which provides details on how to implement and use parameters in your
        components. Parameters can be created for any valid C++ type, and serve the goal of allowing end-users of your
        algorithm to tune some of the behavior of your algorithms.

      - Finally have a look at Manager, which is a special component that is the root of a component hierarchy.

      - You will then be ready to move to the next level by looking at the other elements in the documentation
        module.

      \ingroup core */

  // ######################################################################
  //! A component of a model hierarchy.
  /*! Model Components form the heart of every JeVois application, as their main purpose is to allow 1) building complex
      vision processing pipelines from shared, re-usable processing elements; 2) a uniform interface for tunable
      parameters that these processing elements may have. In fact, every class which needs to expose parameters to the
      user should inherit directly or indirectly from Component. Parameters are simple wrappers around any valid C++
      type, along with some description, default values, possibly specification of valid values, etc. Parameters are
      used to adjust the behavior of processing elements by allowing end-users to change the parameter values (think,
      e.g., of a threshold in some image processing algorithm).

      Components typically form a component hierarchy with a Manager (including Engine) at the root, and a tree of
      sub-Components below. To form this tree, just use the Manager::addComponent() and Component::addSubComponent()
      methods to add some children, and then add some children to those children, etc...

      One intent behind Component is to enable substantial code re-use when implementing complex vision processing
      frameworks, and to alleviate the burden of passing many parameters to function calls when invoking the processing
      elements. This is a strong departure from many other frameworks such as OpenCV, which by and large rely on
      functions with many parameters (though later versions do use classes as well). For example, the way one invokes a
      Canny edge detector in OpenCV is to call the function:

      \code
      void Canny(InputArray image, OutputArray edges, double threshold1, double threshold2,
                 int apertureSize = 3, bool L2gradient = false)
      \endcode

      Beyond possible confusion about which value goes to which argument in the long list (which languages such as
      Python solve by allowing access to arguments by name), one major issue with this approach is that either every
      function using Canny must provide a mechanism for the user to set the parameters (threshold1, threshold2, etc),
      or, in most cases, those will just end up being hardwired, limiting the applicability of the end application to
      different image sizes, environment types, etc.

      In contrast, in the JeVois framework, one would create a Canny Component, with Parameter settings for the
      thresholds, etc and a member function

      \code
      OutputArray process(InputArray in)
      \endcode

      One would typically first set the parameter values, then call the function. Setting the parameters can be done by
      the code that will use the function, but, more often, it is left to the user. In a particular vision pipeline,
      resonable default values may be provided for the parameters at the beginning, then leaving those parameters
      accessible to end users who may want to modify them. Modification of parameters in JeVois is handled either at the
      start of the application by parsing command-line arguments, when a new processing Module is instantiated, or while
      it is running, by interacting with the Engine that manages the system via its Serial ports.

      Any complex vision processing pipeline that includes the Canny component in its hierarchy will expose its
      parameters so that they can be set, either by users (via command-line arguments, commands issued over serial
      ports, etc), or by other components in the hierarchy. If multiple instances of a Component are present in a
      hierarchy, their parameters can either all be set at once to a single (shared) value, or they can be accessed and
      set individually to different values (each Component has a unique instance name, and a so-called descriptor is
      created by concatenating instance names of a component and all its parents; parameters of a specific Component
      instance can be accessed by prefixing that instance's descriptor to the parameter name). Parameters provide a
      uniform interface for setting/getting the parameter values, so that programmers do not have to program accessor
      functions in each of their components for each of their parameters(e.g., adding a Parameter<int> for threshold1 to
      your component will automatically add set(), get(), etc functions for it into your component).


      In addition, Component provides some level of introspection or reflection: one can access a component instance
      name, class name, descriptor that includes names of all its parents in the hierarchy, and iterate over its
      parameters, over its sub-components, etc.

      Component is inherently thread-safe and the component interface is written so as to encourage thread-safe use; for
      example, no function is provided that returns the parent of a component, as another thread could change that
      between the time that parent is returned and the time it is used. Instead, functions are provided that can
      traverse the hierarchy up or down in a thread-safe manner and execute some action on the components that are
      encountered during that traversal.

      Components are brought into action in several phases:

      - Construction: In constructors, Parameter values have not been set yet so nothing that depends on them should be
        initialized yet. Typically, constructors should only initialize fixed component resources which are independent
        of Parameter values, or they can also set default values for parameters, which may later be overriden by
        command-line arguments. In many situations, the constructor would do nothing, and most of the initialization
        work would be delayed until the Parameter values have been set, during the component's init() phase.

      - initialization: When init() is called on the manager (including Engine), it propagates down the entire Component
        hierarchy, as follows:

        - recursively call preInit() on all sub-components.
        - run preInit() on the current component (the preInit() function can be overriden by derived classes to
          implement some custom initialization; note that Parameter values are not yet finalized at this point and
          should not be used yet).
        - Manager then parses the command-line and possibly sets parameters. Parameters are considered ready to use from
          this point on.
        - for parameters of this component that have a callback but it has not yet been invoked because that parameter
          has not been explicitly set at the command line, call the callback one first time with the default parameter
          value. This is so that callbacks that create resources (e.g., callback for a parameter for the device name of
          a device, which opens the device when called) are called at least once before init() is complete.
        - recursively flip the state of all sub-components to initialized, then flip the component state to initialized.
        - recursively run postInit() on all sub-components (derived classes may override postInit(), and would typically
          allocate resources that depend on parameter values in their override of postInit()), then on the current
          component.

        The initialization flow is fixed. Classes that derive from Component can override preInit() for intializations
        that do not rely on Parameter values and postInit() for those that do. Once init() is complete on the Manager,
        all components in the hierarchy are considered ready for operation.

        If a component is added to an already-initialized hierarchy, it will be brought up to the same init state as its
        parent at the time of addition. This is typically the case when one selects a new video format from a USB host
        on a JeVois system that is already running and streaming video. The Module handling the old format is furst
        un-initialized by Engine, then it is destroyed, the new Module is instantiated for the new format, and it is
        initialized before the stream of video frames is directed to it.

      - uninit: This is the dual of init(), with preUninit() and postUninit() following the same logic as preInit() and
        postInit(). Specific order is:

        - preUninit() is recursively called on all sub-component then on the component (note that initialized() is still
          true at this point);
        - component flips to un-initalized, then recursivey all its sub-components
        - postUninit() is called on component, then recursively on all its sub-components.

      Note how both init() and uninit() are private in Component, and thus cannot be called directly. Manager (and hence
      Engine which derives from Manager) makes them public so that you can call them on the manager. That is, your
      entire hierarchy is either initialized or not.
      
      Because powering up the JeVois hardware platform automatically constructs and initializes the default processing
      pipeline, and because of the aforementioned was in which Engine changes processing modules when users change video
      format, most components just assume that their vision processing functions will never be called when the component
      is not initialized, and thus they just skip testing for initialized() altogether (to save CPU cycles).

      \ingroup component */
  class Component : public virtual ParameterRegistry
  {
    public:
      //protected: // FIXME inherited constructor does not compile if protected!
      //! Constructor
      /*! The standard way to create a component is via Component::addSubComponent() or Manager::addComponent(), rather
          than constructing them by hand. Components constructed via the constructor (e.g., calling operator new) will
          not be attached to a Component hierarchy. It is recommended that derived components also have an instance
          constructor argument and pass it down to the base Component class, to allow complex systems with several
          instances of a same Component. In fact, for most components, the inherited constructor is appropriate:

          \code
          class MyComp : public jevois::Component
          {
          public:
            // Inherited constructor
            using jevois::Component::Component;

            // Virtual destructor for safe inheritance
            virtual ~MyComp();
          };
          \endcode */
      Component(std::string const & instance);

      //! Virtual destructor for safe inheritance
      /*! Calls uninit() if component is initialized. */
      virtual ~Component();

      //! @name Component hierarchies
      //! @{

      //! Pseudo-constructor: construct and add another component as a subcomponent of this one
      /*! A child component of type Comp (which must derive from jevois::Component) will be created and added as a
          sub-component of this. The child logically "belongs" to this component, and will automatically be initialized,
          un-initialized, and deleted when the parent component is. In addition to construction, adding a subcomponent
          will bring it up to the same init() state as the owner component.

          When setting parameters, the sub-Component can be referenced as a child of this component. For instance, if
          we have a ComponentParent which has ComponentChild as a sub-Component, and ComponentChild has parameter named
          coolChildParam, then that parameter could be specified on the command line by

          \code
          --ComponentParentInstanceName:ComponentChildInstanceName:coolChildParamName="whatever"
          \endcode

          or over a Serial port by

          \code
          setpar ComponentParentInstanceName:ComponentChildInstanceName:coolChildParamName whatever
          \endcode

          \param instance A string identifying a particular instance of a Component, no two sub-components of a
          component may have the same instance name. The instance name should be passed in through your derived
          Component's constructor, allowing users to disambiguate between multiple instances of your Component.  If the
          instance name is empty, the actual instance will be named ComponentClassname# with # replaced (if necessary)
          by a unique number. If the instance name is not empty but contains a #, only that # is replaced (if necessary)
          by a number that makes the instance name unique. The final name is accessible via instanceName() once your
          component is constructed. There is no default value for instance in the base class to catch derived classes
          that forgot to pass it down to the base, but it may be a good idea to set an empty string default to instance
          in derived classes.

          \note Sub-components always inherit the path of their top-level parent (a top-level component is one that was
          directly added to a Manager via Manager::addComponent()). See absolutePath(). This is so that different
          modules can provide different data for their components; for example, a FaceDetector component may require
          some face template data files to operate; if those are loaded using a relative path name, different modules
          that use the FaceDetector may use different face template data files. */
      template <class Comp, typename... Args>
      std::shared_ptr<Comp> addSubComponent(std::string const & instance, Args && ...args);

      //! Remove a sub-Component from this Component, by shared_ptr
      /*! \note Beware that the passed shared_ptr is invalidated in the process. A warning is issued if the use_count is
          not down to zero after that (i.e., there are additional shared_ptr pointers to this Component floating around,
          which prevent it from actually being deleted. */
      template <class Comp>
      void removeSubComponent(std::shared_ptr<Comp> & component);

      //! Remove a sub-Component from this Component, by instance name
      void removeSubComponent(std::string const & instance, bool warnIfNotFound = true);

      //! Get a sub-component by instance name
      /*! This method does a dynamic_pointer_cast to Comp if it is not the default (jevois::Component). Throws if
          component is not found by instance name, or it is found but not of type Comp (if Comp is specified). Note that
          once you hold a shared_ptr to a Component, it is guaranteed that the component will not be destroyed until
          that shared_ptr is released. If the JeVois system tries to destroy the component (e.g., someone calls
          removeSubComponent()), the component will be un-initialized and its parent will be unset, so it will not be
          fully operational and will be actually deleted when the last shared_ptr to it runs out of scope. */
      template <class Comp = jevois::Component>
      std::shared_ptr<Comp> getSubComponent(std::string const & instance) const;

      //! Returns true if this component is top-level, i.e., its parent is jevois::Manager
      /*! The Module of Engine is top-level. */
      bool isTopLevel() const;

      //! Get a handle to our Engine, or throw if we do not have an Engine as root ancestor
      /*! Use with caution as this could break runtime loading/unloading of component hierarchies. */
      Engine * engine() const;
      
      //! @}
 
      //! @name Component runtime
      //! @{

      //! Has this component been initialized yet?
      bool initialized() const;

      //! @}

      //! @name Component metainfo-related functions
      //! @{

      //! The class name of this component
      std::string const & className() const;

      //! The instance name of this component
      std::string const & instanceName() const;

      //! @}

      /*! @name Component Parameter-related functions

          Each Component can hold Parameter objects (through inheritance) that can be set externally by users to modify
          the operation of the Component, and that can be accessed or set by the Component itself.

          Note how the JeVois framework uses inheritance for parameters as opposed to making them class data members of
          the owning Component. See \ref parameter for detailed explanations. */
      //! @{

      //! Set a parameter value
      /*! Throws if we don't have a parameter by that name or the value does not work out. Here is how the descriptor
          works:

          The basic format is

          \code
          [ComponentInstanceName]:[...]:[paramname]
          \endcode

          Note that any <code>[ComponentInstanceName]</code> token can also be replaced by a *, which is equivalent to
          any number of any ComponentInstanceName specifications. So use the * to reach parameters when you don't know
          how deep they are. If there is some other ComponentInstanceName between a * and the final paramname, then
          recursion is turned off and anything between the * and paramname must match intervening components/instances.

          For example,

          @code
          MyInst:*:CoolComp:MyParam
          @endcode

          would match parameters where MyInst matches the top-level component (the one on which you call setParamVal()),
          then would recurse through any number of subcomponents, until one of them matches CoolComp, and then we would
          look for MyParam parameter in that subcomponent, and we would not look any deeper.

          Finally note that there is an implicit first *: that is automatically prepended to your description, so if you
          just specify a paramname and nothing else before it, we will set all params by that name in all subcomponents
          no matter how deep they are (as if you had specified *:paramname).

          @throws std::exception if no Parameter matches the given descriptor.
          @return list of fully-unrolled (no '*') descriptors of the parameters that were matched and set. The list is
          guaranteed to have at least one element since we throw if no matching parameter is found. */
      template <typename T>
      std::vector<std::string> setParamVal(std::string const & paramdescriptor, T const & val);

      //! Set a parameter value, simple version assuming only one parameter match
      /*! This calls setParamVal(), and checks that exactly one match was found.
          @throws std::exception if not exactly one Parameter matches the given descriptor. */
      template <typename T>
      void setParamValUnique(std::string const & paramdescriptor, T const & val);

      //! Get parameter(s) value(s) by descriptor
      /*! Use this method to get the current value of a Component's parameter from the string descriptor. Values for all
          parameters that match the descriptor are returned.

          For example, if the class MyComponent has an integer parameter named "myparam" you could get the 
          value like so:

          @code
          std::shared_ptr<MyComponent> comp = addSubComponent<MyComponent>("mycomp");
          auto paramValues = comp->getParamVal<int>("myparam");
          for (auto const & pp : paramValues) LINFO("Parameter " << pp.first << " = " << pp.second);
          @endcode

          @throws jevois::exception::ParameterException if no Parameter matches the given descriptor.
          @return list of <paramdescriptor, value> for all parameters that matched the given descriptor. The list is
          guaranteed to have at least one element since we throw if no matching parameter is found.

          \see setParamVal for a detailed explanation of the paramdescriptor */
      template <typename T>
      std::vector<std::pair<std::string, T> > getParamVal(std::string const & paramdescriptor) const;

      //! Get a parameter value, simple version assuming only one parameter match
      /*! This calls getParamVal(), checks that exactly one match was found, and returns its value.
          For example, if the class MyComponent has an integer parameter named "myparam" you could get the 
          value like so:

          @code
          std::shared_ptr<MyComponent> comp(new MyComponent);
          int paramValue = comp->getParamValUnique<int>("myparam");
          @endcode

          @throws std::range_error if not exactly one Parameter matches the given descriptor. */
      template <typename T>
      T getParamValUnique(std::string const & paramdescriptor) const;

      //! Set a parameter value, by string
      /*! \see setParamVal for a detailed explanation of the paramdescriptor 
          @throws jevois::exception::ParameterException if no Parameter matches the given descriptor or if the
          given string cannot be converted to the Parameter's native type.
          @return list of fully-unrolled (no '*') descriptors of the parameters that were matched and set.  The list is
          guaranteed to have at least one element since we throw if no matching parameter is found. */
      std::vector<std::string> setParamString(std::string const & paramdescriptor, std::string const & val);

      //! Set a parameter value by string, simple version assuming only one parameter match
      /*! This calls setParamVal(), and checks that exactly one match was found.
          @throws std::range_error if not exactly one Parameter matches the given descriptor. */
      void setParamStringUnique(std::string const & paramdescriptor, std::string const & val);

      //! Get a parameter value, by string
      /*! \see setParamVal for a detailed explanation of the paramdescriptor 
          Use this method to get the current value of a Component's parameter from the string descriptor. Values for all
          parameters that match the descriptor are returned, as string.
          @throws jevois::exception::ParameterException if no Parameter matches the given descriptor.
          @return list of <paramdescriptor, valuestring> for all parameters that matched the given descriptor.  The list
          is guaranteed to have at least one element since we throw if no matching parameter is found. */
      std::vector<std::pair<std::string, std::string> > getParamString(std::string const & paramdescriptor) const;

      //! Get a parameter value by string, simple version assuming only one parameter match
      /*! This calls getParamVal(), checks that exactly one match was found, and returns its value as a string.
          @throws jevois::exception::ParameterException if not exactly one Parameter matches the given descriptor. */
      std::string getParamStringUnique(std::string const & paramdescriptor) const;

      //! Freeze a parameter, by name, see ParameterBase::freeze()
      void freezeParam(std::string const & paramdescriptor);

      //! Unfreeze a parameter, by name, see ParameterBase::unFreeze()
      void unFreezeParam(std::string const & paramdescriptor);

      //! Freeze all parameters
      void freezeAllParams();

      //! Unfreeze all parameters
      void unFreezeAllParams();
      
      //! Get our full descriptor (including all parents) as [Instancename]:[...]:[...]
      std::string descriptor() const;

      //! Set some parameters from a file
      /*! The file should have entries "descriptor=value", one per line, where the parameter descriptors should be
          relative to this component. If the file name is relative, our component path will be prefixed to it using
          absolutePath(). */
      void setParamsFromFile(std::string const & filename);

      //! Set some parameters from an open stream
      /*! The stream should have entries "descriptor=value", one per line, where the parameter descriptors should be
          relative to this component. absfile is only used for error messages and should be the absolute path of the
          file opened in 'is', or some other text for error messages. */
      std::istream & setParamsFromStream(std::istream & is, std::string const & absfile);

      //! Get machine-oriented descriptions of all parameters
      virtual void paramInfo(std::shared_ptr<UserInterface> s, std::map<std::string, std::string> & categs,
			     bool skipFrozen, std::string const & cname = "", std::string const & pfx = "");

      //! Run a function on every param we hold
      void foreachParam(std::function<void(std::string const & compname, ParameterBase * p)> func,
                        std::string const & cname = "");

      //! Add a new parameter after the Component has already been constructed
      /*! Dynamic parameters can only be accessed by descriptor using Component::setParamVal(),
          Component::getParamVal(), etc., since the owning component does not inherit from them like standard
          parameters. Callbacks can be added manually after creation using
          Component::setDynamicParameterCallback(). This version creates a Parameter with no given valid values, valid
          values are whatever T can take. */
      template <typename T>
      std::shared_ptr<DynamicParameter<T>>
      addDynamicParameter(std::string const & name, std::string const & description, T const & defaultValue,
                          ParameterCategory const & category);

      //! Add a new parameter after the Component has already been constructed
      /*! Dynamic parameters can only be accessed by descriptor, since the owning component does not inherit from them
          like standard parameters. Callbacks can be added manually after creation using
          Component::setDynamicParameterCallback(). This version creates a Parameter with specified valid values from a
          ValidValueSpec */
      template <typename T, template <typename> class ValidValuesSpec>
      std::shared_ptr<DynamicParameter<T>>
      addDynamicParameter(std::string const & name, std::string const & description, T const & defaultValue,
                          ValidValuesSpec<T> const & validValuesSpec, ParameterCategory const & category);

      //! Register a callback with a previously created dynamic parameter
      /*! If callnow is true, the callback will be called right after it is registered, using the parameter's current
          value. This is to mimic standard parameters where the callback is called at least once during init. */
      template <typename T>
      void setDynamicParameterCallback(std::string const & name, std::function<void(T const &)> cb,
                                       bool callnow = true);

      //! Remove a previously added dynamic parameter
      void removeDynamicParameter(std::string const & name, bool throw_if_not_found = true);
      
      //! @}

      /*! @name Component path-related functions

          Each Component can keep track of a preferred filesystem path. Typically, users should not tamper with this,
          but Module objects (which derive from Component) that are dynamically loaded will have that path set to the
          path where the Module's shared object file (.so) was found. This allows those modules to access some of their
          local configuration data, which may, for example, be stored in an \c etc/ directory under the module path. */
      //! @{

      //! Assign a filesystem path to this component
      void setPath(std::string const & path);

      //! If given path is relative (not starting with /), prepend the Component path to it
      /*! If path is absolute, no-op. If path is empty, return the Component's path as set with setPath(). */
      std::filesystem::path absolutePath(std::filesystem::path const & path = "");

      //! @}

    protected:
      //! @name Component setup functions overloadable by derived classes

      //! @{

      //! Called before all sub-Components are init()ed
      virtual void preInit() { }

      //! Called after all sub-Components are init()ed
      virtual void postInit() { }

      //! Called before all sub-Components are uninit()ed
      virtual void preUninit() { }

      //! Called after all sub-Components are uninit()ed
      virtual void postUninit() { }

      //! @}

    private:
      template <typename T> friend class ParameterCore;
      friend class Manager; // Allow Manager to access our subs directly (in addComponent, etc)
      friend class Engine; // Allow Engine to add already-created components (the module)
      friend class Module; // Allow Module to access itsParent
      
      mutable boost::shared_mutex itsMtx; // Mutex used to protect our internals other than subcomps and parameters

      // Storage for sub-Components
      std::vector<std::shared_ptr<Component> > itsSubComponents;

      // Mutex to protect our list of subcomponents
      mutable boost::shared_mutex itsSubMtx;

      // Recursively populate a list of parameters, for help message
      void populateHelpMessage(std::string const & cname,
                               std::unordered_map<std::string /* categ+categdesc */,
                               std::unordered_map<std::string /* name+defaultval+validvals+descrip */,
                               std::vector<std::pair<std::string /* component */, std::string /* value */ > > > > &
                               helplist, bool recurse = true) const;

      //! Initialization
      /*! Invoke: runPreInit(), then setInitialized(), finally runPostInit() */
      virtual void init();

      //! Recursively run preInit()
      void runPreInit();

      //! Recursively set initialized flag
      void setInitialized();

      //! Recursively run postInit()
      void runPostInit();

      //! Un-initialize
      /* Invoke: runPreUninit(), then setUninitialized(), finally runPostUninit() */
      virtual void uninit();

      //! Recursively run preUninit()
      void runPreUninit();

      //! Recursively set uninitialized flag
      void setUninitialized();

      //! Recursively run postUninit()
      void runPostUninit();

      // Our meta information. Only access this through meta(), even from within Component!
      mutable boost::shared_mutex itsMetaMtx;
      std::string const itsClassName;
      std::string itsInstanceName;

      // Has this Component been initialized?
      volatile bool itsInitialized;

      // The Component which contains this one as a sub-Component - this may be NULL if the component is not part of any
      // hierarchy, or is the manager.
      Component * itsParent;

      void findParamAndActOnIt(std::string const & descriptor,
                               std::function<void(jevois::ParameterBase * param, std::string const & unrolled)> doit,
                               std::function<bool()> empty) const;
      void findParamAndActOnIt(std::vector<std::string> const & descrip, bool recur, size_t idx,
                               std::string const & unrolled,
                               std::function<void(jevois::ParameterBase *, std::string const &)> doit) const;

      std::string itsPath; // filesystem path assigned to this Component, empty by default

      //! Helper function to compute an automatic instance name, if needed
      /*! Parent modules call this before adding a sub-module, to make sure its name will not clash with existing
          sub-modules. Parameter classname can be empty if instance is not. Throws if a unique name cannot be created
          (e.g., no # was provided and the instance name clashes with other sub-modules). itsSubMtx on the parent must
          be locked by caller to make sure the computed name remains available as we return it */
      std::string computeInstanceName(std::string const & instance, std::string const & classname = "") const;

      //! Actually remove a sub, no type checking here, for internal use only
      /*! Uplck is assumed to already be locked for read, will be upgraded to write lock. Itr is an iterator on
          itsSubComponents pointing at the component to remove. It will get reset(). displayname is just Component,
          SubModule, etc for messages purposes. */
      void doRemoveSubComponent(std::vector<std::shared_ptr<Component> >::iterator & itr,
                                boost::upgrade_lock<boost::shared_mutex> & uplck,
                                std::string const & displayname);

      // Bank of defs for dynamic parameters, someone needs to hold them
      std::map<std::string /* name */, std::shared_ptr<ParameterBase>> itsDynParams;
      mutable std::mutex itsDynParMtx;
  };


  //! Get the current video processing frame number
  /*! The Engine maintains a master frame counter that is incremented on each call to a Module's process(), whether
      or not the call succeeds. The counter is not incremented when a module has not been loaded (e.g., failed to
      load). The counter is reset to zero each time a new module is loaded.

      This is implemented as a free function so that anyone can access it:

      \code
      size_t current_frame_number = jevois::frameNum();
      \endcode

      It is thread-safe. Programmer note: the implementation is in Engine.C since Engine is the one that manages the
      counter. It is declared here so that users don't have to include Engine.H \relates Engine */
  size_t frameNum();

} //jevois

// Include inlined implementation details that are of no interest to the end user
#include <jevois/Component/details/ComponentImpl.H>

