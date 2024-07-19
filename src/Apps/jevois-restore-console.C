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

#include <jevois/Util/Console.H>
#include <jevois/Debug/Log.H>
#include <linux/input.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cctype>

//! Restore console operation after a violent crash of jevoispro-daemon
/*! This utility is useful when debugging jevoispro-daemon on JeVois-Pro platform, as it mutes the keyboard and console
    framebuffer for direct access to the framebuffer device and keyboard/mouse/etc. */
int main(int, char const **)
{
  int ret = 0;
  jevois::logLevel = LOG_CRIT;

#ifdef JEVOIS_PRO
  try
  {
    int cfd = jevois::getConsoleFd();
    ioctl(cfd, KDSETMODE, KD_TEXT);
    close(cfd);
  }
  catch (...) { ++ret; }
  
  try
  {
    jevois::unMuteKeyboard(STDIN_FILENO, K_UNICODE);
  }
  catch (...)
  {
    // stdin is not a tty, probably we were launched remotely, so we try to disable the active tty:
    try
    {
      int tty = jevois::getActiveTTY();
      jevois::unMuteKeyboard(tty, K_UNICODE);
      close(tty);
    }
    catch (...) { ret += 2; }
  }
#endif

  // Terminate logger:
  jevois::logEnd();

  return ret;
}
