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

#include <jevois/DNN/PostProcessorStub.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIhelper.H>

// ####################################################################################################
jevois::dnn::PostProcessorStub::~PostProcessorStub()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorStub::freeze(bool JEVOIS_UNUSED_PARAM(doit))
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorStub::process(std::vector<cv::Mat> const & JEVOIS_UNUSED_PARAM(outs),
                                             jevois::dnn::PreProcessor * JEVOIS_UNUSED_PARAM(preproc))
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorStub::report(jevois::StdModule * JEVOIS_UNUSED_PARAM(mod),
                                            jevois::RawImage * JEVOIS_UNUSED_PARAM(outimg),
                                            jevois::OptGUIhelper * JEVOIS_UNUSED_PARAM(helper),
                                            bool JEVOIS_UNUSED_PARAM(overlay), bool JEVOIS_UNUSED_PARAM(idle))
{ }
