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

#include <jevois/DNN/PostProcessorDetectYOLO.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Async.H>
#include <jevois/DNN/Utils.H>
#include <nn_detect_common.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <future>


// ####################################################################################################
void jevois::dnn::PostProcessorDetectYOLO::onParamChange(postprocessor::anchors const & JEVOIS_UNUSED_PARAM(param),
                                                         std::string const & val)
{
  itsAnchors.clear();
  if (val.empty()) return;
  
  auto tok = jevois::split(val, "\\s*;\\s*");
  for (std::string const & t : tok)
  {
    auto atok = jevois::split(t, "\\s*,\\s*");
    if (atok.size() & 1) LFATAL("Odd number of values not allowed in anchor spec [" << t << ']');
    std::vector<float> a;
    for (std::string const & at : atok) a.emplace_back(std::stof(at));
    itsAnchors.emplace_back(std::move(a));
  }
}

// ####################################################################################################
jevois::dnn::PostProcessorDetectYOLO::~PostProcessorDetectYOLO()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorDetectYOLO::freeze(bool doit)
{
  anchors::freeze(doit);
}

// ####################################################################################################
// Helper code from the detect_library of the NPU
namespace
{
  inline float logistic_activate(float x)
  { return 1.0F/(1.0F + expf(-x)); }
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetectYOLO::yolo(std::vector<cv::Mat> const & outs, std::vector<int> & classIds,
                                                std::vector<float> & confidences, std::vector<cv::Rect> & boxes,
                                                size_t nclass, float boxThreshold, float confThreshold,
                                                cv::Size const & bsiz, int fudge, size_t const maxbox)
{
  if (nclass == 0) nclass = 1; // Assume 1 class if no list of classes was given
  size_t const nouts = outs.size();
  if (nouts == 0) LTHROW("No output tensors received");
  if (itsAnchors.size() != nouts) LTHROW("Need " << nouts << " sets of anchors");

  // Various networks will yield their YOLO outputs in various orders. But our default anchors (and the doc for the
  // anchors parameter) assumes order from large to small, e.g., first 52x52, then 26x26, then 13x13. So here we need to
  // sort the outputs in decreasing size order to get the correct yolonum:
  if (itsYoloNum.empty())
  {
    for (size_t i = 0; i < nouts; ++i) itsYoloNum.emplace_back(i);
    std::sort(itsYoloNum.begin(), itsYoloNum.end(),
              [&outs](int const & a, int const & b) { return outs[a].total() > outs[b].total(); });

    // Allow users to check our assignment:
    for (size_t i = 0; i < nouts; ++i)
    {
      int const yn = itsYoloNum[i];
      std::vector<float> const & anc = itsAnchors[yn];
      std::string vstr;
      for (size_t a = 0; a < anc.size(); a += 2) vstr += jevois::sformat("%.2f,%.2f ", anc[a], anc[a+1]);
      LINFO("Out " << i << ": " << jevois::dnn::shapestr(outs[i]) << ", scale=1/" << (8<<yn) <<
            ", anchors=[ " << vstr <<']');
    }
  }
  
  // Run each scale in a thread:
  bool sigmo = sigmoid::get();
  float scale_xy = scalexy::get();
  std::vector<std::future<void>> fvec;
  
  for (size_t i = 0; i < nouts; ++i)
    fvec.emplace_back(jevois::async([&](size_t i)
      { yolo_one(outs[i], classIds, confidences, boxes, nclass, itsYoloNum[i], boxThreshold, confThreshold,
                 bsiz, fudge, maxbox, sigmo, scale_xy); }, i));

  // Use joinall() to get() all futures and throw a single consolidated exception if any thread threw:
  jevois::joinall(fvec);
}

// ####################################################################################################
void jevois::dnn::PostProcessorDetectYOLO::yolo_one(cv::Mat const & out, std::vector<int> & classIds,
                                                    std::vector<float> & confidences, std::vector<cv::Rect> & boxes,
                                                    size_t nclass, int yolonum, float boxThreshold,
                                                    float confThreshold, cv::Size const & bsiz, int fudge,
                                                    size_t maxbox, bool sigmo, float scale_xy)
{
  if (out.type() != CV_32F) LTHROW("Need FLOAT32 data");
  cv::MatSize const & msiz = out.size;
  if (msiz.dims() != 4 || msiz[0] != 1)
    LTHROW("Incorrect tensor size: need 1xCxHxW or 1xHxWxC, got " << jevois::dnn::shapestr(out));
  
  // C=(dim[1] or dims[3]) is (coords = 4 + 1 for box score + classes) * n_anchors:
  // n_anchors = 5 for yoloface, yolov2
  // n_anchors = 3 for yolov3/v4/v5/v7 and those have 3 separate output tensors for 3 scales

  // Try NCHW first (e.g., from NPU):
  bool nchw = true;
  int w = msiz[3];
  int h = msiz[2];
  int constexpr coords = 4;
  int const bbsize = coords + 1 + nclass;
  int n = msiz[1] / bbsize;
  if (msiz[1] % bbsize)
  {
    // Ok, try NHWC (e.g., YOLOv5 on Hailo):
    nchw = false;
    w = msiz[2];
    h = msiz[1];
    n = msiz[3] / bbsize;

    if (msiz[3] % bbsize)
      LTHROW("Incorrect tensor size: need 1xCxHxW or 1xHxWxC where "
             "C=num_anchors*(4 coords + 1 box_score + nclass object_scores), got " << jevois::dnn::shapestr(out) <<
             ", nclass=" << nclass << ", num_anchors=" << itsAnchors[yolonum].size()/2);
  }
  
  float const bfac = 1.0F / (8 << yolonum);
  size_t const total = h * w * n * bbsize;
  if (total != out.total()) LTHROW("Ooops");
  std::vector<float> const & biases = itsAnchors[yolonum];
  if (int(biases.size()) != n*2)
    LTHROW(n << " boxes received but only " << biases.size()/2 << " boxw,boxh anchors provided");

  // Stride from one box field (coords, score, class) to the next:
  size_t const stride = nchw ? h * w : 1;
  size_t const nextloc = nchw ? 1 : n * bbsize;
  float const * locptr = (float const *)out.data;
  size_t const ncs = nclass * stride;

  // Loop over all locations:
  for (int row = 0; row < h; ++row)
    for (int col = 0; col < w; ++col)
    {
      // locptr points to the set of boxes at the current location. Initialize ptr to the first box:
      float const * ptr = locptr;
      
      // Loop over all boxes per location:
      for (int nn = 0; nn < n; ++nn)
      {
        // Apply logistic activation to box score:
        float box_score = ptr[coords * stride];
        if (sigmo) box_score = logistic_activate(box_score);
        
        if (box_score > boxThreshold)
        {
          // Get index of highest-scoring class and its score:
          size_t const class_index = (coords + 1) * stride;
          size_t maxidx = 0; float prob = 0.0F;
          for (size_t k = 0; k < ncs; k += stride)
            if (ptr[class_index + k] > prob) { prob = ptr[class_index + k]; maxidx = k; }
          if (sigmo) prob = logistic_activate(prob);

          // Combine box and class scores:
          prob *= box_score;

          // If best class was above threshold, keep that box:
          if (prob > confThreshold)
          {
            // Decode the box and scale it to input blob dims:
            cv::Rect b;

            if (scale_xy)
            {
              // New coordinates style, as in YOLOv5/7:
              float bx = ptr[0 * stride], by = ptr[1 * stride], bw = ptr[2 * stride], bh = ptr[3 * stride];
              if (sigmo)
              {
                bx = logistic_activate(bx);
                by = logistic_activate(by);
                bw = logistic_activate(bw);
                bh = logistic_activate(bh);
              }
              
              b.width = bw * bw * 4.0f * biases[2*nn] * bfac * bsiz.width / w + 0.499F;
              b.height = bh * bh * 4.0F * biases[2*nn+1] * bfac * bsiz.height / h + 0.499F;
              b.x = (bx * scale_xy - 0.5F + col) * bsiz.width / w + 0.499F - b.width / 2;
              b.y = (by * scale_xy - 0.5F + row) * bsiz.height / h + 0.499F - b.height / 2;
            }
            else
            {
              // Old-style coordinates, as in YOLOv2/3/4:
              b.width = expf(ptr[2 * stride]) * biases[2*nn] * bfac * bsiz.width / w + 0.499F;
              b.height = expf(ptr[3 * stride]) * biases[2*nn+1] * bfac * bsiz.height / h + 0.499F;
              b.x = (col + logistic_activate(ptr[0 * stride])) * bsiz.width / w + 0.499F - b.width / 2;
              b.y = (row + logistic_activate(ptr[1 * stride])) * bsiz.height / h + 0.499F - b.height / 2;
            }
            
            std::lock_guard<std::mutex> _(itsOutMtx);
            boxes.emplace_back(b);
            classIds.emplace_back(maxidx / stride + fudge);
            confidences.emplace_back(prob);
            if (classIds.size() > maxbox) return; // Stop if too many boxes
          }
        }

        // Next box within the current location:
        ptr += bbsize * stride;
      }
      // Next location:
      locptr += nextloc;
    }
}
