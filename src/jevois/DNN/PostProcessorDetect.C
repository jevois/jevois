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
#include <opencv2/imgproc/imgproc.hpp> // for findContours()

// ####################################################################################################
jevois::dnn::PostProcessorDetect::~PostProcessorDetect()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::freeze(bool doit)
{
  classes::freeze(doit);
  detecttype::freeze(doit);
  if (itsYOLO) itsYOLO->freeze(doit);
  if (detecttype::get() != postprocessor::DetectType::YOLOv8seg &&
      detecttype::get() != postprocessor::DetectType::YOLOv8segt)
    masksmooth::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::onParamChange(postprocessor::classes const &, std::string const & val)
{
  if (val.empty()) { itsLabels.clear(); return; }
  itsLabels = jevois::dnn::readLabelsFile(jevois::absolutePath(JEVOIS_SHARE_PATH, val));
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::onParamChange(postprocessor::detecttype const &,
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
  bool const sigmo = sigmoid::get();
  bool const clampbox = boxclamp::get();
  int const fudge = classoffset::get();
  bool const smoothmsk = masksmooth::get();
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
  std::vector<cv::Mat> mask_coeffs; // mask coefficients when doing instance segmentation
  cv::Mat mask_proto; // The output containing the mask prototypes (usually the last one)
  int mask_proto_h = 1; // number of rows in the mask prototypes tensor, will be updated
  
  // Here we just scale the coords from [0..1]x[0..1] to blobw x blobh:
  try
  {
    switch(detecttype::get())
    {
      // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::FasterRCNN:
    {
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
        }
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::SSD:
    {
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
        }
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::TPUSSD:
    {
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
        }
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOv10:
    {
      for (size_t i = 0; i < outs.size(); ++i)
      {
        cv::Mat const & out = outs[i];
        cv::MatSize const & ms = out.size; int const nd = ms.dims();

        if (jevois::dnn::effectiveDims(out) != 2 || ms[nd-1] < 5)
          LTHROW("Expected 1 or more output blobs with shape Nx(4+C) where N is the number of "
                 "detected objects, C is the number of classes, and the first 4 columns are "
                 "[x1, y1, x2, y2]. // "
                 "Incorrect size " << jevois::dnn::shapestr(out) << " for output " << i <<
                 ": need Nx(4+C)");

        // Some networks may produce 3D, slice off the last 2 dims:
        int const nbox = ms[nd-2];
        int const ndata = ms[nd-1];
        int sz2[] = { nbox, ndata };
        cv::Mat const out2(2, sz2, out.type(), out.data);

        // Ok, we are ready with Nx(4+C):
        float const * data = (float const *)out2.data;
        for (int j = 0; j < nbox; ++j, data += ndata)
        {
          cv::Mat scores = out2.row(j).colRange(4, ndata);
          cv::Point classIdPoint; double confidence;
          cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);

          if (confidence < confThreshold) continue; // skip if class score too low

          // Boxes are already scaled by input blob size, and are x1, y1, x2, y2:
          boxes.push_back(cv::Rect(data[0], data[1], data[2]-data[0]+1, data[3]-data[1]+1));
          classIds.push_back(classIdPoint.x);
          confidences.push_back((float)confidence);
        }
      }
    }
    break;

    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOv10pp:
    {
      if (outs.size() != 1 || msiz.dims() != 3 || msiz[0] != 1 || msiz[2] != 6)
        LTHROW("Expected 1 output blob with shape 1xNx6 for N detections with values "
               "[left, top, right, bottom, confidence, classId]");
      
      float const * data = (float const *)out.data;
      for (size_t i = 0; i < out.total(); i += 6)
      {
        float confidence = data[i + 4];
        if (confidence > confThreshold)
        {
          // Boxes are already scaled by input blob size, and are x1, y1, x2, y2:
          int left = (int)data[i + 0];
          int top = (int)data[i + 1];
          int right = (int)data[i + 2];
          int bottom = (int)data[i + 3];
          int width = right - left + 1;
          int height = bottom - top + 1;
          classIds.push_back((int)(data[i + 5]) + fudge);  // Skip 0th background class id.
          boxes.push_back(cv::Rect(left, top, width, height));
          confidences.push_back(confidence);
        }
      }
    }
    break;
  
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::RAWYOLO:
    {
      if (itsYOLO) itsYOLO->yolo(outs, classIds, confidences, boxes, itsLabels.size(), boxThreshold, confThreshold,
                                 bsiz, fudge, maxnbox::get(), sigmo);
      else LFATAL("Internal error -- no YOLO subcomponent");
    }
    break;

    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOX:
    {
      if ((outs.size() % 3) != 0 || msiz.dims() != 4 || msiz[0] != 1)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 3 blobs: 1xHxWxC (class scores), 1xHxWx4 (boxes), "
               "1xHxWx1 (objectness scores)");

      int stride = 8;
      
      for (size_t idx = 0; idx < outs.size(); idx += 3)
      {
        cv::Mat const & cls = outs[idx]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xHxWxC");
        float const * cls_data = (float const *)cls.data;
        
        cv::Mat const & bx = outs[idx + 1]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[3] != 4) LTHROW("Output " << idx << " is not 1xHxWx4");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & obj = outs[idx + 2]; cv::MatSize const & obj_siz = obj.size;
        if (obj_siz.dims() != 4 || obj_siz[3] != 1) LTHROW("Output " << idx << " is not 1xHxWx1");
        float const * obj_data = (float const *)obj.data;
        
        for (int i = 1; i < 3; ++i)
          if (cls_siz[i] != bx_siz[i] || cls_siz[i] != obj_siz[i])
            LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 2);

        size_t const nclass = cls_siz[3];

        // Loop over all locations:
        for (int y = 0; y < cls_siz[1]; ++y)
          for (int x = 0; x < cls_siz[2]; ++x)
          {
            // Only consider if objectness score is high enough:
            float objectness = obj_data[0];
            if (objectness >= boxThreshold)
            {
              // Get the top class score:
              size_t best_idx = 0; float confidence = cls_data[0];
              for (size_t i = 1; i < nclass; ++i)
                if (cls_data[i] > confidence) { confidence = cls_data[i]; best_idx = i; }

              confidence *= objectness;

              if (confidence >= confThreshold)
              {
                // Decode the box:
                float cx = (x /*+ 0.5F*/ + bx_data[0]) * stride;
                float cy = (y /*+ 0.5F*/ + bx_data[1]) * stride;
                float width = std::exp(bx_data[2]) * stride;
                float height = std::exp(bx_data[3]) * stride;
                float left = cx - 0.5F * width;
                float top = cy - 0.5F * height;
                
                // Store this detection:
                boxes.push_back(cv::Rect(left, top, width, height));
                classIds.push_back(int(best_idx) + fudge);
                confidences.push_back(confidence);
              }
            }

            // Move to the next location:
            cls_data += nclass;
            bx_data += 4;
            obj_data += 1;
          }

        // Move to the next scale:
        stride *= 2;
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOv8t:
    {
      if ((outs.size() % 2) != 0 || msiz.dims() != 4 || msiz[0] != 1)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 2 blobs: 1xHxWx64 (raw boxes) "
               "and 1xHxWxC (class scores)");

      int stride = 8;
      int constexpr reg_max = 16;
      
      for (size_t idx = 0; idx < outs.size(); idx += 2)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[3] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1xHxWx64");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xHxWxC");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[3];
        
        for (int i = 1; i < 3; ++i)
          if (cls_siz[i] != bx_siz[i]) LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 1);

        // Loop over all locations:
        for (int y = 0; y < cls_siz[1]; ++y)
          for (int x = 0; x < cls_siz[2]; ++x)
          {
            // Get the top class score:
            size_t best_idx = 0; float confidence = cls_data[0];
            for (size_t i = 1; i < nclass; ++i)
              if (cls_data[i] > confidence) { confidence = cls_data[i]; best_idx = i; }

            // Apply sigmoid to it, if needed (output layer did not already have sigmoid activations):
            if (sigmo) confidence = jevois::dnn::sigmoid(confidence);
            
            if (confidence >= confThreshold)
            {
              // Decode a 4-coord box from 64 received values:
              // Code here inspired from https://github.com/trinhtuanvubk/yolo-ncnn-cpp/blob/main/yolov8/yolov8.cpp
              float dst[reg_max];

              float xmin = (x + 0.5f - softmax_dfl(bx_data, dst, reg_max)) * stride;
              float ymin = (y + 0.5f - softmax_dfl(bx_data + reg_max, dst, reg_max)) * stride;
              float xmax = (x + 0.5f + softmax_dfl(bx_data + 2 * reg_max, dst, reg_max)) * stride;
              float ymax = (y + 0.5f + softmax_dfl(bx_data + 3 * reg_max, dst, reg_max)) * stride;

              // Store this detection:
              boxes.push_back(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin));
              classIds.push_back(int(best_idx) + fudge);
              confidences.push_back(confidence);
            }

            // Move to the next location:
            cls_data += nclass;
            bx_data += 4 * reg_max;
          }

        // Move to the next scale:
        stride *= 2;
      }
    }
    break;

    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOv8:
    {
      if ((outs.size() % 2) != 0 || msiz.dims() != 4 || msiz[0] != 1)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 2 blobs: 1x64xHxW (raw boxes) "
               "and 1xCxHxW (class scores)");

      int stride = 8;
      int constexpr reg_max = 16;
      
      for (size_t idx = 0; idx < outs.size(); idx += 2)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[1] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1x64xHxW");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xCxHxW");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[1];
        
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
              // Decode a 4-coord box from 64 received values:
              // Code here inspired from https://github.com/trinhtuanvubk/yolo-ncnn-cpp/blob/main/yolov8/yolov8.cpp
              float dst[reg_max];

              float xmin = (x + 0.5f - softmax_dfl(bx_data, dst, reg_max, step)) * stride;
              float ymin = (y + 0.5f - softmax_dfl(bx_data + reg_max * step, dst, reg_max, step)) * stride;
              float xmax = (x + 0.5f + softmax_dfl(bx_data + 2 * reg_max * step, dst, reg_max, step)) * stride;
              float ymax = (y + 0.5f + softmax_dfl(bx_data + 3 * reg_max * step, dst, reg_max, step)) * stride;

              // Store this detection:
              boxes.push_back(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin));
              classIds.push_back(int(best_idx) + fudge);
              confidences.push_back(confidence);
            }

            // Move to the next location:
            ++cls_data;
            ++bx_data;
          }

        // Move to the next scale:
        stride *= 2;
      }
    }
    break;
 
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOv8seg:
    {
      if (outs.size() % 3 != 1 || msiz.dims() != 4 || msiz[0] != 1)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 3 tensors: 1x64xHxW (raw boxes), "
               "1xCxHxW (class scores), and 1xMxHxW (mask coeffs for M masks); then one 1xMxHxW for M mask prototypes");

      int stride = 8;
      int constexpr reg_max = 16;

      // Get the mask prototypes as 2D 32xHW:
      cv::MatSize const & mps = outs.back().size;
      if (mps.dims() != 4) LTHROW("Mask prototypes not 4D 1xMxHxW");
      mask_proto = cv::Mat(std::vector<int>{ mps[1], mps[2] * mps[3] }, CV_32F, outs.back().data);
      int const mask_num = mps[1];
      mask_proto_h = mps[2]; // will be needed later to unpack from HW to HxW
      
      // Process each scale (aka stride):
      for (size_t idx = 0; idx < outs.size() - 1; idx += 3)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[1] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1x64xHxW");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xCxHxW");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[1];
        
        cv::Mat const & msk = outs[idx + 2]; cv::MatSize const & msk_siz = msk.size;
        if (msk_siz.dims() != 4 || msk_siz[1] != mask_num) LTHROW("Output " << idx << " is not 4D 1xMxHxW");
        float const * msk_data = (float const *)msk.data;
        
        for (int i = 2; i < 4; ++i)
          if (cls_siz[i] != bx_siz[i] || cls_siz[i] != msk_siz[i])
            LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 1);

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
              // Decode a 4-coord box from 64 received values:
              float dst[reg_max];

              float xmin = (x + 0.5f - softmax_dfl(bx_data, dst, reg_max, step)) * stride;
              float ymin = (y + 0.5f - softmax_dfl(bx_data + reg_max * step, dst, reg_max, step)) * stride;
              float xmax = (x + 0.5f + softmax_dfl(bx_data + 2 * reg_max * step, dst, reg_max, step)) * stride;
              float ymax = (y + 0.5f + softmax_dfl(bx_data + 3 * reg_max * step, dst, reg_max, step)) * stride;

              // Store this detection:
              boxes.push_back(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin));
              classIds.push_back(int(best_idx) + fudge);
              confidences.push_back(confidence);

              // Also store raw mask coefficients data, will decode the masks after NMS to save time:
              cv::Mat coeffs(1, mask_num, CV_32F); float * cptr = (float *)coeffs.data;
              for (int i = 0; i < mask_num; ++i) *cptr++ = msk_data[i * step];
              mask_coeffs.emplace_back(coeffs);
            }

            // Move to the next location:
            ++cls_data; ++bx_data; ++msk_data;
          }

        // Move to the next scale:
        stride *= 2;
      }
    }
    break;

    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::DetectType::YOLOv8segt:
    {
      if (outs.size() % 3 != 1 || msiz.dims() != 4 || msiz[0] != 1)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 3 tensors: 1xHxWx64 (raw boxes), "
               "1xHxWxC (class scores), and 1xHxWxM (mask coeffs for M masks); then one 1xHxWxM for M mask prototypes");

      int stride = 8;
      int constexpr reg_max = 16;

      // Get the mask prototypes as 2D HWx32:
      cv::MatSize const & mps = outs.back().size;
      if (mps.dims() != 4) LTHROW("Mask prototypes not 4D 1xHxWxM");
      mask_proto = cv::Mat(std::vector<int>{ mps[1] * mps[2], mps[3] }, CV_32F, outs.back().data);
      int const mask_num = mps[3];
      mask_proto_h = mps[1]; // will be needed later to unpack from HW to HxW
      
      // Process each scale (aka stride):
      for (size_t idx = 0; idx < outs.size() - 1; idx += 3)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[3] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1xHxWx64");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xHxWxC");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[3];
        
        cv::Mat const & msk = outs[idx + 2]; cv::MatSize const & msk_siz = msk.size;
        if (msk_siz.dims() != 4 || msk_siz[3] != mask_num) LTHROW("Output " << idx << " is not 4D 1xHxWxM");
        float const * msk_data = (float const *)msk.data;
        
        for (int i = 1; i < 3; ++i)
          if (cls_siz[i] != bx_siz[i] || cls_siz[i] != msk_siz[i])
            LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 1);

        // Loop over all locations:
        for (int y = 0; y < cls_siz[1]; ++y)
          for (int x = 0; x < cls_siz[2]; ++x)
          {
            // Get the top class score:
            size_t best_idx = 0; float confidence = cls_data[0];
            for (size_t i = 1; i < nclass; ++i)
              if (cls_data[i] > confidence) { confidence = cls_data[i]; best_idx = i; }

            // Apply sigmoid to it, if needed (output layer did not already have sigmoid activations):
            if (sigmo) confidence = jevois::dnn::sigmoid(confidence);
            
            if (confidence >= confThreshold)
            {
              // Decode a 4-coord box from 64 received values:
              float dst[reg_max];

              float xmin = (x + 0.5f - softmax_dfl(bx_data, dst, reg_max)) * stride;
              float ymin = (y + 0.5f - softmax_dfl(bx_data + reg_max, dst, reg_max)) * stride;
              float xmax = (x + 0.5f + softmax_dfl(bx_data + 2 * reg_max, dst, reg_max)) * stride;
              float ymax = (y + 0.5f + softmax_dfl(bx_data + 3 * reg_max, dst, reg_max)) * stride;

              // Store this detection:
              boxes.push_back(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin));
              classIds.push_back(int(best_idx) + fudge);
              confidences.push_back(confidence);

              // Also store raw mask coefficients data, will decode the masks after NMS to save time:
              cv::Mat coeffs(mask_num, 1, CV_32F);
              std::memcpy(coeffs.data, msk_data, mask_num * sizeof(float));
              mask_coeffs.emplace_back(coeffs);
            }

            // Move to the next location:
            cls_data += nclass;
            bx_data += 4 * reg_max;
            msk_data += mask_num;
          }

        // Move to the next scale:
        stride *= 2;
      }
    }
    break;
 
    // ----------------------------------------------------------------------------------------------------
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

  // Cleanup overlapping boxes, either globally or per class, and possibly limit number of reported boxes:
  std::vector<int> indices;
  if (nmsperclass::get())
    cv::dnn::NMSBoxesBatched(boxes, confidences, classIds, confThreshold, nmsThreshold, indices, 1.0F, maxnbox::get());
  else
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices, 1.0F, maxnbox::get());

  // Store results:
  itsDetections.clear(); bool namonly = namedonly::get();
  std::vector<cv::Vec4i> contour_hierarchy;

  for (size_t i = 0; i < indices.size(); ++i)
  {
    int idx = indices[i];
    std::string const label = jevois::dnn::getLabel(itsLabels, classIds[idx], namonly);
    if (namonly == false || label.empty() == false)
    {
      cv::Rect & b = boxes[idx];

      // Now clamp box to be within blob:
      if (clampbox) jevois::dnn::clamp(b, bsiz.width, bsiz.height);

      // Decode the mask if doing instance segmentation:
      std::vector<cv::Point> poly;
      if (mask_coeffs.empty() == false)
      {
        // Multiply the 1x32 mask coeffs by the 32xHW mask prototypes to get a 1xHW weighted mask (YOLOv8seg), or
        // multiply the HWx32 mask prototypes by the 32x1 mask coeffs to get a HWx1 weighted mask (YOLOv8segt):
        cv::Mat weighted_mask;
        if (mask_coeffs[idx].rows == 1) weighted_mask = mask_coeffs[idx] * mask_proto;
        else weighted_mask = mask_proto * mask_coeffs[idx];

        // Reshape to HxW:
        weighted_mask = weighted_mask.reshape(0, mask_proto_h);
        
        // Apply sigmoid to all mask elements:
        jevois::dnn::sigmoid(weighted_mask);
        
        // Typically, mask prototypes are 4x smaller than input blob; we want to detect contours inside the obj rect. We
        // have two approaches here: 1) detect contours on the original masks at low resolution (faster but contours are
        // not very smooth), 2) scale the mask 4x with bilinear interpolation and then detect the contours (slower but
        // smoother contours):
        int mask_scale = bsiz.height / mask_proto_h;
        if (smoothmsk)
        {
          cv::Mat src = weighted_mask;
          cv::resize(src, weighted_mask, cv::Size(), mask_scale, mask_scale, cv::INTER_LINEAR);
          mask_scale = 1;
        }

        cv::Rect scaled_rect(b.tl() / mask_scale, b.br() / mask_scale);
        scaled_rect &= cv::Rect(cv::Point(0, 0), weighted_mask.size()); // constrain roi to within mask image

        // Binarize the mask roi:
        cv::Mat roi_mask; cv::threshold(weighted_mask(scaled_rect), roi_mask, 0.5, 255.0, cv::THRESH_BINARY);
        cv::Mat roi_u8; roi_mask.convertTo(roi_u8, CV_8U);

        // Detect object contours that are inside the scaled rect:
        std::vector<std::vector<cv::Point>> polys;
        cv::findContours(roi_u8, polys, contour_hierarchy, cv::RETR_EXTERNAL,
                         cv::CHAIN_APPROX_SIMPLE, scaled_rect.tl()); // or CHAIN_APPROX_NONE

        // Pick the largest poly:
        size_t polyidx = 0; size_t largest_poly_size = 0; size_t j = 0;
        for (auto const & p : polys)
        {
          if (p.size() > largest_poly_size) { largest_poly_size = p.size(); polyidx = j; }
          ++j;
        }
        
        // Scale from mask to blob to image:
        if (polys.empty() == false)
          for (cv::Point & pt : polys[polyidx])
          {
            float x = pt.x * mask_scale, y = pt.y * mask_scale;
            preproc->b2i(x, y);
            poly.emplace_back(cv::Point(x, y));
          }
      }

      // Rescale the box from blob to (processing) image:
      cv::Point2f tl = b.tl(); preproc->b2i(tl.x, tl.y);
      cv::Point2f br = b.br(); preproc->b2i(br.x, br.y);
      b.x = tl.x; b.y = tl.y; b.width = br.x - tl.x; b.height = br.y - tl.y;

      // Store this detection for later report:
      jevois::ObjReco o { confidences[idx] * 100.0f, label };
      std::vector<jevois::ObjReco> ov;
      ov.emplace_back(o);
      jevois::ObjDetect od { b.x, b.y, b.x + b.width, b.y + b.height, ov, poly };
      itsDetections.emplace_back(od);
    }
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetect::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                              jevois::OptGUIhelper * helper, bool overlay,
                                              bool /*idle*/)
{
  bool const serreport = serialreport::get();
  
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
      if (o.contour.empty() == false) LERROR("Need to implement drawPoly() for RawImage");
      jevois::rawimage::writeText(*outimg, label, o.tlx + 6, o.tly + 2, jevois::yuyv::LightGreen,
                                  jevois::rawimage::Font10x20);
    }
    
#ifdef JEVOIS_PRO
    // If desired, draw results on GUI:
    if (helper)
    {
      int col = jevois::dnn::stringToRGBA(categ, 0xff);
      helper->drawRect(o.tlx, o.tly, o.brx, o.bry, col, true);
      if (o.contour.empty() == false) helper->drawPoly(o.contour, col, false);
      helper->drawText(o.tlx + 3.0f, o.tly + 3.0f, label.c_str(), col);
    }
#else
    (void)helper; // keep compiler happy  
#endif   
    
    // If desired, send results to serial port:
    if (mod && serreport) mod->sendSerialObjDetImg2D(itsImageSize.width, itsImageSize.height, o);
  }
}

// ####################################################################################################
std::vector<jevois::ObjDetect> const & jevois::dnn::PostProcessorDetect::latestDetections() const
{ return itsDetections; }
