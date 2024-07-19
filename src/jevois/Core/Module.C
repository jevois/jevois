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

#include <jevois/Core/Module.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/UserInterface.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Util/Coordinates.H>

#include <opencv2/imgproc/imgproc.hpp>

#include <cmath>
#include <sstream>
#include <iomanip>
 
// ####################################################################################################
jevois::Module::Module(std::string const & instance) :
    jevois::Component(instance)
{ }

// ####################################################################################################
jevois::Module::~Module()
{ }

// ####################################################################################################
void jevois::Module::process(InputFrame &&, OutputFrame &&)
{ LFATAL("Not implemented in this module"); }

// ####################################################################################################
void jevois::Module::process(InputFrame &&)
{ LFATAL("Not implemented in this module"); }

#ifdef JEVOIS_PRO
// ####################################################################################################
void jevois::Module::process(InputFrame &&, GUIhelper &)
{ LFATAL("Not implemented in this module, and only available on JeVois-Pro"); }
#endif

// ####################################################################################################
void jevois::Module::sendSerial(std::string const & str)
{
  jevois::Engine * e = dynamic_cast<jevois::Engine *>(itsParent);
  if (e == nullptr) LFATAL("My parent is not Engine -- CANNOT SEND SERIAL");

  e->sendSerial(str);
}

// ####################################################################################################
void jevois::Module::parseSerial(std::string const & str, std::shared_ptr<jevois::UserInterface>)
{ throw std::runtime_error("Unsupported command [" + str + ']'); }

// ####################################################################################################
void jevois::Module::supportedCommands(std::ostream & os)
{ os << "None" << std::endl; }

// ####################################################################################################
// ####################################################################################################
jevois::StdModule::StdModule(std::string const & instance) :
    jevois::Module(instance)
{ }

// ####################################################################################################
jevois::StdModule::~StdModule()
{ }

// ####################################################################################################
std::string jevois::StdModule::getStamp() const
{
  std::string ret;
  
  switch(serstamp::get())
  {
  case jevois::modul::SerStamp::None:
    break;
    
  case jevois::modul::SerStamp::Frame:
    ret = std::to_string(frameNum());
    break;
    
  case jevois::modul::SerStamp::Time:
  {
    std::time_t t = std::time(nullptr); char str[100];
    std::strftime(str, sizeof(str), "%T", std::localtime(&t));
    ret = std::string(str);
  }
  break;
  
  case jevois::modul::SerStamp::FrameTime:
  {
    std::time_t t = std::time(nullptr); char str[100];
    std::strftime(str, sizeof(str), "%T", std::localtime(&t));
    ret = std::to_string(frameNum()) + '/' + std::string(str);
  }
  break;
  
  case jevois::modul::SerStamp::FrameDateTime:
  {
    std::time_t t = std::time(nullptr); char str[100];
    std::strftime(str, sizeof(str), "%F/%T", std::localtime(&t));
    ret = std::to_string(frameNum()) + '/' + std::string(str);
  }
  break;
  }

  if (ret.empty() == false) ret += ' ';
  return ret;
}
  
// ####################################################################################################
void jevois::StdModule::sendSerialImg1Dx(unsigned int camw, float x, float size, std::string const & id,
                                      std::string const & extra)
{
  // Normalize the coordinate and size using the given precision to do rounding:
  float const eps = std::pow(10.0F, -float(serprec::get()));
  
  jevois::coords::imgToStdX(x, camw, eps);
  float dummy = 0.0F; jevois::coords::imgToStdSize(size, dummy, camw, 100, eps);
  
  // Delegate:
  sendSerialStd1Dx(x, size, id, extra);
}

