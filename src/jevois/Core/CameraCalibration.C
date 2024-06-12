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

#include <jevois/Core/CameraCalibration.H>
#include <jevois/Util/Utils.H>
#include <ctime>

// ####################################################################################################
void jevois::CameraCalibration::load(std::string const & fname)
{
  cv::FileStorage fs(fname, cv::FileStorage::READ);
  if (fs.isOpened() == false) throw std::runtime_error("Failed to load [" + fname + ']');
  std::string str;
  fs["sensor"] >> str; sensor = jevois::from_string<jevois::CameraSensor>(str);
  fs["lens"] >> str; lens = jevois::from_string<jevois::CameraLens>(str);
  fs["image_width"] >> w;
  fs["image_height"] >> h;
  fs["camera_matrix"] >> camMatrix;
  fs["distortion_coefficients"] >> distCoeffs;
  fs["avg_reprojection_error"] >> avgReprojErr;
}

// ####################################################################################################
void jevois::CameraCalibration::save(std::string const & fname) const
{
  cv::FileStorage fs(fname, cv::FileStorage::WRITE);
  if (fs.isOpened() == false) throw std::runtime_error("Failed to save [" + fname + ']');

  time_t tm;
  time(&tm);
  struct tm *t2 = localtime(&tm);
  char buf[1024];
  strftime(buf, sizeof(buf), "%c", t2);
  
  fs << "calibration_time" << buf;
  fs << "sensor" << jevois::to_string(sensor);
  fs << "lens" << jevois::to_string(lens);
  fs << "image_width" << w;
  fs << "image_height" << h;
  fs << "camera_matrix" << camMatrix;
  fs << "distortion_coefficients" << distCoeffs;
  fs << "avg_reprojection_error" << avgReprojErr;
}
