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

#pragma once

#include <jevois/Core/UserInterface.H>
#include <thread>
#include <mutex>
#include <atomic>

namespace jevois
{
  //! String-based user interface, simple terminal input/output to use on host
  /*! When compiling JeVois code on the host, the hardware serial port and serial-over-usb ports will not be
      available. Instead of these two, the Engine will use a single StdioInterface which reads/writes strings from
      standard input/output of the terminal in which jevois-daemon was started. \ingroup core */
  class StdioInterface : public UserInterface
  {
    public:
      //! Constructor
      StdioInterface(std::string const & instance);

      //! Destructor
      virtual ~StdioInterface();
      
      //! Read some bytes if available, and return true and a string when one is complete
      bool readSome(std::string & str) override;
      
      //! Write a string, using the line termination convention of serial::linestyle
      /*! No line terminator should be included in the string, writeString() will add one. */
      void writeString(std::string const & str) override;

      //! Return our port type, here always Stdio
      UserInterface::Type type() const override;

    private:
      std::string itsString;
      std::thread itsThread;
      std::atomic<bool> itsRunning;
      std::mutex itsMtx;
  };
} // namespace jevois
