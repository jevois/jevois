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
#include <jevois/Core/Engine.H>

#include <fstream>

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

// On first error, store the errno so we remember that we are in error:
#define SERFATAL(msg) do {                                              \
    if (itsErrno.load() == 0) itsErrno = errno;                         \
    LFATAL('[' << instanceName() << "] " << msg << " (" << strerror(errno) << ')'); \
  } while (0)

#define SERTHROW(msg) do {                                              \
    if (itsErrno.load() == 0) itsErrno = errno;                         \
    std::ostringstream ostr;                                            \
    ostr << '[' << instanceName() << "] " << msg << " (" << strerror(errno) << ')'; \
    throw std::runtime_error(ostr.str());                               \
  } while (0)

// ######################################################################
void jevois::Serial::tryReconnect()
{
  std::lock_guard<std::mutex> _(itsMtx);

  if (itsOpenFut.valid() == false)
  {
    engine()->reportError('[' + instanceName() + "] connection lost -- Waiting for host to re-connect");
    LINFO('[' << instanceName() << "] Waiting to reconnect to [" << jevois::serial::devname::get() << "] ...");
    itsOpenFut = jevois::async_little([this]() { openPort(); });
  }
  else if (itsOpenFut.wait_for(std::chrono::milliseconds(5)) == std::future_status::ready)
    try
    {
      itsOpenFut.get();
      LINFO('[' << instanceName() << "] re-connected.");
    } catch (...) { }
}

// ######################################################################
jevois::Serial::Serial(std::string const & instance, jevois::UserInterface::Type type) :
    jevois::UserInterface(instance), itsDev(-1), itsWriteOverflowCounter(0), itsType(type), itsErrno(0)
{ }

// ######################################################################
void jevois::Serial::postInit()
{
  std::lock_guard<std::mutex> _(itsMtx);
  openPort();
}

// ######################################################################
void jevois::Serial::openPort()
{
  // Open the port, non-blocking mode by default:
  if (itsDev != -1) ::close(itsDev);
  itsDev = ::open(jevois::serial::devname::get().c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (itsDev == -1) SERTHROW("Could not open serial port [" << jevois::serial::devname::get() << ']');

  // Save current state
  if (tcgetattr(itsDev, &itsSavedState) == -1) SERTHROW("Failed to save current state");

  // reset all the flags
  ////if (fcntl(itsDev, F_SETFL, 0) == -1) SERTHROW("Failed to reset flags");

  // Get the current option set:
  termios options = { };
  if (tcgetattr(itsDev, &options) == -1) SERTHROW("Failed to get options");

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
                       | ICANON // disable canonical mode
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
  default: SERTHROW("Invalid baud rate " <<jevois::serial::baudrate::get());
  }

  cfsetispeed(&options, rate);
  cfsetospeed(&options, rate);

  // Parse the serial format string:
  std::string const format = jevois::serial::format::get();
  if (format.length() != 3) SERTHROW("Incorrect format string: " << format);

  // Set the number of bits:
  options.c_cflag &= ~CSIZE; // mask off the 'size' bits

  switch (format[0])
  {
  case '5': options.c_cflag |= CS5; break;
  case '6': options.c_cflag |= CS6; break;
  case '7': options.c_cflag |= CS7; break;
  case '8': options.c_cflag |= CS8; break;
  default: SERTHROW("Invalid charbits: " << format[0] << " (should be 5..8)");
  }

  // Set parity option:
  options.c_cflag &= ~(PARENB | PARODD);

  switch(format[1])
  {
  case 'N': break;
  case 'E': options.c_cflag |= PARENB; break;
  case 'O': options.c_cflag |= (PARENB | PARODD); break;
  default: SERTHROW("Invalid parity: " << format[1] << " (should be N,E,O)");
  }

  // Set the stop bits option:
  options.c_cflag &= ~CSTOPB;
  switch(format[2])
  {
  case '1': break;
  case '2': options.c_cflag |= CSTOPB; break;
  default: SERTHROW("Invalid stopbits: " << format[2] << " (should be 1..2)");
  }

  // Set the flow control:
  options.c_cflag &= ~CRTSCTS;
  options.c_iflag &= ~(IXON | IXANY | IXOFF);

  if (jevois::serial::flowsoft::get()) options.c_iflag |= (IXON | IXANY | IXOFF);
  if (jevois::serial::flowhard::get()) options.c_cflag |= CRTSCTS;

  // Set all the options now:
  if (tcsetattr(itsDev, TCSANOW, &options) == -1) SERTHROW("Failed to set port options");

  // We are operational:
  itsErrno.store(0);
  LINFO("Serial driver [" << instanceName() << "] ready on " << jevois::serial::devname::get());
}

