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
#include <jevois/GPU/GUIhelper.H>
#include <jevois/Debug/Timer.H>
#include <jevois/Types/Enum.H>
#include <ovxlib/vsi_nn_pub.h> // for data types and quantization types

namespace jevois
{
  class StdModule;
  class RawImage;
  
  namespace dnn
  {
    class PreProcessor;
    class Network;
    class PostProcessor;
    
    namespace pipeline
    {
      static jevois::ParameterCategory const ParamCateg("DNN Pipeline Options");

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(zooroot, std::string, "Path where to find zoo files (.yml). "
                                             "If not absolute, it is relative to this module's path",
                                             JEVOIS_SHARE_PATH "/dnn", ParamCateg);

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(zoo, std::string, "Filename for neural network zoo file (.yml). "
                                             "If not absolute, it is relative to zooroot",
                                             "models.yml", ParamCateg);
      //! Enum \relates jevois::Pipeline
      JEVOIS_DEFINE_ENUM_CLASS(Filter, (All) (OpenCV) (TPU) (NPU) (VPU) (VPUX) (NPUX) (SPU) (ORT) );

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(filter, Filter, "Filter to possibly only show as options in "
                                             "the 'pipe' parameter some class of models from the zoo",
                                             Filter::All, Filter_Values, ParamCateg);

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(pipe, std::string, "Pipeline to use, which should correspond to a "
                                             "top-level entry in the zoo file. Network execution types are: OpenCV: "
                                             "on CPU, NPU: on JeVois NPU accelerator, TPU: on Coral Edge TPU if "
                                             "available, VPU: on MyriadX VPU if available, NPUX: on NPU via TimVX "
                                             "OpenCV extension, VPUX: on CPU via ARM-Compute OpenVino emulation "
                                             "of VPU, and SPU: on Hailo8 SPU if available.",
                                             "", ParamCateg);
      
      //! Enum \relates jevois::Pipeline
      JEVOIS_DEFINE_ENUM_CLASS(PreProc, (Blob) (Python) (Custom) );

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(preproc, PreProc, "Pre-Processor to use, usually set automatically "
                                             "by selecting a pipeline from the zoo file",
                                             PreProc::Blob, PreProc_Values, ParamCateg);
      //! Enum \relates jevois::Pipeline
#ifdef JEVOIS_PRO
      JEVOIS_DEFINE_ENUM_CLASS(NetType, (OpenCV) (ORT) (NPU) (TPU) (SPU) (Python) (Custom) );
#else
      JEVOIS_DEFINE_ENUM_CLASS(NetType, (OpenCV) (Python) (Custom) );
#endif

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(nettype, NetType, "Network runtime framework to use, usually set "
                                             "automatically by selecting a pipeline from the zoo file",
                                             NetType::OpenCV, NetType_Values, ParamCateg);
      //! Enum \relates jevois::Pipeline
      JEVOIS_DEFINE_ENUM_CLASS(PostProc, (Classify) (Detect) (Segment) (YuNet) (Python) (Stub) (Custom) );

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(postproc, PostProc, "Post-Processor to use, usually set automatically "
                                             "by selecting a pipeline from the zoo file",
                                             PostProc::Classify, PostProc_Values, ParamCateg);
      //! Enum \relates jevois::Pipeline
      JEVOIS_DEFINE_ENUM_CLASS(Processing, (Sync) (Async) );

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER(processing, Processing, "Type of processing: Sync runs pre-processing, "
                               "network, and post-processing sequentially for every frame. Use for fast "
                               "networks only, otherwise it will slow down the GUI... Async runs the network in "
                               "a thread and should be used for networks slower than the camera framerate.",
                               Processing::Async, Processing_Values, ParamCateg);
      
      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER(overlay, bool, "Show some pipeline info as an overlay over output or GUI video",
                               true, ParamCateg);

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER(paramwarn, bool, "Issue warning if parameters are specified in the zoo file for "
                               "a pipeline, but the pipeline's component is not using them. Warnings appear "
                               "in the console when the pipeline is loaded.",
                               true, ParamCateg);

      //! Parameter \relates jevois::Pipeline
      JEVOIS_DECLARE_PARAMETER(statsfile, std::string, "Appends pre/net/post timing statistics in HTML table format "
                               "to the specified file if not empty. If path is relative, it is to " JEVOIS_SHARE_PATH,
                               "", ParamCateg);
    }
    
