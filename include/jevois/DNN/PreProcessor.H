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
#include <opencv2/core/core.hpp>
#include <jevois/Image/RawImage.H>
#include <jevois/GPU/GUIhelper.H>
#include <jevois/Core/Module.H>

#include <ovxlib/vsi_nn_pub.h> // for data types and quantization types

namespace jevois
{
  namespace dnn
  {
    class PreProcessorForPython;
    
    namespace preprocessor
    {
      // We define all parameters for all derived classes here to avoid duplicate definitions. Different derived classes
      // will use different subsets of all available parameters:
      static jevois::ParameterCategory const ParamCateg("DNN Pre-Processing Options");

      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(rgb, bool, "When true, model works with RGB input images instead BGR ones",
                               true, ParamCateg);
      
      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(scale, float, "Value scaling factor applied to input pixels after mean subtraction, "
                               "or 0.0 to extract an unscaled UINT8 blob, typically for use with quantized networks",
                               2.0F / 255.0F, ParamCateg);
      
      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(mean, cv::Scalar, "Mean values subtracted from input image, in the same RGB/BGR "
                               "order as the network's input",
                               cv::Scalar(127.5F, 127.5F, 127.5F), ParamCateg);

      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(stdev, cv::Scalar, "Input image is divided by stdev after mean subtraction and scale "
                               "factor are applied. This is rarely used. Same RGB/BGR order as the network's input",
                               cv::Scalar(1.0F, 1.0F, 1.0F), ParamCateg);

      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(letterbox, bool, "When true, extract the largest possible box from the input image "
                               "with same aspect ratio as the network's input tensor, and then rescale it to that "
                               "tensor's width and height (hence with cropping but no distortion). Otherwise, use "
                               "the whole image and rescale it to the network's input width and height with some "
                               "possible stretching.",
                               false, ParamCateg);
      
      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(showin, bool, "Show outline of cropped image fed to network",
                               true, ParamCateg);

      //! Parameter \relates jevois::dnn::PreProcessorPython
      JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(pypre, std::string, "Full path of the python pre-processor file. Name of "
                                             "class defined in the file must match the file name without the "
                                             "trailing '.py'",
                                             "", ParamCateg);

      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(details, bool, "Show more details about the pre-processing steps",
                               false, ParamCateg);

      //! Enum for image resizing modes \relates jevois::dnn::PreProcessor
      JEVOIS_DEFINE_ENUM_CLASS(InterpMode, (Nearest) (Linear) (Cubic) (Area) (Lanczos4) );

      //! Parameter \relates jevois::dnn::PreProcessor
      JEVOIS_DECLARE_PARAMETER(interp, InterpMode, "Image interpolation to use when resizing the input image "
                               "from camera to network input dims",
                               InterpMode::Nearest, InterpMode_Values, ParamCateg);

      //! Parameter \relates jevois::dnn::PreProcessorBlob
      JEVOIS_DECLARE_PARAMETER(numin, size_t, "Number of input blobs to generate from the received video image. "
                               "Any additional inputs required by the network would have to be specified using "
                               "Network parameter extraintensors",
                               1, ParamCateg);
    }

    //! Pre-Processor for neural network pipeline
    /*! This is the first step in a deep neural network processing Pipeline.

        Derived classes must implement the pure virtual methods:
        - process(): process an input image and generate some tensors (blobs)
        - report(): describe what process() did to humans
        - freeze(): freeze/unfreeze parameters that users should not change at runtime

        They should keep some internal state about what to report, since report() is always called on
        every frame, but process() may be called less often if the network is slow.
        
        \ingroup dnn */
    class PreProcessor : public jevois::Component,
                         public jevois::Parameter<preprocessor::rgb, preprocessor::showin, preprocessor::details>
    {
      public:
        
        //! Constructor
        PreProcessor(std::string const & instance);

        //! Destructor
        virtual ~PreProcessor();

        //! Freeze/unfreeze parameters that users should not change while running
        virtual void freeze(bool doit) = 0;

        //! Extract blobs from input image
        std::vector<cv::Mat> process(jevois::RawImage const & img, std::vector<vsi_nn_tensor_attr_t> const & attrs);

        //! Report what happened in last process() to console/output video/GUI
        virtual void sendreport(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                                jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false);

        //! Access the last processed image size
        cv::Size const & imagesize() const;
        
        //! Access the last computed blobs (or empty if process() has not yet been called)
        std::vector<cv::Mat> const & blobs() const;

        //! Access the width and height of a given blob, accounting for NCHW or NHWC
        cv::Size blobsize(size_t num) const;
        
        //! Convert coordinates from blob back to original image
        /*! Given coords x,y should be in [0..w-1]x[0..h-1] where w,h are the blob's width and height. This is useful to
            convert detected boxes back into original input coordinates. */
        void b2i(float & x, float & y, size_t blobnum = 0);

        //! Convert coordinates from blob back to original image, given a known blob size
        /*! Given coords x,y should be in [0..w-1]x[0..h-1] where w,h are the blob's width and height. This is useful to
            convert detected boxes back into original input coordinates. */
        void b2i(float & x, float & y, cv::Size const & bsiz, bool letterboxed);

        //! Get unscaled crop rectangle in image coordinates
        /*! This is useful to display an image overlay on top of the input image. */
        cv::Rect getUnscaledCropRect(size_t blobnum = 0);
        
        //! Get unscaled crop rectangle in image coordinates
        /*! This is useful to display an image overlay on top of the input image. */
        void getUnscaledCropRect(size_t blobnum, int & tlx, int & tly, int & brx, int & bry);

        //! Convert coordinates from image to blob
        /*! Given coords x,y should be in [0..w-1]x[0..h-1] where w,h are the image's width and height. This is useful
            to convert mouse coordinates (after they have been converted from screen to image coords) to locations
            within an input blob. */
        void i2b(float & x, float & y, size_t blobnum = 0);

        //! Convert coordinates from image to blob
        /*! Given coords x,y should be in [0..w-1]x[0..h-1] where w,h are the image's width and height. This is useful
            to convert mouse coordinates (after they have been converted from screen to image coords) to locations
            within an input blob. */
        void i2b(float & x, float & y, cv::Size const & bsiz, bool letterboxed);

        //! Get a pointer to our python-friendly interface
        std::shared_ptr<PreProcessorForPython> getPreProcForPy() const;
        
      protected:
        //! Extract blobs from input image
        /*! isrgb should be true if the given img has RGB color order, or false for BGR. Only 3-channel byte images are
            supported as input. */
        virtual std::vector<cv::Mat> process(cv::Mat const & img, bool isrgb,
                                             std::vector<vsi_nn_tensor_attr_t> const & attrs,
                                             std::vector<cv::Rect> & crops) = 0;

        //! Report what happened in last process() to console/output video/GUI
        virtual void report(jevois::StdModule * mod, jevois::RawImage * outimg = nullptr,
                            jevois::OptGUIhelper * helper = nullptr, bool overlay = true, bool idle = false) = 0;

      private:
        std::vector<vsi_nn_tensor_attr_t> itsAttrs;
        std::vector<cv::Mat> itsBlobs;
        std::vector<cv::Rect> itsCrops; // Unscaled crops, one per blob, used for rescaling from blob to image
        
        cv::Size itsImageSize;
        unsigned int itsImageFmt;

        // Helper class exposed to python
        std::shared_ptr<PreProcessorForPython> itsPP;
    };
    
  } // namespace dnn
} // namespace jevois
