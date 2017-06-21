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

#include <sys/syslog.h> // for the syslog levels
#include <string.h> // for strerror
#include <string>
#include <sstream>
#include <cstdint>
#include <mutex>


namespace jevois
{
  class Engine;
  class RawImage;
  
  /*! \defgroup debugging Debugging helper classes, functions and macros */
  
  //! Current log level
  /*! The log level is set by changing the value of this global variable. The default value is LOG_INFO.

      The possible log values are defined in sys/syslog.h and here we only handle the following with different amounts
      of messages: LOG_CRIT, LOG_ERR, LOG_INFO, LOG_DEBUG. \ingroup debugging */
  extern int logLevel;

  //! Current trace level
  /*! Higher levels yield more verbosity in tracing. Note that this has effect only if JEVOIS_TRACE_ENABLE is specified
      as a compile option, and the trace messages are issued at the LDEBUG log level, so JEVOIS_LDEBUG_ENABLE must also
      be specified at compile time. \ingroup debugging*/
  extern int traceLevel;

  //! Logger class
  /*! Users would typically not use this class directly but instead invoke one of the LDEBUG(msg), LINFO(msg), etc
      macros. Note that by default logging is asynchronous, i.e., when issuing a log message it is assembled and then
      pushed into a queue, and another thread then pops it back from the queue and displays it. Define
      JEVOIS_USE_SYNC_LOG at compile time to have the mesage displayed immediately but beware that this can break USB
      strict timing requirements. \ingroup debugging */
  template <int Level>
  class Log
  {
    public:
      //! Construct a new Log, adding a prefix to the log stream
      /*! If outstr is non-null, the log message will be copied into it upon destruction. */
      Log(char const * fullFileName, char const * functionName, std::string * outstr = nullptr);

      //! Close the Log, outputting the aggregated message
      ~Log();

      //! Overloaded stream input operator for any type that has operator<< defined for ostream.
      template <class T> inline
      Log<Level> & operator<<(T const & out_item) { itsLogStream << out_item; return *this; }

      //! Overload of operator<< for uint8 (displays it as an int rather than char)
      Log<Level> & operator<<(uint8_t const & out_item);

      //! Overload of operator<< for int8 (displays it as an int rather than char)
      Log<Level> & operator<<(int8_t const & out_item);

    private:
      std::ostringstream itsLogStream;
      std::string * itsOutStr;
  };

  //! Convenience function to catch an exception, issue some LERROR (depending on type), and rethrow it
  /*! User code that is not going to swallow exceptions can use this function as follows, to log some trace of the
      exception that was thrown:

      \code
      try { do_something_risky(); } catch (...) { jevois::warnAndRethrowException(); }
      \endcode

      \note This function throws! So obviously it only makes sense to use it inside a catch block. \ingroup debugging */
  void warnAndRethrowException[[noreturn]]();

  //! Convenience function to catch an exception, issue some LERROR (depending on type), and ignore it
  /*! User code can use this function as follows, to log some trace of the exception that was thrown, and then swallow
      (ignore) the exception. Use this sparingly, swallowing exceptions often defeats the whole logic of using
      exceptions in the first place. Example use:

      \code
      try { do_something_risky_and_we_dont_care_if_it_fails(); } catch (...) { jevois::warnAndIgnoreException(); }
      \endcode

      Note that the message that is logged to console is also returned as a string, in case one wants to report it in
      some other way (e.g., in a GUI or in a video frame using drawErrorImage()). \ingroup debugging */
  std::string warnAndIgnoreException();

  //! Display an error message into a RawImage
  /*! The error message should consist of a string where multiple lines may be separated by \\n characters, such as the
      string returned by warnAndIgnoreException(). The message will be written in the image, which should be
      valid(). This is useful to display module exceptions in the video stream that is sent over USB.*/
  void drawErrorImage(std::string const & errmsg, RawImage & videoerrimg);
  
  //! Set an Engine so that all log messages will be forwarded to its serial ports
  /*! This function is not intended for general use, Engine uses it internally when users set one of its
      parameters to enable forwarding of log messages to serial ports. \ingroup debugging*/
  void logSetEngine(Engine * e);
  
} // namespace jevois


