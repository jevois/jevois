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

#include <jevois/Core/VideoInput.H>

// ##############################################################################################################
jevois::VideoInput::VideoInput(std::string const & devname, unsigned int const nbufs) :
    itsDevName(devname), itsNbufs(nbufs)
{ }


// ##############################################################################################################
jevois::VideoInput::~VideoInput()
{ }

// ##############################################################################################################
bool jevois::VideoInput::hasScaledImage() const
{ throw std::runtime_error("hasScaledImage(): Second ISP-scaled camera image not available on this hardware"); }

// ##############################################################################################################
void jevois::VideoInput::get2(RawImage & JEVOIS_UNUSED_PARAM(img))
{ throw std::runtime_error("get2(): Second ISP-scaled camera image not available on this hardware"); }

// ##############################################################################################################
void jevois::VideoInput::done2(RawImage & JEVOIS_UNUSED_PARAM(img))
{ throw std::runtime_error("done2(): Second ISP-scaled camera image not available on this hardware"); }