// ####################################################################################################
void jevois::StdModule::sendSerialStd1Dx(float x, float size, std::string const & id, std::string const & extra)
{
  // Build the message depending on desired style:
  std::ostringstream oss; oss << std::fixed << std::setprecision(serprec::get());

  // Prepend frame/date/time as possibly requested by parameter serstamp:
  oss << getStamp();

  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
    oss << "T1 " << x;
    break;
    
  case jevois::modul::SerStyle::Normal:
    oss << "N1 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << x << ' ' << size;
    break;
    
  case jevois::modul::SerStyle::Detail:
  case jevois::modul::SerStyle::Fine:
    oss << "D1 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << x - 0.5F * size << ' ' << x + 0.5F * size;
    if (extra.empty() == false) oss << ' ' << extra;
    break;
  }
  
  // Send the message:
  sendSerial(oss.str());
}

// ####################################################################################################
void jevois::StdModule::sendSerialImg1Dy(unsigned int camh, float y, float size, std::string const & id,
                                         std::string const & extra)
{
  // Normalize the coordinate and size using the given precision to do rounding:
  float const eps = std::pow(10.0F, -float(serprec::get()));
  jevois::coords::imgToStdY(y, camh, eps);
  float dummy = 0.0F; jevois::coords::imgToStdSize(dummy, size, 100, camh, eps);
  
  // Delegate:
  sendSerialStd1Dy(y, size, id, extra);
}

// ####################################################################################################
void jevois::StdModule::sendSerialStd1Dy(float y, float size, std::string const & id, std::string const & extra)
{
  // Build the message depending on desired style:
  std::ostringstream oss; oss << std::fixed << std::setprecision(serprec::get());
  
  // Prepend frame/date/time as possibly requested by parameter serstamp:
  oss << getStamp();

  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
    oss << "T1 " << y;
    break;
    
  case jevois::modul::SerStyle::Normal:
    oss << "N1 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << y << ' ' << size;
    break;
    
  case jevois::modul::SerStyle::Detail:
  case jevois::modul::SerStyle::Fine:
    oss << "D1 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << y - 0.5F * size << ' ' << y + 0.5F * size;
    if (extra.empty() == false) oss << ' ' << extra;
    break;
  }
  
  // Send the message:
  sendSerial(oss.str());
}

// ####################################################################################################
void jevois::StdModule::sendSerialImg2D(unsigned int camw, unsigned int camh, float x, float y, float w, float h,
                                        std::string const & id, std::string const & extra)
{
  // Normalize the coordinates and sizes using the given precision to do rounding:
  float const eps = std::pow(10.0F, -float(serprec::get()));

  jevois::coords::imgToStd(x, y, camw, camh, eps);
  jevois::coords::imgToStdSize(w, h, camw, camh, eps);

  // Delegate:
  sendSerialStd2D(x, y, w, h, id, extra);
}
// ####################################################################################################
void jevois::StdModule::sendSerialStd2D(float x, float y, float w, float h, std::string const & id,
                                        std::string const & extra)
{
  // Build the message depending on desired style:
  std::ostringstream oss; oss << std::fixed << std::setprecision(serprec::get());

  // Prepend frame/date/time as possibly requested by parameter serstamp:
  oss << getStamp();

  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
    oss << "T2 " << x << ' ' << y;
    break;
    
  case jevois::modul::SerStyle::Normal:
    oss << "N2 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << x << ' ' << y << ' ' << w << ' ' << h;
    break;
    
  case jevois::modul::SerStyle::Detail:
    oss << "D2 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << x - 0.5F * w << ' ' << y - 0.5F * h << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h << ' ';
    oss << x + 0.5F * w << ' ' << y + 0.5F * h << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h;
    if (extra.empty() == false) oss << ' ' << extra;
    break;

  case jevois::modul::SerStyle::Fine:
    oss << "F2 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << 4 << ' '; // number of vertices
    oss << x - 0.5F * w << ' ' << y - 0.5F * h << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h << ' ';
    oss << x + 0.5F * w << ' ' << y + 0.5F * h << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h;
    if (extra.empty() == false) oss << ' ' << extra;
    break;
  }
  
  // Send the message:
  sendSerial(oss.str());
}