    //! Neural processing pipeline
    /*! A neural processing pipeline in JeVois consists of:

        - pre-processing an input image to extract one or more blobs used as input tensors to a deep neural network
        - processing the input blobs through a deep network to obtain output blobs
        - post-processing the output blobs to display results and send serial messages

        A pipeline is typically configured by parsing a YAML config file (zoo file) that determines what kind of
        pre-processing, network, and post-processing to use, and that sets the parameters for those.  \ingroup dnn */
    class Pipeline : public jevois::Component,
                     public jevois::Parameter<pipeline::zooroot, pipeline::zoo, pipeline::filter, pipeline::pipe,
                                              pipeline::processing, pipeline::preproc, pipeline::nettype,
                                              pipeline::postproc, pipeline::overlay, pipeline::paramwarn,
                                              pipeline::statsfile>
    {
      public:
        //! Constructor
        Pipeline(std::string const & instance);

        //! Destructor
        virtual ~Pipeline();

        //! Returns true when all three of preproc, net, and postproc are ready
        bool ready() const;
        
        //! Process an input image, send results to serial/image/gui
        /*! If the network is not ready, no processing will occur. When helper is not null (i.e., using GUI display),
            hide the information window when idle is true. This function catches all exceptions and reports them. */
        void process(jevois::RawImage const & inimg, jevois::StdModule * mod,
                     jevois::RawImage * outimg, jevois::OptGUIhelper * helper, bool idle = false);
          
        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit);

        //! Set a custom pre-processor
        /*! To create a custom pre-processor, create a new class that derives from jevois::dnn::PreProcessor and that
            implements the pure virtual functions of that base class. Then, in your module, you should hold a
            shared_ptr<jevois::Pipeline> which we will call itsPipeline, and in your constructor you instantiate it and
            add it as a sub-component of your module (see constructor of \jvmod{DNN} for an example). Then proceed as
            follows in your constructor or postInit() or parameter callback:

            \code
            // Create an instance of our custom pre-processor and attach it as a sub-component of our pipeline
            auto sptr = itsPipeline->addSubComponent<MyCustomPreProc>("preproc");
            itsPipeline->setCustomPreProcessor(std::static_pointer_cast<jevois::dnn::PreProcessor>(sptr));
            \endcode

            And you are ready to rock. */
        void setCustomPreProcessor(std::shared_ptr<PreProcessor> pp);

        // Set a custom network type
        /*! See doc of setCustomPreProcessor() for instructions, but use the name "network" instead of "preproc". */
        void setCustomNetwork(std::shared_ptr<Network> n);

        // Set a custom post-processor
        /*! See doc of setCustomPreProcessor() for instructions, but use the name "postproc" instead of "preproc". */
        void setCustomPostProcessor(std::shared_ptr<PostProcessor> pp);
        
      protected:
        void postInit() override;
        void preUninit() override;
        std::shared_ptr<PreProcessor> itsPreProcessor;
        std::shared_ptr<Network> itsNetwork;
        std::shared_ptr<PostProcessor> itsPostProcessor;

      private:
        jevois::TimerOne itsTpre, itsTnet, itsTpost;
        
        bool itsZooChanged = false;
        void onParamChange(pipeline::zooroot const & param, std::string const & val) override;
        void onParamChange(pipeline::zoo const & param, std::string const & val) override;
        void onParamChange(pipeline::filter const & param, pipeline::Filter const & val) override;
        void onParamChange(pipeline::pipe const & param, std::string const & val) override;
        void onParamChange(pipeline::nettype const & param, pipeline::NetType const & val) override;
        void onParamChange(pipeline::preproc const & param, pipeline::PreProc const & val) override;
        void onParamChange(pipeline::postproc const & param, pipeline::PostProc const & val) override;

        std::future<std::vector<cv::Mat>> itsNetFut;
        std::array<std::string, 3> itsProcTimes { "PreProc: -", "Network: -", "PstProc: -" };
        std::array<double, 3> itsProcSecs { 0.0, 0.0, 0.0 };
        std::vector<cv::Mat> itsBlobs, itsOuts;
        std::vector<vsi_nn_tensor_attr_t> itsInputAttrs;
        std::vector<std::string> itsNetInfo, itsAsyncNetInfo;
        std::string itsAsyncNetworkTime = "Network: -";
        double itsAsyncNetworkSecs = 0.0;
        void showInfo(std::vector<std::string> const & info, jevois::StdModule * mod,
                      jevois::RawImage * outimg, jevois::OptGUIhelper * helper, bool ovl, bool idle);
        void asyncNetWait();
        bool checkAsyncNetComplete();
        double itsSecsSum = 0.0, itsSecsAvg = 0.0;
        int itsSecsSumNum = 0;
        bool itsPipeThrew = false;
        void scanZoo(std::filesystem::path const & zoofile, std::string const & filt, std::vector<std::string> & pipes,
                     std::string const & indent);
        bool selectPipe(std::string const & zoofile, std::vector<std::string> const & tok);
        void setZooParam(std::string const & name, std::string const & value, std::string const & zf,
                         cv::FileNode const & node);
        int itsOutImgY = 0;

        std::map<std::string, size_t> itsAccelerators;
        std::vector<double> itsPreStats, itsNetStats, itsPstStats;
        bool itsStatsWarmup = true;
    };

  } // namespace dnn
} // namespace jevois
