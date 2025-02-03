// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 20122by Laurent Itti, the University of Southern
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

#pragma once

#include <jevois/Component/Component.H>
#include <boost/python.hpp>

namespace jevois
{
  namespace python
  {
    //! Base helper class to allow creation of Parameter in python \ingroup python
    class PyParHelperBase
    {
      public:
        //! Constructor
        PyParHelperBase(jevois::Component * comp);
        
        //! Remove param from component
        virtual ~PyParHelperBase();
        
        //! Get the value, typed, then wrapped into python object
        virtual boost::python::object get() = 0;
        
        //! Set the value from python object that should contain a value of the correct type
        virtual void set(boost::python::object const & val) = 0;

        //! Set the parameter's callback
        virtual void setCallback(boost::python::object const & cb) = 0;

        //! Set valid values that the parameter may take from a list
        virtual void setValidValues(boost::python::list const & vv) = 0;

        //! Access the associated C++ base Parameter
        virtual std::shared_ptr<jevois::ParameterBase> par() const = 0;

      protected:
        //! Access the associated C++ Component
        jevois::Component * comp() const;

      private:
        jevois::Component * itsComp;
    };
    
    //! Typed class to allow creation of Parameter in Python \ingroup python
    template <typename T>
    class PyParHelper : public PyParHelperBase
    {
      public:
        //! Create and add the parameter to the component
        static std::shared_ptr<jevois::python::PyParHelperBase>
        create(jevois::Component * comp, std::string const & name, std::string const & description,
               boost::python::object const & defaultValue, jevois::ParameterCategory const & category);

        //! Create and add the parameter to the component
        PyParHelper(jevois::Component * comp, std::string const & name, std::string const & description,
                    boost::python::object const & defaultValue, jevois::ParameterCategory const & category);
        
        //! Remove param from component
        virtual ~PyParHelper();
        
        //! Get the value, typed, then wrapped into python object
        boost::python::object get() override;
        
        //! Set the value from python object
        void set(boost::python::object const & val) override;

        //! Set the parameter's callback
        void setCallback(boost::python::object const & cb) override;

        //! Set valid values that the parameter may take from a list
        void setValidValues(boost::python::list const & vv) override;

        //! Access the associated C++ base parameter, used to forward strget(), strset(), freeze(), etc to it
        std::shared_ptr<jevois::ParameterBase> par() const override;

      protected:
        std::shared_ptr<jevois::DynamicParameter<T>> itsParam;
        boost::python::object itsPyCallback;
    };
  } // namespace python

  //! Wrapper for jevois Parameter in python
  /*! This wrapper allows python modules to create JeVois Parameter objects, allowing users to interact with the
      parameters through the console, JeVois-Inventor, or the JeVois-Pro GUI. This class does not actually contain the
      parameter, it is just an interface. Parameters belong to the Component associated with the python code via
      PythonWrapper. \ingroup python */
  class PythonParameter
  {
    public:
      //! Constructor. Adds a dynamic parameter to the Component associated with pyinst
      PythonParameter(boost::python::object & pyinst, std::string const & name, std::string const & typ,
                      std::string const & description, boost::python::object const & defaultValue,
                      jevois::ParameterCategory const & category);

      //! Destructor. Removes the dynamic parameter from the associated Component
      ~PythonParameter();
      
      //! Get the parameter name
      std::string const & name() const;

      //! Get the parameter fully-qualified name, aka descriptor, including names of owning Component and all parents
      std::string descriptor() const;

      //! Get the value of this Parameter
      boost::python::object get() const;

      //! Set the value of this Parameter
      /*! Will throw if the new value is not accepted, in which case the old value will remain in the Parameter. */
      void set(boost::python::object const & newVal);

      //! Get the value as a string
      std::string const strget() const;

      //! Set the value from a string representation of it
      /*! @throws std::range_error if the given string cannot be converted to a Parameter value, or the value is invalid
          according to our valid values spec or rejected by the Parameter's callback (if any). */
      void strset(std::string const & valstring);

      //! Freeze/unfreeze this parameter, it becomes read-only and will not show up in the help message
      void freeze(bool doit);

      //! Returns whether parameter is frozen
      bool frozen() const;

      //! Hide or un-hide a parameter; hidden params will not show up in GUI or help message, but still work normally
      void hide(bool doit);
      
      //! Returns whether parameter is hidden
      bool hidden() const;

      //! Reset this parameter to its default value
      void reset();

      //! Set the parameter's callback
      /*! The callback function is called each time one tries to change the value of the parameter. It will also be
          called one first time, with the current parameter value, as soon as the callback is set.

          The callback should examine the candidate value newval and (1) if it does not like it, throw and with a
          descriptive message of why the value is rejected, (2) otherwise, it is assumed that the value is accepted and
          the callback can then allocate resources or do other work with that value (the actual modification of the
          Parameter object is handled upstream and the callback does not need to worry about it: if it returns without
          throwing, the proposed value will become the new value of the Parameter). The Parameter is locked-up for
          writing as long as the callback is running, to avoid destruction of the parameter and/or concurrent parameter
          value changes by several different threads. Thus, callbacks should try to execute quickly, and should not call
          set(), etc on the parameter as this will always deadlock (get() is allowed if your callback needs to know the
          current value of the parameter). */
      void setCallback(boost::python::object const & cb);

      //! Set valid values that the parameter may take from a list
      void setValidValues(boost::python::list const & vv);
      
    private:
      std::shared_ptr<jevois::python::PyParHelperBase> itsPyPar;
  };
  

  
} // namespace jevois

// Include implementation details
#include <jevois/Core/details/PythonParameterImpl.H>
