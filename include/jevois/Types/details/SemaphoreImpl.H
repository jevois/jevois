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

// This file has been modified / imported from the Neuromorphic Robotics Toolkit (NRT). Original copyright is:

/* author Randolph Voorhies
    // ////////////////////////////////////////////////////////////////////////
    //              The iLab Neuromorphic Robotics Toolkit (NRT)             //
    // Copyright 2010-2012 by the University of Southern California (USC)    //
    //                          and the iLab at USC.                         //
    //                                                                       //
    //                iLab - University of Southern California               //
    //                Hedco Neurociences Building, Room HNB-10               //
    //                    Los Angeles, Ca 90089-2520 - USA                   //
    //                                                                       //
    //      See http://ilab.usc.edu for information about this project.      //
    // ////////////////////////////////////////////////////////////////////////
    // This file is part of The iLab Neuromorphic Robotics Toolkit.          //
    //                                                                       //
    // The iLab Neuromorphic Robotics Toolkit is free software: you can      //
    // redistribute it and/or modify it under the terms of the GNU General   //
    // Public License as published by the Free Software Foundation, either   //
    // version 3 of the License, or (at your option) any later version.      //
    //                                                                       //
    // The iLab Neuromorphic Robotics Toolkit is distributed in the hope     //
    // that it will be useful, but WITHOUT ANY WARRANTY; without even the    //
    // implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       //
    // PURPOSE.  See the GNU General Public License for more details.        //
    //                                                                       //
    // You should have received a copy of the GNU General Public License     //
    // along with The iLab Neuromorphic Robotics Toolkit.  If not, see       //
    // <http://www.gnu.org/licenses/>.                                       //
    // ////////////////////////////////////////////////////////////////////////
*/

#pragma once

// ##############################################################################################################
template <jevois::BlockingBehavior BB> inline
jevois::Semaphore<BB>::Semaphore(size_t n) :
  itsCount(n)
{ }

// ##############################################################################################################
template <jevois::BlockingBehavior BB> inline
void jevois::Semaphore<BB>::increment(size_t n)
{
  std::lock_guard<std::mutex> guard(itsMutex);
  itsCount += n;
  itsCondVar.notify_all();
}

// Doxygen is confused by template specializations
#ifndef JEVOIS_DOXYGEN

// ##############################################################################################################
namespace jevois
{
  // Specialization of decrement() when our behavior is to block
  template <> void Semaphore<BlockingBehavior::Block>::decrement(size_t n);

  // Specialization of decrement() when our behavior is to throw
  template <> void Semaphore<BlockingBehavior::Throw>::decrement(size_t n);
}

// ##############################################################################################################
// Specialization of decrement() when our behavior is to block
template <> inline
void jevois::Semaphore<jevois::BlockingBehavior::Block>::decrement(size_t n)
{
  std::unique_lock<std::mutex> unique_lock(itsMutex);
  
  while (itsCount < n) itsCondVar.wait(unique_lock);
  
  itsCount -= n;
}
  
// ##############################################################################################################
// Specialization of decrement() when our behavior is to throw
template <> inline
void jevois::Semaphore<jevois::BlockingBehavior::Throw>::decrement(size_t n)
{
  std::unique_lock<std::mutex> unique_lock(itsMutex);
  
  if (itsCount < n) throw std::runtime_error("Semaphore decrement failed");
  
  itsCount -= n;
}

#endif // JEVOIS_DOXYGEN
  
// ##############################################################################################################
template <jevois::BlockingBehavior BB> inline
size_t jevois::Semaphore<BB>::count() const
{
  std::lock_guard<std::mutex> guard(itsMutex);
  return itsCount;
}
  
