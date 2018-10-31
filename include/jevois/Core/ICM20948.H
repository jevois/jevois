// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
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

// This code inspired by:

/***************************************************************************//**
 * file ICM20648.cpp
 *******************************************************************************
 * section License
 * <b>(C) Copyright 2017 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <jevois/Component/Component.H>
#include <jevois/Core/IMUdata.H>

namespace jevois
{
  namespace imu
  {
    static jevois::ParameterCategory const ParamCateg("ICM-20948 IMU Options");

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(arate, float, "Accelerometer sampling rate (Hz), or 0.0 to disable "
                                           "accelerometer. Actual sample rate may differ because of finite and "
                                           "discrete divider settings.",
                                           50.0F, jevois::Range(0.0F, 1125.0F), ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(grate, float, "Gyroscope sampling rate (Hz), or 0.0 to disable "
                                           "gyroscope. Actual sample rate may differ because of finite and "
                                           "discrete divider settings.",
                                           50.0F, jevois::Range(0.0F, 1125.0F), ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(mrate, float, "Magnetometer sampling rate (Hz), or 0.0 to disable "
                                           "magnetometer. Actual sample rate may differ because of finite and "
                                           "discrete divider settings.",
                                           50.0F, jevois::Range(0.0F, 1000.0F), ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(abw, unsigned int, "Accelerometer bandwidth rate (Hz), or 0 for no "
                                           "low-pass filter on accelerometer data.",
                                           0, { 0, 6, 12, 24, 50, 111, 246, 470, 1210 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(gbw, unsigned int, "Gyroscope bandwidth rate (Hz), or 0 for no low-pass "
                                           "filter on gyroscope data.",
                                           0, { 0, 6, 12, 24, 51, 120, 150, 200, 360, 12100 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(tbw, unsigned int, "Temperature bandwidth rate (Hz), or 0 to turn off "
                                           "temperature sensor. Temperature sampling rate is always 1125Hz unless "
                                           "tbw is 7932, in which case sampling is at 9kHz.",
                                           34, { 0, 9, 17, 34, 66, 123, 218, 7932 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(arange, unsigned int, "Accelerometer full-scale range (+/-g; for example, "
                                           "2 means +/-2g)).",
                                           4, { 2, 4, 9, 16 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(grange, unsigned int, "Gyroscope full-scale range (+/-dps; for example, "
                                           "500 means +/-500 degrees per second)).",
                                           500, { 250, 500, 1000, 2000 }, ParamCateg);
  }
  
  //! TDK InvenSense IMC-20948 9DOF IMU high-level driver
  /*! This driver communicates with a kernel-side driver that is integrated with the camera sensor driver on some
      optional JeVois sensors only. Currently, only the OnSemi (Aptina) AR0135 global shutter optional camera sensor is
      equipped with an IMU. \ingroup core.

      CAUTION: This class has state and is not re-entrant. Namely, we store the current register bank of the IMU chip in
      a class member variable. This could get out of sync if several member functions of this class are called
      concurrently. Because we assume this will not be the case in normal operation, we do not protect chip access with
      a mutex. Users should use an external mutex to protect this class if used in a multithreaded manner. */
  class ICM20948 : public Component,
                   public Parameter<imu::arate, imu::grate, /*imu::mrate,*/ imu::abw, imu::gbw, imu::tbw,
                                    imu::arange, imu::grange>
  {
    public:
      //! Base class constructor ok
      using jevois::Component::Component;

      //! Virtual destructor for safe inheritance
      virtual ~ICM20948();

      //! Returns true if this camera indeed has a working ICM20948
      bool ready();

      //! Get one round of raw data
      IMUrawData getRaw();
      
      //! Get one round of scaled data
      IMUdata get();
      
    protected:
      //! Connect to and initialize the IMU chip
      void postInit();
      
      //! Write a value to one of the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Bank selection is included in this function. */
      void writeRegister(unsigned short reg, unsigned char val);

      //! Read a value from one of the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Bank selection is included in this function. */
      unsigned char readRegister(unsigned short reg);

      //! Write an array of values to the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. Bank selection is included in this function. */
      void writeRegisterArray(unsigned short reg, unsigned char const * vals, size_t num);

      //! Read an array of values from the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. Bank selection is included in this function. */
      void readRegisterArray(unsigned short reg, unsigned char * vals, size_t num);
 
      void selectBank(unsigned short reg); //!< Select a register bank (0..3) based on high bits of register address
      void reset(); //!< Reset the IMU chip - not recommended in normal operation
      void sleep(bool enable); //!< Turn on/off sleep mode
      void cycle(bool enable); //!< Turn on/off cycle mode vs continuous for accel, gyro and compass
      uint32_t devid(); //!< Read device ID

    private:
      // Parameter callbacks:
      void onParamChange(imu::arate const & param, float const & newval);
      void onParamChange(imu::grate const & param, float const & newval);
      //void onParamChange(imu::mrate const & param, float const & newval);
      void onParamChange(imu::abw const & param, unsigned int const & newval);
      void onParamChange(imu::gbw const & param, unsigned int const & newval);
      void onParamChange(imu::tbw const & param, unsigned int const & newval);
      void onParamChange(imu::arange const & param, unsigned int const & newval);
      void onParamChange(imu::grange const & param, unsigned int const & newval);
      //void onParamChange(imu::mrange const & param, unsigned int const & newval);

      uint8_t itsBank = 0;
  };
}