// ####################################################################################################
template <typename T>
void jevois::StdModule::sendSerialContour2D(unsigned int camw, unsigned int camh, std::vector<cv::Point_<T> > points,
                                            std::string const & id, std::string const & extra)
{
  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
  {
    // Compute center of gravity:
    float cx = 0.0F, cy = 0.0F;
    for (cv::Point2f const p : points) { cx += p.x; cy += p.y; }
    if (points.size()) { cx /= points.size(); cy /= points.size(); }
    sendSerialImg2D(camw, camh, cx, cy, 0.0F, 0.0F, id, extra);
  }
  break;
    
  case jevois::modul::SerStyle::Normal:
  {
    // Compute upright bounding rectangle:
    cv::Rect r = cv::boundingRect(points);
    sendSerialImg2D(camw, camh, r.x + 0.5F * r.width, r.y + 0.5F * r.height, r.width, r.height, id, extra);
  }
  break;
    
  case jevois::modul::SerStyle::Detail:
  {
    // Compute minimal rotated rectangle enclosing the points:
    cv::RotatedRect r = cv::minAreaRect(points);

    // Build the message:
    unsigned int const prec = serprec::get(); float const eps = std::pow(10.0F, -float(prec));
    std::ostringstream oss; oss << std::fixed << std::setprecision(prec);

    // Prepend frame/date/time as possibly requested by parameter serstamp:
    oss << getStamp();

    // Now the rest of the message:
    oss << "D2 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    cv::Point2f corners[4];
    r.points(corners);
    
    oss << 4; // number of vertices

    for (int i = 0; i < 4; ++i)
    {
      float x = corners[i].x, y = corners[i].y;
      jevois::coords::imgToStd(x, y, camw, camh, eps);
      oss << ' ' << x << ' ' << y;
    }
    if (extra.empty() == false) oss << ' ' << extra;

    // Send the message:
    sendSerial(oss.str());
  }
  break;

  case jevois::modul::SerStyle::Fine:
  {
    // Build the message:
    unsigned int const prec = serprec::get(); float const eps = std::pow(10.0F, -float(prec));
    std::ostringstream oss; oss << std::fixed << std::setprecision(prec);

    // Prepend frame/date/time as possibly requested by parameter serstamp:
    oss << getStamp();

    // Now the rest of the message:
    oss << "F2 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << points.size(); // number of vertices

    for (cv::Point2f const p : points)
    {
      float x = p.x, y = p.y;
      jevois::coords::imgToStd(x, y, camw, camh, eps);
      oss << ' ' << x << ' ' << y;
    }
    if (extra.empty() == false) oss << ' ' << extra;

    // Send the message:
    sendSerial(oss.str());
  }
  break;
  }
}

// Compile in explicit template instantiations:
namespace jevois
{
  template
  void StdModule::sendSerialContour2D(unsigned int camw, unsigned int camh, std::vector<cv::Point_<int> > points,
                                      std::string const & id, std::string const & extra);
  template
  void StdModule::sendSerialContour2D(unsigned int camw, unsigned int camh, std::vector<cv::Point_<float> > points,
                                      std::string const & id, std::string const & extra);
  template
  void StdModule::sendSerialContour2D(unsigned int camw, unsigned int camh, std::vector<cv::Point_<double> > points,
                                      std::string const & id, std::string const & extra);
}

