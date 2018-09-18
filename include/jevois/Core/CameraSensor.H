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
  /*! \defgroup camerasensor Helper definitions for different camera sensors
      \ingroup core */

  /*! @{ */ // **********************************************************************

  //! Enum for different sensor models
  /*! Note that the value 'any' should not be used, it is only so that jevois-add-videomapping will always succeed. */
  JEVOIS_DEFINE_ENUM_CLASS(CameraSensor, (any) (ov9650) (ov2640) (ov7725) (ar0135) );

  //! Check whether a given resolution and frame rate is supported by a sensor
  /*! Returns true if the sensor is capable of capturing video at the given format, resolution, and frame rate.*/
  bool sensorSupportsFormat(CameraSensor s, unsigned int fmt, unsigned int w, unsigned int h, float fps);
  
  /*! @} */ // **********************************************************************
}
