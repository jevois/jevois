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

#include <jevois/Component/Component.H>

namespace jevois
{
  //! Abstract base class for a string-based user interface
  /*! Users interact with the JeVois hardware by issuing text commands and receiving text answers. UserInterface is an
      abstract base class for such communications, with derived classes Serial (on the platform hardware, using hardware
      serial and serial-over-usb) or StdioInterface (on host, using user inputs and outputs in the terminal).

      See \ref UserCli for the user documentation of the command-line interface.

      \ingroup core */
  class UserInterface : public Component
  {
    public:
      //! Constructor
      UserInterface(std::string const & instance);

      //! Destructor
      virtual ~UserInterface();
      
      //! Read some bytes if available, and return true and a string when one is complete (RETURN pressed)
      /*! str is untouched if user input is not yet complete (RETURN not yet pressed). The RETURN (end of line) marker
          is not copied into str, only the characters received up to the end of line marker. */
      virtual bool readSome(std::string & str) = 0;
      
      //! Write a string
      /*! No line terminator should be included in the string, writeString() will add one. In the Serial derived class,
          this will be using the line termination convention of serial::linestyle to support different styles (CR, CRLF,
          LF, etc). */
      virtual void writeString(std::string const & str) = 0;

      //! Write a string, with a prefix prepended
      /*! No line terminator should be included in the string, writeString() will add one. In the Serial derived class,
          this will be using the line termination convention of serial::linestyle to support different styles (CR, CRLF,
          LF, etc). */
      virtual void writeString(std::string const & prefix, std::string const & str);

      //! Enum for the interface type
      enum class Type { Hard, USB, Stdio, GUI };

      //! Derived classes must implement this and return their interface type
      virtual Type type() const = 0;
  };
} // namespace jevois
