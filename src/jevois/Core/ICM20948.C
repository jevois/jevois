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

  // Enable 0x80 in ICM20948_REG_PWR_MGMT_2, needed by DMP:
  writeRegister(ICM20948_REG_PWR_MGMT_2, readRegister(ICM20948_REG_PWR_MGMT_2) | 0x80);
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
    unsigned char v = readRegister(ICM20948_REG_USER_CTRL);
    v &= ~(ICM20948_BIT_DMP_EN | ICM20948_BIT_FIFO_EN); v |= ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Reset / setup the FIFO:
    writeRegister(ICM20948_REG_FIFO_CFG, ICM20948_BIT_MULTI_FIFO_CFG); // FIFO Config
    writeRegister(ICM20948_REG_FIFO_RST, 0x1f); // Reset all FIFOs.
    writeRegister(ICM20948_REG_FIFO_RST, 0x1e); // Keep all but Gyro FIFO in reset.
    writeRegister(ICM20948_REG_FIFO_EN_1, 0x0); // Slave FIFO turned off.
    writeRegister(ICM20948_REG_FIFO_EN_2, 0x0); // Hardware FIFO turned off.

    writeRegister(ICM20948_REG_SINGLE_FIFO_PRIORITY_SEL, 0xe4); // Use a single interrupt for FIFO

    // Enable DMP interrupts:
    writeRegister(ICM20948_REG_INT_ENABLE, ICM20948_BIT_DMP_INT_EN); // Enable DMP Interrupt
    writeRegister(ICM20948_REG_INT_ENABLE_1, ICM20948_BIT_RAW_DATA_0_RDY_EN | ICM20948_BIT_RAW_DATA_1_RDY_EN |
                  ICM20948_BIT_RAW_DATA_2_RDY_EN | ICM20948_BIT_RAW_DATA_3_RDY_EN); // Enable raw data ready Interrupt
    writeRegister(ICM20948_REG_INT_ENABLE_2, ICM20948_BIT_FIFO_OVERFLOW_EN_0); // Enable FIFO Overflow Interrupt

    // Enable DMP:
    v |= ICM20948_BIT_DMP_EN | ICM20948_BIT_FIFO_EN; v &= ~ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Force an execution of our dmp callback:
    dmp::set(dmp::get());
    
    LINFO("IMU Digital Motion Processor (DMP) enabled.");
  }
  break;

  case jevois::imu::Mode::FIFO:
  {
    // Disable the DMP:
    unsigned char v = readRegister(ICM20948_REG_USER_CTRL);
    v &= ~ICM20948_BIT_DMP_EN; v |= ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);

    // Enable FIFO, set packet size, nuke any old FIFO data:
    computeFIFOpktSize(-1.0F, -1.0F, -1);

    // DMP is not available:
    dmp::freeze();
    
    LINFO("IMU FIFO mode enabled.");
  }
  break;

  case jevois::imu::Mode::RAW:
  {
    // Disable the DMP and FIFO:
    unsigned char v = readRegister(ICM20948_REG_USER_CTRL);
    v &= ~(ICM20948_BIT_DMP_EN | ICM20948_BIT_FIFO_EN); v |= ICM20948_BIT_DMP_RST;
    writeRegister(ICM20948_REG_USER_CTRL, v);

    // DMP is not available:
    dmp::freeze();
    pktdbg::freeze();

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
    // ----------------------------------------------------------------------------------------------------
  case jevois::imu::Mode::FIFO:
  {
    int siz = dataReady();

    // Gobble up the FIFO if it is getting full, so that we never get out of sync:
    if (siz > 500)
    {
      LERROR("IMU FIFO filling up. You need to call get() more often or reduce rate. Data will be lost.");
      while (siz > 100)
      {
        unsigned char trash[itsFIFOpktSiz];
        readRegisterArray(ICM20948_REG_FIFO_R_W, &trash[0], itsFIFOpktSiz);
        siz = dataReady();
      }
    }
    
    if (siz < itsFIFOpktSiz)
    {
      if (blocking == false) return d;

      auto tooLate = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2);
      do
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (std::chrono::high_resolution_clock::now() > tooLate)
        { LERROR("TIMEOUT waiting for IMU FIFO data - RETURNING BLANK"); return d; }
        siz = dataReady();
      }
      while (siz < itsFIFOpktSiz);
    }

    // Read the packet:
    unsigned char packet[itsFIFOpktSiz];
    readRegisterArray(ICM20948_REG_FIFO_R_W, &packet[0], itsFIFOpktSiz);
  
    // Decode the packet:
    int off = 0;
    if (arate::get() > 0.0F)
    {
      d.ax() = (packet[off + 0] << 8) | packet[off + 1];
      d.ay() = (packet[off + 2] << 8) | packet[off + 3];
      d.az() = (packet[off + 4] << 8) | packet[off + 5];
      off += 6;
    }
    if (grate::get() > 0.0F)
    {
      d.gx() = (packet[off + 0] << 8) | packet[off + 1];
      d.gy() = (packet[off + 2] << 8) | packet[off + 3];
      d.gz() = (packet[off + 4] << 8) | packet[off + 5];
      off += 6;
    }
    if (mrate::get() != jevois::imu::MagRate::Off && mrate::get() != jevois::imu::MagRate::Once)
    {
      d.mx() = (packet[off + 0] << 8) | packet[off + 1];
      d.my() = (packet[off + 2] << 8) | packet[off + 3];
      d.mz() = (packet[off + 4] << 8) | packet[off + 5];
      d.mst2() = (packet[off + 6] << 8) | packet[off + 7];
      off += 8;
    }
    d.temp() = 0; // temp not available in FIFO mode

    // Debug raw packet dump:
    if (pktdbg::get())
    {
      std::stringstream stream; stream << "RAW" << std::hex;
      for (int i = 0; i < itsFIFOpktSiz; ++i) stream << ' ' << (unsigned int)(packet[i]);
      LINFO(stream.str());
    }
  }
  break;
  
  // ----------------------------------------------------------------------------------------------------
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
  return jevois::IMUdata(getRaw(blocking), jevois::imu::arange::get(), jevois::imu::grange::get());
}

