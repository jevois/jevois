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

#include <jevois/DNN/NetworkOpenCV.H>
#include <jevois/DNN/NetworkNPU.H>
#include <jevois/DNN/NetworkTPU.H>

#include <jevois/DNN/PreProcessorBlob.H>

#include <jevois/DNN/PostProcessorClassify.H>
#include <jevois/DNN/PostProcessorDetect.H>
#include <jevois/DNN/PostProcessorSegment.H>

#include <opencv2/core/utils/filesystem.hpp>

#include <dirent.h>

// ####################################################################################################
jevois::dnn::Pipeline::Pipeline(std::string const & instance) :
    jevois::Component(instance), itsTpre("PreProc"), itsTnet("Network"), itsTpost("PstProc")
{
  itsAccelerators["TPU"] = jevois::getNumInstalledTPUs();
  itsAccelerators["VPU"] = jevois::getNumInstalledVPUs();
  itsAccelerators["NPU"] = jevois::getNumInstalledNPUs();
  itsAccelerators["OpenCV"] = 1;
  
  LINFO("Detected " << itsAccelerators["NPU"] << " JeVois-Pro NPUs, " << itsAccelerators["TPU"] <<
        " Coral TPUs, " << itsAccelerators["VPU"] << " Myriad-X VPUs.");
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
                                          jevois::dnn::pipeline::Filter const & JEVOIS_UNUSED_PARAM(val))
{
  // Reload the zoo file so that the filter can be applied to create the parameter def of pipe, but first we need this
  // parameter to indeed be updated. So here we just set a flag and the update will occur in process(), after we run the
  // current model one last time:
  itsZooChanged = true;
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::zooroot const & JEVOIS_UNUSED_PARAM(param),
                                          std::string const & JEVOIS_UNUSED_PARAM(val))
{
  // Reload the zoo file, but first we need this parameter to indeed be updated. So here we just set a flag and the
  // update will occur in process(), after we run the current model one last time:
  itsZooChanged = true;
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
  if (pipes.empty() == false)
  {
    jevois::ParameterDef<std::string> newdef("pipe", "Pipeline to use, determined by entries in the zoo file and "
                                             "by the current filter",
                                             pipes[0], pipes, jevois::dnn::pipeline::ParamCateg);
    pipe::changeParameterDef(newdef);

    // Just changing the def does not change the param value, so change it now:
    pipe::set(pipes[0]);
  }
}

