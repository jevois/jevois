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

#include <jevois/Debug/Log.H>
#include <jevois/Core/IMUdata.H>
#include <jevois/Util/Utils.H>
#include <jevois/Core/ICM20948_regs.H>

// data packet size reg 1
#define JEVOIS_DMP_HEADER_SZ                2
#define JEVOIS_DMP_ACCEL_DATA_SZ            6
#define JEVOIS_DMP_GYRO_DATA_SZ             6
#define JEVOIS_DMP_CPASS_DATA_SZ            6
#define JEVOIS_DMP_ALS_DATA_SZ              8
#define JEVOIS_DMP_QUAT6_DATA_SZ            12
#define JEVOIS_DMP_QUAT9_DATA_SZ            14
#define JEVOIS_DMP_PQUAT6_DATA_SZ           6
#define JEVOIS_DMP_GEOMAG_DATA_SZ           14
#define JEVOIS_DMP_PRESSURE_DATA_SZ         6
#define JEVOIS_DMP_GYRO_BIAS_DATA_SZ        6
#define JEVOIS_DMP_CPASS_CALIBR_DATA_SZ     12
#define JEVOIS_DMP_PED_STEPDET_TIMESTAMP_SZ 4
#define JEVOIS_DMP_FOOTER_SZ                2

// data packet size reg 2
#define JEVOIS_DMP_HEADER2_SZ               2
#define JEVOIS_DMP_ACCEL_ACCURACY_SZ        2
#define JEVOIS_DMP_GYRO_ACCURACY_SZ         2
#define JEVOIS_DMP_CPASS_ACCURACY_SZ        2
#define JEVOIS_DMP_FSYNC_SZ                 2
#define JEVOIS_DMP_FLIP_PICKUP_SZ           2
#define JEVOIS_DMP_ACT_RECOG_SZ             6

// This is always present at end of packet?
#define JEVOIS_DMP_ODR_CNT_GYRO_SZ          2

// ####################################################################################################
size_t jevois::DMPpacketSize(unsigned short ctl1, unsigned short ctl2)
{
  size_t ret = JEVOIS_DMP_HEADER_SZ;
  
  if (ctl1 & JEVOIS_DMP_ACCEL) ret += JEVOIS_DMP_ACCEL_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_GYRO) ret += JEVOIS_DMP_GYRO_DATA_SZ + JEVOIS_DMP_GYRO_BIAS_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_CPASS) ret += JEVOIS_DMP_CPASS_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_ALS) ret += JEVOIS_DMP_ALS_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_QUAT6) ret += JEVOIS_DMP_QUAT6_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_QUAT9) ret += JEVOIS_DMP_QUAT9_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_PQUAT6) ret += JEVOIS_DMP_PQUAT6_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_GEOMAG) ret += JEVOIS_DMP_GEOMAG_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_PRESSURE) ret += JEVOIS_DMP_PRESSURE_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_GYRO_CALIBR) ret += JEVOIS_DMP_GYRO_BIAS_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_CPASS_CALIBR) ret += JEVOIS_DMP_CPASS_CALIBR_DATA_SZ;
  if (ctl1 & JEVOIS_DMP_PED_STEPDET) ret += JEVOIS_DMP_PED_STEPDET_TIMESTAMP_SZ;
  if (ctl1 & JEVOIS_DMP_HEADER2) ret += JEVOIS_DMP_HEADER2_SZ;
  
  if (ctl2 & JEVOIS_DMP_ACCEL_ACCURACY) ret += JEVOIS_DMP_ACCEL_ACCURACY_SZ;
  if (ctl2 & JEVOIS_DMP_GYRO_ACCURACY) ret += JEVOIS_DMP_GYRO_ACCURACY_SZ;
  if (ctl2 & JEVOIS_DMP_CPASS_ACCURACY) ret += JEVOIS_DMP_CPASS_ACCURACY_SZ;
  if (ctl2 & JEVOIS_DMP_FLIP_PICKUP) ret += JEVOIS_DMP_FLIP_PICKUP_SZ;
  if (ctl2 & JEVOIS_DMP_BATCH_MODE_EN) ret += JEVOIS_DMP_ODR_CNT_GYRO_SZ; // is this correct?
  if (ctl2 & JEVOIS_DMP_ACT_RECOG) ret += JEVOIS_DMP_ACT_RECOG_SZ;
  // missing FSYNC

  ret += JEVOIS_DMP_FOOTER_SZ;

  return ret;
}

