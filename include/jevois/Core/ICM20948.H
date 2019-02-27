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
#include <jevois/Types/Enum.H>

namespace jevois
{
  namespace imu
  {
    static jevois::ParameterCategory const ParamCateg("ICM-20948 IMU Options");

    //! Enum for Parameter \relates jevois::ICM20948
    JEVOIS_DEFINE_ENUM_CLASS(Mode, (RAW) (FIFO) (DMP) )

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER(mode, Mode, "Data collection mode. RAW means that the latest available raw "
                             "data is returned each time get() is called, hence timing may not be very "
                             "accurate depending on how regularly get() is called. FIFO collects accel and gyro data "
                             "at the exact rates specified by parameters arate, grate into a 1kb FIFO queue, and "
                             "get() takes that data back from the FIFO. DMP runs the embedded "
                             "digital motion processor on raw data, and accumulates resulting output "
                             "data into the IMU's internal FIFO buffer at a precise, fixed rate. This "
                             "parameter can only be set in a module's params.cfg file.",
                             Mode::RAW, Mode_Values, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(dmp, std::string, "Requested DMP data. Only valid when mode is DMP. "
                                           "Use any string of letters including:"
                                           "A=acceleration, "
                                           "G=gyro, "
                                           "M=magnetometer (compass), "
                                           "R=quaternion 6 (uses accel + gyro), "
                                           "Q=quaternion 9 (uses accel + gyro + compass), "
                                           "E=geomag (uses accel + compass), "
                                           "P=flip/pickup detection, "
                                           "S=step detection, "
                                           "T=activity recognition, "
                                           "w=configure activity recognition for wearable, "
                                           "g=gyro calibration (always on when gyro used), "
                                           "m=compass calibration (always on when compass used), "
                                           "b=accel accuracy (always on when accel used), "
                                           "h=gyro accuracy (always on when gyro used), "
                                           "n=compass accuracy (always on when compass used).",
                                           "QSPT", ParamCateg);
    
    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER(pktdbg, bool, "Send raw FIFO or DMP packets to console for debug/hacking purposes.",
                             false, ParamCateg);
    
    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(arate, float, "Accelerometer sampling rate (Hz), or 0.0 to disable "
                                           "accelerometer. Actual sample rate may differ because of finite and "
                                           "discrete divider settings. In FIFO mode, grate controls the data rate.",
                                           30.0F, jevois::Range<float>(0.0F, 1125.0F), ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(grate, float, "Gyroscope sampling rate (Hz), or 0.0 to disable "
                                           "gyroscope. Actual sample rate may differ because of finite and "
                                           "discrete divider settings. In FIFO mode, grate controls the data rate.",
                                           30.0F, jevois::Range<float>(0.0F, 1125.0F), ParamCateg);

    //! Enum for Parameter \relates jevois::ICM20948
    JEVOIS_DEFINE_ENUM_CLASS(MagRate, (Off) (Once) (M10Hz) (M20Hz) (M50Hz) (M100Hz) )

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(mrate, MagRate, "Magnetometer sampling rate (Hz), or Off to disable "
                                           "magnetometer, or Once to only get one measurement. You can repeatedly "
                                           "set this parameter to Once to obtain repeated measurements "
                                           "at your own pace. In JeVois Inventor, you need to alternate between "
                                           "Off and Once. In FIFO mode, grate controls the data rate.",
                                           MagRate::M50Hz, MagRate_Values, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(abw, unsigned int, "Accelerometer bandwidth rate (Hz), or 0 for no "
                                           "low-pass filter on accelerometer data.",
                                           50, { 0, 6, 12, 24, 50, 111, 246, 470, 1210 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(gbw, unsigned int, "Gyroscope bandwidth rate (Hz), or 0 for no low-pass "
                                           "filter on gyroscope data.",
                                           51, { 0, 6, 12, 24, 51, 120, 150, 200, 360, 12100 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(tbw, unsigned int, "Temperature bandwidth rate (Hz), or 0 to turn off "
                                           "temperature sensor. Temperature sampling rate is always 1125Hz unless "
                                           "tbw is 7932, in which case sampling is at 9kHz.",
                                           34, { 0, 9, 17, 34, 66, 123, 218, 7932 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(arange, unsigned int, "Accelerometer full-scale range (+/-g; for example, "
                                           "2 means +/-2g)).",
                                           4, { 2, 4, 8, 16 }, ParamCateg);

    //! Parameter \relates jevois::ICM20948
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(grange, unsigned int, "Gyroscope full-scale range (+/-dps; for example, "
                                           "500 means +/-500 degrees per second)).",
                                           500, { 250, 500, 1000, 2000 }, ParamCateg);
  }
  
  //! TDK InvenSense ICM-20948 9DOF IMU high-level driver
  /*! This driver communicates with a kernel-side driver that is integrated with the camera sensor driver on some
      optional JeVois sensors only. Currently, only the OnSemi (Aptina) AR0135 global shutter optional camera sensor is
      equipped with an IMU.

      The specifications of this chip are quite impressive:
      - 3-axis 16-bit accelerometer with full-range sensitivity selectable to +/-2g, +/-4g, +/-8g, and +/-16g.
      - Accelerometer data rate from 4 Hz to 1125 Hz.
      - 3-axis 16-bit gyroscope with full-range sensitivity selectable to +/-250dps (degrees/s), +/-500dps,
        +/-1000dps, and +/-2000dps.
      - Gyroscope data rate from 4 Hz to 1125 Hz.
      - 3-axis 16-bit magnetometer (compass) with wide range of +/-4900uT (micro Tesla).
      - Magnetometer data rates 10 Hz, 20 Hz, 50 Hz, or 100 Hz.
      - 16-bit temperature sensor with readout rate of up to 8 kHz.
      - RAW data mode (get current sensor values at any time), buffered (FIFO) data mode (sensor values accumulate into
        a FIFO at a fixed rate), and digital motion processing mode (DMP; raw data is processed on-chip).
      - On-chip digital motion processor (DMP) can compute, inside the IMU chip itself:
        + quaternion 6 (uses accel + gyro),
        + quaternion 9 (uses accel + gyro + compass),
        + geomag quaternion (uses accel + compass),
        + flip/pickup detection,
        + step detection and counting,
        + basic activity recognition: drive, walk, run, bike, tilt, still.

      With quaternions computed on-chip, with an algorithm that gets sensor data at a highly accurate, fixed rate, and
      applies various calibrations, drift corrections, and compensations on the fly, one gets highly accurate real-time
      estimate of the sensor's pose in the 3D world and of how it is moving.

      Note that communication with the IMU is over a 400kHz I2C bus, which may limit data readout rate depending on
      which data is requested from the IMU.

      This IMU has 3 basic modes of operation (parameter mode, which can only be set in params.cfg):

      - RAW: One can access the latest raw sensor data at any time using the getRaw() or get() functions. This is the
        simplest mode of operation. One disadvantage is that if you are not calling get() at a perfectly regular
        interval, there will be some time jitter in your readouts. The IMU does not provide any time stamps for its
        data.

      - FIFO: In this mode, data from the sensor is piled up into a 1 kbyte FIFO buffer at a precise, constant rate
        (when all three of accelerometer, gyroscope, and magnetometer are on, the gyro rate determines the FIFO
        buffering rate). Main advantage is that you can then read out the data without having to worry about calling
        getRaw() or get() at a highly precise interval. But you need to be careful that the FIFO can fill up and
        overflow very quickly when using high sensor data rates.

      - DMP: In this mode, data is captured from the sensor at an accurate, fixed rate, and is fed to the on-chip
        digital motion processor (DMP). The DMP then computes quaternions, activity recognition, etc and pushes data
        packets into the FIFO as results from these algorithms become available.

      CAUTION: This class has state and is not re-entrant. Namely, we store the current register bank of the IMU chip in
      a class member variable. This could get out of sync if several member functions of this class are called
      concurrently. Because we assume this will not be the case in normal operation, we do not protect chip access with
      a mutex. Users should use an external mutex to protect this class if used in a multithreaded manner.

      \ingroup core.*/
  class ICM20948 : public Component,
                   public Parameter<imu::mode, imu::dmp, imu::arate, imu::grate, imu::mrate, imu::abw, imu::gbw,
                                    imu::tbw, imu::arange, imu::grange, imu::pktdbg>
  {
    public:
      //! Base class constructor ok
      using jevois::Component::Component;

      //! Virtual destructor for safe inheritance
      virtual ~ICM20948();

      //! Returns true if this camera indeed has a working ICM20948
      bool ready();

      //! Returns the amount of new data that had not previously been obtained
      /*! In RAW mode, this is either 0 or 1. In FIFO or DMP mode, returns length in bytes of new data in the FIFO. */
      int dataReady();
      
      //! Get one round of raw data
      /*! In RAW or DMP mode, this is the latest available, and blocking has no effect as some latest data always is
          available immediately. In FIFO mode, this is the oldest data in the FIFO, and we may block if the FIFO is
          currently empty. */
      IMUrawData getRaw(bool blocking = true);
      
      //! Get one round of scaled raw data
      /*! In RAW or DMP mode, this is the latest available, and blocking has no effect as some latest data always is
          available immediately. In FIFO mode, this is the oldest data in the FIFO, and we may block if the FIFO is
          currently empty. */
      IMUdata get(bool blocking = true);

      //! Get one packet of DMP data
      /*! Will throw if mode is not set to DMP. If blocking is true, will block until one new data packet is
          available from the IMU; otherwise, may return DMPdata with contents set to zero. */
      DMPdata getDMP(bool blocking = true);
      
    protected:
      //! Connect to and initialize the IMU chip
      void preInit() override;

      //! Configure RAW vs DMP mode:
      void postInit() override;

      //! Unfreeze any previously frozen parameters
      void preUninit() override;
     
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

      //! Write a value to a DMP register (data will be written in big endian)
      void writeDMP(unsigned short reg, unsigned short val);
      
      //! Write values to DMP
      /*! Caution: this code is not handling bank crossings. */
      void writeDMParray(unsigned short reg, unsigned char const * vals, size_t num);
      
      //! Read a value from a DMP register
      unsigned short readDMP(unsigned short reg);

      //! Read values from DMP
      void readDMParray(unsigned short reg, unsigned char * vals, size_t num);

    private:
      // Parameter callbacks:
      void onParamChange(imu::arate const & param, float const & newval);
      void onParamChange(imu::grate const & param, float const & newval);
      void onParamChange(imu::mrate const & param, imu::MagRate const & newval);
      void onParamChange(imu::abw const & param, unsigned int const & newval);
      void onParamChange(imu::gbw const & param, unsigned int const & newval);
      void onParamChange(imu::tbw const & param, unsigned int const & newval);
      void onParamChange(imu::arange const & param, unsigned int const & newval);
      void onParamChange(imu::grange const & param, unsigned int const & newval);
      void onParamChange(imu::dmp const & param, std::string const & newval);

      unsigned char readMagRegister(unsigned char magreg);
      void writeMagRegister(unsigned char magreg, unsigned char val);
      void waitForSlave4();
      void computeFIFOpktSize(float ar, float gr, int mm);
      size_t getDMPsome(bool blocking, size_t desired);
      
      uint8_t itsBank = 0;

      unsigned short itsFIFOpktSiz = 0; // Cached size of a FIFO-mode packet

      unsigned char itsDMPpacket[1024]; // fixme max packet size...
      int itsDMPsz = 0; // size of DMP data received so far
  };
}
