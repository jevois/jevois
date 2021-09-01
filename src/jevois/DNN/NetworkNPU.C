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

#ifdef JEVOIS_PLATFORM_PRO

#include <jevois/DNN/NetworkNPU.H>
#include <jevois/Util/Utils.H>
#include <jevois/DNN/Utils.H>

// Those are from vnn_inceptionv3.h:
#define VNN_APP_DEBUG (FALSE)
#define VNN_VERSION_MAJOR 1
#define VNN_VERSION_MINOR 1
#define VNN_VERSION_PATCH 21

/*-------------------------------------------
                   Macros
 -------------------------------------------*/

#define NEW_VXNODE(_node, _type, _in, _out, _uid) do {          \
    _node = vsi_nn_AddNode(itsGraph, _type, _in, _out, NULL);   \
    _node->uid = (uint32_t)_uid;                                \
    if (NULL == _node) LFATAL("NEW_VXNODE failed");             \
  } while(0)

#define NEW_VIRTUAL_TENSOR(_id, _attr, _dtype) do {                     \
    memset(_attr.size, 0, VSI_NN_MAX_DIM_NUM * sizeof(uint32_t));       \
    _attr.dim_num = VSI_NN_DIM_AUTO;                                    \
    _attr.vtl = !VNN_APP_DEBUG;                                         \
    _attr.is_const = FALSE;                                             \
    _attr.dtype.vx_type = _dtype;                                       \
    _id = vsi_nn_AddTensor(itsGraph, VSI_NN_TENSOR_ID_AUTO, & _attr, NULL); \
    if (VSI_NN_TENSOR_ID_NA == _id) LFATAL("NEW_VIRTUAL_TENSOR failed"); \
  } while(0)

// Set const tensor dims out of this macro.
#define NEW_CONST_TENSOR(_id, _attr, _dtype, _ofst, _size) do {         \
    data = load_data(fp, _ofst, _size);                                 \
    _attr.vtl = FALSE;                                                  \
    _attr.is_const = TRUE;                                              \
    _attr.dtype.vx_type = _dtype;                                       \
    _id = vsi_nn_AddTensor(itsGraph, VSI_NN_TENSOR_ID_AUTO, & _attr, data); \
    free(data);                                                         \
    if (VSI_NN_TENSOR_ID_NA == _id) LFATAL("NEW_CONST_TENSOR failed");  \
  } while(0)

// Set generic tensor dims out of this macro.
#define NEW_NORM_TENSOR(_id, _attr, _dtype) do {                        \
    _attr.vtl = FALSE;                                                  \
    _attr.is_const = FALSE;                                             \
    _attr.dtype.vx_type = _dtype;                                       \
    _id = vsi_nn_AddTensor(itsGraph, VSI_NN_TENSOR_ID_AUTO, & _attr, NULL); \
    if (VSI_NN_TENSOR_ID_NA == _id) LFATAL("NEW_NORM_TENSOR failed");   \
  } while(0)

