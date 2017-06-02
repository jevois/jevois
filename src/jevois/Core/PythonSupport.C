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

#include <boost/python.hpp>

#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/Engine.H>
#include <jevois/Image/RawImage.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/PythonModule.H>
#include <jevois/Core/UserInterface.H>
#include <jevois/Core/StdioInterface.H>
#include <jevois/Core/Serial.H>
#include <jevois/Util/Utils.H>
#include <jevois/Util/Coordinates.H>
#include <jevois/Debug/Log.H>
#include <jevois/Debug/Timer.H>
#include <jevois/Debug/Profiler.H>
#include <jevois/Debug/SysInfo.H>

#define PY_ARRAY_UNIQUE_SYMBOL pbcvt_ARRAY_API
#include <jevois/Core/PythonOpenCV.H>

// ####################################################################################################
//! Convenience macro to define a Python binding for a free function in the jevois namespace
#define JEVOIS_PYTHON_FUNC(funcname)                \
  boost::python::def(#funcname, jevois::funcname)

//! Convenience macro to define a Python binding for a free function in the jevois::rawimage namespace
#define JEVOIS_PYTHON_RAWIMAGE_FUNC(funcname)                   \
  boost::python::def(#funcname, jevois::rawimage::funcname)

//! Convenience macro to define a python enum value where the value is in jevois::rawimage
#define JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(val) value(#val, jevois::rawimage::val)

//! Convenience macro to define a python enum value where the value is in jevois::python
#define JEVOIS_PYTHON_ENUM_VAL(val) value(#val, jevois::python::val)

//! Convenience macro to define a constant that exists in the global C++ namespace
#define JEVOIS_PYTHON_CONSTANT(cst) boost::python::scope().attr(#cst) = cst;


// ####################################################################################################
// Helper to provide jevois.sendSerial() function that emulates a C++ module's sendSerial()
namespace jevois
{
  namespace python
  {
    Engine * engineForPythonModule = nullptr;
  }
}
namespace
{
  void * init_numpy()
  {
    // Initialize Python:
    Py_Initialize();

    // Initialize numpy array. Use the signal handler hack from
    // https://stackoverflow.com/questions/28750774/
    //         python-import-array-makes-it-impossible-to-kill-embedded-python-with-ctrl-c

    //PyOS_sighandler_t sighandler = PyOS_getsig(SIGINT);
    import_array();
    //PyOS_setsig(SIGINT,sighandler);
    return NUMPY_IMPORT_ARRAY_RETVAL;
  }
}

void jevois::pythonModuleSetEngine(jevois::Engine * e)
{
  jevois::python::engineForPythonModule = e;
  init_numpy();
}

// ####################################################################################################
// Thin wrappers to handle default arguments or overloads in free functions

namespace
{
  void pythonSendSerial(std::string const & str)
  {
    if (jevois::python::engineForPythonModule == nullptr) LFATAL("internal error");
    jevois::python::engineForPythonModule->sendSerial(str);
  }

  void pythonLDEBUG(std::string const & JEVOIS_UNUSED_PARAM(str)) { LDEBUG(str); }
  void pythonLINFO(std::string const & str) { LINFO(str); }
  void pythonLERROR(std::string const & str) { LERROR(str); }
  void pythonLFATAL(std::string const & JEVOIS_UNUSED_PARAM(str)) { LFATAL(str); }

} // anonymous namespace

namespace jevois
{
  namespace python
  {
    // Aux enum for YUYV colors - keep this in sync with RawImage.H:
    enum YUYV { Black = 0x8000, DarkGrey = 0x8050, MedGrey = 0x8080, LightGrey = 0x80a0, White = 0x80ff,
                DarkGreen = 0x0000, MedGreen = 0x0040, LightGreen = 0x00ff, DarkTeal = 0x7070, MedTeal = 0x7090,
                LightTeal = 0x70b0, DarkPurple = 0xa030, MedPurple = 0xa050, LightPurple = 0xa080,
                DarkPink = 0xff00, MedPink = 0xff80, LightPink = 0xffff };
  }
}

// ####################################################################################################
BOOST_PYTHON_MODULE(libjevois)
{
  // #################### Initialize converters for cv::Mat support:
  boost::python::to_python_converter<cv::Mat, pbcvt::matToNDArrayBoostConverter>();
  pbcvt::matFromNDArrayBoostConverter();
  
  // #################### module sendSerial() emulation:
  boost::python::def("sendSerial", pythonSendSerial);

  // #################### Log.H
  JEVOIS_PYTHON_CONSTANT(LOG_DEBUG);
  JEVOIS_PYTHON_CONSTANT(LOG_INFO);
  JEVOIS_PYTHON_CONSTANT(LOG_ERR);
  JEVOIS_PYTHON_CONSTANT(LOG_CRIT);

  boost::python::def("LDEBUG", pythonLDEBUG);
  boost::python::def("LINFO", pythonLINFO);
  boost::python::def("LERROR", pythonLERROR);
  boost::python::def("LFATAL", pythonLFATAL);
  
  // #################### Utils.H
  JEVOIS_PYTHON_FUNC(fccstr);
  JEVOIS_PYTHON_FUNC(v4l2BytesPerPix);
  JEVOIS_PYTHON_FUNC(v4l2ImageSize);
  JEVOIS_PYTHON_FUNC(flushcache);

  // #################### Coordinates.H
  void (*imgToStd1)(float & x, float & y, jevois::RawImage const & camimg, float const eps) = jevois::coords::imgToStd;
  void (*imgToStd2)(float & x, float & y, unsigned int const width, unsigned int const height, float const eps) =
    jevois::coords::imgToStd;
  boost::python::def("imgToStd", imgToStd1);
  boost::python::def("imgToStd", imgToStd2);

  void (*stdToImg1)(float & x, float & y, jevois::RawImage const & camimg, float const eps) = jevois::coords::stdToImg;
  void (*stdToImg2)(float & x, float & y, unsigned int const width, unsigned int const height, float const eps) =
    jevois::coords::stdToImg;
  boost::python::def("imgToStd", stdToImg1);
  boost::python::def("imgToStd", stdToImg2);

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

  boost::python::enum_<jevois::python::YUYV>("YUYV")
    .JEVOIS_PYTHON_ENUM_VAL(Black)
    .JEVOIS_PYTHON_ENUM_VAL(DarkGrey)
    .JEVOIS_PYTHON_ENUM_VAL(MedGrey)
    .JEVOIS_PYTHON_ENUM_VAL(LightGrey)
    .JEVOIS_PYTHON_ENUM_VAL(White)
    .JEVOIS_PYTHON_ENUM_VAL(DarkGreen)
    .JEVOIS_PYTHON_ENUM_VAL(MedGreen)
    .JEVOIS_PYTHON_ENUM_VAL(LightGreen)
    .JEVOIS_PYTHON_ENUM_VAL(DarkTeal)
    .JEVOIS_PYTHON_ENUM_VAL(MedTeal)
    .JEVOIS_PYTHON_ENUM_VAL(LightTeal)
    .JEVOIS_PYTHON_ENUM_VAL(DarkPurple)
    .JEVOIS_PYTHON_ENUM_VAL(MedPurple)
    .JEVOIS_PYTHON_ENUM_VAL(LightPurple)
    .JEVOIS_PYTHON_ENUM_VAL(DarkPink)
    .JEVOIS_PYTHON_ENUM_VAL(MedPink)
    .JEVOIS_PYTHON_ENUM_VAL(LightPink)
    ;

  JEVOIS_PYTHON_CONSTANT(V4L2_PIX_FMT_SRGGB8);
  JEVOIS_PYTHON_CONSTANT(V4L2_PIX_FMT_YUYV);
  JEVOIS_PYTHON_CONSTANT(V4L2_PIX_FMT_GREY);
  JEVOIS_PYTHON_CONSTANT(V4L2_PIX_FMT_RGB565);
  JEVOIS_PYTHON_CONSTANT(V4L2_PIX_FMT_MJPEG);
  JEVOIS_PYTHON_CONSTANT(V4L2_PIX_FMT_BGR24);
  
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

  void (*drawRect1)(jevois::RawImage & img, int x, int y, unsigned int w,
                    unsigned int h, unsigned int thick, unsigned int col) = jevois::rawimage::drawRect;
  void (*drawRect2)(jevois::RawImage & img, int x, int y, unsigned int w, unsigned int h, unsigned int col) =
    jevois::rawimage::drawRect;
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

  void (*writeText1)(jevois::RawImage & img, std::string const & txt, int x, int y,
                     unsigned int col, jevois::rawimage::Font font) = jevois::rawimage::writeText;
  boost::python::def("writeText", writeText1);

  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvBGRtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvRGBAtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvGRAYtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(unpackCvRGBAtoGrayRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(hFlipYUYV);

  // #################### Timer.H
  boost::python::class_<jevois::Timer>("Timer", boost::python::init<char const *, size_t, int>())
    .def("start", &jevois::Timer::start)
    .def("stop", &jevois::Timer::stop, boost::python::return_value_policy<boost::python::copy_const_reference>())
    ;

  // #################### Profiler.H
  boost::python::class_<jevois::Profiler>("Profiler", boost::python::init<char const *, size_t, int>())
    .def("start", &jevois::Profiler::start)
    .def("checkpoint", &jevois::Profiler::checkpoint)
    .def("stop", &jevois::Profiler::stop, boost::python::return_value_policy<boost::python::copy_const_reference>())
    ;

  // #################### SysInfo.H
  JEVOIS_PYTHON_FUNC(getSysInfoCPU);
  JEVOIS_PYTHON_FUNC(getSysInfoMem);
  JEVOIS_PYTHON_FUNC(getSysInfoVersion);

}