// ####################################################################################################
void jevois::StdModule::sendSerialStd3D(float x, float y, float z, float w, float h, float d,
                                        float q1, float q2, float q3, float q4,
                                        std::string const & id, std::string const & extra)
{
  // Build the message depending on desired style:
  std::ostringstream oss; oss << std::fixed << std::setprecision(serprec::get());

  // Prepend frame/date/time as possibly requested by parameter serstamp:
  oss << getStamp();

  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
    oss << "T3 " << x << ' ' << y << ' ' << z;
    break;
    
  case jevois::modul::SerStyle::Normal:
    oss << "N3 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << x << ' ' << y << ' ' << z << ' ' << w << ' ' << h << ' ' << d;
    break;
    
  case jevois::modul::SerStyle::Detail:
    oss << "D3 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << x << ' ' << y << ' ' << z << ' ' << w << ' ' << h << ' ' << d << ' '
        << q1 << ' ' << q2 << ' ' << q3 << ' ' << q4;
    if (extra.empty() == false) oss << ' ' << extra;
    break;

  case jevois::modul::SerStyle::Fine:
    oss << "F3 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << 8 << ' '; // number of vertices
    oss << x - 0.5F * w << ' ' << y - 0.5F * h << ' ' << z - 0.5F * d << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h << ' ' << z - 0.5F * d << ' ';
    oss << x + 0.5F * w << ' ' << y + 0.5F * h << ' ' << z - 0.5F * d << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h << ' ' << z - 0.5F * d << ' ';
    oss << x - 0.5F * w << ' ' << y - 0.5F * h << ' ' << z + 0.5F * d << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h << ' ' << z + 0.5F * d << ' ';
    oss << x + 0.5F * w << ' ' << y + 0.5F * h << ' ' << z + 0.5F * d << ' ';
    oss << x + 0.5F * w << ' ' << y - 0.5F * h << ' ' << z + 0.5F * d << ' ';
    if (extra.empty() == false) oss << ' ' << extra;
    break;
  }
  
  // Send the message:
  sendSerial(oss.str());
}

// ####################################################################################################
void jevois::StdModule::sendSerialStd3D(std::vector<cv::Point3f> points, std::string const & id,
                                        std::string const & extra)
{
  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
  {
    // Compute center of gravity:
    cv::Point3f cg(0.0F, 0.0F, 0.0F);
    for (cv::Point3f const & p : points) cg += p;
    if (points.size()) { cg.x /= points.size(); cg.y /= points.size(); cg.z /= points.size(); }
    sendSerialStd3D(cg.x, cg.y, cg.z, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, id, extra);
  }
  break;
    
  case jevois::modul::SerStyle::Normal:
  {
    // Compute upright bounding parallelepiped:
    cv::Point3f cg(0.0F, 0.0F, 0.0F), pmin(1e30F, 1e30F, 1e30F), pmax(-1e30F, -1e30F, -1e30F);
    for (cv::Point3f const & p : points)
    {
      cg += p;
      if (p.x < pmin.x) pmin.x = p.x;
      if (p.y < pmin.y) pmin.y = p.y;
      if (p.z < pmin.z) pmin.z = p.z;
      if (p.x > pmax.x) pmax.x = p.x;
      if (p.y > pmax.y) pmax.y = p.y;
      if (p.z > pmax.z) pmax.z = p.z;
    }
    if (points.size()) { cg.x /= points.size(); cg.y /= points.size(); cg.z /= points.size(); }
    sendSerialStd3D(cg.x, cg.y, cg.z, pmax.x - pmin.x, pmax.y - pmin.y, pmax.z - pmin.z, 0.0F, 0.0F, 0.0F, 0.0F,
                    id, extra);
  }
  break;
  
  case jevois::modul::SerStyle::Detail:
  {
    // Compute upright bounding parallelepiped:
    cv::Point3f cg(0.0F, 0.0F, 0.0F), pmin(1e30F, 1e30F, 1e30F), pmax(-1e30F, -1e30F, -1e30F);
    for (cv::Point3f const & p : points)
    {
      cg += p;
      if (p.x < pmin.x) pmin.x = p.x;
      if (p.y < pmin.y) pmin.y = p.y;
      if (p.z < pmin.z) pmin.z = p.z;
      if (p.x > pmax.x) pmax.x = p.x;
      if (p.y > pmax.y) pmax.y = p.y;
      if (p.z > pmax.z) pmax.z = p.z;
    }
    if (points.size()) { cg.x /= points.size(); cg.y /= points.size(); cg.z /= points.size(); }
    // FIXME what should we send for the quaternion?
    sendSerialStd3D(cg.x, cg.y, cg.z, pmax.x - pmin.x, pmax.y - pmin.y, pmax.z - pmin.z, 0.0F, 0.0F, 0.0F, 1.0F,
                    id, extra);
  }
  break;
  
  case jevois::modul::SerStyle::Fine:
  {
    // Build the message:
    unsigned int const prec = serprec::get();
    std::ostringstream oss; oss << std::fixed << std::setprecision(prec);

    // Prepend frame/date/time as possibly requested by parameter serstamp:
    oss << getStamp();

    // Now the rest of the message:
    oss << "F3 ";
    if (id.empty()) oss << "unknown "; else oss << jevois::replaceWhitespace(id) << ' ';
    oss << points.size(); // number of vertices
    
    for (cv::Point3f const & p : points) oss << ' ' << p.x << ' ' << p.y << ' ' << p.z;
    if (extra.empty() == false) oss << ' ' << extra;
    
    // Send the message:
    sendSerial(oss.str());
  }
  break;
  }
}

