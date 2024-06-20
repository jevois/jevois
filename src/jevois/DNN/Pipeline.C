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

#include <jevois/DNN/Pipeline.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Util/Async.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Debug/SysInfo.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Core/Engine.H>

#include <jevois/DNN/NetworkOpenCV.H>
#include <jevois/DNN/NetworkONNX.H>
#include <jevois/DNN/NetworkNPU.H>
#include <jevois/DNN/NetworkTPU.H>
#include <jevois/DNN/NetworkPython.H>
#include <jevois/DNN/NetworkHailo.H>

#include <jevois/DNN/PreProcessorBlob.H>
#include <jevois/DNN/PreProcessorPython.H>

#include <jevois/DNN/PostProcessorClassify.H>
#include <jevois/DNN/PostProcessorDetect.H>
#include <jevois/DNN/PostProcessorSegment.H>
#include <jevois/DNN/PostProcessorYuNet.H>
#include <jevois/DNN/PostProcessorPython.H>
#include <jevois/DNN/PostProcessorStub.H>

#include <opencv2/core/utils/filesystem.hpp>

#include <fstream>

// ####################################################################################################

// Simple class to hold a list of <name, value> pairs for our parameters, with updating the value of existing parameer
// names if they are set several times (e.g., first set as a global, then set again for a particular network). Note that
// here we do not check the validity of the parameters. This is delegated to Pipeline::setZooParam():
namespace
{
  class ParHelper
  {
    public:
      // ----------------------------------------------------------------------------------------------------
      // Set a param from an entry in our yaml file
      void set(cv::FileNode const & item, std::string const & zf, cv::FileNode const & node)
      {
        std::string k = item.name();
        std::string v;
        switch (item.type())
        {
        case cv::FileNode::INT: v = std::to_string((int)item); break;
        case cv::FileNode::REAL: v = std::to_string((float)item); break;
        case cv::FileNode::STRING: v = (std::string)item; break;
        default:
          if (&node == &item)
            LFATAL("Invalid global zoo parameter " << k << " type " << item.type() << " in " << zf);
          else
            LFATAL("Invalid zoo parameter " << k << " type " << item.type() << " in " << zf << " node " << node.name());
        }

        // Update value if param already exists, or add new key,value pair:
        for (auto & p : params) if (p.first == k) { p.second = v; return; }
        params.emplace_back(std::make_pair(k, v));
      }

      // ----------------------------------------------------------------------------------------------------
      // Get a value for entry subname under item if found, otherwise try our table of globals, otherwise empty
      std::string pget(cv::FileNode & item, std::string const & subname)
      {
        std::string const v = (std::string)item[subname];
        if (v.empty() == false) return v;
        for (auto const & p : params) if (p.first == subname) return p.second;
        return std::string();
      }

      // ----------------------------------------------------------------------------------------------------
      // Un-set a previously set global
      void unset(std::string const & name)
      {
        for (auto itr = params.begin(); itr != params.end(); ++itr)
          if (itr->first == name) { params.erase(itr); return; }
      }
      
      // Ordering matters, so use a vector instead of map or unordered_map
      std::vector<std::pair<std::string /* name */, std::string /* value */>> params;
  };
}

// ####################################################################################################
jevois::dnn::Pipeline::Pipeline(std::string const & instance) :
    jevois::Component(instance), itsTpre("PreProc"), itsTnet("Network"), itsTpost("PstProc")
{
  itsAccelerators["TPU"] = jevois::getNumInstalledTPUs();
  itsAccelerators["VPU"] = jevois::getNumInstalledVPUs();
  itsAccelerators["NPU"] = jevois::getNumInstalledNPUs();
  itsAccelerators["SPU"] = jevois::getNumInstalledSPUs();
  itsAccelerators["OpenCV"] = 1; // OpenCV always available
  itsAccelerators["ORT"] = 1;    // ONNX runtime always available
  itsAccelerators["Python"] = 1; // Python always available
#ifdef JEVOIS_PLATFORM_PRO
  itsAccelerators["VPUX"] = 1;   // VPU emulation on CPU always available through OpenVino
#endif
  itsAccelerators["NPUX"] = 1;   // NPU over Tim-VX always available since compiled into OpenCV
    
  LINFO("Detected " <<
        itsAccelerators["NPU"] << " JeVois-Pro NPUs, " <<
        itsAccelerators["SPU"] << " Hailo8 SPUs, " <<
        itsAccelerators["TPU"] << " Coral TPUs, " <<
        itsAccelerators["VPU"] << " Myriad-X VPUs.");
}

// ####################################################################################################
void jevois::dnn::Pipeline::freeze(bool doit)
{
  preproc::freeze(doit);
  nettype::freeze(doit);
  postproc::freeze(doit);

  if (itsPreProcessor) itsPreProcessor->freeze(doit);
  if (itsNetwork) itsNetwork->freeze(doit);
  if (itsPostProcessor) itsPostProcessor->freeze(doit);
}

// ####################################################################################################
void jevois::dnn::Pipeline::postInit()
{
  // Freeze all params that users should not modify at runtime:
  freeze(true);
}

// ####################################################################################################
void jevois::dnn::Pipeline::preUninit()
{
  // If we have a network running async, make sure we wait here until it is done:
  asyncNetWait();
}

