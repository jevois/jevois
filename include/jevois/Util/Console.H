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

namespace jevois
{
  /*! \defgroup console Console access utilities
      
      Console-related utility and helper functions. \ingroup utils */
  
  /*! @{ */ // **********************************************************************

  //! Indicate whether this fd (which should be from /dev/input/eventX) is keyboard, mouse, etc
  /*! Inspired from SFML-rpi */
  bool isInputDevice(int fd);
  
  //! Get a file descriptor to the console
  int getConsoleFd();

  //! Get current active tty
  /*! Reads /sys/class/tty/tty0/active and open the tty. Caller should close that fd later using close(). */
  int getActiveTTY();
  
  //! Prevent keystrokes from reaching the tty
  /*! Useful when doing direct drawing into the linux framebuffer, as in ImGuiBackendMALI. Returned kb_mode is what it
      was, use it in unMuteKeyboard to restore. */
  void muteKeyboard(int tty, int & kb_mode);

  //! Restore the keyboard mode for given tty
  /*! Use kd_mode that was returned by muteKeyboard(). */
  void unMuteKeyboard(int tty, int kb_mode);

  /*! @} */ // **********************************************************************
  
} // namespace jevois

#endif // JEVOIS_PRO
