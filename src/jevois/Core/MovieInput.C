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

#include <jevois/Core/MovieInput.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>

#include <opencv2/videoio.hpp> // for CV_CAP_PROP_POS_AVI_RATIO
#include <opencv2/imgproc/imgproc.hpp>

// ##############################################################################################################
jevois::MovieInput::MovieInput(std::string const & filename, unsigned int const nbufs) :
    jevois::VideoInput(filename, nbufs)
{
  // Open the movie file:
  if (itsCap.open(filename) == false) LFATAL("Failed to open movie or image sequence [" << filename << ']');
}

// ##############################################################################################################
jevois::MovieInput::~MovieInput()
{ }

// ##############################################################################################################
void jevois::MovieInput::streamOn()
{ }

// ##############################################################################################################
void jevois::MovieInput::abortStream()
{ }

// ##############################################################################################################
void jevois::MovieInput::streamOff()
{ }

// ##############################################################################################################
void jevois::MovieInput::get(RawImage & img)
{
  static size_t frameidx = 0; // only used for conversion info messages
  
  // Grab the next frame:
  cv::Mat frame;
  if (itsCap.read(frame) == false)
  {
    LINFO("End of input - Rewinding...");
    
    // Maybe end of file, reset the position:
    itsCap.set(cv::CAP_PROP_POS_AVI_RATIO, 0);

    // Try again:
    if (itsCap.read(frame) == false) LFATAL("Could not read next video frame");
  }

  // If dims do not match, resize:
  if (frame.cols != int(itsMapping.cw) || frame.rows != int(itsMapping.ch))
  {
    if (frameidx++ % 100 == 0)
      LINFO("Resizing frame from " << frame.cols <<'x'<< frame.rows << " to " << itsMapping.cw <<'x'<< itsMapping.ch);
    cv::resize(frame, frame, cv::Size(itsMapping.cw, itsMapping.ch));
  }
  
  // Reset our VideoBuf:
  itsBuf.reset(new jevois::VideoBuf(-1, itsMapping.csize(), 0, -1));

  // Set the fields in our output RawImage:
  img.width = itsMapping.cw;
  img.height = itsMapping.ch;
  img.fmt = itsMapping.cfmt;
  img.fps = itsMapping.cfps;
  img.buf = itsBuf;
  img.bufindex = 0;

  // Now convert from BGR to desired color format:
  jevois::rawimage::convertCvBGRtoRawImage(frame, img, 75);
}

// ##############################################################################################################
void jevois::MovieInput::done(RawImage & JEVOIS_UNUSED_PARAM(img))
{
  // Just nuke our buffer:
  itsBuf.reset();
}

// ##############################################################################################################
void jevois::MovieInput::queryControl(struct v4l2_queryctrl & JEVOIS_UNUSED_PARAM(qc)) const
{ throw std::runtime_error("Operation queryControl() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::queryMenu(struct v4l2_querymenu & JEVOIS_UNUSED_PARAM(qm)) const
{ throw std::runtime_error("Operation queryMenu() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::getControl(struct v4l2_control & JEVOIS_UNUSED_PARAM(ctrl)) const
{ throw std::runtime_error("Operation getControl() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::setControl(struct v4l2_control const & JEVOIS_UNUSED_PARAM(ctrl))
{ throw std::runtime_error("Operation setControl() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::setFormat(VideoMapping const & m)
{
  // Store the mapping so we can check frame size and format when grabbing:
  itsMapping = m;
}