#ifdef JEVOIS_LDEBUG_ENABLE
//! Convenience macro for users to print out console or syslog messages, DEBUG level
/*! \def LDEBUG(msg)
    \hideinitializer
    
    This macro is intended to be used with a stream-oriented syntax for everything that is passed as argument to the
    macro. The syntax is a bit strange at first but you will rapidly get used to it. This allows any datatype that has
    an operator<< defined to be printed out in a log (contrary to printf-style syntax). For example:
    
    @code
    int x = 3; std::string str = "hello"; jevois::StepRange<int> rng(0, 5, 100);
    LDEBUG("x=" << x << " and str=" << str << " and rng=" << rng);
    @endcode
    
    \note This is the preferred way to issue messages. Do you use printf, do not use cout<<"blah", etc.
    
    By design, your log message will not be evaluated if the current log level is below (stronger than) the debug
    level. This means that you should not be afraid of wasting CPU computing messages that will not be output; for
    example:
    
    @code
    LDEBUG("CPU-intensive function says: " << cpu_intensive_function());
    @endcode
    
    will not run the cpu-intensive function if the current log level is LOG_ERR (it will still run one "if" statement to
    check the current log level). This also means that you should never assume that your log message will be
    evaluated. For example:
    
    @code
    int x = 42;
    LDEBUG("x = " << (x++) ); // x may now be 43 or 42 depending on current log level...
    @endcode

    \note Because LDEBUG() may be used for debugging of many fast loops, including through the use of
    JEVOIS_TRACE(level), it will be compiled in only if JEVOIS_LDEBUG_ENABLE is defined during build (typicaly, this is
    done as an option passed to cmake), otherwise it will simply be commented out so that no CPU is wasted.
    \ingroup debugging */
#define LDEBUG(msg) do { if (jevois::logLevel >= LOG_DEBUG)             \
      jevois::Log<LOG_DEBUG>(__FILE__, __FUNCTION__) << msg; } while (false)

//! Like LDEBUG but appends errno and strerror(errno), to be used when some system call fails
/*! \def PLDEBUG(msg)
    \hideinitializer
    
    Usage syntax is the same as for LDEBUG(msg) \ingroup debugging */
#define PLDEBUG(msg) do { if (jevois::logLevel >= LOG_DEBUG)            \
      jevois::Log<LOG_DEBUG>(__FILE__, __FUNCTION__) << msg << " [" << errno << "](" << strerror(errno) << ')'; } \
  while (false)
#else
#define LDEBUG(msg) do { } while (false)
#define PLDEBUG(msg) do { } while (false)
#endif

//! Convenience macro for users to print out console or syslog messages, INFO level
/*! \def LINFO(msg)
    \hideinitializer
    
    Usage syntax is the same as for LDEBUG(msg) \ingroup debugging */
#define LINFO(msg) do { if (jevois::logLevel >= LOG_INFO) jevois::Log<LOG_INFO>(__FILE__, __FUNCTION__) << msg; } \
  while (false)

//! Like LINFO but appends errno and strerror(errno), to be used when some system call fails
/*! \def PLINFO(msg)
    \hideinitializer
    
    Usage syntax is the same as for LDEBUG(msg) \ingroup debugging */
#define PLINFO(msg) do { if (jevois::logLevel >= LOG_INFO)              \
      jevois::Log<LOG_INFO>(__FILE__, __FUNCTION__) << msg << " [" << errno << "](" << strerror(errno) << ')'; } \
  while (false)

//! Convenience macro for users to print out console or syslog messages, ERROR level
/*! \def LERROR(msg)
    \hideinitializer
    
    Usage syntax is the same as for LDEBUG(msg) \ingroup debugging */
#define LERROR(msg) do { if (jevois::logLevel >= LOG_ERR) jevois::Log<LOG_ERR>(__FILE__, __FUNCTION__) << msg; } \
  while (false)

//! Like LERROR but appends errno and strerror(errno), to be used when some system call fails
/*! \def PLERROR(msg)
    \hideinitializer
    
    Usage syntax is the same as for LDEBUG(msg) \ingroup debugging */
#define PLERROR(msg) do { if (jevois::logLevel >= LOG_ERR)              \
      jevois::Log<LOG_ERR>(__FILE__, __FUNCTION__) << msg << " [" << errno << "](" << strerror(errno) << ')'; } \
  while (false)