// ####################################################################################################
jevois::DMPdata jevois::ICM20948::getDMP(bool blocking)
{
  if (mode::get() != jevois::imu::Mode::DMP) LFATAL("getDMP() only available when mode=DMP, see params.cfg");
  DMPdata d;

  // Start parsing a new packet?
  if (itsDMPsz < 4)
  {
    // We need at least 4 bytes in the FIFO to get going. Could be either two empty packets (may not exist), or the
    // start of a non-empty packet, possibly with a header2 (which is why we want 4):
    size_t got = getDMPsome(blocking, 4);
    if (got < 4) return d;
  }
  
  // Parse the headers:
  unsigned short ctl1 = (itsDMPpacket[0] << 8) | itsDMPpacket[1];
  int off = 2; // offset to where we are in parsing the packet so far
  unsigned short ctl2 = 0;
  if (ctl1 & JEVOIS_DMP_HEADER2) { ctl2 = (itsDMPpacket[off] << 8) | itsDMPpacket[off + 1]; off += 2; }
  
  // Compute how much data remains to be grabbed for this packet:
  size_t need = jevois::DMPpacketSize(ctl1, ctl2) - itsDMPsz;

  // Read the rest of the packet if needed:
  while (need > 0)
  {
    // We can read out max 32 bytes at a time:
    size_t need2 = std::min(need, size_t(32));
    size_t got = getDMPsome(blocking, need2);
    if (got < need2) return d;
    need -= got;
  }

  // We have the whole packet, parse it:
  d.parsePacket(itsDMPpacket, itsDMPsz);
  
  // Debug raw packet dump:
  if (pktdbg::get())
  {
    std::stringstream stream; stream << "RAW" << std::hex;
    for (int i = 0; i < itsDMPsz; ++i) stream << ' ' << (unsigned int)(itsDMPpacket[i]);
    LINFO(stream.str());
  }
  
  // Done with this packet, nuke it:
  itsDMPsz = 0;
  
  return d;
}

