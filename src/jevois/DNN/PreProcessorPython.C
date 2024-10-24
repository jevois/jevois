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

#include <jevois/DNN/PreProcessorPython.H>
#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/PythonModule.H>
#include <jevois/Core/Engine.H>
#include <jevois/DNN/Utils.H>

// ####################################################################################################
namespace jevois
{
  namespace dnn
  {
    class PreProcessorPythonImpl : public Component, public PythonWrapper
    {
      public:
        using Component::Component;
        void loadpy(std::string const & pypath);
        virtual ~PreProcessorPythonImpl();
        void freeze(bool doit);
        std::vector<cv::Mat> process(cv::Mat const & img, bool swaprb, std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                     std::vector<cv::Rect> & crops);
        void report(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                    jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false);
    };
  }
}

// ####################################################################################################
// ####################################################################################################
jevois::dnn::PreProcessorPythonImpl::~PreProcessorPythonImpl()
{
  engine()->unRegisterPythonComponent(this);
}

// ####################################################################################################
void jevois::dnn::PreProcessorPythonImpl::freeze(bool doit)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "freeze")) PythonWrapper::pyinst().attr("freeze")(doit);
}

// ####################################################################################################
void jevois::dnn::PreProcessorPythonImpl::loadpy(std::string const & pypath)
{
  // Load the code and instantiate the python object:
  PythonWrapper::pythonload(JEVOIS_SHARE_PATH "/" + pypath);
  LINFO("Loaded " << pypath);

  // Now that we are fully up and ready, call python module's init() function if implemented:
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "init")) PythonWrapper::pyinst().attr("init")();
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::PreProcessorPythonImpl::process(cv::Mat const & img, bool swaprb,
                                                                  std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                                                  std::vector<cv::Rect> & crops)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "process") == false)
    LFATAL("No process() method provided. It is required, please add it to your Python pre-processor.");

  // Convert the attrs to a list of strings:
  boost::python::list alist;
  for (vsi_nn_tensor_attr_t const & a : attrs) alist.append(jevois::dnn::attrstr(a));

  // Run the python code:
  boost::python::object ret = PythonWrapper::pyinst().attr("process")(img, swaprb, alist);

  // We expect a tuple with first a vector of blobs, then a vector of crops:
  if (boost::python::len(ret) != 2)
    throw std::invalid_argument("Expected two return lists for blobs,crops but received " +
                                std::to_string(boost::python::len(ret)));

  // For the crops, we want cv::Rect but they are not exposing it to python it seems. So let's just accept tuples
  // ( (x,y), (w,h) )
#define CROPERR "PreProcessorPython::process: crops should be ( (x,y), (w,h) )"

  crops.clear();
  boost::python::list cl = boost::python::extract<boost::python::list>(ret[1]);
  for (ssize_t i = 0; i < boost::python::len(cl); ++i)
  {
    boost::python::tuple tup = boost::python::extract<boost::python::tuple>(cl[i]);
    if (boost::python::len(tup) != 2) throw std::runtime_error(CROPERR);
    boost::python::tuple xy = boost::python::extract<boost::python::tuple>(tup[0]);
    if (boost::python::len(xy) != 2) throw std::runtime_error(CROPERR);
    boost::python::tuple wh = boost::python::extract<boost::python::tuple>(tup[1]);
    if (boost::python::len(wh) != 2) throw std::runtime_error(CROPERR);

    float x = boost::python::extract<float>(xy[0]);
    float y = boost::python::extract<float>(xy[1]);
    float w = boost::python::extract<float>(wh[0]);
    float h = boost::python::extract<float>(wh[1]);
    crops.emplace_back(cv::Rect(x, y, w, h));
  }

  // For the blobs, we have a converter:
  boost::python::list ml = boost::python::extract<boost::python::list>(ret[0]);
  return jevois::python::pyListToVec<cv::Mat>(ml);
}

// ####################################################################################################
void jevois::dnn::PreProcessorPythonImpl::report(jevois::StdModule *, jevois::RawImage * outimg,
                                                 jevois::OptGUIhelper * helper, bool overlay, bool idle)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "report") == false)
    LFATAL("No process() method provided. It is required, please add it to your Python pre-processor.");

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
jevois::dnn::PreProcessorPython::PreProcessorPython(std::string const & instance) :
    jevois::dnn::PreProcessor(instance)
{
  itsImpl = addSubComponent<jevois::dnn::PreProcessorPythonImpl>("pypre");
}

// ####################################################################################################
jevois::dnn::PreProcessorPython::~PreProcessorPython()
{ }

// ####################################################################################################
void jevois::dnn::PreProcessorPython::freeze(bool doit)
{
  // First our own params:
  pypre::freeze(doit);
  
  // Then our python params:
  itsImpl->freeze(doit);
}

// ####################################################################################################
void jevois::dnn::PreProcessorPython::onParamChange(jevois::dnn::preprocessor::pypre const &,
                                                    std::string const & newval)
{
  if (newval.empty() == false) itsImpl->loadpy(newval);
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::PreProcessorPython::process(cv::Mat const & img, bool swaprb,
                                                              std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                                              std::vector<cv::Rect> & crops)
{ return itsImpl->process(img, swaprb, attrs, crops); }

// ####################################################################################################
void jevois::dnn::PreProcessorPython::report(jevois::StdModule * mod, jevois::RawImage * outimg,
                                              jevois::OptGUIhelper * helper, bool overlay, bool idle)
{ itsImpl->report(mod, outimg, helper, overlay, idle); }

