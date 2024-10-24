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

#include <jevois/DNN/PostProcessorYuNet.H>
#include <jevois/DNN/PreProcessor.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>

#include <opencv2/dnn.hpp>

// ####################################################################################################
// this code from https://github.com/khadas/OpenCV_NPU_Demo
namespace jevois { namespace dnn { namespace yunet {
    
      class PriorBox
      {
        public:
          PriorBox(cv::Size const & input_shape, cv::Size const & output_shape)
          {
            // initialize
            in_w = input_shape.width;
            in_h = input_shape.height;
            out_w = output_shape.width;
            out_h = output_shape.height;
            
            cv::Size feature_map_2nd { int(int((in_w+1)/2)/2), int(int((in_h+1)/2)/2) };
            cv::Size feature_map_3rd { int(feature_map_2nd.width/2), int(feature_map_2nd.height/2) };
            cv::Size feature_map_4th { int(feature_map_3rd.width/2), int(feature_map_3rd.height/2) };
            cv::Size feature_map_5th { int(feature_map_4th.width/2), int(feature_map_4th.height/2) };
            cv::Size feature_map_6th { int(feature_map_5th.width/2), int(feature_map_5th.height/2) };
            
            // feature_map_sizes.push_back(feature_map_2nd);
            feature_map_sizes.push_back(feature_map_3rd);
            feature_map_sizes.push_back(feature_map_4th);
            feature_map_sizes.push_back(feature_map_5th);
            feature_map_sizes.push_back(feature_map_6th);
            
            // generate the priors:
            for (size_t i = 0; i < feature_map_sizes.size(); ++i)
            {
              cv::Size feature_map_size = feature_map_sizes[i];
              std::vector<float> min_size = min_sizes[i];
              
              for (int _h = 0; _h < feature_map_size.height; ++_h)
              {
                for (int _w = 0; _w < feature_map_size.width; ++_w)
                {
                  for (size_t j = 0; j < min_size.size(); ++j)
                  {
                    float s_kx = min_size[j] / in_w;
                    float s_ky = min_size[j] / in_h;
                    
                    float cx = (_w + 0.5) * steps[i] / in_w;
                    float cy = (_h + 0.5) * steps[i] / in_h;
                    
                    Box anchor = { cx, cy, s_kx, s_ky };
                    priors.push_back(anchor);
                  }
                }
              }
            }
          }
          
          ~PriorBox()
          { }
          
          std::vector<Face> decode(cv::Mat const & loc, cv::Mat const & conf, cv::Mat const & iou,
                                   float const ignore_score = 0.3)
          {
            std::vector<Face> dets; // num * [x1, y1, x2, y2, x_re, y_re, x_le, y_le, x_ml, y_ml, x_n, y_n, x_mr, y_ml]
            
            float const * loc_v = (float const *)(loc.data);
            float const * conf_v = (float const *)(conf.data);
            float const * iou_v = (float const *)(iou.data);
            for (size_t i = 0; i < priors.size(); ++i)
            {
              // get score
              float cls_score = conf_v[i*2+1];
              float iou_score = iou_v[i];
              
              // clamp
              if (iou_score < 0.f) iou_score = 0.f; else if (iou_score > 1.f) iou_score = 1.f;
              float score = std::sqrt(cls_score * iou_score);
              
              // ignore low scores
              if (score < ignore_score) { continue; }
              
              Face face;
              face.score = score;
              
              // get bounding box
              float cx = (priors[i].x + loc_v[i*14+0] * variance[0] * priors[i].width) * out_w;
              float cy = (priors[i].y + loc_v[i*14+1] * variance[0] * priors[i].height) * out_h;
              float w  = priors[i].width * exp(loc_v[i*14+2] * variance[0]) * out_w;
              float h  = priors[i].height * exp(loc_v[i*14+3] * variance[1]) * out_h;
              float x1 = cx - w / 2;
              float y1 = cy - h / 2;
              face.bbox_tlwh = { x1, y1, w, h };
              
              // get landmarks, loc->[right_eye, left_eye, mouth_left, nose, mouth_right]
              float x_re = (priors[i].x + loc_v[i*14+ 4] * variance[0] * priors[i].width) *  out_w;
              float y_re = (priors[i].y + loc_v[i*14+ 5] * variance[0] * priors[i].height) * out_h;
              float x_le = (priors[i].x + loc_v[i*14+ 6] * variance[0] * priors[i].width) *  out_w;
              float y_le = (priors[i].y + loc_v[i*14+ 7] * variance[0] * priors[i].height) * out_h;
              float x_n =  (priors[i].x + loc_v[i*14+ 8] * variance[0] * priors[i].width) *  out_w;
              float y_n =  (priors[i].y + loc_v[i*14+ 9] * variance[0] * priors[i].height) * out_h;
              float x_mr = (priors[i].x + loc_v[i*14+10] * variance[0] * priors[i].width) *  out_w;
              float y_mr = (priors[i].y + loc_v[i*14+11] * variance[0] * priors[i].height) * out_h;
              float x_ml = (priors[i].x + loc_v[i*14+12] * variance[0] * priors[i].width) *  out_w;
              float y_ml = (priors[i].y + loc_v[i*14+13] * variance[0] * priors[i].height) * out_h;
              face.landmarks = {
                                {x_re, y_re},  // right eye
                                {x_le, y_le},  // left eye
                                {x_n,  y_n },  // nose
                                {x_mr, y_mr},  // mouth right
                                {x_ml, y_ml}   // mouth left
              };
              
              dets.push_back(face);
            }
            return dets;
          }
          
