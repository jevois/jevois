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

#include <jevois/DNN/PostProcessorDetectOBB.H>
#include <jevois/DNN/PreProcessor.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>

#include <opencv2/dnn.hpp>
#include <cmath>

// ####################################################################################################
jevois::dnn::PostProcessorDetectOBB::~PostProcessorDetectOBB()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorDetectOBB::freeze(bool doit)
{
  classes::freeze(doit);
  detecttypeobb::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetectOBB::onParamChange(postprocessor::classes const &, std::string const & val)
{
  if (val.empty()) { itsLabels.clear(); return; }
  itsLabels = jevois::dnn::readLabelsFile(jevois::absolutePath(JEVOIS_SHARE_PATH, val));
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetectOBB::onParamChange(postprocessor::detecttypeobb const &,
                                                        postprocessor::DetectTypeOBB const & val)
{
  // Nothing so far
  (void)val;
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetectOBB::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  if (outs.empty()) LFATAL("No outputs received, we need at least one.");
  cv::Mat const & out = outs[0]; cv::MatSize const & msiz = out.size;

  float const confThreshold = cthresh::get() * 0.01F;
  //float const boxThreshold = dthresh::get() * 0.01F;
  float const nmsThreshold = nms::get() * 0.01F;
  bool const sigmo = sigmoid::get();
  int const fudge = classoffset::get();
  itsImageSize = preproc->imagesize();
  
  // To draw boxes, we will need to:
  // - scale from [0..1]x[0..1] to blobw x blobh
  // - scale and center from blobw x blobh to input image w x h, provided by PreProcessor::b2i()
  // - when using the GUI, we further scale and translate to OpenGL display coordinates using GUIhelper::i2d()
  // Here we assume that the first blob sets the input size.
  //cv::Size const bsiz = preproc->blobsize(0);
  
  // We keep 3 vectors here instead of creating a class to hold all of the data because OpenCV will need that for
  // non-maximum suppression:
  std::vector<int> classIds;
  std::vector<float> confidences;
  std::vector<cv::RotatedRect> boxes;

  // Here we just scale the coords from [0..1]x[0..1] to blobw x blobh:
  try
  {
    switch(detecttypeobb::get())
    {
      // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectTypeOBB::YOLOv8:
    {
      // Network produces several (usually 3, for 3 strides) sets of 3 blobs: 1x64xHxW (raw boxes) and 1xCxHxW (class
      // scores), 1x1xHxW (box angles):
      if ((outs.size() % 3) != 0 || msiz.dims() != 4 || msiz[0] != 1)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 2 blobs: 1x64xHxW (raw boxes), "
               "1xCxHxW (class scores), and 1x1xHxW (box angles)");

      int stride = 8;
      int constexpr reg_max = 16;
      
      for (size_t idx = 0; idx < outs.size(); idx += 3)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[1] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1x64xHxW");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xCxHxW");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[1];
        
        cv::Mat const & ang = outs[idx + 2]; cv::MatSize const & ang_siz = ang.size;
        if (cls_siz.dims() != 4 || ang_siz[1] != 1) LTHROW("Output " << idx << " is not 4D 1x1xHxW");
        float const * ang_data = (float const *)ang.data;

        for (int i = 2; i < 4; ++i)
          if (cls_siz[i] != bx_siz[i]) LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 1);

        size_t const step = cls_siz[2] * cls_siz[3]; // HxW
        
        // Loop over all locations:
        for (int y = 0; y < cls_siz[2]; ++y)
          for (int x = 0; x < cls_siz[3]; ++x)
          {
            // Get the top class score:
            size_t best_idx = 0; float confidence = cls_data[0];
            for (size_t i = 1; i < nclass; ++i)
              if (cls_data[i * step] > confidence) { confidence = cls_data[i * step]; best_idx = i; }

            // Apply sigmoid to it, if needed (output layer did not already have sigmoid activations):
            if (sigmo) confidence = jevois::dnn::sigmoid(confidence);
            
            if (confidence >= confThreshold)
            {
              // Decode a rotated box from 64 received values and one angle:
              // See Netron and https://github.com/ultralytics/ultralytics/issues/624

              // Raw angle is in [0..1] with an offset, such that a value 0.25 means 0.0:
              // See in Netron how after the last conv layer for angles, 0.25 is subtracted then mul by pi:
              float angle = (jevois::dnn::sigmoid(*ang_data) - 0.25F) * M_PI;

              // Angle in [-pi/4,3/4 pi) -> [-pi/2,pi/2)
              if (angle >= 0.5F * M_PI && angle <= 0.75F * M_PI) angle -= M_PI;

              float const cosa = std::cos(angle);
              float const sina = std::sin(angle);
              
              // Now the rotated box:
              float dst[reg_max];
              float const ltx = softmax_dfl(bx_data, dst, reg_max, step);
              float const lty = softmax_dfl(bx_data + reg_max * step, dst, reg_max, step);
              float const rbx = softmax_dfl(bx_data + 2 * reg_max * step, dst, reg_max, step);
              float const rby = softmax_dfl(bx_data + 3 * reg_max * step, dst, reg_max, step);

              float const xf = 0.5F * (rbx - ltx);
              float const yf = 0.5F * (rby - lty);

              float const cx = (x + 0.5F + xf * cosa - yf * sina) * stride;
              float const cy = (y + 0.5F + xf * sina + yf * cosa) * stride;
              float const width = (ltx + rbx) * stride;
              float const height = (lty + rby) * stride;
              
              // Store this detection:
              boxes.push_back(cv::RotatedRect(cv::Point2f(cx, cy), cv::Size2f(width, height), angle * 180.0F / M_PI));
              classIds.push_back(int(best_idx) + fudge);
              confidences.push_back(confidence);
            }

            // Move to the next location:
            ++cls_data; ++bx_data; ++ang_data;
          }

        // Move to the next scale:
        stride *= 2;
      }
    }
    break;
 
    // ----------------------------------------------------------------------------------------------------
    default:
      // Do not use strget() here as it will throw!
      LTHROW("Unsupported Post-processor detecttype " << int(detecttypeobb::get()));
    }
  }
  // Abort here if the received outputs were malformed:
  catch (std::exception const & e)
  {
    std::string err = "Selected detecttypeobb is " + detecttypeobb::strget() + " and network produced:\n\n";
    for (cv::Mat const & m : outs) err += "- " + jevois::dnn::shapestr(m) + "\n";
    err += "\nFATAL ERROR(s):\n\n";
    err += e.what();
    LFATAL(err);
  }

  // Cleanup overlapping boxes, either globally or per class, and possibly limit number of reported boxes:
  std::vector<int> indices;
  /* not supported yet by opencv...
  if (nmsperclass::get())
    cv::dnn::NMSBoxesBatched(boxes, confidences, classIds, confThreshold, nmsThreshold, indices, 1.0F, maxnbox::get());
  else */
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices, 1.0F, maxnbox::get());

  // Now adjust the boxes from blob size to input image size:
  for (cv::RotatedRect & b : boxes)
  {
    preproc->b2i(b.center.x, b.center.y);
    preproc->b2is(b.size.width, b.size.height);
  }

  // Store results:
  itsDetections.clear(); bool namonly = namedonly::get();
  for (size_t i = 0; i < indices.size(); ++i)
  {
    int idx = indices[i];
    cv::RotatedRect const & box = boxes[idx];
    std::string const label = jevois::dnn::getLabel(itsLabels, classIds[idx], namonly);
    if (namonly == false || label.empty() == false)
    {
      jevois::ObjReco o { confidences[idx] * 100.0f, label };
      std::vector<jevois::ObjReco> ov;
      ov.emplace_back(o);
      jevois::ObjDetectOBB od { box, ov };
      itsDetections.emplace_back(od);
    }
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetectOBB::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                                 jevois::OptGUIhelper * helper, bool overlay,
                                                 bool /*idle*/)
{
  bool const serreport = serialreport::get();
  
  for (jevois::ObjDetectOBB const & o : itsDetections)
  {
    std::string categ, label;

    if (o.reco.empty())
    {
      categ = "unknown";
      label = "unknown";
    }
    else
    {
      categ = o.reco[0].category;
      label = jevois::sformat("%s: %.2f", categ.c_str(), o.reco[0].score);
    }

    // If desired, draw boxes in output image:
    if (outimg && overlay)
    {
      std::vector<cv::Point2f> pts; o.rect.points(pts);
      for (size_t i = 1; i < pts.size(); ++i)
        jevois::rawimage::drawLine(*outimg, pts[i-1].x, pts[i-1].y, pts[i].x, pts[i].y, 2, jevois::yuyv::LightGreen);
      jevois::rawimage::drawLine(*outimg, pts.back().x, pts.back().y, pts[0].x, pts[0].y, 2, jevois::yuyv::LightGreen);

      jevois::rawimage::writeText(*outimg, label, pts[0].x + 6, pts[0].y + 2, jevois::yuyv::LightGreen,
                                  jevois::rawimage::Font10x20);
    }
    
#ifdef JEVOIS_PRO
    // If desired, draw results on GUI:
    if (helper)
    {
      int col = jevois::dnn::stringToRGBA(categ, 0xff);
      std::vector<cv::Point2f> corners; o.rect.points(corners);
      helper->drawPoly(corners, col, true);
      helper->drawText(corners[1].x + 3.0f, corners[1].y + 3.0f, label.c_str(), col);
    }
#else
    (void)helper; // keep compiler happy  
#endif   
    
    // If desired, send results to serial port:
    if (mod && serreport) mod->sendSerialObjDetImg2D(itsImageSize.width, itsImageSize.height, o);
  }
}

// ####################################################################################################
std::vector<jevois::ObjDetectOBB> const & jevois::dnn::PostProcessorDetectOBB::latestDetectionsOBB() const
{ return itsDetections; }
