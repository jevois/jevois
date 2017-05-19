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

namespace
{
  unsigned int parseFormat(std::string const & str)
  {
    if (str == "BAYER") return V4L2_PIX_FMT_SRGGB8;
    else if (str == "YUYV") return V4L2_PIX_FMT_YUYV;
    else if (str == "GREY" || str == "GRAY") return V4L2_PIX_FMT_GREY;
    else if (str == "MJPG") return V4L2_PIX_FMT_MJPEG;
    else if (str == "RGB565") return V4L2_PIX_FMT_RGB565;
    else if (str == "BGR24") return V4L2_PIX_FMT_BGR24;
    else if (str == "NONE") return 0;
    else throw std::runtime_error("Invalid pixel format " + str);
  }
}

// ####################################################################################################
std::string jevois::VideoMapping::sopath() const
{
  return JEVOIS_MODULE_PATH "/" + vendor + '/' + modulename + '/' + modulename + ".so";
}

// ####################################################################################################
unsigned int jevois::VideoMapping::osize() const
{ return jevois::v4l2ImageSize(ofmt, ow, oh); }

// ####################################################################################################
unsigned int jevois::VideoMapping::csize() const
{ return jevois::v4l2ImageSize(cfmt, cw, ch); }

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
std::string jevois::VideoMapping::str() const
{
  std::ostringstream ss;

  ss << "OUT: " << this->ostr() << " CAM: " << this->cstr();
  //ss << " (uvc " << uvcformat << '/' << uvcframe << '/' << jevois::VideoMapping::fpsToUvc(ofps) << ')';
  ss << " MOD: " << this->vendor << ':' << this->modulename;
  //ss << ' ' << this->sopath();
  return ss.str();
}

// ####################################################################################################
std::ostream & jevois::operator<<(std::ostream & out, jevois::VideoMapping const & m)
{
  out << jevois::fccstr(m.ofmt) << ' ' << m.ow << ' ' << m.oh << ' ' << m.ofps << ' '
      << jevois::fccstr(m.cfmt) << ' ' << m.cw << ' ' << m.ch << ' ' << m.cfps << ' '
      << m.vendor << ' ' << m.modulename;
  return out;
}

// ####################################################################################################
std::istream & jevois::operator>>(std::istream & in, jevois::VideoMapping & m)
{
  std::string of, cf;
  in >> of >> m.ow >> m.oh >> m.ofps >> cf >> m.cw >> m.ch >> m.cfps >> m.vendor >> m.modulename;

  m.ofmt = parseFormat(of);
  m.cfmt = parseFormat(cf);

  return in;
}

// ####################################################################################################
std::vector<jevois::VideoMapping> jevois::loadVideoMappings(size_t & defidx)
{
  std::ifstream ifs(JEVOIS_ENGINE_CONFIG_FILE);
  if (ifs.is_open() == false) LFATAL("Could not open [" << JEVOIS_ENGINE_CONFIG_FILE << ']');
  return jevois::videoMappingsFromStream(ifs, defidx);
}

