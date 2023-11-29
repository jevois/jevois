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

#ifdef JEVOIS_PRO

#include <jevois/DNN/Network.H>
#include <jevois/Types/Enum.H>
#include <onnxruntime_cxx_api.h>
#include <opencv2/core.hpp>

namespace jevois
{
  namespace dnn
  {
    //! Wrapper around an ONNX-Runtime neural network
    /*! \ingroup dnn */
    class NetworkONNX : public jevois::dnn::Network,
                        public jevois::Parameter<network::dataroot, network::model>
    {
      public:
        //! Inherited constructor ok
        NetworkONNX(std::string const & instance);

        //! Destructor
        virtual ~NetworkONNX();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit) override;

        //! Get shapes of all input tensors
        virtual std::vector<vsi_nn_tensor_attr_t> inputShapes() override;

        //! Get shapes of all output tensors
        virtual std::vector<vsi_nn_tensor_attr_t> outputShapes() override;

      protected:
        //! Load from disk
        void load() override;

        //! Process input blobs and obtain output blobs
        std::vector<cv::Mat> doprocess(std::vector<cv::Mat> const & blobs,
                                       std::vector<std::string> & info) override;

      private:
        std::shared_ptr<Ort::Session> itsSession;
        Ort::Env itsEnv;
        Ort::SessionOptions itsSessionOptions;
        std::vector<vsi_nn_tensor_attr_t> itsInAttrs;
        std::vector<vsi_nn_tensor_attr_t> itsOutAttrs;

        std::vector<Ort::AllocatedStringPtr> itsInNamePtrs;
        std::vector<char const *> itsInNames;
        std::vector<Ort::AllocatedStringPtr> itsOutNamePtrs;
        std::vector<char const *> itsOutNames;

        std::vector<Ort::Value> itsOutputs;
    };
    
  } // namespace dnn
} // namespace jevois

#endif // JEVOIS_PRO
