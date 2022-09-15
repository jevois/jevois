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

#include <jevois/DNN/PostProcessorClassify.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>

// ####################################################################################################
jevois::dnn::PostProcessorClassify::~PostProcessorClassify()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorClassify::freeze(bool doit)
{
  classes::freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorClassify::onParamChange(postprocessor::classes const & JEVOIS_UNUSED_PARAM(param),
                                                       std::string const & val)
{
  if (val.empty()) return;

  // Get the dataroot of our network. We assume that there is a sub-component named "network" that is a sibling of us:
  std::vector<std::string> dd = jevois::split(Component::descriptor(), ":");
  dd.back() = "network"; dd.emplace_back("dataroot");
  std::string const dataroot = engine()->getParamStringUnique(jevois::join(dd, ":"));

  itsLabels = jevois::dnn::readLabelsFile(jevois::absolutePath(dataroot, val));
}

// ####################################################################################################
void jevois::dnn::PostProcessorClassify::process(std::vector<cv::Mat> const & outs,
                                                 jevois::dnn::PreProcessor * JEVOIS_UNUSED_PARAM(preproc))
{
  if (outs.size() != 1 && itsFirstTime)
  {
    itsFirstTime = false;
    LERROR("Expected 1 output tensor, got " << outs.size() << " - USING FIRST ONE");
  }
  
  cv::Mat const & out = outs[0]; uint32_t const sz = out.total();
  if (out.type() != CV_32F) LFATAL("Need FLOAT32 tensor");
  uint32_t topk = top::get(); if (topk > sz) topk = sz;
  uint32_t const fudge = classoffset::get();
  itsObjRec.clear();
  
  uint32_t MaxClass[topk]; float fMaxProb[topk];
  if (softmax::get())
  {
    float sm[out.total()];
    jevois::dnn::softmax((float const *)out.data, sz, 1, 1.0F, sm, false);
    jevois::dnn::topK(sm, fMaxProb, MaxClass, sz, topk);
  }
  else jevois::dnn::topK((float const *)out.data, fMaxProb, MaxClass, sz, topk);

  // Collect the top-k results that are also above threshold:
  float const t = cthresh::get(); float const fac = 100.0F * scorescale::get();
  
  for (uint32_t i = 0; i < topk; ++i)
  {
    if (fMaxProb[i] * fac < t) break;
    jevois::ObjReco o { fMaxProb[i] * fac, jevois::dnn::getLabel(itsLabels, MaxClass[i] + fudge) };
    itsObjRec.push_back(o);
  }
}

// ####################################################################################################
void jevois::dnn::PostProcessorClassify::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                                jevois::OptGUIhelper * helper, bool overlay, bool idle)
{
  uint32_t const topk = top::get();

  // If desired, write results to output image:
  if (outimg && overlay)
  {
    int y = 16;
    jevois::rawimage::writeText(*outimg, jevois::sformat("Top-%u above %.2F%%", topk, cthresh::get()),
                                220, y, jevois::yuyv::White);
    y += 15;
    
    for (jevois::ObjReco const & o : itsObjRec)
    {
      jevois::rawimage::writeText(*outimg, jevois::sformat("%s: %.2F", o.category.c_str(), o.score),
                                  220, y, jevois::yuyv::White);
      y += 11;
    }
  }

#ifdef JEVOIS_PRO
  // If desired, write results to GUI:
  if (helper)
  {
    if (idle == false && ImGui::CollapsingHeader("Classification results", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::Text("Top-%u classes above threshold %.2f", topk, cthresh::get());
      ImGui::Separator();
      uint32_t done = 0;
      for (jevois::ObjReco const & o : itsObjRec) { ImGui::Text("%s: %.2F", o.category.c_str(), o.score); ++done; }
      while (done++ < topk) ImGui::TextUnformatted("-");
    }
    
    if (overlay)
    {
      uint32_t done = 0;
      for (jevois::ObjReco const & o : itsObjRec)
      { helper->itext(jevois::sformat("%s: %.2F", o.category.c_str(), o.score)); ++done; }
      while (done++ < topk) helper->itext("-");
    }
  }
#else
  (void)idle; (void)helper; // keep compiler happy  
#endif
  
  // If desired, send results to serial port:
  if (mod) mod->sendSerialObjReco(itsObjRec);
}
