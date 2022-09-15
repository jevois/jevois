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

#include <jevois/Component/Component.H>
#include <opencv2/core/core.hpp>
#include <jevois/GPU/GUIhelper.H>
#include <jevois/Types/Enum.H>

namespace jevois
{
  class StdModule;
  class RawImage;

  namespace dnn
  {
    class PreProcessor;
    
    namespace postprocessor
    {
      // We define all parameters for all derived classes here to avoid duplicate definitions. Different derived classes
      // will use different subsets of all available parameters:
      static jevois::ParameterCategory const ParamCateg("DNN Post-Processing Options");
      
      //! Parameter \relates jevois::PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(classoffset, int, "Offset added to model output when looking up class name. Useful if "
                               "your model uses a background class but your class file does not (use -1), or if your "
                               "model does not use a background class but your class file has one (use 1). If unsure, "
                               "use 0 and check whether reported class names are off.",
                               0, ParamCateg);
      
      //! Parameter \relates jevois::PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(classes, std::string, "Path to text file with names of object classes",
                                             "", ParamCateg);
      
      //! Parameter \relates jevois::PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(top, unsigned int, "Max number of top-scoring predictions that score above "
                               "threshold to report",
                               5, ParamCateg);
      
      //! Parameter \relates jevois::PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER(maxnbox, unsigned int, "Max number of top-scoring boxes to report (for YOLO flavors, "
                               "this is the max for each scale)",
                               500, ParamCateg);
      
      //! Parameter \relates jevois::PostProcessorClassify \relates jevois::PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER(cthresh, float, "Classification threshold (in percent confidence) above which "
                               "predictions will be reported",
                               20.0F, jevois::Range<float>(0.0F, 100.0F), ParamCateg);

      //! Parameter \relates jevois::PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER(dthresh, float, "Detection box threshold (in percent confidence) above which "
                               "predictions will be reported. Not all networks use a separate box threshold, "
                               "many only use one threshold confidence threshold (cthresh parameter). The YOLO "
                               "family is an example that uses both box and classification confidences",
                               15.0F, jevois::Range<float>(0.0F, 100.0F), ParamCateg);
      
      //! Parameter \relates jevois::PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(softmax, bool, "Apply a softmax to classification outputs",
                               false, ParamCateg);

      //! Parameter \relates jevois::PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(scorescale, float, "Scaling factors applied to recognition scores. Mainly "
                               "for debugging if your scores seem too high or too low. If too high, usually "
                               "that means that you should turn on parameter softmax instead.",
                               1.0F, ParamCateg);
      
      //! Enum \relates jevois::PostProcessorDetect
      JEVOIS_DEFINE_ENUM_CLASS(DetectType, (FasterRCNN) (YOLO) (SSD) (TPUSSD) (RAWYOLO) );
      
      //! Parameter \relates jevois::PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(detecttype, DetectType, "Type of detection output format",
                                             DetectType::YOLO, DetectType_Values, ParamCateg);
      
      //! Parameter \relates jevois::PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER(nms, float, "Non-maximum suppression intersection-over-union threshold in percent",
                               45.0F, jevois::Range<float>(0.0F, 100.0F), ParamCateg);

      //! Parameter \relates jevois::PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(anchors, std::string, "For YOLO-type detection models with raw outputs, "
                               "list of anchors. Should be formatted as: w1, h1, w2, h2, ... ; ww1, hh1, ww2, hh2, "
                               "... ; ... where individual entries for a given YOLO layer are separated by commas, "
                               "and successive YOLO layers (from large to small, e.g., first the anchors for 52x52, "
                               "then for 26x26, then for 13x13) are separated by semicolons. Leave empty "
                               "for other models.",
                               "", ParamCateg);

      //! Parameter \relates jevois::PostProcessorDetect \relates jevois::PostProcessorSegment
      JEVOIS_DECLARE_PARAMETER(alpha, unsigned char, "Alpha channel value for drawn results",
                               64, ParamCateg);

      //! Parameter \relates jevois::PostProcessorSegment
      JEVOIS_DECLARE_PARAMETER(bgid, unsigned char, "Class ID for the background, will show as fully transparent in "
                               "semantic segmentation overlays",
                               0, ParamCateg);

      //! Enum \relates jevois::PostProcessorSegment
      JEVOIS_DEFINE_ENUM_CLASS(SegType, (ClassesHWC) (ClassesCHW) (ArgMax) );

      //! Parameter \relates jevois::PostProcessorSegment
      JEVOIS_DECLARE_PARAMETER(segtype, SegType, "Type of segmentation network output. ClassesHWC: output is 1xHxWxC "
                               "for C classes and we get one score per class; we will show "
                               "the top scoring class for each pixel (e.g., UNet-MobileNet on TPU). ClassesCHW: "
                               "output is 1xCxHxW and the rest is as for ClassesHWC (e.g., DeepLabV3 OpenCV). ArgMax: "
                               "output is HxW, 1xHxW, or 1xHxWx1 and contains the class ID for each pixel "
                               "(e.g., DeepLabV3 on TPU).",
                               SegType::ClassesHWC, SegType_Values, ParamCateg);

      //! Parameter \relates jevois::PostProcessorPython
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(pypost, std::string, "Path below " JEVOIS_SHARE_PATH "/ of the python "
                                             "post-processor file. Name of class defined in the file must match "
                                             "the file name without the trailing '.py'",
                                             "", ParamCateg);

      //! Parameter \relates jevois::PostProcessorDetectYOLO
      JEVOIS_DECLARE_PARAMETER(scalexy, float, "If 0, use old-style YOLO boxes (YOLOv2/v3/v4); otherwise, this is "
                               "the scale_xy factor for new-style YOLO coordinates (YOLOv5/v7; value is usually 2.0 "
                               "but check the yolo layer in the model's .cfg file)",
                               0.0F, ParamCateg);

      //! Parameter \relates jevois::PostProcessorDetectYOLO
      JEVOIS_DECLARE_PARAMETER(sigmoid, bool, "Apply sigmoid to raw YOLO outputs, use when the last conv layers "
                               "just before yolo/detection/region layers have linear activation (most "
                               "YOLOv2/v3/v4 models, but not YOLOv5/v7 which have logistic activation on their "
                               "last conv)",
                               true, ParamCateg);
    }

    //! Post-Processor for neural network pipeline
    /*! This is the last step in a deep neural network processing Pipeline. \ingroup dnn */
    class PostProcessor : public jevois::Component
    {
      public:
        
        //! Inherited constructor ok
        using jevois::Component::Component;

        //! Destructor
        virtual ~PostProcessor();

        //! Freeze/unfreeze parameters that users should not change while running
        virtual void freeze(bool doit) = 0;

        //! Process outputs
        virtual void process(std::vector<cv::Mat> const & outs, PreProcessor * preproc) = 0;

        //! Report what happened in last process() to console/output video/GUI
        virtual void report(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                            jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false) = 0;
   };
    
  } // namespace dnn
} // namespace jevois
