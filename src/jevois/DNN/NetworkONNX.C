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

#ifdef JEVOIS_PRO

#include <jevois/DNN/NetworkONNX.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>

// ####################################################################################################
jevois::dnn::NetworkONNX::NetworkONNX(std::string const & instance) :
    jevois::dnn::Network(instance),
    itsEnv(ORT_LOGGING_LEVEL_WARNING, "NetworkONNX")
{
  itsSessionOptions.SetIntraOpNumThreads(4);
  itsSessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
}

// ####################################################################################################
jevois::dnn::NetworkONNX::~NetworkONNX()
{ waitBeforeDestroy(); }

// ####################################################################################################
void jevois::dnn::NetworkONNX::freeze(bool doit)
{
  dataroot::freeze(doit);
  model::freeze(doit);
  jevois::dnn::Network::freeze(doit); // base class parameters
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkONNX::inputShapes()
{
  if (ready() == false) LFATAL("Network is not ready");
  return itsInAttrs;
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkONNX::outputShapes()
{
  if (ready() == false) LFATAL("Network is not ready");
  return itsOutAttrs;
}

// ####################################################################################################
void jevois::dnn::NetworkONNX::load()
{
  // Need to nuke the network first if it exists or we could run out of RAM:
  if (! itsSession) itsSession.reset();

  std::string const m = jevois::absolutePath(dataroot::get(), model::get());
  LINFO("Loading " << m << " ...");
    
  // Create and load the network:
  itsSession.reset(new Ort::Session(itsEnv, m.c_str(), itsSessionOptions));
  itsInAttrs.clear();
  itsOutAttrs.clear();
  itsInNamePtrs.clear();
  itsInNames.clear();
  itsOutNamePtrs.clear();
  itsOutNames.clear();
  
  // Print information about inputs:
  size_t const num_input_nodes = itsSession->GetInputCount();
  Ort::AllocatorWithDefaultOptions allocator;
  LINFO("Network has " << num_input_nodes << " inputs:");
  for (size_t i = 0; i < num_input_nodes; ++i)
  {
    Ort::AllocatedStringPtr input_name = itsSession->GetInputNameAllocated(i, allocator);
    Ort::TypeInfo const type_info = itsSession->GetInputTypeInfo(i);
    Ort::ConstTensorTypeAndShapeInfo const tensor_info = type_info.GetTensorTypeAndShapeInfo();
    LINFO("- Input " << i << " [" << input_name.get() << "]: " << jevois::dnn::shapestr(tensor_info));
    itsInAttrs.emplace_back(jevois::dnn::tensorattr(tensor_info));
    itsInNames.emplace_back(input_name.get());
    itsInNamePtrs.emplace_back(std::move(input_name));
  }
  
  // Print information about outputs:
  size_t const num_output_nodes = itsSession->GetOutputCount();
  LINFO("Network has " << num_output_nodes << " outputs:");
  for (size_t i = 0; i < num_output_nodes; ++i)
  {
    Ort::AllocatedStringPtr output_name = itsSession->GetOutputNameAllocated(i, allocator);
    Ort::TypeInfo const type_info = itsSession->GetOutputTypeInfo(i);
    Ort::ConstTensorTypeAndShapeInfo const tensor_info = type_info.GetTensorTypeAndShapeInfo();
    LINFO("- Output " << i << " [" << output_name.get() << "]: " << jevois::dnn::shapestr(tensor_info));
    itsOutAttrs.emplace_back(jevois::dnn::tensorattr(tensor_info));
    itsOutNames.emplace_back(output_name.get());
    itsOutNamePtrs.emplace_back(std::move(output_name));
  }
  LINFO("Network " << m << " ready.");
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkONNX::doprocess(std::vector<cv::Mat> const & blobs,
                                                         std::vector<std::string> & info)
{
  if (! itsSession) LFATAL("Internal inconsistency");

  if (blobs.size() != itsInAttrs.size())
    LFATAL("Received " << blobs.size() << " inputs but network wants " << itsInAttrs.size());

  // Create input tensor objects from input blobs:
  std::vector<Ort::Value> inputs;
  std::vector<char const *> input_node_names;
  for (size_t i = 0; i < itsInAttrs.size(); ++i)
  {
    vsi_nn_tensor_attr_t const & attr = itsInAttrs[i];
    cv::Mat const & m = blobs[i];

    if (jevois::dnn::vsi2cv(attr.dtype.vx_type) != m.type())
      LFATAL("Input " << i << " has type " << jevois::cvtypestr(m.type()) <<
             " but network wants " << jevois::dnn::attrstr(attr));
    
    std::vector<int64_t> dims; size_t sz = jevois::cvBytesPerPix(m.type());
    for (size_t k = 0; k < attr.dim_num; ++k)
    {
      dims.emplace_back(attr.size[attr.dim_num - 1 - k]);
      sz *= attr.size[attr.dim_num - 1 - k];
    }
    
    if (sz != m.total() * m.elemSize())
      LFATAL("Input " << i << " size mismatch: got " << jevois::dnn::shapestr(m) <<
             " but network wants " << jevois::dnn::shapestr(attr));
    
    Ort::MemoryInfo meminfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    switch (attr.dtype.vx_type)
    {
    case VSI_NN_TYPE_FLOAT32:
      inputs.emplace_back(Ort::Value::CreateTensor<float>(meminfo, reinterpret_cast<float *>(m.data),
                                                          sz, dims.data(), dims.size()));
      break;
      
    case VSI_NN_TYPE_UINT8:
      inputs.emplace_back(Ort::Value::CreateTensor<uint8_t>(meminfo, reinterpret_cast<uint8_t *>(m.data),
                                                            sz, dims.data(), dims.size()));
      break;
      
    case VSI_NN_TYPE_INT8:
      inputs.emplace_back(Ort::Value::CreateTensor<int8_t>(meminfo, reinterpret_cast<int8_t *>(m.data),
                                                           sz, dims.data(), dims.size()));
      break;
      
    case VSI_NN_TYPE_UINT32:
      inputs.emplace_back(Ort::Value::CreateTensor<uint32_t>(meminfo, reinterpret_cast<uint32_t *>(m.data),
                                                             sz, dims.data(), dims.size()));
      break;
      
    case VSI_NN_TYPE_INT32:
      inputs.emplace_back(Ort::Value::CreateTensor<int32_t>(meminfo, reinterpret_cast<int32_t *>(m.data),
                                                            sz, dims.data(), dims.size()));
      break;
      
    default:
      LFATAL("Sorry, input tensor type " << jevois::dnn::attrstr(attr) << " is not yet supported...");
    }
    if (inputs.back().IsTensor() == false) LFATAL("Failed to create tensor for input " << i);
  }
  
  // Run inference:
  itsOutputs = itsSession->Run(Ort::RunOptions{nullptr}, itsInNames.data(), inputs.data(), inputs.size(),
                               itsOutNames.data(), itsOutNames.size());
  if (itsOutputs.size() != itsOutNames.size())
    LFATAL("Received " << itsOutputs.size() << " outputs but network should produce " << itsOutNames.size());
  
  // Convert output tensors to cv::Mat with zero-copy:
  std::vector<cv::Mat> outs;
  for (size_t i = 0; i < itsOutputs.size(); ++i)
  {
    Ort::Value & out = itsOutputs[i];
    vsi_nn_tensor_attr_t const & attr = itsOutAttrs[i];
    if (out.IsTensor() == false) LFATAL("Network produced a non-tensor output " << i);

    switch (attr.dtype.vx_type)
    {
    case VSI_NN_TYPE_FLOAT32:
      outs.emplace_back(cv::Mat(jevois::dnn::attrmat(itsOutAttrs[i], out.GetTensorMutableData<float>())));
      break;
      
    case VSI_NN_TYPE_UINT8:
      outs.emplace_back(cv::Mat(jevois::dnn::attrmat(itsOutAttrs[i], out.GetTensorMutableData<uint8_t>())));
      break;
      
    case VSI_NN_TYPE_INT8:
      outs.emplace_back(cv::Mat(jevois::dnn::attrmat(itsOutAttrs[i], out.GetTensorMutableData<int8_t>())));
      break;
      
    case VSI_NN_TYPE_UINT32:
      outs.emplace_back(cv::Mat(jevois::dnn::attrmat(itsOutAttrs[i], out.GetTensorMutableData<uint32_t>())));
      break;
      
    case VSI_NN_TYPE_INT32:
      outs.emplace_back(cv::Mat(jevois::dnn::attrmat(itsOutAttrs[i], out.GetTensorMutableData<int32_t>())));
      break;
      
    default:
      LFATAL("Sorry, output tensor type " << jevois::dnn::attrstr(attr) << " is not yet supported...");
    }
  }
  
  info.emplace_back("Forward Network OK");
  
  return outs;
}

#endif // JEVOIS_PRO
