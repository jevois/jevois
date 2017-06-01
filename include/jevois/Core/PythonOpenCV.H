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

// This file is derived from https://github.com/Algomorph/pyboostcvconverter

/*
 * CVBoostConverter.hpp
 *
 *  Created on: Mar 20, 2014
 *      Author: Gregory Kramida
 *   Copyright: (c) 2014 Gregory Kramida
 *     License: MIT

 The MIT License (MIT)

 Copyright (c) 2014 Gregory Kramida
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#pragma once

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <numpy/ndarrayobject.h>
#include <opencv2/core/core.hpp>
#include <boost/python.hpp>
#include <cstdio>

namespace pbcvt
{
  using namespace cv;

  extern PyObject * opencv_error;


  //===================    MACROS    =================================================================
#define ERRWRAP2(expr)                          \
  try                                           \
  {                                             \
    PyAllowThreads allowThreads;                \
    expr;                                       \
  }                                             \
  catch (const cv::Exception &e)                \
  {                                             \
    PyErr_SetString(opencv_error, e.what());    \
    return 0;                                   \
  }
  
  //===================   THREADING     ==============================================================
  class PyAllowThreads;
  class PyEnsureGIL;
  
  static size_t REFCOUNT_OFFSET = (size_t)&(((PyObject*)0)->ob_refcnt) +
    (0x12345678 != *(const size_t*)"\x78\x56\x34\x12\0\0\0\0\0")*sizeof(int);
  
  static inline PyObject* pyObjectFromRefcount(const int* refcount)
  {
    return (PyObject*)((size_t)refcount - REFCOUNT_OFFSET);
  }

  static inline int* refcountFromPyObject(const PyObject* obj)
  {
    return (int*)((size_t)obj + REFCOUNT_OFFSET);
  }
  
  //===================   NUMPY ALLOCATOR FOR OPENCV     =============================================
  class NumpyAllocator;
  
  //===================   STANDALONE CONVERTER FUNCTIONS     =========================================
  PyObject* fromMatToNDArray(const Mat& m);
  Mat fromNDArrayToMat(PyObject* o);
  
  //===================   BOOST CONVERTERS     =======================================================
  struct matToNDArrayBoostConverter
  {
      static PyObject* convert(Mat const& m);
  };

  //! Converter from Python numpy NDarray to cv::Mat
  struct matFromNDArrayBoostConverter
  {
      matFromNDArrayBoostConverter();
      
      /// @brief Check if PyObject is an array and can be converted to OpenCV matrix.
      static void* convertible(PyObject* object);
      
      /// @brief Construct a Mat from an NDArray object.
      static void construct(PyObject* object,
                            boost::python::converter::rvalue_from_python_stage1_data* data);
  };
} // end namespace pbcvt