// ####################################################################################################
void jevois::DMPdata::parsePacket(unsigned char const * packet, size_t siz)
{
  // Parse the headers:
  header1 = (packet[0] << 8) | packet[1];
  size_t off = JEVOIS_DMP_HEADER_SZ; // offset to where we are in parsing the packet so far
  header2 = 0;
  if (header1 & JEVOIS_DMP_HEADER2) { header2 = (packet[off] << 8) | packet[off + 1]; off += JEVOIS_DMP_HEADER2_SZ; }

  // Now decode the various data:
  if (header1 & JEVOIS_DMP_ACCEL)
  {
    accel[0] = (packet[off + 0] << 8) | packet[off + 1];
    accel[1] = (packet[off + 2] << 8) | packet[off + 3];
    accel[2] = (packet[off + 4] << 8) | packet[off + 5];
    off += JEVOIS_DMP_ACCEL_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_GYRO)
  {
    gyro[0] = (packet[off + 0] << 8) | packet[off + 1];
    gyro[1] = (packet[off + 2] << 8) | packet[off + 3];
    gyro[2] = (packet[off + 4] << 8) | packet[off + 5];
    off += JEVOIS_DMP_GYRO_DATA_SZ;
    gbias[0] = (packet[off + 0] << 8) | packet[off + 1];
    gbias[1] = (packet[off + 2] << 8) | packet[off + 3];
    gbias[2] = (packet[off + 4] << 8) | packet[off + 5];
    off += JEVOIS_DMP_GYRO_BIAS_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_CPASS)
  {
    cpass[0] = (packet[off + 0] << 8) | packet[off + 1];
    cpass[1] = (packet[off + 2] << 8) | packet[off + 3];
    cpass[2] = (packet[off + 4] << 8) | packet[off + 5];
    off += JEVOIS_DMP_CPASS_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_ALS)
  {
    // Not supported by InvenSense?
    off += JEVOIS_DMP_ALS_DATA_SZ;
  }
  
  if (header1 & JEVOIS_DMP_QUAT6)
  {
    quat6[0] = (packet[off + 0] << 24) | (packet[off + 1] << 16) | (packet[off + 2] << 8) | packet[off + 3];
    quat6[1] = (packet[off + 4] << 24) | (packet[off + 5] << 16) | (packet[off + 6] << 8) | packet[off + 7];
    quat6[2] = (packet[off + 8] << 24) | (packet[off + 9] << 16) | (packet[off + 10] << 8) | packet[off + 11];
    off += JEVOIS_DMP_QUAT6_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_QUAT9)
  {
    quat9[0] = (packet[off + 0] << 24) | (packet[off + 1] << 16) | (packet[off + 2] << 8) | packet[off + 3];
    quat9[1] = (packet[off + 4] << 24) | (packet[off + 5] << 16) | (packet[off + 6] << 8) | packet[off + 7];
    quat9[2] = (packet[off + 8] << 24) | (packet[off + 9] << 16) | (packet[off + 10] << 8) | packet[off + 11];
    quat9acc = (packet[off + 12] << 24) | (packet[off + 13] << 16);
    off += JEVOIS_DMP_QUAT9_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_PED_STEPDET)
  {
    stepts = (packet[off + 0] << 24) | (packet[off + 1] << 16) | (packet[off + 2] << 8) | packet[off + 3];
    steps = header1 & JEVOIS_DMP_PED_STEPIND;
    if (steps == 0) steps = 1; // FIXME: docs say we should get the number of steps, but it is always 0...
    off += JEVOIS_DMP_PED_STEPDET_TIMESTAMP_SZ;
  }

  if (header1 & JEVOIS_DMP_GEOMAG)
  {
    geomag[0] = (packet[off + 0] << 24) | (packet[off + 1] << 16) | (packet[off + 2] << 8) | packet[off + 3];
    geomag[1] = (packet[off + 4] << 24) | (packet[off + 5] << 16) | (packet[off + 6] << 8) | packet[off + 7];
    geomag[2] = (packet[off + 8] << 24) | (packet[off + 9] << 16) | (packet[off + 10] << 8) | packet[off + 11];
    geomagacc = (packet[off + 12] << 24) | (packet[off + 13] << 16);
    off += JEVOIS_DMP_GEOMAG_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_PRESSURE)
  {
    // Not supported by InvenSense?
    off += JEVOIS_DMP_PRESSURE_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_GYRO_CALIBR)
  {
    gyrobias[0] = (packet[off + 0] << 8) | packet[off + 1];
    gyrobias[1] = (packet[off + 2] << 8) | packet[off + 3];
    gyrobias[2] = (packet[off + 4] << 8) | packet[off + 5];
    off += JEVOIS_DMP_GYRO_BIAS_DATA_SZ;
  }

  if (header1 & JEVOIS_DMP_CPASS_CALIBR)
  {
    cpasscal[0] = (packet[off + 0] << 24) | (packet[off + 1] << 16) | (packet[off + 2] << 8) | packet[off + 3];
    cpasscal[1] = (packet[off + 4] << 24) | (packet[off + 5] << 16) | (packet[off + 6] << 8) | packet[off + 7];
    cpasscal[2] = (packet[off + 8] << 24) | (packet[off + 9] << 16) | (packet[off + 10] << 8) | packet[off + 11];
    off += JEVOIS_DMP_CPASS_CALIBR_DATA_SZ;
  }

  if (header2 & JEVOIS_DMP_ACCEL_ACCURACY)
  {
    accelacc = (packet[off + 0] << 8) | packet[off + 1];
    off += JEVOIS_DMP_ACCEL_ACCURACY_SZ;
  }

  if (header2 & JEVOIS_DMP_GYRO_ACCURACY)
  {
    gyroacc = (packet[off + 0] << 8) | packet[off + 1];
    off += JEVOIS_DMP_GYRO_ACCURACY_SZ;
  }

  if (header2 & JEVOIS_DMP_CPASS_ACCURACY)
  {
    cpassacc = (packet[off + 0] << 8) | packet[off + 1];
    off += JEVOIS_DMP_CPASS_ACCURACY_SZ;
  }

  if (header2 & JEVOIS_DMP_FLIP_PICKUP)
  {
    pickup = (packet[off + 0] << 8) | packet[off + 1];
    off += JEVOIS_DMP_FLIP_PICKUP_SZ;
  }

  if (header2 & JEVOIS_DMP_ACT_RECOG)
  {
    bacstate = (packet[off + 0] << 8) | packet[off + 1];
    bacts = (packet[off + 2] << 24) | (packet[off + 3] << 16) | (packet[off + 4] << 8) | packet[off + 5];
    off += JEVOIS_DMP_ACT_RECOG_SZ;
  }

  odrcnt = (packet[off + 0] << 8) | packet[off + 1];
  off += JEVOIS_DMP_FOOTER_SZ;

  if (off != siz) LERROR("Decoded " << off << " bytes from " << siz << " bytes of IMU packet");
}

