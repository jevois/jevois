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

#include <jevois/DNN/PostProcessNPUhelpers.H>
#include <jevois/Debug/Log.H>

#include <nn_detect_common.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

// ####################################################################################################
// Helper code from the detect_library of the NPU
namespace
{
  inline float logistic_activate(float x)
  { return 1./(1. + exp(-x)); }

  void flatten(float const * src, float * dst, int size, int layers, int batch, int forward)
  {
    for (int b = 0; b < batch; ++b)
      for (int c = 0; c < layers; ++c)
        for (int i = 0; i < size; ++i)
        {
          int i1 = b*layers*size + c*size + i;
          int i2 = b*layers*size + i*layers + c;
          if (forward) dst[i2] = src[i1];
          else dst[i1] = src[i2];
        }
  }
} // anonymous namespace


// ####################################################################################################
void jevois::dnn::npu::yolo(cv::Mat const & out, std::vector<int> & classIds, std::vector<float> & confidences,
                            std::vector<cv::Rect> & boxes, size_t nclass, float const * biases, int const yolonum,
                            float confThreshold, cv::Size const & bsiz, int fudge)
{
  if (nclass == 0) nclass = 1; // Assume 1 class if no list of classes was given
  if (out.type() != CV_32F) LFATAL("Need FLOAT data");
  cv::MatSize const & msiz = out.size;
  if (msiz.dims() != 4 || msiz[0] != 1)
    LFATAL("Incorrect tensor size: need 1xNxHxW where N=anchors*(4(coords)+1(box score)+nclass(object scores)), got "<<
           jevois::dnn::shapestr(out));
  // dim[1] is (coords = 4 + 1 for box score + classes) * n boxes:
  // n = 5 for yoloface, yolov2
  // n = 3 for yolo_v3
  
  int const w = msiz[3];
  int const h = msiz[2];
  int const coords = 4;
  int bbsize = coords + 1 + nclass;
  int const n = msiz[1] / bbsize;
  if (msiz[1] % bbsize) LFATAL("Incorrect tensor size: need 1xNxHxW where N=anchors*(4(coords)+1(box score)"
                               "+nclass(object scores)), got "<< jevois::dnn::shapestr(out));
  float const bfac = 1.0F / (8 << yolonum);
  int const boff = n * 2 * yolonum;
  int const whn = w * h * n;
  int const total = whn * bbsize;
  if (total != int(out.total())) LFATAL("Ooops");

  // Re-arrange the data order:
  float predictions[total];
  flatten((float const *)out.data, predictions, w*h, bbsize*n, 1, 1);
  
  // Apply logistic activation to box score and softmax to object scores:
  for (int i = 0; i < total; i += bbsize)
  {
    predictions[i + coords] = logistic_activate(predictions[i + coords]);
    jevois::dnn::softmax(predictions + i + coords + 1, nclass, 1, predictions + i + coords + 1);
  }

  // Loop over all locations:
  for (int i = 0; i < w * h; ++i)
  {
    int const row = i / w;
    int const col = i % w;

    // Loop over all boxes per location:
    for (int nn = 0; nn < n; ++nn)
    {
      int index = i*n + nn; // box number
      int box_index = index * bbsize; // address of box in predictions[]
      float scale = predictions[box_index + coords]; // box score
      int class_index = box_index + coords + 1; // address of the nclass object scores for our box
        
      // Find max object score and its Id:
      int maxid = -1; float maxscore = -1.0F;
      for (size_t j = 0; j < nclass; ++j)
      {
        float prob = scale * predictions[class_index + j];
        if (prob > confThreshold && prob > maxscore) { maxid = j; maxscore = prob; }
      }

      // If at least one class was above threshold, keep that box:
      if (maxid >= 0)
      {
        // Decode the box and scale it to input blob dims:
        cv::Rect b( (col + logistic_activate(predictions[box_index + 0])) * bsiz.width / w + 0.499F, // x
                    (row + logistic_activate(predictions[box_index + 1])) * bsiz.height / h + 0.499F, // y
                    exp(predictions[box_index + 2]) * biases[2*nn + boff] * bfac * bsiz.width / w + 0.499F, // w
                    exp(predictions[box_index + 3]) * biases[2*nn+1 + boff] * bfac * bsiz.height / h + 0.499F); // h
        b.x -= b.width / 2;
        b.y -= b.height / 2;

        boxes.emplace_back(b);
        classIds.emplace_back(maxid + fudge);
        confidences.emplace_back(maxscore);
      }
    }
  }
}
