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

#include <jevois/DNN/PostProcessorPose.H>
#include <jevois/DNN/PreProcessor.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>

#ifdef JEVOIS_PRO
#include <jevois/DNN/NetworkHailo.H>
#include <jevois/DNN/hailo/yolov8pose_postprocess.hpp>
#endif

#include <opencv2/dnn.hpp>

// ####################################################################################################
jevois::dnn::PostProcessorPose::~PostProcessorPose()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorPose::freeze(bool doit)
{
  classes::freeze(doit);
  posetype::freeze(doit);
  skeleton::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorPose::onParamChange(postprocessor::classes const &, std::string const & val)
{
  if (val.empty()) { itsLabels.clear(); return; }
  itsLabels = jevois::dnn::readLabelsFile(jevois::absolutePath(JEVOIS_SHARE_PATH, val));
}

// ####################################################################################################
void jevois::dnn::PostProcessorPose::onParamChange(postprocessor::skeleton const &, std::string const & val)
{
  itsSkeletonDef.reset(new jevois::PoseSkeletonDefinition(val));
}
  
// ####################################################################################################
void jevois::dnn::PostProcessorPose::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  if (outs.empty()) LFATAL("No outputs received, we need at least one.");
  float const confThreshold = cthresh::get() * 0.01F;
  float const nmsThreshold = nms::get() * 0.01F;
  float const jointThreshold = jthresh::get() * 0.01F;
  bool const sigmo = sigmoid::get();
  bool const clampbox = boxclamp::get();
  int const fudge = classoffset::get();
  itsImageSize = preproc->imagesize();
  size_t const boxmax = maxnbox::get();

  // Clear any old results:
  itsDetections.clear();
  itsSkeletons.clear();
  
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
  // Initially we will store 1 skeleton for each box above threshold. After NMS on boxes, we will keep only the best
  // boxes and their associated best skeletons:
  std::vector<PoseSkeleton> skeletons;

  // Here we scale the coords from [0..1]x[0..1] to blobw x blobh and then to image w x h:
  try
  {
    switch(posetype::get())
    {
#ifdef JEVOIS_PRO
      // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::PoseType::YOLOv8HAILO:
    {
      // Network produces 9 output blobs, which should not be dequantized:
      if (outs.size() != 9) LTHROW("Expected 9 quantized output blobs from split YOLOv8-pose Hailo network");
      if (outs[0].type() == CV_32F) LTHROW("Expected quantized outputs, turn 'dequant' off");

      // We need the network to be of type Hailo so we can get the quantized output tensors and their Hailo tensor
      // infos. We assume that there is a sub-component named "network" that is a sibling of us:
      std::vector<std::string> dd = jevois::split(Component::descriptor(), ":"); dd.pop_back();
      std::shared_ptr<jevois::Component> comp = engine()->getComponent(dd[0]); dd.erase(dd.begin());
      for (std::string const & c : dd) { comp = comp->getSubComponent(c); if (!comp) LFATAL("Internal error"); }
      auto net = comp->getSubComponent<jevois::dnn::NetworkHailo>("network");
      std::vector<hailo_vstream_info_t> outinfos = net->outputInfos();

      // Create a Hailo ROI with the quantized tensors:
      itsROI = std::make_shared<HailoROI>(HailoROI(HailoBBox(0.0f, 0.0f, 1.0f, 1.0f)));
      for (size_t i = 0; i < outs.size(); ++i)
        itsROI->add_tensor(std::make_shared<HailoTensor>(outs[i].data, outinfos[i]));

      // Post-process using Hailo code:
      int constexpr regression_length = 15;
      std::vector<int> const strides = { 8, 16, 32 };
      std::vector<int> const network_dims = { bsiz.width, bsiz.height };

      std::vector<HailoTensorPtr> tensors = itsROI->get_tensors();
      auto filtered_decodings = yolov8pose_postprocess(tensors, network_dims, strides,
                                                       regression_length, 1 /* NUM_CLASSES */, confThreshold,
                                                       nmsThreshold);
      
      // Collect both unscaled boxes for further keypoint processing, and scaled boxes for display in report():
      itsDetections.clear();
      std::vector<HailoDetection> detections;
      for (auto & dec : filtered_decodings)
      {
        // Unscaled boxes:
        HailoDetection & detbox = dec.detection_box;
        detections.push_back(detbox); //dec.detection_box);

        // Scaled boxes:
        if (detbox.get_confidence() == 0.0) continue;

        HailoBBox box = detbox.get_bbox();
        float xmin = box.xmin() * bsiz.width, ymin = box.ymin() * bsiz.height;
        float xmax = box.xmax() * bsiz.width, ymax = box.ymax() * bsiz.height;
        preproc->b2i(xmin, ymin);
        preproc->b2i(xmax, ymax);

        jevois::ObjReco o { detbox.get_confidence() * 100.0f, detbox.get_label() };
        std::vector<jevois::ObjReco> ov; ov.emplace_back(o);
        jevois::ObjDetect od { int(xmin), int(ymin), int(xmax), int(ymax), ov, std::vector<cv::Point>() };
        itsDetections.emplace_back(od);
      }
      
      hailo_common::add_detections(itsROI, detections);
      
      itsKeypointsAndPairs = filter_keypoints(filtered_decodings, network_dims, jointThreshold);
      
      // Scale all the keypoints and pairs (links):
      for (auto & keypoint : itsKeypointsAndPairs.first)
      {
        float xs = keypoint.xs * bsiz.width, ys = keypoint.ys * bsiz.height;
        preproc->b2i(xs, ys);
        keypoint.xs = xs; keypoint.ys = ys;
      }
      
      for (PairPairs & p : itsKeypointsAndPairs.second)
      {
        float x1 = p.pt1.first * bsiz.width, y1 = p.pt1.second * bsiz.height;
        float x2 = p.pt2.first * bsiz.width, y2 = p.pt2.second * bsiz.height;
        preproc->b2i(x1, y1);
        preproc->b2i(x2, y2);
        p.pt1.first = x1; p.pt1.second = y1; p.pt2.first = x2; p.pt2.second = y2; 
      }
    }
    break;
#endif
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::PoseType::YOLOv8t:
    {
      if ((outs.size() % 3) != 0)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 3 blobs: 1xHxWx64 (raw boxes), "
               "1xHxWxC (class scores), and 1xHxWx3K for K skeleton keypoints [x,y,score]");
      
      int stride = 8;
      int constexpr reg_max = 16;
      
      for (size_t idx = 0; idx < outs.size(); idx += 3)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[3] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1xHxWx64");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xHxWxC");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[3];
        
        cv::Mat const & kpt = outs[idx + 2]; cv::MatSize const & kpt_siz = kpt.size;
        if (kpt_siz.dims() != 4 || (kpt_siz[3] % 3) != 0) LTHROW("Output " << idx << " is not 4D 1xHxWx3K");
        float const * kpt_data = (float const *)kpt.data;
        size_t const nkpt = kpt_siz[3];

        unsigned int const nn = itsSkeletonDef->nodeNames.size();
        if (nkpt != 3 * nn)
          LFATAL("Received keypoint data size " << nkpt << " but skeleton has " << nn <<
                 " joints: data size must be 3x number of joints, for [x,y,conf]");
              
        for (int i = 1; i < 3; ++i)
          if (cls_siz[i] != bx_siz[i] || cls_siz[i] != kpt_siz[i])
            LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 1);
        
        // Keep track of detected joints so we can later set the links:
        bool has_joint[nn]; float jconf[nn]; float jx[nn]; float jy[nn];
              
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
              boxes.emplace_back(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin));
              classIds.emplace_back(int(best_idx) + fudge);
              confidences.emplace_back(confidence);

              // Now the skeleton keypoints:
              skeletons.emplace_back(jevois::PoseSkeleton(itsSkeletonDef));
              jevois::PoseSkeleton & skel = skeletons.back();
              
              for (unsigned int i = 0; i < nn; ++i)
              {
                // With Hailo nets, we want sigmo for these scores but not box scores...
                // May need another param for joint score sigmoid.
                float kconf = jevois::dnn::sigmoid(kpt_data[3*i + 2]);
                
                if (kconf >= jointThreshold)
                {
                  float kpx = (x + kpt_data[3*i] * 2.0F) * stride;
                  float kpy = (y + kpt_data[3*i + 1] * 2.0F) * stride;
                  preproc->b2i(kpx, kpy);
                  skel.nodes.emplace_back(jevois::PoseSkeleton::Node { i, kpx, kpy, kconf * 100.0F });
                  has_joint[i] = true; jconf[i] = kconf; jx[i] = kpx; jy[i] = kpy;
                }
                else has_joint[i] = false;
              }
              
              // Add the links if we have some keypoint:
              if (skel.nodes.empty() == false)
              {
                std::vector<std::pair<unsigned int, unsigned int>> const & linkdefs = skel.linkDefinitions();
                unsigned int id = 0;

                for (std::pair<unsigned int, unsigned int> const & lnk : linkdefs)
                {
                  if (has_joint[lnk.first] && has_joint[lnk.second])
                    skel.links.emplace_back(jevois::PoseSkeleton::Link { id, jx[lnk.first], jy[lnk.first],
                                                                         jx[lnk.second], jy[lnk.second],
                                                                         jconf[lnk.first]*jconf[lnk.second]*10000.0F });
                  ++id;
                }
              }
            }
            
            // Move to the next location:
            cls_data += nclass;
            bx_data += 4 * reg_max;
            kpt_data += nkpt;
          }
              
        // Move to the next scale:
        stride *= 2;
      }
    }
    break;
    
    // ----------------------------------------------------------------------------------------------------
    case jevois::dnn::postprocessor::PoseType::YOLOv8:
    {
      if ((outs.size() % 3) != 0)
        LTHROW("Expected several (usually 3, for 3 strides) sets of 3 blobs: 1x64xHxW (raw boxes), "
               "1xCxHxW (class scores), and 1x3KxHxW for K skeleton keypoints [x,y,score]");
      
      int stride = 8;
      int constexpr reg_max = 16;
      
      for (size_t idx = 0; idx < outs.size(); idx += 3)
      {
        cv::Mat const & bx = outs[idx]; cv::MatSize const & bx_siz = bx.size;
        if (bx_siz.dims() != 4 || bx_siz[1] != 4 * reg_max) LTHROW("Output " << idx << " is not 4D 1x64xHxW");
        float const * bx_data = (float const *)bx.data;

        cv::Mat const & cls = outs[idx + 1]; cv::MatSize const & cls_siz = cls.size;
        if (cls_siz.dims() != 4) LTHROW("Output " << idx << " is not 4D 1xCxHxW");
        float const * cls_data = (float const *)cls.data;
        size_t const nclass = cls_siz[1];
        
        cv::Mat const & kpt = outs[idx + 2]; cv::MatSize const & kpt_siz = kpt.size;
        if (kpt_siz.dims() != 4 || (kpt_siz[1] % 3) != 0) LTHROW("Output " << idx << " is not 4D 1x3KxHxW");
        float const * kpt_data = (float const *)kpt.data;
        size_t const nkpt = kpt_siz[1];

        unsigned int const nn = itsSkeletonDef->nodeNames.size();
        if (nkpt != 3 * nn)
          LFATAL("Received keypoint data size " << nkpt << " but skeleton has " << nn <<
                 " joints: data size must be 3x number of joints, for [x,y,conf]");
              
        for (int i = 2; i < 4; ++i)
          if (cls_siz[i] != bx_siz[i] || cls_siz[i] != kpt_siz[i])
            LTHROW("Mismatched HxW sizes for outputs " << idx << " .. " << idx + 1);

        size_t const step = cls_siz[2] * cls_siz[3]; // HxW

        // Keep track of detected joints so we can later set the links:
        bool has_joint[nn]; float jconf[nn]; float jx[nn]; float jy[nn];
              
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
              boxes.emplace_back(cv::Rect(xmin, ymin, xmax - xmin, ymax - ymin));
              classIds.emplace_back(int(best_idx) + fudge);
              confidences.emplace_back(confidence);

              // Now the skeleton keypoints:
              skeletons.emplace_back(jevois::PoseSkeleton(itsSkeletonDef));
              jevois::PoseSkeleton & skel = skeletons.back();
              
              for (unsigned int i = 0; i < nn; ++i)
              {
                unsigned int const offset = 3 * i * step;
                
                // With Hailo nets, we want sigmo for these scores but not box scores...
                // May need another param for joint score sigmoid.
                float kconf = jevois::dnn::sigmoid(kpt_data[offset + 2 * step]);
                
                if (kconf >= jointThreshold)
                {
                  float kpx = (x + kpt_data[offset] * 2.0F) * stride;
                  float kpy = (y + kpt_data[offset + step] * 2.0F) * stride;
                  preproc->b2i(kpx, kpy);
                  skel.nodes.emplace_back(jevois::PoseSkeleton::Node { i, kpx, kpy, kconf * 100.0F });
                  has_joint[i] = true; jconf[i] = kconf; jx[i] = kpx; jy[i] = kpy;
                }
                else has_joint[i] = false;
              }
              
              // Add the links if we have some keypoint:
              if (skel.nodes.empty() == false)
              {
                std::vector<std::pair<unsigned int, unsigned int>> const & linkdefs = skel.linkDefinitions();
                unsigned int id = 0;
                
                for (std::pair<unsigned int, unsigned int> const & lnk : linkdefs)
                {
                  if (has_joint[lnk.first] && has_joint[lnk.second])
                    skel.links.emplace_back(jevois::PoseSkeleton::Link { id, jx[lnk.first], jy[lnk.first],
                                                                         jx[lnk.second], jy[lnk.second],
                                                                         jconf[lnk.first]*jconf[lnk.second]*10000.0F });
                  ++id;
                }
              }
            }
            
            // Move to the next location:
            ++cls_data; ++bx_data; ++kpt_data;
          }
              
        // Move to the next scale:
        stride *= 2;
      }
    }
    break;
   
    default:
      // Do not use strget() here as it will throw!
      LTHROW("Unsupported Post-processor posetype " << int(posetype::get()));
    }
  }
  
  // Abort here if the received outputs were malformed:
  catch (std::exception const & e)
  {
    std::string err = "Selected posetype is " + posetype::strget() + " and network produced:\n\n";
    for (cv::Mat const & m : outs) err += "- " + jevois::dnn::shapestr(m) + "\n";
    err += "\nFATAL ERROR(s):\n\n";
    err += e.what();
    LFATAL(err);
  }

  // Stop here if detections were already post-processed (e.g., YOLOv8HAILO); otherwise clean them up in the same way as
  // we do in PostProcessorDetect:
  if (itsDetections.empty() == false) return;
  
  // Keep the code below in sync with PostProcessorDetect:

  // Cleanup overlapping boxes, either globally or per class, and possibly limit number of reported boxes:
  std::vector<int> indices;
  if (nmsperclass::get())
    cv::dnn::NMSBoxesBatched(boxes, confidences, classIds, confThreshold, nmsThreshold, indices, 1.0F, boxmax);
  else
    cv::dnn::NMSBoxes(boxes, confidences, confThreshold, nmsThreshold, indices, 1.0F, boxmax);

  // Now clamp boxes to be within blob, and adjust the boxes from blob size to input image size:
  for (cv::Rect & b : boxes)
  {
    if (clampbox) jevois::dnn::clamp(b, bsiz.width, bsiz.height);

    cv::Point2f tl = b.tl(); preproc->b2i(tl.x, tl.y);
    cv::Point2f br = b.br(); preproc->b2i(br.x, br.y);
    b.x = tl.x; b.y = tl.y; b.width = br.x - tl.x; b.height = br.y - tl.y;
  }

  // Store results:
  bool namonly = namedonly::get();
  for (size_t i = 0; i < indices.size(); ++i)
  {
    int idx = indices[i];
    cv::Rect const & box = boxes[idx];
    std::string const label = jevois::dnn::getLabel(itsLabels, classIds[idx], namonly);
    if (namonly == false || label.empty() == false)
    {
      
      std::vector<jevois::ObjReco> ov;
      ov.emplace_back(jevois::ObjReco{ confidences[idx] * 100.0f, label } );
      itsDetections.emplace_back(jevois::ObjDetect{ box.x, box.y, box.x+box.width, box.y+box.height,
                                                    std::move(ov), std::vector<cv::Point>() });

      // If we keep that box, also keep the associated skeleton:
      itsSkeletons.emplace_back(std::move(skeletons[idx]));
    }
  }

}

