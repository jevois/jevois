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

#include <jevois/DNN/NetworkPython.H>
#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/Engine.H>
#include <jevois/DNN/Utils.H>

// ####################################################################################################
namespace jevois
{
  namespace dnn
  {
    class NetworkPythonImpl : public Component, public PythonWrapper
    {
      public:
        using Component::Component;
        void loadpy(std::string const & pypath);
        virtual ~NetworkPythonImpl();
        void freeze(bool doit);
        void load();
        std::vector<cv::Mat> doprocess(std::vector<cv::Mat> const & outs, std::vector<std::string> & info);
    };
  }
}

// ####################################################################################################
// ####################################################################################################
jevois::dnn::NetworkPythonImpl::~NetworkPythonImpl()
{
  engine()->unRegisterPythonComponent(this);
}

// ####################################################################################################
void jevois::dnn::NetworkPythonImpl::freeze(bool doit)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "freeze")) PythonWrapper::pyinst().attr("freeze")(doit);
}

// ####################################################################################################
void jevois::dnn::NetworkPythonImpl::loadpy(std::string const & pypath)
{
  // Load the code and instantiate the python object:
  PythonWrapper::pythonload(JEVOIS_SHARE_PATH "/" + pypath);
  LINFO("Loaded " << pypath);

  // Now that we are fully up and ready, call python module's init() function if implemented:
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "init")) PythonWrapper::pyinst().attr("init")();
}

// ####################################################################################################
void jevois::dnn::NetworkPythonImpl::load()
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "load")) PythonWrapper::pyinst().attr("load")();
  else LFATAL("No load() method provided. It is required, please add it to your Python network processor.");
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkPythonImpl::doprocess(std::vector<cv::Mat> const & blobs,
                                                               std::vector<std::string> & info)
{
  if (jevois::python::hasattr(PythonWrapper::pyinst(), "process") == false)
    LFATAL("No process() method provided. It is required, please add it to your Python network processor.");
  boost::python::list bloblst = jevois::python::pyVecToList(blobs);
  boost::python::object ret = PythonWrapper::pyinst().attr("process")(bloblst);

  boost::python::tuple rett = boost::python::extract<boost::python::tuple>(ret);
  if (boost::python::len(rett) != 2) LFATAL("Expected tuple(list of output blobs, list of info strings)");

  std::vector<cv::Mat> retm = jevois::python::pyListToVec<cv::Mat>(rett[0]);
  info = jevois::python::pyListToVec<std::string>(rett[1]);

  return retm;
}

// ####################################################################################################
// ####################################################################################################
jevois::dnn::NetworkPython::NetworkPython(std::string const & instance) :
    jevois::dnn::Network(instance)
{
  itsImpl = addSubComponent<jevois::dnn::NetworkPythonImpl>("pynet");
}

// ####################################################################################################
jevois::dnn::NetworkPython::~NetworkPython()
{ }

// ####################################################################################################
void jevois::dnn::NetworkPython::freeze(bool doit)
{
  // First our own params:
  pynet::freeze(doit);
  
  // Then our python params:
  itsImpl->freeze(doit);

  jevois::dnn::Network::freeze(doit); // base class parameters
}

// ####################################################################################################
void jevois::dnn::NetworkPython::onParamChange(jevois::dnn::network::pynet const &, std::string const & newval)
{
  if (newval.empty() == false) itsImpl->loadpy(newval);
}

// ####################################################################################################
std::vector<cv::Mat> jevois::dnn::NetworkPython::doprocess(std::vector<cv::Mat> const & outs,
                                                           std::vector<std::string> & info)
{ return itsImpl->doprocess(outs, info); }

// ####################################################################################################
void jevois::dnn::NetworkPython::load()
{ itsImpl->load(); }

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkPython::inputShapes()
{ return jevois::dnn::parseTensorSpecs(getParamStringUnique("intensors")); }

// ####################################################################################################
std::vector<vsi_nn_tensor_attr_t> jevois::dnn::NetworkPython::outputShapes()
{ return jevois::dnn::parseTensorSpecs(getParamStringUnique("outtensors")); }
 