// ####################################################################################################
void jevois::dnn::Pipeline::scanZoo(std::string const & zoofile, std::string const & filt,
                                    std::vector<std::string> & pipes, std::string const & indent)
{
  LINFO(indent << "Scanning model zoo file " << zoofile << " with filter [" << filt << "]...");
  int ntot = 0, ngood = 0;
  
  // Scan the zoo file to update the parameter def of the pipe parameter:
  cv::FileStorage fs(zoofile, cv::FileStorage::READ);
  if (fs.isOpened() == false) LFATAL("Could not open zoo file " << zoofile);
  cv::FileNode fn = fs.root();

  for (cv::FileNodeIterator fit = fn.begin(); fit != fn.end(); ++fit)
  {
    cv::FileNode item = *fit;

    // Process include: directives recursively:
    if (item.name() == "include")
      scanZoo(jevois::absolutePath(zooroot::get(), (std::string)item), filt, pipes, indent + "  ");

    // Process includedir: directives (only one level of directory is scanned):
    if (item.name() == "includedir")
    {
      std::string const dir = jevois::absolutePath(zooroot::get(), (std::string)item);
      DIR *dirp = opendir(dir.c_str());
      if (dirp == nullptr) LFATAL("includedir: " << dir << " in zoo file " << zoofile << ": cannot read directory");
      struct dirent * dent;
      while ((dent = readdir(dirp)) != nullptr)
      {
        std::string const node = dent->d_name;
        size_t const nl = node.length();
        if ((nl > 4 && node.substr(nl-4) == ".yml") || (nl > 5 && node.substr(nl-5) == ".yaml"))
          scanZoo(dir + "/" + node, filt, pipes, indent + "  ");
      }
      closedir(dirp);
    }
    
    // Process map types, ignore others:
    if (item.isMap() == false) continue;

    ++ntot;

    // As a prefix, we use OpenCV for OpenCV models on CPU/OpenCL backends, and VPU for InferenceEngine backend with
    // Myriad target:
    std::string typ = (std::string)item["nettype"];
    if (typ == "OpenCV" &&
        (std::string)item["backend"] == "InferenceEngine" &&
        (std::string)item["target"] == "Myriad")
      typ = "VPU";

    // Do not consider a model if we do not have the accelerator for it:
    bool has_accel = false;
    auto itr = itsAccelerators.find(typ);
    if (itr != itsAccelerators.end() && itr->second > 0) has_accel = true;

    // Add this pipe if it matches our filter and we have the accelerator for it:
    if ((filt == "All" || typ == filt) && has_accel)
    {
      pipes.emplace_back(typ + ':' + (std::string)item["postproc"] + ':' + item.name());
      ++ngood;
    }
  }
  LINFO(indent << "Found " << ntot << " pipelines, " << ngood << " passed the filter.");
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::pipe const & JEVOIS_UNUSED_PARAM(param), std::string const & val)
{
  if (val.empty()) return;
  itsPipeThrew = false;
  freeze(false);

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
  // Open the zoo file:
  cv::FileStorage fs(zoofile, cv::FileStorage::READ);
  if (fs.isOpened() == false) LFATAL("Could not open zoo file " << zoofile);

  // Find the desired pipeline:
  std::vector<cv::FileNodeIterator> globals;
  
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
    if (item.name() == "includedir")
    {
      std::string const dir = jevois::absolutePath(zooroot::get(), (std::string)item);
      DIR *dirp = opendir(dir.c_str());
      if (dirp == nullptr) LFATAL("includedir: " << dir << " in zoo file " << zoofile << ": cannot read directory");
      struct dirent * dent;
      while ((dent = readdir(dirp)) != nullptr)
      {
        std::string const node = dent->d_name;
        size_t const nl = node.length();
        if ((nl > 4 && node.substr(nl-4) == ".yml") || (nl > 5 && node.substr(nl-5) == ".yaml"))
        {
          if (selectPipe(dir + "/" + node, tok)) { closedir(dirp); return true; }
        }
      }
      closedir(dirp);
    }
    
    // This is an entry for a pipeline with a bunch of params under it:
    else if (item.isMap())
    {
      if (item.name() != tok.back()) continue;
      if (tok.size() == 1) { node = item; break; }
      if (tok.size() != 3) LFATAL("Malformed pipeline name: " << jevois::join(tok, ":"));
      
      if (tok[0] == "VPU")
      {
        if ((std::string)item["nettype"] == "OpenCV" &&
            ((std::string)item["postproc"] == tok[1] || postproc::strget() == tok[1]) &&
            (std::string)item["backend"] == "InferenceEngine")
        { node = item; break; }
      }
      else
      {
        if ((std::string)item["nettype"] == tok[0] &&
            ((std::string)item["postproc"] == tok[1] || postproc::strget() == tok[1]))
        { node = item; break; }
      }
    }
    // Global parameter in the current file, keep it for later, if we find our pipe:
    else globals.emplace_back(fit);
  }
  
  // If the spec was not a match with any entries in the file, return false:
  if (node.empty()) return false;
      
  // Found the pipe. First nuke our current pre/net/post:
  asyncNetWait();
  itsPreProcessor.reset(); try { removeSubComponent("preproc"); } catch (...) { }
  itsNetwork.reset(); try { removeSubComponent("network"); } catch (...) { }
  itsPostProcessor.reset(); try { removeSubComponent("postproc"); } catch (...) { }

  // Then set all the global parameters of the current file:
  for (cv::FileNodeIterator fit : globals)
  {
    cv::FileNode item = *fit;
    setZooParam(item, zoofile, fs.root());
  }
  
  // Then iterate over all pipeline params and set them:
  for (cv::FileNodeIterator fit = node.begin(); fit != node.end(); ++fit)
  {
    cv::FileNode item = *fit;
    setZooParam(item, zoofile, node);
  }
  return true;
}

// ####################################################################################################
void jevois::dnn::Pipeline::setZooParam(cv::FileNode & item, std::string const & zf, cv::FileNode const & node)
{
  std::string k = item.name();
  std::string v;
  switch (item.type())
  {
  case cv::FileNode::INT: v = std::to_string((int)item); break;
  case cv::FileNode::REAL: v = std::to_string((float)item); break;
  case cv::FileNode::STRING: v = (std::string)item; break;
  default: LFATAL("Invalid zoo parameter " << k << " type " << item.type() << " in " << zf << " node " << node.name());
  }
  
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
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::preproc const & JEVOIS_UNUSED_PARAM(param),
                                          pipeline::PreProc const & val)
{
  itsPreProcessor.reset(); try { removeSubComponent("preproc"); } catch (...) { }
  
  switch (val)
  {
  case jevois::dnn::pipeline::PreProc::Blob:
    itsPreProcessor = addSubComponent<jevois::dnn::PreProcessorBlob>("preproc");
    break;

  case jevois::dnn::pipeline::PreProc::Custom:
    // nothing here, user must call setCustomPreProcessor() later
    break;
  }

  if (itsPreProcessor) LINFO("Instantiated pre-processor of type " << itsPreProcessor->className());
  else LINFO("No pre-processor");
}

