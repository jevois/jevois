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

#ifdef JEVOIS_PRO

#include <jevois/Core/IMUspi.H>
#include <jevois/Core/ICM20948_regs.H>
#include <linux/spi/spidev.h>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>
#include <thread>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// ####################################################################################################
jevois::IMUspi::IMUspi(std::string const & devname) :
    jevois::IMU(), itsDevName(devname), itsFd(-1), itsIMUbank(0)
{
  // FIXME: clock speed seems to have no effect on whether we can load the DMP data or not.
  // So setting retry to 1 for now, trying 7MHz only.
  int retry = 1; // number of tries, halving the speed each time
  unsigned int speed = 7000000;

  while (retry)
  {
    try
    {
      itsFd = open(devname.c_str(), O_RDWR);
      if (itsFd < 0) LFATAL("Error opening IMU SPI device " << devname);

      unsigned char mode = SPI_CPOL | SPI_CPHA; // mode 3 for ICM20948
      XIOCTL(itsFd, SPI_IOC_WR_MODE, &mode);

      unsigned char bits = 8;
      XIOCTL(itsFd, SPI_IOC_WR_BITS_PER_WORD, &bits);

      XIOCTL(itsFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

      // At the beginning, we need to read at least once to get the clock polarity in order:
      readRegister(ICM20948_REG_WHO_AM_I);
      
      // Force a reset. Reset bit will auto-clear:
      writeRegister(ICM20948_REG_PWR_MGMT_1, ICM20948_BIT_H_RESET);
      
      // Disable I2C as we use SPI, reset DMP:
      writeRegister(ICM20948_REG_USER_CTRL, ICM20948_BIT_I2C_IF_DIS | ICM20948_BIT_DMP_RST |
                    ICM20948_BIT_DIAMOND_DMP_RST);
      
      // Check that the ICM20948 is detected:
      auto ret = readRegister(ICM20948_REG_WHO_AM_I);
      if (ret == ICM20948_DEVICE_ID) LINFO("Detected ICM20948 IMU on " << devname << " @ " << speed << "Hz");
      else LFATAL("Failed to detect ICM20948 on " << devname << " @ " << speed << "Hz: device ID=0x" << std::hex <<
                  ret << ", should be 0x" << ICM20948_DEVICE_ID);
      
      // Init sequence for SPI operation:
      writeRegister(ICM20948_REG_USER_CTRL, ICM20948_BIT_I2C_IF_DIS | ICM20948_BIT_I2C_MST_EN | ICM20948_BIT_FIFO_EN);
      writeRegister(ICM20948_REG_PWR_MGMT_1, ICM20948_BIT_CLK_PLL);

      // Upload the DMP firmware:
      loadDMPfirmware(true, (retry > 1));

      // Enable the DMP:
      writeRegister(ICM20948_REG_USER_CTRL, ICM20948_BIT_I2C_IF_DIS | ICM20948_BIT_I2C_MST_EN | ICM20948_BIT_FIFO_EN |
                    ICM20948_BIT_DMP_EN);

      // All good, let's get out of here:
      return;
    }
    catch (...)
    {
      if (itsFd >= 0) close(itsFd);
      --retry;
      speed /= 2;
    }
  }
  LERROR("Giving up trying to setup ICM20948 IMU with DMP -- DMP NOT OPERATIONAL, BASIC IMU MAY WORK OR NOT");
}

// ####################################################################################################
jevois::IMUspi::~IMUspi()
{
  if (itsFd >= 0) close(itsFd);
}

// ####################################################################################################
bool jevois::IMUspi::isSPI() const
{ return true; }

// ####################################################################################################
void jevois::IMUspi::spi_xfer(unsigned char reg, unsigned char dir, size_t siz, unsigned char * datain,
                                unsigned char const * dataout)
{
  unsigned char dreg = (reg & 0x7f) | dir;
  unsigned int speed = 7000000;

  struct spi_ioc_transfer xfer[2] =
    {
     {
      .tx_buf = (unsigned long)&dreg,
      .rx_buf = 0UL,
      .len = 1,
      .speed_hz = speed,
      .delay_usecs = 0,
      .bits_per_word = 8,
      .cs_change = 0,
      .tx_nbits = 8,
      .rx_nbits = 8,
      .word_delay_usecs = 0,
      .pad = 0
     },
     {
      .tx_buf = (unsigned long)dataout,
      .rx_buf = (unsigned long)datain,
      .len = static_cast<unsigned int>(siz),
      .speed_hz = speed,
      .delay_usecs = 0,
      .bits_per_word = 8,
      .cs_change = 0,
      .tx_nbits = 8,
      .rx_nbits = 8,
      .word_delay_usecs = 0,
      .pad = 0
     }
    };
  
  XIOCTL(itsFd, SPI_IOC_MESSAGE(2), xfer);
}

// ####################################################################################################
void jevois::IMUspi::selectBank(unsigned short reg)
{
  uint8_t const bank = (reg >> 7) & 0x03;
  if (itsIMUbank == bank) return;

  uint8_t dataout = uint8_t(bank << 4);
  LDEBUG("Writing 0x" << std::hex << dataout << " to 0x" << ICM20948_REG_BANK_SEL);
  spi_xfer(ICM20948_REG_BANK_SEL, ICM20948_SPI_WRITE, 1, nullptr, &dataout);
  itsIMUbank = bank;
}

// ##############################################################################################################
void jevois::IMUspi::writeRegister(unsigned short reg, unsigned char val)
{
  LDEBUG("Writing 0x" << std::hex << val << " to 0x" << reg);
  selectBank(reg); unsigned char gogo;
  spi_xfer(reg, ICM20948_SPI_WRITE, 1, &gogo/*nullptr*/, &val);

  bool verify = true;
  bool delay = false;
  
  switch (reg)
  {
    // These registers always return 0, do not verify:
  case ICM20948_REG_I2C_MST_CTRL:
  case ICM20948_REG_I2C_SLV4_CTRL:
  case ICM20948_REG_TEMP_CONFIG:
    // These registers auto increment, do not verify:
  case ICM20948_REG_MEM_START_ADDR:
  case ICM20948_REG_MEM_R_W:
  case ICM20948_REG_MEM_BANK_SEL:
    // These registers have autoclear bits, do not verify:
  case ICM20948_REG_USER_CTRL:
  case ICM20948_REG_PWR_MGMT_1:
  case ICM20948_REG_PWR_MGMT_2:
    verify = false;
    delay = true;
    break;
  }

  if (delay) std::this_thread::sleep_for(std::chrono::milliseconds(5));
  
  if (verify)
  {
    unsigned char ret = readRegister(reg);
    if (ret != val)
    {
      LERROR("Read back reg 0x"<<std::hex<<reg<<" returned 0x"<<ret<<" instead of 0x"<<val);

      // Try again:
      spi_xfer(reg, ICM20948_SPI_WRITE, 1, nullptr, &val);

      if (delay) std::this_thread::sleep_for(std::chrono::milliseconds(5));

      ret = readRegister(reg);
      if (ret != val)
        LERROR("RETRY Read back reg 0x"<<std::hex<<reg<<" returned 0x"<<ret<<" instead of 0x"<<val);
      else
        LERROR("RETRY Read back reg 0x"<<std::hex<<reg<<" returned 0x"<<ret<<" -- OK");
    }
  }
}

// ##############################################################################################################
unsigned char jevois::IMUspi::readRegister(unsigned short reg)
{
  selectBank(reg);
  unsigned char datain; unsigned char gogo = 0;
  spi_xfer(reg, ICM20948_SPI_READ, 1, &datain, &gogo/*nullptr*/);
  LDEBUG("Register 0x" << std::hex << reg << " has value 0x" << datain);
  return datain;
}

// ##############################################################################################################
void jevois::IMUspi::writeRegisterArray(unsigned short reg, unsigned char const * vals, size_t num)
{
  if (num > 256) LFATAL("Maximum allowed size 256 bytes. You must break down larger transfers into 256 byte chunks.");
  LDEBUG("Writing " << num << " values to 0x"<< std::hex << reg);
  selectBank(reg);
  spi_xfer(reg, ICM20948_SPI_WRITE, num, nullptr, vals);
}

// ##############################################################################################################
void jevois::IMUspi::readRegisterArray(unsigned short reg, unsigned char * vals, size_t num)
{
  if (num > 256) LFATAL("Maximum allowed size 256 bytes. You must break down larger transfers into 256 byte chunks.");
  selectBank(reg);
  spi_xfer(reg, ICM20948_SPI_READ, num, vals, nullptr);
  LDEBUG("Received " << num <<" values from register 0x" << std::hex << reg);
}

#endif // JEVOIS_PRO
