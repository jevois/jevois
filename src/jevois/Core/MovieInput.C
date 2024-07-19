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
bool jevois::MovieInput::hasScaledImage() const
{
  return (itsMapping.c2fmt != 0);
}

// ##############################################################################################################
void jevois::MovieInput::get(RawImage & img)
{
  static size_t frameidx = 0; // only used for conversion info messages

  // Users may call get() several times on a given frame. The switch to the next frame is when done() is called, which
  // invalidates itsBuf:
  if (itsBuf)
  {
    // Just pass the buffer to the rawimage:
    img.width = itsMapping.cw;
    img.height = itsMapping.ch;
    img.fmt = itsMapping.cfmt;
    img.fps = itsMapping.cfps;
    img.buf = itsBuf;
    img.bufindex = 0;

    return;
  }
    
  // Grab the next frame:
  if (itsCap.read(itsRawFrame) == false)
  {
    LINFO("End of input - Rewinding...");
    
    // Maybe end of file, reset the position:
    itsCap.set(cv::CAP_PROP_POS_AVI_RATIO, 0);
    itsCap.set(cv::CAP_PROP_POS_FRAMES, 0);
    
    // Try again:
    if (itsCap.read(itsRawFrame) == false) LFATAL("Could not read next video frame");
  }
  
  // If dims do not match, resize:
  cv::Mat frame;
  if (itsRawFrame.cols != int(itsMapping.cw) || itsRawFrame.rows != int(itsMapping.ch))
  {
    if ((frameidx++ % 100) == 0)
      LINFO("Note: Resizing get() frame from " << itsRawFrame.cols <<'x'<< itsRawFrame.rows << " to " <<
            itsMapping.cw <<'x'<< itsMapping.ch);
    cv::resize(itsRawFrame, frame, cv::Size(itsMapping.cw, itsMapping.ch));
  }
  else frame = itsRawFrame;
  
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
void jevois::MovieInput::get2(RawImage & img)
{
  static size_t frameidx2 = 0; // only used for conversion info messages

  // Users may call get2() several times on a given frame. The switch to the next frame is when done2() is called, which
  // invalidates itsBuf2:
  if (itsBuf2)
  {
    // Just pass the buffer to the rawimage:
    img.width = itsMapping.c2w;
    img.height = itsMapping.c2h;
    img.fmt = itsMapping.c2fmt;
    img.fps = itsMapping.cfps;
    img.buf = itsBuf2;
    img.bufindex = 0;

    return;
  }

  // If get2() is called before get() let's call get() now:
  if (! itsBuf)
  {
    jevois::RawImage tmp;
    get(tmp);
  }

  // Now both itsBuf and itsRawFrame are valid, let's just convert/resize itsRawFrame into our second frame format:
  cv::Mat frame;
  if (itsRawFrame.cols != int(itsMapping.c2w) || itsRawFrame.rows != int(itsMapping.c2h))
  {
    if ((frameidx2++ % 100) == 0)
      LINFO("Note: Resizing get2() frame from " << itsRawFrame.cols <<'x'<< itsRawFrame.rows << " to " <<
            itsMapping.c2w <<'x'<< itsMapping.c2h);
    cv::resize(itsRawFrame, frame, cv::Size(itsMapping.c2w, itsMapping.c2h));
  }
  else frame = itsRawFrame;
  
  // Reset our VideoBuf:
  itsBuf2.reset(new jevois::VideoBuf(-1, itsMapping.c2size(), 0, -1));
  
  // Set the fields in our output RawImage:
  img.width = itsMapping.c2w;
  img.height = itsMapping.c2h;
  img.fmt = itsMapping.c2fmt;
  img.fps = itsMapping.cfps;
  img.buf = itsBuf2;
  img.bufindex = 0;

  // Now convert from BGR to desired color format:
  jevois::rawimage::convertCvBGRtoRawImage(frame, img, 75);
}

// ##############################################################################################################
void jevois::MovieInput::done(RawImage &)
{
  // Just nuke our buffer:
  itsBuf.reset();
  itsRawFrame = cv::Mat();
}

// ##############################################################################################################
void jevois::MovieInput::done2(RawImage &)
{
  // Just nuke our buffer:
  itsBuf2.reset();
}

// ##############################################################################################################
void jevois::MovieInput::queryControl(struct v4l2_queryctrl &) const
{ throw std::runtime_error("Operation queryControl() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::queryMenu(struct v4l2_querymenu &) const
{ throw std::runtime_error("Operation queryMenu() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::getControl(struct v4l2_control &) const
{ throw std::runtime_error("Operation getControl() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::setControl(struct v4l2_control const &)
{ throw std::runtime_error("Operation setControl() not supported by MovieInput"); }

// ##############################################################################################################
void jevois::MovieInput::setFormat(VideoMapping const & m)
{
  // Store the mapping so we can check frame size and format when grabbing:
  itsMapping = m;
}