// Set generic tensor dims out of this macro.
#define NEW_NORM_TENSOR_FROM_HANDLE(_id, _attr, _dtype) do {            \
    _attr.vtl = FALSE;                                                  \
    _attr.is_const = FALSE;                                             \
    _attr.dtype.vx_type = _dtype;                                       \
    _id = vsi_nn_AddTensorFromHandle(itsGraph, VSI_NN_TENSOR_ID_AUTO, & _attr, NULL); \
    if (VSI_NN_TENSOR_ID_NA == _id) LFATAL("NEW_NORM_TENSOR_FROM_HANDLE failed"); \
  } while(0)

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkNPU::inputShapes()
{
  return jevois::dnn::parseTensorSpecs(intensors::get());
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkNPU::outputShapes()
{
  return jevois::dnn::parseTensorSpecs(outtensors::get());
}

// ####################################################################################################
void jevois::dnn::NetworkNPU::create_tensors(std::vector<vsi_nn_tensor_attr_t> & attrs, vsi_nn_node_t * node, bool isin)
{
  if (attrs.empty()) LFATAL("Invalid empty " << (isin ? "in" : "out") << "tensors specification");

  int tnum = 0;
  for (vsi_nn_tensor_attr_t & attr : attrs)
  {
    // Allocate the tensor:
    vsi_nn_tensor_id_t id;
    NEW_NORM_TENSOR(id, attr, attr.dtype.vx_type);

    // Connect the tensor:
    if (isin)
    {
      node->input.tensors[tnum] = id;
      itsGraph->input.tensors[tnum] = id;
      LINFO("Input tensor " << tnum << ": " << jevois::dnn::attrstr(attr));
    }
    else
    {
      node->output.tensors[tnum] = id;
      itsGraph->output.tensors[tnum] = id;
      LINFO("Output tensor " << tnum << ": " << jevois::dnn::attrstr(attr));
    }
    ++tnum;
  }
}

// ####################################################################################################
jevois::dnn::NetworkNPU::~NetworkNPU()
{
  waitBeforeDestroy();
  if (itsGraph) vsi_nn_ReleaseGraph(&itsGraph);
  if (itsCtx) vsi_nn_ReleaseContext(&itsCtx);
}

// ####################################################################################################
void jevois::dnn::NetworkNPU::freeze(bool doit)
{
  dataroot::freeze(doit);
  config::freeze(doit);
  model::freeze(doit);
  intensors::freeze(doit);
  outtensors::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::NetworkNPU::load()
{
  // Need to nuke the network first if it exists or we could run out of RAM:
  if (itsGraph) { vsi_nn_ReleaseGraph(&itsGraph); itsGraph = nullptr; }

  // Create context if needed:
  if (itsCtx == 0) itsCtx = vsi_nn_CreateContext();

  // Parse input and output tensor specs:
  std::vector<vsi_nn_tensor_attr_t> iattrs = jevois::dnn::parseTensorSpecs(intensors::get());
  std::vector<vsi_nn_tensor_attr_t> oattrs = jevois::dnn::parseTensorSpecs(outtensors::get());
  size_t const numin = iattrs.size();
  size_t const numout = oattrs.size();
  
  // Create graph:
  itsGraph = vsi_nn_CreateGraph(itsCtx, numin + numout * 2, 1);
  if (itsGraph == NULL) LFATAL("Graph creation failed");
  vsi_nn_SetGraphVersion(itsGraph, VNN_VERSION_MAJOR, VNN_VERSION_MINOR, VNN_VERSION_PATCH);
  vsi_nn_SetGraphInputs(itsGraph, NULL, numin);
  vsi_nn_SetGraphOutputs(itsGraph, NULL, numout);
  LINFO("Created graph with " << numin << " inputs and " << numout << " outputs");
  
  // Get NBG file name:
  std::string const m = jevois::absolutePath(dataroot::get(), model::get());

  // Initialize node:
  vsi_nn_node_t * node[1];
  NEW_VXNODE(node[0], VSI_NN_OP_NBG, numin, numout, 0);
  node[0]->nn_param.nbg.type = VSI_NN_NBG_FILE;
  node[0]->nn_param.nbg.url = m.c_str();

  // Create input and output tensors and attach them to the node and graph:
  create_tensors(oattrs, node[0], false);
  create_tensors(iattrs, node[0], true);
  
  // Setup the graph:
  auto status = vsi_nn_SetupGraph(itsGraph, FALSE);
  if (status != VSI_SUCCESS) LFATAL("Failed to setup graph");
  LINFO("Graph ready.");
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkNPU::doprocess(std::vector<cv::Mat> const & blobs,
                                                        std::vector<std::string> & info)
{
  if (blobs.size() != itsGraph->input.num)
    LFATAL("Received " << blobs.size() << " blobs, but network has " << itsGraph->input.num << " inputs");

  for (size_t b = 0; b < blobs.size(); ++b)
  {
    cv::Mat const & blob = blobs[b];
  
    // Get the input tensor:
    vsi_nn_tensor_t * tensor = vsi_nn_GetTensor(itsGraph, itsGraph->input.tensors[b]);
    if (tensor == nullptr) LFATAL("Network does not have input tensor " << b);
    auto const & attr = tensor->attr;

    // Check that blob and tensor are a complete match:
    bool ok = blob.channels() == 1 &&
      blob.depth() == jevois::dnn::vsi2cv(tensor->attr.dtype.vx_type) &&
      uint32_t(blob.size.dims()) == attr.dim_num;
    if (ok)
      for (size_t i = 0; i < attr.dim_num; ++i)
        if (int(attr.size[attr.dim_num - 1 - i]) != blob.size[i]) { ok = false; break; }
    
    if (ok == false)
      LFATAL("Input " << b << ": received " << jevois::dnn::shapestr(blob) <<
             " but want: " << jevois::dnn::shapestr(attr));

    // Copy blob data to tensor:
    auto status = vsi_nn_CopyDataToTensor(itsGraph, tensor, (uint8_t *)blob.data);
    if (status != VSI_SUCCESS) LFATAL("Error setting input tensor: " << status);
    info.emplace_back("- Copied " + jevois::dnn::shapestr(blob) + " to input tensor " + std::to_string(b));
  }
    /*
    // Convert blob to dtype if needed:
    switch (blob.type())
    {
    case CV_8U:
    {
      if (tensor->attr.dtype.vx_type == VSI_NN_TYPE_UINT8)
      {
        auto status = vsi_nn_CopyDataToTensor(itsGraph, tensor, blob.data);
        if (status != VSI_SUCCESS) LFATAL("Error setting input tensor: " << status);
        info.emplace_back("- Copied UINT8 data to input tensor");
      }
      else if (tensor->attr.dtype.vx_type == VSI_NN_TYPE_INT8)
      {
        uint8_t const * bdata = (uint8_t const *)blob.data;
        uint32_t sz = vsi_nn_GetElementNum(tensor);
        uint32_t stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
        int8_t * data = (int8_t *)malloc(stride * sz * sizeof(uint8_t));
        int const shift = 8 - tensor->attr.dtype.fl;
        for (uint32_t i = 0; i < sz; i++) data[i] = bdata[i] >> shift;
        
        // Copy the Pre-processed data to input tensor:
        auto status = vsi_nn_CopyDataToTensor(itsGraph, tensor, (uint8_t *)data);
        free(data);
        if (status != VSI_SUCCESS) LFATAL("Error setting input tensor: " << status);
        info.emplace_back("- Converted UINT8 input tensor to INT8");
      }
    }
    break;
    
    case CV_32F:
    {
      float const * fdata = (float const *)blob.data;
      uint32_t sz = vsi_nn_GetElementNum(tensor);
      uint32_t stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
      uint8_t * data = (uint8_t *)malloc(stride * sz * sizeof(uint8_t));
      for (uint32_t i = 0; i < sz; i++) vsi_nn_Float32ToDtype(fdata[i], &data[stride * i], &tensor->attr.dtype);
      
      // Copy the Pre-processed data to input tensor:
      auto status = vsi_nn_CopyDataToTensor(itsGraph, tensor, data);
      free(data);
      if (status != VSI_SUCCESS) LFATAL("Error setting input tensor: " << status);
      info.emplace_back("- Converted FLOAT input tensor to NPU data type");
    }
    break;
    
    default: LFATAL("FIXME unsupported input blob type");
    }
  }
    */
    
  // Ok, let's run the network:
  auto status = vsi_nn_RunGraph(itsGraph);
  if (status != VSI_SUCCESS) LFATAL("Error running graph: " << status);
  info.emplace_back("- Network forward pass ok");
  
  // Collect the outputs:
  std::vector<cv::Mat> outs;
  if (flattenoutputs::get())
  {
    // Concatenate all outputs into one big float vector:
    uint32_t nout = itsGraph->output.num;
    uint32_t sz[nout]; // total number of elements in each output
    uint32_t totsz = 0; // total number of elements over all outputs
    for (size_t i = 0; i < nout; ++i)
    {
      vsi_nn_tensor_t * tensor = vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i]);
      sz[i] = 1;
      for (uint32_t j = 0; j < tensor->attr.dim_num; ++j) sz[i] *= tensor->attr.size[j];
      totsz += sz[i];
    }
    
    cv::Mat out(1, totsz, CV_32F);
    float * buffer = (float *)out.data;

    for (uint32_t i = 0; i < nout; ++i)
    {
      vsi_nn_tensor_t * tensor = vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i]);
      uint32_t stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
      uint8_t * tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(itsGraph, tensor);

      // FIXME: i guess this should be replaced by more general vsi_nn_DtypeToFloat32 like below
      float fl = pow(2.0F, -tensor->attr.dtype.fl);
      for (uint32_t j = 0; j < sz[i]; j++) {
        int val = tensor_data[stride*j];
        int tmp1 = (val >= 128) ? val-256 : val;
        *buffer++ = tmp1 * fl;
      }
      vsi_nn_Free(tensor_data);
      info.emplace_back("- Converted output tensor " + std::to_string(i) + " to FLOAT");
    }
    info.emplace_back("- Concatenated " + std::to_string(nout) + " outputs to 1x" + std::to_string(totsz) + " FLOAT");
    outs.emplace_back(out);
  }
  else
  {
    // Convert all outputs to one float Mat each:
    for (uint32_t i = 0; i < itsGraph->output.num; ++i)
    {
      vsi_nn_tensor_t * ot = vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i]);
      size_t const ndim = ot->attr.dim_num;
      int cvdims[ndim]; uint32_t sz = 1;
      for (uint32_t i = 0; i < ndim; ++i) { cvdims[ndim - 1 - i] = ot->attr.size[i]; sz *= ot->attr.size[i]; }
      cv::Mat out(ndim, cvdims, CV_32F);
      
      uint32_t stride = vsi_nn_TypeGetBytes(ot->attr.dtype.vx_type);
      uint8_t * tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(itsGraph, ot);
      float * buffer = (float *)out.data;
      
      for (uint32_t i = 0; i < sz; ++i) vsi_nn_DtypeToFloat32(&tensor_data[stride * i], &buffer[i], &ot->attr.dtype);
      
      vsi_nn_Free(tensor_data);
      outs.emplace_back(out);
      info.emplace_back("- Converted output tensor " + std::to_string(i) + " to FLOAT");
    }
  }

  return outs;
}

#endif // JEVOIS_PLATFORM_PRO
