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

#include <jevois/Core/Camera.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Core/VideoMapping.H>
#include <jevois/Image/RawImageOps.H>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

#define ISP_META_WIDTH 272
#define ISP_META_HEIGHT 1

// ##############################################################################################################
jevois::Camera::Camera(std::string const & devname, jevois::CameraSensor s, unsigned int const nbufs) :
    jevois::VideoInput(devname, nbufs), itsSensor(s)
{
  JEVOIS_TRACE(1);

  JEVOIS_TIMED_LOCK(itsMtx);
  
#ifdef JEVOIS_PLATFORM_A33
  // Get the sensor flags (if supported, currently only JeVois-A33 platform):
  itsFlags = readFlags();
  LDEBUG("Sensor " << s << (itsFlags & JEVOIS_SENSOR_MONO) ? " Monochrome" : " Color" <<
         (itsFlags & JEVOIS_SENSOR_ICM20948) ? " with ICM20948 IMU" : " ");
#endif
}

#ifdef JEVOIS_PLATFORM_PRO
// ##############################################################################################################
void jevois::Camera::setFormat(jevois::VideoMapping const & m)
{
  JEVOIS_TRACE(2);

  JEVOIS_TIMED_LOCK(itsMtx);

  // Destroy our devices, if any, in reverse order of creation:
  while (itsDev.empty() == false) itsDev.pop_back();

  // Load sensor preset sequence if needed, get the native sensor capture dims for requested format:
  unsigned int capw = m.cw, caph = m.ch; int preset = -1;
  jevois::sensorPrepareSetFormat(itsSensor, m, capw, caph, preset);
  std::string pstr; if (preset != -1) pstr = " [preset " + std::to_string(preset) + ']';
  LINFO(itsSensor << ": using native video capture size " << capw << 'x' << caph << pstr << " + crop/resize as needed");

  // Crop or scale or both:
  switch (m.crop)
  {
  case jevois::CropType::Scale:
  {
    // Start 3 streams to get the scaled image in the third one. If desired capture size is the same as native sensor
    // size, revert to single-stream crop mode instead of scale:
    if (capw != m.cw || caph != m.ch)
    {
      LINFO("Capture: " << capw << 'x' << caph << ", rescale to " << m.cw << 'x' << m.ch);

      // Open 3 devices: raw frame, meta data, scaled frame, and set their formats:
      itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, 2, true)); // raw frame
      itsDev.back()->setFormat(m.cfmt, capw, caph, m.cfps, m.cw, m.ch, preset);
      
      itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, 2, true)); // metadata
      itsDev.back()->setFormat(ISP_V4L2_PIX_FMT_META, ISP_META_WIDTH, ISP_META_HEIGHT,
                               0.0F, ISP_META_WIDTH, ISP_META_HEIGHT);
      
      itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, itsNbufs, false)); // DS1 frame
      itsDev.back()->setFormat(m.cfmt, m.cw, m.ch, 0.0F, m.cw, m.ch);
      
      itsFd = itsDev.back()->getFd(); itsDevIdx = itsDev.size() - 1;
      itsFd2 = -1; itsDev2Idx = -1;
      break;
    }
  }
  // fall-through ...
  
  case jevois::CropType::Crop:
  {
    if (capw == m.cw && caph == m.ch) LINFO("Capture: " << capw << 'x' << caph);
    else LINFO("Capture: " << capw << 'x' << caph << ", crop to " << m.cw << 'x' << m.ch);

    // Open one device: raw frame
    itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, itsNbufs, false));
    itsDev.back()->setFormat(m.cfmt, capw, caph, m.cfps, m.cw, m.ch, preset);

    itsFd = itsDev.back()->getFd(); itsDevIdx = itsDev.size() - 1;
    itsFd2 = -1; itsDev2Idx = -1;
  }
  break;
  
  case jevois::CropType::CropScale:
  {
    LINFO("Capture: " << capw << 'x' << caph << ", plus ISP scaled to " << m.c2w << 'x' << m.c2h);

    // Open 3 devices: raw frame, meta data, scaled frame, and set their formats:
    itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, 2, false)); // raw frame
    itsDev.back()->setFormat(m.cfmt, capw, caph, m.cfps, m.cw, m.ch, preset);

    itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, 2, true)); // metadata
    itsDev.back()->setFormat(ISP_V4L2_PIX_FMT_META, ISP_META_WIDTH, ISP_META_HEIGHT,
                             0.0F, ISP_META_WIDTH, ISP_META_HEIGHT);

    itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, itsNbufs, false)); // DS1 frame
    itsDev.back()->setFormat(m.c2fmt, m.c2w, m.c2h, m.cfps, m.c2w, m.c2h);

    itsFd = itsDev.front()->getFd(); itsDevIdx = 0;
    itsFd2 = itsDev.back()->getFd(); itsDev2Idx = itsDev.size() - 1;
  }
  break;

  default: LFATAL("Invalid crop type: " << int(m.crop));
  }
}

#else // JEVOIS_PLATFORM_PRO
// ##############################################################################################################
void jevois::Camera::setFormat(jevois::VideoMapping const & m)
{
  JEVOIS_TRACE(2);

  JEVOIS_TIMED_LOCK(itsMtx);

  // Destroy our devices, if any, in reverse order of creation:
  while (itsDev.empty() == false) itsDev.pop_back();
  
  // Open one device: raw frame, no cropping or scaling supported:
  itsDev.push_back(std::make_shared<jevois::CameraDevice>(itsDevName, itsNbufs, false));
  itsDev.back()->setFormat(m.cfmt, m.cw, m.ch, m.cfps, m.cw, m.ch);
  itsFd = itsDev.back()->getFd(); itsDevIdx = itsDev.size() - 1;
  itsFd2 = -1; itsDev2Idx = -1;
}
#endif // JEVOIS_PLATFORM_PRO