// ####################################################################################################
void jevois::dnn::Pipeline::setCustomPreProcessor(std::shared_ptr<jevois::dnn::PreProcessor> pp)
{
  itsPreProcessor.reset(); try { removeSubComponent("preproc"); } catch (...) { }
  itsPreProcessor = pp;
  preproc::set(jevois::dnn::pipeline::PreProc::Custom);

  if (itsPreProcessor) LINFO("Attached pre-processor of type " << itsPreProcessor->className());
  else LINFO("No pre-processor");
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::nettype const & JEVOIS_UNUSED_PARAM(param),
                                          pipeline::NetType const & val)
{
  asyncNetWait(); // If currently processing async net, wait until done

  itsNetwork.reset(); try { removeSubComponent("network"); } catch (...) { }
  
  switch (val)
  {
  case jevois::dnn::pipeline::NetType::OpenCV:
    itsNetwork = addSubComponent<jevois::dnn::NetworkOpenCV>("network");
    break;

#ifdef JEVOIS_PRO

  case jevois::dnn::pipeline::NetType::NPU:
#ifdef JEVOIS_PLATFORM
    itsNetwork = addSubComponent<jevois::dnn::NetworkNPU>("network");
#else // JEVOIS_PLATFORM
    LFATAL("NPU network is only supported on JeVois-Pro Platform");
#endif
    break;
    
  case jevois::dnn::pipeline::NetType::TPU:
    itsNetwork = addSubComponent<jevois::dnn::NetworkTPU>("network");
    break;
#endif
    
  case jevois::dnn::pipeline::NetType::Custom:
    // Nothing here, user must call setCustomNetwork() later
    break;
  }

  if (itsNetwork) LINFO("Instantiated network of type " << itsNetwork->className());
  else LINFO("No network");

  itsInputAttrs.clear();
}

// ####################################################################################################
void jevois::dnn::Pipeline::setCustomNetwork(std::shared_ptr<jevois::dnn::Network> n)
{
  asyncNetWait(); // If currently processing async net, wait until done
  itsNetwork.reset(); try { removeSubComponent("network"); } catch (...) { }
  itsNetwork = n;
  nettype::set(jevois::dnn::pipeline::NetType::Custom);

  if (itsNetwork) LINFO("Attached network of type " << itsNetwork->className());
  else LINFO("No network");

  itsInputAttrs.clear();
}

// ####################################################################################################
void jevois::dnn::Pipeline::onParamChange(pipeline::postproc const & JEVOIS_UNUSED_PARAM(param),
                                          pipeline::PostProc const & val)
{
  asyncNetWait(); // If currently processing async net, wait until done

  itsPostProcessor.reset(); try { removeSubComponent("postproc"); } catch (...) { }

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
  case jevois::dnn::pipeline::PostProc::Custom:
    // Nothing here, user must call setCustomPostProcessor() later
    break;
  }

  if (itsPostProcessor) LINFO("Instantiated post-processor of type " << itsPostProcessor->className());
  else LINFO("No post-processor");
}

// ####################################################################################################
void jevois::dnn::Pipeline::setCustomPostProcessor(std::shared_ptr<jevois::dnn::PostProcessor> pp)
{
  asyncNetWait(); // If currently processing async net, wait until done
  itsPostProcessor.reset(); try { removeSubComponent("postproc"); } catch (...) { }
  itsPostProcessor = pp;
  postproc::set(jevois::dnn::pipeline::PostProc::Custom);

  if (itsPostProcessor) LINFO("Attached post-processor of type " << itsPostProcessor->className());
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
  if (itsZooChanged) { itsZooChanged = false; zoo::set(zoo::get()); }

  // If the pipeline is throwing exception at any stage, do not do anything here, itsPipeThrew is cleared when selecting
  // a new pipe:
  if (itsPipeThrew) return;
  
  bool const ovl = overlay::get();
  itsOutImgY = 5; // y text position when using outimg text drawings
  
#ifdef JEVOIS_PRO
  // Open an info window if using GUI and not idle:
  if (helper && idle == false) ImGui::Begin((instanceName() + ':' + getParamStringUnique("pipe")).c_str());
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
        }
        
        // Report/draw post-processing results on every frame:
        itsPostProcessor->report(mod, outimg, helper, ovl, idle);
      }
      break;
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

  // Update our rolling average of total processing time:
  itsSecsSum += itsProcSecs[0] + itsProcSecs[1] + itsProcSecs[2];
  ++itsSecsSumNum;
  if (itsSecsSumNum == 20) { itsSecsAvg = itsSecsSum / itsSecsSumNum; itsSecsSum = 0.0; itsSecsSumNum = 0; }
  
#ifdef JEVOIS_PRO
  // Report processing times and close info window if we opened it:
  if (helper)
  {
    std::string total;
    if (idle == false || ovl) total = jevois::secs2str(itsSecsAvg);
    
    if (idle == false)
    {
      if (ImGui::CollapsingHeader("Processing Times", ImGuiTreeNodeFlags_DefaultOpen))
      {
        for (std::string const & s : itsProcTimes) ImGui::TextUnformatted(s.c_str());
        ImGui::Text("OVERALL: %s/inference", total.c_str());
      }
      ImGui::End();
    }
    
    if (ovl)
    {
      for (std::string const & s : itsProcTimes) helper->itext(s);
      helper->itext("OVERALL: " + total + "/inference");
    }
  }
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

