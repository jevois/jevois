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

#include <jevois/Core/VideoMapping.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

#define PERROR(x) LERROR("In file " << JEVOIS_ENGINE_CONFIG_FILE << ':' << linenum << ": " << x)

// ####################################################################################################
std::string jevois::VideoMapping::path() const
{
  return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename;
}

// ####################################################################################################
std::string jevois::VideoMapping::sopath(bool delete_old_versions) const
{
  if (ispython) return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename + '/' + modulename + ".py";

  // For C++ live installs on the camera, we may have several versions, use the latest:
  std::filesystem::path const dir = JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename;
  std::filesystem::path const stem = modulename + ".so";
  
  int ver = 0;
  for (auto const & entry : std::filesystem::directory_iterator(dir))
    if (entry.path().stem() == stem)
      try { ver = std::max(ver, std::stoi(entry.path().extension().string().substr(1))); } // ext without leading dot
      catch (...) { }

  if (ver)
  {
    std::filesystem::path const latest = (dir / stem).string() + '.' + std::to_string(ver);

    if (delete_old_versions)
      for (auto const & entry : std::filesystem::directory_iterator(dir))
        if (entry.path().stem() == stem && entry.path() != latest)
          std::filesystem::remove(entry.path());

    return latest.string();
  }

  return (dir / stem).string();
}

// ####################################################################################################
std::string jevois::VideoMapping::srcpath() const
{
  if (ispython) return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename + '/' + modulename + ".py";
  else return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename + '/' + modulename + ".C";
}

// ####################################################################################################
std::string jevois::VideoMapping::cmakepath() const
{
  return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename + "/CMakeLists.txt";
}

// ####################################################################################################
std::string jevois::VideoMapping::modinfopath() const
{
  return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename + "/modinfo.html";
}

// ####################################################################################################
unsigned int jevois::VideoMapping::osize() const
{ return jevois::v4l2ImageSize(ofmt, ow, oh); }

// ####################################################################################################
unsigned int jevois::VideoMapping::csize() const
{ return jevois::v4l2ImageSize(cfmt, cw, ch); }

// ####################################################################################################
unsigned int jevois::VideoMapping::c2size() const
{ return jevois::v4l2ImageSize(c2fmt, c2w, c2h); }

// ####################################################################################################
float jevois::VideoMapping::uvcToFps(unsigned int interval)
{
  // Let's round it off to the nearest 1/100Hz:
  return float(1000000000U / interval) * 0.01F;
}

// ####################################################################################################
unsigned int jevois::VideoMapping::fpsToUvc(float fps)
{
  return (unsigned int)(10000000.0F / fps + 0.499F);
}

// ####################################################################################################
float jevois::VideoMapping::v4l2ToFps(struct v4l2_fract const & interval)
{
  // Let's round it off to the nearest 1/100Hz:
  return float(interval.denominator * 100U / interval.numerator) * 0.01F;
}

// ####################################################################################################
struct v4l2_fract jevois::VideoMapping::fpsToV4l2(float fps)
{
  return { 100U, (unsigned int)(fps * 100.0F) };
}

// ####################################################################################################
std::string jevois::VideoMapping::ostr() const
{
  std::ostringstream ss;
  ss << jevois::fccstr(ofmt) << ' ' << ow << 'x' << oh << " @ " << ofps << "fps";
  return ss.str();
}

// ####################################################################################################
std::string jevois::VideoMapping::cstr() const
{
  std::ostringstream ss;
  ss << jevois::fccstr(cfmt) << ' ' << cw << 'x' << ch << " @ " << cfps << "fps";
  return ss.str();
}

// ####################################################################################################
std::string jevois::VideoMapping::c2str() const
{
  std::ostringstream ss;
  ss << jevois::fccstr(c2fmt) << ' ' << c2w << 'x' << c2h << " @ " << cfps << "fps";
  return ss.str();
}

// ####################################################################################################
std::string jevois::VideoMapping::cstrall() const
{
  std::string ret = cstr();
  if (crop == jevois::CropType::CropScale) ret += " + " + c2str();
  return ret;
}

