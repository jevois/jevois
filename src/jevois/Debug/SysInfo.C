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
#include <jevois/Debug/Log.H>
#include <fstream>
#include <algorithm>

namespace
{
  std::string getFileString(char const * fname, int skip = 0)
  {
    std::ifstream ifs(fname);
    if (ifs.is_open() == false) LFATAL("Cannot read file: " << fname);
    std::string str;
    while (skip-- >= 0) std::getline(ifs, str);
    ifs.close();
    
    return str;
  }
}

// ####################################################################################################
std::string jevois::getSysInfoCPU()
{
  int freq = std::stoi(getFileString("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq")) / 1000;
  int temp = std::stoi(getFileString("/sys/class/thermal/thermal_zone0/temp"));

  // On some hosts, temp is in millidegrees:
  if (temp > 200) temp /= 1000;
  
  std::string load = getFileString("/proc/loadavg");
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
  std::string memtotal = getFileString("/proc/meminfo"); cleanSpaces(memtotal);
  std::string memfree = getFileString("/proc/meminfo", 1); cleanSpaces(memfree);
  return memtotal + ", " + memfree;
}

// ####################################################################################################
std::string jevois::getSysInfoVersion()
{
  std::string ver = getFileString("/proc/version");

  // Truncate at "Linux Version XXX":
  size_t pos = ver.find(' ');
  pos = ver.find(' ', pos + 1);
  pos = ver.find(' ', pos + 1);

  return ver.substr(0, pos);
}
