// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2022 by Laurent Itti, the University of Southern
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
#include <hailo/hailort.hpp>

namespace hailort
{ class Device; class ConfiguredNetworkGroup; class InputVStream; class OutputVStream; class ActivatedNetworkGroup; }

namespace jevois
{
  namespace dnn
  {
    //! Wrapper around an DNN neural network running on the Hailo8 neural accelerator
    /*! Note that this network type does not support setting intensors or outtensors externally, these are fixed in the
        HEF file. \ingroup dnn */
    class NetworkHailo : public jevois::dnn::Network,
                         public jevois::Parameter<network::dataroot, network::model, network::dequant, network::turbo>
    {
      public:
        //! Constructor
        NetworkHailo(std::string const & instance);

        //! Destructor
        virtual ~NetworkHailo();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit) override;

        //! Get shapes of all input tensors
        virtual std::vector<vsi_nn_tensor_attr_t> inputShapes() override;

        //! Hailo-only get input stream infos
        std::vector<hailo_vstream_info_t> inputInfos() const;
        
        //! Get shapes of all output tensors
        virtual std::vector<vsi_nn_tensor_attr_t> outputShapes() override;
 
        //! Hailo-only get input stream infos, can be used to post-process quantized outputs
        std::vector<hailo_vstream_info_t> outputInfos() const;

      protected:
        //! Load from disk
        void load() override;

        //! Process input blobs and obtain output blobs
        std::vector<cv::Mat> doprocess(std::vector<cv::Mat> const & blobs,
                                       std::vector<std::string> & info) override;

        void onParamChange(network::turbo const & par, bool const & newval) override;
        
      private:
        std::unique_ptr<hailort::Device> itsDevice;
        std::shared_ptr<hailort::ConfiguredNetworkGroup> itsNetGroup;
        std::vector<hailort::InputVStream> itsInStreams;
        std::vector<hailort::OutputVStream> itsOutStreams;
        std::unique_ptr<hailort::ActivatedNetworkGroup> itsActiveNetGroup;
        std::vector<vsi_nn_tensor_attr_t> itsInAttrs, itsOutAttrs;
        std::vector<cv::Mat> itsRawOutMats, itsOutMats;
    };
    
  } // namespace dnn
} // namespace jevois

#endif // JEVOIS_PRO
