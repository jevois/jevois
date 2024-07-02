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
#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/UserInterface.H>
#include <jevois/Core/Engine.H>
#include <jevois/Debug/PythonException.H>
#include <jevois/DNN/Utils.H>
#include <jevois/DNN/PreProcessorPython.H>
#include <jevois/DNN/PostProcessorDetectYOLO.H>

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################

jevois::InputFramePython::InputFramePython(InputFrame * src) : itsInputFrame(src)
{ if (itsInputFrame == nullptr) LFATAL("Internal error"); }

jevois::RawImage const & jevois::InputFramePython::get1(bool casync) const
{
  return itsInputFrame->get(casync);
}

jevois::RawImage const & jevois::InputFramePython::get() const
{
  return itsInputFrame->get();
}

bool jevois::InputFramePython::hasScaledImage() const
{
  return itsInputFrame->hasScaledImage();
}

jevois::RawImage const & jevois::InputFramePython::get21(bool casync) const
{
  return itsInputFrame->get2(casync);
}

jevois::RawImage const & jevois::InputFramePython::get2() const
{
  return itsInputFrame->get2();
}

jevois::RawImage const & jevois::InputFramePython::getp1(bool casync) const
{
  return itsInputFrame->getp(casync);
}

jevois::RawImage const & jevois::InputFramePython::getp() const
{
  return itsInputFrame->getp();
}

void jevois::InputFramePython::done() const
{
  itsInputFrame->done();
}

void jevois::InputFramePython::done2() const
{
  itsInputFrame->done2();
}

cv::Mat jevois::InputFramePython::getCvGRAY1(bool casync) const
{
  return itsInputFrame->getCvGRAY(casync);
}

cv::Mat jevois::InputFramePython::getCvGRAY() const
{
  return itsInputFrame->getCvGRAY();
}

cv::Mat jevois::InputFramePython::getCvBGR1(bool casync) const
{
  return itsInputFrame->getCvBGR(casync);
}

cv::Mat jevois::InputFramePython::getCvBGR() const
{
  return itsInputFrame->getCvBGR();
}

cv::Mat jevois::InputFramePython::getCvRGB1(bool casync) const
{
  return itsInputFrame->getCvRGB(casync);
}

cv::Mat jevois::InputFramePython::getCvRGB() const
{
  return itsInputFrame->getCvRGB();
}

cv::Mat jevois::InputFramePython::getCvRGBA1(bool casync) const
{
  return itsInputFrame->getCvRGBA(casync);
}

cv::Mat jevois::InputFramePython::getCvRGBA() const
{
  return itsInputFrame->getCvRGBA();
}

cv::Mat jevois::InputFramePython::getCvGRAYp() const
{
  return itsInputFrame->getCvGRAYp();
}

cv::Mat jevois::InputFramePython::getCvBGRp() const
{
  return itsInputFrame->getCvBGRp();
}

cv::Mat jevois::InputFramePython::getCvRGBp() const
{
  return itsInputFrame->getCvRGBp();
}

cv::Mat jevois::InputFramePython::getCvRGBAp() const
{
  return itsInputFrame->getCvRGBAp();
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::OutputFramePython::OutputFramePython(OutputFrame * src) : itsOutputFrame(src)
{ if (itsOutputFrame == nullptr) LFATAL("Internal error"); }

jevois::RawImage const & jevois::OutputFramePython::get() const
{
  return itsOutputFrame->get();
}

void jevois::OutputFramePython::send() const
{
  itsOutputFrame->send();
}

void jevois::OutputFramePython::sendCv1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendCv(img, quality);
}

void jevois::OutputFramePython::sendCv(cv::Mat const & img) const
{
  itsOutputFrame->sendCv(img);
}

void jevois::OutputFramePython::sendCvGRAY1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendCvGRAY(img, quality);
}

void jevois::OutputFramePython::sendCvGRAY(cv::Mat const & img) const
{
  itsOutputFrame->sendCvGRAY(img);
}

void jevois::OutputFramePython::sendCvBGR1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendCvBGR(img, quality);
}

void jevois::OutputFramePython::sendCvBGR(cv::Mat const & img) const
{
  itsOutputFrame->sendCvBGR(img);
}

void jevois::OutputFramePython::sendCvRGB1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendCvRGB(img, quality);
}

void jevois::OutputFramePython::sendCvRGB(cv::Mat const & img) const
{
  itsOutputFrame->sendCvRGB(img);
}

void jevois::OutputFramePython::sendCvRGBA1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendCvRGBA(img, quality);
}

void jevois::OutputFramePython::sendCvRGBA(cv::Mat const & img) const
{
  itsOutputFrame->sendCvRGBA(img);
}

void jevois::OutputFramePython::sendScaledCvGRAY1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendScaledCvGRAY(img, quality);
}

