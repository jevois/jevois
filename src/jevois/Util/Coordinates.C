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

#include <jevois/Util/Coordinates.H>
#include <cmath>

// ####################################################################################################
void jevois::coords::imgToStd(float & x, float & y, jevois::RawImage const & camimg, float const eps)
{ jevois::coords::imgToStd(x, y, camimg.width, camimg.height, eps); }

// ####################################################################################################
void jevois::coords::imgToStd(float & x, float & y, unsigned int const width, unsigned int const height,
                              float const eps)
{
  x = 2000.0F * x / width - 1000.0F;
  y = (1.0F / JEVOIS_CAMERA_ASPECT) * (2000.0F * y / height - 1000.0F);

  if (eps) { x = std::round(x / eps) * eps; y = std::round(y / eps) * eps; }
}

// ####################################################################################################
void jevois::coords::stdToImg(float & x, float & y, jevois::RawImage const & camimg, float const eps)
{ jevois::coords::stdToImg(x, y, camimg.width, camimg.height, eps); }

// ####################################################################################################
void jevois::coords::stdToImg(float & x, float & y, unsigned int const width, unsigned int const height,
                              float const eps)
{
  x = (x * 0.0005F + 0.5F) * width;
  y = (JEVOIS_CAMERA_ASPECT * y * 0.0005F + 0.5F) * height;

  if (eps) { x = std::round(x / eps) * eps; y = std::round(y / eps) * eps; }
}
