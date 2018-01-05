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

#include <jevois/Core/PythonModule.H>
#include <jevois/Core/UserInterface.H>

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################

jevois::InputFramePython::InputFramePython(InputFrame * src) : itsInputFrame(src)
{ }

jevois::RawImage const & jevois::InputFramePython::get1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->get(casync);
}

jevois::RawImage const & jevois::InputFramePython::get() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->get();
}

void jevois::InputFramePython::done() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  itsInputFrame->done();
}

cv::Mat jevois::InputFramePython::getCvGRAY1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvGRAY(casync);
}

cv::Mat jevois::InputFramePython::getCvGRAY() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvGRAY();
}

cv::Mat jevois::InputFramePython::getCvBGR1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvBGR(casync);
}

cv::Mat jevois::InputFramePython::getCvBGR() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvBGR();
}

cv::Mat jevois::InputFramePython::getCvRGB1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvRGB(casync);
}

cv::Mat jevois::InputFramePython::getCvRGB() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvRGB();
}

cv::Mat jevois::InputFramePython::getCvRGBA1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvRGBA(casync);
}

cv::Mat jevois::InputFramePython::getCvRGBA() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvRGBA();
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::OutputFramePython::OutputFramePython(OutputFrame * src) : itsOutputFrame(src)
{ }

jevois::RawImage const & jevois::OutputFramePython::get() const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  return itsOutputFrame->get();
}

void jevois::OutputFramePython::send() const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->send();
}

void jevois::OutputFramePython::sendCvGRAY1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvGRAY(img, quality);
}

void jevois::OutputFramePython::sendCvGRAY(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvGRAY(img);
}

void jevois::OutputFramePython::sendCvBGR1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvBGR(img, quality);
}

void jevois::OutputFramePython::sendCvBGR(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvBGR(img);
}

void jevois::OutputFramePython::sendCvRGB1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvRGB(img, quality);
}

void jevois::OutputFramePython::sendCvRGB(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvRGB(img);
}

void jevois::OutputFramePython::sendCvRGBA1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvRGBA(img, quality);
}

void jevois::OutputFramePython::sendCvRGBA(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCvRGBA(img);
}

// ####################################################################################################
namespace
{
  // from https://stackoverflow.com/questions/39924912/finding-if-member-function-exists-in-a-boost-pythonobject
  bool hasattr(boost::python::object & o, char const * name) { return PyObject_HasAttrString(o.ptr(), name); }
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::PythonModule::PythonModule(jevois::VideoMapping const & m) :
    jevois::Module(m.modulename)
{
  if (m.ispython == false) LFATAL("Passed video mapping is not for a python module");

  // Get the python interpreter going:
  itsMainModule = boost::python::import("__main__");
  itsMainNamespace = itsMainModule.attr("__dict__");

  // Import the module. Note that we import the whole directory:
  std::string const pypath = m.sopath();
  std::string const pydir = pypath.substr(0, pypath.rfind('/'));
  std::string const execstr =
    "import sys\n"
    "sys.path.append(\"/usr/lib\")\n" // To find libjevois module in /usr/lib
    "sys.path.append(\"" JEVOIS_OPENCV_PYTHON_PATH "\")\n" // To find cv2 module
    "sys.path.append(\"" + pydir + "\")\n" +
    "from " + m.modulename + " import " + m.modulename + "\n";
  boost::python::exec(execstr.c_str(), itsMainNamespace, itsMainNamespace);

  // Create an instance of the python class defined in the module:
  itsInstance = boost::python::eval((m.modulename + "()").c_str(), itsMainNamespace, itsMainNamespace);
}

// ####################################################################################################
jevois::PythonModule::~PythonModule()
{ }

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe, OutputFrame && outframe)
{
  jevois::InputFramePython inframepy(&inframe);
  jevois::OutputFramePython outframepy(&outframe);
  itsInstance.attr("process")(boost::ref(inframepy), boost::ref(outframepy));
}

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe)
{
  jevois::InputFramePython inframepy(&inframe);
  itsInstance.attr("processNoUSB")(boost::ref(inframepy));
}

// ####################################################################################################
void jevois::PythonModule::parseSerial(std::string const & str, std::shared_ptr<UserInterface> s)
{
  if (hasattr(itsInstance, "parseSerial"))
  {
    boost::python::object ret = itsInstance.attr("parseSerial")(str);
    std::string retstr = boost::python::extract<std::string>(ret);
    if (retstr.empty() == false) s->writeString(retstr);
  }
  else jevois::Module::parseSerial(str, s);
}

// ####################################################################################################
void jevois::PythonModule::supportedCommands(std::ostream & os)
{
  if (hasattr(itsInstance, "supportedCommands"))
  {
    boost::python::object ret = itsInstance.attr("supportedCommands")();
    std::string retstr = boost::python::extract<std::string>(ret);
    if (retstr.empty() == false) os << retstr;
  } else jevois::Module::supportedCommands(os);
}
