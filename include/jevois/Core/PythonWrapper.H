// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2022 by Laurent Itti, the University of Southern
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

#include <boost/python.hpp>
#include <mutex>

namespace jevois
{
  //! Helper class to run python code from C++
  /*! This class loads python code and makes it accessible through pyinst(). It is designed to be composed with other
      classes that will actually execute some python code via pyinst().

      The main convention here is that the python code will define a class with the exact same name as the python
      filename (without trailing .py extension). This is the class that will then become accessible via pyinst().

      Note that constructors and pythonload() will not throw, but pyinst() will if an error occurred at construction.

      Often this class is inherited from by a class that also inherits from Component, to create a Component derivative
      that can run python code. In such case, PythonWrapper will register that sibling association with Engine when the
      python code is loaded. It is critical that you inherit from Component (or a derivative) before you inherit from
      PythonWrapper, so that by the time the PythonWrapper destructor is called the Component base should not yet have
      been destroyed, and it will be unregistered from Engine.

      \ingroup python */
  class PythonWrapper
  {
    public:
      //! Default constructor. Will need to call pythonload() later
      PythonWrapper();

      //! Destructor
      virtual ~PythonWrapper();
      
      //! Construct from path
      PythonWrapper(std::string const & path);

      //! Init from path if default constructor was used
      void pythonload(std::string const & path);

      //! Get the python class pyinst, or throw if construction error occurred (e.g., file not found)
      boost::python::object & pyinst();

      //! Get the main module
      boost::python::object & mainModule();

      //! Get the main namespace
      boost::python::object & mainNamespace();

      //! Get the construction error if any, or empty string
      std::string const & constructionError() const;

    private:
      boost::python::object itsMainModule, itsMainNamespace, itsInstance;
      std::string itsConstructionError;
      mutable std::mutex itsMtx; // make sure we don't get destroyed while loading python code
  };

} // namespace jevois
