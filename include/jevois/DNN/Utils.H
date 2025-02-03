// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

#include <map>
#include <string>
#include <opencv2/core/core.hpp>
#include <tensorflow/lite/c/common.h> // for TfLiteType
#include <ovxlib/vsi_nn_pub.h> // for data types and quantization types

#ifdef JEVOIS_PRO
#include <hailo/hailort.h>
#include <onnxruntime_cxx_api.h>
#endif

namespace jevois
{
  //! Deep neural networks
  namespace dnn
  {
    /*! \defgroup dnn Tensor/Neural Processing networks

        Classes and utilities to provide abstraction to deep neural networks. Provides interfacing to OpenCV backends
        (CPU, OpenCL), tensor processing units (TPU) such as Coral Edge TPU and neural processing units (NPU) such as
        Amlogic A311D NPU. */
    
    /*! @{ */ // **********************************************************************

    //! Get class labels from either a list or a file
    /*! If arg corresponds to a file that exists (either absolute path or relative to JEVOIS_SHARE_PATH), then load the
        classes from that file. Otherwise, set them from arg, which should then be a comma-separated list of values. */
    std::map<int, std::string> getClassLabels(std::string const & arg);
    
    //! Read a label file
    /*! Two formats are allowed: one class name per line, or one class number followed by one class name per file.*/
    std::map<int, std::string> readLabelsFile(std::string const & fname);

    //! Get a label from an id
    /*! If no entry is found in the map, return the id as a string (if namedonly is false) or an empty string (if
        namedonly is true). */
    std::string getLabel(std::map<int, std::string> const & labels, int id, bool namedonly = false);

    //! Compute a color from a label name
    int stringToRGBA(std::string const & label, unsigned char alpha = 128);

    //! Get top-k entries and their indices
    void topK(float const * pfProb, float * pfMaxProb, uint32_t * pMaxClass, uint32_t outputCount, uint32_t topNum);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional cv::Mat with data type TYPE
    std::string shapestr(cv::Mat const & m);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional size vector and OpenCV data type TYPE
    std::string shapestr(std::vector<size_t> dims, int typ);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional size vector and OpenCV data type TYPE
    std::string shapestr(std::vector<int> dims, int typ);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional TfLiteTensor with data type TYPE
    std::string shapestr(TfLiteTensor const * t);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional NPU tensor with data type TYPE
    std::string shapestr(vsi_nn_tensor_attr_t const & attr);

    //! Get a vector of size_t from a string containing AxBxC...
    std::vector<size_t> strshape(std::string const & str);

    //! Convert from TensorFlow data type to OpenCV
    int tf2cv(TfLiteType t);

    //! Convert from TensorFlow data type to vsi_nn
    vsi_nn_type_e tf2vsi(TfLiteType t);

    //! Convert from NPU data type to OpenCV
    int vsi2cv(vsi_nn_type_e t);

    //! Clamp a rectangle to within given image width and height
    void clamp(cv::Rect & r, int width, int height);

    //! Clamp a rectangle to within given image width and height
    void clamp(cv::Rect2f & r, float width, float height);

    //! Parse tensor specification
    /*! If the specification is empty, an empty vector is returned. Throws std::range_error on any parsing error. */
    std::vector<vsi_nn_tensor_attr_t> parseTensorSpecs(std::string const & specs);

    //! Construct a cv::Mat from attr and possibly data pointer
    /*! If dataptr is nullptr, new memory will be allocated for the cv::Mat. Caller must ensure data outlives the
        cv::Mat, and is responsible for eventually de-allocating the data. Usually, with non-null dataptr, this is only
        to be used as a temporary re-casting, e.g., to recast a received tensor into a Mat before dequantizing it, then
        forgetting about that Mat. */
    cv::Mat attrmat(vsi_nn_tensor_attr_t const & attr, void * dataptr = nullptr);

    //! Get a tensor dims as a vector of int, useful to construct a matching cv::Mat
    std::vector<int> attrdims(vsi_nn_tensor_attr_t const & attr);

    //! Get a tensor's (width, height) size in cv::Size format, skipping over other dimensions
    cv::Size attrsize(vsi_nn_tensor_attr_t const & attr);

    //! Get a string describing the specs of a tensor, including quantification specs (not provided by shapestr())
    std::string attrstr(vsi_nn_tensor_attr_t const & attr);