// ######################################################################
void jevois::Serial::postUninit()
{
  std::lock_guard<std::mutex> _(itsMtx);

  if (itsDev != -1)
  {
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
  if (flags == -1) SERFATAL("Cannot get flags");
  if (blocking) flags &= (~O_NONBLOCK); else flags |= O_NONBLOCK;
  if (fcntl(itsDev, F_SETFL, flags) == -1) SERFATAL("Cannot set flags");

  // If blocking, set a timeout on the descriptor:
  if (blocking)
  {
    termios options;
    if (tcgetattr(itsDev, &options) == -1) SERFATAL("Failed to get options");
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = timeout.count() / 100; // vtime is in tenths of second
    if (tcsetattr(itsDev, TCSANOW, &options) == -1) SERFATAL("Failed to set port options");
  }
}

// ######################################################################
void jevois::Serial::toggleDTR(std::chrono::milliseconds const & dur)
{
  std::lock_guard<std::mutex> _(itsMtx);
 
  struct termios tty, old;

  if (tcgetattr(itsDev, &tty) == -1 || tcgetattr(itsDev, &old) == -1) SERFATAL("Failed to get attributes");

  cfsetospeed(&tty, B0);
  cfsetispeed(&tty, B0);

  if (tcsetattr(itsDev, TCSANOW, &tty) == -1) SERFATAL("Failed to set attributes");

  std::this_thread::sleep_for(dur);

  if (tcsetattr(itsDev, TCSANOW, &old) == -1) SERFATAL("Failed to restore attributes");
}

// ######################################################################
void jevois::Serial::sendBreak(void)
{
  std::lock_guard<std::mutex> _(itsMtx);

  // Send a Hangup to the port
  tcsendbreak(itsDev, 0);
}

// ######################################################################
bool jevois::Serial::readSome(std::string & str)
{
  if (itsErrno.load()) { tryReconnect(); if (itsErrno.load()) return false; }
  
  std::lock_guard<std::mutex> _(itsMtx);

  unsigned char c;

  while (true)
  {
    int n = ::read(itsDev, reinterpret_cast<char *>(&c), 1);

    if (n == -1)
    {
      if (errno == EAGAIN) return false; // no new char available
      else SERFATAL("Read error");
    }
    else if (n == 0) return false; // no new char available
    
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
void jevois::Serial::writeString(std::string const & str)
{
  // If in error, silently drop all data until we successfully reconnect:
  if (itsErrno.load()) { tryReconnect(); if (itsErrno.load()) return; }
  
  std::string fullstr(str);

  switch (jevois::serial::linestyle::get())
  {
  case jevois::serial::LineStyle::CR: fullstr += '\r'; break;
  case jevois::serial::LineStyle::LF: fullstr += '\n'; break;
  case jevois::serial::LineStyle::CRLF: fullstr += "\r\n"; break;
  case jevois::serial::LineStyle::Zero: fullstr += '\0'; break;
  case jevois::serial::LineStyle::Sloppy: fullstr += "\r\n"; break;
  }

  std::lock_guard<std::mutex> _(itsMtx);
  writeInternal(fullstr.c_str(), fullstr.length());
}

// ######################################################################
void jevois::Serial::writeInternal(void const * buffer, const int nbytes, bool nodrop)
{
  // Nodrop is used to prevent dropping even if the user wants it, e.g., during fileGet().
  if (nodrop)
  {
    // Just write it all, never quit, never drop:
    int ndone = 0; char const * b = reinterpret_cast<char const *>(buffer);
    while (ndone < nbytes)
    {
      int n = ::write(itsDev, b + ndone, nbytes - ndone);
      if (n == -1 && errno != EAGAIN) SERFATAL("Write error");
      
      // If we did not write the whole thing, the serial port is saturated, we need to wait a bit:
      if (n > 0) ndone += n;
      if (ndone < nbytes) tcdrain(itsDev); // on USB disconnect, this will hang forever...
    }
  }
  else if (drop::get())
  {
    // Just write and silently drop (after a few attempts) if we could not write everything:
    int ndone = 0; char const * b = reinterpret_cast<char const *>(buffer); int iter = 0;
    while (ndone < nbytes && iter++ < 10)
    {
      int n = ::write(itsDev, b + ndone, nbytes - ndone);
      if (n == -1 && errno != EAGAIN) SERFATAL("Write error");
      
      // If we did not write the whole thing, the serial port is saturated, we need to wait a bit:
      if (n > 0) { ndone += n; iter = 0; }
      if (ndone < nbytes) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (ndone < nbytes) SERFATAL("Timeout (host disconnect or overflow) -- SOME DATA LOST");
  }
  else
  {
    // Try to write a few times, then, if not done, report overflow and drop what remains:
    int ndone = 0; char const * b = reinterpret_cast<char const *>(buffer); int iter = 0;
    while (ndone < nbytes && iter++ < 50)
    {
      int n = ::write(itsDev, b + ndone, nbytes - ndone);
      if (n == -1 && errno != EAGAIN) SERFATAL("Write error");
      
      // If we did not write the whole thing, the serial port is saturated, we need to wait a bit:
      if (n > 0) ndone += n;
      if (ndone < nbytes) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    
    if (ndone < nbytes)
    {
      // If we had a serial overflow, we need to let the user know, but how, since the serial is overflowed already?
      // Let's first throttle down big time, and then we throw:
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      
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

// ####################################################################################################
void jevois::Serial::fileGet(std::string const & abspath)
{
  std::lock_guard<std::mutex> _(itsMtx);

  std::ifstream fil(abspath, std::ios::in | std::ios::binary);
  if (fil.is_open() == false) throw std::runtime_error("Could not read file " + abspath);

  // Get file length and send it out in ASCII:
  fil.seekg(0, fil.end); size_t num = fil.tellg(); fil.seekg(0, fil.beg);

  std::string startstr = "JEVOIS_FILEGET " + std::to_string(num) + '\n';
  writeInternal(startstr.c_str(), startstr.length(), true);
  
  // Read blocks and send them to serial:
  size_t const bufsiz = std::min(num, size_t(1024 * 1024)); char buffer[1024 * 1024];
  while (num)
  {
    size_t got = std::min(bufsiz, num); fil.read(buffer, got); if (!fil) got = fil.gcount();
    writeInternal(buffer, got, true);
    num -= got;
  }
}

// ####################################################################################################
void jevois::Serial::filePut(std::string const & abspath)
{
  std::ofstream fil(abspath, std::ios::out | std::ios::binary);
  if (fil.is_open() == false) throw std::runtime_error("Could not write file " + abspath);

  // Get file length as ASCII:
  std::string lenstr; int timeout = 1000;
  while (readSome(lenstr) == false)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    --timeout;
    if (timeout == 0) throw std::runtime_error("Timeout waiting for file length for " + abspath);
  }

  if (jevois::stringStartsWith(lenstr, "JEVOIS_FILEPUT ") == false)
    throw std::runtime_error("Incorrect header while receiving file " + abspath);

  auto vec = jevois::split(lenstr);
  if (vec.size() != 2) throw std::runtime_error("Incorrect header fields while receiving file " + abspath);

  size_t num = std::stoul(vec[1]);
    
  // Read blocks from serial and write them to file:
  std::lock_guard<std::mutex> _(itsMtx);
  size_t const bufsiz = std::min(num, size_t(1024 * 1024)); char buffer[1024 * 1024];
  while (num)
  {
    int got = ::read(itsDev, buffer, bufsiz);
    if (got == -1 && errno != EAGAIN) throw std::runtime_error("Serial: Read error");
      
    if (got > 0)
    {
      fil.write(buffer, got);
      num -= got;
    }
    else std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  fil.close();
}
