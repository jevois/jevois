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

#include <boost/thread.hpp>

// Get our helpers
#include <jevois/Component/details/ParameterHelpers.H>

namespace jevois
{
  /*! \defgroup parameter Parameter-related classes and functions
      \ingroup component

      The concept of parameter in the JeVois framework embodies wrappers around a single value of any type, with
      associated documentation (description), default values, possible specification of valid values, accessor functions
      to obtain or change the value, and optional callback functions that are triggered when the value is
      changed. Parameters are intended to be used in objects that inherit from Component. The goal of parameters is to
      expose parameters of a given vision algorithm in such a way that any piece of code that is using that algorithm
      will automatically inherit and expose these parameters.

      How to explore this documentation module:

      - Start with a bit of general philosophy about components and parameters: Component
      - Then understand how one may specify valid values for parameters: \ref validvalues
      - Then have a look at how one may define the name, type, description, default value, category, and optionally
        valid values for a parameter: ParameterDef
      - Then you are ready for [Parameter](classjevois_1_1Parameter_3_01Param_00_01Tail_8_8_8_4.html)

      The other classes in this documentation module are mainly for support of the above ones.



      Convenience macro to define a Parameter type
      --------------------------------------------

      JEVOIS_DECLARE_PARAMETER(ParamName, ParamType, ...)
      
      ParamName is the name chosen for the parameter. A new class type will be created with that name, so it must be
      syntactically correct as a class name. ParamType is the type of the parameter value. The remaining arguments are
      passed to the constructor of jevois::ParameterDef<T> with T=ParamType.
  
      Convenience macro to define a Parameter type, with callback
      -----------------------------------------------------------

      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(ParamName, ParamType, ...)
      
      ParamName is the name chosen for the parameter. A new class type will be created with that name, so it must be
      syntactically correct as a class name. ParamType is the type of the parameter value. The remaining arguments are
      passed to the constructor of jevois::ParameterDef<T> with T=ParamType.
      
      In this version with callback, a pure virtual method is included in the new class that is defined, with the
      following signature:
      
      \code
      virtual void onParamChange(ParamName const & param, ParamType const & newval) = 0;
      \endcode
      
      The host class (typically, a Component) that inherits from Parameter<ParamName> must implement an override of this
      function. A compile-time error will be issued if that override has not been implemented. */

  /*! @{ */ // **********************************************************************

  // ######################################################################
  //! ParameterSummary provides a summary about a parameter
  class ParameterSummary
  {
    public:
      //! Descriptor. This is the name of the parameter, qualified by a chain of component names
      std::string descriptor;

      //! Plain name of the parameter
      std::string name;

      //! Description of the parameter
      std::string description;

      //! Parameter value type, as a string
      std::string valuetype;

      //! Default value of the parameter, as a string
      std::string defaultvalue;

      //! Current value of the parameter, as a string
      std::string value;

      //! Description of the parameter's valid values specification, as a string
      std::string validvalues;

      //! Category of the parameter, as a string
      std::string category;

      //! Category description
      std::string categorydescription;

      //! Flag that indicates whether parameter is frozen
      bool frozen;
  };

  // ######################################################################
  //! Base class for Parameter
  /*! This exposes the string interface to the Parameter while derived template classes will expose the
      value-based interface. */
  class ParameterBase
  {
    public:
      //! Constructor
      ParameterBase();

      //! Destructor, will remove the parameter from its owner component
      virtual ~ParameterBase();

      //! Get the parameter name
      virtual std::string const & name() const = 0;

      //! Get the parameter fully-qualified name, aka descriptor, including names of owning Component and all parents
      virtual std::string descriptor() const = 0;

      //! Set the value from a string representation of it
      /*! @throws std::range_error if the given string cannot be converted to a Parameter value, or the value is invalid
          according to our valid values spec or rejected by the Parameter's callback (if any). */
      virtual void strset(std::string const & valstring) = 0;

      //! Get the value as a string
      virtual std::string const strget() const = 0;

      //! Get summary info about this parameter
      virtual ParameterSummary const summary() const = 0;

      //! Freeze this parameter, it becomes read-only and will not show up in the help message
      void freeze();

      //! Unfreeze this parameter, it becomes read-write and will show up in the help message
      void unFreeze();

      //! Reset this parameter to its default value
      virtual void reset() = 0;
      
    protected:
      mutable boost::shared_mutex itsMutex; //!< Mutex to protect the parameter value
      volatile bool itsFrozen; //!< When true, parameter is frozen (read-only, does not show up in help message)
      bool itsVirgin; //!< Param has not yet been explicitly set, need to call the callback (if any) at init time

    private:
      friend class ParameterRegistry; // allow the registry to call our callbacks with defaut val

      // Call our callback with our current value, used at init() time
      /* We cannot call the callback during parameter construction because the host Component object is not fully
         constructed yet (since it derives from its parameters). Thus, for all parameters that have a callback, we will
         call that callback once during init(), unless it is already called during command-line parsing. */
      virtual void callbackInitCall() = 0;
  };
} // namespace jevois
  
