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
void jevois::ICM20948::writeDMP(unsigned short reg, unsigned short val)
{
  // Write the data in big endian:
  unsigned char data[2];
  data[0] = val >> 8;
  data[1] = val & 0xff;
  
  writeDMParray(reg, &data[0], 2);
}

// ####################################################################################################
void jevois::ICM20948::writeDMParray(unsigned short reg, unsigned char const * vals, size_t num)
{
  // Select MEMs bank from the 8 MSBs of reg:
  writeRegister(ICM20948_REG_MEM_BANK_SEL, reg >> 8);

  // Set address:
  writeRegister(ICM20948_REG_MEM_START_ADDR, reg & 0xff);

  // Write data:
  writeRegisterArray(ICM20948_REG_MEM_R_W, vals, num);
}

// ####################################################################################################
unsigned short jevois::ICM20948::readDMP(unsigned short reg)
{
  // Read data in big endian:
  unsigned char data[2];
  readDMParray(reg, &data[0], 2);

  return (data[0] << 8) | data[1];
}

// ####################################################################################################
void jevois::ICM20948::readDMParray(unsigned short reg, unsigned char * vals, size_t num)
{
  // Select MEMs bank from the 8 MSBs of reg:
  writeRegister(ICM20948_REG_MEM_BANK_SEL, reg >> 8);

  // Set address:
  writeRegister(ICM20948_REG_MEM_START_ADDR, reg & 0xff);
  
  // Write data:
  readRegisterArray(ICM20948_REG_MEM_R_W, vals, num);
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
  try
  {
    writeRegister(ICM20948_REG_PWR_MGMT_1, 0x01); // Auto clock selection as recommended by datasheet
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    if (ready() == false) throw std::runtime_error("not ready");
  }
  catch (...)
  {
    LFATAL("Cannot access ICM20948 inertial measurement unit (IMU) chip. This chip is only "
           "available with a modified Global Shutter OnSemi (Aptina) AR0135 camera sensor. "
           "It is not included with standard JeVois cameras.");
  }
  
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

  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  // Check the who-am-I of the magnetometer, to make sure I2C master is working:
  unsigned char wia2 = readMagRegister(REG_AK09916_WIA);
  if (wia2 != VAL_AK09916_WIA) LFATAL("Cannot communicate with magnetometer");
  LINFO("AK09916 magnetometer ok.");
}