// ####################################################################################################
void jevois::dnn::PostProcessorPose::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                                 jevois::OptGUIhelper * helper, bool overlay,
                                                 bool /*idle*/)
{
  bool const serreport = serialreport::get();

  for (jevois::ObjDetect const & o : itsDetections)
  {
    std::string categ, label;

    if (o.reco.empty()) { categ = "unknown"; label = "unknown"; }
    else { categ = o.reco[0].category; label = jevois::sformat("%s: %.2f", categ.c_str(), o.reco[0].score); }

    // If desired, draw boxes in output image:
    if (outimg && overlay)
    {
      jevois::rawimage::drawRect(*outimg, o.tlx, o.tly, o.brx - o.tlx, o.bry - o.tly, 2, jevois::yuyv::LightGreen);
      jevois::rawimage::writeText(*outimg, label, o.tlx + 6, o.tly + 2, jevois::yuyv::LightGreen,
                                  jevois::rawimage::Font10x20);
    }
    
#ifdef JEVOIS_PRO
    // If desired, draw boxes on GUI:
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
    if (mod && serreport) mod->sendSerialObjDetImg2D(itsImageSize.width, itsImageSize.height, o);
  }

  // If desired, draw skeleton in output image:
  if (outimg && overlay)
  {
#ifdef JEVOIS_PRO
    for (auto const & keypoint : itsKeypointsAndPairs.first)
      jevois::rawimage::drawCircle(*outimg, keypoint.xs, keypoint.ys, 5, 2, jevois::yuyv::LightGreen);
    
    for (PairPairs const & p : itsKeypointsAndPairs.second)
      jevois::rawimage::drawLine(*outimg, p.pt1.first, p.pt1.second, p.pt2.first, p.pt2.second,
                                 2, jevois::yuyv::LightGreen);
#endif
    
    for (jevois::PoseSkeleton const & skel : itsSkeletons)
    {
      for (jevois::PoseSkeleton::Node const & n : skel.nodes)
        jevois::rawimage::drawCircle(*outimg, n.x, n.y, 5, 2, jevois::yuyv::LightGreen);

      for (jevois::PoseSkeleton::Link const & lnk : skel.links)
        jevois::rawimage::drawLine(*outimg, lnk.x1, lnk.y1, lnk.x2, lnk.y2, 2, jevois::yuyv::LightGreen);
    }
  }
  
#ifdef JEVOIS_PRO
  // If desired, draw skeleton on GUI:
  if (helper)
  {
    for (auto const & keypoint : itsKeypointsAndPairs.first)
      helper->drawCircle(keypoint.xs, keypoint.ys, 5, IM_COL32(255,0,0,255), true);

    for (PairPairs const & p : itsKeypointsAndPairs.second)
      helper->drawLine(p.pt1.first, p.pt1.second, p.pt2.first, p.pt2.second, IM_COL32(255,0,0,255));

    for (jevois::PoseSkeleton const & skel : itsSkeletons)
    {
      for (jevois::PoseSkeleton::Node const & n : skel.nodes)
        helper->drawCircle(n.x, n.y, 5, skel.nodeColor(n.id), true);

      for (jevois::PoseSkeleton::Link const & lnk : skel.links)
        helper->drawLine(lnk.x1, lnk.y1, lnk.x2, lnk.y2, skel.linkColor(lnk.id));
    }
  }
#endif

}

// ####################################################################################################
std::vector<jevois::ObjDetect> const & jevois::dnn::PostProcessorPose::latestDetections() const
{ return itsDetections; }

// ####################################################################################################
std::vector<jevois::PoseSkeleton> const & jevois::dnn::PostProcessorPose::latestSkeletons() const
{ return itsSkeletons; }
