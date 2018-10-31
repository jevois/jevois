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

#include <jevois/Core/Engine.H>
#include <jevois/Core/ICM20948.H>
#include <jevois/Core/ICM20948_regs.H>

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
 
// ####################################################################################################
void jevois::ICM20948::selectBank(unsigned short reg)
{
  uint8_t const bank = (reg >> 7) & 0x03;
  if (itsBank == bank) return;
  
  engine()->writeIMUregister(ICM20948_REG_BANK_SEL, bank << 4);
  itsBank = bank;
}
 
// ####################################################################################################
void jevois::ICM20948::writeRegister(unsigned short reg, unsigned char val)
{
  selectBank(reg);
  engine()->writeIMUregister(reg & 0x7f, val);
}

// ####################################################################################################
unsigned char jevois::ICM20948::readRegister(unsigned short reg)
{
  selectBank(reg);
  return engine()->readIMUregister(reg & 0x7f);
}

// ####################################################################################################
void jevois::ICM20948::writeRegisterArray(unsigned short reg, unsigned char const * vals, size_t num)
{
  selectBank(reg);
  engine()->writeIMUregisterArray(reg & 0x7f, vals, num);
}

// ####################################################################################################
void jevois::ICM20948::readRegisterArray(unsigned short reg, unsigned char * vals, size_t num)
{
  selectBank(reg);
  return engine()->readIMUregisterArray(reg & 0x7f, vals, num);
}

// ####################################################################################################
jevois::ICM20948::~ICM20948(void)
{ }

// ####################################################################################################
void jevois::ICM20948::postInit()
{
  if (ready() == false) LFATAL("Cannot access ICM20948 inertial measurement unit (IMU) chip. This chip is only "
                               "available with a modified Global Shutter OnSemi (Aptina) AR0135 camera sensor. "
                               "It is not available on standard JeVois cameras.");
}

// ####################################################################################################
bool jevois::ICM20948::ready()
{
  return (readRegister(ICM20948_REG_WHO_AM_I) == ICM20948_DEVICE_ID);
}

// ####################################################################################################
jevois::IMUrawData jevois::ICM20948::getRaw()
{
  IMUrawData d;

  // Grab the raw data register contents:
  readRegisterArray(ICM20948_REG_ACCEL_XOUT_H_SH, reinterpret_cast<unsigned char *>(&d.v[0]), 10*2);

  // The data from the sensor is in big endian, convert to little endian:
  for (short & s : d.v) { short hi = (s & 0xff00) >> 8; short lo = s & 0x00ff; s = (lo << 8) | hi; }

  return d;
}

// ####################################################################################################
jevois::IMUdata jevois::ICM20948::get()
{
  // Get the raw data:
  IMUrawData rd = getRaw();

  // Scale it:
  IMUdata d;

  float const ar = jevois::imu::arange::get();
  d.ax() = rd.ax() * ar / 32768.0F;
  d.ay() = rd.ay() * ar / 32768.0F;
  d.az() = rd.az() * ar / 32768.0F;

  float const gr = jevois::imu::grange::get();
  d.gx() = rd.gx() * gr / 32768.0F;
  d.gy() = rd.gy() * gr / 32768.0F;
  d.gz() = rd.gz() * gr / 32768.0F;

  d.temp() = rd.temp() / 333.87F + 21.0F;

  float const mr = 32768.0F;///////////FIXME jevois::imu::mrange::get();
  d.mx() = rd.mx() * mr / 32768.0F;
  d.my() = rd.my() * mr / 32768.0F;
  d.mz() = rd.mz() * mr / 32768.0F;
  
  return d;
}
 
