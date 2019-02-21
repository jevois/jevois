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

namespace jevois
{
  /*! \defgroup imu Inertial Measurement Unit (IMU) classes and functions
      \ingroup core

      These classes and functions provide support for optional IMU chips that may be present in some of the optional
      JeVois imaging sensors. */

  //! Raw IMU data
  /*! These are raw measurements from the sensor. Note that they are unscaled and thus their meaning depends on the
      selected scales for the sensors (e.g., +/-2g or +/-16g for the accelerometer). Use IMUdata for scaled
      measurements.  \ingroup imu */
  struct IMUrawData
  {
      short v[11]; //!< The values: ax, ay, az, gy, gy, gz, temp, mx, my, mz, mst2

      inline short & ax() { return v[0]; }
      inline short const & ax() const { return v[0]; }
      inline short & ay() { return v[1]; }
      inline short const & ay() const { return v[1]; }
      inline short & az() { return v[2]; }
      inline short const & az() const { return v[2]; }
      inline short & gx() { return v[3]; }
      inline short const & gx() const { return v[3]; }
      inline short & gy() { return v[4]; }
      inline short const & gy() const { return v[4]; }
      inline short & gz() { return v[5]; }
      inline short const & gz() const { return v[5]; }
      inline short & temp() { return v[6]; }
      inline short const & temp() const { return v[6]; }
      inline short & mx() { return v[7]; }
      inline short const & mx() const { return v[7]; }
      inline short & my() { return v[8]; }
      inline short const & my() const { return v[8]; }
      inline short & mz() { return v[9]; }
      inline short const & mz() const { return v[9]; }
      inline short & mst2() { return v[10]; }
      inline short const & mst2() const { return v[10]; }
  };
  
  //! IMU data
  /*! These are scaled measurements from the sensor, in absolute units (g for accelerations, degrees per second (dps)
      for gyro, degrees Celsius for temperature, and micro-Tesla for magnetometer).  \ingroup imu */
  struct IMUdata
  {
      float v[10]; //!< The values: ax, ay, az, gy, gy, gz, temp, mx, my, mz
      bool magovf; //!< True if magnetometer overflow
      
      inline float & ax() { return v[0]; }
      inline float const & ax() const { return v[0]; }
      inline float & ay() { return v[1]; }
      inline float const & ay() const { return v[1]; }
      inline float & az() { return v[2]; }
      inline float const & az() const { return v[2]; }
      inline float & gx() { return v[3]; }
      inline float const & gx() const { return v[3]; }
      inline float & gy() { return v[4]; }
      inline float const & gy() const { return v[4]; }
      inline float & gz() { return v[5]; }
      inline float const & gz() const { return v[5]; }
      inline float & temp() { return v[6]; }
      inline float const & temp() const { return v[6]; }
      inline float & mx() { return v[7]; }
      inline float const & mx() const { return v[7]; }
      inline float & my() { return v[8]; }
      inline float const & my() const { return v[8]; }
      inline float & mz() { return v[9]; }
      inline float const & mz() const { return v[9]; }
  };

  //! \defgroup icmdmpheader ICM-20948 DMP computation header fields
  /*! @{ */
#define JEVOIS_DMP_ACCEL          0x8000 /*!< calibrated accel if accel calibrated, raw accel otherwise */
#define JEVOIS_DMP_GYRO           0x4000 /*!< raw gyro */
#define JEVOIS_DMP_CPASS          0x2000 /*!< raw magnetic */
#define JEVOIS_DMP_ALS            0x1000 /*!< ALS/proximity */
#define JEVOIS_DMP_QUAT6          0x0800 /*!< game rotation vector */
#define JEVOIS_DMP_QUAT9          0x0400 /*!< rotation vector with heading accuracy */
#define JEVOIS_DMP_PQUAT6         0x0200 /*!< truncated game rotation vector for batching */
#define JEVOIS_DMP_GEOMAG         0x0100 /*!< geomag rotation vector with heading accuracy */
#define JEVOIS_DMP_PRESSURE       0x0080 /*!< pressure */
#define JEVOIS_DMP_GYRO_CALIBR    0x0040 /*!< calibrated gyro */
#define JEVOIS_DMP_CPASS_CALIBR   0x0020 /*!< calibrated magnetic */
#define JEVOIS_DMP_PED_STEPDET    0x0010 /*!< timestamp when each step is detected */
#define JEVOIS_DMP_HEADER2        0x0008 /*!< enable/disable data output in data output control register 2 */
#define JEVOIS_DMP_PED_STEPIND    0x0007 /*!< number of steps detected is in 3 LSBs of header */

#define JEVOIS_DMP_ACCEL_ACCURACY 0x4000 /*!< accel accuracy when changes (HEADER2) */
#define JEVOIS_DMP_GYRO_ACCURACY  0x2000 /*!< gyro accuracy when changes (HEADER2) */
#define JEVOIS_DMP_CPASS_ACCURACY 0x1000 /*!< compass accuracy when changes (HEADER2) */
#define JEVOIS_DMP_BATCH_MODE_EN  0x0100 /*!< enable batching  (HEADER2) */
      /*! @} */

  //! DMP data (Digital Motion Processor)
  /*! The digital motion processor (DMP) inside the ICM-20948 IMU chip can compute a variety of things that range from
      simply the raw data buffered at a fixed rate to quaternions to activity detection (footsteps, driving, biking,
      etc). The chip must be operating in DMP mode for this data to be available. In addition, one can configure which
      data is computed and output by the DMP. \ingroup imu */
  struct DMPdata
  {
      unsigned short contents = 0; //!< Indicates which contents are populated in this packet \see icmdmpheader
      
  };

  //! Helper function to determine DMP packet size depending on options
  /*! \ingroup imu */
  size_t DMPpacketSize(unsigned short ctl1, unsigned short ctl2) { return 8; }
};
