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
#include <jevois/Core/PythonParameter.H>
#include <jevois/Core/UserInterface.H>
#include <jevois/Core/StdioInterface.H>
#include <jevois/Core/Serial.H>
#include <jevois/Util/Utils.H>
#include <jevois/Util/Coordinates.H>
#include <jevois/Debug/Log.H>
#include <jevois/Debug/Timer.H>
#include <jevois/Debug/Profiler.H>
#include <jevois/Debug/SysInfo.H>
#include <jevois/Core/Camera.H>
#include <jevois/Core/IMU.H>
#include <jevois/Component/ParameterStringConversion.H>
#include <jevois/DNN/PreProcessorPython.H>

#define PY_ARRAY_UNIQUE_SYMBOL pbcvt_ARRAY_API
#include <jevois/Core/PythonOpenCV.H>

// ####################################################################################################
// Convenience macro to define a Python binding for a free function in the jevois namespace
#define JEVOIS_PYTHON_FUNC(funcname)                \
  boost::python::def(#funcname, jevois::funcname)

// Convenience macro to define a Python binding for a free function in the jevois::rawimage namespace
#define JEVOIS_PYTHON_RAWIMAGE_FUNC(funcname)               \
  boost::python::def(#funcname, jevois::rawimage::funcname)

// Convenience macro to define a python enum value where the value is in jevois::rawimage
#define JEVOIS_PYTHON_RAWIMAGE_ENUM_VAL(val) value(#val, jevois::rawimage::val)

// Convenience macro to define a python enum value where the value is in jevois::python
#define JEVOIS_PYTHON_ENUM_VAL(val) value(#val, jevois::python::val)

// Convenience macro to define a constant that exists in the global C++ namespace
#define JEVOIS_PYTHON_CONSTANT(cst) boost::python::scope().attr(#cst) = cst;

// Convenience macro to define a Python binding for a free function in the jevois:dnn namespace
#define JEVOIS_PYTHON_DNN_FUNC(funcname)                \
  boost::python::def(#funcname, jevois::dnn::funcname)

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
    Py_SetProgramName(Py_DecodeLocale("", nullptr)); // black magic
    Py_Initialize();

    // Initialize numpy array. Use the signal handler hack to prevent numpy from grabbing CTRL-C from
    // https://stackoverflow.com/questions/28750774/
    //         python-import-array-makes-it-impossible-to-kill-embedded-python-with-ctrl-c
    //PyOS_sighandler_t sighandler = PyOS_getsig(SIGINT);
    import_array();
    //PyOS_setsig(SIGINT,sighandler);
    return nullptr; // was NUMPY_IMPORT_ARRAY_RETVAL in older numpy
  }
}

void jevois::python::setEngine(jevois::Engine * e)
{
  jevois::python::engineForPythonModule = e;
  init_numpy();
}

jevois::Engine * jevois::python::engine()
{
  if (jevois::python::engineForPythonModule == nullptr) LFATAL("Internal error");
  return jevois::python::engineForPythonModule;
}

// ####################################################################################################
// from https://stackoverflow.com/questions/39924912/finding-if-member-function-exists-in-a-boost-pythonobject
bool jevois::python::hasattr(boost::python::object & o, char const * name)
{
  return PyObject_HasAttrString(o.ptr(), name);
}

// ####################################################################################################
// Thin wrappers to handle default arguments or overloads in free functions

namespace
{
  void pythonSendSerial(std::string const & str)
  { jevois::python::engine()->sendSerial(str); }
  
  size_t pythonFrameNum()
  { return jevois::frameNum(); }

  void pythonWriteCamRegister(unsigned short reg, unsigned short val)
  {
    auto cam = jevois::python::engine()->camera();
    if (!cam) LFATAL("Not using a Camera for video input");
    cam->writeRegister(reg, val);
  }

  unsigned short pythonReadCamRegister(unsigned short reg)
  {
    auto cam = jevois::python::engine()->camera();
    if (!cam) LFATAL("Not using a Camera for video input");
    return cam->readRegister(reg);
  }

  void pythonWriteIMUregister(unsigned short reg, unsigned short val)
  {
    auto imu = jevois::python::engine()->imu();
    if (!imu) LFATAL("No IMU driver loaded");
    imu->writeRegister(reg, val);
  }
  
  unsigned short pythonReadIMUregister(unsigned short reg)
  {
    auto imu = jevois::python::engine()->imu();
    if (!imu) LFATAL("No IMU driver loaded");
    return imu->readRegister(reg);
  }

  void pythonWriteDMPregister(unsigned short reg, unsigned short val)
  {
    auto imu = jevois::python::engine()->imu();
    if (!imu) LFATAL("No IMU driver loaded");
    imu->writeDMPregister(reg, val);
  }
  
  unsigned short pythonReadDMPregister(unsigned short reg)
  {
    auto imu = jevois::python::engine()->imu();
    if (!imu) LFATAL("No IMU driver loaded");
    return imu->readDMPregister(reg);
  }
  
#ifdef JEVOIS_LDEBUG_ENABLE
  void pythonLDEBUG(std::string const & logmsg) { LDEBUG(logmsg); }
#else
  void pythonLDEBUG(std::string const & JEVOIS_UNUSED_PARAM(logmsg)) { }
#endif
  void pythonLINFO(std::string const & logmsg) { LINFO(logmsg); }
  void pythonLERROR(std::string const & logmsg) { LERROR(logmsg); }
  void pythonLFATAL(std::string const & logmsg) { LFATAL(logmsg); }


  // ####################################################################################################
  // Python parameter access support
  std::string pythonGetParamStringUnique(boost::python::object & pyinst, std::string const & descriptor)
  {
    jevois::Component * comp = jevois::python::engine()->getPythonComponent(pyinst.ptr()->ob_type);
    return comp->getParamStringUnique(descriptor);
  }

  void pythonSetParamStringUnique(boost::python::object & pyinst, std::string const & descriptor,
                                  std::string const & val)
  {
    jevois::Component * comp = jevois::python::engine()->getPythonComponent(pyinst.ptr()->ob_type);
    comp->setParamStringUnique(descriptor, val);
  }

} // anonymous namespace

// ####################################################################################################
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
#ifdef JEVOIS_PRO
BOOST_PYTHON_MODULE(libjevoispro)
#else
BOOST_PYTHON_MODULE(libjevois)
#endif
{
  // #################### Initialize converters for cv::Mat support:
  boost::python::to_python_converter<cv::Mat, pbcvt::matToNDArrayBoostConverter>();
  pbcvt::matFromNDArrayBoostConverter();
  
  // #################### Define a few constants for python users:
  // jevois.pro is True if running on JeVois-Pro
#ifdef JEVOIS_PRO
  boost::python::scope().attr("pro") = true;
#else
  boost::python::scope().attr("pro") = false;
#endif

  // jevois.platform is True if running on platform hardware
#ifdef JEVOIS_PLATFORM
  boost::python::scope().attr("platform") = true;
#else
  boost::python::scope().attr("platform") = false;
#endif

  // Jevois software version:
  boost::python::scope().attr("version_major") = JEVOIS_VERSION_MAJOR;
  boost::python::scope().attr("version_minor") = JEVOIS_VERSION_MINOR;
  boost::python::scope().attr("version_patch") = JEVOIS_VERSION_PATCH;

  // Location of shared directories: jevois.share points to /jevois/share or /jevoispro/share
  boost::python::scope().attr("share") = JEVOIS_SHARE_PATH;
  boost::python::scope().attr("pydnn") = JEVOIS_PYDNN_PATH;

  // #################### module sendSerial() and other functions emulation:
  boost::python::def("sendSerial", pythonSendSerial);
  boost::python::def("frameNum", pythonFrameNum);
  boost::python::def("writeCamRegister", pythonWriteCamRegister);
  boost::python::def("readCamRegister", pythonReadCamRegister);
  boost::python::def("writeIMUregister", pythonWriteIMUregister);
  boost::python::def("readIMUregister", pythonReadIMUregister);
  boost::python::def("writeDMPregister", pythonWriteDMPregister);
  boost::python::def("readDMPregister", pythonReadDMPregister);
  
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
  JEVOIS_PYTHON_FUNC(cvtypestr);
  JEVOIS_PYTHON_FUNC(cvBytesPerPix);
  JEVOIS_PYTHON_FUNC(strfcc);
  JEVOIS_PYTHON_FUNC(v4l2BytesPerPix);
  JEVOIS_PYTHON_FUNC(v4l2ImageSize);
  JEVOIS_PYTHON_FUNC(blackColor);
  JEVOIS_PYTHON_FUNC(whiteColor);
  JEVOIS_PYTHON_FUNC(flushcache);
  JEVOIS_PYTHON_FUNC(system);

  // #################### Coordinates.H
  void (*imgToStd1)(float & x, float & y, jevois::RawImage const & camimg, float const eps) = jevois::coords::imgToStd;
  void (*imgToStd2)(float & x, float & y, unsigned int const width, unsigned int const height, float const eps) =
    jevois::coords::imgToStd;
  boost::python::def("imgToStd", imgToStd1);
  boost::python::def("imgToStd", imgToStd2);

  void (*stdToImg1)(float & x, float & y, jevois::RawImage const & camimg, float const eps) = jevois::coords::stdToImg;
  void (*stdToImg2)(float & x, float & y, unsigned int const width, unsigned int const height, float const eps) =
    jevois::coords::stdToImg;
  boost::python::def("stdToImg", stdToImg1);
  boost::python::def("stdToImg", stdToImg2);

  boost::python::def("imgToStdX", jevois::coords::imgToStdX);
  boost::python::def("imgToStdY", jevois::coords::imgToStdY);
  boost::python::def("imgToStdSize", jevois::coords::imgToStdSize);
  boost::python::def("stdToImgSize", jevois::coords::stdToImgSize);
  
  // #################### RawImage.H
  boost::python::class_<jevois::RawImage>("RawImage") // default constructor is included
    .def("invalidate", &jevois::RawImage::invalidate)
    .def("valid", &jevois::RawImage::valid)
    .def("clear", &jevois::RawImage::clear)
    .def("require", &jevois::RawImage::require)
    .def_readwrite("width", &jevois::RawImage::width)
    .def_readwrite("height", &jevois::RawImage::height)
    .def_readwrite("fmt", &jevois::RawImage::fmt)
    .def_readwrite("fps", &jevois::RawImage::fps)
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
    .def("hasScaledImage", &jevois::InputFramePython::hasScaledImage)
    .def("get2", &jevois::InputFramePython::get21,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("get2", &jevois::InputFramePython::get2,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("getp", &jevois::InputFramePython::getp1,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("getp", &jevois::InputFramePython::getp,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("done", &jevois::InputFramePython::done)
    .def("done2", &jevois::InputFramePython::done2)
    .def("getCvGRAY",  &jevois::InputFramePython::getCvGRAY1)
    .def("getCvGRAY",  &jevois::InputFramePython::getCvGRAY)
    .def("getCvBGR",  &jevois::InputFramePython::getCvBGR1)
    .def("getCvBGR",  &jevois::InputFramePython::getCvBGR)
    .def("getCvRGB",  &jevois::InputFramePython::getCvRGB1)
    .def("getCvRGB",  &jevois::InputFramePython::getCvRGB)
    .def("getCvRGBA",  &jevois::InputFramePython::getCvRGBA1)
    .def("getCvRGBA",  &jevois::InputFramePython::getCvRGBA)

    .def("getCvGRAYp",  &jevois::InputFramePython::getCvGRAYp)
    .def("getCvBGRp",  &jevois::InputFramePython::getCvBGRp)
    .def("getCvRGBp",  &jevois::InputFramePython::getCvRGBp)
    .def("getCvRGBAp",  &jevois::InputFramePython::getCvRGBAp)
    ;
  
  boost::python::class_<jevois::OutputFramePython>("OutputFrame")
    .def("get", &jevois::OutputFramePython::get,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("send", &jevois::OutputFramePython::send)

    .def("sendCv",  &jevois::OutputFramePython::sendCv1)
    .def("sendCv",  &jevois::OutputFramePython::sendCv)

    .def("sendCvGRAY",  &jevois::OutputFramePython::sendCvGRAY1)
    .def("sendCvGRAY",  &jevois::OutputFramePython::sendCvGRAY)
    .def("sendCvBGR",  &jevois::OutputFramePython::sendCvBGR1)
    .def("sendCvBGR",  &jevois::OutputFramePython::sendCvBGR)
    .def("sendCvRGB",  &jevois::OutputFramePython::sendCvRGB1)
    .def("sendCvRGB",  &jevois::OutputFramePython::sendCvRGB)
    .def("sendCvRGBA",  &jevois::OutputFramePython::sendCvRGBA1)
    .def("sendCvRGBA",  &jevois::OutputFramePython::sendCvRGBA)

    .def("sendScaledCvGRAY",  &jevois::OutputFramePython::sendScaledCvGRAY1)
    .def("sendScaledCvGRAY",  &jevois::OutputFramePython::sendScaledCvGRAY)
    .def("sendScaledCvBGR",  &jevois::OutputFramePython::sendScaledCvBGR1)
    .def("sendScaledCvBGR",  &jevois::OutputFramePython::sendScaledCvBGR)
    .def("sendScaledCvRGB",  &jevois::OutputFramePython::sendScaledCvRGB1)
    .def("sendScaledCvRGB",  &jevois::OutputFramePython::sendScaledCvRGB)
    .def("sendScaledCvRGBA",  &jevois::OutputFramePython::sendScaledCvRGBA1)
    .def("sendScaledCvRGBA",  &jevois::OutputFramePython::sendScaledCvRGBA)
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

  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvGRAYtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvBGRtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvRGBtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvRGBAtoRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(unpackCvRGBAtoGrayRawImage);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(hFlipYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvRGBtoCvYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvBGRtoCvYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvGRAYtoCvYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertCvRGBAtoCvYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertBayerToYUYV);
  JEVOIS_PYTHON_RAWIMAGE_FUNC(convertGreyToYUYV);
  boost::python::def("rescaleCv", jevois::rescaleCv);

  // #################### Timer.H
  std::string const & (jevois::Timer::*timer_stop)() = &jevois::Timer::stop; // select overload with no args
  boost::python::class_<jevois::Timer>("Timer", boost::python::init<char const *, size_t, int>())
    .def("start", &jevois::Timer::start)
    .def("stop", timer_stop, boost::python::return_value_policy<boost::python::copy_const_reference>());

  // #################### Profiler.H
  boost::python::class_<jevois::Profiler>("Profiler", boost::python::init<char const *, size_t, int>())
    .def("start", &jevois::Profiler::start)
    .def("checkpoint", &jevois::Profiler::checkpoint)
    .def("stop", &jevois::Profiler::stop, boost::python::return_value_policy<boost::python::copy_const_reference>());

  // #################### SysInfo.H
  JEVOIS_PYTHON_FUNC(getSysInfoCPU);
  JEVOIS_PYTHON_FUNC(getSysInfoMem);
  JEVOIS_PYTHON_FUNC(getSysInfoVersion);

#ifdef JEVOIS_PRO
  // #################### GUIhelper.H
  boost::python::class_<ImVec2>("ImVec2", boost::python::init<float, float>())
    .def_readwrite("x", &ImVec2::x)
    .def_readwrite("y", &ImVec2::y);

  boost::python::class_<ImVec4>("ImVec4", boost::python::init<float, float, float, float>())
    .def_readwrite("x", &ImVec4::x)
    .def_readwrite("y", &ImVec4::y)
    .def_readwrite("z", &ImVec4::z)
    .def_readwrite("w", &ImVec4::w);

  boost::python::class_<ImColor>("ImColor", boost::python::init<int, int, int, int>())
    .def(boost::python::init<float, float, float, float>())
    .def(boost::python::init<ImU32>())
    .def_readwrite("Value", &ImColor::Value);

  boost::python::class_<jevois::GUIhelperPython>("GUIhelper", boost::python::init<jevois::GUIhelper *>())
    .def("startFrame", &jevois::GUIhelperPython::startFrame)
    .def("frameStarted", &jevois::GUIhelperPython::frameStarted)
    .def("drawImage", &jevois::GUIhelperPython::drawImage)
    .def("drawImage", &jevois::GUIhelperPython::drawImage1)
    .def("drawInputFrame", &jevois::GUIhelperPython::drawInputFrame)
    .def("drawInputFrame2", &jevois::GUIhelperPython::drawInputFrame2)
    .def("i2d", &jevois::GUIhelperPython::i2d)
    .def("i2d", &jevois::GUIhelperPython::i2d1)
    .def("i2ds", &jevois::GUIhelperPython::i2ds)
    .def("i2ds", &jevois::GUIhelperPython::i2ds1)
    .def("drawLine", &jevois::GUIhelperPython::drawLine)
    .def("drawRect", &jevois::GUIhelperPython::drawRect)
    .def("drawPoly", &jevois::GUIhelperPython::drawPoly)
    .def("drawPoly", &jevois::GUIhelperPython::drawPoly1)
    .def("drawPoly", &jevois::GUIhelperPython::drawPoly2)
    .def("drawCircle", &jevois::GUIhelperPython::drawCircle)
    .def("drawText", &jevois::GUIhelperPython::drawText)
    .def("iline", &jevois::GUIhelperPython::iline)
    .def("itext", &jevois::GUIhelperPython::itext)
    .def("iinfo", &jevois::GUIhelperPython::iinfo)
    .def("releaseImage", &jevois::GUIhelperPython::releaseImage)
    .def("releaseImage2", &jevois::GUIhelperPython::releaseImage2)
    .def("endFrame", &jevois::GUIhelperPython::endFrame)
    .def("d2i", &jevois::GUIhelperPython::d2i)
    .def("d2i", &jevois::GUIhelperPython::d2i1)
    .def("d2is", &jevois::GUIhelperPython::d2is)
    .def("d2is", &jevois::GUIhelperPython::d2is1)
    .def("reportError", &jevois::GUIhelperPython::reportError)
    .def("reportAndIgnoreException", &jevois::GUIhelperPython::reportAndIgnoreException)
    .def("reportAndRethrowException", &jevois::GUIhelperPython::reportAndRethrowException)
    ;
#endif
  
  // #################### ParameterDef.H
  boost::python::class_<jevois::ParameterCategory>("ParameterCategory", boost::python::init<std::string, std::string>())
    .def_readonly("name", &jevois::ParameterCategory::name)
    .def_readonly("description", &jevois::ParameterCategory::description)
    ;

  // #################### Parameter support:
  boost::python::def("getParamStr", pythonGetParamStringUnique);
  boost::python::def("setParamStr", pythonSetParamStringUnique);

  // #################### Dynamic parameter support:
  boost::python::class_<jevois::PythonParameter>("Parameter", boost::python::init<boost::python::object &,
                                                 std::string const &, std::string const &, std::string const &,
                                                 boost::python::object const &, jevois::ParameterCategory const &>())
    .def("name", &jevois::PythonParameter::name,
         boost::python::return_value_policy<boost::python::reference_existing_object>())
    .def("descriptor", &jevois::PythonParameter::descriptor)
    .def("get", &jevois::PythonParameter::get)
    .def("set", &jevois::PythonParameter::set)
    .def("strget", &jevois::PythonParameter::strget)
    .def("strset", &jevois::PythonParameter::strset)
    .def("freeze", &jevois::PythonParameter::freeze)
    .def("reset", &jevois::PythonParameter::reset)
    .def("setCallback", &jevois::PythonParameter::setCallback)
    ;

  // #################### Allow python code to access dnn::PreProcessor helper functions:
  // python cannot construct PreProcessor, but PostProcessorPython can use an existing one.
  boost::python::class_<jevois::dnn::PreProcessorForPython>("PreProcessor", boost::python::no_init)
    .def("imagesize", &jevois::dnn::PreProcessorForPython::imagesize)
    .def("blobs", &jevois::dnn::PreProcessorForPython::blobs)
    .def("blobsize", &jevois::dnn::PreProcessorForPython::blobsize)
    .def("b2i", &jevois::dnn::PreProcessorForPython::b2i)
    .def("getUnscaledCropRect", &jevois::dnn::PreProcessorForPython::getUnscaledCropRect)
    ;

}
