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

#include <jevois/Debug/Log.H>
#include <jevois/Debug/PythonException.H>
#include <jevois/Image/RawImageOps.H>
#include <mutex>
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace jevois
{
  int logLevel = LOG_INFO;
  int traceLevel = 0;
}

namespace
{
  template <int Level> char const * levelStr();
  template <> char const * levelStr<LOG_DEBUG>() { return "DBG"; }
  template <> char const * levelStr<LOG_INFO>() { return "INF"; }
  template <> char const * levelStr<LOG_ERR>() { return "ERR"; }
  template <> char const * levelStr<LOG_CRIT>() { return "FTL"; }
}

#ifdef JEVOIS_USE_SYNC_LOG
namespace jevois
{
  // Mutex used to avoid clashing synchronous outputs from multiple threads
  std::mutex logOutputMutex;
}

void jevois::logSetEngine(Engine * e)
{ LFATAL("Cannot set Engine for logs when JeVois has been compiled with -D JEVOIS_USE_SYNC_LOG"); }

#else // JEVOIS_USE_SYNC_LOG
#include <future>
#include <jevois/Types/BoundedBuffer.H>
#include <jevois/Types/Singleton.H>
#include <jevois/Core/Engine.H>

namespace
{
  class LogCore : public jevois::Singleton<LogCore>
  {
    public:
      LogCore() : itsBuffer(10000), itsRunning(true)
#ifdef JEVOIS_LOG_TO_FILE
                , itsStream("jevois.log")
#endif
                , itsEngine(nullptr)
      {
        itsRunFuture = std::async(std::launch::async, &LogCore::run, this);
      }

      virtual ~LogCore()
      {
        // Tell run() thread to quit:
        itsRunning = false;
        
        // Push a message so we unblock our run() thread in case the buffer was empty:
        itsBuffer.push("Terminating Log activity");

        // Wait for the run() thread to complete:
        if (itsRunFuture.valid()) try { itsRunFuture.get(); } catch (...) { jevois::warnAndIgnoreException(); }
      }

      void run()
      {
        while (itsRunning)
        {
          std::string msg = itsBuffer.pop();
#ifdef JEVOIS_LOG_TO_FILE
          itsStream << msg << std::endl;
#else
#ifdef JEVOIS_PLATFORM         
          // When using the serial port debug on platform and screen connected to it, screen gets confused if we do not
          // send a CR here, since some other messages do send CR (and screen might get confused as to which line end to
          // use). So send a CR too:
          std::cerr << msg << '\r' << std::endl;
#else
          std::cerr << msg << std::endl;
#endif
#endif
          if (itsEngine) itsEngine->sendSerial(msg, true);
        }
      }

      jevois::BoundedBuffer<std::string, jevois::BlockingBehavior::Block, jevois::BlockingBehavior::Block> itsBuffer;
      volatile bool itsRunning;
      std::future<void> itsRunFuture;
#ifdef JEVOIS_LOG_TO_FILE
      std::ofstream itsStream;
#endif
      jevois::Engine * itsEngine;
  };
}

void jevois::logSetEngine(Engine * e) { LogCore::instance().itsEngine = e; }

#endif // JEVOIS_USE_SYNC_LOG

// ##############################################################################################################
template <int Level>
jevois::Log<Level>::Log(char const * fullFileName, char const * functionName, std::string * outstr) :
    itsOutStr(outstr)
{
  // Strip out the file path and extension from the full file name
  std::string const fn(fullFileName);
  size_t const lastSlashPos = fn.rfind('/');
  size_t const lastDotPos = fn.rfind('.');
  std::string const partialFileName = fn.substr(lastSlashPos+1, lastDotPos-lastSlashPos-1);

  // Print out a pretty prefix to the log message
  itsLogStream << levelStr<Level>() << ' ' << partialFileName << "::" << functionName << ": ";
}

// ##############################################################################################################
#ifdef JEVOIS_USE_SYNC_LOG

template <int Level>
jevois::Log<Level>::~Log()
{
  std::lock_guard<std::mutex> guard(jevois::logOutputMutex);
  std::string const msg = itsLogStream.str();
  std::cerr << msg << std::endl;
  if (itsOutStr) *itsOutStr = msg;
}

#else // JEVOIS_USE_SYNC_LOG

template <int Level>
jevois::Log<Level>::~Log()
{
  std::string const msg = itsLogStream.str();
  LogCore::instance().itsBuffer.push(msg);
  if (itsOutStr) *itsOutStr = msg;
}
#endif // JEVOIS_USE_SYNC_LOG

// ##############################################################################################################
template <int Level>
jevois::Log<Level> & jevois::Log<Level>::operator<<(uint8_t const & out_item)
{
  itsLogStream << static_cast<int>(out_item);
  return * this;
}

