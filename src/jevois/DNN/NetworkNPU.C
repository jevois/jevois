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
#include <jevois/Core/DynamicLoader.H>
#include <jevois/Debug/Timer.H>
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

// From code generated during model conversion (vnn_global.h):
typedef struct {
    uint32_t graph_input_idx;
    vsi_nn_preprocess_base_t *preprocesses;
    uint32_t preprocess_count;
} vsi_nn_preprocess_map_element_t;


typedef struct {
    uint32_t graph_output_idx;
    vsi_nn_postprocess_base_t *postprocesses;
    uint32_t postprocess_count;
} vsi_nn_postprocess_map_element_t;

#ifndef VSI_SIZE_T
typedef uint32_t vsi_size_t;
typedef int32_t vsi_ssize_t;
#endif

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkNPU::inputShapes()
{
  // If using a library, get the tensors from the graph after it has been loaded:
  if (library::get().empty() == false)
  {
    if (ready() == false) LFATAL("Network is not ready");
    std::vector<vsi_nn_tensor_attr_t> ret;
    for (uint32_t i = 0; i < itsGraph->input.num; ++i)
    {
      ret.emplace_back(vsi_nn_GetTensor(itsGraph, itsGraph->input.tensors[i])->attr);

      // When loading a net from library obtained with the Khadas convert tool, input fmt seems to always be NCHW even
      // when the dims disagree... So fix that up here to avoid confusing the pre-processor:
      if (library::get().empty() == false && ret.back().dim_num >= 3)
      {
        vsi_nn_tensor_attr_t & attr = ret.back();
        switch (attr.dtype.fmt)
        {
        case VSI_NN_DIM_FMT_NCHW:
          if (attr.size[2] /* C */ > attr.size[0] /* W */) attr.dtype.fmt = VSI_NN_DIM_FMT_NHWC;
          break;
        case VSI_NN_DIM_FMT_NHWC:
          if (attr.size[0] /* C */ > attr.size[1] /* W */) attr.dtype.fmt = VSI_NN_DIM_FMT_NCHW;
          break;
        default: break;
        }
      }
    }
    return ret;
  }

  // Not using library, tensors have been specified by user:
  return jevois::dnn::parseTensorSpecs(intensors::get());
}

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkNPU::outputShapes()
{
  // If using a library, get the tensors from the graph after it has been loaded:
  if (library::get().empty() == false)
  {
    if (ready() == false) LFATAL("Network is not ready");
    std::vector<vsi_nn_tensor_attr_t> ret;
    for (uint32_t i = 0; i < itsGraph->output.num; ++i)
      ret.emplace_back(vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i])->attr);
    return ret;
  }

  // Not using library, tensors have been specified by user:
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

  // itsLibLoader destructor will close the library if we opened one.
}

// ####################################################################################################
void jevois::dnn::NetworkNPU::freeze(bool doit)
{
  dataroot::freeze(doit);
  model::freeze(doit);
  library::freeze(doit);
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

  // Get NBG file name:
  std::string const m = jevois::absolutePath(dataroot::get(), model::get());

  // Check that the file does exist so we avoid cryptic errors if not:
  if (std::filesystem::exists(m) == false) LFATAL("Missing network file " << m << " -- ABORT");
  
  // Use a library to instantiate the graph, or parse tensor specs from intensors and outtensors and instantiate the
  // graph from those?
  std::string libname = library::get();
  if (libname.empty())
  {
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
  }
  else
  {
    // Using a library. It should contain 2 functions: vnn_CreateModel() which we will use, and vnn_ReleaseModel() which
    // we will not use as it does the same as our destructor:
    std::string const libpath = jevois::absolutePath(dataroot::get(), libname);
    if (std::filesystem::exists(libpath) == false) LFATAL("Missing library file " << libpath << " -- ABORT");
    itsLibLoader.reset(new jevois::DynamicLoader(libpath, true /* close on destroy */));

    typedef vsi_nn_graph_t*(signature)(char const * /* data_file_name */,
                                       vsi_nn_context_t /* in_ctx */,
                                       vsi_nn_preprocess_map_element_t const * /* pre_process_map */,
                                       uint32_t /* pre_process_map_count */,
                                       vsi_nn_postprocess_map_element_t const * /* post_process_map */,
                                       uint32_t /* post_process_map_count */);
    
    auto createmodel = itsLibLoader->load<signature>("vnn_CreateModel");

    itsGraph = createmodel(m.c_str(), itsCtx, nullptr, 0, nullptr, 0);
    // vnn_GetPreProcessMap(), vnn_GetPreProcessMapCount(),
    // vnn_GetPostProcessMap(), vnn_GetPostProcessMapCount());

    if (itsGraph == NULL)
      LFATAL("Graph creation using library failed:\n"
             "- Wrong NPU model? Use kboard=VIM3 in convert\n"
             "- Wrong NPU SDK version? Running ovxlib " <<
             VSI_NN_VERSION_MAJOR << '.' << VSI_NN_VERSION_MINOR << '.' << VSI_NN_VERSION_PATCH);
    LINFO("Graph successfully created using library.");

    // Show the inputs and outputs of the loaded net:
    for (uint32_t i = 0; i < itsGraph->input.num; ++i)
      LINFO("Input tensor " << i << ": " <<
            jevois::dnn::attrstr(vsi_nn_GetTensor(itsGraph, itsGraph->input.tensors[i])->attr));

    for (uint32_t i = 0; i < itsGraph->output.num; ++i)
      LINFO("Output tensor " << i << ": " <<
            jevois::dnn::attrstr(vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i])->attr));
  }
  
  LINFO("Graph ready.");

  if (verifygraph::get())
  {
    auto status = vsi_nn_VerifyGraph(itsGraph);
    if (status != VSI_SUCCESS) LFATAL("Graph verification failed -- \n"
                                      "check that intensors/outtensors specs exactly match\n"
                                      "those provided during model conversion.");
    else LINFO("Graph verification ok");
  }
}

