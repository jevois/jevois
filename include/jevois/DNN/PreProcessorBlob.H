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

#include <jevois/DNN/PreProcessor.H>
#include <opencv2/core/core.hpp>

namespace jevois
{
  namespace dnn
  {
    //! Pre-Processor for neural network pipeline
    /*! This is the first step in a deep neural network processing Pipeline.
        
        This pre-processor works as follows. As an example, assume a 1024x576 camera input frame and a 224x224 neural
        network input:

        - If camera frame is not RGB or BGR, convert to that (e.g., YUYV to RGB or BGR)

        - If \p letterbox is specified, fit the largest possible rectangle, with the aspect ratio of the network input,
          within the camera frame. For example, 224x224 is square so that would compute a 576x576 square box around the
          center of the camera frame. Otherwise, use the whole camera frame.
        
        - Crop that rectangle and resize it to network input size (possibly with stretching if \p letterbox was off)

        - Swap BGR/RGB if needed (combination of \p rgb parameter and color order in the received camera frame)

        - Most accurate but also slowest path (may be replaced by an optimized path below):
          + convert pixel data to float32
          + subtract \p mean if not zero
          + divide by \p stdev if not 1
          + multiply by \p scale if not 1. At this point, values will typically be in [0..1] or [-1..1]
          + quantize if needed. For example, if the network expects uint8 with asymmetric affine quantization
            NHWC:8U:1x224x224x3:AA:0.0078125:128, divide by quantizer scale (here, 0.0078125, so that multiplies the
            pixel values by 128) then add zero point (here, 128). The goal here is to use as much of the 8-bit dynamic
            range as possible. What the network wants (specified by its intensors parameter) is determined during the
            network quantization process.
          + convert to desired data type for the network (e.g., uint8)
          + possibly convert from packed RGB from the camera (NHWC) to planar (NCHW)
          + convert shape to 4D, with batch size (N) always 1

        - Because for uint8 (and also signed int8 and dynamic-fixed-point) this leads to nearly a no-op (first transform
          from native camera range [0..255] to, say, [0..1], then, during quantization, stretch back to [0..255]), fast
          paths are implemented for these special cases (e.g., uint8 camera input to quantized asymmetric affine uint8
          network input). For dynamic fixed point, the fast path uses fast bit-shifting operations; for uint8
          asymmetric affine, it is sometimes a no-op.

          You can see these steps in the JeVois-Pro GUI (in the window that shows network processing details) by
          enabling pre-processor parameter \p details

          \ingroup dnn */
    class PreProcessorBlob : public PreProcessor,
                             public jevois::Parameter<preprocessor::letterbox, preprocessor::scale, preprocessor::mean,
                                                      preprocessor::stdev, preprocessor::interp>
    {
      public:
        //! Inherited constructor ok
        using PreProcessor::PreProcessor;

        //! Destructor
        virtual ~PreProcessorBlob();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit) override;

      protected:
        //! Extract blobs from input image
        std::vector<cv::Mat> process(cv::Mat const & img, bool swaprb, std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                     std::vector<cv::Rect> & crops) override;

        //! Report what happened in last process() to console/output video/GUI
        void report(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                    jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false) override;

        std::vector<std::string> itsInfo;
    };
    
  } // namespace dnn
} // namespace jevois
