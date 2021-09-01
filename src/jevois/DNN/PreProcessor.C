// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2021 by Laurent Itti, the University of Southern
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

#include <jevois/DNN/PreProcessor.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Util/Utils.H>
#include <jevois/Core/Engine.H>

// ####################################################################################################
jevois::dnn::PreProcessor::~PreProcessor()
{ }

// ####################################################################################################
std::vector<cv::Mat> const & jevois::dnn::PreProcessor::blobs() const
{ return itsBlobs; }

// ####################################################################################################
cv::Size const & jevois::dnn::PreProcessor::imagesize() const
{ return itsImageSize; }

// ####################################################################################################
cv::Size jevois::dnn::PreProcessor::blobsize(size_t num) const
{
  if (num >= itsAttrs.size()) LFATAL("Invalid blob number " << num << ", only have " << itsAttrs.size() << " blobs");
  return jevois::dnn::attrsize(itsAttrs[num]);
}

// ####################################################################################################
void jevois::dnn::PreProcessor::b2i(float & x, float & y, size_t blobnum)
{
  if (blobnum >= itsCrops.size())
    LFATAL("Invalid blob number " << blobnum << ", only have " << itsCrops.size() << " crops");

  cv::Rect const & r = itsCrops[blobnum];
  b2i(x, y, blobsize(blobnum), (r.x != 0 || r.y != 0));
}

// ####################################################################################################
void jevois::dnn::PreProcessor::b2i(float & x, float & y, cv::Size const & bsiz, bool letterboxed)
{
  if (bsiz.width == 0 || bsiz.height == 0) LFATAL("Cannot handle zero blob width or height");

  if (letterboxed)
  {
    // We did letterbox and crop, so we need to apply scale and offset:
    float const fac = std::min(itsImageSize.width / float(bsiz.width), itsImageSize.height / float(bsiz.height));
    float const cropw = fac * bsiz.width + 0.4999F;
    float const croph = fac * bsiz.height + 0.4999F;
    x = (itsImageSize.width - cropw) * 0.5F + x * fac;
    y = (itsImageSize.height - croph) * 0.5F + y * fac;
  }
  else
  {
    x *= itsImageSize.width / float(bsiz.width);
    y *= itsImageSize.height / float(bsiz.height);
  }
}

// ####################################################################################################
cv::Rect jevois::dnn::PreProcessor::getUnscaledCropRect(size_t num)
{
  if (num >= itsCrops.size()) LFATAL("Invalid blob number " << num << ", only have " << itsCrops.size() << " blobs");
  return itsCrops[num];
}

// ####################################################################################################
void jevois::dnn::PreProcessor::getUnscaledCropRect(size_t num, int & tlx, int & tly, int & brx, int & bry)
{
  cv::Rect const & r = getUnscaledCropRect(num);
  tlx = r.x; tly = r.y; brx = r.x + r.width; bry = r.y + r.height;
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::PreProcessor::process(jevois::RawImage const & img,
                                                        std::vector<vsi_nn_tensor_attr_t> const & attrs)
{
  // Store input image size and format for future use:
  itsImageSize.width = img.width; itsImageSize.height = img.height; itsImageFmt = img.fmt;
  itsCrops.clear(); itsBlobs.clear();

  if (itsAttrs.empty()) itsAttrs = attrs;
  if (itsAttrs.empty()) LFATAL("Cannot work with no input tensors");

  // Do the pre-processing:
  if (img.fmt == V4L2_PIX_FMT_RGB24)
    itsBlobs = process(jevois::rawimage::cvImage(img), rgb::get(), itsAttrs, itsCrops);
  else if (img.fmt == V4L2_PIX_FMT_BGR24)
    itsBlobs = process(jevois::rawimage::cvImage(img), ! rgb::get(), itsAttrs, itsCrops);
  else if (rgb::get())
    itsBlobs = process(jevois::rawimage::convertToCvRGB(img), false, itsAttrs, itsCrops);
  else
    itsBlobs = process(jevois::rawimage::convertToCvBGR(img), false, itsAttrs, itsCrops);
  
  return itsBlobs;
}

// ####################################################################################################
void jevois::dnn::PreProcessor::sendreport(jevois::StdModule * mod, jevois::RawImage * outimg,
                                           jevois::OptGUIhelper * helper, bool overlay, bool idle)
{
  // First some info about the input:
#ifdef JEVOIS_PRO
  if (helper && idle == false && ImGui::CollapsingHeader("Pre-Processing", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::BulletText("Input image: %dx%d %s", itsImageSize.width, itsImageSize.height,
                      jevois::fccstr(itsImageFmt).c_str());

    if (itsImageFmt != V4L2_PIX_FMT_RGB24 && itsImageFmt != V4L2_PIX_FMT_BGR24)
    {
      if (rgb::get()) ImGui::BulletText("Convert to RGB");
      else ImGui::BulletText("Convert to BGR");
    }
  }
#endif

  // Now a report from the derived class:
  report(mod, outimg, helper, overlay, idle);

  // If desired, draw a rectangle around the network input:
  if (showin::get())
  {
#ifdef JEVOIS_PRO
    if (helper)
      for (cv::Rect const & r : itsCrops)
        ImGui::GetBackgroundDrawList()->AddRect(helper->i2d(r.x, r.y),
                                                helper->i2d(r.x + r.width, r.y + r.height), 0x80808080, 0, 0, 5);
#else
    (void)helper; // keep compiler happy
#endif
    
    if (outimg)
      for (cv::Rect const & r : itsCrops)
        jevois::rawimage::drawRect(*outimg, r.x, r.y, r.width, r.height, 3, jevois::yuyv::MedGrey);
  }

  // Finally some info about the blobs:
#ifdef JEVOIS_PRO
  if (helper && idle == false)
  {
    int idx = 0;
    for (cv::Mat const & blob : itsBlobs)
    {
      cv::Rect const & r = itsCrops[idx];
      bool const stretch = (r.x == 0 && r.y == 0);
      
      ImGui::BulletText("Crop %d: %dx%d @ %d,%d %s", idx, r.width, r.height, r.x, r.y,
                        stretch ? "" : "(letterbox)");
      ImGui::BulletText("Blob %d: %s %s", idx, jevois::dnn::shapestr(blob).c_str(),
                        stretch ? "(stretch)" : "(uniform)");

      ++idx;
    }
  }
#endif
}
