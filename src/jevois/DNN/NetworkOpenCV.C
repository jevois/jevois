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

#include <jevois/DNN/NetworkOpenCV.H>
#include <jevois/DNN/Utils.H>

// ####################################################################################################
jevois::dnn::NetworkOpenCV::~NetworkOpenCV()
{ waitBeforeDestroy(); }

// ####################################################################################################
void jevois::dnn::NetworkOpenCV::freeze(bool doit)
{
  dataroot::freeze(doit);
  config::freeze(doit);
  model::freeze(doit);
  backend::freeze(doit);
  target::freeze(doit);
  intensors::freeze(doit);
  outtensors::freeze(doit);
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkOpenCV::inputShapes()
{
  return jevois::dnn::parseTensorSpecs(intensors::get());
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkOpenCV::outputShapes()
{
  return jevois::dnn::parseTensorSpecs(outtensors::get());
}

// ####################################################################################################
void jevois::dnn::NetworkOpenCV::load()
{
  // Need to nuke the network first if it exists or we could run out of RAM:
  if (itsNet.empty() == false) itsNet = cv::dnn::Net();

  std::string const m = jevois::absolutePath(dataroot::get(), model::get());
  std::string const c = jevois::absolutePath(dataroot::get(), config::get());
  
  // Create and load the network:
  itsNet = cv::dnn::readNet(m, c);

  switch(backend::get())
  {
  case network::Backend::Default: itsNet.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT); break;
#ifdef JEVOIS_PRO
  case network::Backend::OpenCV: itsNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV); break;
  case network::Backend::InferenceEngine: itsNet.setPreferableBackend(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE); break;
#endif
  }

  switch(target::get())
  {
  case network::Target::CPU: itsNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU); break;
#ifdef JEVOIS_PRO
  case network::Target::OpenCL: itsNet.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL); break;
  case network::Target::OpenCL_FP16: itsNet.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL_FP16); break;
  case network::Target::Myriad: itsNet.setPreferableTarget(cv::dnn::DNN_TARGET_MYRIAD); break;
#endif
  }

  // Get names of the network's output layers:
  itsOutLayers = itsNet.getUnconnectedOutLayers();
  std::vector<cv::String> layersNames = itsNet.getLayerNames();
  itsOutNames.resize(itsOutLayers.size());
  for (size_t i = 0; i < itsOutLayers.size(); ++i) itsOutNames[i] = layersNames[itsOutLayers[i] - 1];
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkOpenCV::doprocess(std::vector<cv::Mat> const & blobs,
                                                           std::vector<std::string> & info)
{
  if (blobs.size() != 1) LFATAL("Expecting exactly one input blob");

  if (itsNet.empty()) LFATAL("Internal inconsistency");
  
  itsNet.setInput(blobs[0]);
  std::vector<cv::Mat> outs;
  itsNet.forward(outs, itsOutNames);

  // Show some info:
  std::vector<cv::dnn::MatShape> inshapes;
  for (size_t i = 0; i < blobs.size(); ++i)
  {
    cv::dnn::MatShape s; cv::MatSize const & ms = blobs[i].size;
    for (int k = 0; k < ms.dims(); ++k) s.emplace_back(ms[k]);
    inshapes.emplace_back(s);
  }
  info.emplace_back("Forward Network FLOPS: " + std::to_string(itsNet.getFLOPS(inshapes)));
  
  return outs;
}