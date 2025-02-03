// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2024 by Laurent Itti, the University of Southern
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

#include <jevois/DNN/YOLOjevois.H>
#include <jevois/DNN/CLIP.H>
#include <jevois/DNN/Utils.H>
#include <jevois/DNN/NetworkONNX.H>
#include <jevois/GPU/GUIhelper.H>

// ####################################################################################################
jevois::dnn::YOLOjevois::YOLOjevois(std::string const & instance, std::map<int, std::string> & labels) :
    jevois::Component(instance), itsLabels(labels)
{ }

// ####################################################################################################
void jevois::dnn::YOLOjevois::setup(size_t nclass, jevois::GUIhelper * helper, std::shared_ptr<jevois::dnn::Network> net)
{
  itsNumClasses = nclass;
  itsHelper = helper;
  itsNetwork = net;
}

// ####################################################################################################
jevois::dnn::YOLOjevois::~YOLOjevois()
{ }

// ####################################################################################################
void jevois::dnn::YOLOjevois::freeze(bool doit)
{
  clipmodel::freeze(doit);
  textmodel::freeze(doit);
}

// ####################################################################################################
int jevois::dnn::YOLOjevois::textEmbeddingSize()
{
  if (ready() == false) LFATAL("Not ready yet");
  if (itsCLIP) return itsCLIP->textEmbeddingSize();
  else return 0;
}

// ####################################################################################################
int jevois::dnn::YOLOjevois::imageEmbeddingSize()
{
  if (ready() == false) LFATAL("Not ready yet");
  if (itsCLIP) return itsCLIP->imageEmbeddingSize();
  else return 0;
}

namespace
{
  // Caution this function checks nothing, only to be used internally here:
  inline void setEmbedding(cv::Mat & all, size_t clsid, cv::Mat const & e)
  {
    memcpy(all.data + clsid * e.cols * sizeof(float), e.data, e.cols * sizeof(float));
  }
}

// ####################################################################################################
void jevois::dnn::YOLOjevois::update(size_t const classnum, std::string const & label)
{
  if (ready() == false) LFATAL("Not ready");
  if (itsCLIP->textEmbeddingSize() == 0) LFATAL("Our CLIP model does not have a text encoder");
  if (classnum >= itsNumClasses) LFATAL("Invalid class #" << classnum << " given " << itsNumClasses << " classes");

  setEmbedding(itsEmbeddings, classnum, itsCLIP->textEmbedding(label));
  itsLabels[classnum] = label;
  itsCLIPimages[classnum] = cv::Mat();
  updateMainNetwork();
  itsHelper->reportInfo("Updated class " + std::to_string(classnum) + " to [" + label + ']');
}

// ####################################################################################################
void jevois::dnn::YOLOjevois::update(size_t const classnum, cv::Mat const & img)
{
  if (ready() == false) LFATAL("Not ready");
  if (itsCLIP->imageEmbeddingSize() == 0) LFATAL("Our CLIP model does not have an image encoder");
  if (classnum >= itsNumClasses) LFATAL("Invalid class #" << classnum << " given " << itsNumClasses << " classes");

  setEmbedding(itsEmbeddings, classnum, itsCLIP->imageEmbedding(img));
  itsLabels[classnum] = "<image for class " + std::to_string(classnum) + '>';
  itsCLIPimages[classnum] = img;
  updateMainNetwork();
  itsHelper->reportInfo("Updated class " + std::to_string(classnum) + " from image");
}

// ####################################################################################################
bool jevois::dnn::YOLOjevois::ready()
{
  // If we are loaded, we are ready to process:
  if (itsLoaded.load()) return true;
  
  // If we are loading, check whether loading is complete or threw, otherwise return false as we keep loading:
  if (itsLoading.load())
  {
    if (itsLoadFut.valid() && itsLoadFut.wait_for(std::chrono::milliseconds(2)) == std::future_status::ready)
    {
      try { itsLoadFut.get(); LINFO("YOLOjevois loaded."); return true; }
      catch (...) { itsLoading.store(false); jevois::warnAndRethrowException(); }
    }
    return false;
  }

  // Otherwise, trigger an async load:
  itsLoading.store(true);
  itsLoadFut = jevois::async(std::bind(&jevois::dnn::YOLOjevois::load, this));
  LINFO("Loading YOLOjevois helper networks...");

  return false;
}