// ####################################################################################################
std::string jevois::VideoMapping::str() const
{
  std::ostringstream ss;

  ss << "OUT: " << ostr() << " CAM: " << cstr();
  if (crop == jevois::CropType::CropScale) ss << " CAM2: " << c2str();
  //ss << " (uvc " << uvcformat << '/' << uvcframe << '/' << jevois::VideoMapping::fpsToUvc(ofps) << ')';
  ss << " MOD: " << vendor << ':' << modulename << ' ' << (ispython ? "Python" : "C++");
  //ss << ' ' << this->sopath();
  return ss.str();
}

// ####################################################################################################
std::string jevois::VideoMapping::menustr() const
{
  std::ostringstream ss;

  ss << modulename << (ispython ? " (Py)" : " (C++)");
  ss << " CAM: " << cstr();
  if (crop == jevois::CropType::CropScale) ss << " + " << c2str();
  if (ofmt != 0 && ofmt != JEVOISPRO_FMT_GUI) ss << ", OUT: " << ostr() << ' ';
  return ss.str();
}

// ####################################################################################################
std::string jevois::VideoMapping::menustr2() const
{
  std::ostringstream ss;

  ss << modulename << (ispython ? " (Py)" : " (C++)");
  ss << " CAM: " << cstr();
  if (crop == jevois::CropType::CropScale) ss << " + " << c2str();

  switch (ofmt)
  {
  case JEVOISPRO_FMT_GUI: ss << ", OUT: GUI"; break;
  case 0: ss << ", OUT: None (headless)"; break;
  default: ss << ", OUT: " << ostr() << ' ';
  }
  
  return ss.str();
}

// ####################################################################################################
bool jevois::VideoMapping::hasSameSpecsAs(VideoMapping const & other) const
{
  return (ofmt == other.ofmt && ow == other.ow && oh == other.oh && std::abs(ofps - other.ofps) < 0.01F &&
          cfmt == other.cfmt && cw == other.cw && ch == other.ch && std::abs(cfps - other.cfps) < 0.01F &&
          crop == other.crop &&
          (crop != jevois::CropType::CropScale ||
           (c2fmt == other.c2fmt && c2w == other.c2w && c2h == other.c2h && std::abs(cfps - other.cfps) < 0.01F)));
}

// ####################################################################################################
bool jevois::VideoMapping::isSameAs(VideoMapping const & other) const
{
  return (hasSameSpecsAs(other) && wdr == other.wdr && vendor == other.vendor && modulename == other.modulename &&
          ispython == other.ispython);
}

// ####################################################################################################
std::ostream & jevois::operator<<(std::ostream & out, jevois::VideoMapping const & m)
{
  out << jevois::fccstr(m.ofmt) << ' ' << m.ow << ' ' << m.oh << ' ' << m.ofps << ' ';

  if (m.wdr != jevois::WDRtype::Linear)
    out << m.wdr << ':';

  switch (m.crop)
  {
  case jevois::CropType::Scale:
    break;
  case jevois::CropType::Crop:
    out << m.crop << ':'; break;
  case jevois::CropType::CropScale:
    out << m.crop << '=' << jevois::fccstr(m.c2fmt) << '@' << m.c2w << 'x' << m.c2h << ':'; break;
  }
  
  out << jevois::fccstr(m.cfmt) << ' ' << m.cw << ' ' << m.ch << ' ' << m.cfps << ' '
      << m.vendor << ' ' << m.modulename;
  return out;
}

namespace
{
  // Return either the absolute value in str, or c +/- stoi(str)
  int parse_relative_dim(std::string const & str, int c)
  {
    if (str.empty()) throw std::range_error("Invalid empty output width");
    if (str[0] == '+') return c + std::stoi(str.substr(1));
    else if (str[0] == '-') return c - std::stoi(str.substr(1));
    return std::stoi(str);
  }
  