// ##############################################################################################################
template <int Level>
jevois::Log<Level> & jevois::Log<Level>::operator<<(int8_t const & out_item)
{
  itsLogStream << static_cast<int>(out_item);
  return * this;
}

// ##############################################################################################################
// Explicit instantiations:
namespace jevois
{
  template class Log<LOG_DEBUG>;
  template class Log<LOG_INFO>;
  template class Log<LOG_ERR>;
  template class Log<LOG_CRIT>;
}

// ##############################################################################################################
void jevois::warnAndRethrowException()
{
  // great trick to get back the type of an exception caught via a catch(...), just rethrow it and catch again:
  try { throw; }

  catch (std::exception const & e)
  {
    LERROR("Passing through std::exception: " << e.what());
    throw;
  }

  catch (boost::python::error_already_set & e)
  {
    LERROR("Received exception from the Python interpreter:");
    std::string str = jevois::getPythonExceptionString(e);
    std::vector<std::string> lines = jevois::split(str, "\\n");
    for (std::string const & li : lines) LERROR("   " << li);
    throw;
  }
  
  catch (...)
  {
    LERROR("Passing through unknown exception");
    throw;
  }
}

// ##############################################################################################################
std::string jevois::warnAndIgnoreException()
{
  std::vector<std::string> retvec;

  // great trick to get back the type of an exception caught via a catch(...), just rethrow it and catch again:
  try { throw; }

  catch (std::exception const & e)
  {
    retvec.push_back("Caught std::exception [" + std::string(e.what()) + ']');
  }

  catch (boost::python::error_already_set & e)
  {
    retvec.push_back("Caught exception from the Python interpreter:");
    std::string str = jevois::getPythonExceptionString(e);
    std::vector<std::string> lines = jevois::split(str, "\\n");
    for (std::string const & li : lines) retvec.push_back("   " + li);
  }
  
  catch (...)
  {
    retvec.push_back("Caught unknown exception");
  }

  // Write out the message:
  std::string ret;
  for (std::string & m : retvec) { LERROR(m); ret += m + "\n"; }

  return ret;
}

// ##############################################################################################################
void jevois::drawErrorImage(std::string const & errmsg, jevois::RawImage & videoerrimg)
{
  if (videoerrimg.valid() == false) { LERROR("Cannot draw in empty image -- IGNORED"); return; }

  int ypos = 40; int fw = 6, fh = 10; jevois::rawimage::Font font = jevois::rawimage::Font6x10;
  unsigned int white = jevois::whiteColor(videoerrimg.fmt);

  // Clear image:
  videoerrimg.clear();

  // Draw a sad face:
  jevois::rawimage::drawDisk(videoerrimg, 10, 8, 4, white);
  jevois::rawimage::drawDisk(videoerrimg, 25, 8, 4, white);
  jevois::rawimage::drawLine(videoerrimg, 8, 20, 27, 23, 2, white);
  
  // Initial message:
  jevois::rawimage::writeText(videoerrimg, "Oooops...", 45, 3, white, jevois::rawimage::Font14x26);

  // Prepare font size for error log:
  if (videoerrimg.width <= 352 || videoerrimg.height <= 240)
  { font = jevois::rawimage::Font6x10; fw = 6; fh = 10; }
  else if (videoerrimg.width <= 640 || videoerrimg.height <= 480)
  { font = jevois::rawimage::Font7x13; fw = 7; fh = 13; }
  else { font = jevois::rawimage::Font10x20; fw = 10; fh = 20; }

  // Write out the message:
  std::vector<std::string> lines = jevois::split(errmsg, "\\n");
  for (std::string & m : lines)
  {
    // Do some simple linewrap:
    unsigned int nchar = (videoerrimg.width - 6) / fw; // yes this will be a huge number if width < 6
    while (m.size() > nchar)
    {
      jevois::rawimage::writeText(videoerrimg, m.substr(0, nchar), 3, ypos, white, font);
      ypos += fh + 2;
      m = m.substr(nchar, m.npos);
    }
    // Print out the last chunk (or the whole thing if it was short):
    jevois::rawimage::writeText(videoerrimg, m, 3, ypos, white, font);
    ypos += fh + 2;
  }
}

// ##############################################################################################################
// ##############################################################################################################
jevois::timed_lock_guard::timed_lock_guard(std::timed_mutex & mtx, char const * file, char const * func) :
    itsMutex(mtx)
{
  if (itsMutex.try_lock_for(std::chrono::seconds(2)) == false)
  {
    jevois::Log<LOG_CRIT>(file, func) << "Timeout trying to acquire lock";
    throw std::runtime_error("FATAL DEADLOCK ERROR");
  }
}

// ##############################################################################################################
jevois::timed_lock_guard::~timed_lock_guard()
{ itsMutex.unlock(); }
