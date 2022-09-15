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

  // Show info about input tensors:
  info.emplace_back("* Input Tensors");
  for (size_t i = 0; i < blobs.size(); ++i) info.emplace_back("- " + jevois::dnn::shapestr(blobs[i]));

  // Run processing on the derived class:
  info.emplace_back("* Network");
  std::string const c = comment::get();
  if (c.empty() == false) info.emplace_back(c);
  
  std::vector<cv::Mat> outs = doprocess(blobs, info);

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