  void parse_cam_format(std::string const & str, unsigned int & fmt, jevois::WDRtype & wdr, jevois::CropType & crop,
                        unsigned int & c2fmt, unsigned int & c2w, unsigned int & c2h)
  {
    // Set the defaults in case no qualifier is given:
    wdr = jevois::WDRtype::Linear;
    crop = jevois::CropType::Scale;
    
    // Parse:
    auto tok = jevois::split(str, ":");
    if (tok.empty()) throw std::range_error("Empty camera format is not allowed");
    fmt = jevois::strfcc(tok.back()); tok.pop_back();
    for (std::string & t : tok)
    {
      // WDR, crop type can be specified in any order. So try to get each and see if we succeed:
      try { wdr = jevois::from_string<jevois::WDRtype>(t); continue; } catch (...) { }

      // If not WDR, then it should be Crop|Scale|CropScale=FCC@WxH
      auto ttok = jevois::split(t, "[=@x]");
      if (ttok.empty()) throw std::range_error("Invalid empty camera format modifier: " + t);

      try
      {
        crop = jevois::from_string<jevois::CropType>(ttok[0]);
        
        switch (crop)
        {
        case jevois::CropType::Crop: if (ttok.size() == 1) continue; break;
        case jevois::CropType::Scale: if (ttok.size() == 1) continue; break;
        case jevois::CropType::CropScale:
          if (ttok.size() == 4)
          {
            c2fmt = jevois::strfcc(ttok[1]);
            c2w = std::stoi(ttok[2]);
            c2h = std::stoi(ttok[3]);
            continue;
          }
        }
      } catch (...) { }
      
      throw std::range_error("Invalid camera format modifier [" + t +
                             "] - must be Linear|DOL or Crop|Scale|CropScale=FCC@WxH");
    }
  }
}

// ####################################################################################################
std::istream & jevois::operator>>(std::istream & in, jevois::VideoMapping & m)
{
  std::string of, cf, ows, ohs;
  in >> of >> ows >> ohs >> m.ofps >> cf >> m.cw >> m.ch >> m.cfps >> m.vendor >> m.modulename;

  // Output width and height can be either absolute or relative to camera width and height; for relative values, they
  // must start with either a + or - symbol:
  m.ow = parse_relative_dim(ows, m.cw);
  m.oh = parse_relative_dim(ohs, m.ch);
  
  m.ofmt = jevois::strfcc(of);

  // Parse any wdr, crop, or stream modulators on camera format, and the format itself:
  parse_cam_format(cf, m.cfmt, m.wdr, m.crop, m.c2fmt, m.c2w, m.c2h);

  m.setModuleType(); // set python vs C++, check that file is here, and throw otherwise
  
  return in;
}

// ####################################################################################################
std::vector<jevois::VideoMapping> jevois::loadVideoMappings(jevois::CameraSensor s, size_t & defidx, bool checkso,
                                                            bool hasgui)
{
  std::ifstream ifs(JEVOIS_ENGINE_CONFIG_FILE);
  if (ifs.is_open() == false) LFATAL("Could not open [" << JEVOIS_ENGINE_CONFIG_FILE << ']');
  return jevois::videoMappingsFromStream(s, ifs, defidx, checkso, hasgui);
}

