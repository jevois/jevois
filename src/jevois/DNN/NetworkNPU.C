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
#include <vsi_nn_version.h>
#include <filesystem>

#define VNN_APP_DEBUG (FALSE)

/*-------------------------------------------
                   Macros
 -------------------------------------------*/

#define NEW_VXNODE(_node, _type, _in, _out, _uid) do {          \
    _node = vsi_nn_AddNode(itsGraph, _type, _in, _out, NULL);   \
    _node->uid = (uint32_t)_uid;                                \
    if (NULL == _node) LFATAL("NEW_VXNODE failed");             \
  } while(0)

#define NEW_VIRTUAL_TENSOR(_id, _attr, _dtype) do {                     \
    memset(_attr.size, 0, VSI_NN_MAX_DIM_NUM * sizeof(vsi_size_t));     \
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

  for (int tnum = 0; vsi_nn_tensor_attr_t & attr : attrs)
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
  model::freeze(doit);
  intensors::freeze(doit);
  outtensors::freeze(doit);
  ovxver::freeze(doit);
  jevois::dnn::Network::freeze(doit); // base class parameters
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

  if (ovxver::get().empty() == false)
  {
    std::vector<std::string> tok = jevois::split(ovxver::get(), "\\.");
    if (tok.size() != 3) LFATAL("Malformed ovxver version [" << ovxver::get() <<"] -- should be x.y.z");
    vsi_nn_SetGraphVersion(itsGraph, std::stoi(tok[0]), std::stoi(tok[1]), std::stoi(tok[2]));
  }
  else
    vsi_nn_SetGraphVersion(itsGraph, VSI_NN_VERSION_MAJOR, VSI_NN_VERSION_MINOR, VSI_NN_VERSION_PATCH);

  vsi_nn_SetGraphInputs(itsGraph, NULL, numin);
  vsi_nn_SetGraphOutputs(itsGraph, NULL, numout);

  LINFO("Created graph with " << numin << " inputs and " << numout << " outputs");
  
  // Get NBG file name:
  std::string const m = jevois::absolutePath(dataroot::get(), model::get());

  // Check that the file does exist so we avoid cryptic errors if not:
  if (std::filesystem::exists(m) == false) LFATAL("Missing network file " << m << " -- ABORT");
  
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
  if (status != VSI_SUCCESS)
    LFATAL("Failed to setup graph -- Possible causes:\n"
           "- Incorrect intensors/outtensors in your YAML file?\n"
           "- Wrong NPU model? Check --optimize VIPNANOQI_PID0X88\n"
           "- Wrong NPU SDK version? Running ovxlib " <<
           VSI_NN_VERSION_MAJOR << '.' << VSI_NN_VERSION_MINOR << '.' << VSI_NN_VERSION_PATCH);
  LINFO("Graph ready.");

  if (verifygraph::get())
  {
    status = vsi_nn_VerifyGraph(itsGraph);
    if (status != VSI_SUCCESS) LFATAL("Graph verification failed -- \n"
                                      "check that intensors/outtensors specs exactly match\n"
                                      "those provided during model conversion.");
    else LINFO("Graph verification ok");
  }
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
    auto const & iattr = tensor->attr;

    // Check that blob and tensor are a complete match:
    if (jevois::dnn::attrmatch(iattr, blob) == false)
      LFATAL("Input " << b << ": received " << jevois::dnn::shapestr(blob) <<
             " but want: " << jevois::dnn::shapestr(iattr));

    // Copy blob data to tensor:
    auto status = vsi_nn_CopyDataToTensor(itsGraph, tensor, (uint8_t *)blob.data);
    if (status != VSI_SUCCESS) LFATAL("Error setting input tensor: " << status);

    info.emplace_back("- In " + std::to_string(b) + ": " + jevois::dnn::attrstr(iattr));
  }
    
  // Ok, let's run the network:
  auto status = vsi_nn_RunGraph(itsGraph);
  if (status != VSI_SUCCESS) LFATAL("Error running graph: " << status);
  info.emplace_back("- Network forward pass ok");
  
  // Collect the outputs:
  std::vector<cv::Mat> outs;
  for (uint32_t i = 0; i < itsGraph->output.num; ++i)
  {
    vsi_nn_tensor_t * ot = vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i]);
    vsi_nn_tensor_attr_t const & oattr = ot->attr;
    uint8_t * tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(itsGraph, ot);

    try
    {
      cv::Mat rawout = jevois::dnn::attrmat(oattr, tensor_data);

      if (dequant::get())
      {
        outs.emplace_back(jevois::dnn::dequantize(rawout, oattr));
        info.emplace_back("- Out " + std::to_string(i) + ": " + jevois::dnn::attrstr(oattr) + " -> 32F");
      }
      else
      {
        outs.emplace_back(rawout.clone());
        info.emplace_back("- Out " + std::to_string(i) + ": " + jevois::dnn::attrstr(oattr));
      }
    } catch (...) { vsi_nn_Free(tensor_data); jevois::warnAndRethrowException(); }
    
    vsi_nn_Free(tensor_data);
  }

  return outs;
}

#endif // JEVOIS_PLATFORM_PRO