// ####################################################################################################
namespace
{
  // Make a function to dequantize one tensor. We place dequantized tensor i into o and return an info string:
  // Remember to use std::ref around the cv::Mat arg to pass it by reference.
  static std::function<std::string(vsi_nn_graph_t *, size_t, cv::Mat &)>
  dequantize_one = [](vsi_nn_graph_t * graph, size_t i, cv::Mat & o) -> std::string
  {
    vsi_nn_tensor_t * ot = vsi_nn_GetTensor(graph, graph->output.tensors[i]);
    vsi_nn_tensor_attr_t const & oattr = ot->attr;
    uint8_t * tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(graph, ot);
    
    try
    {
      cv::Mat rawout = jevois::dnn::attrmat(oattr, tensor_data);
      o = jevois::dnn::dequantize(rawout, oattr);
      vsi_nn_Free(tensor_data);
      return "- Out " + std::to_string(i) + ": " + jevois::dnn::attrstr(oattr) + " -> 32F";
    }
    catch (...) { vsi_nn_Free(tensor_data); jevois::warnAndRethrowException(); }
  };
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkNPU::doprocess(std::vector<cv::Mat> const & blobs,
                                                        std::vector<std::string> & info)
{
  if (blobs.size() != itsGraph->input.num)
    LFATAL("Received " << blobs.size() << " blobs, but network has " << itsGraph->input.num << " inputs");
  
  static jevois::TimerOne intimer("Send inputs");
  static jevois::TimerOne infertimer("Inference");
  static jevois::TimerOne dqtimer("Dequantize");

  intimer.start();
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
  info.emplace_back(intimer.stop());

  // Ok, let's run the network:
  infertimer.start();
  auto status = vsi_nn_RunGraph(itsGraph);
  if (status != VSI_SUCCESS) LFATAL("Error running graph: " << status);
  info.emplace_back(infertimer.stop());

  // Collect the outputs, and possibly dequantize them:
  size_t const numouts = itsGraph->output.num;
  if (numouts == 0) return std::vector<cv::Mat>();
  
  std::vector<cv::Mat> outs(numouts);
  if (dequant::get())
  {
    // Dequantize and store, processing all outputs in parallel:
    dqtimer.start();

    // Avoid threading overhead if only one output:
    if (numouts == 1)
      info.emplace_back(dequantize_one(itsGraph, 0, std::ref(outs[0])));
    else
    {
      // Dequantize multiple outputs in parallel:
      std::vector<std::future<std::string>> fvec;

      for (uint32_t i = 0; i < numouts; ++i)
        fvec.emplace_back(jevois::async(dequantize_one, itsGraph, i, std::ref(outs[i])));

      // Use joinall() to get() all futures and throw a single consolidated exception if any thread threw:
      std::vector<std::string> retvec = jevois::joinall(fvec);

      // Collect all the info strings returned:
      info.insert(info.end(), std::make_move_iterator(retvec.begin()), std::make_move_iterator(retvec.end()));
    }
    info.emplace_back(dqtimer.stop());
  }
  else
  {
    // No dequantization simply copy the raw outputs into a vector of cv::Mat:
    for (uint32_t i = 0; i < numouts; ++i)
    {
      vsi_nn_tensor_t * ot = vsi_nn_GetTensor(itsGraph, itsGraph->output.tensors[i]);
      vsi_nn_tensor_attr_t const & oattr = ot->attr;
      uint8_t * tensor_data = (uint8_t *)vsi_nn_ConvertTensorToData(itsGraph, ot);

      try
      {
        cv::Mat rawout = jevois::dnn::attrmat(oattr, tensor_data);
        outs[i] = rawout.clone();
        info.emplace_back("- Out " + std::to_string(i) + ": " + jevois::dnn::attrstr(oattr));
      }
      catch (...) { vsi_nn_Free(tensor_data); jevois::warnAndRethrowException(); }

      vsi_nn_Free(tensor_data);
    }
  }

  return outs;
}

#endif // JEVOIS_PLATFORM_PRO