// ####################################################################################################
std::vector<jevois::VideoMapping> jevois::videoMappingsFromStream(std::istream & is, size_t & defidx)
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
      m.ofmt = parseFormat(tok[0]);
      m.ow = std::stoi(tok[1]);
      m.oh = std::stoi(tok[2]);
      m.ofps = std::stof(tok[3]);

      m.cfmt = parseFormat(tok[4]);
      m.cw = std::stoi(tok[5]);
      m.ch = std::stoi(tok[6]);
      m.cfps = std::stof(tok[7]);
    }
    catch (std::exception const & e) { PERROR("Skipping entry because of parsing error: " << e.what()); }
    catch (...) { PERROR("Skipping entry because of parsing errors"); }
    
    m.vendor = tok[8];
    m.modulename = tok[9];

    std::string const sopath = m.sopath();
    std::ifstream testifs(sopath);
    if (testifs.is_open() == false) { PERROR("Could not open module " << sopath << " -- SKIPPING"); continue; }

    // Handle optional star for default mapping. We tolerate several and pick the first one:
    if (tok.size() > 10)
    {
      if (tok[10] == "*")
      {
        if (defmapping.cfmt == 0) defmapping = m;
        if (tok.size() > 11 && tok[11][0] != '#') PERROR("Extra garbage after 11th token ignored");
      }
      else if (tok[10][0] != '#') PERROR("Extra garbage after 10th token ignored");
    }

    mappings.push_back(m);
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
                      // The two output modes are identical. Warn unless the output format is NONE. We will adjust the
                      // framerates later to distinguish the offenders:
                      if (a.ofmt != 0)
                        PERROR("WARNING: Two modes have identical output format: " <<
                               jevois::fccstr(a.ofmt) << ' ' << a.ow << 'x' << a.oh << " @ " << a.ofps << "fps");

                      // All right, all USB stuff being equal, just sort according to the camera format:
                      if (a.cfmt < b.cfmt) return true;
                      if (a.cfmt == b.cfmt) {
                        if (a.cw > b.cw) return true;
                        if (a.cw == b.cw) {
                          if (a.ch > b.ch) return true;
                          if (a.ch == b.ch) {
                            if (a.cfps > b.cfps) return true;
                            // it's ok to have duplicates here since either those are NONE USB modes that are selected
                            // manually, or we will adjust below
                          }
                        }
                      }
                    }
                  }
                }
              }
              return false;
            });

  // If we had duplicate output formats, adjust framerates slightly: In the sorting above, we ordered by decreasing ofps
  // (all else being equal). Here we are going to decrease ofps on the second mapping when we hit a match. We need to
  // beware that this should propagate down to subsequent matching mappings while preserving the ordering:
  for (size_t i = 1; i < mappings.size(); ++i)
  {
    jevois::VideoMapping const & a = mappings[i-1];
    jevois::VideoMapping & b = mappings[i];
    
    if (b.ofmt != 0 && a.ofmt == b.ofmt && a.ow == b.ow && a.oh == b.oh)
    {
      if (std::abs(a.ofps - b.ofps) < 0.01F) b.ofps -= 1.0F; // equal fps, decrease b.ofps by 1fps
      else if (b.ofps > a.ofps) b.ofps = a.ofps - 1.0F; // got out of order because of a previous decrease
    }
  }
  
  // We need at least one mapping to work, and we need at least one with UVC output too keep hosts happy:
  if (mappings.empty() || mappings.back().ofmt == 0)
  {
    PERROR("No valid video mapping with UVC output found -- INSERTING A DEFAULT ONE");
    jevois::VideoMapping m;
    m.ofmt = V4L2_PIX_FMT_YUYV; m.ow = 640; m.oh = 480; m.ofps = 30.0F;
    m.cfmt = V4L2_PIX_FMT_YUYV; m.cw = 640; m.ch = 480; m.cfps = 30.0F;
    m.vendor = "JeVois"; m.modulename = "PassThrough";

    // We are guaranteed that this will not create a duplicate output mapping:
    mappings.push_back(m);
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
    for (size_t i = 0; i < mappings.size(); ++i)
      if (mappings[i].ofmt == defmapping.ofmt && mappings[i].ow == defmapping.ow &&
          mappings[i].oh == defmapping.oh && mappings[i].ofps == defmapping.ofps)
      { defidx = i; break; }
  }
  
  // Now that everything is sorted, compute our UVC format and frame indices, those are 1-based, and frame is reset each
  // time format changes. Note that all the intervals will be passed as a list to the USB host for a given format and
  // frame combination, so all the mappings that have identical pixel format and frame size here receive the same
  // uvcformat and uvcframe numbers. Note that we skip over all the NONE ofmt modes here:
  unsigned int ofmt = ~0U, ow = ~0U, oh = ~0U, iformat = 0, iframe = 0;
  for (jevois::VideoMapping & m : mappings)
  {
    if (m.ofmt == 0) { m.uvcformat = 0; m.uvcframe = 0; LDEBUG(m.str()); continue; }
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

