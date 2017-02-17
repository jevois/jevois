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

#include <jevois/Types/Semaphore.H>
#include <queue>

namespace jevois
{
  //! Thread-safe synchronized producer/consumer queue
  /*! BoundedBuffer is designed for use in producer/consumer scenarios where multiple threads wish to push and pop data
      onto/from the buffer asynchronously.  Threads that try to pop data when the buffer is empty will either sleep
      until data is actually available or get an exception (depending on template args), and threads that try to push
      data when the buffer is full will either block until some space is available in the buffer or get an exception
      (depending on template args).

      @tparam WhenFull blocking behavior (as jevois::BlockingBehavior) when attempting to push into a full buffer
      @tparam WhenEmpty blocking behavior (as jevois::BlockingBehavior) when attempting to pop from an empty buffer

      \ingroup types */
  template <typename T, BlockingBehavior WhenFull, BlockingBehavior WhenEmpty>
  class BoundedBuffer
  {
    public:
      //! Create a new BoundedBuffer with no data and a given size
      BoundedBuffer(size_t const siz);

      //! Push a new data element into the buffer, potentially sleeping or throwing if buffer is full, copy version
      void push(T const & val);

      //! Push a new data element into the buffer, potentially sleeping or throwing if buffer is full, move version
      void push(T && val);

      //! Pop oldest data element off of the buffer, potentially sleeping until one is available or throwing if empty
      T pop();

      //! Current number of items actually in the buffer
      /*! This function is mostly provided for informational messages and beware that the actual filled size may
	  change in a multithreaded environment between the time we return here and the time the caller tries to
	  use the result. */
      size_t filled_size() const;

      //! Max (allocated at construction) size of the buffer
      size_t size() const;

      //! Clear all contents, resetting filled_size() to zero (size() remains unchanged at the max possible size)
      void clear();
      
    private:
      size_t const itsSize;
      std::queue<T> itsQueue;
      jevois::Semaphore<WhenFull> itsEmptySemaphore;
      jevois::Semaphore<WhenEmpty> itsFullSemaphore;
      mutable std::mutex itsMutex;
  };
} // namespace jevois

// Include implementation details
#include <jevois/Types/details/BoundedBufferImpl.H>
