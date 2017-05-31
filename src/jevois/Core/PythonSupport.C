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

#include <jevois/Core/PythonSupport.H>
#include <jevois/Image/RawImage.H>
#include <jevois/Core/PythonModule.H>

#include <jevois/Util/Utils.H>

BOOST_PYTHON_MODULE(libjevois)
{
    boost::python::def("fccstr", jevois::fccstr);

    boost::python::class_<jevois::RawImage>("RawImage") // default constructor is included
      .def("invalidate", &jevois::RawImage::invalidate)
      .def("valid", &jevois::RawImage::valid)
      .def("require", &jevois::RawImage::require)
      .def("bytesperpix", &jevois::RawImage::bytesperpix)
      .def("bytesize", &jevois::RawImage::bytesize)
      .def("coordsOk", &jevois::RawImage::coordsOk)
      //.def("pixels", &jevois::RawImage::pixelsw<unsigned char>,
      //     boost::python::return_value_policy<boost::python::reference_existing_object>())
      ;
    
    boost::python::class_<jevois::InputFramePython>("InputFrame")
      .def("get", &jevois::InputFramePython::get,
           boost::python::return_value_policy<boost::python::reference_existing_object>())
      .def("done", &jevois::InputFramePython::done)
      ;
    
    boost::python::class_<jevois::OutputFramePython>("OutputFrame")
      .def("get", &jevois::OutputFramePython::get,
           boost::python::return_value_policy<boost::python::reference_existing_object>())
      .def("send", &jevois::OutputFramePython::send)
      ;
    
  
}
