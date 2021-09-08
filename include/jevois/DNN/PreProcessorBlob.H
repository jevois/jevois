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
    /*! This is the first step in a deep neural network processing Pipeline. \ingroup dnn */
    class PreProcessorBlob : public PreProcessor,
                             public jevois::Parameter<preprocessor::letterbox, preprocessor::scale, preprocessor::mean>
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
    };
    
  } // namespace dnn
} // namespace jevois