// ####################################################################################################
void jevois::ICM20948::postInit()
{
  // Configure the DMP if desired:
  mode::freeze();
  switch (mode::get())
  {
  case jevois::imu::Mode::DMP:
  {
    // The DMP code was loaded by the kernel driver, which also has set the start address. So here we just need to
    // launch the DMP. First, disable FIFO and DMP:
    LINFO("DMP reset start");
    unsigned char v = readRegister(ICM20948_REG_USER_CTRL);
    v &= ~(ICM20948_BIT_DMP_EN | ICM20948_BIT_FIFO_EN);
    v |= ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    LINFO("DMP is in reset");
    
    // Reset / setup the FIFO:
    writeRegister(ICM20948_REG_FIFO_CFG, ICM20948_BIT_SINGLE_FIFO_CFG); // FIFO Config
    writeRegister(ICM20948_REG_FIFO_RST, 0x1f); // Reset all FIFOs.
    writeRegister(ICM20948_REG_FIFO_RST, 0x1e); // Keep all but Gyro FIFO in reset.
    writeRegister(ICM20948_REG_FIFO_EN_1, 0x0); // Slave FIFO turned off.
    writeRegister(ICM20948_REG_FIFO_EN_2, 0x0); // Hardware FIFO turned off.

    writeRegister(ICM20948_REG_SINGLE_FIFO_PRIORITY_SEL, 0xe4); // Use a single interrupt for FIFO

    // Enable FIFO and DMP:
    LINFO("DMP enable start");
    LINFO("addr = " << std::hex << readRegister(ICM20948_BANK_2 | 0x50) << ' ' << readRegister(ICM20948_BANK_2 | 0x51));
    v |= ICM20948_BIT_DMP_EN | ICM20948_BIT_FIFO_EN;
    v &= ~ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Enable DMP interrupts:
    //writeRegister(ICM20948_REG_INT_ENABLE_1, 0x02); // Enable DMP Interrupt
    //writeRegister(ICM20948_REG_INT_ENABLE_2, ICM20948_BIT_FIFO_OVERFLOW_EN_0); // Enable FIFO Overflow Interrupt

    LINFO("DMP enabled");

    LINFO("IMU Digital Motion Processor (DMP) enabled.");
  }
  break;

  case jevois::imu::Mode::FIFO:
  {
    // The DMP code was loaded by the kernel driver, which also has set the start address. So here we just need to
    // launch the DMP. First, disable FIFO and DMP:
    unsigned char v = readRegister(ICM20948_REG_USER_CTRL);
    v &= ~(ICM20948_BIT_DMP_EN | ICM20948_BIT_FIFO_EN); v |= ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Reset / setup the FIFO:
    writeRegister(ICM20948_REG_FIFO_CFG, ICM20948_BIT_SINGLE_FIFO_CFG); // FIFO Config
    writeRegister(ICM20948_REG_FIFO_RST, 0x1f); // Reset all FIFOs.
    writeRegister(ICM20948_REG_FIFO_RST, 0x1e); // Keep all but Gyro FIFO in reset.
    writeRegister(ICM20948_REG_FIFO_EN_1, 0x0); // Slave FIFO turned off.
    writeRegister(ICM20948_REG_FIFO_EN_2, 0x0); // Hardware FIFO turned off.

    writeRegister(ICM20948_REG_SINGLE_FIFO_PRIORITY_SEL, 0xe4); // Use a single interrupt for FIFO

    // Enable FIFO:
    v |= ICM20948_BIT_FIFO_EN; v &= ~ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    computeFIFOpktSize(-1.0F, -1.0F, 1);

    LINFO("IMU FIFO mode enabled.");
  }
  break;

  case jevois::imu::Mode::RAW:
  {
    // Enable data ready interrupt so we can report when new data is ready:
    ///writeRegister(ICM20948_REG_INT_ENABLE_1, ICM20948_BIT_RAW_DATA_0_RDY_EN);
    LINFO("IMU raw data mode enabled.");
  }
  break;
  }
}

// ####################################################################################################
void jevois::ICM20948::preUninit()
{
  mode::unFreeze();
}

// ####################################################################################################
bool jevois::ICM20948::ready()
{
  return (readRegister(ICM20948_REG_WHO_AM_I) == ICM20948_DEVICE_ID);
}

// ####################################################################################################
int jevois::ICM20948::dataReady()
{
  switch(mode::get())
  {
  case jevois::imu::Mode::FIFO:
  case jevois::imu::Mode::DMP:
  {
    unsigned char datalen[2];
    readRegisterArray(ICM20948_REG_FIFO_COUNT_H, &datalen[0], 2);
    short dlen = (datalen[0] << 8) + datalen[1];
    return dlen;
  }
  case jevois::imu::Mode::RAW:
  {
    unsigned char val = readRegister(ICM20948_REG_INT_STATUS_1);
    if (val & ICM20948_BIT_RAW_DATA_0_RDY_INT) return 1;
    return 0;
  }
  }
  return 0; // Keep compiler happy
}

