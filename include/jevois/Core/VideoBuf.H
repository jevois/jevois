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

#include <cstddef>

namespace jevois
{
  //! A V4L2 video buffer, to be held in a shared_ptr
  /*! Requests an MMAP'ed memory area from the given file descriptor at construction, and unmaps it at
      destruction. VideoBuf is used to pass MMAP'ed video buffers from Camera and Gadget drivers to application code,
      via RawImage. The actual memory allocation is performed by the kernel driver. Hence, VideoBuf pixel arrays cannot
      be moved from one memory location to another. \ingroup core */
  class VideoBuf
  {
    public:
      //! Construct and allocate MMAP'd memory
      /*! Mostly for debugging purposes (supporting VideoDisplay), if fd is -1 then we perform a regular memory
          allocation instead of mmap. */
      VideoBuf(int const fd, size_t const length, unsigned int offset, int const dmafd);

      //! Destructor unmaps the memory
      ~VideoBuf();

      //! Sync the data
      /*! This may be useful in some cases to avoid cache coherency issues between DMA-capable driver and CPU. */
      void sync();
      
      //! Get a pointer to the buffer data
      void * data() const;

      //! Get the allocated memory length
      size_t length() const;

      //! Set the number of bytes used, eg, for MJPEG images that application code compressed into the buffer
      void setBytesUsed(size_t n);

      //! Get the number of bytes used, valid only for MJPEG images
      size_t bytesUsed() const;

      //! Get the dma_buf fd associated with this buffer, which was given at construction
      int dmaFd() const;
      

    private:
      int const itsFd;
      size_t const itsLength;
      size_t itsBytesUsed;
      void * itsAddr;
      int const itsDmaBufFd;
  };
  
} // namespace jevois