// ####################################################################################################
void jevois::StdModule::sendSerialMarkStart()
{
  jevois::modul::SerMark const m = sermark::get();
  if (m == jevois::modul::SerMark::None || m == jevois::modul::SerMark::Stop) return;
  sendSerial(getStamp() + "MARK START");
}

// ####################################################################################################
void jevois::StdModule::sendSerialMarkStop()
{
  jevois::modul::SerMark const m = sermark::get();
  if (m == jevois::modul::SerMark::None || m == jevois::modul::SerMark::Start) return;
  sendSerial(getStamp() + "MARK STOP");
}

// ####################################################################################################
void jevois::StdModule::sendSerialObjReco(std::vector<jevois::ObjReco> const & res)
{
  if (res.empty()) return;

  // Build the message depending on desired style:
  std::ostringstream oss; oss << std::fixed << std::setprecision(serprec::get());

  // Prepend frame/date/time as possibly requested by parameter serstamp:
  oss << getStamp();

  // Format the message depending on parameter serstyle:
  switch (serstyle::get())
  {
  case jevois::modul::SerStyle::Terse:
    oss << "TO " << jevois::replaceWhitespace(res[0].category);
    break;
    
  case jevois::modul::SerStyle::Normal:
    oss << "NO " << jevois::replaceWhitespace(res[0].category) << ':' << res[0].score;
    break;
    
  case jevois::modul::SerStyle::Detail:
    oss << "DO";
    for (auto const & r : res) oss << ' ' << jevois::replaceWhitespace(r.category) << ':' << r.score;
    break;

  case jevois::modul::SerStyle::Fine:
    oss << "FO";
    for (auto const & r : res) oss << ' ' << jevois::replaceWhitespace(r.category) << ':' << r.score;
    break;
  }
  
  // Send the message:
  sendSerial(oss.str());
}

// ####################################################################################################
void jevois::StdModule::sendSerialObjDetImg2D(unsigned int camw, unsigned int camh, float x, float y, float w, float h,
                                              std::vector<ObjReco> const & res)
{
  if (res.empty()) return;

  std::string best, extra; std::string * ptr = &best;
  std::string fmt = "%s:%." + std::to_string(serprec::get()) + "f";
  
  for (auto const & r : res)
  {
    switch (serstyle::get())
    {
    case jevois::modul::SerStyle::Terse:
      (*ptr) += jevois::replaceWhitespace(r.category);
      break;
      
    default:
      (*ptr) += jevois::sformat(fmt.c_str(), jevois::replaceWhitespace(r.category).c_str(), r.score);
    }
    if (ptr == &extra) (*ptr) += ' ';
    ptr = &extra;
  }

  // Remove last space:
  if (extra.empty() == false) extra = extra.substr(0, extra.length() - 1);
  
  sendSerialImg2D(camw, camh, x, y, w, h, best, extra);
}

// ####################################################################################################
void jevois::StdModule::sendSerialObjDetImg2D(unsigned int camw, unsigned int camh, jevois::ObjDetect const & det)
{
  sendSerialObjDetImg2D(camw, camh, det.tlx, det.tly, det.brx - det.tlx, det.bry - det.tly, det.reco);
}
