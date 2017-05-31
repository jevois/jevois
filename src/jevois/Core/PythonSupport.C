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
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/PythonModule.H>
#include <jevois/Core/UserInterface.H>
#include <jevois/Core/StdioInterface.H>
#include <jevois/Core/Serial.H>

#include <jevois/Util/Utils.H>

// ####################################################################################################
//! Convenience macro to define a Python binding for a free function in the jevois namespace
#define JEVOIS_PYTHON_FUNC(funcname)                \
  boost::python::def(#funcname, jevois::funcname)

//! Convenience macro to define a Python binding for a free function in the jevois::rawimage namespace
#define JEVOIS_PYTHON_RAWIMAGE_FUNC(funcname)                   \
  boost::python::def(#funcname, jevois::rawimage::funcname)

//! Convenience macro to define a python enum value
#define JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(val) value(#val, jevois::rawimage::val)

// ####################################################################################################
// Thin wrappers to handle default arguments or overloads in free functions

namespace
{
  void drawRect1(jevois::RawImage & img, int x, int y, unsigned int w,
                 unsigned int h, unsigned int thick, unsigned int col)
  { jevois::rawimage::drawRect(img, x, y, w, h, thick, col); }
  
  void drawRect2(jevois::RawImage & img, int x, int y, unsigned int w, unsigned int h, unsigned int col)
  { jevois::rawimage::drawRect(img, x, y, w, h, col); }
  
  void writeText1(jevois::RawImage & img, std::string const & txt, int x, int y,
                  unsigned int col, jevois::rawimage::Font font)
  { jevois::rawimage::writeText(img, txt, x, y, col, font); }

} // anonymous namespace

// ####################################################################################################
BOOST_PYTHON_MODULE(libjevois)
{
  // #################### Utils.H
  JEVOIS_PYTHON_FUNC(fccstr);
  
  // #################### RawImage.H
  boost::python::class_<jevois::RawImage>("RawImage") // default constructor is included
    .def("invalidate", &jevois::RawImage::invalidate)
    .def("valid", &jevois::RawImage::valid)
    .def("require", &jevois::RawImage::require)
    .def_readwrite("width", &jevois::RawImage::width)
    .def_readwrite("height", &jevois::RawImage::height)
    .def_readwrite("fmt", &jevois::RawImage::fmt)
    .def("bytesperpix", &jevois::RawImage::bytesperpix)
    .def("bytesize", &jevois::RawImage::bytesize)
    .def("coordsOk", &jevois::RawImage::coordsOk)
    //.def("pixels", &jevois::RawImage::pixelsw<unsigned char>,
    //     boost::python::return_value_policy<boost::python::reference_existing_object>())
    ;

  // #################### PythonModule.H
  boost::python::class_<jevois::InputFramePython>("InputFrame")
    .def("get", &jevois::InputFramePython::get1,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("get", &jevois::InputFramePython::get,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("done", &jevois::InputFramePython::done)
    ;
  
  boost::python::class_<jevois::OutputFramePython>("OutputFrame")
    .def("get", &jevois::OutputFramePython::get,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("send", &jevois::OutputFramePython::send)
    ;

  // #################### RawImageOps.H
  JEVOIS_PYTHON_RAWIMAGE_FUNC(cvImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertToCvGray);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertToCvBGR);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertToCvRGB);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertToCvRGBA);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(byteSwap);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(paste);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(pasteGreyToYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(roipaste);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(drawDisk);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(drawCircle);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(drawLine);
  boost::python::def("drawRect", drawRect1);
  boost::python::def("drawRect", drawRect2);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(drawFilledRect);

  boost::python::enum_<jevois::rawimage::Font>("Font")
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font5x7)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font6x10)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font7x13)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font8x13bold)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font9x15bold)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font10x20)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font11x22)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font12x22)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font14x26)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font15x28)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font16x29)
    .JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(Font20x38);
  boost::python::def("writeText", writeText1);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvBGRtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvRGBAtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(unpackCvRGBAtoGrayRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(hFlipYUYV);
}
