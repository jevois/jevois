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

#pragma once

#include <jevois/DNN/PostProcessor.H>

namespace jevois
{
  namespace dnn
  {
    //! Post-Processor sub-component for raw YOLO decoding
    class PostProcessorDetectYOLO : public Component,
                                    public Parameter<postprocessor::anchors, postprocessor::scalexy>
    {
      public:
        //! Inherited constructor ok
        using jevois::Component::Component;

        //! Destructor
        virtual ~PostProcessorDetectYOLO();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit);

        //! Generic raw YOLO processing
        void yolo(std::vector<cv::Mat> const & outs, std::vector<int> & classIds, std::vector<float> & confidences,
                  std::vector<cv::Rect> & boxes, size_t nclass, float boxThreshold, float confThreshold,
                  cv::Size const & bsiz, int fudge, size_t const maxbox, bool sigmo);

      private:
        void onParamChange(postprocessor::anchors const & param, std::string const & val);

        // Raw YOLO processing for one scale
        void yolo_one(cv::Mat const & out, std::vector<int> & classIds, std::vector<float> & confidences,
                      std::vector<cv::Rect> & boxes, size_t nclass, int yolonum, float boxThreshold,
                      float confThreshold, cv::Size const & bsiz, int fudge, size_t maxbox,
                      bool sigmo, float scale_xy);

        std::vector<std::vector<float>> itsAnchors;
        std::vector<int> itsYoloNum;
        std::mutex itsOutMtx;
    };
    
  } // namespace dnn
} // namespace jevois
