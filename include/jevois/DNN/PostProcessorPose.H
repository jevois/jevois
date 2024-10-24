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
#include <jevois/Types/ObjDetect.H>
#include <jevois/Types/PoseSkeleton.H>

#ifdef JEVOIS_PRO
#include <jevois/DNN/hailo/yolov8pose_postprocess.hpp>
#endif

namespace jevois
{
  namespace dnn
  {
    //! Post-Processor for neural network pipeline, for human/animal/other pose detection (skeleton)
    /*! This is the last step in a deep neural network processing Pipeline. It estimates the pose (skeleton) of humans
        or animals seen by the camera. \ingroup dnn */
    class PostProcessorPose : public PostProcessor,
                              public Parameter<postprocessor::nms, postprocessor::nmsperclass,
                                               postprocessor::posetype, postprocessor::cthresh,
                                               postprocessor::jthresh, postprocessor::skeleton,
                                               postprocessor::serialreport, postprocessor::maxnbox,
                                               postprocessor::sigmoid, postprocessor::boxclamp,
                                               postprocessor::classes, postprocessor::classoffset,
                                               postprocessor::namedonly>
    {
      public:
        
        //! Inherited constructor ok
        using PostProcessor::PostProcessor;

        //! Destructor
        virtual ~PostProcessorPose();
        
        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit) override;
        
        //! Process outputs and draw/send some results
        void process(std::vector<cv::Mat> const & outs, PreProcessor * preproc) override;
        
        //! Report what happened in last process() to console/output video/GUI
        void report(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                    jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false) override;

        //! Get the latest detections, use with caution, not thread-safe
        /*! This returns a reference to our internal vector of detections. That vector will get overwritten every time
            process() is called. It is ok to use this after you have called process() on a given frame, or, if using a
            Pipeline, after you have called Pipeline::process(). Do not hold this ref past the end of the current video
            frame. If you need to keep a persistent copy of the data, make a deep copy of the vector. */
        std::vector<ObjDetect> const & latestDetections() const;

        //! Get the latest skeletons, use with caution, not thread-safe
        /*! This returns a reference to our internal vector of skeletons. That vector will get overwritten every time
            process() is called. It is ok to use this after you have called process() on a given frame, or, if using a
            Pipeline, after you have called Pipeline::process(). Do not hold this ref past the end of the current video
            frame. If you need to keep a persistent copy of the data, make a deep copy of the vector. */
        std::vector<PoseSkeleton> const & latestSkeletons() const;

      protected:
        void onParamChange(postprocessor::classes const & param, std::string const & val) override;
        void onParamChange(postprocessor::skeleton const & param, std::string const & val) override;
        std::map<int, std::string> itsLabels; //!< Mapping from object ID to class name
#ifdef JEVOIS_PRO
        // This is used by the Hailo decoder code only:
        HailoROIPtr itsROI = std::make_shared<HailoROI>(HailoROI(HailoBBox(0.0f, 0.0f, 1.0f, 1.0f)));
        std::pair<std::vector<KeyPt>, std::vector<PairPairs>> itsKeypointsAndPairs;
#endif
        std::shared_ptr<jevois::PoseSkeletonDefinition> itsSkeletonDef;
        cv::Size itsImageSize;
        std::vector<ObjDetect> itsDetections;
        std::vector<PoseSkeleton> itsSkeletons;
    };
    
  } // namespace dnn
} // namespace jevois
