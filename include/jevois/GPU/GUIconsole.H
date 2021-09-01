// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

#include <jevois/Core/UserInterface.H>
#include <deque>
#include <memory>

class ImGuiInputTextCallbackData;
  
namespace jevois
{
  //! Simple console with coloring and completion
  class GUIconsole : public UserInterface
  {
    public:
      //! Constructor
      GUIconsole(std::string const & instance);

      //! Destructor
      virtual ~GUIconsole();

      //! Read some bytes if available, and return true and a string when one is complete (RETURN pressed)
      /*! str is untouched if user input is not yet complete (RETURN not yet pressed). The RETURN (end of line) marker
          is not copied into str, only the characters received up to the end of line marker. */
      bool readSome(std::string & str) override;

      //! Write a string
      /*! No line terminator should be included in the string, writeString() will add one. In the Serial derived class,
          this will be using the line termination convention of serial::linestyle to support different styles (CR, CRLF,
          LF, etc). */
      void writeString(std::string const & str) override;

      //! Our type is: GUI
      Type type() const override;

      //! Render into ImGui
      void draw();

    protected:
      //! Clear the contents of the window
      void clear();

      //void AddLog(const char* fmt, ...) IM_FMTARGS(2);


      //void ExecCommand(const char* command_line);

      //int TextEditCallback(ImGuiInputTextCallbackData* data);

    private:
      mutable std::mutex itsDataMtx;
      std::deque<std::pair<bool /* user/jevois */, std::string> > itsData;
      std::string itsLastInput;
      char itsInputBuf[1024];
      std::vector<std::string> itsHistory;
      int itsHistoryPos = 0;

    public:
      int callback(ImGuiInputTextCallbackData * data);
  };
}

#endif // JEVOIS_PRO
