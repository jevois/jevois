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

#ifdef JEVOIS_PRO

#include <jevois/Core/IMU.H>
#include <string>

namespace jevois
{
  //! IMU with SPI interface, such as the ICM20948 IMU on the JeVois-Pro IMX290 camera sensor board
  class IMUspi : public IMU
  {
    public:
      //! Constructor
      IMUspi(std::string const & devname);

      //! Destructor
      virtual ~IMUspi();

      //! Returns true if we use SPI for transfers. Used when ICM20948_REG_USER_CTRL is written to
      virtual bool isSPI() const override;

      //! @name Access functions for IMU registers
      //! @{

      //! Write a value to one of the IMU registers
      /*! See base class jevois::IMU for full docs. */
      void writeRegister(unsigned short reg, unsigned char val) override;

      //! Read a value from one of the camera's IMU registers
      /*! See base class jevois::IMU for full docs. */
      unsigned char readRegister(unsigned short reg) override;

      //! Write an array of values to the camera's IMU registers
      /*! See base class jevois::IMU for full docs. */
      void writeRegisterArray(unsigned short reg, unsigned char const * vals, size_t num) override;

      //! Read an array of values from the camera's IMU registers
      /*! See base class jevois::IMU for full docs. */
      void readRegisterArray(unsigned short reg, unsigned char * vals, size_t num) override;

      //! @}
      
    protected:
      std::string const itsDevName;
      int itsFd;
      void selectBank(unsigned short reg);
      uint8_t itsIMUbank;
      void spi_xfer(unsigned char addr, unsigned char dir, size_t siz,
                    unsigned char * datain, unsigned char const * dataout);
  };

} // namespace jevois

#endif // JEVOIS_PRO