    //! Check that a cv::Mat blob matches exactly the spec of an attr
    bool attrmatch(vsi_nn_tensor_attr_t const & attr, cv::Mat const & blob);
    
    //! Get tensor shape and type attributes for a TensorFlow Lite tensor
    vsi_nn_tensor_attr_t tensorattr(TfLiteTensor const * t);

    //! Compute fast exponential using approximation formula
    /*! From https://github.com/Qengineering/YoloV8-ncnn-Raspberry-Pi-4/blob/main/yoloV8.cpp */
    float fastexp(float x);

    //! Compute sigmoid using fastexp
    float sigmoid(float x);
    
    //! Compute sigmoid using fastexp on every pixel of a Mat of type CV_32F, in-place
    void sigmoid(cv::Mat & m);
    
    //! Apply softmax to a float vector
    /*! n is the number of elements to process, stride is the increment in the arrays from one element to the next. So
        the arrays should have size n * stride. Returns the index in [0..n*stride[ of the highest scoring element. If
        maxonly is true, only output[returned index] is valid. */
    size_t softmax(float const * input, size_t const n, size_t const stride, float const fac, float * output,
                   bool maxonly);

    //! Compute softmax and return DFL distance
    /*! src should have size n * stride. Note: even if stride > 1, dst should always have size n */
    float softmax_dfl(float const * src, float * dst, size_t const n, size_t const stride = 1);

    //! Quantize from float32 to fixed-point according to the quantization spec in attr
    /*! m should be float32, typically normalized to [0..1[ or [-1..1[ already. attr is the desired quantized type and
        method (DFP, AA, etc) */
    cv::Mat quantize(cv::Mat const & m, vsi_nn_tensor_attr_t const & attr);

    //! Dequantize an output to float32 according to the quantization spec in attr
    /*! attr should have the type and quantization details of m, returned tensor is float32 */
    cv::Mat dequantize(cv::Mat const & m, vsi_nn_tensor_attr_t const & attr);

    //! Returns the number of non-unit dims in a cv::Mat
    /*! For example, returns 2 for a 4D Mat with size 1x1x224x224, since it effectively is a 224x224 2D array */
    size_t effectiveDims(cv::Mat const & m);

    //! Concatenate several tensors into one
    /*! Axis may be positive starting at 0 for the first dimension (when reading dims from left to right), or negative
        starting at -1 for the last dimension. For example, for a 10x20x30 tensor, axis 0 has size 10 and is also axis
        -3, axis 1 has size 20 and is also axis -2, and axis 2 has size 30 and is also axis -1. The input tensors must
        all have the same number of dimensions, same pixel type, and sizes must match for all dimensions except the one
        that is being concatenated. */
    cv::Mat concatenate(std::vector<cv::Mat> const & tensors, int axis);

    //! Split a tensor into several, along a given axis
    /*! The sum of all given sizes must equal the original size along the selected axis. */
    std::vector<cv::Mat> split(cv::Mat const & tensor, int axis, std::vector<int> const & sizes);
    
#ifdef JEVOIS_PRO
    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional Hailo tensor with data type TYPE
    std::string shapestr(hailo_vstream_info_t const & vi);

    //! Get tensor shape and type attributes for a Hailo tensor
    vsi_nn_tensor_attr_t tensorattr(hailo_vstream_info_t const & vi);

    //! Convert from Hailo data type to vsi_nn
    vsi_nn_type_e hailo2vsi(hailo_format_type_t t);

    //! Convert from ONNX-Runtime data type to vsi_nn
    vsi_nn_type_e onnx2vsi(ONNXTensorElementDataType t);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional ONNX tensor with data type TYPE
    std::string shapestr(Ort::ConstTensorTypeAndShapeInfo const & ti);

    //! Get tensor shape and type attributes for an ONNX-runtime tensor
    vsi_nn_tensor_attr_t tensorattr(Ort::ConstTensorTypeAndShapeInfo const & ti);
#endif
    
    /*! @} */ // **********************************************************************


    /*! \defgroup pydnn DNN-related processors written in python
  
    In addition to writing DNN pre/net/post processors in C++, JeVois supports writing them in Python.

    \ingroup dnn */


  } // namespace dnn
} // namespace jevois

// Inlcude inlined functions implementation
#include <jevois/DNN/details/UtilsImpl.H>
