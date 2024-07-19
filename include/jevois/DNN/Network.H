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

#include <jevois/Component/Component.H>
#include <jevois/Types/Enum.H>

#include <ovxlib/vsi_nn_pub.h> // for data types and quantization types

#include <opencv2/core/core.hpp>
#include <vector>

namespace jevois
{
  namespace dnn
  {
    namespace network
    {
      // We define all parameters for all derived classes here to avoid duplicate definitions. Different derived classes
      // will use different subsets of all available parameters:
      static jevois::ParameterCategory const ParamCateg("DNN Network Options");

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(comment, std::string, "Optional comment about the network",
                               "", ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(url, std::string, "Optional URL for the network",
                               "", ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(dataroot, std::string, "Root directory to use when config or model parameters "
                               "are relative paths.",
                               JEVOIS_SHARE_PATH, ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(config, std::string, "Path to a text file that contains network configuration. "
                               "Can have extension .prototxt (Caffe), .pbtxt (TensorFlow), or .cfg (Darknet). "
                               "If path is relative, it will be prefixed by dataroot.",
                               "", ParamCateg);
      
      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(model, std::string, "Path to a binary file of model contains trained weights. "
                               "Can have extension .caffemodel (Caffe), .pb (TensorFlow), .t7 or .net (Torch), "
                               ".tflite (TensorFlow Lite), or .weights (Darknet). If path is relative, it will be "
                               "prefixed by dataroot.",
                               "", ParamCateg);
      
#ifdef JEVOIS_PRO
      //! Enum \relates jevois::dnn::Network
      JEVOIS_DEFINE_ENUM_CLASS(Target, (CPU) (OpenCL) (OpenCL_FP16) (Myriad) (NPU) );
#else
      //! Enum \relates jevois::dnn::Network
      JEVOIS_DEFINE_ENUM_CLASS(Target, (CPU) );
#endif
      
      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(target, Target, "OpenCV compute target to use. Changes will take effect "
                               "next time you load a different model.",
                               Target::CPU, Target_Values, ParamCateg);
#ifdef JEVOIS_PRO
      //! Enum \relates jevois::dnn::Network
      JEVOIS_DEFINE_ENUM_CLASS(Backend, (OpenCV) (InferenceEngine) (TimVX) );
#define JEVOIS_BACKEND_DEFAULT Backend::OpenCV
#else
      //! Enum \relates jevois::dnn::Network
      JEVOIS_DEFINE_ENUM_CLASS(Backend, (Default) );
#define JEVOIS_BACKEND_DEFAULT Backend::Default
#endif
      
      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(backend, Backend, "OpenCV compute backend to use. Default will use the inference "
                               "engine if available, otherwise OpenCV (note that inference engine only works on Intel "
                               "processors or MyriadX hardware, thus you should normally select OpenCV when running "
                               "on JeVois-Pro Platform, unless you want to use an optional MyriadX accelerator). "
                               "Changes will take effect next time you load a model.",
                               JEVOIS_BACKEND_DEFAULT, Backend_Values, ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(intensors, std::string, "Specification of input tensors",
                               "", ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(extraintensors, std::string, "Specification of extra fixed input tensors that will be "
                               "added after the regular intensors. Format is: "
                               "<type>:<shape>:val1 val2 ... valN, <type>:<shape>:val1 ... valN. For example, for "
                               "URetinex-Net: 32F:1x1x1:3.0",
                               "", ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(outtensors, std::string, "Specification of output tensors",
                               "", ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(outreshape, std::string, "Specification of reshaped output tensors; "
                               "sometimes useful to re-interpret tensors to what a post-processor expects; for "
                               "example, TPU YoloV4-Int-VOC outputs 5D tensors 32F:1x52x52x3x85, 32F:1x26x26x3x85, "
                               "32F:1x13x13x3x85 but the YOLO post-processor expects 4D, which would be specified here "
                               "as 32F:1x52x52x255, 32F:1x26x26x255, 32F:1x13x13x255. Note that this only changes "
                               "the description of dimensions, but does not move any pixel data around (e.g., cannot "
                               "convert from NCHW to NHWC, convert data types, etc). Use sparingly and with caution.",
                               "", ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(dequant, bool, "Dequantize output tensors to float32 from their native quantized type",
                               true, ParamCateg);

      //! Parameter \relates jevois::dnn::NetworkPython
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(pynet, std::string, "Full path of the python network processor file. "
                                             "Name of class defined in the file must match the file name without "
                                             "the trailing '.py'",
                                             "", ParamCateg);
#ifdef JEVOIS_PRO
      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(tpunum, size_t, "Coral EdgeTPU number to use to run this model, typically 0, or can be "
                               "1 when using a dual-TPU add-on board, or more when using additional TPUs connected "
                               "to USB ports",
                               0, ParamCateg);

      //! Parameter \relates jevois::dnn::Network
      JEVOIS_DECLARE_PARAMETER(spunum, size_t, "Hailo8 device number to use to run this model, typically 0 unless "
                               "several Hailo8 accelerators are connected to the system",
                               0, ParamCateg);

      //! Parameter \relates jevois::dnn::NetworkNPU
      JEVOIS_DECLARE_PARAMETER(verifygraph, bool, "Verify NPU graph after loading it",
                               true, ParamCateg);

      //! Parameter \relates jevois::dnn::NetworkNPU
      JEVOIS_DECLARE_PARAMETER(ovxver, std::string, "ovxlib version to use with NPU network, or leave blank "
                               "to use latest version",
                               "", boost::regex("^$|^[0-9]+\\.[0-9]+\\.[0-9]+$"), ParamCateg);

      //! Parameter \relates jevois::dnn::NetworkHailo
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(turbo, bool, "Turbo mode. Has no significant effect on small or "
                                             "fast networks. Use with caution as it may lead to overheating, not "
                                             "recommended for production",
                                             false, ParamCateg);
#endif
    }
    
    //! Abstract class to represent a neural network
    /*! Derived classes provide implementation via OpenCV (on CPU, OpenCL, or OpenVino/Myriad-X), Amlogic/Vivante NPU,
        Hailo-8, Python, or Google Coral TPU. \ingroup dnn */
    class Network : public Component,
                    public Parameter<network::comment, network::url, network::outreshape, network::extraintensors>
    {
      public:
        //! Inherited constructor ok
        using jevois::Component::Component;

        //! Destructor
        /*! CAUTION: derived classes must call waitBeforeDestroy() in their destructor */
        virtual ~Network();

        //! If network is currently loading, wait until that is done before destroying
        /*! CAUTION: derived classes must call waitBeforeDestroy() in their destructor */
        void waitBeforeDestroy();
        
        //! Returns true when network is ready to run (loaded and initialized)
        bool ready();

        //! Get shapes of all input tensors
        virtual std::vector<vsi_nn_tensor_attr_t> inputShapes() = 0;

        //! Get shapes of all output tensors
        virtual std::vector<vsi_nn_tensor_attr_t> outputShapes() = 0;
        
        //! Process input blobs and obtain output blobs
        /*! Network implementations may push information data into the info string, which will be displayed to
            user. Convention is: if an info line starts with '* ', it is a header, and if it starts with '- ' it is a
            bullet. Info should always be organized into headers at the top level. */
        std::vector<cv::Mat> process(std::vector<cv::Mat> const & blobs, std::vector<std::string> & info);

        //! Freeze/unfreeze parameters that users should not change while running
        /*! Note: derived classes can freeze their own params by overriding this function, and should remember to still
            call the base class jevois::dnn::Network::freeze(doit) */
        virtual void freeze(bool doit);
        
      protected:
        //! Load from disk
        virtual void load() = 0;

        //! Process input blobs and obtain output blobs
        virtual std::vector<cv::Mat> doprocess(std::vector<cv::Mat> const & blobs,
                                               std::vector<std::string> & info) = 0;

        void onParamChange(network::outreshape const & param, std::string const & val) override;
        
      private:
        std::atomic<bool> itsLoading = false;
        std::atomic<bool> itsLoaded = false;
        std::future<void> itsLoadFut;
        std::vector<vsi_nn_tensor_attr_t> itsReshape;
    };
    
  } // namespace dnn
} // namespace jevois