// ####################################################################################################
std::vector<jevois::VideoMapping> jevois::videoMappingsFromStream(jevois::CameraSensor s, std::istream & is,
                                                                  size_t & defidx, bool checkso, bool hasgui)
{
  size_t linenum = 1;
  std::vector<jevois::VideoMapping> mappings;
  jevois::VideoMapping defmapping = { };
  
  for (std::string line; std::getline(is, line); ++linenum)
  {
    std::vector<std::string> tok = jevois::split(line);
    if (tok.empty()) continue; // skip blank lines
    if (tok.size() == 1 && tok[0].empty()) continue; // skip blank lines
    if (tok[0][0] == '#') continue; // skip comments
    if (tok.size() < 10) { PERROR("Found " << tok.size() << " tokens instead of >= 10 -- SKIPPING"); continue; }

    jevois::VideoMapping m;
    try
    {
      m.ofmt = jevois::strfcc(tok[0]);
      m.ofps = std::stof(tok[3]);

      parse_cam_format(tok[4], m.cfmt, m.wdr, m.crop, m.c2fmt, m.c2w, m.c2h);
      m.cw = std::stoi(tok[5]);
      m.ch = std::stoi(tok[6]);
      m.cfps = std::stof(tok[7]);

      m.ow = parse_relative_dim(tok[1], m.cw);
      m.oh = parse_relative_dim(tok[2], m.ch);
    }
    catch (std::exception const & e) { PERROR("Skipping entry because of parsing error: " << e.what()); continue; }
    catch (...) { PERROR("Skipping entry because of parsing errors"); continue; }
    
    m.vendor = tok[8];
    m.modulename = tok[9];

    // Determine C++ vs python, silently skip this module if none of those and checkso was given:
    try { m.setModuleType(); }
    catch (...)
    {
      if (checkso)
      { PERROR("No .so|.py found for " << m.vendor << '/' << m.modulename << " -- SKIPPING."); continue; }
    }

    // Skip if the sensor cannot support this mapping:
    if (jevois::sensorSupportsFormat(s, m) == false)
    { PERROR("Camera video format [" << m.cstr() << "] not supported by sensor -- SKIPPING."); continue; }

    // Skip gui modes if we do not have a gui:
    if (hasgui == false && m.ofmt == JEVOISPRO_FMT_GUI)
    { PERROR("Graphical user interface not available or disabled -- SKIPPING"); continue; }
    
#ifndef JEVOIS_PRO
    // Skip if not jevois-pro and trying to use GUI output:
    if (m.ofmt == JEVOISPRO_FMT_GUI)
    { PERROR("GUI output only supported on JeVois-Pro -- SKIPPING"); continue; }

#ifndef JEVOIS_PLATFORM
    // Skip if not jevois-pro platform and trying to do dual-frame hardware scaling through the ISP:
    if (m.crop == jevois::CropType::CropScale || m.crop == jevois::CropType::Crop)
    { PERROR("Crop or Crop+Scale camera input only supported on JeVois-Pro platform -- SKIPPING"); continue; }
#endif // JEVOIS_PLATFORM
#endif // JEVOIS_PRO
  
    // Handle optional star for default mapping. We tolerate several and pick the first one:
    if (tok.size() > 10)
    {
      if (tok[10] == "*")
      {
        if (defmapping.cfmt == 0) { defmapping = m; LINFO("Default in videomappings.cfg is " << m.str()); }
        if (tok.size() > 11 && tok[11][0] != '#') PERROR("Extra garbage after 11th token ignored");
      }
      else if (tok[10][0] != '#') PERROR("Extra garbage after 10th token ignored");
    }

    mappings.push_back(m);
    //LINFO("Successfully parsed mapping: " << m.str());
  }

  // Sort the array:
  std::sort(mappings.begin(), mappings.end(),
            [=](jevois::VideoMapping const & a, jevois::VideoMapping const & b)
            {
              // Return true if a should be ordered before b:
              if (a.ofmt < b.ofmt) return true;
              if (a.ofmt == b.ofmt) {
                if (a.ow > b.ow) return true;
                if (a.ow == b.ow) {
                  if (a.oh > b.oh) return true;
                  if (a.oh == b.oh) {
                    if (a.ofps > b.ofps) return true;
                    if (std::abs(a.ofps - b.ofps) < 0.01F) {
#ifndef JEVOIS_PRO
                      // JeVois-A33 only: The two output modes are identical. Warn unless the output format is NONE or
                      // JVUI. We will adjust the framerates later to distinguish the offenders:
                      if (a.ofmt != 0 && a.ofmt != JEVOISPRO_FMT_GUI)
                        PERROR("WARNING: Two modes have identical output format: " << a.ostr());
#endif
                      // All right, all USB stuff being equal, just sort according to the camera format:
                      if (a.cfmt < b.cfmt) return true;
                      if (a.cfmt == b.cfmt) {
                        if (a.cw > b.cw) return true;
                        if (a.cw == b.cw) {
                          if (a.ch > b.ch) return true;
                          if (a.ch == b.ch) {
                            if (a.cfps > b.cfps) return true;
                            // it's ok to have duplicates here since either those are NONE USB modes or JVUI that are
                            // selected manually, or we will adjust below
                          }
                        }
                      }
                    }
                  }
                }
              }
              return false;
            });

  // If we are not checking for .so, run the shortcut version where we also do not check for no USB mode, duplicates,
  // default index, etc. This is used, by, e.g., the jevois-add-videomapping program:
  if (checkso == false) { defidx = 0; return mappings; }
  
  // We need at least one mapping to work, and we need at least one with UVC output too keep hosts happy:
  if (mappings.empty() || mappings.back().ofmt == 0 || mappings.back().ofmt == JEVOISPRO_FMT_GUI)
  {
    PERROR("No valid video mapping with UVC output found -- INSERTING A DEFAULT ONE");
    jevois::VideoMapping m;
    m.ofmt = V4L2_PIX_FMT_YUYV; m.ow = 640; m.oh = 480; m.ofps = 30.0F;
    m.cfmt = V4L2_PIX_FMT_YUYV; m.cw = 640; m.ch = 480; m.cfps = 30.0F;
    m.vendor = "JeVois"; m.modulename = "PassThrough"; m.ispython = false;

    // We are guaranteed that this will not create a duplicate output mapping since either mappings was empty or it had
    // no USB-out modes:
    mappings.push_back(m);
  }

  // If we had duplicate output formats, discard full exact duplicates (including same module), and otherwise
  // (JeVois-A33 only) adjust framerates slightly: In the sorting above, we ordered by decreasing ofps (all else being
  // equal). Here we are going to decrease ofps on the second mapping when we hit a match. We need to beware that this
  // should propagate down to subsequent matching mappings while preserving the ordering:
  auto a = mappings.begin(), b = a + 1;
  while (b != mappings.end())
  {
    // Discard exact duplicates, adjust frame rates for matching specs but different modules:
    if (a->isSameAs(*b)) { b = mappings.erase(b); continue; }
#ifndef JEVOIS_PRO
    else if (b->ofmt != 0 && b->ofmt != JEVOISPRO_FMT_GUI && a->ofmt == b->ofmt && a->ow == b->ow && a->oh == b->oh)
    {
      if (std::abs(a->ofps - b->ofps) < 0.01F) b->ofps -= 1.0F; // equal fps, decrease b.ofps by 1fps
      else if (b->ofps > a->ofps) b->ofps = a->ofps - 1.0F; // got out of order because of a previous decrease
    }
#endif
    a = b; ++b;
  }
  
  // Find back our default mapping index in the sorted array:
  if (defmapping.cfmt == 0)
  {
    LERROR("No default video mapping provided, using first one with UVC output");
    for (size_t i = 0; i < mappings.size(); ++i) if (mappings[i].ofmt) { defidx = i; break; }
  }
  else
  {
    // Default was set, find its index after sorting:
    defidx = 0;
    for (size_t i = 0; i < mappings.size(); ++i) if (mappings[i].isSameAs(defmapping)) { defidx = i; break; }
  }
  
  // Now that everything is sorted, compute our UVC format and frame indices, those are 1-based, and frame is reset each
  // time format changes. Note that all the intervals will be passed as a list to the USB host for a given format and
  // frame combination, so all the mappings that have identical pixel format and frame size here receive the same
  // uvcformat and uvcframe numbers. Note that we skip over all the NONE ofmt modes here:
  unsigned int ofmt = ~0U, ow = ~0U, oh = ~0U, iformat = 0, iframe = 0;
  for (jevois::VideoMapping & m : mappings)
  {
    if (m.ofmt == 0 || m.ofmt == JEVOISPRO_FMT_GUI) { m.uvcformat = 0; m.uvcframe = 0; LDEBUG(m.str()); continue; }
    if (m.ofmt != ofmt) { ofmt = m.ofmt; ow = ~0U; oh = ~0U; ++iformat; iframe = 0; } // Switch to the next format
    if (m.ow != ow || m.oh != oh) { ow = m.ow; oh = m.oh; ++iframe; } // Switch to the next frame size
    m.uvcformat = iformat; m.uvcframe = iframe;
    LDEBUG(m.str());
  }
  
  return mappings;
}

// ####################################################################################################
bool jevois::VideoMapping::match(unsigned int oformat, unsigned int owidth, unsigned int oheight,
                                 float oframespersec) const
{
  if (ofmt == oformat && ow == owidth && oh == oheight && (std::abs(ofps - oframespersec) < 0.1F)) return true;
  return false;
}

// ####################################################################################################
void jevois::VideoMapping::setModuleType()
{
  // First assume that it is a C++ compiled module and check for the .so file:
  ispython = false;
  std::string sopa = sopath();
  std::ifstream testifs(sopa);
  if (testifs.is_open() == false)
  {
    // Could not find the .so, maybe it is a python module:
    ispython = true; sopa = sopath();
    std::ifstream testifs2(sopa);
    if (testifs2.is_open() == false) throw std::runtime_error("Could not open module file " + sopa + "|.so");
  }
}
