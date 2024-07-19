// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2024 by Laurent Itti, the University of Southern
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

#ifdef JEVOIS_PRO

#include <jevois/Core/Serial.H>
#include <string>
#include <deque>
#include <memory>

namespace jevois
{
  //! Simple class to monitor a serial port in the JeVois-Pro GUI
  /*! This class opens a window and displays traffic on a given serial port. Its interface is the same as a regular
      Serial (or its parent, UserInterface), with the addition of a draw() function to render into a window managed by
      the caller. */
  class GUIserial : public Serial
  {
    public:
      //! Inherited constructor ok
      using Serial::Serial;
      
      //! Virtual destructor for safe inheritance
      virtual ~GUIserial();

      //! Read some bytes if available, and return true and a string when one is complete
      /*! If the monitor window is open, anything read will also be displayed in the window. */
      bool readSome(std::string & str) override;
      
      //! Write a string, using the line termination convention of serial::linestyle
      /*! No line terminator should be included in the string, writeString() will add one.  If the monitor window is
          open, anything written will also be displayed in the window. */
      void writeString(std::string const & str) override;

      //! Render into ImGui
      /*! Render into a window that was already opened and will be closed by the caller. */
      void draw();

    protected:
      //! Clear the contents of the window
      void clear();

    private:
      mutable std::mutex itsDataMtx;
      std::deque<std::pair<bool /* user/jevois */, std::string> > itsData;
  };
} // namespace jevois
    
#endif // JEVOIS_PRO