// ####################################################################################################
std::vector<std::string> jevois::DMPdata::activity()
{
  std::vector<std::string> ret;

  // The activity classifier reports start/stop of activities:
  // bacstate is a set of 2 bytes:
  // - high byte indicates activity start
  // - low byte indicates activity end
  unsigned char bs[2]; bs[0] = bacstate >> 8; bs[1] = bacstate & 0xff;
  unsigned short const mask = DMP_BAC_DRIVE | DMP_BAC_WALK | DMP_BAC_RUN | DMP_BAC_BIKE | DMP_BAC_TILT | DMP_BAC_STILL;
  for (int i = 0; i < 2; ++i)
  {
    std::string const ss = (i == 0) ? "start " : "stop ";
    if (bs[i] & DMP_BAC_DRIVE) ret.push_back(ss + "drive");
    if (bs[i] & DMP_BAC_WALK) ret.push_back(ss + "walk");
    if (bs[i] & DMP_BAC_RUN) ret.push_back(ss + "run");
    if (bs[i] & DMP_BAC_BIKE) ret.push_back(ss + "bike");
    if (bs[i] & DMP_BAC_TILT) ret.push_back(ss + "tilt");
    if (bs[i] & DMP_BAC_STILL) ret.push_back(ss + "still");
    if (bs[i] & ~mask) ret.push_back(ss + jevois::sformat("Unk(0x%02x)", bs[i] & ~mask));
  }
  
  return ret;
}

// ####################################################################################################
std::vector<std::string> jevois::DMPdata::activity2()
{
  std::vector<std::string> ret;
  static unsigned char act = 0; // Activities that currently are ongoing

  // The activity classifier reports start/stop of activities:
  // - high byte indicates activity start
  // - low byte indicates activity end
  unsigned char bs[2]; bs[0] = bacstate >> 8; bs[1] = bacstate & 0xff;

  act |= bs[0]; // Activities that started are turned on
  act &= ~bs[1]; // Activities that stopped are turned off
  
  if (act & DMP_BAC_DRIVE) ret.push_back("drive");
  if (act & DMP_BAC_WALK) ret.push_back("walk");
  if (act & DMP_BAC_RUN) ret.push_back("run");
  if (act & DMP_BAC_BIKE) ret.push_back("bike");
  if (act & DMP_BAC_TILT) ret.push_back("tilt");
  if (act & DMP_BAC_STILL) ret.push_back("still");
  
  return ret;
}

// ####################################################################################################
float jevois::DMPdata::fix2float(long val)
{ return float(val * 1.0 / (1<<30)); }

