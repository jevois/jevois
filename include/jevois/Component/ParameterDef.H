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

#include <jevois/Component/ValidValuesSpec.H>
#include <memory>

namespace jevois
{
  template <class T> class ParameterCore;

  // ######################################################################
  //! A category to which multiple ParameterDef definitions can belong
  /*! All the ParameterDef classes which belong to the same category will show up next to each other when the user
      requests the help message.  \ingroup parameter */
  struct ParameterCategory
  {
      //! Constructor
      ParameterCategory(std::string const & name_ = "General Options", std::string const & description_ = "");

      std::string name;         //!< The name of the category
      std::string description;  //!< An optional short description of the category
  };

  // ######################################################################
  //! Base class for a Parameter definition
  /*! This exposes the string-based interface to the definition while derived class templates will expose the
      value-based interface. Users should never directly create a ParameterDefBase, but should instead create a properly
      templated ParameterDef<T>, as ParameterDef<T> derives from ParameterBase. \ingroup parameter */
  class ParameterDefBase
  {
    public:
      //! Construct a ParameterDefBase
      ParameterDefBase(std::string const & name, std::string const & description,
                       ParameterCategory const & category = ParameterCategory());

      //! Destructor
      virtual ~ParameterDefBase();

      //! The parameter name (without the leading "--")
      std::string const & name() const;

      //! Description of what the parameter does
      std::string const & description() const;

      //! The Parameter category
      ParameterCategory const & category() const;

      //! Default value in string representation
      virtual std::string const defaultValueString() const = 0;

      //! Valid values in string representation
      virtual std::string const validValuesString() const = 0;

    protected:
      std::string itsName; //!< Our name
      std::string itsDescription; //!< Our description string
      ParameterCategory itsCategory; //!< Our category
  };

  // ######################################################################
  //! A Parameter Definition
  /*! ParameterDef is used to specify the name, description, default value, category, and optionally valid values for a
      Parameter. The value type of the parameter is specified by template parameter T.

      See \ref validvalues for definitions of specifications for valid values, allowing any value, values from a list,
      values in a range, values that match a regex, etc.

      \ingroup parameter */
  template <class T>
  class ParameterDef : public ParameterDefBase
  {
    public:
      //! Construct a ParameterDef with no given valid values, valid values are whatever T can take
      ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                   ParameterCategory const & category);

      //! Create a Parameter definition with specified valid values from a ValidValueSpec
      template <template <typename> class ValidValuesSpec>
      ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                   ValidValuesSpec<T> const & validValuesSpec, ParameterCategory const & category);

      //! Shorthand to create a Parameter definition with specified valid values from a list
      /*! If your Parameter can only be set to a finite list of values, you can use this convenience constructor
          to specify them.

          For example, if you want to create a Parameter for some kernel convolution for which you only have 
          kernels of sizes 3, 5, 7 and 9 you could create a ParameterDef as follows:
          \code
          ParameterDef<int> KernelSizeDef("kernel_size", "The size of the convolution kernel", 3, { 3, 5, 7, 9} );
          \endcode 

          The list of valid values will be shown for this parameter in the help message. */
      ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                   std::vector<T> const & validvalues, ParameterCategory const & category);

      //! Shorthand to create a Parameter definition with specified valid values from a range (bounds inclusive)
      ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                   jevois::Range<T> const & validrange, ParameterCategory const & category);

      //! Shorthand to create a Parameter definition with specified valid values from a step range (bounds inclusive)
      ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                   jevois::StepRange<T> const & validrange, ParameterCategory const & category);

      //! Shorthand to create a Parameter definition with specified valid values in a regex
      /*! This allows for highly flexible valid values definitions. For example, say you want an int parameter to be in
          range [0..59] but it could also have value 72, your regex would be:

          \verbatim
          ^(([0-5]?[0-9])|72)$
          \endverbatim */
      ParameterDef(std::string const & name, std::string const & description, T const & defaultValue,
                   boost::regex const & validregex, ParameterCategory const & category);

      //! Get the default value that was specified for this Parameter definition
      T const defaultValue() const;

      //! Default value in string representation
      virtual std::string const defaultValueString() const;

      //! Valid values in string representation
      virtual std::string const validValuesString() const;

    private:
      friend class ParameterCore<T>;
      T itsDefaultValue;
      std::shared_ptr<ValidValuesSpecBase<T> > itsValidValuesSpec;
      void checkDefaultValue();
  };

} // namespace jevois

// Include inlined implementation details that are of no interest to the end user
#include <jevois/Component/details/ParameterDefImpl.H>


