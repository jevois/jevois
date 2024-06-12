// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2018 by Laurent Itti, the University of Southern
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

#include <jevois/Types/Enum.H>

namespace jevois
{
  class VideoMapping;
  class Camera;
  
  /*! \defgroup camerasensor Helper definitions for different camera sensors
      \ingroup core */

  /*! @{ */ // **********************************************************************

  //! Enum for different sensor models
  /*! Use 'any' when you either don't know (e.g., running in host mode with an unknown dumb USB webcam), or when you
      don't care (e.g., we use 'any' in jevois-add-videomapping so that it will always succeed).
      
      - Parallel CSI sensors are for JeVois-A33 hardware: ov9650, ov2640, ov7725, ar0135
      - MIPI sensors are for JeVois-Pro hardware: imx290, os08a10, ar0234

      Here, we also define a CameraLens enum that is used for camera calibration. */
#ifdef JEVOIS_PRO
  JEVOIS_DEFINE_ENUM_CLASS(CameraSensor, (any) (imx290) (os08a10) (ar0234) );
  JEVOIS_DEFINE_ENUM_CLASS(CameraLens, (standard) (custom1) (custom2) (custom3) (custom4) (custom5) );
#else
  JEVOIS_DEFINE_ENUM_CLASS(CameraSensor, (any) (ov9650) (ov2640) (ov7725) (ar0135) );
  JEVOIS_DEFINE_ENUM_CLASS(CameraLens, (standard) (noir) (wide90) (wide120)
                           (m12_18) (m12_25) (m12_36) (m12_6) (m12_12)
                           (custom1) (custom2) (custom3) (custom4) (custom5) );
#endif
  
  //! Check whether a given resolution and frame rate is supported by a sensor
  /*! Returns true if the sensor is capable of capturing video at the given format, resolution, and frame rate.*/
  bool sensorSupportsFormat(CameraSensor s, VideoMapping const & m);

  //! Check whether sensor has an IMU (inertial measurement unit)
  bool sensorHasIMU(CameraSensor s);

  //! Load a sensor preset (JeVois-Pro only) and return native sensor grab dims (when cropping and/or scaling)
  void sensorPrepareSetFormat(CameraSensor s, VideoMapping const & m, unsigned int & capw, unsigned int & caph,
                              int & preset);
  
  /*! @} */ // **********************************************************************
}
