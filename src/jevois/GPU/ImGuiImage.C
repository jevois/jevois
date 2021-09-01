// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

#ifdef JEVOIS_PRO

#include <jevois/GPU/ImGuiImage.H>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <imgui.h>

// ##############################################################################################################
jevois::ImGuiImage::ImGuiImage()
{ }

// ##############################################################################################################
void jevois::ImGuiImage::load(cv::Mat const & img, bool isbgr)
{
  clear();

  if (img.depth() != CV_8U) LFATAL("Only 8-bit/channel images are supported");
  
  // OpenGL-ES does not support BGR textures, so we need to convert to RGB if image is BGR:
  cv::Mat cvt; GLint fmt;
  switch (img.channels())
  {
  case 1: cvt = img;
    fmt = GL_LUMINANCE;
    break;
    
  case 3:
    if (isbgr) cv::cvtColor(img, cvt, cv::COLOR_BGR2RGB); else cvt = img;
    fmt = GL_RGB;
    break;
    
  case 4: if (isbgr) cv::cvtColor(img, cvt, cv::COLOR_BGRA2RGBA); else cvt = img;
    fmt = GL_RGBA;
    break;

  default: LFATAL("Unsupported image format with " << img.channels() << " channels, should be 1, 3, or 4");
  }
  
  glGenTextures(1, &itsId);
  glBindTexture(GL_TEXTURE_2D, itsId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, fmt, cvt.cols, cvt.rows, 0, fmt, GL_UNSIGNED_BYTE, cvt.ptr());
  glBindTexture(GL_TEXTURE_2D, 0);
}

// ##############################################################################################################
void jevois::ImGuiImage::load(std::string const & fname)
{
  cv::Mat img = cv::imread(fname, cv::IMREAD_UNCHANGED);
  if (img.data == nullptr) LFATAL("Failed to load image [" << fname << ']');
  load(img, true);
}

// ##############################################################################################################
bool jevois::ImGuiImage::loaded() const
{ return (itsId != 0); }

// ##############################################################################################################
void jevois::ImGuiImage::clear()
{
  if (itsId) { glDeleteTextures(1, &itsId); itsId = 0; }
}

// ##############################################################################################################
jevois::ImGuiImage::~ImGuiImage()
{ clear(); }

// ##############################################################################################################
void jevois::ImGuiImage::draw(ImVec2 const & pos, ImVec2 const & size, ImDrawList * dl)
{
  if (dl == nullptr) dl = ImGui::GetWindowDrawList();
    
  dl->AddImage(reinterpret_cast<ImTextureID>(itsId), pos, ImVec2(pos.x + size.x, pos.y + size.y));
}

#endif // JEVOIS_PRO