// Include ParameterDef now (it needs to know about ParameterBase):
#include <jevois/Component/ParameterDef.H>

namespace jevois
{
  class Component;

  // ######################################################################
  //! A changeable parameter for a Component, core class
  /*! Parameters are used to expose user-configurable settings for a Component.  They can be specified from the command
      line and will be set by the time Component::postInit() is called on the Component which owns the
      Parameter. A Parameter may have a callback function which is invoked each time an attempt is made to change the
      Parameter value. */
  template <typename T>
  class ParameterCore : public ParameterBase
  {
    public:
      //! Constructor
      /*! \param def A pointer to the definition for this parameter (given by a ParameterDef). */
      ParameterCore(ParameterDef<T> const & def);

      //! Destructor
      virtual ~ParameterCore();

      //! Get the parameter name
      virtual std::string const & name() const override;

      //! Get the parameter fully-qualified name, aka descriptor
      virtual std::string descriptor() const override;

      //! Get the value of this Parameter
      T get() const;

      //! Set the value of this Parameter
      /*! Will throw jevois::exception::ParameterException if the new value is not accepted, in which case the old value
          will remain in the Parameter. */
      void set(T const & newVal);

      //! Set the value from a string representation of it
      /*! @throws std::range_error if the given string cannot be converted to a valid Parameter value. */
      virtual void strset(std::string const & valstring) override;

      //! Get the value as a string representation of it
      virtual std::string const strget() const override;

      //! Get summary info about this parameter
      virtual ParameterSummary const summary() const override;

      //! Reset this parameter to its default value
      virtual void reset() override;
      
      //! Change the ParameterDef of this parameter
      /*! Use with caution, only people who know what they are doing should use this function. Its thread safety and
          possible side effects are dubious. */
      void changeParameterDef(ParameterDef<T> const & def);

      //! Set the parameter's callback
      /*! The callback function is called each time one tries to change the value of the parameter. Try to avoid using
          setCallback() so you won't confuse users of your class. In most cases, just use the convenience
          JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK() macro.

          The callback should examine the candidate value newval and (1) if it does not like it, throw and
          std::range_error with a descriptive message of why the value is rejected, (2) otherwise, it is assumed that
          the value is accepted and the callback can then allocate resources or do other work with that value (the
          actual modification of the Parameter object is handled upstream and the callback does not need to worry about
          it: if it returns without throwing, the proposed value will become the new value of the Parameter). The
          Parameter is locked-up for writing as long as the callback is running, to avoid destruction of the parameter
          and/or concurrent parameter value changes by several different threads. Thus, callbacks should try to execute
          quickly, and should not call set(), etc on the parameter as this will always deadlock (get() is allowed if
          your callback needs to know the current value of the parameter). */
      void setCallback(std::function<void(T const &)> cb);
      
    protected:
      //! Get the Component to which this Parameter is attached, or nullptr (individual parameters must override)
      virtual Component const * owner() const = 0;

    private:
      void callbackInitCall() override; // Call our callback with the default value in our def
      
      std::function<void(T const &)> itsCallback;              // optional callback function
      T itsVal;                                                // The actual value of the parameter
      ParameterDef<T> const itsDef;                            // The parameter's definition
  };

