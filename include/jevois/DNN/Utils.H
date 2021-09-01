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

namespace jevois
{
  namespace dnn
  {
    /*! \defgroup dnn Tensor/Neural Processing networks

        Classes and utilities to provide abstraction to deep neural networks. Provides interfacing to OpenCV backends
        (CPU, OpenCL), tensor processing units (TPU) such as Coral Edge TPU and neural processing units (NPU) such as
        Amlogic A311D NPU. Supported on JeVois-Pro only.
        
        \ingroup core */
    
    /*! @{ */ // **********************************************************************
    
    //! Read a label file
    /*! Two formats are allowed: one class name per line, or one class number followed by one class name per file.*/
    std::map<int, std::string> readLabelsFile(std::string const & fname);

    //! Get a label from an id
    /*! If no entry is found in the map, return the id as a string. */
    std::string getLabel(std::map<int, std::string> const & labels, int id);

    //! Compute a color from a label name
    int stringToRGBA(std::string const & label, unsigned char alpha = 128);

    //! Get top-k entries and their indices
    void topK(float const * pfProb, float * pfMaxProb, uint32_t * pMaxClass, uint32_t outputCount, uint32_t topNum);

    //! Get a string of the form: "nD AxBxC... TYPE" from an n-dimensional cv::Mat with data type TYPE
    std::string shapestr(cv::Mat const & m);

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

    //! Parse tensor specification
    /*! If the specification is empty, an empty vector is returned. Throws std::range_error on any parsing error. */
    std::vector<vsi_nn_tensor_attr_t> parseTensorSpecs(std::string const & specs);

    //! Get a tensor's size in cv::Size format
    cv::Size attrsize(vsi_nn_tensor_attr_t const & attr);

    //! Get a string describing the specs of a tensor
    std::string attrstr(vsi_nn_tensor_attr_t const & attr);
    
    //! Get tensor shape and type attributes for a TensorFlow Lite tensor
    vsi_nn_tensor_attr_t tensorattr(TfLiteTensor const * t);

    //! Apply softmax to a float vector
    void softmax(float const * input, size_t n, float fac, float * output);
    
    /*! @} */ // **********************************************************************

  } // namespace dnn
} // namespace jevois
