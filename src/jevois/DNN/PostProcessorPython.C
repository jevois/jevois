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

#include <jevois/DNN/PostProcessorPython.H>
#include <jevois/Core/PythonModule.H>
#include <jevois/Core/PythonSupport.H>
#include <jevois/DNN/PreProcessorPython.H>

// ####################################################################################################
namespace jevois
{
  namespace dnn
  {
    class PostProcessorPythonImpl : public jevois::Component, public PythonWrapper
    {
      public:
        using Component::Component;
        void loadpy(std::string const & pypath);
        virtual ~PostProcessorPythonImpl();
        void freeze(bool doit);
        void process(std::vector<cv::Mat> const & outs, PreProcessor * preproc);
        void report(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                    jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false);
    };
  }
}

// ####################################################################################################
// ####################################################################################################
jevois::dnn::PostProcessorPythonImpl::~PostProcessorPythonImpl()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorPythonImpl::freeze(bool doit)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "freeze")) PythonWrapper::pyinst().attr("freeze")(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorPythonImpl::loadpy(std::string const & pypath)
{
  // Load the code and instantiate the python object:
  PythonWrapper::pythonload(JEVOIS_SHARE_PATH "/" + pypath);
  LINFO("Loaded " << pypath);

  // Now that we are fully up and ready, call python module's init() function if implemented:
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "init")) PythonWrapper::pyinst().attr("init")();
}

// ####################################################################################################
void jevois::dnn::PostProcessorPythonImpl::process(std::vector<cv::Mat> const & outs,
                                                   jevois::dnn::PreProcessor * preproc)
{
  boost::python::list lst = jevois::python::pyVecToList(outs);
  PythonWrapper::pyinst().attr("process")(lst, boost::python::ptr(preproc->getPreProcForPy().get()));
}

// ####################################################################################################
void jevois::dnn::PostProcessorPythonImpl::report(jevois::StdModule *, jevois::RawImage * outimg,
                                                  jevois::OptGUIhelper * helper, bool overlay, bool idle)
{
  // default constructed boost::python::object is None on the python side
  if (outimg)
  {
#ifdef JEVOIS_PRO
    if (helper)
    {
      jevois::GUIhelperPython helperpy(helper);
      PythonWrapper::pyinst().attr("report")(boost::ref(*outimg), boost::ref(helperpy), overlay, idle);
    }
    else
#endif
      PythonWrapper::pyinst().attr("report")(boost::ref(*outimg), boost::python::object(), overlay, idle);
  }
  else
  {
#ifdef JEVOIS_PRO
    if (helper)
    {
      jevois::GUIhelperPython helperpy(helper);
      PythonWrapper::pyinst().attr("report")(boost::python::object(), boost::ref(helperpy), overlay, idle);
    }
    else
#endif
      PythonWrapper::pyinst().attr("report")(boost::python::object(), boost::python::object(), overlay, idle);
  }
  
#ifndef JEVOIS_PRO
  (void)helper; // avoid compiler warning
#endif
}

// ####################################################################################################
// ####################################################################################################
jevois::dnn::PostProcessorPython::PostProcessorPython(std::string const & instance) :
    jevois::dnn::PostProcessor(instance)
{
  itsImpl = addSubComponent<jevois::dnn::PostProcessorPythonImpl>("pypost");
}

// ####################################################################################################
jevois::dnn::PostProcessorPython::~PostProcessorPython()
{ }

// ####################################################################################################
void jevois::dnn::PostProcessorPython::freeze(bool doit)
{
  // First our own params:
  pypost::freeze(doit);
  
  // Then our python params:
  itsImpl->freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PostProcessorPython::onParamChange(jevois::dnn::postprocessor::pypost const &,
                                                     std::string const & newval)
{
  if (newval.empty() == false) itsImpl->loadpy(newval);
}

// ####################################################################################################
void jevois::dnn::PostProcessorPython::process(std::vector<cv::Mat> const & outs, jevois::dnn::PreProcessor * preproc)
{ itsImpl->process(outs, preproc); }

// ####################################################################################################
void jevois::dnn::PostProcessorPython::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                              jevois::OptGUIhelper * helper, bool overlay, bool idle)
{ itsImpl->report(mod, outimg, helper, overlay, idle); }

