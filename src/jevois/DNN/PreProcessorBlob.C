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

#include <jevois/DNN/PreProcessorBlob.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Image/RawImageOps.H>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// ####################################################################################################
jevois::dnn::PreProcessorBlob::~PreProcessorBlob()
{ }

// ####################################################################################################
void jevois::dnn::PreProcessorBlob::freeze(bool JEVOIS_UNUSED_PARAM(doit))
{ }

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::PreProcessorBlob::process(cv::Mat const & img, bool swaprb,
                                                            std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                                            std::vector<cv::Rect> & crops)
{
  float sc = scale::get(); bool docrop = letterbox::get();

  // Get the blobs:
  std::vector<cv::Mat> blobs;
  for (vsi_nn_tensor_attr_t const & attr : attrs)
  {
    cv::Mat blob;
    cv::Size bsiz = jevois::dnn::attrsize(attr);
    cv::Rect crop;

    // Start with an unscaled crop:
    unsigned int bw = bsiz.width, bh = bsiz.height;
    if (bw == 1 || bw == 3 || bh == 1 || bh == 3)
      LFATAL("Incorrect input tensor " << jevois::dnn::shapestr(attr) <<"; did you swap NHWC vs NCHW?");

    if (docrop)
    {
      jevois::applyLetterBox(bw, bh, img.cols, img.rows, false);
      
      cv::Rect roi;
      roi.x = (img.cols - bw) / 2;
      roi.y = (img.rows - bh) / 2;
      roi.width = bw;
      roi.height = bh;
      blob = img(roi);
      
      crop.x = (img.cols - bw) / 2;
      crop.y = (img.rows - bh) / 2;
      crop.width = bw;
      crop.height = bh;
    }
    else
    {
      blob = img;
      
      crop.x = 0;
      crop.y = 0;
      crop.width = img.cols;
      crop.height = img.rows;
    }
    
    // Resize to desired network input dims:
    cv::resize(blob, blob, bsiz);

    // Swap red/blue byte order if we have color and will not do planar; would be better below except that cvtColor
    // always outputs 8U pixels so we have to do this here before possible conversion to 8S:
    bool swapped = false;
    if (swaprb && attr.dtype.fmt == VSI_NN_DIM_FMT_NHWC)
    {
      switch (blob.channels())
      {
      case 3: cv::cvtColor(blob, blob, cv::COLOR_RGB2BGR); swapped = true; break;
      case 4: cv::cvtColor(blob, blob, cv::COLOR_RGBA2BGRA); swapped = true; break;
      default: break; // Ignore swaprb value if not 3 or 4 channels
      }
    }
    
    // Convert if needed:
    unsigned int tt = jevois::dnn::vsi2cv(attr.dtype.vx_type);
    unsigned int bt = blob.depth();
    if (bt == tt)
    {
      // Ready to go, no conversion
    }
    else if (bt == CV_8U && tt == CV_8S)
    {
      // Convert from 8U to 8S: need DFP quantization:
      if (attr.dtype.qnt_type != VSI_NN_QNT_TYPE_DFP) LFATAL("Need DFP quantization for 8S intensors");
      cv::Mat newblob(bsiz, CV_MAKETYPE(tt, blob.channels()));
      
      uint8_t const * bdata = (uint8_t const *)blob.data;
      uint32_t const sz = blob.total() * blob.channels();
      int8_t * data = (int8_t *)newblob.data;
      int const shift = 8 - attr.dtype.fl;
      for (uint32_t i = 0; i < sz; i++) data[i] = bdata[i] >> shift;
      blob = newblob;
    }
    else
    {
      // This is the slowest path... you should add optimizations above for some specific cases:
      blob.convertTo(blob, CV_32F);

      // Apply mean and scale:
      cv::Scalar m = mean::get();
      if (swaprb && swapped == false) std::swap(m[0], m[2]);
      if (m != cv::Scalar()) blob -= m;
      if (sc != 1.0F) blob *= sc;

      if (tt != CV_32F) blob.convertTo(blob, tt);
      /*
      if (attr.dtype.vx_type != VSI_NN_TYPE_FLOAT32)
      {
        // Convert to tensor type, applying whatever quantization is specified in attr:
        cv::Mat newblob(bsiz, blob.type());
        float const * fdata = (float const *)blob.data;
        uint32_t const sz = blob.total() * blob.channels();
        uint32_t const stride = vsi_nn_TypeGetBytes(attr.dtype.vx_type);
        uint8_t * data = (uint8_t *)newblob.data;
        for (uint32_t i = 0; i < sz; i++) vsi_nn_Float32ToDtype(fdata[i], &data[stride * i], &attr.dtype);
        blob = newblob;
        }
      */
    }

    // Ok, blob has desired width, height, and type, but is still packed RGB. Now deal with making a 4D shape, and R/G
    // swapping if we have channels:
    int const nch = blob.channels();
    switch (nch)
    {
    case 1:
      break; // Nothing to do

    case 3:
    case 4:
    {
      switch (attr.dtype.fmt)
      {
      case VSI_NN_DIM_FMT_NCHW:
      {
        // Convert from packed to planar:
        int dims[] = { 1, nch, blob.rows, blob.cols };
        cv::Mat newblob(4, dims, tt);

        // Create some pointers in newblob for each channel:
        cv::Mat nbc[nch];
        for (int i = 0; i < nch; ++i) nbc[i] = cv::Mat(blob.rows, blob.cols, tt, newblob.ptr(0, i));
        if (swaprb) std::swap(nbc[0], nbc[2]);

        // Split:
        cv::split(blob, nbc);

        // This our final 4D blob:
        blob = newblob;
      }
      break;

      case VSI_NN_DIM_FMT_NHWC:
      {
        // red/blue byte swap was handled above...
        
        // Finally convert to a 4D blob:
        blob = blob.reshape(1, { 1, bsiz.height, bsiz.width, 3 });
      }
      break;

      default: LFATAL("Can only handle NCHW or NHWC intensors shapes");
      }
    }
    break;

    default: LFATAL("Can only handle input images with 1, 3, or 4 channels");
    }

    // Done with this blob:
    blobs.emplace_back(blob);
    crops.emplace_back(crop);
  }
  return blobs;
}

// ####################################################################################################
void jevois::dnn::PreProcessorBlob::report(jevois::StdModule * JEVOIS_UNUSED_PARAM(mod),
                                           jevois::RawImage * JEVOIS_UNUSED_PARAM(outimg),
                                           jevois::OptGUIhelper * JEVOIS_UNUSED_PARAM(helper),
                                           bool JEVOIS_UNUSED_PARAM(overlay),
                                           bool JEVOIS_UNUSED_PARAM(idle))
{
  
}