// ##############################################################################################################
jevois::Camera::~Camera()
{
  JEVOIS_TRACE(1);

  // Turn off streaming if it was on:
  try { streamOff(); } catch (...) { jevois::warnAndIgnoreException(); }

  // Close all devices:
  itsDev.clear();
}

// ##############################################################################################################
void jevois::Camera::streamOn()
{
  JEVOIS_TRACE(2);
  JEVOIS_TIMED_LOCK(itsMtx);

  for (auto & dev : itsDev) try { dev->streamOn(); } catch (...) { jevois::warnAndIgnoreException(); }
}

// ##############################################################################################################
void jevois::Camera::abortStream()
{
  JEVOIS_TRACE(2);
  JEVOIS_TIMED_LOCK(itsMtx);

  for (auto & dev : itsDev) try { dev->abortStream(); } catch (...) { jevois::warnAndIgnoreException(); }
}

// ##############################################################################################################
void jevois::Camera::streamOff()
{
  JEVOIS_TRACE(2);
  JEVOIS_TIMED_LOCK(itsMtx);

  for (auto & dev : itsDev) try { dev->streamOff(); } catch (...) { jevois::warnAndIgnoreException(); }
}

// ##############################################################################################################
void jevois::Camera::get(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsDevIdx == -1) LFATAL("Need to call setFormat() first");
  itsDev[itsDevIdx]->get(img);
}

// ##############################################################################################################
bool jevois::Camera::hasScaledImage() const
{
  return (itsDev2Idx != -1);
}

// ##############################################################################################################
void jevois::Camera::get2(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);
  JEVOIS_TIMED_LOCK(itsMtx);

  if (itsDev2Idx == -1) LFATAL("No JeVois Pro Platform ISP-scaled image available");
  itsDev[itsDev2Idx]->get(img);
}

// ##############################################################################################################
void jevois::Camera::done(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsDevIdx == -1) LFATAL("Need to call setFormat() first");
  itsDev[itsDevIdx]->done(img);
}

// ##############################################################################################################
void jevois::Camera::done2(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsDev2Idx == -1) LFATAL("No JeVois Pro Platform ISP-scaled image available");
  itsDev[itsDev2Idx]->done(img);
}

// ##############################################################################################################
void jevois::Camera::queryControl(struct v4l2_queryctrl & qc) const
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");
  XIOCTL_QUIET(itsFd, VIDIOC_QUERYCTRL, &qc);
}

// ##############################################################################################################
void jevois::Camera::queryMenu(struct v4l2_querymenu & qm) const
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");
  XIOCTL_QUIET(itsFd, VIDIOC_QUERYMENU, &qm);
}

// ##############################################################################################################
void jevois::Camera::getControl(struct v4l2_control & ctrl) const
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");
#ifdef JEVOIS_PLATFORM_A33
  XIOCTL_QUIET(itsFd, 0xc00c561b /* should be VIDIOC_G_CTRL, bug in kernel headers? */, &ctrl);
#else
  XIOCTL_QUIET(itsFd, VIDIOC_G_CTRL, &ctrl);
#endif
}

// ##############################################################################################################
void jevois::Camera::setControl(struct v4l2_control const & ctrl)
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");
#ifdef JEVOIS_PLATFORM_A33
  XIOCTL_QUIET(itsFd, 0xc00c561c /* should be VIDIOC_S_CTRL, bug in kernel headers? */, &ctrl);
#else
  XIOCTL_QUIET(itsFd, VIDIOC_S_CTRL, &ctrl);
#endif  
}

// ##############################################################################################################
void jevois::Camera::writeRegister(unsigned short reg, unsigned short val)
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");

#ifdef JEVOIS_PRO
  std::ofstream ofs("/sys/devices/platform/sensor/sreg");
  ofs << jevois::sformat("w %x %x\n", reg, val);
#else
  unsigned short data[2] = { reg, val };
  LDEBUG("Writing 0x" << std::hex << val << " to 0x" << reg);
  XIOCTL(itsFd, _IOW('V', 192, int), data);
#endif
}

// ##############################################################################################################
unsigned short jevois::Camera::readRegister(unsigned short reg)
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");
#ifdef JEVOIS_PRO
  {
    std::ofstream ofs("/sys/devices/platform/sensor/sreg");
    ofs << jevois::sformat("r %x\n", reg);
  }
  std::ifstream ifs("/sys/devices/platform/sensor/sreg");
  unsigned short val; ifs >> std::hex >> val;
  return val;
#else
  unsigned short data[2] = { reg, 0 };
  XIOCTL(itsFd, _IOWR('V', 193, int), data);
  LDEBUG("Register 0x" << std::hex << reg << " has value 0x" << data[1]);
  return data[1];
#endif
}

// ##############################################################################################################
int jevois::Camera::lock()
{
  itsMtx.lock();
  if (itsFd == -1) LFATAL("Not initialized");
  return itsFd;
}

// ##############################################################################################################
void jevois::Camera::unlock()
{
  itsMtx.unlock();
}

// ##############################################################################################################
jevois::Camera::Flags jevois::Camera::readFlags()
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsFd == -1) LFATAL("Not initialized");

  int data;
  try { XIOCTL(itsFd, _IOWR('V', 198, int), &data); } catch (...) { return jevois::Camera::JEVOIS_SENSOR_COLOR; }
  return Flags(data);
}

