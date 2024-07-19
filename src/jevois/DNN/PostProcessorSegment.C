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
template <typename T>
void jevois::dnn::PostProcessorSegment::process(cv::Mat const & results)
{
  int const bgclass = bgid::get();
  uint32_t const alph = alpha::get() << 24;
  cv::MatSize const rs = results.size;
  T const * r = reinterpret_cast<T const *>(results.data);
  T const thresh(cthresh::get() * 0.01F);

  switch (segtype::get())
  {
    // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::SegType::ClassesHWC:
  {
    // tensor should be 4D 1xHxWxC, where C is the number of classes. We pick the class index with max value and
    // apply the colormap to it:
    if (rs.dims() != 4 || rs[0] != 1) LTHROW("Need 1xHxWxC for C classes");
    int const numclass = rs[3]; int const siz = rs[1] * rs[2] * numclass;
    
    // Apply colormap, converting from RGB to RGBA:
    itsOverlay = cv::Mat(rs[1], rs[2], CV_8UC4);
    uint32_t * im = reinterpret_cast<uint32_t *>(itsOverlay.data);
    
    for (int i = 0; i < siz; i += numclass)
    {
      int maxc = -1; T maxval = thresh;
      for (int c = 0; c < numclass; ++c)
      {
        T v = *r++;
        if (v > maxval) { maxval = v; maxc = c; }
      }
      
      // Use full transparent for class bgclass or if out of bounds, otherwise colormap:
      if (maxc < 0 || maxc > 255 || maxc == bgclass) *im++ = 0; else *im++ = itsColor[maxc] | alph;
    }
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::SegType::ClassesCHW:
  {
    // tensor should be 4D 1xCxHxW, where C is the number of classes. We pick the class index with max value and
    // apply the colormap to it:
    if (rs.dims() != 4 || rs[0] != 1) LTHROW("Need 1xCxHxW for C classes");
    int const numclass = rs[1]; int const hw = rs[2] * rs[3];
    
    // Apply colormap, converting from RGB to RGBA:
    itsOverlay = cv::Mat(rs[2], rs[3], CV_8UC4);
    uint32_t * im = reinterpret_cast<uint32_t *>(itsOverlay.data);
    
    for (int i = 0; i < hw; ++i)
    {
      int maxc = -1; T maxval = thresh;
      for (int c = 0; c < numclass; ++c)
      {
        T v = results.at<T>(i + c * hw);
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
    // tensor should be 2D HxW, 3D 1xHxW, or 4D 1xHxWx1 and contain class ID in each pixel:
    if (rs.dims() != 2 && (rs.dims() != 3 || rs[0] != 1) && (rs.dims() != 4 || rs[0] != 1 || rs[3] != 1))
      LTHROW("Need shape HxW, 1xHxW, or 1xHxWx1 with class ID in each pixel");
    int const siz = rs[1] * rs[2];
    
    // Apply colormap, converting from RGB to RGBA:
    itsOverlay = cv::Mat(rs[1], rs[2], CV_8UC4);
    uint32_t * im = reinterpret_cast<uint32_t *>(itsOverlay.data);
    
    for (int i = 0; i < siz; ++i)
    {
      // Use full transparent for class bgclass or if out of bounds, otherwise colormap:
      int32_t const id = *r++;
      if (id < 0 || id > 255 || id == bgclass) *im++ = 0; else *im++ = itsColor[id] | alph;
    }
  }
  break;
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorSegment::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  try
  {
    if (outs.size() != 1) LTHROW("Need exactly one output blob");

    // Patch up the colormap if background class ID is 0:
    if (bgid::get() != 0) itsColor[0] = 0xff0000; else itsColor[0] = 0;
    
    // Post-process:
    cv::Mat const & results = outs[0];
    
    switch (results.type())
    {
    case CV_8UC1: process<uint8_t>(results); break;
    case CV_16UC1: process<uint16_t>(results); break;
    case CV_32FC1: process<float>(results); break;
    case CV_32SC1: process<int32_t>(results); break;
      
    default: LTHROW("Unsupported data type in tensor " << jevois::dnn::shapestr(results));
    }
  }
  // Abort here if the received outputs were malformed:
  catch (std::exception const & e)
  {
    std::string err = "Selected segtype is " + segtype::strget() + " and network produced:\n\n";
    for (cv::Mat const & m : outs) err += "- " + jevois::dnn::shapestr(m) + "\n";
    err += "\nFATAL ERROR(s):\n\n";
    err += e.what();
    LFATAL(err);
  }

  // Compute overlay corner coords within the input image, for use in report():
  preproc->getUnscaledCropRect(0, itsTLx, itsTLy, itsBRx, itsBRy);
}

// ####################################################################################################
void jevois::dnn::PostProcessorSegment::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                               jevois::OptGUIhelper * helper, bool /*overlay*/, bool /*idle*/)
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
