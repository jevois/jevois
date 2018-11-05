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
unsigned char jevois::ICM20948::readMagRegister(unsigned char magreg)
{
  // We use slave4, which is oneshot:
  writeRegister(ICM20948_REG_I2C_SLV4_ADDR, ICM20948_BIT_I2C_READ | COMPASS_SLAVEADDR);
  writeRegister(ICM20948_REG_I2C_SLV4_REG, magreg);
  writeRegister(ICM20948_REG_I2C_SLV4_CTRL, ICM20948_BIT_I2C_SLV_EN);

  waitForSlave4();
  
  return readRegister(ICM20948_REG_I2C_SLV4_DI);
}

// ####################################################################################################
void jevois::ICM20948::waitForSlave4()
{
  // Wait until the data is ready:
  auto tooLate = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(300);

  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    unsigned char status = readRegister(ICM20948_REG_I2C_MST_STATUS);
    if (status & ICM20948_BIT_SLV4_NACK) LFATAL("Failed to communicate with compass: NACK");
    if (status & ICM20948_BIT_SLV4_DONE) return; // Transaction to slave is complete
  }
  while (std::chrono::high_resolution_clock::now() < tooLate);

  LFATAL("Failed to communicate with compass: timeout");
}

// ####################################################################################################
void jevois::ICM20948::writeMagRegister(unsigned char magreg, unsigned char val)
{
  // We use slave4, which is oneshot:
  writeRegister(ICM20948_REG_I2C_SLV4_ADDR, COMPASS_SLAVEADDR);
  writeRegister(ICM20948_REG_I2C_SLV4_REG, magreg);
  writeRegister(ICM20948_REG_I2C_SLV4_DO, val);
  writeRegister(ICM20948_REG_I2C_SLV4_CTRL, ICM20948_BIT_I2C_SLV_EN);

  waitForSlave4();
}

// ####################################################################################################
void jevois::ICM20948::preInit()
{
  // Make sure the chip is on:
  writeRegister(ICM20948_REG_PWR_MGMT_1, 0x01); // Auto clock selection as recommended by datasheet
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  if (ready() == false) LFATAL("Cannot access ICM20948 inertial measurement unit (IMU) chip. This chip is only "
                               "available with a modified Global Shutter OnSemi (Aptina) AR0135 camera sensor. "
                               "It is not available on standard JeVois cameras.");

  // Configure the compass:

  // First, disable slaves:
  writeRegister(ICM20948_REG_I2C_SLV0_CTRL, 0);
  writeRegister(ICM20948_REG_I2C_SLV1_CTRL, 0);
  writeRegister(ICM20948_REG_I2C_SLV2_CTRL, 0);
  writeRegister(ICM20948_REG_I2C_SLV3_CTRL, 0);
  writeRegister(ICM20948_REG_I2C_SLV4_CTRL, 0);

  // Recommended target for I2C master clock:
  writeRegister(ICM20948_REG_I2C_MST_CTRL, 0x07 | ICM20948_BIT_I2C_MST_P_NSR);

  // Enable master I2C:
  unsigned char v = readRegister(ICM20948_REG_USER_CTRL);
  v |= ICM20948_BIT_I2C_MST_EN;
  writeRegister(ICM20948_REG_USER_CTRL, v);

  // Check the who-am-I of the magnetometer, to make sure I2C master is working:
  unsigned char wia2 = readMagRegister(REG_AK09916_WIA);
  if (wia2 != VAL_AK09916_WIA) LFATAL("Cannot communicate with magnetometer");
  LINFO("AK09916 magnetometer ok.");
  
  // Turn on the magnetometer:
  writeMagRegister(REG_AK09916_CNTL2, VAL_AK09916_CNTL2_PD); // first, power down mode
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  writeMagRegister(REG_AK09916_CNTL2, VAL_AK09916_CNTL2_MOD4); // then, mode 4: 100Hz

  // Set output data rate (but is overridden by gyro ODR wen gyro is on):
  writeRegister(ICM20948_REG_I2C_MST_ODR_CONFIG, 0x04); // Rate is 1.1kHz/(2^value)

  // Wait for magnetometer to be on:
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  // Check that the magnetometer indeed is in mode 4:
  int mode = readMagRegister(REG_AK09916_CNTL2);
  if (mode != VAL_AK09916_CNTL2_MOD4) LERROR("Warning: magnetometer slave mode is " << mode);

  // Setup to transfer 6 bytes of data from magnetometer to ICM20948 main:
  writeRegister(ICM20948_REG_I2C_SLV0_ADDR, ICM20948_BIT_I2C_READ | COMPASS_SLAVEADDR);
  writeRegister(ICM20948_REG_I2C_SLV0_REG, REG_AK09916_HXL);
  writeRegister(ICM20948_REG_I2C_SLV0_CTRL, // Enable, byteswap, odd-grouping, and read 8 bytes
                ICM20948_BIT_I2C_SLV_EN | ICM20948_BIT_I2C_BYTE_SW | ICM20948_BIT_I2C_GRP | 8);
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
  readRegisterArray(ICM20948_REG_ACCEL_XOUT_H_SH, reinterpret_cast<unsigned char *>(&d.v[0]), 11*2);

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

  double const ar = jevois::imu::arange::get() / 32768.0;
  d.ax() = rd.ax() * ar;
  d.ay() = rd.ay() * ar;
  d.az() = rd.az() * ar;

  double const gr = jevois::imu::grange::get() / 32768.0;
  d.gx() = rd.gx() * gr;
  d.gy() = rd.gy() * gr;
  d.gz() = rd.gz() * gr;

  d.temp() = rd.temp() / 333.87F + 21.0F;

  double const mr = 4912.0 / 32752.0; // full range is +/-4912uT for raw values +/-32752
  d.mx() = rd.mx() * mr;
  d.my() = rd.my() * mr;
  d.mz() = rd.mz() * mr;

  d.magovf = ((rd.mst2() & BIT_AK09916_STATUS2_HOFL) != 0);
  
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