// ####################################################################################################
jevois::IMUrawData jevois::ICM20948::getRaw(bool blocking)
{
  IMUrawData d;

  switch(mode::get())
  {
  case jevois::imu::Mode::FIFO:
  {
    int siz = dataReady();

    // Gobble up the FIFO if it is getting full, so that we never get out of sync:
    if (siz > 800)
    {
      LERROR("IMU FIFO almost full. You need to call get() more often or reduce rate. Data will be lost.");
      while (siz > 500)
      {
        unsigned char trash[itsFIFOpktSiz];
        readRegisterArray(ICM20948_REG_FIFO_R_W, &trash[0], itsFIFOpktSiz);
        siz = dataReady();
      }
    }
    
    if (siz < itsFIFOpktSiz)
    {
      if (blocking == false) return d;

      auto n = std::chrono::high_resolution_clock::now();
      do
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        siz = dataReady();
      }
      while (siz < itsFIFOpktSiz && std::chrono::high_resolution_clock::now() - n < std::chrono::seconds(2));
      if (siz < itsFIFOpktSiz) { LERROR("TIMEOUT waiting for IMU FIFO data - RETURNING BLANK"); return d; }
    }

    // Read the packet:
    unsigned char packet[itsFIFOpktSiz];
    readRegisterArray(ICM20948_REG_FIFO_R_W, &packet[0], itsFIFOpktSiz);
  
    // Decode the packet:
    int off = 0;
    if (arate::get() > 0.0F)
    {
      d.ax() = packet[off + 0] * 256 + packet[off + 1];
      d.ay() = packet[off + 2] * 256 + packet[off + 3];
      d.az() = packet[off + 4] * 256 + packet[off + 5];
      off += 6;
    }
    if (grate::get() > 0.0F)
    {
      d.gx() = packet[off + 0] * 256 + packet[off + 1];
      d.gy() = packet[off + 2] * 256 + packet[off + 3];
      d.gz() = packet[off + 4] * 256 + packet[off + 5];
      off += 6;
    }
    if (mrate::get() != jevois::imu::MagRate::Off && mrate::get() != jevois::imu::MagRate::Once)
    {
      d.mx() = packet[off + 0] * 256 + packet[off + 1];
      d.my() = packet[off + 2] * 256 + packet[off + 3];
      d.mz() = packet[off + 4] * 256 + packet[off + 5];
      d.mst2() = packet[off + 6] * 256 + packet[off + 7];
      off += 8;
    }
    d.temp() = 0; // temp not available in FIFO mode
  }
  break;
  
  case jevois::imu::Mode::RAW:
  case jevois::imu::Mode::DMP:
  {
    // Grab the raw data register contents:
    readRegisterArray(ICM20948_REG_ACCEL_XOUT_H_SH, reinterpret_cast<unsigned char *>(&d.v[0]), 11*2);

    // The data from the sensor is in big endian, convert to little endian:
    for (short & s : d.v) { short hi = (s & 0xff00) >> 8; short lo = s & 0x00ff; s = (lo << 8) | hi; }
  }
  break;
  }
  
  return d;
}

