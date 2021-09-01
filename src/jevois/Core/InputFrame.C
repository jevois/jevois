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

#include <jevois/Core/InputFrame.H>

#include <jevois/Core/VideoInput.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Util/Utils.H>
#include <opencv2/imgproc/imgproc.hpp>

// ####################################################################################################
jevois::InputFrame::InputFrame(std::shared_ptr<jevois::VideoInput> const & cam, bool turbo) :
    itsCamera(cam), itsTurbo(turbo)
{ }

// ####################################################################################################
jevois::InputFrame::~InputFrame()
{
  // If itsCamera is invalidated, we have been moved to another object, so do not do anything here:
  if (itsCamera.get() == nullptr) return;

  // If we did not get(), do it now to avoid choking the camera:
  if (itsDidGet == false) try { get(); } catch (...) { }

  // If we did get() but not done(), signal done now:
  if (itsDidGet && itsDidDone == false) try { itsCamera->done(itsImage); } catch (...) { }

  // If we did not get2() and we are CropScale, do a get2() now:
  if (itsCamera->hasScaledImage() && itsDidGet2 == false) try { get2(); } catch (...) { }
  
  // If we did get2() but not done2(), signal done now:
  if (itsDidGet2 && itsDidDone2 == false) try { itsCamera->done2(itsImage2); } catch (...) { }
}

// ####################################################################################################
jevois::RawImage const & jevois::InputFrame::get(bool casync) const
{
  if (itsDidGet == false)
  {
    itsCamera->get(itsImage);
    itsDidGet = true;
    if (casync && itsTurbo) itsImage.buf->sync();
  }
  return itsImage;
}
// ####################################################################################################
bool jevois::InputFrame::hasScaledImage() const
{
  return itsCamera->hasScaledImage();
}

// ####################################################################################################
jevois::RawImage const & jevois::InputFrame::get2(bool casync) const
{
  if (itsDidGet2 == false)
  {
    itsCamera->get2(itsImage2);
    itsDidGet2 = true;
    if (casync && itsTurbo) itsImage2.buf->sync();
  }
  return itsImage2;
}

// ####################################################################################################
jevois::RawImage const & jevois::InputFrame::getp(bool casync) const
{
  if (hasScaledImage()) return get2(casync);
  return get(casync);
}

// ####################################################################################################
int jevois::InputFrame::getDmaFd(bool casync) const
{
  get(casync);
  itsDmaFd = itsImage.buf->dmaFd();
  return itsDmaFd;
}

// ####################################################################################################
int jevois::InputFrame::getDmaFd2(bool casync) const
{
  get2(casync);
  itsDmaFd2 = itsImage2.buf->dmaFd();
  return itsDmaFd2;
}

// ####################################################################################################
void jevois::InputFrame::done() const
{
  itsCamera->done(itsImage);
  itsDidDone = true;
}

// ####################################################################################################
void jevois::InputFrame::done2() const
{
  itsCamera->done2(itsImage2);
  itsDidDone2 = true;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvGRAY(bool casync) const
{
  jevois::RawImage const & rawimg = get(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvGray(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvBGR(bool casync) const
{
  jevois::RawImage const & rawimg = get(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvBGR(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvRGB(bool casync) const
{
  jevois::RawImage const & rawimg = get(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvRGB(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvRGBA(bool casync) const
{
  jevois::RawImage const & rawimg = get(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvRGBA(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvGRAYp(bool casync) const
{
  jevois::RawImage const & rawimg = getp(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvGray(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvBGRp(bool casync) const
{
  jevois::RawImage const & rawimg = getp(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvBGR(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvRGBp(bool casync) const
{
  jevois::RawImage const & rawimg = getp(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvRGB(rawimg);
  done();
  return cvimg;
}

// ####################################################################################################
cv::Mat jevois::InputFrame::getCvRGBAp(bool casync) const
{
  jevois::RawImage const & rawimg = getp(casync);
  cv::Mat cvimg = jevois::rawimage::convertToCvRGBA(rawimg);
  done();
  return cvimg;
}

