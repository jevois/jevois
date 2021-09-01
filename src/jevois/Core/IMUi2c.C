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

#include <jevois/Core/IMUi2c.H>
#include <jevois/Core/ICM20948_regs.H>
#include <jevois/Core/Camera.H>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>

namespace
{
  struct jevois_data
  {
      unsigned char addr;
      unsigned char size;
      unsigned char data[32];
  };
}

// ####################################################################################################
jevois::IMUi2c::IMUi2c(std::shared_ptr<Camera> cam) :
    jevois::IMU(), itsCam(cam), itsIMUbank(0xff)
{ }

// ####################################################################################################
jevois::IMUi2c::~IMUi2c()
{ }

// ####################################################################################################
bool jevois::IMUi2c::isSPI() const
{ return false; }

// ####################################################################################################
void jevois::IMUi2c::selectBank(int fd, unsigned short reg)
{
  uint8_t const bank = (reg >> 7) & 0x03;
  if (itsIMUbank == bank) return;
  
  unsigned short data[2] = { ICM20948_REG_BANK_SEL, static_cast<unsigned short>(bank << 4) };

  LDEBUG("Writing 0x" << std::hex << data[1] << " to 0x" << data[0]);
  XIOCTL(fd, _IOW('V', 194, int), data);

  itsIMUbank = bank;
}

// ##############################################################################################################
void jevois::IMUi2c::writeRegister(unsigned short reg, unsigned char val)
{
  int fd = itsCam->lock();
  try
  {
    selectBank(fd, reg);

    unsigned short data[2] = { static_cast<unsigned short>(reg & 0x7f), val };
    
    LDEBUG("Writing 0x" << std::hex << val << " to 0x" << reg);
    XIOCTL(fd, _IOW('V', 194, int), data);
  } catch (...) { }
  
  itsCam->unlock();
}

// ##############################################################################################################
unsigned char jevois::IMUi2c::readRegister(unsigned short reg)
{
  int fd = itsCam->lock();
  unsigned short data[2] = { static_cast<unsigned short>(reg & 0x7f), 0 };
  try
  {
    selectBank(fd, reg);
    
    XIOCTL(fd, _IOWR('V', 195, int), data);
    LDEBUG("Register 0x" << std::hex << reg << " has value 0x" << data[1]);
  } catch (...) { }
  
  itsCam->unlock();
  return data[1];
}

// ##############################################################################################################
void jevois::IMUi2c::writeRegisterArray(unsigned short reg, unsigned char const * vals, size_t num)
{
  if (num > 32) LFATAL("Maximum allowed size is 32 bytes. You must break down larger transfers into 32 byte chunks.");

  int fd = itsCam->lock();
  try
  {
    selectBank(fd, reg);

    static jevois_data d;
    d.addr = reg & 0x7f;
    d.size = num;
    memcpy(d.data, vals, num);
    
    LDEBUG("Writing " << num << " values to 0x"<< std::hex << reg);
    XIOCTL(fd, _IOW('V', 196, struct jevois_data), &d);
  } catch (...) { }
  
  itsCam->unlock();
}

// ##############################################################################################################
void jevois::IMUi2c::readRegisterArray(unsigned short reg, unsigned char * vals, size_t num)
{
  if (num > 32) LFATAL("Maximum allowed size is 32 bytes. You must break down larger transfers into 32 byte chunks.");

  int fd = itsCam->lock();
  try
  {
    selectBank(fd, reg);

    static jevois_data d;
    d.addr = reg & 0x7f;
    d.size = num;

    XIOCTL(fd, _IOWR('V', 197, struct jevois_data), &d);
    LDEBUG("Received " << num <<" values from register 0x" << std::hex << reg);
    memcpy(vals, d.data, num);
  } catch (...) { }
  
  itsCam->unlock();
}

