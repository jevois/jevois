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

#include <jevois/Core/CameraSensor.H>
#include <opencv2/core.hpp>

namespace jevois
{
  //! Helper class for camera calibration, which allows some modules to compute 3D locations of objects
  /*! This code heavily derived from calibrate_camera.cpp in the OpenCV source tree. Keep it updated as OpenCV is
      updated. \ingroup core */
  class CameraCalibration
  {
    public:
      jevois::CameraSensor sensor = jevois::CameraSensor::any; //! Camera sensor
      jevois::CameraLens lens = jevois::CameraLens::standard;  //! Camera lens
      int w = 0, h = 0;                                        //! Image width and height (camera resolution)
      cv::Mat camMatrix = cv::Mat::eye(3, 3, CV_64F);          //! 3x3 camera matrix
      cv::Mat distCoeffs = cv::Mat::zeros(5, 1, CV_64F);       //! 5x1 distortion coefficients
      double avgReprojErr = 0.0;                               //! Average reprojection error

      //! Load from file
      /*! Throws std::runtime_error if file not found */
      void load(std::string const & fname);

      //! Save to file
      /*! Throws std::runtime_error if file cannot be written */
      void save(std::string const & fname) const;
  };
  
} // namespace jevois
