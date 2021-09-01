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
#include <jevois/Debug/PythonException.H>

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

bool jevois::InputFramePython::hasScaledImage() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->hasScaledImage();
}

jevois::RawImage const & jevois::InputFramePython::get21(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->get2(casync);
}

jevois::RawImage const & jevois::InputFramePython::get2() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->get2();
}

jevois::RawImage const & jevois::InputFramePython::getp1(bool casync) const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getp(casync);
}

jevois::RawImage const & jevois::InputFramePython::getp() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getp();
}

void jevois::InputFramePython::done() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  itsInputFrame->done();
}

void jevois::InputFramePython::done2() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  itsInputFrame->done2();
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

cv::Mat jevois::InputFramePython::getCvGRAYp() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvGRAYp();
}

cv::Mat jevois::InputFramePython::getCvBGRp() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvBGRp();
}

cv::Mat jevois::InputFramePython::getCvRGBp() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvRGBp();
}

cv::Mat jevois::InputFramePython::getCvRGBAp() const
{
  if (itsInputFrame == nullptr) LFATAL("Internal error");
  return itsInputFrame->getCvRGBAp();
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

void jevois::OutputFramePython::sendCv1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCv(img, quality);
}

void jevois::OutputFramePython::sendCv(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendCv(img);
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

void jevois::OutputFramePython::sendScaledCvGRAY1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvGRAY(img, quality);
}

void jevois::OutputFramePython::sendScaledCvGRAY(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvGRAY(img);
}

void jevois::OutputFramePython::sendScaledCvBGR1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvBGR(img, quality);
}

void jevois::OutputFramePython::sendScaledCvBGR(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvBGR(img);
}

void jevois::OutputFramePython::sendScaledCvRGB1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvRGB(img, quality);
}

void jevois::OutputFramePython::sendScaledCvRGB(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvRGB(img);
}

void jevois::OutputFramePython::sendScaledCvRGBA1(cv::Mat const & img, int quality) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvRGBA(img, quality);
}

void jevois::OutputFramePython::sendScaledCvRGBA(cv::Mat const & img) const
{
  if (itsOutputFrame == nullptr) LFATAL("Internal error");
  itsOutputFrame->sendScaledCvRGBA(img);
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
#ifdef JEVOIS_PRO
jevois::GUIhelperPython::GUIhelperPython(GUIhelper * src) : itsGUIhelper(src)
{ }

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::startFrame()
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  unsigned short w, h;
  bool idle = itsGUIhelper->startFrame(w, h);
  return boost::python::make_tuple(idle, w, h);
}

// ####################################################################################################
bool jevois::GUIhelperPython::frameStarted() const
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  return itsGUIhelper->frameStarted();
}
   
// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawImage(char const * name, RawImage const & img,
                                                        bool noalias, bool isoverlay)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawImage(name, img, x, y, w, h, noalias, isoverlay);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawImage1(char const * name, cv::Mat const & img, bool rgb,
                                                         bool noalias, bool isoverlay)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawImage(name, img, rgb, x, y, w, h, noalias, isoverlay);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawInputFrame(char const * name, InputFramePython const & frame,
                                                             bool noalias, bool casync)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawInputFrame(name, *frame.itsInputFrame, x, y, w, h, noalias, casync);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawInputFrame2(char const * name, InputFramePython const & frame,
                                                              bool noalias, bool casync)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawInputFrame(name, *frame.itsInputFrame, x, y, w, h, noalias, casync);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2d(ImVec2 p, char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  return itsGUIhelper->i2d(p, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2d1(float x, float y, char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  return itsGUIhelper->i2d(x, y, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2ds(ImVec2 p, char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  return itsGUIhelper->i2ds(p, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2ds1(float x, float y, char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  return itsGUIhelper->i2ds(x, y, name);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawLine(float x1, float y1, float x2, float y2, ImU32 col)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->drawLine(x1, y1, x2, y2, col);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawRect(float x1, float y1, float x2, float y2, ImU32 col, bool filled)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->drawRect(x1, y1, x2, y2, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawPoly(std::vector<cv::Point> const & pts, ImU32 col, bool filled)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->drawPoly(pts, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawPoly1(std::vector<cv::Point2f> const & pts, ImU32 col, bool filled)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->drawPoly(pts, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawPoly2(cv::Mat const & pts, ImU32 col, bool filled)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  if (pts.type() != CV_32FC2) LFATAL("Incorrect type: should be 32FC2");
  
  // Convert mat to vector of Point2f:
  std::vector<cv::Point2f> p;
  size_t const sz = pts.total() * 2;
  float const * ptr = pts.ptr<float>(0);
  for (size_t i = 0; i < sz; i += 2) p.emplace_back(cv::Point2f(ptr[i], ptr[i+1]));

  // Draw:
  itsGUIhelper->drawPoly(p, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawCircle(float x, float y, float r, ImU32 col, bool filled)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->drawCircle(x, y, r, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawText(float x, float y, char const * txt, ImU32 col)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->drawText(x, y, txt, col);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::iline(int line, char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  return itsGUIhelper->iline(line, name);
}

// ####################################################################################################
void jevois::GUIhelperPython::itext(char const * txt, ImU32 const & col, int line)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->itext(txt, col, line);
}

// ####################################################################################################
void jevois::GUIhelperPython::iinfo(jevois::InputFramePython const & inframe, std::string const & fpscpu,
                                    unsigned short winw, unsigned short winh)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->iinfo(*inframe.itsInputFrame, fpscpu, winw, winh);
}

// ####################################################################################################
void jevois::GUIhelperPython::releaseImage(char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->releaseImage(name);
}

// ####################################################################################################
void jevois::GUIhelperPython::releaseImage2(char const * name)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->releaseImage2(name);
}

// ####################################################################################################
void jevois::GUIhelperPython::endFrame()
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->endFrame();
}

// ####################################################################################################
void jevois::GUIhelperPython::reportError(std::string const & err)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->reportError(err);
}

// ####################################################################################################
void jevois::GUIhelperPython::reportAndIgnoreException(std::string const & prefix)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->reportAndIgnoreException(prefix);
}

// ####################################################################################################
void jevois::GUIhelperPython::reportAndRethrowException(std::string const & prefix)
{
  if (itsGUIhelper == nullptr) LFATAL("Internal error");
  itsGUIhelper->reportAndRethrowException(prefix);
}

#endif // JEVOIS_PRO

// ####################################################################################################
// ####################################################################################################
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
    jevois::Module(m.modulename), itsMainModule(), itsMainNamespace(), itsInstance(), itsConstructionError()
{
  if (m.ispython == false) LFATAL("Passed video mapping is not for a python module");

  // Do not throw during construction because we need users to be able to save modified code from JeVois Inventor, which
  // requires a valid module with a valid path being set by Engine:
  try
  {
    // Get the python interpreter going:
    itsMainModule = boost::python::import("__main__");
    itsMainNamespace = itsMainModule.attr("__dict__");
    
    // Import the module. Note that we import the whole directory:
    std::string const pypath = m.sopath();
    std::string const pydir = pypath.substr(0, pypath.rfind('/'));
    std::string const execstr =
      "import sys\n"
      "sys.path.append(\"/usr/lib\")\n" // To find libjevois[pro] module in /usr/lib
      "sys.path.append(\"" JEVOIS_CONFIG_PATH "\")\n" // To find pyjevois.py config
      "sys.path.append(\"" JEVOIS_OPENCV_PYTHON_PATH "\")\n" // To find cv2 module
      "sys.path.append(\"" + pydir + "\")\n" +
      "import " + m.modulename + "\n" +
      "import importlib\n" +
      "importlib.reload(" + m.modulename + ")\n"; // reload so we are always fresh if file changed on SD card

    boost::python::exec(execstr.c_str(), itsMainNamespace, itsMainNamespace);
    
    // Create an instance of the python class defined in the module:
    itsInstance = boost::python::eval((m.modulename + "." + m.modulename + "()").c_str(),
                                      itsMainNamespace, itsMainNamespace);
  }
  catch (boost::python::error_already_set & e)
  {
    itsConstructionError = "Initialization of Module " + m.modulename + " failed:\n\n" +
      jevois::getPythonExceptionString(e);
  }
}

// ####################################################################################################
void jevois::PythonModule::postUninit()
{
  // Call python module's uninit() function if implemented:
  if (hasattr(itsInstance, "uninit")) itsInstance.attr("uninit")();
}

// ####################################################################################################
jevois::PythonModule::~PythonModule()
{ }

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe, OutputFrame && outframe)
{
  if (itsInstance.is_none()) throw std::runtime_error(itsConstructionError);
  
  jevois::InputFramePython inframepy(&inframe);
  jevois::OutputFramePython outframepy(&outframe);
  itsInstance.attr("process")(boost::ref(inframepy), boost::ref(outframepy));
}

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe)
{
  if (itsInstance.is_none()) throw std::runtime_error(itsConstructionError);
  
  jevois::InputFramePython inframepy(&inframe);
  itsInstance.attr("processNoUSB")(boost::ref(inframepy));
}

#ifdef JEVOIS_PRO

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe, GUIhelper & helper)
{
  if (itsInstance.is_none()) throw std::runtime_error(itsConstructionError);
  
  jevois::InputFramePython inframepy(&inframe);
  jevois::GUIhelperPython helperpy(&helper);
  itsInstance.attr("processGUI")(boost::ref(inframepy), boost::ref(helperpy));
}

#endif

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

 
