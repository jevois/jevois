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

#define DETAILS(fmt, ...)                                               \
  do { if (detail) itsInfo.emplace_back(prefix + jevois::sformat(fmt, ## __VA_ARGS__)); } while(0)

#define DETAILS2(fmt, ...)                                               \
  do { itsInfo.emplace_back(prefix + jevois::sformat(fmt, ## __VA_ARGS__)); } while(0)

// ####################################################################################################
jevois::dnn::PreProcessorBlob::~PreProcessorBlob()
{ }

// ####################################################################################################
void jevois::dnn::PreProcessorBlob::freeze(bool doit)
{
  numin::freeze(doit);
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::PreProcessorBlob::process(cv::Mat const & img, bool swaprb,
                                                            std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                                            std::vector<cv::Rect> & crops)
{
  bool const detail = details::get();
  itsInfo.clear();
  cv::Scalar m = mean::get();
  cv::Scalar sd = stdev::get();
  if (sd[0] == 0.0 || sd[1] == 0.0 || sd[2] == 0.0) LFATAL("stdev cannot be zero");
  float sc = scale::get();
  if (sc == 0.0F) LFATAL("Scale cannot be zero");
  
  std::vector<cv::Mat> blobs; size_t bnum = 0;
  for (vsi_nn_tensor_attr_t const & attr : attrs)
  {
    // --------------------------------------------------------------------------------
    // Get the blob:
    cv::Mat blob;
    cv::Size bsiz = jevois::dnn::attrsize(attr);
    cv::Rect crop;
    std::string prefix; if (detail) prefix = "Blob " + std::to_string(bnum) + ": ";

    // Start with an unscaled crop:
    unsigned int bw = bsiz.width, bh = bsiz.height;
    if (bw == 1 || bw == 3 || bh == 1 || bh == 3)
      LFATAL("Incorrect input tensor " << jevois::dnn::shapestr(attr) <<
             "; did you swap NHWC vs NCHW in your intensors specification?");

    // --------------------------------------------------------------------------------
    // Compute crop rectangle:
    if (letterbox::get())
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
      DETAILS("Letterbox %dx%d @ %d,%d", bw, bh, crop.x, crop.y);
    }
    else
    {
      blob = img;
      
      crop.x = 0;
      crop.y = 0;
      crop.width = img.cols;
      crop.height = img.rows;
    }
    
    // --------------------------------------------------------------------------------
    // Crop and resize to desired network input dims:
    cv::InterpolationFlags interpflags;
    switch (interp::get())
    {
    case jevois::dnn::preprocessor::InterpMode::Linear: interpflags = cv::INTER_LINEAR; break;
    case jevois::dnn::preprocessor::InterpMode::Cubic: interpflags = cv::INTER_CUBIC; break;
    case jevois::dnn::preprocessor::InterpMode::Area: interpflags = cv::INTER_AREA; break;
    case jevois::dnn::preprocessor::InterpMode::Lanczos4: interpflags = cv::INTER_LANCZOS4; break;
    default: interpflags = cv::INTER_NEAREST;
    }
    
    cv::resize(blob, blob, bsiz, 0.0, 0.0, interpflags);
    DETAILS("Resize to %dx%d%s", blob.cols, blob.rows, letterbox::get() ? "" : " (stretch)");
    
    // --------------------------------------------------------------------------------
    // Swap red/blue byte order if we have color and will not do planar; would be better below except that cvtColor
    // always outputs 8U pixels so we have to do this here before possible conversion to 8S or others:
    bool swapped = false;
    if (swaprb && attr.dtype.fmt == VSI_NN_DIM_FMT_NHWC)
    {
      switch (blob.channels())
      {
      case 3: cv::cvtColor(blob, blob, cv::COLOR_RGB2BGR); swapped = true; break;
      case 4: cv::cvtColor(blob, blob, cv::COLOR_RGBA2BGRA); swapped = true; break;
      default: break; // Ignore swaprb value if not 3 or 4 channels
      }
      DETAILS("Swap Red <-> Blue");
    }

    // If we need to swap but will do it later, swap mean and std red/blue now:
    if (swaprb && swapped == false) { std::swap(m[0], m[2]); std::swap(sd[0], sd[2]); }

    // --------------------------------------------------------------------------------
    // Convert and quantize if needed: First try some fast paths:
    unsigned int const tt = jevois::dnn::vsi2cv(attr.dtype.vx_type);
    unsigned int const bt = blob.depth();
    bool const uniformsd = (sd[0] == sd[1] && sd[1] == sd[2]);
    bool const uniformmean = (m[0] == m[1] && m[1] == m[2]);
    bool const unitsd = (uniformsd && sd[0] > 0.99 && sd[0] < 1.01);
    bool notdone = true;
    
    if (bt  == CV_8U && tt == CV_8U && attr.dtype.qnt_type == VSI_NN_QNT_TYPE_NONE)
    {
      DETAILS("8U to 8U direct no quantization");
      DETAILS("(ignoring mean, scale, stdev)");
      notdone = false;
    }
    
    else if (unitsd && attr.dtype.qnt_type == VSI_NN_QNT_TYPE_DFP)
    {
      if (bt == CV_8U && tt == CV_8S)
      {
        // --------------------
        // Convert from 8U to 8S with DFP quantization:
        cv::Mat newblob(bsiz, CV_MAKETYPE(tt, blob.channels()));
 
        uint8_t const * bdata = (uint8_t const *)blob.data;
        uint32_t const sz = blob.total() * blob.channels();
        int8_t * data = (int8_t *)newblob.data;
        if (attr.dtype.fl > 7) LFATAL("Invalid DFP fl value " << attr.dtype.fl << ": must be in [0..7]");
        int const shift = 8 - attr.dtype.fl;
        for (uint32_t i = 0; i < sz; ++i) *data++ = *bdata++ >> shift;
        
        DETAILS("8U to 8S DFP:%d: bit-shift >> %d", attr.dtype.fl, shift);
        blob = newblob;

        if (m[0] > 1.0 || m[1] > 1.0 || m[2] > 1.0)
        {
          blob -= m;
          DETAILS("Subtract mean [%.2f %.2f %.2f]", m[0], m[1], m[2]);
        }
        notdone = false;
      }
      else if (bt == CV_8U && tt == CV_16S)
      {
        // --------------------
        // Convert from 8U to 16S with DFP quantization:
        int const fl = attr.dtype.fl;
        uint8_t const * bdata = (uint8_t const *)blob.data;
        uint32_t const sz = blob.total() * blob.channels();
        if (fl > 15) LFATAL("Invalid DFP fl value " << fl << ": must be in [0..15]");
        if (fl > 8)
        {
          cv::Mat newblob(bsiz, CV_MAKETYPE(tt, blob.channels()));
          int16_t * data = (int16_t *)newblob.data;
          int const shift = fl - 8;
          for (uint32_t i = 0; i < sz; ++i) *data++ = int16_t(*bdata++) << shift;
          blob = newblob;
          DETAILS("8U to 16S DFP:%d: bit-shift << %d", fl, shift);
        }
        else if (fl < 8)
        {
          cv::Mat newblob(bsiz, CV_MAKETYPE(tt, blob.channels()));
          int16_t * data = (int16_t *)newblob.data;
          int const shift = 8 - fl;
          for (uint32_t i = 0; i < sz; ++i) *data++ = int16_t(*bdata++) >> shift;
          blob = newblob;
          DETAILS("8U to 16S DFP:%d: bit-shift >> %d", fl, shift);
        }
        else
        {
          blob.convertTo(blob, tt);
          DETAILS("8U to 16S DFP:%d: direct conversion", fl);
        }
 
        if (m[0] > 1.0 || m[1] > 1.0 || m[2] > 1.0)
        {
          blob -= m;
          DETAILS("Subtract mean [%.2f %.2f %.2f]", m[0], m[1], m[2]);
        }
        notdone = false;
      }
      // We only handle DFP: 8U->8S and 8U->16S with unit stdev here, more general code below for other cases.
    }
    
    if (notdone && uniformsd && uniformmean)
    {
      double qs, zp;
      switch (attr.dtype.qnt_type)
      {
      case VSI_NN_QNT_TYPE_AFFINE_ASYMMETRIC: qs = attr.dtype.scale; zp = attr.dtype.zero_point; notdone = false; break;
      case VSI_NN_QNT_TYPE_DFP: qs = 1.0 / (1 << attr.dtype.fl); zp = 0.0; notdone = false; break;
      default: break;
      }
      
      if (notdone == false)
      {
        if (qs == 0.0) LFATAL("Quantizer scale must not be zero");
        double alpha = sc / (sd[0] * qs);
        double beta = zp - m[0] * alpha;
        if (alpha > 0.99 && alpha < 1.01) alpha = 1.0; // will run faster
        if (beta > -0.51 && beta < 0.51) beta = 0.0; // will run faster

        if (alpha == 1.0 && beta == 0.0 && bt == tt)
          DETAILS("No conversion needed");
        else
        {
          cv::Mat newblob;
          blob.convertTo(newblob, tt, alpha, beta);
          blob = newblob;
          if (detail)
          {
            DETAILS2("%s to %s fast path", jevois::cvtypestr(bt).c_str(), jevois::cvtypestr(tt).c_str());
            if (m[0]) DETAILS2("Subtract mean [%.2f %.2f %.2f]", m[0], m[1], m[2]);
            if (sd[0] != 1.0) DETAILS2("Divide by stdev [%f %f %f]", sd[0], sd[1], sd[2]);
            if (sc != 1.0F) DETAILS2("Multiply by scale %f (=1/%.2f)", sc, 1.0/sc);
            if (qs != 1.0F) DETAILS2("Divide by quantizer scale %f (=1/%.2f)", qs, 1.0/qs);
            if (zp) DETAILS2("Add quantizer zero-point %.2f", zp);
            if (alpha == 1.0 && beta == 0.0) DETAILS2("Summary: out = in");
            else if (alpha == 1.0) DETAILS2("Summary: out = in%+f", beta);
            else if (beta == 0.0) DETAILS2("Summary: out = in*%f", alpha);
            else DETAILS2("Summary: out = in*%f%+f", alpha, beta);
          }
        }
      }
    }

    if (notdone)
    {
      // This is the slowest path... you should add optimizations above for some specific cases:
      blob.convertTo(blob, CV_32F);
      DETAILS("Convert to 32F");

      // Apply mean and scale:
      if (m != cv::Scalar())
      {
        blob -= m;
        DETAILS("Subtract mean [%.2f %.2f %.2f]", m[0], m[1], m[2]);
      }
      
      if (sd != cv::Scalar(1.0F, 1.0F, 1.0F))
      {
        if (sd[0] == 0.0F || sd[1] == 0.0F || sd[2] == 0.0F) LFATAL("Parameter stdev cannot contain any zero");
        if (sc != 1.0F && sc != 0.0F)
        {
          sd *= 1.0F / sc;
          DETAILS("Divide stdev by scale %f (=1/%.2f)", sc, 1.0/sc);
        }
        blob /= sd;
        DETAILS("Divide by stdev [%f %f %f]", sd[0], sd[1], sd[2]);
      }
      else if (sc != 1.0F)
      {
        blob *= sc;
        DETAILS("Multiply by scale %f (=1/%.2f)", sc, 1.0/sc);
      }

      if (tt == CV_16F || tt == CV_64F)
      {
        blob.convertTo(blob, tt);
        DETAILS("Convert to %s", jevois::dnn::attrstr(attr).c_str());
      }
      else if (tt != CV_32F)
      {
        blob = jevois::dnn::quantize(blob, attr);
        DETAILS("Quantize to %s", jevois::dnn::attrstr(attr).c_str());
      }
    }

    // --------------------------------------------------------------------------------
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
      // If fmt type is auto (e.g., ONNX runtime), guess it as NCHW or NHWC based on dims:
      vsi_nn_dim_fmt_e fmt = attr.dtype.fmt;
      if (fmt == VSI_NN_DIM_FMT_AUTO)
      {
        if (attr.size[0] > attr.size[2]) fmt = VSI_NN_DIM_FMT_NCHW;
        else fmt = VSI_NN_DIM_FMT_NHWC;
      }
      
      switch (fmt)
      {
      case VSI_NN_DIM_FMT_NCHW:
      {
        // Convert from packed to planar:
        int dims[] = { 1, nch, blob.rows, blob.cols };
        cv::Mat newblob(4, dims, tt);

        // Create some pointers in newblob for each channel:
        cv::Mat nbc[nch];
        for (int i = 0; i < nch; ++i) nbc[i] = cv::Mat(blob.rows, blob.cols, tt, newblob.ptr(0, i));
        if (swaprb)
        {
          std::swap(nbc[0], nbc[2]);
          DETAILS("Swap Red <-> Blue");
        }
        
        // Split:
        cv::split(blob, nbc);
        DETAILS("Split channels (NHWC->NCHW)");

        // This our final 4D blob:
        blob = newblob;
      }
      break;

      case VSI_NN_DIM_FMT_NHWC:
      {
        // red/blue byte swap was handled above... Just convert to a 4D blob:
        blob = blob.reshape(1, { 1, bsiz.height, bsiz.width, 3 });
      }
      break;

      default: LFATAL("Can only handle NCHW or NHWC intensors shapes");
      }
    }
    break;

    default: LFATAL("Can only handle input images with 1, 3, or 4 channels");
    }
    
    // --------------------------------------------------------------------------------
    // Done with this blob:
    DETAILS("%s", jevois::dnn::attrstr(attr).c_str());
    blobs.emplace_back(blob);
    crops.emplace_back(crop);
    ++bnum;


    // --------------------------------------------------------------------------------
    // NOTE: in principle, our code here is ready to generate several blobs.
    // However, in practice all nets tested so far expect just one input, since they are machine vision models, except
    // for URetinex-Net, which expects an image and a single float. Thus, here, we only generate the first blob
    // (when numin param is at its default value of 1, otherwise up to numin blobs).
    if (bnum >= numin::get()) break;
  }
  return blobs;
}

// ####################################################################################################
void jevois::dnn::PreProcessorBlob::report(jevois::StdModule *, jevois::RawImage *, jevois::OptGUIhelper * helper,
                                           bool /*overlay*/, bool idle)
{
#ifdef JEVOIS_PRO
    if (helper && idle == false)
      for (std::string const & s : itsInfo) ImGui::BulletText("%s", s.c_str());
#else
    (void)helper; (void)idle;
#endif
}