// ####################################################################################################
void jevois::dnn::YOLOjevois::load()
{
  if (! itsNetwork || itsNumClasses == 0 || itsHelper == nullptr) LFATAL("Need to call setup() first");
  
  itsCLIP.reset();
  if (itsAuxNet) { itsAuxNet.reset(); removeSubComponent("auxnet", false); }

  // First, load the CLIP model:
  std::string const clipname = yolojevois::clipmodel::get();
  if (clipname.empty()) return;
  itsCLIP.reset(new jevois::dnn::CLIP(jevois::absolutePath(JEVOIS_SHARE_PATH "/clip", clipname)));
  if (itsCLIP->textEmbeddingSize() == 0) LFATAL("CLIP model must have at least a text encoder");
  bool const has_image_encoder = (itsCLIP->imageEmbeddingSize() > 0) ? true : false;

  // Then process all the labels through the CLIP encoder:
  int const vec_dim = itsCLIP->textEmbeddingSize();
  itsEmbeddings = cv::Mat(std::vector<int> { 1, int(itsNumClasses), vec_dim }, CV_32F );
  itsCLIPimages.clear();
    
  for (size_t i = 0; i < itsNumClasses; ++i)
  {
    std::string label = jevois::dnn::getLabel(itsLabels, i, true);
    cv::Mat img;
    
    if (label.empty() || jevois::stringStartsWith(label, "<live-selected "))
    {
      itsHelper->reportError("Invalid label for class " + std::to_string(i) + " -- FORCING TO 'person'");
      label = "person"; itsLabels[i] = label;
    }
      
    if (jevois::stringStartsWith(label, "imagefile:"))
    {
      if (has_image_encoder)
      {
        // Class is defined by an image on disk; load it and compute embedding:
        std::string imgpath = jevois::absolutePath(JEVOIS_CUSTOM_DNN_PATH, label.substr(10));
        cv::Mat img_bgr = cv::imread(imgpath, cv::IMREAD_COLOR);
        if (img_bgr.empty())
        {
          itsHelper->reportError("Failed to read " + imgpath + " -- FORCING CLASS "+std::to_string(i)+" TO 'person'");
          label = "person"; itsLabels[i] = label;
          setEmbedding(itsEmbeddings, i, itsCLIP->textEmbedding(label));
        }
        else
        {
          cv::cvtColor(img_bgr, img, cv::COLOR_BGR2RGB);
          LINFO("Computing CLIP image embedding for class " << i << " [" << imgpath << "] ...");
          setEmbedding(itsEmbeddings, i, itsCLIP->imageEmbedding(img));
          itsLabels[i] = "<from image file>"; // we lose the file name here but will recompute it on save anyway
        }
      }
      else
      {
        itsHelper->reportError("No CLIP image encoder -- FORCING CLASS "+std::to_string(i)+" TO 'person'");
        label = "person"; itsLabels[i] = label;
        setEmbedding(itsEmbeddings, i, itsCLIP->textEmbedding(label));
      }
    }
    else
    {
      LINFO("Computing CLIP text embedding for class " << i << " [" << label << "] ...");
      setEmbedding(itsEmbeddings, i, itsCLIP->textEmbedding(label));
    }
    itsCLIPimages.emplace_back(img);
  }
  LINFO("CLIP embeddings ready for " << itsNumClasses << " object classes");
  
  // Then possibly load the ONNX helper:
  std::string const onnxmodel = yolojevois::textmodel::get();
  if (onnxmodel.empty() == false)
  {
    std::string const m = jevois::absolutePath(JEVOIS_SHARE_PATH, onnxmodel);
    LINFO("Loading embedding helper " << m << " ...");

    itsAuxNet = addSubComponent<jevois::dnn::NetworkONNX>("auxnet");
    itsAuxNet->hideAllParams(true);
    itsAuxNet->setParamStringUnique("model", m);

    int iter = 0;
    while (itsAuxNet->ready() == false && iter++ < 1000) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (iter == 1000) LFATAL("Timeout waiting for embedding helper to load...");

    LINFO("Embedding helper ready.");
  }

  // We are officially loaded now:
  itsLoaded.store(true);
  itsLoading.store(false);
        
  // Update our outputs:
  updateMainNetwork();
}

// ####################################################################################################
void jevois::dnn::YOLOjevois::updateMainNetwork()
{
  if (ready() == false) LFATAL("Not ready");
  if (! itsNetwork) LFATAL("No main network to update");
  if (itsNetwork->ready() == false) LFATAL("Main network not ready");

  if (itsAuxNet)
  {
    // Run the aux net to get 5 tensors from the CLIP embeddings:
    std::vector<cv::Mat> ins; ins.push_back(itsEmbeddings);
    std::vector<std::string> ignored_info;
    std::vector<cv::Mat> outs = itsAuxNet->process(ins, ignored_info);
    
    // Now update the main network:
    for (size_t i = 0; i < outs.size(); ++i)
      itsNetwork->setExtraInputFromFloat32(i + 1 /* input number */, outs[i]);
  }
  else
  {
    // Using CLIP only, main network should expect only 1 extra input for CLIP embeddings:
    itsNetwork->setExtraInputFromFloat32(1 /* input number */, itsEmbeddings);
  }
  
  LINFO("Updated main network with modified classes.");
}

// ####################################################################################################
cv::Mat const & jevois::dnn::YOLOjevois::image(size_t const classid) const
{
  if (classid >= itsNumClasses) LFATAL("Invalid class id "<<classid<<" (only have " << itsNumClasses<<" classes)");
  return itsCLIPimages[classid];
}

#endif // JEVOIS_PRO