// ####################################################################################################
jevois::dnn::Pipeline::~Pipeline()
{
  // Make sure network is not running as we die:
  asyncNetWait();
}

// ####################################################################################################
void jevois::dnn::Pipeline::asyncNetWait()
{
  // If we were currently doing async processing, wait until network is done:
  if (itsNetFut.valid())
    while (true)
    {
      if (itsNetFut.wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
        LERROR("Still waiting for network to finish running...");
      else break;
    }
  
  try { itsNetFut.get(); } catch (...) { }
  itsOuts.clear();
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::filter const & JEVOIS_UNUSED_PARAM(param),
                                          jevois::dnn::pipeline::Filter const & val)
{
  // Reload the zoo file so that the filter can be applied to create the parameter def of pipe, but first we need this
  // parameter to indeed be updated. So here we just set a flag and the update will occur in process(), after we run the
  // current model one last time:
  if (val != filter::get()) itsZooChanged = true;
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::zooroot const & JEVOIS_UNUSED_PARAM(param),
                                          std::string const & val)
{
  // Reload the zoo file, but first we need this parameter to indeed be updated. So here we just set a flag and the
  // update will occur in process(), after we run the current model one last time:
  if (val.empty() == false && val != zooroot::get()) itsZooChanged = true;
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::benchmark const & JEVOIS_UNUSED_PARAM(param),
                                          bool const & val)
{
  if (val)
  {
    statsfile::set("benchmark.html");
    statsfile::freeze(true);
  }
  else
  {
    statsfile::freeze(false);
    statsfile::reset();
  }
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::zoo const & JEVOIS_UNUSED_PARAM(param),
                                          std::string const & val)
{
  // Just nuke everything:
  itsPreProcessor.reset();
  itsNetwork.reset();
  itsPostProcessor.reset();
  // Will get instantiated again when pipe param is set.

  // Load zoo file:
  std::vector<std::string> pipes;
  scanZoo(jevois::absolutePath(zooroot::get(), val), filter::strget(), pipes, "");
  LINFO("Found a total of " << pipes.size() << " valid pipelines.");

  // Update the parameter def of pipe:
  jevois::ParameterDef<std::string> newdef("pipe", "Pipeline to use, determined by entries in the zoo file and "
                                           "by the current filter",
                                           pipes[0], pipes, jevois::dnn::pipeline::ParamCateg);
  pipe::changeParameterDef(newdef);

  // Just changing the def does not change the param value, so change it now:
  pipe::set(pipes[0]);

  // Mark the zoo as not changed anymore, unless we are just starting the module and need to load a first net:
  itsZooChanged = false;
}

// ####################################################################################################
void jevois::dnn::Pipeline::scanZoo(std::filesystem::path const & zoofile, std::string const & filt,
                                    std::vector<std::string> & pipes, std::string const & indent)
{
  LINFO(indent << "Scanning model zoo file " << zoofile << " with filter [" << filt << "]...");
  int ntot = 0, ngood = 0;

  bool has_vpu = false;
  auto itr = itsAccelerators.find("VPU");
  if (itr != itsAccelerators.end() && itr->second > 0) has_vpu = true;
  
  // Scan the zoo file to update the parameter def of the pipe parameter:
  cv::FileStorage fs(zoofile, cv::FileStorage::READ);
  if (fs.isOpened() == false) LFATAL("Could not open zoo file " << zoofile);
  cv::FileNode fn = fs.root();
  ParHelper ph;
  
  for (cv::FileNodeIterator fit = fn.begin(); fit != fn.end(); ++fit)
  {
    cv::FileNode item = *fit;

    // Process include: directives recursively:
    if (item.name() == "include")
    {
      scanZoo(jevois::absolutePath(zooroot::get(), (std::string)item), filt, pipes, indent + "  ");
    }
    // Process includedir: directives (only one level of directory is scanned):
    else if (item.name() == "includedir")
    {
      std::filesystem::path const dir = jevois::absolutePath(zooroot::get(), (std::string)item);
      for (auto const & dent : std::filesystem::recursive_directory_iterator(dir))
        if (dent.is_regular_file())
        {
          std::filesystem::path const path = dent.path();
          std::filesystem::path const ext = path.extension();
          if (ext == ".yml" || ext == ".yaml") scanZoo(path, filt, pipes, indent + "  ");
        }
    }
    // Unset a previously set global?
    else if (item.name() == "unset")
    {
      ph.unset((std::string)item);
    }
    // Set a global:
    else if (! item.isMap())
    {
      ph.set(item, zoofile, item);
    }
    // Map type (model definition):
    else
    {
      ++ntot;
      
      // As a prefix, we use OpenCV for OpenCV models on CPU/OpenCL backends, and VPU for InferenceEngine backend with
      // Myriad target, VPUX for InferenceEngine/CPU (arm-compute OpenVino plugin, only works on platform), and NPUX for
      // TimVX/NPU (NPU using TimVX OpenCV extension, uses NPU on platform or emulator on host):

      // Set then get nettype to account for globals:
      std::string typ = ph.pget(item, "nettype");
      
      if (typ == "OpenCV")
      {
        std::string backend = ph.pget(item, "backend");
        std::string target = ph.pget(item, "target");

        if (backend == "InferenceEngine")
        {
          if (target == "Myriad")
          {
            if (has_vpu) typ = "VPU"; // run VPU models on VPU if MyriadX accelerator is present
#ifdef JEVOIS_PLATFORM_PRO
            else typ = "VPUX"; // emulate VPU models on CPU through ARM-Compute if MyriadX accelerator not present
#else
            else continue; // VPU emulation does not work on host...
#endif
          }
          else if (target == "CPU") typ = "VPUX";
        }
        else if (backend == "TimVX" && target == "NPU") typ = "NPUX";
      }
      
      // Do not consider a model if we do not have the accelerator for it:
      bool has_accel = false;
      itr = itsAccelerators.find(typ);
      if (itr != itsAccelerators.end() && itr->second > 0) has_accel = true;
      
      // Add this pipe if it matches our filter and we have the accelerator for it:
      if ((filt == "All" || typ == filt) && has_accel)
      {
        std::string const postproc = ph.pget(item, "postproc");
        pipes.emplace_back(typ + ':' + postproc + ':' + item.name());
        ++ngood;
      }
    }
  }
  
  LINFO(indent << "Found " << ntot << " pipelines, " << ngood << " passed the filter.");
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::pipe const & JEVOIS_UNUSED_PARAM(param), std::string const & val)
{
#ifdef JEVOIS_PRO
  // Reset the data peekin on each pipe change:
  itsShowDataPeek = false;
  itsDataPeekOutIdx = 0;
  itsDataPeekFreeze = false;
  itsDataPeekStr.clear();
#endif
  
  if (val.empty()) return;
  itsPipeThrew = false;
  freeze(false);

  // Clear any errors related to previous pipeline:
  engine()->clearErrors();
  
  // Find the desired pipeline, and set it up:
  std::string const z = jevois::absolutePath(zooroot::get(), zoo::get());
  std::vector<std::string> tok = jevois::split(val, ":");
  if (selectPipe(z, tok) == false)
    LFATAL("Could not find pipeline entry [" << val << "] in zoo file " << z << " and its includes");

  freeze(true);
}

// ####################################################################################################
bool jevois::dnn::Pipeline::selectPipe(std::string const & zoofile, std::vector<std::string> const & tok)
{
  // We might have frozen processing to Sync if we ran a NetworkPython previously, so unfreeze here:
  processing::freeze(false);
  processing::set(jevois::dnn::pipeline::Processing::Async);

  // Check if we have a VPU, to use VPU vs VPUX:
  bool has_vpu = false;
  auto itr = itsAccelerators.find("VPU");
  if (itr != itsAccelerators.end() && itr->second > 0) has_vpu = true;
  bool vpu_emu = false;

  // Clear any old stats:
  itsPreStats.clear(); itsNetStats.clear(); itsPstStats.clear();
  itsStatsWarmup = true; // warmup before computing new stats
  
  // Open the zoo file:
  cv::FileStorage fs(zoofile, cv::FileStorage::READ);
  if (fs.isOpened() == false) LFATAL("Could not open zoo file " << zoofile);

  // Find the desired pipeline:
  ParHelper ph;
  cv::FileNode fn = fs.root(), node;

  for (cv::FileNodeIterator fit = fn.begin(); fit != fn.end(); ++fit)
  {
    cv::FileNode item = *fit;
    
    // Process include: directives recursively, end the recursion if we found our pipe in there:
    if (item.name() == "include")
    {
      if (selectPipe(jevois::absolutePath(zooroot::get(), (std::string)item), tok)) return true;
    }

    // Process includedir: directives (only one level of directory is scanned), end recursion if we found our pipe:
    else if (item.name() == "includedir")
    {
      std::filesystem::path const dir = jevois::absolutePath(zooroot::get(), (std::string)item);
      for (auto const & dent : std::filesystem::recursive_directory_iterator(dir))
        if (dent.is_regular_file())
        {
          std::filesystem::path const path = dent.path();
          std::filesystem::path const ext = path.extension();
          if (ext == ".yml" || ext == ".yaml") if (selectPipe(path, tok)) return true;
        }
    }
    
    // Unset a previously set global?
    else if (item.name() == "unset")
    {
      ph.unset((std::string)item);
    }
    // Set a global:
    else if (! item.isMap())
    {
      ph.set(item, zoofile, node);
    }
    // This is an entry for a pipeline with a bunch of params under it:
    else
    {
      if (item.name() != tok.back()) continue;
      if (tok.size() == 1) { node = item; break; }
      if (tok.size() != 3) LFATAL("Malformed pipeline name: " << jevois::join(tok, ":"));
      
      // Skip if postproc is no match:
      std::string postproc = ph.pget(item, "postproc");
      if (postproc != tok[1] && postproc::strget() != tok[1]) continue;
      
      std::string nettype = ph.pget(item, "nettype");
      std::string backend = ph.pget(item, "backend");
      std::string target = ph.pget(item, "target");
      
      if (tok[0] == "VPU")
      {
        if (nettype == "OpenCV" && backend == "InferenceEngine" && target == "Myriad")
        { node = item; break; }
      }
      else if (tok[0] == "VPUX")
      {
        if (nettype == "OpenCV" && backend == "InferenceEngine")
        {
          if (target == "Myriad" && has_vpu == false) { vpu_emu = true; node = item; break; }
          else if (target == "CPU") { node = item; break; }
        }
      }
      else if (tok[0] == "NPUX")
      {
        if (nettype == "OpenCV" && backend == "TimVX" && target == "NPU")
        { node = item; break; }
      }
      else
      {
        if (nettype == tok[0])
        { node = item; break; }
      }
    }
  }
  
  // If the spec was not a match with any entries in the file, return false:
  if (node.empty()) return false;
      
  // Found the pipe. First nuke our current pre/net/post:
  asyncNetWait();
  itsPreProcessor.reset(); removeSubComponent("preproc", false);
  itsNetwork.reset(); removeSubComponent("network", false);
  itsPostProcessor.reset(); removeSubComponent("postproc", false);

  // Then set all the global parameters of the current file:
  //for (auto const & pp : ph.params)
  //  setZooParam(pp.first, pp.second, zoofile, fs.root());
  
  // Then iterate over all pipeline params and set them: first update our table, then set params from the whole table:
  for (cv::FileNodeIterator fit = node.begin(); fit != node.end(); ++fit)
    ph.set(*fit, zoofile, node);

  for (auto const & pp : ph.params)
  {
    if (vpu_emu && pp.first == "target") setZooParam(pp.first, "CPU", zoofile, node);
    else setZooParam(pp.first, pp.second, zoofile, node);
  }

  // Running a python net async segfaults instantly if we are also concurrently running pre or post processing in
  // python, as python is not re-entrant... so force sync here:
  if (dynamic_cast<jevois::dnn::NetworkPython *>(itsNetwork.get()) &&
      (dynamic_cast<jevois::dnn::PreProcessorPython *>(itsPreProcessor.get()) ||
       dynamic_cast<jevois::dnn::PostProcessorPython *>(itsPostProcessor.get())))
  {
    if (processing::get() != jevois::dnn::pipeline::Processing::Sync)
    {
      LERROR("Network of type Python cannot run Async if pre- or post- processor are also Python "
             "-- FORCING Sync processing");
      processing::set(jevois::dnn::pipeline::Processing::Sync);
    }
    processing::freeze(true);
  }

  return true;
}
  
// ####################################################################################################
void jevois::dnn::Pipeline::setZooParam(std::string const & k, std::string const & v,
                                        std::string const & zf, cv::FileNode const & node)
{
  // The zoo file may contain extra params, like download URL, etc. To ignore those while still catching invalid
  // values on our parameters, we first check whether the parameter exists, and, if so, try to set it:
  bool hasparam = false;
  try { getParamStringUnique(k); hasparam = true; } catch (...) { }
  
  if (hasparam)
  {
    LINFO("Setting ["<<k<<"] to ["<<v<<']');
    
    try { setParamStringUnique(k, v); }
    catch (std::exception const & e)
    { LFATAL("While parsing [" << node.name() << "] in model zoo file " << zf << ": " << e.what()); }
    catch (...)
    { LFATAL("While parsing [" << node.name() << "] in model zoo file " << zf << ": unknown error"); }
  }
  else if (paramwarn::get())
    engine()->reportError("WARNING: Unused parameter [" + k + "] in " + zf + " node [" + node.name() + "]");
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::preproc const & JEVOIS_UNUSED_PARAM(param),
                                          pipeline::PreProc const & val)
{
  itsPreProcessor.reset(); removeSubComponent("preproc", false);
  
  switch (val)
  {
  case jevois::dnn::pipeline::PreProc::Blob:
    itsPreProcessor = addSubComponent<jevois::dnn::PreProcessorBlob>("preproc");
    break;

  case jevois::dnn::pipeline::PreProc::Python:
    itsPreProcessor = addSubComponent<jevois::dnn::PreProcessorPython>("preproc");
    break;
  }

  if (itsPreProcessor) LINFO("Instantiated pre-processor of type " << itsPreProcessor->className());
  else LINFO("No pre-processor");
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::nettype const & JEVOIS_UNUSED_PARAM(param),
                                          pipeline::NetType const & val)
{
  asyncNetWait(); // If currently processing async net, wait until done

  itsNetwork.reset(); removeSubComponent("network", false);
  
  switch (val)
  {
  case jevois::dnn::pipeline::NetType::OpenCV:
    itsNetwork = addSubComponent<jevois::dnn::NetworkOpenCV>("network");
    break;

#ifdef JEVOIS_PRO

  case jevois::dnn::pipeline::NetType::ORT:
    itsNetwork = addSubComponent<jevois::dnn::NetworkONNX>("network");
    break;
    
  case jevois::dnn::pipeline::NetType::NPU:
#ifdef JEVOIS_PLATFORM
    itsNetwork = addSubComponent<jevois::dnn::NetworkNPU>("network");
#else // JEVOIS_PLATFORM
    LFATAL("NPU network is only supported on JeVois-Pro Platform");
#endif
    break;
    
  case jevois::dnn::pipeline::NetType::SPU:
    itsNetwork = addSubComponent<jevois::dnn::NetworkHailo>("network");
    break;
    
  case jevois::dnn::pipeline::NetType::TPU:
    itsNetwork = addSubComponent<jevois::dnn::NetworkTPU>("network");
    break;
#endif
    
  case jevois::dnn::pipeline::NetType::Python:
    itsNetwork = addSubComponent<jevois::dnn::NetworkPython>("network");
    break;
  }

  if (itsNetwork) LINFO("Instantiated network of type " << itsNetwork->className());
  else LINFO("No network");

  // We already display a "loading..." message while the network is loading, but some OpenCV networks take a long time
  // to process their first frame after they are loaded (e.g., YuNet initializes all the anchors on first frame). So
  // here we set some placeholder text that will appear after the network is loaded and is processing the first frame:
  itsInputAttrs.clear();
  itsNetInfo.clear();
  itsNetInfo.emplace_back("* Input Tensors");
  itsNetInfo.emplace_back("Initializing network...");
  itsNetInfo.emplace_back("* Network");
  itsNetInfo.emplace_back("Initializing network...");
  itsNetInfo.emplace_back("* Output Tensors");
  itsNetInfo.emplace_back("Initializing network...");
  itsAsyncNetInfo = itsNetInfo;
  itsAsyncNetworkTime = "Network: -";
  itsAsyncNetworkSecs = 0.0;
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::postproc const & JEVOIS_UNUSED_PARAM(param),
                                          pipeline::PostProc const & val)
{
  asyncNetWait(); // If currently processing async net, wait until done

  itsPostProcessor.reset(); removeSubComponent("postproc", false);

  switch (val)
  {
  case jevois::dnn::pipeline::PostProc::Classify:
    itsPostProcessor = addSubComponent<jevois::dnn::PostProcessorClassify>("postproc");
    break;
  case jevois::dnn::pipeline::PostProc::Detect:
    itsPostProcessor = addSubComponent<jevois::dnn::PostProcessorDetect>("postproc");
    break;
  case jevois::dnn::pipeline::PostProc::Segment:
    itsPostProcessor = addSubComponent<jevois::dnn::PostProcessorSegment>("postproc");
    break;
  case jevois::dnn::pipeline::PostProc::YuNet:
    itsPostProcessor = addSubComponent<jevois::dnn::PostProcessorYuNet>("postproc");
    break;
  case jevois::dnn::pipeline::PostProc::Python:
    itsPostProcessor = addSubComponent<jevois::dnn::PostProcessorPython>("postproc");
    break;
  case jevois::dnn::pipeline::PostProc::Stub:
    itsPostProcessor = addSubComponent<jevois::dnn::PostProcessorStub>("postproc");
    break;
  }

  if (itsPostProcessor) LINFO("Instantiated post-processor of type " << itsPostProcessor->className());
  else LINFO("No post-processor");
}

// ####################################################################################################
bool jevois::dnn::Pipeline::ready() const
{
  return itsPreProcessor && itsNetwork && itsNetwork->ready() && itsPostProcessor;
}

// ####################################################################################################
bool jevois::dnn::Pipeline::checkAsyncNetComplete()
{
  if (itsNetFut.valid() && itsNetFut.wait_for(std::chrono::milliseconds(2)) == std::future_status::ready)
  {
    itsOuts = itsNetFut.get();
    itsNetInfo.clear();
    std::swap(itsNetInfo, itsAsyncNetInfo);
    itsProcTimes[1] = itsAsyncNetworkTime;
    itsProcSecs[1] = itsAsyncNetworkSecs;
    return true;
  }
  return false;
}

// ####################################################################################################
void jevois::dnn::Pipeline::process(jevois::RawImage const & inimg, jevois::StdModule * mod, jevois::RawImage * outimg,
                                    jevois::OptGUIhelper * helper, bool idle)
{
  // Reload the zoo file if filter has changed:
  if (itsZooChanged) zoo::set(zoo::get());

  // If the pipeline is throwing exception at any stage, do not do anything here, itsPipeThrew is cleared when selecting
  // a new pipe:
  if (itsPipeThrew) return;
  
  bool const ovl = overlay::get();
  itsOutImgY = 5; // y text position when using outimg text drawings
  bool refresh_data_peek = false; // Will be true after each post-processing is actually run
  
#ifdef JEVOIS_PRO
  // Open an info window if using GUI and not idle:
  if (helper && idle == false)
  {
    // Set window size applied only on first use ever, otherwise from imgui.ini:
    ImGui::SetNextWindowPos(ImVec2(24, 159), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(464, 877), ImGuiCond_FirstUseEver);
    
    // Open the window:
    ImGui::Begin((instanceName() + ':' + getParamStringUnique("pipe")).c_str());
  }
#else
  (void)helper; // avoid compiler warning
#endif

  // If we want an overlay, show network name on first line:
  if (ovl)
  {
    if (outimg)
    {
      jevois::rawimage::writeText(*outimg, instanceName() + ':' + getParamStringUnique("pipe"),
                                  5, itsOutImgY, jevois::yuyv::White);
      itsOutImgY += 11;
    }
    
#ifdef JEVOIS_PRO
    if (helper) helper->itext(instanceName() + ':' + getParamStringUnique("pipe"));
#endif
  }
  
  // If network is not ready, inform user. Be careful that ready() may throw, eg, if bad network name was given and
  // network could not be loaded:
  try
  {
    if (ready() == false)
    {
      char const * msg = itsNetwork ? "Loading network..." : "No network selected...";
      
      if (outimg)
      {
        jevois::rawimage::writeText(*outimg, msg, 5, itsOutImgY, jevois::yuyv::White);
        itsOutImgY += 11;
      }
      
#ifdef JEVOIS_PRO
      if (helper)
      {
        if (idle == false) ImGui::TextUnformatted(msg);
        if (ovl) helper->itext(msg);
      }
#endif
      
      itsProcTimes = { "PreProc: -", "Network: -", "PstProc: -" };
      itsProcSecs = { 0.0, 0.0, 0.0 };
    }
    else
    {
      // Network is ready, run processing, either single-thread (Sync) or threaded (Async):
      switch (processing::get())
      {
        // --------------------------------------------------------------------------------
      case jevois::dnn::pipeline::Processing::Sync:
      {
        asyncNetWait(); // If currently processing async net, wait until done
        
        // Pre-process:
        itsTpre.start();
        if (itsInputAttrs.empty()) itsInputAttrs = itsNetwork->inputShapes();
        itsBlobs = itsPreProcessor->process(inimg, itsInputAttrs);
        itsProcTimes[0] = itsTpre.stop(&itsProcSecs[0]);
        itsPreProcessor->sendreport(mod, outimg, helper, ovl, idle);
        
        // Network forward pass:
        itsNetInfo.clear();
        itsTnet.start();
        itsOuts = itsNetwork->process(itsBlobs, itsNetInfo);
        itsProcTimes[1] = itsTnet.stop(&itsProcSecs[1]);
        
        // Show network info:
        showInfo(itsNetInfo, mod, outimg, helper, ovl, idle);
        
        // Post-Processing:
        itsTpost.start();
        itsPostProcessor->process(itsOuts, itsPreProcessor.get());
        itsProcTimes[2] = itsTpost.stop(&itsProcSecs[2]);
        itsPostProcessor->report(mod, outimg, helper, ovl, idle);
        refresh_data_peek = true;
      }
      break;
      
      // --------------------------------------------------------------------------------
      case jevois::dnn::pipeline::Processing::Async:
      {
        // We are going to run pre-processing and post-processing synchronously, and network in a thread. One small
        // complication is that we are going to run post-processing on every frame so that the drawings do not
        // flicker. We will keep post-processing the same results until new results replace them.
        
        // Are we running the network, and is it done? If so, get the outputs:
        bool needpost = checkAsyncNetComplete();
        
        // If we are not running a network, start it:
        if (itsNetFut.valid() == false)
        {
          // Pre-process in the current thread:
          itsTpre.start();
          if (itsInputAttrs.empty()) itsInputAttrs = itsNetwork->inputShapes();
          itsBlobs = itsPreProcessor->process(inimg, itsInputAttrs);
          itsProcTimes[0] = itsTpre.stop(&itsProcSecs[0]);
          
          // Network forward pass in a thread:
          itsNetFut =
            jevois::async([this]()
                          {
                            itsTnet.start();
                            std::vector<cv::Mat> outs = itsNetwork->process(itsBlobs, itsAsyncNetInfo);
                            itsAsyncNetworkTime = itsTnet.stop(&itsAsyncNetworkSecs);
                            
                            // OpenCV DNN seems to be re-using and overwriting the same output matrices,
                            // so we need to make a deep copy of the outputs if the network type is OpenCV:
                            if (dynamic_cast<jevois::dnn::NetworkOpenCV *>(itsNetwork.get()) == nullptr)
                              return outs;
                            
                            std::vector<cv::Mat> outscopy;
                            for (cv::Mat const & m : outs) outscopy.emplace_back(m.clone());
                            return outscopy;
                          });
        }
        
        // Report pre-processing results on every frame:
        itsPreProcessor->sendreport(mod, outimg, helper, ovl, idle);
        
        // Show network info on every frame:
        showInfo(itsNetInfo, mod, outimg, helper, ovl, idle);
        
        // Run post-processing if needed:
        if (needpost && itsOuts.empty() == false)
        {
          itsTpost.start();
          itsPostProcessor->process(itsOuts, itsPreProcessor.get());
          itsProcTimes[2] = itsTpost.stop(&itsProcSecs[2]);
          refresh_data_peek = true;
        }
        
        // Report/draw post-processing results on every frame:
        itsPostProcessor->report(mod, outimg, helper, ovl, idle);
      }
      break;
      }
      
      // Update our rolling average of total processing time:
      itsSecsSum += itsProcSecs[0] + itsProcSecs[1] + itsProcSecs[2];
      if (++itsSecsSumNum == 20) { itsSecsAvg = itsSecsSum / itsSecsSumNum; itsSecsSum = 0.0; itsSecsSumNum = 0; }
      
      // If computing benchmarking stats, update them now:
      if (statsfile::get().empty() == false && itsOuts.empty() == false)
      {
        static std::vector<std::string> pipelines;
        static bool statswritten = false;
        static size_t benchpipe = 0;
        
        if (benchmark::get())
        {
          if (pipelines.empty())
          {
            // User just turned on benchmark mode. List all pipes and start iterating over them:
            // Valid values string format is List:[A|B|C] where A, B, C are replaced by the actual elements.
            std::string pipes = pipe::def().validValuesString();
            size_t const idx = pipes.find('[');
            pipes = pipes.substr(idx + 1, pipes.length() - idx - 2); // risky code but we control the string's contents
            pipelines = jevois::split(pipes, "\\|");
            benchpipe = 0;
            statswritten = false;
            pipe::set(pipelines[benchpipe]);
#ifdef JEVOIS_PRO
            if (helper)
            {
              helper->reportError("Starting DNN benchmark...");
              helper->reportError("Benchmarking: " +pipelines[benchpipe]);
            }
#endif
          }
          else
          {
            // Switch to the next pipeline after enough stats have been written:
            if (statswritten)
            {
              ++benchpipe;
              statswritten = false;
              if (benchpipe >= pipelines.size())
              {
                pipelines.clear();
                benchmark::set(false);
#ifdef JEVOIS_PRO
                if (helper) helper->reportError("DNN benchmark complete.");
#endif
              }
              else
              {
                pipe::set(pipelines[benchpipe]);
#ifdef JEVOIS_PRO
                if (helper) helper->reportError("Benchmarking: " +pipelines[benchpipe]);
#endif
              }
            }
          }
        }
        else pipelines.clear();
        
        itsPreStats.push_back(itsProcSecs[0]);
        itsNetStats.push_back(itsProcSecs[1]);
        itsPstStats.push_back(itsProcSecs[2]);
        
        // Discard data for a few warmup frames after we start a new net:
        if (itsStatsWarmup && itsPreStats.size() == 200)
        { itsStatsWarmup = false; itsPreStats.clear(); itsNetStats.clear(); itsPstStats.clear(); }
        
        if (itsPreStats.size() == 500)
        {
          // Compute totals:
          std::vector<double> tot;
          for (size_t i = 0; i < itsPreStats.size(); ++i)
            tot.emplace_back(itsPreStats[i] + itsNetStats[i] + itsPstStats[i]);
          
          // Append to stats file:
          std::string const fn = jevois::absolutePath(JEVOIS_SHARE_PATH, statsfile::get());
          std::ofstream ofs(fn, std::ios_base::app);
          if (ofs.is_open())
          {
            ofs << "<tr><td class=jvpipe>" << pipe::get() << " </td>";
            
            std::vector<std::string> insizes;
            for (cv::Mat const & m : itsBlobs)
              insizes.emplace_back(jevois::replaceAll(jevois::dnn::shapestr(m), " ", "&nbsp;"));
            ofs << "<td class=jvnetin>" << jevois::join(insizes, ", ") << "</td>";
            
            std::vector<std::string> outsizes;
            for (cv::Mat const & m : itsOuts)
              outsizes.emplace_back(jevois::replaceAll(jevois::dnn::shapestr(m), " ", "&nbsp;"));
            ofs << "<td class=jvnetout>" << jevois::join(outsizes, ", ") << "</td>";
            
            ofs <<
              "<td class=jvprestats>" << jevois::replaceAll(jevois::secs2str(itsPreStats), " ", "&nbsp;") << "</td>"
              "<td class=jvnetstats>" << jevois::replaceAll(jevois::secs2str(itsNetStats), " ", "&nbsp;") << "</td>"
              "<td class=jvpststats>" << jevois::replaceAll(jevois::secs2str(itsPstStats), " ", "&nbsp;") << "</td>"
              "<td class=jvtotstats>" << jevois::replaceAll(jevois::secs2str(tot), " ", "&nbsp;") << "</td>";
            
            // Finally report average fps:
            double avg = 0.0;
            for (double t : tot) avg += t;
            avg /= tot.size();
            if (avg) avg = 1.0 / avg; // from s/frame to frames/s
            ofs << "<td class=jvfps>" << std::fixed << std::showpoint << std::setprecision(1) <<
              avg << "&nbsp;fps</td></tr>" << std::endl;
            
            // Ready for next round:
            itsPreStats.clear();
            itsNetStats.clear();
            itsPstStats.clear();
            LINFO("Network stats appended to " << fn);
            statswritten = true;
          }
        }
      }
    }
  }
  catch (...)
  {
    itsPipeThrew = true;
    
#ifdef JEVOIS_PRO
    if (helper) helper->reportAndIgnoreException(instanceName());
    else jevois::warnAndIgnoreException(instanceName());
#else
    jevois::warnAndIgnoreException(instanceName());
#endif
  }
  
#ifdef JEVOIS_PRO
  // Report processing times and close info window if we opened it:
  if (helper)
  {
    std::string total;
    if (idle == false || ovl) total = jevois::secs2str(itsSecsAvg);
    
    if (idle == false)
    {
      // Show processing times:
      if (ImGui::CollapsingHeader("Processing Times", ImGuiTreeNodeFlags_DefaultOpen))
      {
        for (std::string const & s : itsProcTimes) ImGui::TextUnformatted(s.c_str());
        ImGui::Text("OVERALL: %s/inference", total.c_str());
      }
      ImGui::Separator();
      
      // Show a button to allow users to peek output data:
      if (ImGui::Button("Peek output data")) itsShowDataPeek = true;

      // Done with this window:
      ImGui::End();

      // Allow user to peek into output data:
      showDataPeekWindow(helper, refresh_data_peek);
    }
    
    if (ovl)
    {
      for (std::string const & s : itsProcTimes) helper->itext(s);
      helper->itext("OVERALL: " + total + "/inference");
    }
  }
#else
  (void)refresh_data_peek; // prevent compiler warning
#endif

  // Report processing times to outimg if present:
  if (outimg && ovl)
  {
    for (std::string const & s : itsProcTimes)
    {
      jevois::rawimage::writeText(*outimg, s, 5, itsOutImgY, jevois::yuyv::White);
      itsOutImgY += 11;
    }
    jevois::rawimage::writeText(*outimg, "OVERALL: " + jevois::secs2str(itsSecsAvg) + "/inference",
                                5, itsOutImgY, jevois::yuyv::White);
    itsOutImgY += 11;
  }
}

// ####################################################################################################
void jevois::dnn::Pipeline::showInfo(std::vector<std::string> const & info,
                                     jevois::StdModule * JEVOIS_UNUSED_PARAM(mod),
                                     jevois::RawImage * outimg,
                                     jevois::OptGUIhelper * helper, bool ovl, bool idle)
{
  bool show = true;

  for (std::string const & s : info)
  {
    // On JeVois Pro, display info in the GUI:
#ifdef JEVOIS_PRO
    if (helper && idle == false)
    {
      // Create collapsible header and get its collapsed status:
      if (jevois::stringStartsWith(s, "* "))
        show = ImGui::CollapsingHeader(s.c_str() + 2, ImGuiTreeNodeFlags_DefaultOpen);
      else if (show)
      {
        // If header not collapsed, show data:
        if (jevois::stringStartsWith(s, "- ")) ImGui::BulletText("%s", s.c_str() + 2);
        else ImGui::TextUnformatted(s.c_str());
      }
    }
#else
    (void)idle; (void)show; (void)helper; // avoid warning
#endif
    
    if (outimg && ovl)
    {
      jevois::rawimage::writeText(*outimg, s, 5, itsOutImgY, jevois::yuyv::White);
      itsOutImgY += 11;
    }
  }
}

#ifdef JEVOIS_PRO
// ####################################################################################################
void jevois::dnn::Pipeline::showDataPeekWindow(jevois::GUIhelper * helper, bool refresh)
{
  // Do not show anything if user closed the window:
  if (itsShowDataPeek == false) return;

  // Set window size applied only on first use ever, otherwise from imgui.ini:
  ImGui::SetNextWindowPos(ImVec2(100, 50), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

  // Light blue window background:
  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0xf0ffe0e0);

  // Open the window:
  ImGui::Begin("DNN Output Peek", &itsShowDataPeek, ImGuiWindowFlags_HorizontalScrollbar);

  // Draw a combo to select which output:
  std::vector<std::string> outspecs;
  for (size_t i = 0; cv::Mat const & out : itsOuts)
    outspecs.emplace_back("Out " + std::to_string(i++) + ": " + jevois::dnn::shapestr(out));
  if (helper->combo("##dataPeekOutSelect", outspecs, itsDataPeekOutIdx)) itsDataPeekFreeze = false;

  ImGui::SameLine(); ImGui::TextUnformatted("  "); ImGui::SameLine();
  helper->toggleButton("Freeze", &itsDataPeekFreeze);
  ImGui::Separator();

  // Draw the data:
  if ( (itsDataPeekFreeze && itsDataPeekStr.empty() == false) || refresh == false)
    ImGui::TextUnformatted(itsDataPeekStr.c_str());
  else
  {
    // OpenCV Mat::operator<< cannot handle >2D, try to collapse any dimensions with size 1:
    cv::Mat const & out = itsOuts[itsDataPeekOutIdx];
    std::vector<int> newsz;
    cv::MatSize const & ms = out.size; int const nd = ms.dims();
    for (int i = 0; i < nd; ++i) if (ms[i] > 1) newsz.emplace_back(ms[i]);
    cv::Mat const out2(newsz, out.type(), out.data);

    try
    {
      std::ostringstream oss;
      if (newsz.size() > 3)
        throw "too many dims";
      else if (newsz.size() == 3)
      {
        cv::Range ranges[3];
        ranges[2] = cv::Range::all();
        ranges[1] = cv::Range::all();
        for (int i = 0; i < newsz[0]; ++i)
        {
          oss << "-------------------------------------------------------------------------------\n";
          oss << "Third dimension index = " << i << ":\n";
          oss << "-------------------------------------------------------------------------------\n\n";
          ranges[0] = cv::Range(i, i+1);
          cv::Mat slice = out2(ranges); // still 3D but with 1 as 3D dimension...
          cv::Mat slice2d(cv::Size(newsz[2], newsz[1]), slice.type(), slice.data); // Now 2D
          oss << slice2d << "\n\n";
        }
      }
      else
        oss << out2;

      itsDataPeekStr = oss.str();
    }
    catch (...) { itsDataPeekStr = "Sorry, cannot display this type of tensor..."; }

    ImGui::TextUnformatted(itsDataPeekStr.c_str());

    if (out2.total() > 10000)
    {
      helper->reportError("Large data peek - Freezing data display\n"
                          "Click the Freeze button to refresh once");
      itsDataPeekFreeze = true;
    }
  }
  
  // Done with this window:
  ImGui::End();
  ImGui::PopStyleColor();
}
#endif
  