        private:
          std::vector<std::vector<float>> const min_sizes
          {
            {10.0f,  16.0f,  24.0f},
            {32.0f,  48.0f},
            {64.0f,  96.0f},
            {128.0f, 192.0f, 256.0f}
          };
          std::vector<int> const steps { 8, 16, 32, 64 };
          std::vector<float> const variance { 0.1, 0.2 };
          
          int in_w;
          int in_h;
          int out_w;
          int out_h;
          
          std::vector<cv::Size> feature_map_sizes;
          std::vector<Box> priors;
          std::vector<Box> generate_priors();
      };
      
    } // namespace yunet
  } // namespace dnn
} // namespace jevois


// ####################################################################################################
jevois::dnn::PostProcessorYuNet::~PostProcessorYuNet()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorYuNet::freeze(bool)
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorYuNet::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{
  if (outs.size() != 3) LFATAL("Need exactly 3 outputs, received " << outs.size());
  if (outs[0].rows < 16 || outs[0].cols != 2) LFATAL("loc size " << outs[0].size() << " instead of 2xN_Anchors");
  if (outs[1].rows < 16 || outs[1].cols != 1) LFATAL("conf size " << outs[1].size() << " instead of 1xN_Anchors");
  if (outs[2].rows < 16 || outs[2].cols != 14) LFATAL("iou size " << outs[2].size() << " instead of 14xN_Anchors");
  
  float const confThreshold = cthresh::get() * 0.01F;
  float const boxThreshold = dthresh::get() * 0.01F;
  float const nmsThreshold = nms::get() * 0.01F;
  itsImageSize = preproc->imagesize();

  // To draw boxes, we will need to:
  // - scale from [0..1]x[0..1] to blobw x blobh
  // - scale and center from blobw x blobh to input image w x h, provided by PreProcessor::b2i()
  // - when using the GUI, we further scale and translate to OpenGL display coordinates using GUIhelper::i2d()
  // Here we assume that the first blob sets the input size.
  cv::Size const bsiz = preproc->blobsize(0);

  // Initialize PriorBox on first inference:
  if (!itsPriorBox) itsPriorBox.reset(new jevois::dnn::yunet::PriorBox(bsiz, bsiz));
    
  // Get the detections from the raw network outputs:
  std::vector<jevois::dnn::yunet::Face> dets = itsPriorBox->decode(outs[2], outs[0], outs[1], boxThreshold);

  // NMS
  itsDetections.clear();
  if (dets.size() > 1)
  {
    std::vector<cv::Rect> face_boxes;
    std::vector<float> face_scores;
    for (auto const & d : dets) { face_boxes.push_back(d.bbox_tlwh); face_scores.push_back(d.score); }

    std::vector<int> keep_idx;
    cv::dnn::NMSBoxes(face_boxes, face_scores, confThreshold, nmsThreshold, keep_idx, 1.f, top::get());
    for (size_t i = 0; i < keep_idx.size(); i++) itsDetections.emplace_back(dets[keep_idx[i]]);
  }

  // Now clamp boxes to be within blob, and adjust the boxes from blob size to input image size:
  for (jevois::dnn::yunet::Face & f : itsDetections)
  {
    jevois::dnn::yunet::Box & b = f.bbox_tlwh;
    jevois::dnn::clamp(b, bsiz.width, bsiz.height);

    cv::Point2f tl = b.tl(); preproc->b2i(tl.x, tl.y);
    cv::Point2f br = b.br(); preproc->b2i(br.x, br.y);
    b.x = tl.x; b.y = tl.y; b.width = br.x - tl.x; b.height = br.y - tl.y;

    preproc->b2i(f.landmarks.right_eye.x, f.landmarks.right_eye.y);
    preproc->b2i(f.landmarks.left_eye.x, f.landmarks.left_eye.y);
    preproc->b2i(f.landmarks.nose_tip.x, f.landmarks.nose_tip.y);
    preproc->b2i(f.landmarks.mouth_right.x, f.landmarks.mouth_right.y);
    preproc->b2i(f.landmarks.mouth_left.x, f.landmarks.mouth_left.y);
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorYuNet::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                             jevois::OptGUIhelper * helper, bool overlay, bool /*idle*/)
{
  for (jevois::dnn::yunet::Face const & f : itsDetections)
  {
    jevois::dnn::yunet::Box const & b = f.bbox_tlwh;
    std::string const scorestr = jevois::sformat("face: %.1f%%", f.score * 100.0F);
    
    // If desired, draw boxes in output image:
    if (outimg && overlay)
    {
      unsigned int const col = jevois::yuyv::LightGreen;
      unsigned int const rad = 5;

      // Draw the face box:
      jevois::rawimage::drawRect(*outimg, b.x, b.y, b.width, b.height, 2, col);
      jevois::rawimage::writeText(*outimg, scorestr, b.x + 6, b.y + 2, col, jevois::rawimage::Font10x20);

      // Draw the face landmarks:
      jevois::rawimage::drawCircle(*outimg, f.landmarks.right_eye.x, f.landmarks.right_eye.y, rad*2, 2, col);
      jevois::rawimage::drawCircle(*outimg, f.landmarks.left_eye.x, f.landmarks.left_eye.y, rad*2, 2, col);
      jevois::rawimage::drawCircle(*outimg, f.landmarks.nose_tip.x, f.landmarks.nose_tip.y, rad, 2, col);
      jevois::rawimage::drawCircle(*outimg, f.landmarks.mouth_right.x, f.landmarks.mouth_right.y, rad, 2, col);
      jevois::rawimage::drawCircle(*outimg, f.landmarks.mouth_left.x, f.landmarks.mouth_left.y, rad, 2, col);
      jevois::rawimage::drawLine(*outimg, f.landmarks.mouth_left.x, f.landmarks.mouth_left.y,
                                 f.landmarks.mouth_right.x, f.landmarks.mouth_right.y, 2, col);
    }
    
#ifdef JEVOIS_PRO
    // If desired, draw results on GUI:
    if (helper)
    {
      ImU32 const col = 0x80ff80ff;
      float const rad = 5.0f;
      
      // Draw the face box:
      helper->drawRect(b.x, b.y, b.x + b.width, b.y + b.height, col, true);
      helper->drawText(b.x + 3.0f, b.y + 3.0f, scorestr.c_str(), col);

      // Draw the face landmarks:
      helper->drawCircle(f.landmarks.right_eye.x, f.landmarks.right_eye.y, rad*2, IM_COL32(255,0,0,255));
      helper->drawCircle(f.landmarks.left_eye.x, f.landmarks.left_eye.y, rad*2, IM_COL32(0,0,255,255));
      helper->drawCircle(f.landmarks.nose_tip.x, f.landmarks.nose_tip.y, rad, IM_COL32(0,255,0,255));
      helper->drawCircle(f.landmarks.mouth_right.x, f.landmarks.mouth_right.y, rad, IM_COL32(255,0,255,255));
      helper->drawCircle(f.landmarks.mouth_left.x, f.landmarks.mouth_left.y, rad, IM_COL32(0,255,255,255));
      helper->drawLine(f.landmarks.mouth_left.x, f.landmarks.mouth_left.y,
                       f.landmarks.mouth_right.x, f.landmarks.mouth_right.y, IM_COL32(255,255,255,255));
    }
#else
    (void)helper; // keep compiler happy  
#endif   
    
    // If desired, send results to serial port:
    (void)mod;
    //FIXME if (mod && serreport::get()) mod->sendSerialObjDetImg2D(itsImageSize.width, itsImageSize.height, o);
  }
}
