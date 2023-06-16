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
#include <jevois/Types/Enum.H>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <mutex>
#include <future>

namespace jevois
{
  namespace serial
  {
    static ParameterCategory const ParamCateg("Serial Port Options");

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(devname, std::string, "Device file name",
                             "", ParamCateg);

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(baudrate, unsigned int, "Baudrate",
                             115200, { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200,
                                 230400, 460800, 921600, 1000000, 1152000, 1500000, 2000000,
                                 2500000, 3000000, 3500000, 4000000 }, ParamCateg);

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(format, std::string, "Data format",
                             "8N1", boost::regex("^[5-8][NEO][12]$"), ParamCateg);

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(flowsoft, bool, "Use soft (XON/XOFF) flow control",
                             false, ParamCateg);

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(flowhard, bool, "Use hard (RTS/CTS) flow control",
                             false, ParamCateg);

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(drop, bool, "Silently drop write data when write buffer is full. Useful to "
			     "avoid blocking when writing messages to serial-over-USB port and the host is "
			     "not listening to it. Note that even when drop is false, we will still drop "
			     "data after repeated attempts to send it, and will report an error (as opposed to "
			     "silently dropping when drop is true).",
                             true, ParamCateg);

    //! Enum for Parameter \relates jevois::Serial
    JEVOIS_DEFINE_ENUM_CLASS(LineStyle, (LF) (CR) (CRLF) (Zero) (Sloppy) );

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(linestyle, LineStyle, "End of line style: LF is for 0x0a [\\n]; CR is for 0x0d [\\r]; "
                             "CRLF is for 0x0d 0x0a [\\r\\n]; Zero is for 0x00 [\\0]; Sloppy accepts any of "
                             "CR, LF, CRLF, 0xd0 (issued by some keyboards instead of Return), and Zero as input "
                             "and issues CRLF in outputs.",
                             LineStyle::Sloppy, LineStyle_Values, ParamCateg);

    //! Enum for Parameter \relates jevois::Serial
    JEVOIS_DEFINE_ENUM_CLASS(TerminalMode, (Plain) (VT100) );

    //! Parameter \relates jevois::Serial
    JEVOIS_DECLARE_PARAMETER(mode, TerminalMode, "Terminal emulation mode for input",
                             TerminalMode::Plain, TerminalMode_Values, ParamCateg);
  } // namespace serial
  
  //! Interface to a serial port
  /*! This class is thread-safe. Concurrent read and write (which do not seem to be supported by the O.S. or hardware)
      are serialized through the use of a mutex in the Serial class. \ingroup core */
  class Serial : public UserInterface,
                 public Parameter<serial::devname, serial::baudrate, serial::format, serial::flowsoft,
                                  serial::flowhard, serial::drop, serial::linestyle, serial::mode>
  {
    public:
      //! Constructor
      Serial(std::string const & instance, UserInterface::Type type);

      //! destructor
      virtual ~Serial();
      
      //! Set the access to blocking or not
      /*! Default is non-blocking. If blocking, can also specify a timeout (rounded to tenth of seconds) */
      void setBlocking(bool blocking, std::chrono::milliseconds const & timeout);
      
      //! Set the DTR mode off momentarily.
      void toggleDTR(std::chrono::milliseconds const & dur);
      
      //! transmit continuous stream of zero-valued bits for specific duration.
      void sendBreak(void);

      //! Read some bytes if available, and return true and a string when one is complete
      /*! This would usually only make sense to use in non-blocking mode. */
      bool readSome(std::string & str) override;
      
      //! Write a string, using the line termination convention of serial::linestyle
      /*! No line terminator should be included in the string, writeString() will add one. */
      void writeString(std::string const & str) override;
      
      //! Send a file from the local microSD to the host computer
      /*! abspath should be the full absolute path of the file. The port will be locked during the entire
          transaction. */
      void fileGet(std::string const & abspath);

      //! Receive a file from the host and write it to the local microSD
      /*! abspath should be the full absolute path of the file. The port will be locked during the entire
          transaction. */
      void filePut(std::string const & abspath);
      
      //! Flush all inputs
      void flush(void);

      //! Return our port type, here Hard or USB
      UserInterface::Type type() const override;

    protected:
      void postInit() override;
      void postUninit() override;

    private:
      void tryReconnect();
      void openPort(); // must be locked
      void writeInternal(void const * buffer, const int nbytes, bool nodrop = false);
      int itsDev; // descriptor associated with the device file
      termios itsSavedState; // saved state to restore in the destructor
      std::string itsPartialString;
      std::mutex itsMtx;
      int itsWriteOverflowCounter; // counter so we do not send too many write overflow errors
      jevois::UserInterface::Type itsType;
      std::atomic<int> itsErrno;
      std::future<void> itsOpenFut;
  };
} // namespace jevois
