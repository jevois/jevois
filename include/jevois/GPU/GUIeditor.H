// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2022 by Laurent Itti, the University of Southern
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

// This is only available on JeVoisPro
#ifdef JEVOIS_PRO

#include <ImGuiColorTextEdit/TextEditor.h>
#include <filesystem>
#include <set>

namespace ImGui { class FileBrowser; }

namespace jevois
{
  class GUIhelper;
  
  //! Helper enum for actions to execute after saving a config file \relates GUIeditor
  enum class EditorSaveAction { None, Reload, Reboot, RefreshMappings };

  //! Helper class to represent a GUIeditor file in a pull-down menu \relates GUIeditor
  struct EditorItem {
      std::filesystem::path filename;
      std::string displayname;
      EditorSaveAction action;
  };
  
  //! Editor panel for JeVois-Pro GUI
  /*! This panel allows one to select one of several files, and to edit it.
      \ingroup gui */
  class GUIeditor : public TextEditor
  {
    public:
      //! Constructor
      GUIeditor(GUIhelper * helper, std::string const & imguiid, std::vector<EditorItem> && fixeditems,
                std::string const & scanpath, std::string const & prefix, std::set<std::string> && extensions);

      //! Destructor
      virtual ~GUIeditor();

      //! Draw the editor into ImGui
      void draw();
      
      //! Load a file
      /*! If the load fails, we can either display failtxt in the editor and make it read-only, or, if failtxt is empty,
          assume that we want to create a new file (e.g., module's params.cfg) and hence set the editor read-write. */
      void loadFile(std::filesystem::path const & fn, std::string const & failtxt);

      //! Save a file
      void saveFile();

      //! Refresh list of files
      void refresh();

    private:
      GUIhelper * const itsHelper;
      std::string const itsId;
      
      std::vector<EditorItem> itsItems;
      size_t itsNumFixedItems;
      std::string itsScanPath;
      std::string const itsPrefix;
      std::set<std::string> const itsExtensions;

      int itsCurrentItem = 0;
      EditorSaveAction itsCurrentAction = EditorSaveAction::None;
      bool itsWantLoad = true;
      bool itsWantAction = false;
      bool itsOkToLoad = false;
      bool itsOverrideReloadModule = false;
      int itsNewItem = 0;
      std::filesystem::path itsFilename;
      std::unique_ptr<ImGui::FileBrowser> itsBrowser;
  };


  
} // namespace jevois

#endif // JEVOIS_PRO