  // ######################################################################
  //! A set of Parameters attached to a component
  /*! This variadic template class is just for the convenience of adding several parameters to a Component in one
      statement.

      The way in which we have implemented Parameter in the JeVois framework may seem unorthodox at first, but is the
      best way we have found so far in terms of minimizing burden when writing new components with lots of
      parameters. In our earlier framework, the iLab Neuromorphic Vision Toolkit (iNVT) started in 1995, parameters were
      included in components as member variables. The burden to programmers was so high that often they just did not
      include parameters and hardwired values instead, just to avoid that burden. The burden comes from the
      requirements:

      - we want to be able to support parameters of any type
      - we want each parameter to have a name, description, default value, specification of valid values
      - we want parameters to appear in related groups in the help message
      - we want to support callbacks, i.e., functions that are called when one tries to change the parameter value
      - we typically want the callback to be a member function of the Component that owns a given parameter,
        since changing that parameter value will typically trigger some re-organization in that Component (otherwise
        the callback might not be needed).

      Possible implementation using class data members for parameters (similar to what we used in iNVT), here shown for
      a sample int parameter to specify the size of a queue held in a class MyComp that derives from Component:

      \code
      ParamDef<int> sizeparamdef("size", "Queue size", 5, Range<int>(1, 100), categ);
      class MyComp : public jevois::Component
      {
      public:
      Param<int> sizeparam; // ouch

      void sizeParamCallback(int newval) { myqueue.resize(newval); }

      MyComp(std::string const & instance) :
         jevois::Component(instance),
         sizeparam(sizeparamdef)  // ouch
      {
        sizeparam.setCallback(&MyComp::sizeParamCallback); // ouch
      }
      };
      \endcode

      So we basically end up with 3 names that people have no idea what to do with and will just use confusing names for
      (sizeparamdef, sizeparam, sizeParamCallback), and we have to 1) specify the definition of name, description, etc
      somewhere using some arbitrary name (here sizeparamdef), then add the member variable for the param to the
      component using some other name (here sizeparam), then construct the param which would typically require linking
      it to its definition so we can get the default value and such, and finally hook the callback up (note how MyComp
      is not fully constructed yet when we construct sizeparam hence referencing sizeParamCallback() at that time is
      dubious at best). In reality, things are even worse since typically the paramdef, component class declaration, and
      component implementation, should be in 3 different files.

      The approach we developed for the Neuromorphic Robotics Toolkit (NRT) and refined for JeVois works as follows:

      - each parameter is a unique new class type. We create that type once with one name, and it holds the parameter
        value and the definition data. This is further facilitated by the
        JEVOIS_DECLARE_PARAMETER(ParamName, ParamType, ...) variadic macro.
      
      - for parameters with callbacks, their class type includes a pure virtual onParamChange(param, value) function
        that will need to be implemented by the host component. This is facilitated by the
        JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(ParamName, ParamType, ...) variadic macro. The first argument of
        onParamChange() is the parameter class type, so that a host component with many parameters will have many
        different onParamChange() functions, one per parameter that has a callback.

      - components inherit from their parameters using variadic templates to make inheriting from multiple parameters
        short and easy.

      - each parameter exposes simple functions get(), set(), etc (see ParameterCore and ParameterBase). In a component
        that has many parameters, accessing parameters is achieved by disambiguating on which base class (i.e., which
        parameter) one wants to access the get(), set(), etc function, which is achieved by calling paramx::get() vs
        paramy::get(), etc

      - No need to declare parameter member variables (we inherit from them instead).
      - No need to do anything at construction of the component.
      - No need to manually hook the callback function in the component host class to the parameter.
      - Strong compile-time checking that the programmer did not forget to write the callback function for each
        parameter that was declared as having a callback.
      - Only one name used throughout for that parameter and all its associated machinery (definition, callback).
      - It is easy to write scripts that search the source tree for information about all the parameters of a component,
        since those are always all specified in the Parameter< ... > inheritance statement.

      Remaining caveat: it is often desirable to use short names for parameters, such as "size", "length", "dims", etc
      and those names may clash between several components as the .H files for these components are included when
      building a more complex component that uses those. This is not an issue for Module, which is a terminal entity and
      is typically written as a single .C file with no .H file. For components intended for broad use, we currently
      recommend putting all the parameters in a namespace that is the lowercase version of the component class name.

      Below is the resulting implementation in Manager.H. We start with declaring the parameters, and we inherit from
      them when declaring the Manager class:

      \include snip/manager1.C

      For the parameters that we declared as having a callback, we further include in our definition of the Manager
      class overrides for the pure virtual onParamChange() functions that they added to our manager class. Note the
      signatures of these functions: The first argument is a const reference to the parameter for which this callback
      is, and its main role is to disambiguate between the different onParamChange() functions a component may have. The
      second argument is the proposed new parameter value. The onParamChange() function should examine the candidate new
      value and (1) if it does not like it, throw and std::range_error with a descriptive message of why the value is
      rejected, (2) otherwise, it is assumed that the value is accepted and the callback can then allocate resources or
      do other work with that value (the actual modification of the Parameter object is handled upstream and the
      callback does not need to worry about it: if it returns without throwing, the proposed value will become the new
      value of the Parameter). The Parameter is locked-up for writing as long as the callback is running, to avoid
      destruction of the parameter and/or concurrent parameter value changes by several different threads. Thus,
      callbacks should try to execute quickly, and should not call set(), etc on the parameter as this will always
      deadlock (get() is allowed if your callback needs to know the current value of the parameter).

      \include snip/manager2.C

      There is nothing to do in the constructor, destructor, etc of Manager. The only thing that remains to be done is
      to implement the onParamChange() functions in Manager.C. Note how we use the JEVOIS_UNUSED_PARAM(x) macro to avoid
      compiler warnings about our callbacks not using the "param" function parameter:

      \include snip/manager3.C

      For completeness, if you wonder what JEVOIS_DECLARE_PARAMETER(ParamName, ParamType, ...) and 
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(ParamName, ParamType, ...)  exactly do, here they are in
      ParameterHelpers.H and reproduced here:

      \include snip/parametermacros.C

      Also see Engine.H or the many components in the jevoisbase library. */
  template <class Param, class ... Tail>
  class Parameter<Param, Tail ...> : public Param, public Parameter<Tail ...>
  {
      static_assert(std::is_base_of<jevois::ParameterBase, Param>::value,
                    "jevois::Parameter<...> template arguments must all be parameters "
                    "(derive from jevois::ParameterBase");
  };
  
  /*! @} */ // **********************************************************************

} // namespace jevois

