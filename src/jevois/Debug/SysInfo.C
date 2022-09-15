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

#include <jevois/Debug/SysInfo.H>
#include <jevois/Util/Utils.H>
#include <fstream>
#include <algorithm>

// ####################################################################################################
std::string jevois::getSysInfoCPU()
{
#ifdef JEVOIS_PLATFORM_PRO
  // One JeVois Pro, use cpu 2 (big core) and thermal zone 1:
  int freq = 2208;
  try { freq = std::stoi(jevois::getFileString("/sys/devices/system/cpu/cpu2/cpufreq/scaling_cur_freq")) / 1000; }
  catch (...) { } // silently ignore any errors
  
  int temp = 30;
  try { temp = std::stoi(jevois::getFileString("/sys/class/thermal/thermal_zone1/temp")); }
  catch (...) { } // silently ignore any errors
#else
  int freq = 1344;
  try { freq = std::stoi(jevois::getFileString("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq")) / 1000; }
  catch (...) { } // silently ignore any errors
  
  int temp = 30;
  try { temp = std::stoi(jevois::getFileString("/sys/class/thermal/thermal_zone0/temp")); }
  catch (...) { } // silently ignore any errors
#endif
  
  // On some hosts, temp is in millidegrees:
  if (temp > 200) temp /= 1000;
  
  std::string load = jevois::getFileString("/proc/loadavg");
  return "CPU: " + std::to_string(freq) + "MHz, " + std::to_string(temp) + "C, load: " + load;
}

// ####################################################################################################
namespace
{
  // from http://stackoverflow.com/questions/8362094/replace-multiple-spaces-with-one-space-in-a-string
  bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }

  void cleanSpaces(std::string & str)
  {
    std::string::iterator new_end = std::unique(str.begin(), str.end(), BothAreSpaces);
    str.erase(new_end, str.end());   
  }
}

// ####################################################################################################
std::string jevois::getSysInfoMem()
{
  std::string memtotal = jevois::getFileString("/proc/meminfo"); cleanSpaces(memtotal);
  std::string memfree = jevois::getFileString("/proc/meminfo", 1); cleanSpaces(memfree);
  return memtotal + ", " + memfree;
}

// ####################################################################################################
std::string jevois::getSysInfoVersion()
{
  std::string ver = jevois::getFileString("/proc/version");

  // Truncate at "Linux Version XXX":
  size_t pos = ver.find(' ');
  pos = ver.find(' ', pos + 1);
  pos = ver.find(' ', pos + 1);

  return ver.substr(0, pos);
}

// ####################################################################################################
size_t jevois::getNumInstalledTPUs()
{
  size_t n = 0;

  // First detect any PCIe accelerators:
  while (true)
  {
    try { jevois::getFileString(("/sys/class/apex/apex_" + std::to_string(n) + "/temp").c_str()); }
    catch (...) { break; }
    ++n;
  }

  // Then detect any USB accelerators:
  // Note: reported name and USB ID in lsusb changes once we start using the device...
  // Before running a network: ID 1a6e:089a Global Unichip Corp. 
  // After running a network: ID 18d1:9302 Google Inc. 

  try { n += std::stoi(jevois::system("/usr/bin/lsusb | /usr/bin/grep 1a6e:089a | /usr/bin/wc -l")); } catch (...) { }

  try { n += std::stoi(jevois::system("/usr/bin/lsusb | /usr/bin/grep 18d1:9302 | /usr/bin/wc -l")); } catch (...) { }

  return n;
}

// ####################################################################################################
size_t jevois::getNumInstalledVPUs()
{
  // Note: reported name and USB ID in lsusb changes once we start using the device...
  // Before running a network: ID 03e7:2485 Intel Movidius MyriadX
  // After running a network: ID 03e7:f63b Myriad VPU [Movidius Neural Compute Stick]
  size_t n = 0;

  try { n += std::stoi(jevois::system("/usr/bin/lsusb | /usr/bin/grep 03e7:2485 | /usr/bin/wc -l")); } catch (...) { }

  try { n += std::stoi(jevois::system("/usr/bin/lsusb | /usr/bin/grep 03e7:f63b | /usr/bin/wc -l")); } catch (...) { }

  return n;
}

// ####################################################################################################
size_t jevois::getNumInstalledNPUs()
{
  // Note: could also check /proc/cpuinfo to get the NPU hardware version
#ifdef JEVOIS_PLATFORM_PRO
  return 1;
#else
  return 0;
#endif
}

// ####################################################################################################
size_t jevois::getNumInstalledSPUs()
{
  size_t n = 0;

  while (true)
  {
    try { jevois::getFileString(("/sys/class/hailo_chardev/hailo" + std::to_string(n) + "/device_id").c_str()); }
    catch (...) { break; }
    ++n;
  }

  return n;
}

// ####################################################################################################
int jevois::getFanSpeed()
{
#ifdef JEVOIS_PLATFORM_PRO
  try
  {
    int period = std::stoi(jevois::getFileString("/sys/class/pwm/pwmchip8/pwm0/period"));
    int duty = std::stoi(jevois::getFileString("/sys/class/pwm/pwmchip8/pwm0/duty_cycle"));
    if (period == 0) return 100;
    return 100 * duty / period;
  }
  catch (...) { }
#endif

  return 0;
}
