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

#include <jevois/Core/OutputFrame.H>

#include <jevois/Core/VideoOutput.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Util/Utils.H>
#include <opencv2/imgproc/imgproc.hpp>

// ####################################################################################################
jevois::OutputFrame::OutputFrame(std::shared_ptr<jevois::VideoOutput> const & gad, jevois::RawImage * excimg) :
    itsGadget(gad), itsDidGet(false), itsDidSend(false), itsImagePtrForException(excimg)
{ }

// ####################################################################################################
jevois::OutputFrame::~OutputFrame()
{
  // If itsGadget is invalidated, we have been moved to another object, so do not do anything here:
  if (itsGadget.get() == nullptr) return;

  // If we did not get(), just end now:
  if (itsDidGet == false) return;
  
  // If Engine gave us a non-zero image pointer for exceptions, and we did get(), pass down the buffer we did get, so
  // that Engine can write exception text into it, unless it is too late (exception occurred after send()) or too early
  // (before get()), in which case Engine will get its own buffer:
  if (itsImagePtrForException)
  {
    if (itsDidSend == false) { *itsImagePtrForException = itsImage; }
    // Engine will be responsible for the final send()
  }
  else
  {
    // If we did get() but not send(), send now (the image will likely contain garbage):
    if (itsDidSend == false) try { itsGadget->send(itsImage); } catch (...) { }
  }
}

// ####################################################################################################
jevois::RawImage const & jevois::OutputFrame::get() const
{
  itsGadget->get(itsImage);
  itsDidGet = true;
  return itsImage;
}

// ####################################################################################################
void jevois::OutputFrame::send() const
{
  itsGadget->send(itsImage);
  itsDidSend = true;
  if (itsImagePtrForException) itsImagePtrForException->invalidate();
}

// ####################################################################################################
void jevois::OutputFrame::sendCv(cv::Mat const & img, int quality) const
{
  switch(img.type())
  {
  case CV_8UC3: sendScaledCvBGR(img, quality); break;
  case CV_8UC1: sendScaledCvGRAY(img, quality); break;
  case CV_8UC4: sendScaledCvRGBA(img, quality); break;
  default: LFATAL("cv::Mat of type " << cvtypestr(img.type()) << " not supported.");
  }
}

// ####################################################################################################
void jevois::OutputFrame::sendCvGRAY(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvGRAYtoRawImage(img, rawimg, quality);
  send();
}

// ####################################################################################################
void jevois::OutputFrame::sendCvBGR(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvBGRtoRawImage(img, rawimg, quality);
  send();
}
// ####################################################################################################
void jevois::OutputFrame::sendCvRGB(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvRGBtoRawImage(img, rawimg, quality);
  send();
}

// ####################################################################################################
void jevois::OutputFrame::sendCvRGBA(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvRGBAtoRawImage(img, rawimg, quality);
  send();
}

// ####################################################################################################
void jevois::OutputFrame::sendScaledCvGRAY(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvGRAYtoRawImage(jevois::rescaleCv(img, cv::Size(rawimg.width, rawimg.height)),
                                            rawimg, quality);
  send();
}

// ####################################################################################################
void jevois::OutputFrame::sendScaledCvBGR(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvBGRtoRawImage(jevois::rescaleCv(img, cv::Size(rawimg.width, rawimg.height)),
                                           rawimg, quality);
  send();
}
// ####################################################################################################
void jevois::OutputFrame::sendScaledCvRGB(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvRGBtoRawImage(jevois::rescaleCv(img, cv::Size(rawimg.width, rawimg.height)),
                                           rawimg, quality);
  send();
}

// ####################################################################################################
void jevois::OutputFrame::sendScaledCvRGBA(cv::Mat const & img, int quality) const
{
  jevois::RawImage rawimg = get();
  jevois::rawimage::convertCvRGBAtoRawImage(jevois::rescaleCv(img, cv::Size(rawimg.width, rawimg.height)),
                                            rawimg, quality);
  send();
}

