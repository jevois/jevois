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

#include <jevois/Debug/PythonException.H>
#include <Python.h>
#include <frameobject.h>
#include <sstream>
#include <vector>

#include <boost/python/object.hpp>
#include <boost/python/str.hpp>
#include <boost/python/extract.hpp>

// This code inspired from:
// https://github.com/abingham/boost_python_exception/tree/master/src/boost_python_exception/auto_translation


namespace
{
  std::string python_string_as_std_string(PyObject* obj)
  {
#if PY_MAJOR_VERSION == 2
    return PyString_AsString(obj);
#else
    return PyUnicode_AsUTF8(obj);
#endif
  }
  
  long python_integral_as_long(PyObject* obj)
  {
#if PY_MAJOR_VERSION == 2
    return PyInt_AsLong(obj);
#else
    return PyLong_AsLong(obj);
#endif
  }

  void clear_exception()
  {
    PyErr_Clear();
  }
  
  boost::python::object ptr_to_obj(PyObject * ptr)
  {
    if (ptr) return boost::python::object(boost::python::handle<>(boost::python::borrowed(ptr)));
    else return boost::python::object();
  }

  struct traceback_step
  {
      int line_number;
      std::string file_name;
      std::string source;
  };

  std::ostream & operator<<(std::ostream& stream, traceback_step const & info)
  {
    stream << "File \"" << info.file_name << "\", line " << info.line_number << ", in " << info.source;
    return stream;
  }
  
  typedef std::vector<traceback_step> traceback;

  std::ostream & operator<<(std::ostream & stream, traceback const & tb)
  {
    for (traceback_step const & step : tb) stream << step << "\n";
    return stream;
  }

  void add_traceback_step(std::vector<traceback_step> & output, PyTracebackObject const * pytb)
  {
    traceback_step entry = {
      pytb->tb_lineno,
      python_string_as_std_string(pytb->tb_frame->f_code->co_filename),
      python_string_as_std_string(pytb->tb_frame->f_code->co_name)
    };
    output.push_back(entry);
  }


  traceback extract_traceback(boost::python::object py_traceback)
  {
    if (py_traceback.is_none()) return traceback();

    PyTracebackObject const * tb = reinterpret_cast<PyTracebackObject *>(py_traceback.ptr());
    std::vector<traceback_step> result;
    for (; tb != 0; tb = tb->tb_next) add_traceback_step(result, tb);
    return result;
  }
  
  std::string extract_exception_type(boost::python::object type)
  {
    if (PyExceptionClass_Check(type.ptr())) {
      return PyExceptionClass_Name(type.ptr());
    } else {
      throw std::logic_error("Given type is not a standard python exception class");
    }
  }
  
  std::string extract_message(boost::python::object value)
  {
    return boost::python::extract<std::string>(boost::python::str(value));
  }

  std::string generate_message(std::string const & type, std::string const & message, traceback const & traceback)
  {
    std::ostringstream result;
    if (traceback.empty() == false) result << "Python traceback (most recent calls last):\n" << traceback;
    result << type;
    if (message.empty() == false) result << ": " << message;
    return result.str();
  }
  
  std::string make_syntax_error_message(boost::python::object error)
  {
    std::string const module_name = boost::python::extract<std::string>(error.attr("filename"));
    std::string const code = boost::python::extract<std::string>(error.attr("text"));
    
    // for some reason, extract<long> does not work while a python error is set, so do it with CPython
    long const line_number = python_integral_as_long(boost::python::object(error.attr("lineno")).ptr());
    long const pos_in_line = python_integral_as_long(boost::python::object(error.attr("offset")).ptr());
    
    std::ostringstream message;
    message << "In module \"" << module_name << "\", line " << line_number << ", position " << pos_in_line << ":\n";
    message << "Offending code: " << code;
    message << "                " << std::string(pos_in_line-1, ' ') << "^";

    return message.str();
  }
} // anonymous namespace

std::string jevois::getPythonExceptionString(boost::python::error_already_set &)
{
  // Get some info from the python exception:
  PyObject *t, *v, *tb;

  try
  {
    PyErr_Fetch(&t, &v, &tb);
    PyErr_NormalizeException(&t, &v, &tb);
  }
  catch (...) { return "Internal error trying to fetch exception data from Python"; }
  
  try
  {
    boost::python::object objtype = ::ptr_to_obj(t);
    boost::python::object objvalue = ::ptr_to_obj(v);
    boost::python::object objtraceback = ::ptr_to_obj(tb);
  
    std::string const type = extract_exception_type(objtype);
    std::string const message = (type == "SyntaxError") ?
      make_syntax_error_message(objvalue) : extract_message(objvalue);
    traceback const traceback = extract_traceback(objtraceback);

    PyErr_Restore(t, v, tb);
    clear_exception();

    return generate_message(type, message, traceback);
  }
  catch (...)
  { PyErr_Restore(t, v, tb); clear_exception(); return "Internal error trying to fetch exception data from Python"; }
}
