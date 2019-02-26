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

#include <vector>
#include <string>

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
      //! Construct from a raw data reading
      IMUdata(IMUrawData const & rd, double arange, double grange);
      
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

  //! \defgroup icmdmpheader ICM-20948 DMP computation header fields \ingroup imu
  /*! @{ */
#define JEVOIS_DMP_ACCEL           0x8000 /*!< calibrated accel if accel calibrated, raw accel otherwise */
#define JEVOIS_DMP_GYRO            0x4000 /*!< raw gyro */
#define JEVOIS_DMP_CPASS           0x2000 /*!< raw magnetic */
#define JEVOIS_DMP_ALS             0x1000 /*!< ALS/proximity */
#define JEVOIS_DMP_QUAT6           0x0800 /*!< game rotation vector */
#define JEVOIS_DMP_QUAT9           0x0400 /*!< rotation vector with heading accuracy */
#define JEVOIS_DMP_PQUAT6          0x0200 /*!< truncated game rotation vector for batching */
#define JEVOIS_DMP_GEOMAG          0x0100 /*!< geomag rotation vector with heading accuracy */
#define JEVOIS_DMP_PRESSURE        0x0080 /*!< pressure */
#define JEVOIS_DMP_GYRO_CALIBR     0x0040 /*!< calibrated gyro */
#define JEVOIS_DMP_CPASS_CALIBR    0x0020 /*!< calibrated magnetic */
#define JEVOIS_DMP_PED_STEPDET     0x0010 /*!< timestamp when each step is detected */
#define JEVOIS_DMP_HEADER2         0x0008 /*!< enable/disable data output in data output control register 2 */
#define JEVOIS_DMP_PED_STEPIND     0x0007 /*!< number of steps detected is in 3 LSBs of header (JEVOIS: always 0...) */

#define JEVOIS_DMP_ACCEL_ACCURACY  0x4000 /*!< accel accuracy when changes (HEADER2) */
#define JEVOIS_DMP_GYRO_ACCURACY   0x2000 /*!< gyro accuracy when changes (HEADER2) */
#define JEVOIS_DMP_CPASS_ACCURACY  0x1000 /*!< compass accuracy when changes (HEADER2) */
#define JEVOIS_DMP_FLIP_PICKUP     0x0400 /*!< Flip/pick-up gesture detector (HEADER2) */
#define JEVOIS_DMP_BATCH_MODE_EN   0x0100 /*!< enable batching (HEADER2) */
#define JEVOIS_DMP_ACT_RECOG       0x0080 /*!< Activity recognition engine (HEADER2)*/
      /*! @} */

  // motion event control reg
#define JEVOIS_DMP_BAC_WEARABLE_EN          0x8000
#define JEVOIS_DMP_PEDOMETER_EN             0x4000
#define JEVOIS_DMP_PEDOMETER_INT_EN         0x2000
#define JEVOIS_DMP_SMD_EN                   0x0800
#define JEVOIS_DMP_BTS_EN                   0x0020
#define JEVOIS_DMP_FLIP_PICKUP_EN           0x0010
#define JEVOIS_DMP_GEOMAG_EN                0x0008
#define JEVOIS_DMP_ACCEL_CAL_EN             0x0200
#define JEVOIS_DMP_GYRO_CAL_EN              0x0100
#define JEVOIS_DMP_COMPASS_CAL_EN           0x0080
#define JEVOIS_DMP_NINE_AXIS_EN             0x0040
#define JEVOIS_DMP_BRING_AND_LOOK_T0_SEE_EN 0x0004

  //! DMP data (Digital Motion Processor)
  /*! The digital motion processor (DMP) inside the ICM-20948 IMU chip can compute a variety of things that range from
      simply the raw data buffered at a fixed rate to quaternions to activity detection (footsteps, driving, biking,
      etc). The chip must be operating in DMP mode for this data to be available. In addition, one can configure which
      data is computed and output by the DMP. \ingroup imu */
  struct DMPdata
  {
      //! Populate our fields from a packet received from the DMP
      void parsePacket(unsigned char const * packet, size_t siz);

      unsigned short header1; //!< Header 1 fields that indicate what data is valid
      unsigned short header2; //!< Header 2 fields that indicate what data is valid

      short accel[3]; //!< Raw accelerometer data (when JEVOIS_DMP_ACCEL in header1)
      short gyro[3]; //!< Raw gyro data (when JEVOIS_DMP_GYRO in header1)
      short cpass[3]; //!< Raw compass data (when JEVOIS_DMP_CPASS in header1)
      short gbias[3]; //!< Raw gyro bias data (when JEVOIS_DMP_GYRO in header1)

      long quat6[3]; //!< Quaternion6 data (when JEVOIS_DMP_QUAT6 in header1)
      long quat9[3]; //!< Quaternion9 data (when JEVOIS_DMP_QUAT9 in header1)
      long quat9acc; //!< Quaternion9 accuracy (when JEVOIS_DMP_QUAT9 in header1)

      unsigned long stepts; //!< Step detection timestamp (when JEVOIS_DMP_PED_STEPDET)
      unsigned short steps; //!< Number of steps (0..7) detected this cycle (when JEVOIS_DMP_PED_STEPDET)
      
      long geomag[3]; //!< Geomag data (when JEVOIS_DMP_GEOMAG in header1)
      long geomagacc; //!< Geomag accuracy (when JEVOIS_DMP_GEOMAG in header1)
      
      short gyrobias[3]; //!< Gyro bias/calibration data (when JEVOIS_DMP_GYRO_CALIBR in header1)
      long cpasscal[3]; //!< Compass calibration data (when JEVOIS_DMP_CPASS_CALIBR in header1)

      short accelacc; //!< Accelerometer accuracy data (when JEVOIS_DMP_ACCEL_ACCURACY in header2)
      short gyroacc; //!< Gyro accuracy data (when JEVOIS_DMP_GYRO_ACCURACY in header2)
      short cpassacc; //!< Compass accuracy data (when JEVOIS_DMP_CPASS_ACCURACY in header2)

      short pickup; //!< Flip/pickup detection (when JEVOIS_DMP_FLIP_PICKUP in header2)

      unsigned short bacstate; //!< Activity recognition state (when JEVOIS_DMP_ACT_RECOG in header2)
      long bacts; //!< Activity recognition timestamp (when JEVOIS_DMP_ACT_RECOG in header2)

      unsigned short odrcnt; //!< Output data rate counter (always here but unclear what it is)

      //!< Decode start/stop of activities into a string
      /*! Requires that JEVOIS_DMP_ACT_RECOG be set in header2 and correct data rates are used. This function returns
          strings that contain start or stop as a prefix, then each activity. Activities are: drive, walk, run, bike,
          tilt, still. */
      std::vector<std::string> activity();

      //!< Decode current ongoing activities into a string
      /*! Requires that JEVOIS_DMP_ACT_RECOG be set in header2 and correct data rates are used. This function returns
          strings that contain the name of each ongoing event. Warning, it has state, so you should only call this
          function for any activity-related parsing after you start using it, and you should call it for every reading
          from the DMP. Activities are: drive, walk, run, bike, tilt, still. */
      std::vector<std::string> activity2();

      //! Convert a long fixed-point value to float
      static float fix2float(long val);
};

  //! Helper function to determine DMP packet size depending on options
  /*! \ingroup imu */
  size_t DMPpacketSize(unsigned short ctl1, unsigned short ctl2);
};
