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

#include <jevois/Core/VideoDisplay.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>

#include <linux/videodev2.h>

// Do not compile any highgui-dependent code on the JeVois platform, since it does not have a display.
#ifndef JEVOIS_PLATFORM
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// ##############################################################################################################
jevois::VideoDisplay::VideoDisplay(char const * displayname, size_t nbufs) :
    jevois::VideoOutput(), itsImageQueue(std::max(size_t(2), nbufs)), itsName(displayname)
{
  // Open an openCV window:
  cv::namedWindow(itsName, CV_WINDOW_AUTOSIZE); // autosize keeps the original size
  //cv::namedWindow(itsName, CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO); // normal can be resized
}

// ##############################################################################################################
void jevois::VideoDisplay::setFormat(jevois::VideoMapping const & m)
{
  // Nuke any old buffers:
  itsBuffers.clear();
  itsImageQueue.clear();
  size_t const nbufs = itsImageQueue.size();
  
  // Allocate the buffers and make them all immediately available as RawImage:
  unsigned int imsize = m.osize();

  for (size_t i = 0; i < nbufs; ++i)
  {
    itsBuffers.push_back(std::make_shared<jevois::VideoBuf>(-1, imsize, 0));

    jevois::RawImage img;
    img.width = m.ow;
    img.height = m.oh;
    img.fmt = m.ofmt;
    img.buf = itsBuffers[i];
    img.bufindex = i;

    // Push the RawImage to outside consumers:
    itsImageQueue.push(img);
  }
  
  LDEBUG("Allocated " << nbufs << " buffers");

  // Open an openCV window:
  cv::namedWindow(itsName, CV_WINDOW_AUTOSIZE);
}

// ##############################################################################################################
jevois::VideoDisplay::~VideoDisplay()
{
  // Free all our buffers:
  for (auto & b : itsBuffers)
  {
    if (b.use_count() > 1) LERROR("Ref count non zero when attempting to free VideoBuf");

    b.reset(); // VideoBuf destructor will do the memory freeing
  }

  itsBuffers.clear();

  // Close opencv window, we need a waitKey() for it to actually close:
  cv::waitKey(1);
  cv::destroyWindow(itsName);
  cv::waitKey(20);
}

// ##############################################################################################################
void jevois::VideoDisplay::get(jevois::RawImage & img)
{
  // Take this buffer out of our queue and hand it over:
  img = itsImageQueue.pop();
  LDEBUG("Empty image " << img.bufindex << " handed over to application code for filling");
}

// ##############################################################################################################
void jevois::VideoDisplay::send(jevois::RawImage const & img)
{
  // OpenCV uses BGR color for display:
  cv::Mat imgbgr;

  // Convert the image to openCV and to BGR:
  switch (img.fmt)
  {
  case V4L2_PIX_FMT_YUYV:
  {
    cv::Mat imgcv(img.height, img.width, CV_8UC2, img.buf->data());
    cv::cvtColor(imgcv, imgbgr, CV_YUV2BGR_YUYV);
  }
  break;

  case V4L2_PIX_FMT_GREY:
  {
    cv::Mat imgcv(img.height, img.width, CV_8UC1, img.buf->data());
    cv::cvtColor(imgcv, imgbgr, CV_GRAY2BGR);
  }
  break;

  case V4L2_PIX_FMT_SRGGB8:
  {
    cv::Mat imgcv(img.height, img.width, CV_8UC1, img.buf->data());
    cv::cvtColor(imgcv, imgbgr, CV_BayerBG2BGR);
  }
  break;

  case V4L2_PIX_FMT_RGB565:
  {
    cv::Mat imgcv(img.height, img.width, CV_8UC2, img.buf->data());
    cv::cvtColor(imgcv, imgbgr, CV_BGR5652BGR);
  }
  break;

  default: LFATAL("Unsupported video format");
  }
      
  // Display image:
  cv::imshow(itsName, imgbgr);

  // OpenCV needs this to actually update the display. Delay is in millisec:
  cv::waitKey(1);

  // Just push the buffer back into our queue. Note: we do not bother clearing the data or checking that the image is
  // legit, i.e., matches one that was obtained via get():
  itsImageQueue.push(img);
  LDEBUG("Empty image " << img.bufindex << " ready for filling in by application code");
}

// ##############################################################################################################
void jevois::VideoDisplay::streamOn()
{ }

// ##############################################################################################################
void jevois::VideoDisplay::abortStream()
{ }

// ##############################################################################################################
void jevois::VideoDisplay::streamOff()
{ }

#else //  JEVOIS_PLATFORM

// OpenCV is not compiled with HighGui support by buildroot by default, and anyway we can't use it on the platform since
// it has no display, so let's not waste resources linking to it:
jevois::VideoDisplay::VideoDisplay(char const * displayname, size_t nbufs) :
  itsImageQueue(nbufs), itsName(displayname)
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }
 
jevois::VideoDisplay::~VideoDisplay()
{ LERROR("VideoDisplay is not supported on JeVois hardware platform"); }

void jevois::VideoDisplay::setFormat(jevois::VideoMapping const & JEVOIS_UNUSED_PARAM(m))
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }

void jevois::VideoDisplay::get(jevois::RawImage & JEVOIS_UNUSED_PARAM(img))
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }

void jevois::VideoDisplay::send(jevois::RawImage const & JEVOIS_UNUSED_PARAM(img))
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }

void jevois::VideoDisplay::streamOn()
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }

void jevois::VideoDisplay::abortStream()
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }

void jevois::VideoDisplay::streamOff()
{ LFATAL("VideoDisplay is not supported on JeVois hardware platform"); }

#endif //  JEVOIS_PLATFORM

