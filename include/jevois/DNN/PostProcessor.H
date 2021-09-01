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
      
      //! Parameter \relates PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(classoffset, int, "Offset added to model output when looking up class name. Useful if "
                               "your model uses a background class but your class file does not (use -1), or if your "
                               "model does not use a background class but your class file has one (use 1). If unsure, "
                               "use 0 and check whether reported class names are off.",
                               0, ParamCateg);
      
      //! Parameter \relates PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(classes, std::string, "Path to text file with names of object classes",
                                             "", ParamCateg);
      
      //! Parameter \relates PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(top, unsigned int, "Max number of top-scoring predictions that score above "
                               "thresh to report",
                               5, ParamCateg);
      
      //! Parameter \relates PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(thresh, float, "Threshold (in percent confidence) above which predictions will be "
                               "reported",
                               20.0F, jevois::Range<float>(0.0F, 100.0F), ParamCateg);
      
      //! Parameter \relates PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(softmax, bool, "Apply a softmax to classification outputs",
                               false, ParamCateg);

      //! Parameter \relates PostProcessorClassify
      JEVOIS_DECLARE_PARAMETER(scorescale, float, "Scaling factors applied to recognition scores, useful for "
                               "InceptionV3 and possibly other networks",
                               1.0F, ParamCateg);
      
      //! Enum \relates PostProcessorDetect
      JEVOIS_DEFINE_ENUM_CLASS(DetectType, (FasterRCNN) (YOLO) (SSD) (TPUSSD) (RAWYOLOface) (RAWYOLOv2)
                               (RAWYOLOv3) (RAWYOLOv4) (RAWYOLOv3tiny) )
      
      //! Parameter \relates PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER(detecttype, DetectType, "Type of detection output format",
                               DetectType::YOLO, DetectType_Values, ParamCateg);
      
      //! Parameter \relates PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER(nms, float, "Non-maximum suppression intersection-over-union threshold in percent",
                               45.0F, jevois::Range<float>(0.0F, 100.0F), ParamCateg);

      //! Parameter \relates PostProcessorDetect
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(anchors, std::string, "For YOLO-type detection models with raw outputs, "
                               "list of anchors. Should be formatted as: w1, h1, w2, h2, ... ; ww1, hh1, ww2, hh2, "
                               "... ; ... where individual entries for a given YOLO layer are separated by commas, "
                               "and successive YOLO layers (in the order in which they appear in the Darknet .cfg "
                               "file) are separated by semicolons. Leave empty for other models. If your anchors are "
                               "the same for all YOLO layers, you may just specify them once.",
                               "", ParamCateg);

      //! Parameter \relates PostProcessorDetect \relates PostProcessorSegment
      JEVOIS_DECLARE_PARAMETER(alpha, unsigned char, "Alpha channel value for drawn results",
                               64, ParamCateg);

      //! Parameter \relates PostProcessorSegment
      JEVOIS_DECLARE_PARAMETER(bgid, unsigned char, "Class ID for the background, will show as fully transparent in "
                               "semantic segmentation overlays",
                               0, ParamCateg);

      //! Enum \relates PostProcessorSegment
      JEVOIS_DEFINE_ENUM_CLASS(SegType, (Classes) (Classes2) (ArgMax) );

      //! Parameter \relates PostProcessorSegment
      JEVOIS_DECLARE_PARAMETER(segtype, SegType, "Type of segmentation network output. If Classes, output is HxWxN "
                               "where N is the number of classes and we get one score per class, and we will show "
                               "the top scoring class for each pixel (e.g., UNet-MobileNet on TPU). If Classes2, "
                               "output is NxHxW and the rest is as for Classes (e.g., DeepLabV3 OpenCV). If ArgMax, "
                               "output is HxW and contains the class ID for each pixel (e.g., DeepLabV3 on TPU).",
                               SegType::Classes, SegType_Values, ParamCateg);

      
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
