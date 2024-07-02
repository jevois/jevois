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
#include <string>
#include <vector>
#include <deque>

class ImGuiInputTextCallbackData;

namespace jevois
{
//! A simple helper class for a chat box rendered in ImGui
/*! This is used, for example, to create interactive dialogues with large language models (LLMs). Users can input text
    in an input box, and answers can be shown in the main window above. This chatbox is intended to operate fully
    asynchronously:

    - call get() to retrieve any new input messages entered by the user. This is non-blocking and returns
      immediately. The returned string will be empty except after the user presses [RETURN] in the input box, in which
      case the whole entered input will be returned.

    - call writeString() to add to the text that is written out in the main window.

    - call draw() on every video frame.

    Note: because it is primarily intended for use in python, this class is not thread-safe (for example, calling
    writeString() and draw() concurrently might crash the system. In most applications, the CharBox member functions
    would all be called sequentially from a single thread).
    
    \ingroup core */
  class ChatBox
  {
    public:
      //! Constructor
      ChatBox(std::string title = "JeVois-Pro ChatBox");

      //! Destructor
      virtual ~ChatBox();

      //! Get input string from user, or empty if no new input
      std::string get();

      //! Update text that is displayed above input box (output from the underlying chat bot)
      void writeString(std::string const & out);

      //! Render into an ImGui window
      void draw();

      //! Freeze/unfreeze the input box, typically to prevent new inputs until current reply is done
      void freeze(bool doit);
      
      //! Clear all displayed text:
      void clear();
      
    protected:
      std::deque<std::pair<bool /* user/jevois */, std::string> > itsData;
      std::string itsLastInput;
      char itsInputBuf[1024];
      std::vector<std::string> itsHistory;
      int itsHistoryPos = 0;
      std::string const itsTitle;
      bool itsFrozen = false;
      int itsWaitState = 10; // number of dots to display while frozen, in [0..20]
      
    public:
      int callback(ImGuiInputTextCallbackData * data);
  };
  
} // namespace jevois

#endif // JEVOIS_PRO
