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
#include <jevois/DNN/PostProcessNPUhelpers.H>
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
  anchors::freeze(doit);
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
void jevois::dnn::PostProcessorDetect::onParamChange(postprocessor::anchors const & JEVOIS_UNUSED_PARAM(param),
                                                     std::string const & val)
{
  if (val.empty()) { itsAnchors.clear(); return; }
  auto tok = jevois::split(val, "\\s*;\\s*");
  if (tok.size() >= 64) LFATAL("Maximum 32 anchors is supported");
  for (std::string const & t : tok)
  {
    std::array<float, 64> a { };
    auto atok = jevois::split(t, "\\s*,\\s*");
    int i = 0;
    for (std::string const & at : atok) a[i++] = std::stoi(at);
    itsAnchors.emplace_back(std::move(a));
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  float const confThreshold = thresh::get() * 0.01F;
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

  // Here we just scale the coords from [0..1]x[0..1] to blobw x blobh:
  switch(detecttype::get())
  {
    // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::FasterRCNN:
  {
    // Network produces output blob with a shape 1x1xNx7 where N is a number of detections and an every detection is
    // a vector of values [batchId, classId, confidence, left, top, right, bottom]
    if (outs.size() != 1) LFATAL("Malformed output layers");
    cv::Mat const & out = outs[0]; cv::MatSize const & msiz = out.size;
    if (msiz.dims() != 4 || msiz[0] != 1 || msiz[1] != 1 || msiz[3] != 7) LFATAL("Incorrect tensor size: need 1x1xNx7");
    
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
      }
    }
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::SSD:
  {
    // Network produces output blob with a shape 1x1xNx7 where N is a number of detections and an every detection is
    // a vector of values [batchId, classId, confidence, left, top, right, bottom]
    if (outs.size() != 1) LFATAL("Malformed output layers");
    cv::Mat const & out = outs[0]; cv::MatSize msiz = out.size;
    if (msiz.dims() != 4 || msiz[0] != 1 || msiz[1] != 1 || msiz[3] != 7) LFATAL("Incorrect tensor size: need 1x1xNx7");

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
      }
    }
  }
  break;

  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::TPUSSD:
  {
    // Network produces 4 output blobs with shapes 4xN for boxes, N for IDs, N for scores, and 1x1 for count
    // (see GetDetectionResults in detection/adapter.cc of libcoral):
    if (outs.size() != 4) LFATAL("Malformed output layers");
    cv::Mat const & bboxes = outs[0];
    cv::Mat const & ids = outs[1];
    cv::Mat const & scores = outs[2];
    cv::Mat const & count = outs[3];
    if (bboxes.total() != 4 * ids.total()) LFATAL("Incorrect bbox vs ids sizes");
    if (bboxes.total() != 4 * scores.total()) LFATAL("Incorrect bbox vs scores sizes");
    if (count.total() != 1) LFATAL("Incorrect size for count");
    size_t num = count.at<float>(0);
    if (num > ids.total()) LFATAL("Too many detections: " << num << " for only " << ids.total() << " ids");
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
    }
  }
  break;

  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::YOLO:
  {
    for (size_t i = 0; i < outs.size(); ++i)
    {
      // Network produces output blob with a shape NxC where N is a number of detected objects and C is a number of
      // classes + 4 where the first 4 numbers are [center_x, center_y, width, height]
      cv::Mat const & out = outs[i];
      if (out.size.dims() != 2) LFATAL("Incorrect tensor size: need NxC");

      float const * data = (float const *)out.data;
      for (int j = 0; j < out.rows; ++j, data += out.cols)
      {
        cv::Mat scores = out.row(j).colRange(5, out.cols);
        cv::Point classIdPoint;
        double confidence;
        cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
        if (confidence > confThreshold)
        {
          int centerX = (int)(data[0] * bsiz.width);
          int centerY = (int)(data[1] * bsiz.height);
          int width = (int)(data[2] * bsiz.width);
          int height = (int)(data[3] * bsiz.height);
          int left = centerX - width / 2;
          int top = centerY - height / 2;
          
          classIds.push_back(classIdPoint.x);
          confidences.push_back((float)confidence);
          boxes.push_back(cv::Rect(left, top, width, height));
        }
      }
    }
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::RAWYOLOface:
  {
    if (outs.size() != 1) LFATAL("Expected 1 output tensor but received " << outs.size());
    static float const defaultbiases[10] {1.08*8,1.19*8, 3.42*8,4.41*8, 6.63*8,11.38*8, 9.42*8,5.11*8, 16.62*8,10.52*8};
    float const * biases = itsAnchors.size() >= 1 ? itsAnchors[0].data() : defaultbiases;
    jevois::dnn::npu::yolo(outs[0], classIds, confidences, boxes, itsLabels.size(), biases, 0,
                           confThreshold, bsiz, fudge);
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::RAWYOLOv2:
  {
    if (outs.size() != 1) LFATAL("Expected 1 output tensor but received " << outs.size());
    static float const defaultbiases[10] { 0.738768*8,0.874946*8,2.422040*8,2.657040*8,4.309710*8,
        7.044930*8,10.246000*8,4.594280*8,12.686800*8,11.874100*8 };
    float const * biases = itsAnchors.size() >= 1 ? itsAnchors[0].data() : defaultbiases;
    // Myriad-X model gives [1, 21125], reshape to [5, 25, 13, 13] for VOC or
    // [1,71825] for COCO with 8 classes
    if (outs[0].size.dims() == 2)
    {
      int const n = outs[0].size[1] / (13 * 13);
      cv::Mat o = outs[0].reshape(0, { 1, n, 13, 13 });
      jevois::dnn::npu::yolo(o, classIds, confidences, boxes, itsLabels.size(), biases, 0,
                             confThreshold, bsiz, fudge);
    }
    else
      jevois::dnn::npu::yolo(outs[0], classIds, confidences, boxes, itsLabels.size(), biases, 0,
                             confThreshold, bsiz, fudge);
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::RAWYOLOv3:
  case jevois::dnn::postprocessor::DetectType::RAWYOLOv4:
  {
    if (outs.size() != 3) LFATAL("Expected 3 output tensors but received " << outs.size());
    static float const defaultbiases[18] {10, 13, 16, 30, 33, 23, 30, 61, 62, 45,
        59, 119, 116, 90, 156, 198, 373, 326};
    float const * b0 = itsAnchors.size() >= 1 ? itsAnchors[0].data() : defaultbiases;
    float const * b1 = itsAnchors.size() >= 2 ? itsAnchors[1].data() : b0;
    float const * b2 = itsAnchors.size() >= 3 ? itsAnchors[2].data() : b1;
    jevois::dnn::npu::yolo(outs[0], classIds, confidences, boxes, itsLabels.size(), b2, 2,
                           confThreshold, bsiz, fudge);
    jevois::dnn::npu::yolo(outs[1], classIds, confidences, boxes, itsLabels.size(), b1, 1,
                           confThreshold, bsiz, fudge);
    jevois::dnn::npu::yolo(outs[2], classIds, confidences, boxes, itsLabels.size(), b0, 0,
                           confThreshold, bsiz, fudge);
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
  case jevois::dnn::postprocessor::DetectType::RAWYOLOv3tiny:
  {
    if (outs.size() != 2) LFATAL("Expected 2 output tensors but received " << outs.size());
    static float const defaultbiases[12] {10, 14, 23, 27, 37, 58, 81, 82, 135, 169, 344, 319};
    float const * b0 = itsAnchors.size() >= 1 ? itsAnchors[0].data() : defaultbiases;
    float const * b1 = itsAnchors.size() >= 2 ? itsAnchors[1].data() : b0;
    jevois::dnn::npu::yolo(outs[0], classIds, confidences, boxes, itsLabels.size(), b1, 1,
                           confThreshold, bsiz, fudge);
    jevois::dnn::npu::yolo(outs[1], classIds, confidences, boxes, itsLabels.size(), b0, 0,
                           confThreshold, bsiz, fudge);
  }
  break;
   
  default:
    LFATAL("Post-processor detecttype " << detecttype::strget() << " not available on this hardware");
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
