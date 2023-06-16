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

#include <jevois/DNN/Network.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Async.H>

// ####################################################################################################
jevois::dnn::Network::~Network()
{ }

// ####################################################################################################
void jevois::dnn::Network::freeze(bool doit)
{
  comment::freeze(doit);
  url::freeze(doit);
  extraintensors::freeze();
}

// ####################################################################################################
void jevois::dnn::Network::onParamChange(network::outreshape const & JEVOIS_UNUSED_PARAM(param),
                                         std::string const & val)
{
  itsReshape.clear();
  if (val.empty()) return;

  itsReshape = jevois::dnn::parseTensorSpecs(val);
}
  
// ####################################################################################################
void jevois::dnn::Network::waitBeforeDestroy()
{
  // Do not destroy a network that is loading, and do not throw...
  size_t count = 0;
  while (itsLoading.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    try { if (ready()) break; } catch (...) { }
    if (count++ == 200) { LINFO("Waiting for network load to complete..."); count = 0; }
  }
}

// ####################################################################################################
bool jevois::dnn::Network::ready()
{
  // If we are loaded, we are ready to process:
  if (itsLoaded.load()) return true;
  
  // If we are loading, check whether loading is complete or threw, otherwise return false as we keep loading:
  if (itsLoading.load())
  {
    if (itsLoadFut.valid() && itsLoadFut.wait_for(std::chrono::milliseconds(2)) == std::future_status::ready)
    {
      try { itsLoadFut.get(); itsLoaded.store(true); itsLoading.store(false); LINFO("Network loaded."); return true; }
      catch (...) { itsLoading.store(false); jevois::warnAndRethrowException(); }
    }
    return false;
  }
  
  // Otherwise, trigger an async load:
  itsLoading.store(true);
  itsLoadFut = jevois::async(std::bind(&jevois::dnn::Network::load, this));
  LINFO("Loading network...");

  return false;
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::Network::process(std::vector<cv::Mat> const & blobs,
                                                   std::vector<std::string> & info)
{
  if (ready() == false) LFATAL("Network is not ready");

  std::vector<cv::Mat> outs;
  std::string const c = comment::get();
  
  // Add any extra input tensors?
  std::string const extra = extraintensors::get();
  if (extra.empty() == false)
  {
    std::vector<cv::Mat> newblobs = blobs;
      
    std::vector<std::string> ins = jevois::split(extra, ",\\s*");
    for (std::string const & in : ins)
    {
      vsi_nn_tensor_attr_t attr; memset(&attr, 0, sizeof(attr));

      std::vector<std::string> tok = jevois::split(in, ":");
      if (tok.size() != 3)
        LFATAL("Malformed extra tensor, need <type>:<shape>:val1 val2 ... valN (separate multiple tensors by comma)");

      // Decode type and convert to vsi, only those types that OpenCV can support:
      if (tok[0] == "8U") attr.dtype.vx_type = VSI_NN_TYPE_UINT8;
      else if (tok[0] == "8S") attr.dtype.vx_type = VSI_NN_TYPE_INT8;
      else if (tok[0] == "16U") attr.dtype.vx_type = VSI_NN_TYPE_UINT16;
      else if (tok[0] == "16S") attr.dtype.vx_type = VSI_NN_TYPE_INT16;
      else if (tok[0] == "16F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT16;
      else if (tok[0] == "32S") attr.dtype.vx_type = VSI_NN_TYPE_INT32;
      else if (tok[0] == "32F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT32; 
      else if (tok[0] == "64F") attr.dtype.vx_type = VSI_NN_TYPE_FLOAT64; 
      else throw std::range_error("Unsupported extra input tensor type [" + tok[0] + "] in " + extra);

      // Decode the dims:
      std::vector<size_t> dims = jevois::dnn::strshape(tok[1]);
      attr.dim_num = dims.size();
      for (size_t i = 0; i < attr.dim_num; ++i) attr.size[attr.dim_num - 1 - i] = dims[i];

      // Allocate the tensor:
      attr.dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
      attr.dtype.fmt = VSI_NN_DIM_FMT_AUTO;
      cv::Mat b = jevois::dnn::attrmat(attr);

      // Populate the values:
      std::vector<std::string> vals = jevois::split(tok[2], "\\s+");
      size_t const nvals = vals.size();
      if (nvals != b.total())
        LFATAL("Extra in tensor needs " << b.total() << " values, but " << nvals << " given in [" << in << ']');
      switch (attr.dtype.vx_type)
      {
      case VSI_NN_TYPE_UINT8:
      {
        uint8_t * ptr = reinterpret_cast<uint8_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;

      case VSI_NN_TYPE_INT8:
      {
        int8_t * ptr = reinterpret_cast<int8_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;
      
      case VSI_NN_TYPE_UINT16:
      {
        uint16_t * ptr = reinterpret_cast<uint16_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;

      case VSI_NN_TYPE_INT16:
      {
        int16_t * ptr = reinterpret_cast<int16_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;
      
      case VSI_NN_TYPE_FLOAT16:
      {
        cv::float16_t * ptr = reinterpret_cast<cv::float16_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = cv::float16_t(std::stof(v));
      }
      break;

      case VSI_NN_TYPE_INT32:
      {
        int32_t * ptr = reinterpret_cast<int32_t *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stoi(v);
      }
      break;

      case VSI_NN_TYPE_FLOAT32:
      {
        float * ptr = reinterpret_cast<float *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stof(v);
      }
      break;

      case VSI_NN_TYPE_FLOAT64:
      {
        double * ptr = reinterpret_cast<double *>(b.data);
        for (std::string const & v : vals) *ptr++ = std::stod(v);
      }
      break;
      
      default: LFATAL("internal inconsistency");
      }
      
      newblobs.emplace_back(std::move(b));
    }

    // NOTE: Keep the code below in sync with the default case (no extra inputs). Both branches are duplicated to avoid
    // having to make a copy of blobs into newblobs in the standard case when we do not have any extra inputs:
    
    // Show info about input tensors:
    info.emplace_back("* Input Tensors");
    for (cv::Mat const & b : newblobs) info.emplace_back("- " + jevois::dnn::shapestr(b));

    // Run processing on the derived class:
    info.emplace_back("* Network");
    if (c.empty() == false) info.emplace_back(c);
  
    outs = std::move(doprocess(newblobs, info));
  }
  else
  {
    // Show info about input tensors:
    info.emplace_back("* Input Tensors");
    for (cv::Mat const & b : blobs) info.emplace_back("- " + jevois::dnn::shapestr(b));
    
    // Run processing on the derived class:
    info.emplace_back("* Network");
    if (c.empty() == false) info.emplace_back(c);
    
    outs = std::move(doprocess(blobs, info));
  }
    
  // Show info about output tensors:
  info.emplace_back("* Output Tensors");
  for (size_t i = 0; i < outs.size(); ++i) info.emplace_back("- " + jevois::dnn::shapestr(outs[i]));

  // Possibly reshape the tensors:
  if (itsReshape.empty() == false)
  {
    if (itsReshape.size() != outs.size())
      LFATAL("Received " << outs.size() << " but outreshape has " << itsReshape.size() << " entries");

    info.emplace_back("* Reshaped Output Tensors");
    for (size_t i = 0; i < outs.size(); ++i)
    {
      outs[i] = outs[i].reshape(1, jevois::dnn::attrdims(itsReshape[i]));
      info.emplace_back("- " + jevois::dnn::shapestr(outs[i]));
    }
  }
  
  return outs;
}
