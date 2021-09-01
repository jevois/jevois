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

#include <jevois/Core/IMU.H>
#include <jevois/Core/ICM20948_regs.H>
#include <jevois/Debug/Log.H>
#include <thread>

// Icm20948 device requires a DMP image to be loaded on init
static unsigned char const dmp3_image[] = {
#include "ICM20948_dmp3a.H"
};

// ####################################################################################################
jevois::IMU::IMU()
{ }

// ####################################################################################################
jevois::IMU::~IMU()
{ }

// ####################################################################################################
void jevois::IMU::loadDMPfirmware(bool verify)
{
  unsigned char currbank = 0xff;
  
  LINFO("Loading ICM20948 DMP firmware...");

  unsigned short addr = DMP_LOAD_START; size_t chunksiz = DMP_MEM_BANK_SIZE - (addr % DMP_MEM_BANK_SIZE);
  for (size_t i = 0; i < sizeof(dmp3_image); i += chunksiz)
  {
    // Select DMP memory bank if it changed (banks have DMP_MEM_BANK_SIZE bytes):
    unsigned char const bank = addr / DMP_MEM_BANK_SIZE;
    if (bank != currbank) { writeRegister(ICM20948_REG_MEM_BANK_SEL, bank); currbank = bank; }

    // Write load address:
    writeRegister(ICM20948_REG_MEM_START_ADDR, addr & 0xff);

    LDEBUG("   Writing " << chunksiz << " bytes to MEMS addr 0x" << std::hex << addr);

    // Then write the data into REG_MEM_R_W:
    if (addr + chunksiz > sizeof(dmp3_image)) chunksiz = sizeof(dmp3_image) - addr;
    writeRegisterArray(ICM20948_REG_MEM_R_W, &dmp3_image[i], chunksiz);

    addr += chunksiz; chunksiz = DMP_MEM_BANK_SIZE;
  }

  if (verify)
  {
    LINFO("Verifying ICM20948 DMP firmware...");
    
    // Read the data back to verify:
    addr = DMP_LOAD_START; currbank = 0xff; unsigned char buf[DMP_MEM_BANK_SIZE];
    chunksiz = DMP_MEM_BANK_SIZE - (addr % DMP_MEM_BANK_SIZE);
    for (size_t i = 0; i < sizeof(dmp3_image); i += chunksiz)
    {
      // Select DMP memory bank if it changed (banks have DMP_MEM_BANK_SIZE bytes):
      unsigned char const bank = addr / DMP_MEM_BANK_SIZE;
      if (bank != currbank) { writeRegister(ICM20948_REG_MEM_BANK_SEL, bank); currbank = bank; }

      // Write load address:
      writeRegister(ICM20948_REG_MEM_START_ADDR, addr & 0xff);

      LDEBUG("   Reading " << chunksiz << " bytes from MEMS addr 0x" << std::hex << addr);
  
      // Then read the data from REG_MEM_R_W:
      if (addr + chunksiz > sizeof(dmp3_image)) chunksiz = sizeof(dmp3_image) - addr;
      readRegisterArray(ICM20948_REG_MEM_R_W, buf, chunksiz);
      for (size_t j = 0; j < chunksiz; ++j)
        if (buf[j] != dmp3_image[i + j])
          LERROR("DMP code verify error addr=" << std::hex << std::showbase << addr + j << ", read=" << buf[j] <<
                 ", orig=" << dmp3_image[i + j]);
      
      addr += chunksiz; chunksiz = DMP_MEM_BANK_SIZE;
    }
  }

  // Get back to MEMS bank 0:
  writeRegister(ICM20948_REG_MEM_BANK_SEL, 0);
  
  // Set the DMP start address:
  unsigned char dmp_addr[2] = { (DMP_START_ADDRESS >> 8) & 0xff, DMP_START_ADDRESS & 0xff };
  writeRegisterArray(ICM20948_REG_PRGM_START_ADDRH, &dmp_addr[0], 2);

  LINFO("Loaded " << sizeof(dmp3_image) << " bytes of DMP firmware.");
  // User code will actually enable the DMP if desired.
}

// ####################################################################################################
void jevois::IMU::writeDMPregister(unsigned short reg, unsigned short val)
{
  // Write the data in big endian:
  unsigned char data[2];
  data[0] = val >> 8;
  data[1] = val & 0xff;
  
  writeDMPregisterArray(reg, &data[0], 2);
}

// ####################################################################################################
void jevois::IMU::writeDMPregisterArray(unsigned short reg, unsigned char const * vals, size_t num)
{
  // Select MEMs bank from the 8 MSBs of reg:
  writeRegister(ICM20948_REG_MEM_BANK_SEL, reg >> 8);

  // Set address:
  writeRegister(ICM20948_REG_MEM_START_ADDR, reg & 0xff);

  // Write data:
  writeRegisterArray(ICM20948_REG_MEM_R_W, vals, num);
}

// ####################################################################################################
unsigned short jevois::IMU::readDMPregister(unsigned short reg)
{
  // Read data in big endian:
  unsigned char data[2];
  readDMPregisterArray(reg, &data[0], 2);

  // Return it in little endian:
  return (data[0] << 8) | data[1];
}

// ####################################################################################################
void jevois::IMU::readDMPregisterArray(unsigned short reg, unsigned char * vals, size_t num)
{
  // Select MEMs bank from the 8 MSBs of reg:
  writeRegister(ICM20948_REG_MEM_BANK_SEL, reg >> 8);

  // Set address:
  writeRegister(ICM20948_REG_MEM_START_ADDR, reg & 0xff);
  
  // Write data:
  readRegisterArray(ICM20948_REG_MEM_R_W, vals, num);
}