//! Convenience macro for users to print out console or syslog messages, FATAL level
/*! \def LFATAL(msg)
    \hideinitializer
    
    Usage syntax is the same as for LDEBUG(msg)
    \note After printing the message, this also throws std::runtime_error \ingroup debugging */
#define LFATAL(msg) do { std::string str; { jevois::Log<LOG_CRIT>(__FILE__, __FUNCTION__, &str) << msg; } \
    throw std::runtime_error(str); } while (false)

//! Like LDEBUG but appends errno and strerror(errno), to be used when some system call fails
/*! \def PLFATAL(msg)
    \hideinitializer

    Usage syntax is the same as for LDEBUG(msg)
    \note After printing the message, this also throws std::runtime_error \ingroup debugging */
#define PLFATAL(msg) do { std::string str; { jevois::Log<LOG_CRIT>(__FILE__, __FUNCTION__, &str) \
        << msg << " [" << errno << "](" << strerror(errno) << ')'; }    \
    throw std::runtime_error(str); } while (false)

//! Test whether something is true and issue an LFATAL if not
/*! \def JEVOIS_ASSERT(cond)
    \hideinitializer \ingroup debugging */
#define JEVOIS_ASSERT(cond) do { if (cond) { } else                     \
    { std::string str; { jevois::Log<LOG_CRIT>(__FILE__, __FUNCTION__, &str) << "Assertion failed: " #cond; } \
      throw std::runtime_error(str); } } while (false)

// ##############################################################################################################
#ifdef JEVOIS_TRACE_ENABLE
namespace jevois
{
  namespace trace
  {
    //! Helper class for tracing, issues one message on construction, and another on destruction
    /*! Users would typically use the JEVOIS_TRACE(level) macro rather than this class directly. \ingroup debugging */
    class TraceObject
    {
      public:
        //! Constructor, logs "file:function Enter"
        inline TraceObject(int level, char const * file, char const * func) :
            itsLevel(level), itsFile(file), itsFunc(func)
        { if (jevois::traceLevel >= itsLevel) jevois::Log<LOG_DEBUG>(file, func) << "Enter"; }

        //! Destructor, logs "file:function Exit"
        inline ~TraceObject()
        { if (jevois::traceLevel >= itsLevel) jevois::Log<LOG_DEBUG>(itsFile, itsFunc) << "Exit"; }

      private:
        int const itsLevel;
        char const * const itsFile;
        char const * const itsFunc;
    };
  }
}

//! Trace object
/*! \def JEVOIS_TRACE(level)
    \hideinitializer

    Use this as you do with, e.g., std::lock_guard. Issues one LDEBUG() message on construction, and one on
    destruction. Typically, you would hence invoke JEVOIS_TRACE as the first command in each of the functions you want
    to trace. \ingroup debugging */
#define JEVOIS_TRACE(level) jevois::trace::TraceObject __jevois_trace_reserved(level, __FILE__, __FUNCTION__)
#else
#define JEVOIS_TRACE(level) do { } while (0)
#endif

// ##############################################################################################################
namespace jevois
{
  //! Acquire a lock object on a std::timed_mutex, or LFATAL after 1 second of waiting
  /*! Use this as you would use lock_guard (but make sure your mutex is std::timed_mutex). It will throw in case of
      deadlock, useful for debugging. Users would typically use the JEVOIS_TIMED_LOCK(mtx) macro rather than this class
      directly. \ingroup debugging */
  class timed_lock_guard
  {
    public:
      //! Constructor, locks the mutex or throw if it cannot be locked before timeout
      explicit timed_lock_guard(std::timed_mutex & mtx, char const * file, char const * func);

      //! Destructor, unlocks the mutex
      ~timed_lock_guard();

    private:
      std::timed_mutex & itsMutex;
  };
}

//! Helper macro to create a timed_lock_guard object
/*! \def JEVOIS_TIMED_LOCK(mtx)
    \hideinitializer

    Creates a timed_lock_guard over std::timed_mutex mtx, which will throw if mtx cannot be locked before timeout. The
    guard will unlock the mutex upon destruction. \ingroup debugging */
#define JEVOIS_TIMED_LOCK(mtx) jevois::timed_lock_guard __jevois_timed_lock_guard_reserved(mtx, __FILE__, __FUNCTION__)
  
