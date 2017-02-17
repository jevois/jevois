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

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::BoundedBuffer(size_t const siz) :
    itsSize(siz), itsEmptySemaphore(siz), itsFullSemaphore(0)
{ }

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
void jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::push(T const & val)
{
  itsEmptySemaphore.decrement(1);

  itsMutex.lock();
  itsQueue.push(val);
  itsMutex.unlock();

  itsFullSemaphore.increment(1);
}

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
void jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::push(T && val)
{
  itsEmptySemaphore.decrement(1);

  itsMutex.lock();
  itsQueue.push(std::move(val));
  itsMutex.unlock();

  itsFullSemaphore.increment(1);
}

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
T jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::pop()
{
  itsFullSemaphore.decrement(1);

  itsMutex.lock();
  T val = itsQueue.front();
  itsQueue.pop();
  itsMutex.unlock();

  itsEmptySemaphore.increment(1);

  return val;
}

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
size_t jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::filled_size() const
{ return itsFullSemaphore.count(); }

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
size_t jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::size() const
{ return itsSize; }

// ##############################################################################################################
template <typename T, jevois::BlockingBehavior WhenFull, jevois::BlockingBehavior WhenEmpty> inline
void jevois::BoundedBuffer<T, WhenFull, WhenEmpty>::clear()
{
  itsMutex.lock();

  while (itsQueue.size())
  {
    itsFullSemaphore.decrement(1);
    itsQueue.pop();
    itsEmptySemaphore.increment(1);
  }

  itsMutex.unlock();
}

