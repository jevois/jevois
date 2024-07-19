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

#pragma once

#include <boost/python.hpp>
#include <opencv2/core/core.hpp>
#include <array>

#ifdef JEVOIS_PRO
#include <imgui.h>
#endif

namespace jevois
{
  class Engine;

  //! Python-related helpers and functions
  namespace python
  {
   /*! \defgroup pysupport Python support functions

       Helpers to facilitate data transfer to/from Python
        
       \ingroup python */
    
    /*! @{ */ // **********************************************************************
    
    //! Initialize Python, numpy, and allow python modules to send serial outputs through the JeVois Engine
    /*! This command is not for common use, only Engine should use it. */
    void setEngine(jevois::Engine * e);

    //! Get the Engine
    jevois::Engine * engine();

    //! Check whether a boost::python::object has an attribute
    bool hasattr(boost::python::object & o, char const * name);

    //! Helper to convert std::vector<T> to python list
    template <class T>
    boost::python::list pyVecToList(std::vector<T> const & v);
    
    //! Helper to convert python list (or any iterable) to std::vector<T>
    template <class T>
    std::vector<T> pyListToVec(boost::python::object const & lst);

    //! Helper to extract a tuple with elements of uniform type T into an std::array
    template <typename T, size_t N>
    std::array<T, N> tupleToArray(boost::python::object const & o);
    
    //! Helper to extract a tuple with elements of uniform type T into an std::array
    /*! minN is the smallest number of tuple elements required, allowing for partial tuples. If minN<N, we assume that
        the array has been initialized with default values, and we allow receiving a tuple that is smaller than N, in
        which case we only assign the first elements of the array. Useful to exract things like cv::Scalar which can
        contain 1 to 4 elements. */
    template <typename T, size_t N>
    void tupleToArray(boost::python::object const & o, std::array<T, N> & arr, size_t minN = N);

    //! Generic value extraction, pass-through to boost::python::extract() for types that python knows how to extract
    template <typename T>
    T pyextract(boost::python::object const & o);

    //! Specialization for cv::Scalar_<float>, extract from tuple of 1..4 float values
    template <>
    cv::Scalar_<float> pyextract<cv::Scalar_<float>>(boost::python::object const & o);
  
    //! Specialization for cv::Scalar_<int>, extract from tuple of 1..4 int values
    template <>
    cv::Scalar_<int> pyextract<cv::Scalar_<int>>(boost::python::object const & o);
  
    //! Specialization for cv::Point_<float>, extract from tuple of 2 float values
    template <>
    cv::Point_<float> pyextract<cv::Point_<float>>(boost::python::object const & o);
  
    //! Specialization for cv::Point_<int>, extract from tuple of 2 int values
    template <>
    cv::Point_<int> pyextract<cv::Point_<int>>(boost::python::object const & o);
  
    //! Specialization for cv::Size_<float>, extract from tuple of 2 float values
    template <>
    cv::Size_<float> pyextract<cv::Size_<float>>(boost::python::object const & o);
  
    //! Specialization for cv::Size_<int>, extract from tuple of 2 int values
    template <>
    cv::Size_<int> pyextract<cv::Size_<int>>(boost::python::object const & o);

#ifdef JEVOIS_PRO  
    //! Specialization for ImColor, extract from tuple of 3..4 int values (if 3 then, alpha=255)
    template <>
    ImColor pyextract<ImColor>(boost::python::object const & o);
#endif
    
    //! Convert value to python, pass-through to python::object(val)
    template <typename T>
    boost::python::object topyobj(T const & val);

    //! Specialization for cv::Scalar<float>, returns a tuple with 4 float elements
    template <>
    boost::python::object topyobj(cv::Scalar_<float> const & val);

    //! Specialization for cv::Scalar<int>, returns a tuple with 4 int elements
    template <>
    boost::python::object topyobj(cv::Scalar_<int> const & val);
    
    //! Specialization for cv::Point<float>, returns a tuple with 2 float elements
    template <>
    boost::python::object topyobj(cv::Point_<float> const & val);

    //! Specialization for cv::Point<int>, returns a tuple with 2 int elements
    template <>
    boost::python::object topyobj(cv::Point_<int> const & val);
    
    //! Specialization for cv::Size<float>, returns a tuple with 2 float elements
    template <>
    boost::python::object topyobj(cv::Size_<float> const & val);

    //! Specialization for cv::Size<int>, returns a tuple with 2 int elements
    template <>
    boost::python::object topyobj(cv::Size_<int> const & val);
    
#ifdef JEVOIS_PRO  
    //! Specialization for ImColor, returns a tuple with 4 int elements
    template <>
    boost::python::object topyobj(ImColor const & val);
#endif
    
    /*! @} */ // **********************************************************************
  } // namespace python
} // namespace jevois

// Include implementation details
#include <jevois/Core/details/PythonSupportImpl.H>
