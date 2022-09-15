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

#pragma once

#include <jevois/DNN/Network.H>
#include <jevois/Core/PythonWrapper.H>

namespace jevois
{
  namespace dnn
  {
    class NetworkPythonImpl;

    //! Wrapper around an DNN neural network invoked through python
    /*! \ingroup dnn */
    class NetworkPython : public Network, public Parameter<network::pynet>
    {
      public:
        //! Constructor
        NetworkPython(std::string const & instance);

        //! Destructor
        virtual ~NetworkPython();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit) override;

        //! Get shapes of all input tensors
        std::vector<vsi_nn_tensor_attr_t> inputShapes() override;

        //! Get shapes of all output tensors
        std::vector<vsi_nn_tensor_attr_t> outputShapes() override;
 
        //! Load from disk
        void load() override;

        //! Process input blobs and obtain output blobs
        std::vector<cv::Mat> doprocess(std::vector<cv::Mat> const & blobs,
                                       std::vector<std::string> & info) override;

      protected:
        // Load the python code as soon as the file is set, and call init() on it if defined
        void onParamChange(network::pynet const & param, std::string const & newval) override;

        // Python init() may create new dynamic parameters, but in onParamChange() we are locked up, so we need to use a
        // sub-Component for our actual implementation, and the sub can get its dynamic parameters created while our own
        // params are locked up...
        std::shared_ptr<NetworkPythonImpl> itsImpl;
    };
    
  } // namespace dnn
} // namespace jevois

