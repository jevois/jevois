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

#include <jevois/DNN/PostProcessorDetect.H>
#include <jevois/DNN/PostProcessorDetectYOLO.H>
#include <jevois/DNN/PreProcessor.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>

#include <opencv2/dnn.hpp>

// ####################################################################################################
jevois::dnn::PostProcessorDetect::~PostProcessorDetect()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::freeze(bool doit)
{
  classes::freeze(doit);
  detecttype::freeze(doit);
  if (itsYOLO) itsYOLO->freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::onParamChange(postprocessor::classes const & JEVOIS_UNUSED_PARAM(param),
                                                     std::string const & val)
{
  if (val.empty()) { itsLabels.clear(); return; }

  // Get the dataroot of our network. We assume that there is a sub-component named "network" that is a sibling of us:
  std::vector<std::string> dd = jevois::split(Component::descriptor(), ":");
  dd.back() = "network"; dd.emplace_back("dataroot");
  std::string const dataroot = engine()->getParamStringUnique(jevois::join(dd, ":"));

  itsLabels = jevois::dnn::readLabelsFile(jevois::absolutePath(dataroot, val));
}
// ####################################################################################################
void jevois::dnn::PostProcessorDetect::onParamChange(postprocessor::detecttype const & JEVOIS_UNUSED_PARAM(param),
                                                     postprocessor::DetectType const & val)
{
  if (val == postprocessor::DetectType::RAWYOLO)
    itsYOLO = addSubComponent<jevois::dnn::PostProcessorDetectYOLO>("yolo");
  else
  {
    itsYOLO.reset();
    removeSubComponent("yolo", false);
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  if (outs.empty()) LFATAL("No outputs received, we need at least one.");
  cv::Mat const & out = outs[0]; cv::MatSize const & msiz = out.size;

  float const confThreshold = cthresh::get() * 0.01F;
  float const boxThreshold = dthresh::get() * 0.01F;
  float const nmsThreshold = nms::get() * 0.01F;
  int const fudge = classoffset::get();
  itsImageSize = preproc->imagesize();
  
  // To draw boxes, we will need to:
  // - scale from [0..1]x[0..1] to blobw x blobh
  // - scale and center from blobw x blobh to input image w x h, provided by PreProcessor::b2i()
  // - when using the GUI, we further scale and translate to OpenGL display coordinates using GUIhelper::i2d()
  // Here we assume that the first blob sets the input size.
  cv::Size const bsiz = preproc->blobsize(0);
  
  // We keep 3 vectors here instead of creating a class to hold all of the data because OpenCV will need that for
  // non-maximum suppression:
  std::vector<int> classIds;
  std::vector<float> confidences;
  std::vector<cv::Rect> boxes;
  size_t const maxbox = maxnbox::get();

  // Here we just scale the coords from [0..1]x[0..1] to blobw x blobh:
  try
  {
    switch(detecttype::get())
    {
      // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::FasterRCNN:
    {
      // Network produces output blob with a shape 1x1xNx7 where N is a number of detections and an every detection is
      // a vector of values [batchId, classId, confidence, left, top, right, bottom]
      if (outs.size() != 1 || msiz.dims() != 4 || msiz[0] != 1 || msiz[1] != 1 || msiz[3] != 7)
        LTHROW("Expected 1 output blob with shape 1x1xNx7 for N detections with values "
               "[batchId, classId, confidence, left, top, right, bottom]");
      
      float const * data = (float const *)out.data;
      for (size_t i = 0; i < out.total(); i += 7)
      {
        float confidence = data[i + 2];
        if (confidence > confThreshold)
        {
          int left = (int)data[i + 3];
          int top = (int)data[i + 4];
          int right = (int)data[i + 5];
          int bottom = (int)data[i + 6];
          int width = right - left + 1;
          int height = bottom - top + 1;
          classIds.push_back((int)(data[i + 1]) + fudge);  // Skip 0th background class id.
          boxes.push_back(cv::Rect(left, top, width, height));
          confidences.push_back(confidence);
          if (classIds.size() > maxbox) break; // Stop if too many boxes
        }
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::SSD:
    {
      // Network produces output blob with a shape 1x1xNx7 where N is a number of detections and an every detection is
      // a vector of values [batchId, classId, confidence, left, top, right, bottom]
      if (outs.size() != 1 || msiz.dims() != 4 || msiz[0] != 1 || msiz[1] != 1 || msiz[3] != 7)
        LTHROW("Expected 1 output blob with shape 1x1xNx7 for N detections with values "
               "[batchId, classId, confidence, left, top, right, bottom]");
    
      float const * data = (float const *)out.data;
      for (size_t i = 0; i < out.total(); i += 7)
      {
        float confidence = data[i + 2];
        if (confidence > confThreshold)
        {
          int left = (int)(data[i + 3] * bsiz.width);
          int top = (int)(data[i + 4] * bsiz.height);
          int right = (int)(data[i + 5] * bsiz.width);
          int bottom = (int)(data[i + 6] * bsiz.height);
          int width = right - left + 1;
          int height = bottom - top + 1;
          classIds.push_back((int)(data[i + 1]) + fudge);  // Skip 0th background class id.
          boxes.push_back(cv::Rect(left, top, width, height));
          confidences.push_back(confidence);
          if (classIds.size() > maxbox) break; // Stop if too many boxes
        }
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::TPUSSD:
    {
      // Network produces 4 output blobs with shapes 4xN for boxes, N for IDs, N for scores, and 1x1 for count
      // (see GetDetectionResults in detection/adapter.cc of libcoral):
      if (outs.size() != 4)
        LTHROW("Expected 4 output blobs with shapes 4xN for boxes, N for IDs, N for scores, and 1x1 for count");
      cv::Mat const & bboxes = outs[0];
      cv::Mat const & ids = outs[1];
      cv::Mat const & scores = outs[2];
      cv::Mat const & count = outs[3];
      if (bboxes.total() != 4 * ids.total() || bboxes.total() != 4 * scores.total() || count.total() != 1)
        LTHROW("Expected 4 output blobs with shapes 4xN for boxes, N for IDs, N for scores, and 1x1 for count");

      size_t num = count.at<float>(0);
      if (num > ids.total()) LTHROW("Too many detections: " << num << " for only " << ids.total() << " ids");
      float const * bb = (float const *)bboxes.data;
      
      for (size_t i = 0; i < num; ++i)
      {
        if (scores.at<float>(i) < confThreshold) continue;
        
        int top = (int)(bb[4 * i] * bsiz.height);
        int left = (int)(bb[4 * i + 1] * bsiz.width);
        int bottom = (int)(bb[4 * i + 2] * bsiz.height);
        int right = (int)(bb[4 * i + 3] * bsiz.width);
        int width = right - left + 1;
        int height = bottom - top + 1;
        classIds.push_back((int)(ids.at<float>(i)) + fudge);  // Skip 0th background class id.
        boxes.push_back(cv::Rect(left, top, width, height));
        confidences.push_back(scores.at<float>(i));
        if (classIds.size() > maxbox) break; // Stop if too many boxes
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLO:
    {
      for (size_t i = 0; i < outs.size(); ++i)
      {
        // Network produces output blob(s) with shape Nx(5+C) where N is a number of detected objects and C is a number
        // of classes + 5 where the first 5 numbers are [center_x, center_y, width, height, box score].
        cv::Mat const & out = outs[i];
        cv::MatSize const & ms = out.size; int const nd = ms.dims();
        int nbox = -1, ndata = -1;
        
        if (nd >= 2)
        {
          nbox = ms[nd-2];
          ndata = ms[nd-1];
          for (int i = 0; i < nd-2; ++i) if (ms[i] != 1) nbox = -1; // reject if more than 2 effective dims
        }

        if (nbox < 0 || ndata < 5)
          LTHROW("Expected 1 or more output blobs with shape Nx(5+C) where N is the number of "
                 "detected objects, C is the number of classes, and the first 5 columns are "
                 "[center_x, center_y, width, height, box score]. // "
                 "Incorrect size " << jevois::dnn::shapestr(out) << " for output " << i <<
                 ": need Nx(5+C) or 1xNx(5+C)");

        // Some networks, like YOLOv5 or YOLOv7, output 3D 1xNx(5+C), so here we slice off the last 2 dims:
        int sz2[] = { nbox, ndata };
        cv::Mat const out2(2, sz2, out.type(), out.data);
        
        float const * data = (float const *)out2.data;
        for (int j = 0; j < nbox; ++j, data += ndata)
        {
          if (data[4] < boxThreshold) continue; // skip if box score is too low
          
          cv::Mat scores = out2.row(j).colRange(5, ndata);
          cv::Point classIdPoint; double confidence;
          cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);

          if (confidence < confThreshold) continue; // skip if class score too low

          // YOLO<5 produces boxes in [0..1[x[0..1[ and 2D output blob:
          int centerX, centerY, width, height;
          if (nd == 2)
          {
            centerX = (int)(data[0] * bsiz.width);
            centerY = (int)(data[1] * bsiz.height);
            width = (int)(data[2] * bsiz.width);
            height = (int)(data[3] * bsiz.height);
          }
          else
          {
            // YOLOv5, YOLOv7 produce boxes already scaled by input blob size, and 3D output blob:
            centerX = (int)(data[0]);
            centerY = (int)(data[1]);
            width = (int)(data[2]);
            height = (int)(data[3]);
          }
          
          int left = centerX - width / 2;
          int top = centerY - height / 2;
          boxes.push_back(cv::Rect(left, top, width, height));
          classIds.push_back(classIdPoint.x);
          confidences.push_back((float)confidence);
          if (classIds.size() > maxbox) break; // Stop if too many boxes
        }
      }
    }
    break;

    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::RAWYOLO:
    {
      if (itsYOLO) itsYOLO->yolo(outs, classIds, confidences, boxes, itsLabels.size(), boxThreshold, confThreshold,
                                 bsiz, fudge, maxbox);
      else LFATAL("Internal error -- no YOLO subcomponent");
    }
    break;
    
    default:
      // Do not use strget() here as it will throw!
      LTHROW("Unsupported Post-processor detecttype " << int(detecttype::get()));
    }
  }
  // Abort here if the received outputs were malformed:
  catch (std::exception const & e)
  {
    std::string err = "Selected detecttype is " + detecttype::strget() + " and network produced:\n\n";
    for (cv::Mat const & m : outs) err += "- " + jevois::dnn::shapestr(m) + "\n";
    err += "\nFATAL ERROR(s):\n\n";
    err += e.what();
    LFATAL(err);
  }

  // Cleanup overlapping boxes:
  std::vector<int> indices;
  cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices);

  // Now clamp boxes to be within blob, and adjust the boxes from blob size to input image size:
  for (cv::Rect & b : boxes)
  {
    jevois::dnn::clamp(b, bsiz.width, bsiz.height);

    cv::Point2f tl = b.tl(); preproc->b2i(tl.x, tl.y);
    cv::Point2f br = b.br(); preproc->b2i(br.x, br.y);
    b.x = tl.x; b.y = tl.y; b.width = br.x - tl.x; b.height = br.y - tl.y;
  }

  // Store results:
  itsDetections.clear();
  for (size_t i = 0; i < indices.size(); ++i)
  {
    int idx = indices[i];
    cv::Rect const & box = boxes[idx];
    jevois::ObjReco o {confidences[idx] * 100.0f, jevois::dnn::getLabel(itsLabels, classIds[idx]) };
    std::vector<jevois::ObjReco> ov;
    ov.emplace_back(o);
    jevois::ObjDetect od { box.x, box.y, box.x + box.width, box.y + box.height, ov };
    itsDetections.emplace_back(od);
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                              jevois::OptGUIhelper * helper, bool overlay,
                                              bool JEVOIS_UNUSED_PARAM(idle))
{
  for (jevois::ObjDetect const & o : itsDetections)
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
      jevois::rawimage::drawRect(*outimg, o.tlx, o.tly, o.brx - o.tlx, o.bry - o.tly, 2, jevois::yuyv::LightGreen);
      jevois::rawimage::writeText(*outimg, label, o.tlx + 6, o.tly + 2, jevois::yuyv::LightGreen,
                                  jevois::rawimage::Font10x20);
    }
    
#ifdef JEVOIS_PRO
    // If desired, draw results on GUI:
    if (helper)
    {
      int col = jevois::dnn::stringToRGBA(categ, 0xff);
      helper->drawRect(o.tlx, o.tly, o.brx, o.bry, col, true);
      helper->drawText(o.tlx + 3.0f, o.tly + 3.0f, label.c_str(), col);
    }
#else
      (void)helper; // keep compiler happy  
#endif   

    // If desired, send results to serial port:
    if (mod) mod->sendSerialObjDetImg2D(itsImageSize.width, itsImageSize.height, o);
  }
}
