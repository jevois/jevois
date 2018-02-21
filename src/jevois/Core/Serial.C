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

#include <jevois/Core/Serial.H>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

// ######################################################################
jevois::Serial::Serial(std::string const & instance, jevois::UserInterface::Type type) :
    jevois::UserInterface(instance), itsDev(-1), itsWriteOverflowCounter(0), itsType(type)
{ }

// ######################################################################
void jevois::Serial::postInit()
{
  // Open the port, non-blocking mode by default:
  if (itsDev != -1) ::close(itsDev);
  itsDev = ::open(jevois::serial::devname::get().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (itsDev == -1) LFATAL("Could not open serial port [" << jevois::serial::devname::get() << ']');

  // Save current state
  if (tcgetattr(itsDev, &itsSavedState) == -1) LFATAL("Failed to save current state");

  // reset all the flags
  ////if (fcntl(itsDev, F_SETFL, 0) == -1) LFATAL("Failed to reset flags");

  // Get the current option set:
  termios options = { };
  if (tcgetattr(itsDev, &options) == -1) LFATAL("Failed to get options");

  // get raw input from the port
  options.c_cflag |= ( CLOCAL     // ignore modem control lines
                       | CREAD ); // enable the receiver

  options.c_iflag &= ~(  IGNBRK    // ignore BREAK condition on input
                         | BRKINT  // If IGNBRK is not set, generate SIGINT on BREAK condition, else read BREAK as \0
                         | PARMRK
                         | ISTRIP  // strip off eighth bit
                         | INLCR   // donot translate NL to CR on input
                         | IGNCR   // ignore CR
                         | ICRNL   // translate CR to newline on input
                         | IXON    // disable XON/XOFF flow control on output
                         );

  // disable implementation-defined output processing
  options.c_oflag &= ~OPOST;
  options.c_lflag &= ~(ECHO  // dont echo i/p chars
                       | ECHONL // do not echo NL under any circumstance
                       | ICANON // disable cannonical mode
                       | ISIG   // do not signal for INTR, QUIT, SUSP etc
                       | IEXTEN // disable platform dependent i/p processing
                       );

  // Set the baudrate:
  unsigned int rate;
  switch (jevois::serial::baudrate::get())
	{
    case 4000000: rate = B4000000; break;
    case 3500000: rate = B3500000; break;
    case 3000000: rate = B3000000; break;
    case 2500000: rate = B2500000; break;
    case 2000000: rate = B2000000; break;
    case 1500000: rate = B1500000; break;
    case 1152000: rate = B1152000; break;
    case 1000000: rate = B1000000; break;
    case 921600: rate = B921600; break;
    case 576000: rate = B576000; break;
    case 500000: rate = B500000; break;
    case 460800: rate = B460800; break;
    case 230400: rate = B230400; break;
    case 115200: rate = B115200; break;
    case 57600: rate = B57600; break;
    case 38400: rate = B38400; break;
    case 19200: rate = B19200; break;
    case 9600: rate = B9600; break;
    case 4800: rate = B4800; break;
    case 2400: rate = B2400; break;
    case 1200: rate = B1200; break;
    case 600: rate = B600; break;
    case 300: rate = B300; break;
    case 110: rate = B110; break;
    case 0: rate = B0; break;
    default: LFATAL("Invalid baud rate " <<jevois::serial::baudrate::get());
    }

  cfsetispeed(&options, rate);
  cfsetospeed(&options, rate);

  // Parse the serial format string:
  std::string const format = jevois::serial::format::get();
  if (format.length() != 3) LFATAL("Incorrect format string: " << format);

  // Set the number of bits:
  options.c_cflag &= ~CSIZE; // mask off the 'size' bits

  switch (format[0])
  {
  case '5': options.c_cflag |= CS5; break;
  case '6': options.c_cflag |= CS6; break;
  case '7': options.c_cflag |= CS7; break;
  case '8': options.c_cflag |= CS8; break;
  default: LFATAL("Invalid charbits: " << format[0] << " (should be 5..8)");
  }

  // Set parity option:
  options.c_cflag &= ~(PARENB | PARODD);

  switch(format[1])
  {
  case 'N': break;
  case 'E': options.c_cflag |= PARENB; break;
  case 'O': options.c_cflag |= (PARENB | PARODD); break;
  default: LFATAL("Invalid parity: " << format[1] << " (should be N,E,O)");
  }

  // Set the stop bits option:
  options.c_cflag &= ~CSTOPB;
  switch(format[2])
  {
  case '1': break;
  case '2': options.c_cflag |= CSTOPB; break;
  default: LFATAL("Invalid stopbits: " << format[2] << " (should be 1..2)");
  }

  // Set the flow control:
  options.c_cflag &= ~CRTSCTS;
  options.c_iflag &= ~(IXON | IXANY | IXOFF);

  if (jevois::serial::flowsoft::get()) options.c_iflag |= (IXON | IXANY | IXOFF);
  if (jevois::serial::flowhard::get()) options.c_cflag |= CRTSCTS;

  // Set all the options now:
  if (tcsetattr(itsDev, TCSANOW, &options) == -1) LFATAL("Failed to set port options");
  LINFO("Serial driver ready on " << jevois::serial::devname::get());
}

// ######################################################################
void jevois::Serial::postUninit()
{
  if (itsDev != -1)
  {
    sendBreak();
    if (tcsetattr(itsDev, TCSANOW, &itsSavedState) == -1) LERROR("Failed to restore serial port state -- IGNORED");
    ::close(itsDev);
    itsDev = -1;
  }
}

// ######################################################################
void jevois::Serial::setBlocking(bool blocking, std::chrono::milliseconds const & timeout)
{
  std::lock_guard<std::mutex> _(itsMtx);
  
  int flags = fcntl(itsDev, F_GETFL, 0);
  if (flags == -1) LFATAL("Cannot get flags");
  if (blocking) flags &= (~O_NONBLOCK); else flags |= O_NONBLOCK;
  if (fcntl(itsDev, F_SETFL, flags) == -1) LFATAL("Cannot set flags");

  // If blocking, set a timeout on the descriptor:
  if (blocking)
  {
    termios options;
    if (tcgetattr(itsDev, &options) == -1) LFATAL("Failed to get options");
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = timeout.count() / 100; // vtime is in tenths of second
    if (tcsetattr(itsDev, TCSANOW, &options) == -1) LFATAL("Failed to set port options");
  }
}

// ######################################################################
void jevois::Serial::toggleDTR(std::chrono::milliseconds const & dur)
{
  std::lock_guard<std::mutex> _(itsMtx);

  struct termios tty, old;

  if (tcgetattr(itsDev, &tty) == -1 || tcgetattr(itsDev, &old) == -1) LFATAL("Failed to get attributes");

  cfsetospeed(&tty, B0);
  cfsetispeed(&tty, B0);

  if (tcsetattr(itsDev, TCSANOW, &tty) == -1) LFATAL("Failed to set attributes");

  std::this_thread::sleep_for(dur);

  if (tcsetattr(itsDev, TCSANOW, &old) == -1) LFATAL("Failed to restore attributes");
}

// ######################################################################
void jevois::Serial::sendBreak(void)
{
  std::lock_guard<std::mutex> _(itsMtx);

  // Send a Hangup to the port
  tcsendbreak(itsDev, 0);
}

// ######################################################################
int jevois::Serial::read(void * buffer, const int nbytes)
{
  std::lock_guard<std::mutex> _(itsMtx);

  int n = ::read(itsDev, buffer, nbytes);

  if (n == -1) throw std::runtime_error("Serial: Read error");
  if (n == 0) throw std::runtime_error("Serial: Read timeout");

  return n;
}

// ######################################################################
int jevois::Serial::read2(void * buffer, const int nbytes)
{
  std::lock_guard<std::mutex> _(itsMtx);

  int n = ::read(itsDev, buffer, nbytes);

  if (n == -1)
  {
    if (errno == EAGAIN) return false; // no new char available
    else throw std::runtime_error("Serial: Read error");
  }

  if (n == 0) return false; // no new char available

  return n;
}

// ######################################################################
bool jevois::Serial::readSome(std::string & str)
{
  std::lock_guard<std::mutex> _(itsMtx);

  unsigned char c;

  while (true)
  {
    int n = ::read(itsDev, reinterpret_cast<char *>(&c), 1);

    if (n == -1)
    {
      if (errno == EAGAIN) return false; // no new char available
      else throw std::runtime_error("Serial: Read error");
    }

    if (n == 0) return false; // no new char available
    
    switch (jevois::serial::linestyle::get())
    {
    case jevois::serial::LineStyle::LF:
      if (c == '\n') { str = std::move(itsPartialString); itsPartialString.clear(); return true; }
      else itsPartialString += c;
      break;
      
    case jevois::serial::LineStyle::CR:
      if (c == '\r') { str = std::move(itsPartialString); itsPartialString.clear(); return true; }
      else itsPartialString += c;
      break;

    case jevois::serial::LineStyle::CRLF:
      if (c == '\n') { str = std::move(itsPartialString); itsPartialString.clear(); return true; }
      else if (c != '\r') itsPartialString += c;
      break;

    case jevois::serial::LineStyle::Zero:
      if (c == 0x00) { str = std::move(itsPartialString); itsPartialString.clear(); return true; }
      else itsPartialString += c;
      break;

    case jevois::serial::LineStyle::Sloppy: // Return when we receive first separator, ignore others
      if (c == '\r' || c == '\n' || c == 0x00 || c == 0xd0)
      {
        if (itsPartialString.empty() == false)
        { str = std::move(itsPartialString); itsPartialString.clear(); return true; }
      }
      else itsPartialString += c;
      break;
    }
  }
}

// ######################################################################
std::string jevois::Serial::readString()
{
  std::lock_guard<std::mutex> _(itsMtx);

  std::string str; unsigned char c;
  
  while (true)
  {
    int n = ::read(itsDev, reinterpret_cast<char *>(&c), 1);

    if (n == -1)
    {
      if (errno == EAGAIN) std::this_thread::sleep_for(std::chrono::milliseconds(2));
      else throw std::runtime_error("Serial: Read error");
    }
    else if (n == 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(2)); // no new char available
    else
    {
      switch (jevois::serial::linestyle::get())
      {
      case jevois::serial::LineStyle::LF: if (c == '\n') return str; else str += c; break;
        
      case jevois::serial::LineStyle::CR: if (c == '\r') return str; else str += c; break;
        
      case jevois::serial::LineStyle::CRLF: if (c == '\n') return str; else if (c != '\r') str += c; break;
        
      case jevois::serial::LineStyle::Zero: if (c == 0x00) return str; else str += c; break;
        
      case jevois::serial::LineStyle::Sloppy: // Return when we receive first separator, ignore others
        if (c == '\r' || c == '\n' || c == 0x00 || c == 0xd0) { if (str.empty() == false) return str; }
        else str += c;
        break;
      }
    }
  }
}

// ######################################################################
void jevois::Serial::writeString(std::string const & str)
{
  std::string fullstr(str);

  switch (jevois::serial::linestyle::get())
  {
  case jevois::serial::LineStyle::CR: fullstr += '\r'; break;
  case jevois::serial::LineStyle::LF: fullstr += '\n'; break;
  case jevois::serial::LineStyle::CRLF: fullstr += "\r\n"; break;
  case jevois::serial::LineStyle::Zero: fullstr += '\0'; break;
  case jevois::serial::LineStyle::Sloppy: fullstr += "\r\n"; break;
  }

  this->write(fullstr.c_str(), fullstr.length());
}

// ######################################################################
void jevois::Serial::write(void const * buffer, const int nbytes)
{
  if (drop::get())
  {
    // Just write and ignore (after a few attempts) if the number of bytes written is not what we wanted to write:
    std::lock_guard<std::mutex> _(itsMtx);

    int ndone = 0; char const * b = reinterpret_cast<char const *>(buffer); int iter = 0;
    while (ndone < nbytes && iter++ < 10)
    {
      int n = ::write(itsDev, b + ndone, nbytes - ndone);
      if (n == -1 && errno != EAGAIN) throw std::runtime_error("Serial: Write error");
      
      // If we did not write the whole thing, the serial port is saturated, we need to wait a bit:
      ndone += n;
      if (ndone < nbytes) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }
  else
  {
    std::lock_guard<std::mutex> _(itsMtx);
    
    int ndone = 0; char const * b = reinterpret_cast<char const *>(buffer); int iter = 0;
    while (ndone < nbytes && iter++ < 10)
    {
      int n = ::write(itsDev, b + ndone, nbytes - ndone);
      if (n == -1 && errno != EAGAIN) throw std::runtime_error("Serial: Write error");
      
      // If we did not write the whole thing, the serial port is saturated, we need to wait a bit:
      ndone += n;
      if (ndone < nbytes) tcdrain(itsDev);
    }
    
    if (ndone < nbytes)
    {
      // If we had a serial overflow, we need to let the user know, but how, since the serial is overflowed already?
      // Let's first throttle down big time, and then we throw once in a while:
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      
      tcdrain(itsDev);
      
      // Report the overflow once in a while:
      ++itsWriteOverflowCounter; if (itsWriteOverflowCounter > 100) itsWriteOverflowCounter = 0;
      if (itsWriteOverflowCounter == 1)
	throw std::overflow_error("Serial write overflow: need to reduce amount ot serial writing");
      
      // Note how we are otherwise just ignoring the overflow and hence dropping data.
    }
    else itsWriteOverflowCounter = 0;
  }
}

// ######################################################################
void jevois::Serial::writeNoCheck(void const * buffer, const int nbytes)
{
    std::lock_guard<std::mutex> _(itsMtx);

    int ndone = 0; char const * b = reinterpret_cast<char const *>(buffer); int iter = 0;
    while (ndone < nbytes && iter++ < 50)
    {
        int n = ::write(itsDev, b + ndone, nbytes - ndone);
        if (n == -1 && errno != EAGAIN) throw std::runtime_error("Serial: Write error");
        ndone += n;
    }

    if (ndone < nbytes)
    {
      // If after a number of iterations there are still unbuffered bytes, flush the output buffer
      if (tcflush(itsDev, TCOFLUSH) != 0) LDEBUG("Serial flushOut error -- IGNORED");

    }
}

// ######################################################################
void jevois::Serial::flush(void)
{
  std::lock_guard<std::mutex> _(itsMtx);
  
  // Flush the input
  if (tcflush(itsDev, TCIFLUSH) != 0) LDEBUG("Serial flush error -- IGNORED");
}


// ######################################################################
jevois::Serial::~Serial(void)
{  }

// ####################################################################################################
jevois::UserInterface::Type jevois::Serial::type() const
{ return itsType; }