// ####################################################################################################
jevois::IMUdata jevois::ICM20948::get(bool blocking)
{
  // Get the raw data:
  IMUrawData rd = getRaw(blocking);

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
jevois::DMPdata jevois::ICM20948::getDMP(bool blocking)
{
  if (mode::get() != jevois::imu::Mode::DMP) LFATAL("getDMP() only available when mode=DMP, see params.cfg");
  DMPdata d;
  
  // Check how much data is in the FIFO; we need at least one packet:
  int const pktsiz = jevois::DMPpacketSize(itsDMPctl1, itsDMPctl2);
  int siz = dataReady();
  if (siz < pktsiz)
  {
    if (blocking == false) return d;

    auto n = std::chrono::high_resolution_clock::now();
    do
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      siz = dataReady();
    }
    while (siz < pktsiz && std::chrono::high_resolution_clock::now() - n < std::chrono::seconds(2));
    if (siz < pktsiz) { LERROR("TIMEOUT waiting for IMU DMP data - RETURNING BLANK"); return d; }
  }

  // Read the packet:
  unsigned char packet[pktsiz];
  readRegisterArray(ICM20948_REG_FIFO_R_W, &packet[0], pktsiz);
  
  // to be continued....
  std::stringstream stream; stream << "FIFO data:" << std::hex;
  for (int i = 0; i < pktsiz; ++i) stream << ' ' << (unsigned int)(packet[i]);
  LINFO(stream.str());

  return DMPdata();
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

  if (newval)
  {
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
  
  computeFIFOpktSize(newval, -1.0F, -1);
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::grate const & JEVOIS_UNUSED_PARAM(param), float const & newval)
{
  // Disable or enable gyro:
  uint8_t pwr = readRegister(ICM20948_REG_PWR_MGMT_2);
  if (newval == 0.0F) pwr |= ICM20948_BIT_PWR_GYRO_STBY; else pwr &= ~ICM20948_BIT_PWR_GYRO_STBY;
  writeRegister(ICM20948_REG_PWR_MGMT_2, pwr);

  if (newval)
  {
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

  computeFIFOpktSize(-1.0F, newval, -1);
}

// ####################################################################################################
void jevois::ICM20948::onParamChange(jevois::imu::mrate const & JEVOIS_UNUSED_PARAM(param),
                                     jevois::imu::MagRate const & newval)
{
  // Turn off the magnetometer:
  writeMagRegister(REG_AK09916_CNTL2, VAL_AK09916_CNTL2_PD);

  // Set the mode value:
  unsigned char mode;
  switch (newval)
  {
  case jevois::imu::MagRate::Off: mode = VAL_AK09916_CNTL2_PD; break;
  case jevois::imu::MagRate::Once: mode = VAL_AK09916_CNTL2_SNGL; break;
  case jevois::imu::MagRate::M10Hz: mode = VAL_AK09916_CNTL2_MOD1; break;
  case jevois::imu::MagRate::M20Hz: mode = VAL_AK09916_CNTL2_MOD2; break;
  case jevois::imu::MagRate::M50Hz: mode = VAL_AK09916_CNTL2_MOD3; break;
  case jevois::imu::MagRate::M100Hz: mode = VAL_AK09916_CNTL2_MOD4; break;
  default: LFATAL("Invalid mode value: " << newval);
  }

  // Wait until mag is down:
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Now turn it back on in the specified mode:
  writeMagRegister(REG_AK09916_CNTL2, mode);

  // Set output data rate (but is overridden by gyro ODR wen gyro is on):
  writeRegister(ICM20948_REG_I2C_MST_ODR_CONFIG, 0x04); // Rate is 1.1kHz/(2^value)

  // Wait for magnetometer to be on:
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  computeFIFOpktSize(-1.0F, -1.0F, mode);

  // Setup to transfer 8 bytes of data from magnetometer to ICM20948 main:
  writeRegister(ICM20948_REG_I2C_SLV0_ADDR, ICM20948_BIT_I2C_READ | COMPASS_SLAVEADDR);
  writeRegister(ICM20948_REG_I2C_SLV0_REG, REG_AK09916_HXL);
  writeRegister(ICM20948_REG_I2C_SLV0_CTRL, // Enable, byteswap, odd-grouping, and read 8 bytes
                ICM20948_BIT_I2C_SLV_EN | ICM20948_BIT_I2C_BYTE_SW | ICM20948_BIT_I2C_GRP | 8);
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

// ####################################################################################################
void jevois::ICM20948::onParamChange(imu::dmp const & JEVOIS_UNUSED_PARAM(param), std::string const & newval)
{
  itsDMPctl1 = 0; itsDMPctl2 = 0;
  
  for (unsigned char c : newval)
    switch(c)
    {
    case 'A': itsDMPctl1 |= JEVOIS_DMP_ACCEL; break;
    case 'G': itsDMPctl1 |= JEVOIS_DMP_GYRO; break;
    case 'M': itsDMPctl1 |= JEVOIS_DMP_CPASS; break;
    case 'R': itsDMPctl1 |= JEVOIS_DMP_QUAT6; break;
    case 'Q': itsDMPctl1 |= JEVOIS_DMP_QUAT9; break;
    case 'E': itsDMPctl1 |= JEVOIS_DMP_GEOMAG; break;
    case 'g': itsDMPctl1 |= JEVOIS_DMP_GYRO_CALIBR; break;
    case 'm': itsDMPctl1 |= JEVOIS_DMP_CPASS_CALIBR; break;
    case 'S': itsDMPctl1 |= JEVOIS_DMP_PED_STEPDET; break;
    default: LERROR("Phony character '" << c << "' ignored while parsing parameter dmp.");
    }

  // Set the two control parameters in the IMU:
  writeDMP(DMP_DATA_OUT_CTL1, itsDMPctl1);
  writeDMP(DMP_DATA_OUT_CTL2, itsDMPctl2);
  writeDMP(DMP_FIFO_WATERMARK, 800);

  //writeDMP(DMP_DATA_INTR_CTL, 0);
  //writeDMP(DMP_MOTION_EVENT_CTL, 0); // fixme
  //writeDMP(DMP_DATA_RDY_STATUS, 0);




  // note BAC and B2S require 56Hz
  // see dmp_icm20948_set_bac_rate
  // and more generally we should set ODR
}

// ####################################################################################################
void jevois::ICM20948::computeFIFOpktSize(float ar, float gr, int mm)
{
  itsFIFOpktSiz = 0;

  // If we are in FIFO mode, make sure we send that data to the FIFO:
  if (mode::get() == jevois::imu::Mode::FIFO)
  {
    // Since this is called from within parameter callbacks, we need a bit of trickery to get the new rate:
    float acc; if (ar < 0.0F) acc = arate::get(); else acc = ar;
    float gyr; if (gr < 0.0F) gyr = grate::get(); else gyr = gr;
    jevois::imu::MagRate mag; if (mm == -1) mag = mrate::get(); else mag = jevois::imu::MagRate::M100Hz; // any value ok
    
    // Packet size may change. We need to nuke the FIFO to avoid mixed packets:
    unsigned char ctl = readRegister(ICM20948_REG_USER_CTRL);
    ctl &= ~ICM20948_BIT_FIFO_EN;
    writeRegister(ICM20948_REG_USER_CTRL, ctl);

    writeRegister(ICM20948_REG_FIFO_CFG, ICM20948_BIT_SINGLE_FIFO_CFG); // FIFO Config
    writeRegister(ICM20948_REG_FIFO_RST, 0x1f); // Reset all FIFOs.
    writeRegister(ICM20948_REG_FIFO_EN_1, 0x0); // Slave FIFO turned off.
    writeRegister(ICM20948_REG_FIFO_EN_2, 0x0); // Hardware FIFO turned off.

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // FIXME: should wait more at low rates...
    
    int siz = dataReady(); unsigned char trash[16];
    while (siz)
    {
      readRegisterArray(ICM20948_REG_FIFO_R_W, &trash[0], std::min(siz, 16));
      siz = dataReady();
    }

    writeRegister(ICM20948_REG_FIFO_RST, 0x00); // Stop FIFO reset

    unsigned char v = readRegister(ICM20948_REG_FIFO_EN_2);
    if (acc > 0.0F) { v |= ICM20948_BIT_ACCEL_FIFO_EN; itsFIFOpktSiz += 6; } // 3 axes, short int values
    else v &= ~ICM20948_BIT_ACCEL_FIFO_EN;
    if (gyr > 0.0F) { v |= ICM20948_BITS_GYRO_FIFO_EN; itsFIFOpktSiz += 6; } // 3 axes, short int values
    else v &= ~ICM20948_BITS_GYRO_FIFO_EN;
    writeRegister(ICM20948_REG_FIFO_EN_2, v);

    v = readRegister(ICM20948_REG_FIFO_EN_1);
    if (mag != jevois::imu::MagRate::Off && mag != jevois::imu::MagRate::Once)
    { v |= ICM20948_BIT_SLV_0_FIFO_EN; itsFIFOpktSiz += 8; } // 3 axes, short int values + short status
    else v &= ~ICM20948_BIT_SLV_0_FIFO_EN;
    writeRegister(ICM20948_REG_FIFO_EN_1, v);

    // Enable FIFO:
    ctl |= ICM20948_BIT_FIFO_EN;
    writeRegister(ICM20948_REG_USER_CTRL, ctl);
  }
}

