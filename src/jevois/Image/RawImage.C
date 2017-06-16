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

#include <jevois/Image/RawImage.H>
#include <jevois/Util/Utils.H>

// ####################################################################################################
jevois::RawImage::RawImage()
{ }

// ####################################################################################################
jevois::RawImage::RawImage(unsigned int w, unsigned int h, unsigned int f, float fs,
                           std::shared_ptr<VideoBuf> b, size_t bindex) :
    width(w), height(h), fmt(f), fps(fs), buf(b), bufindex(bindex)
{ }

// ####################################################################################################
unsigned int jevois::RawImage::bytesperpix() const
{ return jevois::v4l2BytesPerPix(fmt); }

// ####################################################################################################
unsigned int jevois::RawImage::bytesize() const
{ return width * height * jevois::v4l2BytesPerPix(fmt); }

// ####################################################################################################
void jevois::RawImage::invalidate()
{ buf.reset(); width = 0; height = 0; fmt = 0; fps = 0.0F; }

// ####################################################################################################
bool jevois::RawImage::valid() const
{ return (buf.get() != nullptr); }

// ####################################################################################################
void jevois::RawImage::require(char const * info, unsigned int w, unsigned int h, unsigned int f) const
{
  if (w != width || h != height || f != fmt)
    LFATAL("Incorrect format for RawImage " << info << ": want " << w << 'x' << h << ' ' << jevois::fccstr(f)
           << " but image is " << width << 'x' << height << ' ' << jevois::fccstr(fmt));
}

// ####################################################################################################
bool jevois::RawImage::coordsOk(int x, int y) const
{
  if (x >= 0 && x < int(width) && y >= 0 && y < int(height)) return true;
  else return false;
}