void jevois::OutputFramePython::sendScaledCvGRAY(cv::Mat const & img) const
{
  itsOutputFrame->sendScaledCvGRAY(img);
}

void jevois::OutputFramePython::sendScaledCvBGR1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendScaledCvBGR(img, quality);
}

void jevois::OutputFramePython::sendScaledCvBGR(cv::Mat const & img) const
{
  itsOutputFrame->sendScaledCvBGR(img);
}

void jevois::OutputFramePython::sendScaledCvRGB1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendScaledCvRGB(img, quality);
}

void jevois::OutputFramePython::sendScaledCvRGB(cv::Mat const & img) const
{
  itsOutputFrame->sendScaledCvRGB(img);
}

void jevois::OutputFramePython::sendScaledCvRGBA1(cv::Mat const & img, int quality) const
{
  itsOutputFrame->sendScaledCvRGBA(img, quality);
}

void jevois::OutputFramePython::sendScaledCvRGBA(cv::Mat const & img) const
{
  itsOutputFrame->sendScaledCvRGBA(img);
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
#ifdef JEVOIS_PRO
jevois::GUIhelperPython::GUIhelperPython(GUIhelper * src) : itsGUIhelper(src)
{ if (itsGUIhelper == nullptr) LFATAL("Internal error"); }

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::startFrame()
{
  unsigned short w, h;
  bool idle = itsGUIhelper->startFrame(w, h);
  return boost::python::make_tuple(idle, w, h);
}

// ####################################################################################################
bool jevois::GUIhelperPython::frameStarted() const
{
  return itsGUIhelper->frameStarted();
}
   
// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawImage(char const * name, RawImage const & img,
                                                        bool noalias, bool isoverlay)
{
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawImage(name, img, x, y, w, h, noalias, isoverlay);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawImage1(char const * name, cv::Mat const & img, bool rgb,
                                                         bool noalias, bool isoverlay)
{
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawImage(name, img, rgb, x, y, w, h, noalias, isoverlay);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawImage2(char const * name, RawImage const & img,
                                                        int x, int y, int w, int h, bool noalias, bool isoverlay)
{
  if (w < 0) LFATAL("w must be positive");
  if (h < 0) LFATAL("h must be positive");
  unsigned short ww = (unsigned short)(w); unsigned short hh = (unsigned short)(h);
  itsGUIhelper->drawImage(name, img, x, y, ww, hh, noalias, isoverlay);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawImage3(char const * name, cv::Mat const & img, bool rgb,
                                                         int x, int y, int w, int h, bool noalias, bool isoverlay)
{
  if (w < 0) LFATAL("w must be positive");
  if (h < 0) LFATAL("h must be positive");
  unsigned short ww = (unsigned short)(w); unsigned short hh = (unsigned short)(h);
  itsGUIhelper->drawImage(name, img, rgb, x, y, ww, hh, noalias, isoverlay);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawInputFrame(char const * name, InputFramePython const & frame,
                                                             bool noalias, bool casync)
{
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawInputFrame(name, *frame.itsInputFrame, x, y, w, h, noalias, casync);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
boost::python::tuple jevois::GUIhelperPython::drawInputFrame2(char const * name, InputFramePython const & frame,
                                                              bool noalias, bool casync)
{
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsGUIhelper->drawInputFrame(name, *frame.itsInputFrame, x, y, w, h, noalias, casync);
  return boost::python::make_tuple(x, y, w, h);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2d(ImVec2 p, char const * name)
{
  return itsGUIhelper->i2d(p, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2d1(float x, float y, char const * name)
{
  return itsGUIhelper->i2d(x, y, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2ds(ImVec2 p, char const * name)
{
  return itsGUIhelper->i2ds(p, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::i2ds1(float x, float y, char const * name)
{
  return itsGUIhelper->i2ds(x, y, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::d2i(ImVec2 p, char const * name)
{
  return itsGUIhelper->d2i(p, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::d2i1(float x, float y, char const * name)
{
  return itsGUIhelper->d2i(x, y, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::d2is(ImVec2 p, char const * name)
{
  return itsGUIhelper->d2is(p, name);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::d2is1(float x, float y, char const * name)
{
  return itsGUIhelper->d2is(x, y, name);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawLine(float x1, float y1, float x2, float y2, ImU32 col)
{
  itsGUIhelper->drawLine(x1, y1, x2, y2, col);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawRect(float x1, float y1, float x2, float y2, ImU32 col, bool filled)
{
  itsGUIhelper->drawRect(x1, y1, x2, y2, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawPoly(std::vector<cv::Point> const & pts, ImU32 col, bool filled)
{
  itsGUIhelper->drawPoly(pts, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawPoly1(std::vector<cv::Point2f> const & pts, ImU32 col, bool filled)
{
  itsGUIhelper->drawPoly(pts, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawPoly2(cv::Mat const & pts, ImU32 col, bool filled)
{
  itsGUIhelper->drawPoly(pts, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawCircle(float x, float y, float r, ImU32 col, bool filled)
{
  itsGUIhelper->drawCircle(x, y, r, col, filled);
}

// ####################################################################################################
void jevois::GUIhelperPython::drawText(float x, float y, char const * txt, ImU32 col)
{
  itsGUIhelper->drawText(x, y, txt, col);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::iline(int line, char const * name)
{
  return itsGUIhelper->iline(line, name);
}

// ####################################################################################################
void jevois::GUIhelperPython::itext(char const * txt, ImU32 const & col, int line)
{
  itsGUIhelper->itext(txt, col, line);
}

// ####################################################################################################
void jevois::GUIhelperPython::itext2(char const * txt)
{
  itsGUIhelper->itext(txt, 0, -1);
}

// ####################################################################################################
void jevois::GUIhelperPython::iinfo(jevois::InputFramePython const & inframe, std::string const & fpscpu,
                                    unsigned short winw, unsigned short winh)
{
  itsGUIhelper->iinfo(*inframe.itsInputFrame, fpscpu, winw, winh);
}

// ####################################################################################################
void jevois::GUIhelperPython::releaseImage(char const * name)
{
  itsGUIhelper->releaseImage(name);
}

// ####################################################################################################
void jevois::GUIhelperPython::releaseImage2(char const * name)
{
  itsGUIhelper->releaseImage2(name);
}

// ####################################################################################################
void jevois::GUIhelperPython::endFrame()
{
  itsGUIhelper->endFrame();
}

// ####################################################################################################
void jevois::GUIhelperPython::reportError(std::string const & err)
{
  itsGUIhelper->reportError(err);
}

// ####################################################################################################
void jevois::GUIhelperPython::reportAndIgnoreException(std::string const & prefix)
{
  itsGUIhelper->reportAndIgnoreException(prefix);
}

// ####################################################################################################
void jevois::GUIhelperPython::reportAndRethrowException(std::string const & prefix)
{
  itsGUIhelper->reportAndRethrowException(prefix);
}

// ####################################################################################################
ImVec2 jevois::GUIhelperPython::getMousePos()
{ return ImGui::GetMousePos(); }

// ####################################################################################################
bool jevois::GUIhelperPython::isMouseClicked(int button_num)
{ return ImGui::IsMouseClicked(button_num); }

// ####################################################################################################
bool jevois::GUIhelperPython::isMouseDoubleClicked(int button_num)
{ return ImGui::IsMouseDoubleClicked(button_num); }

// ####################################################################################################
bool jevois::GUIhelperPython::isMouseDragging(int button_num)
{ return ImGui::IsMouseDragging(button_num); }

// ####################################################################################################
bool jevois::GUIhelperPython::isMouseDown(int button_num)
{ return ImGui::IsMouseDown(button_num); }

// ####################################################################################################
bool jevois::GUIhelperPython::isMouseReleased(int button_num)
{ return ImGui::IsMouseReleased(button_num); }

#endif // JEVOIS_PRO

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::PythonModule::PythonModule(jevois::VideoMapping const & m) :
    jevois::Module(m.modulename), itsPyPath(m.sopath())
{
  if (m.ispython == false) LFATAL("Passed video mapping is not for a python module");
}

// ####################################################################################################
void jevois::PythonModule::preInit()
{
  // Load the python code and instantiate the python class:
  PythonWrapper::pythonload(itsPyPath);

  // Call python module's init() function if implemented:
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "init")) PythonWrapper::pyinst().attr("init")();
}

// ####################################################################################################
void jevois::PythonModule::postUninit()
{
  // Call python module's uninit() function if implemented:
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "uninit")) PythonWrapper::pyinst().attr("uninit")();
}

// ####################################################################################################
jevois::PythonModule::~PythonModule()
{ }

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe, OutputFrame && outframe)
{
  jevois::InputFramePython inframepy(&inframe);
  jevois::OutputFramePython outframepy(&outframe);
  PythonWrapper::pyinst().attr("process")(boost::ref(inframepy), boost::ref(outframepy));
}

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe)
{
  jevois::InputFramePython inframepy(&inframe);
  PythonWrapper::pyinst().attr("processNoUSB")(boost::ref(inframepy));
}

#ifdef JEVOIS_PRO

// ####################################################################################################
void jevois::PythonModule::process(InputFrame && inframe, GUIhelper & helper)
{
  jevois::InputFramePython inframepy(&inframe);
  jevois::GUIhelperPython helperpy(&helper);
  PythonWrapper::pyinst().attr("processGUI")(boost::ref(inframepy), boost::ref(helperpy));
}

#endif

// ####################################################################################################
void jevois::PythonModule::parseSerial(std::string const & str, std::shared_ptr<UserInterface> s)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "parseSerial"))
  {
    boost::python::object ret = PythonWrapper::pyinst().attr("parseSerial")(str);
    std::string retstr = boost::python::extract<std::string>(ret);
    if (retstr.empty() == false) s->writeString(retstr);
  }
  else jevois::Module::parseSerial(str, s);
}

// ####################################################################################################
void jevois::PythonModule::supportedCommands(std::ostream & os)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "supportedCommands"))
  {
    boost::python::object ret = PythonWrapper::pyinst().attr("supportedCommands")();
    std::string retstr = boost::python::extract<std::string>(ret);
    if (retstr.empty() == false) os << retstr;
  } else jevois::Module::supportedCommands(os);
}

// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::dnn::PreProcessorForPython::PreProcessorForPython(PreProcessor * pp) : itsPP(pp)
{ }

boost::python::tuple jevois::dnn::PreProcessorForPython::imagesize() const
{
  cv::Size const s = itsPP->imagesize();
  return boost::python::make_tuple(s.width, s.height);
}

boost::python::list jevois::dnn::PreProcessorForPython::blobs() const
{
  return jevois::python::pyVecToList(itsPP->blobs());
}

boost::python::tuple jevois::dnn::PreProcessorForPython::blobsize(size_t num) const
{
  cv::Size s = itsPP->blobsize(num);
  return boost::python::make_tuple(s.width, s.height);
}
        
boost::python::tuple jevois::dnn::PreProcessorForPython::b2i(float x, float y, size_t blobnum)
{
  float px = x, py = y;
  itsPP->b2i(px, py, blobnum);
  return boost::python::make_tuple(px, py);
}

boost::python::tuple jevois::dnn::PreProcessorForPython::getUnscaledCropRect(size_t blobnum)
{
  cv::Rect const r = itsPP->getUnscaledCropRect(blobnum);
  return boost::python::make_tuple(r.x, r.y, r.width, r.height);
}

boost::python::tuple jevois::dnn::PreProcessorForPython::i2b(float x, float y, size_t blobnum)
{
  float px = x, py = y;
  itsPP->i2b(px, py, blobnum);
  return boost::python::make_tuple(px, py);
}


// ####################################################################################################
// ####################################################################################################
// ####################################################################################################
jevois::dnn::PostProcessorDetectYOLOforPython::PostProcessorDetectYOLOforPython()
{
  // We need to add our sub under an existing "pypost" sub of our module. The hierarchy typically is:
  // DNN->pipeline->postproc->pypost
  std::shared_ptr<Module> m = jevois::python::engine()->module();
  if (!m) LFATAL("Cannot instantiate without a current running module");
  std::shared_ptr<Component> pi = m->getSubComponent("pipeline");
  if (!pi) LFATAL("Cannot instantiate without a current DNN pipeline");
  std::shared_ptr<Component> pp = pi->getSubComponent("postproc");
  if (!pp) LFATAL("Cannot instantiate without a current python-type post-processor");
  std::shared_ptr<Component> ppp = pp->getSubComponent("pypost");
  if (!ppp) LFATAL("Cannot instantiate without a current pypost post-processor");
  itsYOLO = ppp->addSubComponent<PostProcessorDetectYOLO>("yolo");
}

jevois::dnn::PostProcessorDetectYOLOforPython::~PostProcessorDetectYOLOforPython()
{ }

void jevois::dnn::PostProcessorDetectYOLOforPython::freeze(bool doit)
{ itsYOLO->freeze(doit); }

boost::python::tuple
jevois::dnn::PostProcessorDetectYOLOforPython::yolo(boost::python::list outs, int nclass,
                                                    float boxThreshold, float confThreshold,
                                                    int bw, int bh, int fudge, int maxbox)
{
  std::vector<cv::Mat> const outvec = jevois::python::pyListToVec<cv::Mat>(outs);

  std::vector<int> classIds;
  std::vector<float> confidences;
  std::vector<cv::Rect> boxes;
  
  itsYOLO->yolo(outvec, classIds, confidences, boxes, nclass, boxThreshold, confThreshold,
                cv::Size(bw, bh), fudge, maxbox);

  boost::python::list ids = jevois::python::pyVecToList(classIds);
  boost::python::list conf = jevois::python::pyVecToList(confidences);
  boost::python::list b;
  for (cv::Rect const & r : boxes) b.append(boost::python::make_tuple(r.x, r.y, r.width, r.height));

  return boost::python::make_tuple(ids, conf, b);
}
   
