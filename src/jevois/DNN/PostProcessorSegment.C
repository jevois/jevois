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

#include <jevois/DNN/PostProcessorSegment.H>
#include <jevois/DNN/PreProcessor.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>
#include <jevois/Types/ObjReco.H>

#include <opencv2/dnn.hpp>

// ####################################################################################################
jevois::dnn::PostProcessorSegment::~PostProcessorSegment()
{
#ifdef JEVOIS_PRO
  if (itsHelper) itsHelper->releaseImage("ppsr");
#endif
}

// ####################################################################################################
void jevois::dnn::PostProcessorSegment::freeze(bool doit)
{
  segtype::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorSegment::postInit()
{
  // Colormap from pascal VOC segmentation benchmark: 
  for (size_t i = 0; i < 256; ++i)
  {
    uint32_t & c = itsColor[i]; c = 0;
    uint32_t ind = i;
    
    for (int shift = 7; shift >= 0; --shift)
    {
      for (int channel = 0; channel < 3; ++channel) c |= ((ind >> channel) & 1) << (shift + 8 * (3-channel));
      ind >>= 3;
    }
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorSegment::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  if (outs.empty()) return;

  switch (segtype::get())
  {
    // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::SegType::Classes:
  {
    // tensor should be 4D 1xHxWxN, where N is the number of classes. We pick the class index with max value and
    // apply the colormap to it:
    cv::Mat results = outs[0]; cv::MatSize rs = results.size;
    if (rs.dims() != 4 || rs[0] != 1 || results.type() != CV_8UC1)
      LFATAL("Output tensor is " << jevois::dnn::shapestr(results) << ", but need 4D UINT8 with 1 as first size");
    int const numclass = rs[3]; int const siz = rs[1] * rs[2] * numclass;
    
    // Apply colormap, converting from RGB to RGBA. If the background class ID is 0, change its color in the colormap:
    uint32_t const alph = alpha::get() << 24;
    int const bgclass = bgid::get(); if (bgclass != 0) itsColor[0] = 0xff0000; else itsColor[0] = 0;
    
    itsOverlay = cv::Mat(rs[1], rs[2], CV_8UC4);
    uint32_t * im = reinterpret_cast<uint32_t *>(itsOverlay.data);
    uint8_t * r = reinterpret_cast<uint8_t *>(results.data);

    for (int i = 0; i < siz; i += numclass)
    {
      int maxc = -1; int maxval = -1;
      for (int c = 0; c < numclass; ++c)
      {
        int v = *r++;
        if (v > maxval) { maxval = v; maxc = c; }
      }
      
      // Use full transparent for class bgclass or if out of bounds, otherwise colormap:
      if (maxc < 0 || maxc > 255 || maxc == bgclass) *im++ = 0; else *im++ = itsColor[maxc] | alph;
    }
  }
  break;

  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::SegType::Classes2:
  {
    // tensor should be 4D 1xNxHxW, where N is the number of classes. We pick the class index with max value and
    // apply the colormap to it:
    cv::Mat results = outs[0]; cv::MatSize rs = results.size;
    if (rs.dims() != 4 || rs[0] != 1 || results.type() != CV_32FC1)
      LFATAL("Output tensor is " << jevois::dnn::shapestr(results) << ", but need 4D FLOAT32 with 1 as first size");
    int const numclass = rs[1]; int const siz = rs[2] * rs[3];
    
    // Apply colormap, converting from RGB to RGBA. If the background class ID is 0, change its color in the colormap:
    uint32_t const alph = alpha::get() << 24;
    int const bgclass = bgid::get(); if (bgclass != 0) itsColor[0] = 0xff0000; else itsColor[0] = 0;
    
    itsOverlay = cv::Mat(rs[2], rs[3], CV_8UC4);
    uint32_t * im = reinterpret_cast<uint32_t *>(itsOverlay.data);
    
    for (int i = 0; i < siz; ++i)
    {
      int maxc = -1; float maxval = -1.0F;
      for (int c = 0; c < numclass; ++c)
      {
        float v = results.at<float>(i + c*siz);
        if (v > maxval) { maxval = v; maxc = c; }
      }
    
      // Use full transparent for class bgclass or if out of bounds, otherwise colormap:
      if (maxc < 0 || maxc > 255 || maxc == bgclass) *im++ = 0; else *im++ = itsColor[maxc] | alph;
    }
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::SegType::ArgMax:
  {
    // tensor should be 3D 1xHxW INT32 and contain class ID in each pixel:
    cv::Mat results = outs[0]; cv::MatSize rs = results.size;
    if (rs.dims() != 3 || rs[0] != 1 || results.type() != CV_32SC1)
      LFATAL("Output tensor is " << jevois::dnn::shapestr(results) << ", but need 3D INT32 with 1 as first size");
    int const siz = rs[1] * rs[2];
    
    // Apply colormap, converting from RGB to RGBA. If the background class ID is 0, change its color in the colormap:
    uint32_t const alph = alpha::get() << 24;
    int const bgclass = bgid::get(); if (bgclass != 0) itsColor[0] = 0xff0000; else itsColor[0] = 0;
    
    itsOverlay = cv::Mat(rs[1], rs[2], CV_8UC4);
    uint32_t * im = reinterpret_cast<uint32_t *>(itsOverlay.data);
    int32_t const * r = reinterpret_cast<int32_t const *>(results.data);
    
    for (int i = 0; i < siz; ++i)
    {
      // Use full transparent for class bgclass or if out of bounds, otherwise colormap:
      int32_t const id = *r++;
      if (id < 0 || id > 255 || id == bgclass) *im++ = 0; else *im++ = itsColor[id] | alph;
    }
  }
  break;
  }

  // Compute overlay corner coords within the input image, for use in report():
  preproc->getUnscaledCropRect(0, itsTLx, itsTLy, itsBRx, itsBRy);
}

// ####################################################################################################
void jevois::dnn::PostProcessorSegment::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                               jevois::OptGUIhelper * helper, bool JEVOIS_UNUSED_PARAM(overlay),
                                               bool JEVOIS_UNUSED_PARAM(idle))
{
  // Remember our helper, will be used in destructor to free overlay OpenGL texture:
  itsHelper = helper;

  // Outputs may not be ready yet:
  if (itsOverlay.empty()) return;
  
  // If desired, draw boxes in output image:
  if (outimg)
  {
    // todo: blend into YUYV
  }

#ifdef JEVOIS_PRO
  // Draw the image on top of our input image, as a semi-transparent overlay. OpenGL will do scaling and blending:
  if (helper)
  {
    // Convert box coords from input image to display:
    ImVec2 tl = helper->i2d(itsTLx, itsTLy), br = helper->i2d(itsBRx, itsBRy);
    int dtlx = tl.x, dtly = tl.y;
    unsigned short dw = br.x - tl.x, dh = br.y - tl.y;

    // Draw overlay:
    helper->drawImage("ppsr", itsOverlay, true, dtlx, dtly, dw, dh, false /* noalias */, true /* isoverlay */);
  }
#else
  (void)helper; // keep compiler happy  
#endif   

  if (mod)
  {
    // todo send results to serial
  }
}