// ####################################################################################################
void jevois::ICM20948::reset(void)
{
  // Set H_RESET bit to initiate soft reset:
  writeRegister(ICM20948_REG_PWR_MGMT_1, ICM20948_BIT_H_RESET);
 
  // Wait 100ms to complete the reset sequence:
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::arate const & JEVOIS_UNUSED_PARAM(param), float const & newval)
{
  // Disable or enable accelerometer:
  uint8_t pwr = readRegister(ICM20948_REG_PWR_MGMT_2);
  if (newval == 0.0F) pwr |= ICM20948_BIT_PWR_ACCEL_STBY; else pwr &= ~ICM20948_BIT_PWR_ACCEL_STBY;
  writeRegister(ICM20948_REG_PWR_MGMT_2, pwr);

  if (newval == 0.0F) return;

  // Calculate the sample rate divider:
  float accelSampleRate = (1125.0F / newval) - 1.0F;
 
  // Check if it fits in the divider registers:
  if (accelSampleRate > 4095.0F) accelSampleRate = 4095.0F; else if (accelSampleRate < 0.0F) accelSampleRate = 0.0F;
 
  // Write the value to the registers:
  uint16_t const accelDiv = uint16_t(accelSampleRate);
  writeRegister(ICM20948_REG_ACCEL_SMPLRT_DIV_1, uint8_t(accelDiv >> 8) );
  writeRegister(ICM20948_REG_ACCEL_SMPLRT_DIV_2, uint8_t(accelDiv & 0xFF) );
 
  // Calculate the actual sample rate from the divider value:
  LINFO("Accelerometer sampling rate set to " << 1125.0F / (accelDiv + 1.0F) << " Hz");
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::grate const & JEVOIS_UNUSED_PARAM(param), float const & newval)
{
  // Disable or enable gyro:
  uint8_t pwr = readRegister(ICM20948_REG_PWR_MGMT_2);
  if (newval == 0.0F) pwr |= ICM20948_BIT_PWR_GYRO_STBY; else pwr &= ~ICM20948_BIT_PWR_GYRO_STBY;
  writeRegister(ICM20948_REG_PWR_MGMT_2, pwr);

  if (newval == 0.0F) return;

  // Calculate the sample rate divider:
  float gyroSampleRate = (1125.0F / newval) - 1.0F; // FIXME: code says 1125, datasheet says 1100
 
  // Check if it fits in the divider registers:
  if (gyroSampleRate > 255.0F) gyroSampleRate = 255.0F; else if (gyroSampleRate < 0.0F) gyroSampleRate = 0.0F;
 
  // Write the value to the register:
  uint8_t const gyroDiv = uint8_t(gyroSampleRate);
  writeRegister(ICM20948_REG_GYRO_SMPLRT_DIV, gyroDiv);
 
  // Calculate the actual sample rate from the divider value:
  LINFO("Gyroscope sampling rate set to " << 1125.0F / (gyroDiv + 1.0F) << " Hz");
}
 
// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::abw const & JEVOIS_UNUSED_PARAM(param),
                                     unsigned int const & newval)
{
  uint8_t reg = readRegister(ICM20948_REG_ACCEL_CONFIG);

  switch (newval)
  {
  case 0: reg &= ~ICM20948_BIT_ACCEL_FCHOICE; break; // turn off low-pass filter
  case 6: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_6HZ; break;
  case 12: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_12HZ; break;
  case 24: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_24HZ; break;
  case 50: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_50HZ; break;
  case 111: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_111HZ; break;
  case 246: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_246HZ; break;
  case 470: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_470HZ; break;
  case 1210: reg &= ~ICM20948_MASK_ACCEL_BW; reg |= ICM20948_ACCEL_BW_1210HZ; break;
  default: LFATAL("Invalid value");
  }
  
  writeRegister(ICM20948_REG_ACCEL_CONFIG, reg);
}
 
// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::gbw const & JEVOIS_UNUSED_PARAM(param),
                                     unsigned int const & newval)
{
  uint8_t reg = readRegister(ICM20948_REG_GYRO_CONFIG_1);

  switch (newval)
  {
  case 0: reg &= ~ICM20948_BIT_GYRO_FCHOICE; break; // turn off low-pass filter
  case 6: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_6HZ; break;
  case 12: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_12HZ; break;
  case 24: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_24HZ; break;
  case 51: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_51HZ; break;
  case 120: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_120HZ; break;
  case 150: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_150HZ; break;
  case 200: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_200HZ; break;
  case 360: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_360HZ; break;
  case 12100: reg &= ~ICM20948_MASK_GYRO_BW; reg |= ICM20948_GYRO_BW_12100HZ; break;
  default: LFATAL("Invalid value");
  }
  
  writeRegister(ICM20948_REG_GYRO_CONFIG_1, reg);
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::tbw const & JEVOIS_UNUSED_PARAM(param),
                                     unsigned int const & newval)
{
  // Disable or enable temperature:
  uint8_t pwr = readRegister(ICM20948_REG_PWR_MGMT_1);
  if (newval == 0) pwr |= ICM20948_BIT_TEMP_DIS; else pwr &= ~ICM20948_BIT_TEMP_DIS;
  writeRegister(ICM20948_REG_PWR_MGMT_1, pwr);

  if (newval == 0) return;

  switch (newval)
  {
  case 9: writeRegister(ICM20948_REG_TEMP_CONFIG, 6); break;
  case 17: writeRegister(ICM20948_REG_TEMP_CONFIG, 5); break;
  case 34: writeRegister(ICM20948_REG_TEMP_CONFIG, 4); break;
  case 66: writeRegister(ICM20948_REG_TEMP_CONFIG, 3); break;
  case 123: writeRegister(ICM20948_REG_TEMP_CONFIG, 2); break;
  case 218: writeRegister(ICM20948_REG_TEMP_CONFIG, 1); break;
  case 7932: writeRegister(ICM20948_REG_TEMP_CONFIG, 0); break;
  default: LFATAL("Invalid value");
  }
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::arange const & JEVOIS_UNUSED_PARAM(param),
                                     unsigned int const & newval)
{
  uint8_t reg = readRegister(ICM20948_REG_ACCEL_CONFIG) & ~ICM20948_MASK_ACCEL_FULLSCALE;

  switch (newval)
  {
  case 2: reg |= ICM20948_ACCEL_FULLSCALE_2G; break;
  case 4: reg |= ICM20948_ACCEL_FULLSCALE_4G; break;
  case 8: reg |= ICM20948_ACCEL_FULLSCALE_8G; break;
  case 16: reg |= ICM20948_ACCEL_FULLSCALE_16G; break;
  default: LFATAL("Invalid value");
  }
  writeRegister(ICM20948_REG_ACCEL_CONFIG, reg);
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::grange const & JEVOIS_UNUSED_PARAM(param),
                                     unsigned int const & newval)
{
  uint8_t reg = readRegister(ICM20948_REG_GYRO_CONFIG_1) & ~ICM20948_MASK_GYRO_FULLSCALE;

  switch (newval)
  {
  case 250: reg |= ICM20948_GYRO_FULLSCALE_250DPS; break;
  case 500: reg |= ICM20948_GYRO_FULLSCALE_500DPS; break;
  case 1000: reg |= ICM20948_GYRO_FULLSCALE_1000DPS; break;
  case 2000: reg |= ICM20948_GYRO_FULLSCALE_2000DPS; break;
  default: LFATAL("Invalid value");
  }
  writeRegister(ICM20948_REG_GYRO_CONFIG_1, reg);
}

// ####################################################################################################
void jevois::ICM20948::sleep(bool enable)
{
  uint8_t reg = readRegister(ICM20948_REG_PWR_MGMT_1);

  if (enable) reg |= ICM20948_BIT_SLEEP;
  else reg &= ~(ICM20948_BIT_SLEEP);

  writeRegister(ICM20948_REG_PWR_MGMT_1, reg);
}

// ####################################################################################################
void jevois::ICM20948::cycle(bool enable)
{
  uint8_t reg = ICM20948_REG_LP_CONFIG;
  uint8_t const mask = ICM20948_BIT_I2C_MST_CYCLE | ICM20948_BIT_ACCEL_CYCLE | ICM20948_BIT_GYRO_CYCLE;

  if (enable) reg |= mask; else reg &= ~mask;
 
  writeRegister(ICM20948_REG_LP_CONFIG, reg);
}

// ####################################################################################################
uint32_t jevois::ICM20948::devid()
{ return readRegister(ICM20948_REG_WHO_AM_I); }