// ####################################################################################################
  size_t jevois::ICM20948::getDMPsome(bool blocking, size_t desired)
{
  size_t siz = dataReady();
  
  if (siz < desired)
  {
    if (blocking == false) return 0;

    auto tooLate = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2);
    do
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      if (std::chrono::high_resolution_clock::now() >= tooLate)
      { LERROR("TIMEOUT waiting for IMU DMP data - RETURNING BLANK"); return 0; }

      siz = dataReady();
    }
    while (siz < desired);
  }

  // We have enough data in the FIFO, read out what we wanted:
  readRegisterArray(ICM20948_REG_FIFO_R_W, &itsDMPpacket[itsDMPsz], desired);
  itsDMPsz += desired;
  
  return desired;
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

  // Setup to transfer 8 bytes (RAW, FIFO) or 6 bytes (DMP) of data from magnetometer to ICM20948 main:
  writeRegister(ICM20948_REG_I2C_SLV0_ADDR, ICM20948_BIT_I2C_READ | COMPASS_SLAVEADDR);
  writeRegister(ICM20948_REG_I2C_SLV0_REG, REG_AK09916_HXL);
  int siz = (mode::get() == jevois::imu::Mode::DMP) ? 6 : 8;
  writeRegister(ICM20948_REG_I2C_SLV0_CTRL, // Enable, byteswap, odd-grouping, and read 8 or 6 bytes
                ICM20948_BIT_I2C_SLV_EN | ICM20948_BIT_I2C_BYTE_SW | ICM20948_BIT_I2C_GRP | siz);
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
  unsigned short ctl1 = 0, ctl2 = 0, mec = 0;
  bool a = false, g = false, m = false, h2 = false;
  
  LINFO("Setting dmp parameter to " << newval);  

  for (unsigned char c : newval)
    switch(c)
    {
    case 'A': ctl1 |= JEVOIS_DMP_ACCEL; a = true; break;
    case 'G': ctl1 |= JEVOIS_DMP_GYRO; g = true; break;
    case 'M': ctl1 |= JEVOIS_DMP_CPASS; m = true; break;
    case 'R': ctl1 |= JEVOIS_DMP_QUAT6; a = true; g = true; break;
    case 'Q': ctl1 |= JEVOIS_DMP_QUAT9; a = true; g = true; m = true; mec |= JEVOIS_DMP_NINE_AXIS_EN; break;
    case 'E': ctl1 |= JEVOIS_DMP_GEOMAG; a = true; g = true; m = true; mec |= JEVOIS_DMP_GEOMAG_EN; break;
    case 'g': ctl1 |= JEVOIS_DMP_GYRO_CALIBR; g = true; break;
    case 'm': ctl1 |= JEVOIS_DMP_CPASS_CALIBR; m = true; break;
    case 'S': ctl1 |= JEVOIS_DMP_PED_STEPDET; a = true; mec |= JEVOIS_DMP_PEDOMETER_EN; break;
    case 'b': ctl2 |= JEVOIS_DMP_ACCEL_ACCURACY; h2 = true; a = true; break;
    case 'h': ctl2 |= JEVOIS_DMP_GYRO_ACCURACY; h2 = true; g = true; break;
    case 'n': ctl2 |= JEVOIS_DMP_CPASS_ACCURACY; h2 = true; m = true; break;
    case 'P': ctl2 |= JEVOIS_DMP_FLIP_PICKUP; h2 = true; a = true; g = true; mec |= JEVOIS_DMP_FLIP_PICKUP_EN; break;
    case 'T': ctl2 |= JEVOIS_DMP_ACT_RECOG; h2 = true; a = true; g = true;
      mec |= JEVOIS_DMP_SMD_EN | JEVOIS_DMP_BTS_EN | JEVOIS_DMP_PEDOMETER_EN |
        JEVOIS_DMP_BRING_AND_LOOK_T0_SEE_EN; break;
    case 'w': mec |= JEVOIS_DMP_BAC_WEARABLE_EN; break;

    default: LERROR("Phony character '" << c << "' ignored while parsing parameter dmp.");
    }

  // Apply a few dependencies:
  if (a) { h2 = true; ctl2 |= JEVOIS_DMP_ACCEL_ACCURACY; mec |= JEVOIS_DMP_ACCEL_CAL_EN; }
  if (g) { h2 = true; ctl2 |= JEVOIS_DMP_GYRO_ACCURACY; mec |= JEVOIS_DMP_GYRO_CAL_EN; }
  if (m) { h2 = true; ctl2 |= JEVOIS_DMP_CPASS_ACCURACY; mec |= JEVOIS_DMP_COMPASS_CAL_EN; }
  if (h2) ctl1 |= JEVOIS_DMP_HEADER2;
  
  // Set the two control parameters in the IMU:
  writeDMP(DMP_DATA_OUT_CTL1, ctl1);
  writeDMP(DMP_DATA_OUT_CTL2, ctl2);
  writeDMP(DMP_FIFO_WATERMARK, 800);

  // Setup the DMP:
  writeDMP(DMP_DATA_INTR_CTL, ctl1);
  writeDMP(DMP_MOTION_EVENT_CTL, mec);
  writeDMP(DMP_DATA_RDY_STATUS, (a ? 0x02 : 0x00) | (g ? 0x01 : 0x00) | (m ? 0x08 : 0x00));
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

