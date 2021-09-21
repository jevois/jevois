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

#include <cstddef> // for size_t

namespace jevois
{
  /*! \defgroup imu Inertial Measurement Unit (IMU)

      Interfacing to an Inertial Measurement Unit (IMU), TDK InvenSense ICM20948: 3-axis accelerometer, 3-axis
      gyroscope, 3-axis magnetometer, integrated digital motion processor (DMP).

      \ingroup core */
  
  //! Abstract interface to an ICM20948 inertial measurement unit (IMU)
  /*! This class abstracts the low-level communication interface (over SPI bus on JeVois-Pro hardware, or over I2C bus
      shared with the camera sensor chip and through custom ioctl() calls on JeVois-A33.  \ingroup imu */
  class IMU
  {
    public:
      
      //! Constructor
      IMU();

      //! Destructor
      virtual ~IMU();

      //! Returns true if we use SPI for transfers. Used when ICM20948_REG_USER_CTRL is written to
      virtual bool isSPI() const = 0;
      
      //! @name Access functions for IMU registers
      //! @{

      //! Write a value to one of the IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Bank selection is included in this function. */
      virtual void writeRegister(unsigned short reg, unsigned char val) = 0;

      //! Read a value from one of the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Bank selection is included in this function. */
      virtual unsigned char readRegister(unsigned short reg) = 0;

      //! Write an array of values to the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. Bank selection is included in this function. */
      virtual void writeRegisterArray(unsigned short reg, unsigned char const * vals, size_t num) = 0;

      //! Read an array of values from the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. Bank selection is included in this function. */
      virtual void readRegisterArray(unsigned short reg, unsigned char * vals, size_t num) = 0;

      //! @}

      //! @name Access functions for DMP registers
      //! @{

      //! Load the DMP firmware
      /*! On JeVois-A33, the firmware is already loaded by the camera kernel module. So this is only useful on
          JeVois-Pro to load the firmware over SPI. If verify is true, we will read it back and check. If errthrow is
          also true, throws an exception if verification fails. */
      void loadDMPfirmware(bool verify = false, bool errthrow = false);
      
      //! Write a value to a DMP register
      /*! This very low-level access is for development of optimal DMP settings only and should not be used in normal
          operation, it can crash your system. Note that the value will be converted to big endian, as required by the
          DMP processor. */
      void writeDMPregister(unsigned short reg, unsigned short val);

      //! Read a value from a DMP register
      /*! This very low-level access is for development of optimal DMP settings only and should not be used in normal
          operation, it can crash your system. Note that the value will be converted from big endian of DMP processor to
          little endian of the JeVois processor.*/
      unsigned short readDMPregister(unsigned short reg);

      //! Write an array of values to DMP registers
      /*! This very low-level access is for development of optimal DMP settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. This function does not handle crossing of memory banks, so the caller
          should segment the calls to match memory bank boundaries. Finally, this function does not perform any endian
          conversion. Callers should be aware that the DMP is a 16-bit big endian system while the JeVois processor is
          little endian. */
      void writeDMPregisterArray(unsigned short reg, unsigned char const * vals, size_t num);

      //! Read an array of values from DMP registers
      /*! This very low-level access is for development of optimal DMP settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. This function does not handle crossing of memory banks, so the caller
          should segment the calls to match memory bank boundaries. Finally, this function does not perform any endian
          conversion. Callers should be aware that the DMP is a 16-bit big endian system while the JeVois processor is
          little endian. */
      void readDMPregisterArray(unsigned short reg, unsigned char * vals, size_t num);
      
      //! @}
  };
} // namespace jevois
