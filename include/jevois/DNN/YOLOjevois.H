// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2024 by Laurent Itti, the University of Southern
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

#include <jevois/Component/Component.H>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <ovxlib/vsi_nn_pub.h> // for data types and quantization types

namespace jevois
{
  class GUIhelper;
  
  namespace dnn
  {
    class CLIP;
    class Network;
    
    namespace yolojevois
    {
      static jevois::ParameterCategory const ParamCateg("YOLOjevois Model Options");

      JEVOIS_DECLARE_PARAMETER(clipmodel, std::string, "Path to ggml CLIP model to use, in gguf "
                               "format. If path is relative, it is within " JEVOIS_SHARE_PATH "/clip/",
                               "clip-vit-base-patch32_ggml-model-q8_0.gguf", ParamCateg);
      
      JEVOIS_DECLARE_PARAMETER(textmodel, std::string, "Path ONNX YOLO-JeVois model to use "
                               "to process CLIP text embeddings, or empty to just use the raw "
                               "CLIP embeddings. If path is relative, it "
                               "is within " JEVOIS_SHARE_PATH "/ort/detection/",
                               "", ParamCateg);
    }

    //! Helper class for runtime-configurable, quantized open-vocabulary object detection
    /*! YOLO-JeVois splits the YOLO-World model into 3 components to allow runtime changes of class definitions on a
        running quantized model: 1) a CLIP model to convert text or image class definitions into 512D embeddings; 2) A
        YOLOjevois helper that runs an ONNX model on CPU when classes are updated, to convert the 512D CLIP embeddings
        into 5 tensors that will be used by the quantized object detection model; 3) a trimmed YOLO-World model (usually
        quantized for NPU) that takes an image plus those 5 tensors as an input, to generate detection boxes. In
        addition, a second variant is available, which is a bit slower, where: 1) class names or images are converted to
        CLIP embeddings; 2) these are input along with an image to a full YOLO-World model. This approach is slower and
        only works well on NPU when using 16-bit quantization.  \ingroup dnn */
    class YOLOjevois : public Component,
                       public Parameter<yolojevois::clipmodel, yolojevois::textmodel>
    {
      public:
        //! Inherited constructor ok; must call setup() before using
        YOLOjevois(std::string const & instance, std::map<int, std::string> & labels);
        
        /*! Initialize for nclass object classes. All labels in the label map will be processed upon load(), which is
            triggered by ready(), giving rise to embeddings; or, if the label map is missing some class labels, default
            ones will be created. Note that YOLOjevois may modify some of the labels during load() and update(). */
        void setup(size_t nclass, GUIhelper * helper, std::shared_ptr<jevois::dnn::Network> net);

        //! Virtual destructor
        virtual ~YOLOjevois();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit);

        //! Are we ready to work, or still loading our networks?
        bool ready();

        //! Get CLIP text embedding size, or 0 if we do not have a CLIP text encoder
        int textEmbeddingSize();

        //! Get CLIP image embedding size, or 0 if we do not have a CLIP image encoder
        int imageEmbeddingSize();

        //! Update one class using text
        void update(size_t const classnum, std::string const & label);

        //! Update one class using an RGB image
        void update(size_t const classnum, cv::Mat const & img);

        //! Access our class definition images
        /*! Returned vector always has one cv::Mat per class, but that Mat may be empty if class was not updated by
            image. Caution not thread-safe. */
        cv::Mat const & image(size_t const classid) const;

      protected:
        std::map<int, std::string> & itsLabels;
        size_t itsNumClasses = 0;
        cv::Mat itsEmbeddings; // text/image embeddings as 1xCx512 for C classes
        std::vector<cv::Mat> itsCLIPimages;
        std::shared_ptr<jevois::dnn::Network> itsNetwork; //!< Pointer to the main YOLO to update its extra inputs
        std::shared_ptr<CLIP> itsCLIP; //!< CLIP network to get embeddings from text or image queries
        std::shared_ptr<Network> itsAuxNet; //!< Optional aux network to process CLIP embeddings

        void load(); //!< Load CLIP and ONNX networks, in a non-blocking thread

      private:
        void updateMainNetwork();
        std::atomic<bool> itsLoading = false;
        std::atomic<bool> itsLoaded = false;
        std::future<void> itsLoadFut;
        jevois::GUIhelper * itsHelper = nullptr;
    };
  }
}

#endif // JEVOIS_PRO
