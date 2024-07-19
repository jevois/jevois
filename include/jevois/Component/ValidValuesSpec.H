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

#include <vector>
#include <string>
#include <jevois/Types/Range.H>
#include <jevois/Types/StepRange.H>
#include <boost/regex.hpp>

namespace jevois
{
  /*! \defgroup validvalues Specification of sets of valid values, e.g., that some Parameter may take

      This is used by ParameterDef to specify valid values that a Parameter may take as unrestricted, from a given list,
      in a range, matching a regex, etc

      \ingroup parameter */

  /*! @{ */ // **********************************************************************

  //! Base class for specifying a set of valid values for a type
  /*! Note when defining new derived valid values specs: make sure you define operator<<() and also that the copy
      constructor works as intended (which means that you may have to define it explicitly if your data members are
      complicated). */
  template <typename T>
  class ValidValuesSpecBase
  {
    public:
      //! Construct, for the base class this is a no-op
      /*! Note that we make the constructor explicit otherwise we get false hits against strings and what not in our
          operator<< defined on various ValidValueSpec derivatives. */
      explicit ValidValuesSpecBase();

      //! Destructor
      virtual ~ValidValuesSpecBase();

      //! Check whether a proposed value is valid, return true if it is
      virtual bool checkValueValidity(T const & val) const = 0;

      //! Convert the specification of valid values to a readable string
      /*! Caution, a GUI may wish to parse this string so keep the format tight. Typically,
          type:[valuesdescription], e.g., see derived classes for None:[], List:[A|B|C], etc. */
      virtual std::string const str() const = 0;
  };

  // ######################################################################
  //! Open/None valid values spec, anything that T can take is valid
  template <typename T>
  class ValidValuesSpecNone : public ValidValuesSpecBase<T>
  {
    public:
      //! Construct with no specification, any value that T can take goes
      explicit ValidValuesSpecNone();

      //! Destructor
      virtual ~ValidValuesSpecNone();

      //! Check whether a proposed value is valid, here always returns true
      virtual bool checkValueValidity(T const & val) const;

      //! Convert to a readable string
      /*! Returns None:[] */
      virtual std::string const str() const;
  };

  // ######################################################################
  //! Finite list valid values spec, everything listed at construction is valid, anything else is not
  template <typename T>
  class ValidValuesSpecList : public ValidValuesSpecBase<T>
  {
    public:
      //! No default constructor, always need to provide a list
      ValidValuesSpecList() = delete;

      //! Construct from a given list of valid values in a vector
      explicit ValidValuesSpecList(std::vector<T> const & valid_list);

      //! Destructor
      virtual ~ValidValuesSpecList();

      //! Check whether a proposed value is valid, returns true iff value is in our list
      virtual bool checkValueValidity(T const & val) const;

      //! Convert to a readable string
      /*! Returns List:[A|B|C] where A, B, C are replaced by the actual elements. */
      virtual std::string const str() const;

    protected:
      std::vector<T> const itsValidList; //!< Our list of valid values
  };

  // ######################################################################
  //! Range-based valid values spec, bounds are included
  template <typename T>
  class ValidValuesSpecRange : public ValidValuesSpecBase<T>
  {
    public:
      //! No default constructor, always need to provide a range
      ValidValuesSpecRange() = delete;

      //! Construct from a Range of valid values (convention: bounds are inclusive)
      explicit ValidValuesSpecRange(Range<T> const & valid_range);

      //! Destructor
      virtual ~ValidValuesSpecRange();

      //! Check whether a proposed value is valid, returns true iff value is in our range (bounds included)
      virtual bool checkValueValidity(T const & val) const;

      //! Convert to a readable string: Range:[MIN-MAX] where MIN and MAX are replaced by the actual range bounds.
      virtual std::string const str() const;

    protected:
      Range<T> const itsValidRange; //!< Our range of valid values
  };

  // ######################################################################
  //! StepRange-based valid values spec, bounds are included
  template <typename T>
  class ValidValuesSpecStepRange : public ValidValuesSpecBase<T>
  {
    public:
      //! No default constructor, always need to provide a range
      ValidValuesSpecStepRange() = delete;

      //! Construct from a StepRange of valid values (convention: bounds are inclusive)
      explicit ValidValuesSpecStepRange(StepRange<T> const & valid_range);

      //! Destructor
      virtual ~ValidValuesSpecStepRange();

      //! Check whether a proposed value is valid, returns true iff value is in our range (bounds included)
      virtual bool checkValueValidity(T const & val) const;

      //! Convert to a readable string: StepRange:[MIN-STEP-MAX] where MIN, STEP and MAX are replaced by actual values.
      virtual std::string const str() const;

    protected:
      StepRange<T> const itsValidStepRange; //!< Our step-range of valid values
  };
  
  // ######################################################################
  //! Regex-based valid values spec, everything that is a match to the regex is considered valid
  /*! Uses boost::regex internally (because std::regex does not alow one to get the original string specification back
      from the regex, but we need that to display help messages). This allows for highly flexible valid values
      definitions. For example, say you want an int parameter to be in range [0..59] but it could also have value 72,
      your regex would be:

      \verbatim
      ^(([0-5]?[0-9])|72)$
      \endverbatim

      You can find on the web regex examples to match things like a valid filename, a valid URL, a valid credit card
      number, etc. Just make sure your regex is in the syntax expected by boost::regex since several syntaxes are
      floating around for regular expressions. */
  template <typename T>
  class ValidValuesSpecRegex : public ValidValuesSpecBase<T>
  {
    public:
      //! No default constructor, always need to provide a regex
      ValidValuesSpecRegex() = delete;

      //! Construct from a given regex that specifies valid values
      explicit ValidValuesSpecRegex(boost::regex const & valid_regex);

      //! Destructor
      virtual ~ValidValuesSpecRegex();

      //! Check whether a proposed value is valid, returns true iff value is a match against our regex
      virtual bool checkValueValidity(T const & val) const;

      //! Convert to a readable string
      /*! Returns Regex:[expression] where expression is replaced by the actual regex. */
      virtual std::string const str() const;

    protected:
      boost::regex const itsValidRegex; //!< The regex that defines our valid values
  };

  /*! @} */ // **********************************************************************

} // namespace jevois

// Include inlined implementation details that are of no interest to the end user
#include <jevois/Component/details/ValidValuesSpecImpl.H>


