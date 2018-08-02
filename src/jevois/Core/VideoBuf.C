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

#include <sys/mman.h>
#include <fstream>

#include <jevois/Core/VideoBuf.H>
#include <jevois/Debug/Log.H>

// ####################################################################################################
jevois::VideoBuf::VideoBuf(int const fd, size_t const length, unsigned int offset) :
    itsFd(fd), itsLength(length), itsBytesUsed(0)
{
  if (itsFd > 0)
  {
    // mmap the buffer to any address:
#ifdef JEVOIS_PLATFORM
    // PROT_EXEC needed for clearcache() to work:
    itsAddr = mmap(NULL, length, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, offset);
#else
    itsAddr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
#endif
    if (itsAddr == MAP_FAILED) PLFATAL("Unable to map buffer");
  }
  else
  {
    // Simple memory allocation:
    itsAddr = reinterpret_cast<void *>(new char[length]);
  }
}

// ####################################################################################################
jevois::VideoBuf::~VideoBuf()
{
  if (itsFd > 0)
  {
    if (munmap(itsAddr, itsLength) < 0) PLERROR("munmap failed");
  }
  else
  {
    delete [] reinterpret_cast<char *>(itsAddr);
  }
}

#ifdef JEVOIS_PLATFORM
void clearcache(char* begin, char *end)
{
  const int syscall = 0xf0002;
  __asm __volatile (
                    "mov r0, %0\n"
                    "mov r1, %1\n"
                    "mov r7, %2\n"
                    "mov r2, #0x0\n"
                    "svc 0x00000000\n"
                    :
                    : "r" (begin), "r" (end), "r" (syscall)
                    : "r0", "r1", "r7"
                    );
}
#endif

// ####################################################################################################
void jevois::VideoBuf::sync()
{
#ifdef JEVOIS_PLATFORM
  if (itsFd > 0)
  {
    // This does nothing useful for us:
    // msync(itsAddr, itsLength, MS_SYNC | MS_INVALIDATE);

    // This works ok but a bit brutal:
    std::ofstream ofs("/proc/sys/vm/drop_caches");
    if (ofs.is_open() == false) { LERROR("Cannot flush cache -- IGNORED"); return; }
    ofs << "1" << std::endl;

    // Here is some arm-specific code:
    clearcache(reinterpret_cast<char *>(itsAddr), reinterpret_cast<char *>(itsAddr) + itsLength);
  }
#endif
}

// ####################################################################################################
void * jevois::VideoBuf::data() const
{
  return itsAddr;
}

// ####################################################################################################
size_t jevois::VideoBuf::length() const
{
  return itsLength;
}

// ####################################################################################################
void jevois::VideoBuf::setBytesUsed(size_t n)
{
  itsBytesUsed = n;
}

// ####################################################################################################
size_t jevois::VideoBuf::bytesUsed() const
{
  return itsBytesUsed;
}
