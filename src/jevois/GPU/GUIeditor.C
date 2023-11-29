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

// This is only available on JeVoisPro
#ifdef JEVOIS_PRO

#include <jevois/GPU/GUIeditor.H>
#include <jevois/GPU/GUIhelper.H>
#include <jevois/Core/Module.H>
#include <jevois/Core/Engine.H>
#include <imgui-filebrowser/imfilebrowser.h>
#include <fstream>

#include <jevois/Debug/Log.H>

// ##############################################################################################################
jevois::GUIeditor::GUIeditor(GUIhelper * helper, std::string const & imguiid,
                             std::vector<jevois::EditorItem> && fixeditems, std::string const & scanpath,
                             std::string const & prefix, std::set<std::string> && extensions) :
    TextEditor(), itsHelper(helper), itsId(imguiid), itsItems(fixeditems), itsNumFixedItems(fixeditems.size()),
    itsScanPath(scanpath), itsPrefix(prefix), itsExtensions(extensions),
    itsBrowser(new ImGui::FileBrowser(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir))
{
  TextEditor::SetSaveCallback([this]() { saveFile(); } ); // to enable Ctrl-S saving

  itsBrowser->SetTitle("Select a file to open or create...");
  itsBrowser->SetPwd(JEVOIS_SHARE_PATH);
  //itsBrowser->SetTypeFilters({ ".cfg", ".py", ".txt", ".C", ".H" });
}

// ##############################################################################################################
jevois::GUIeditor::~GUIeditor()
{ }

// ##############################################################################################################
void jevois::GUIeditor::refresh()
{
  // If we have an open file that is not one of our fixed items, we want to keep that one open:
  bool keep_current = false; EditorItem current_item;
  if (itsCurrentItem >= int(itsNumFixedItems)) { current_item = itsItems[itsCurrentItem]; keep_current = true; }
  
  // Remove all dynamic files, keep the fixed ones:
  itsItems.resize(itsNumFixedItems);

  // Do we have a CMakeLists for the current module?
  if (itsItems[0].filename == "*")
  {
    jevois::VideoMapping const & vm = itsHelper->engine()->getCurrentVideoMapping();
    if (std::filesystem::exists(vm.cmakepath()))
      itsItems.emplace_back(EditorItem {"#", "Module's CMakeLists.txt", EditorSaveAction::Compile } );
  }
  
  // Rescan recursively:
  for (auto const & dent : std::filesystem::recursive_directory_iterator(itsScanPath))
  {
    if (dent.is_regular_file())
    {
      std::filesystem::path const path = dent.path();
      
      // Check that the extension is one we want:
      if (itsExtensions.find(path.extension()) == itsExtensions.end()) continue;

      // Create an entry:
      itsItems.emplace_back(EditorItem { path, itsPrefix + path.string(), EditorSaveAction::Reload });
    }
  }

  // Keep the current item?
  if (keep_current) itsItems.emplace_back(std::move(current_item));
  
  // Add an entry for file browser / creation:
  itsItems.emplace_back(EditorItem { "**", "Browse / Create file...", EditorSaveAction::Reload });
  
  // Update index of current file:
  bool not_found = true;

  for (int i = 0; auto const & item : itsItems)
    if (item.filename == itsFilename)
    {
      itsCurrentItem = i;
      itsNewItem = i;
      not_found = false;
      break;
    }
    else ++i;

  // If the currently open file is not in our list anymore, load file 0:
  if (not_found)
  {
    itsNewItem = 0;
    itsWantLoad = true;
    // If we have some edits, we will ask to save, and then to possibly reload the module (or reboot, etc). Prevent
    // asking to reload:
    itsOverrideReloadModule = IsEdited();
  }
}

// ##############################################################################################################
void jevois::GUIeditor::draw()
{
  // Create combo entries for imgui:
  char const * items[itsItems.size()];
  for (int i = 0; EditorItem const & c : itsItems) items[i++] = c.displayname.c_str();

  // Check if the user is trying to select a different file:
  if (ImGui::Combo(("##"+itsId+"editorcombo").c_str(), &itsNewItem, items, itsItems.size())) itsWantLoad = true;
  
  // Want to load a new file? check if we need to save the current one first:
  if (itsWantLoad && itsWantAction == false)
  {
    if (IsEdited())
    {
      static int discard_edits_default = 0;
      int ret = itsHelper->modal("Discard edits?", "File was edited. Discard all edits? This cannot be undone.",
                                 &discard_edits_default, "Discard", "Save");
      switch (ret)
      {
      case 1: itsWantLoad = false; itsOkToLoad = true; break; // Discard selected
      case 2: saveFile(); /* itsWantAction = false; */ break; // save selected
      default: break;  // Need to wait
      }
    }
    else
    {
      itsWantLoad = false;
      itsOkToLoad = true;
    }
  }
  
  // Need to execute an action after a save?
  if (itsWantAction)
  {
    switch (itsItems[itsCurrentItem].action)
    {
      // --------------------------------------------------
    case jevois::EditorSaveAction::None:
      itsWantAction = false;
      break;
      
      // --------------------------------------------------
    case jevois::EditorSaveAction::Reload:
    {
      // Skip if override requested by refresh(), typically because we loaded a new module but a config file from the
      // old module was open so we asked to save, now we don't want to ask to reload again:
      if (itsOverrideReloadModule)
      {
        itsOverrideReloadModule = false;
        itsWantAction = false;
        itsOkToLoad = itsWantLoad;
        break;
      }

      // Ask whether to reload the module now or later:
      static int reload_default = 0;
      int ret = itsHelper->modal("Reload Module?", "Reload Machine Vision Module for changes to take effect?",
                                 &reload_default, "Reload", "Later");
      switch (ret)
      {
      case 1:  // Reload selected
        itsHelper->engine()->requestSetFormat(-1);
        itsWantAction = false;
        itsOkToLoad = itsWantLoad;
        break;
        
      case 2: // Later selected: we don't want action anymore
        itsWantAction = false;
        itsOkToLoad = itsWantLoad;
        break;

      default: break; // need to wait
      }
    }
    break;
    
    // --------------------------------------------------
    case jevois::EditorSaveAction::Reboot:
    {
      int ret = itsHelper->modal("Restart?", "Restart JeVois-Pro for changes to take effect?",
                                 nullptr, "Restart", "Later");
      switch (ret)
      {
      case 1: // Reboot selected
        itsHelper->engine()->reboot();
        itsWantAction = false;
        break;
        
      case 2: // Later selected: we don't want action anymore
        itsWantAction = false;
        break;

      default: break;  // Need to wait
      }
    }
    break;

    // --------------------------------------------------
    case jevois::EditorSaveAction::RefreshMappings:
    {
      itsHelper->engine()->reloadVideoMappings();
      itsWantAction = false;
    }
    break;

    // --------------------------------------------------
    case jevois::EditorSaveAction::Compile:
    {
      // Ask whether to compile the module now or later:
      static int compile_default = 0;
      int ret = itsHelper->modal("Compile Module?", "Compile Machine Vision Module for changes to take effect?",
                                 &compile_default, "Compile", "Later");
      switch (ret)
      {
      case 1:  // Compile selected
        itsHelper->startCompilation();
        itsWantAction = false;
        itsOkToLoad = itsWantLoad;
        break;
        
      case 2: // Later selected: we don't want action anymore
        itsWantAction = false;
        itsOkToLoad = itsWantLoad;
        break;
        
      default: break; // need to wait
      }
    }
    break;
    }
  }
  
  // Ready to load a new file?
  if (itsOkToLoad)
  {
    // Do we want to browse or create a new file?
    if (itsItems[itsNewItem].filename == "**")
    {
      ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xf0ffe0e0);
      
      if (itsBrowser->IsOpened() == false)
      {
        itsBrowser->Open();
        itsBrowser->Display();
      }
      else
      {
        itsBrowser->Display();

        if (itsBrowser->HasSelected())
        {
          std::filesystem::path const fn = itsBrowser->GetSelected();
          
          if (std::filesystem::exists(fn))
            loadFileInternal(fn, "Could not load " + fn.string()); // load with error and read-only on fail
          else
            loadFileInternal(fn, ""); // load with no error and read-write on fail (create new file)
          
          itsBrowser->Close();
          itsBrowser->Display();
        }
        
        // Clicking "Cancel" in the browser just closes the popup:
        if (itsBrowser->IsOpened() == false)
        {
          itsOkToLoad = false; // Record that we don't want to load anymore:
          itsNewItem = itsCurrentItem; // Snap back the combo selector to the current item
          itsBrowser->Close();
          itsBrowser->Display();
        }
      }
      ImGui::PopStyleColor();
    }
    else
    {
      // Load the file for itsNewItem:
      itsCurrentItem = itsNewItem;
      loadFileInternal(itsItems[itsCurrentItem].filename, "");
    }
  }
  
  // Add a pop-up menu for editor actions:
  bool const ro = IsReadOnly();
  ImGui::SameLine();
  if (ImGui::Button("...")) ImGui::OpenPopup("editor_actions");
  if (ImGui::BeginPopup("editor_actions"))
  {
    constexpr int ok = ImGuiSelectableFlags_None;
    constexpr int disa = ImGuiSelectableFlags_Disabled;

    if (ImGui::Selectable("Save   [Ctrl-S]", false, !ro && IsEdited() ? ok : disa)) saveFile();

    ImGui::Separator();

    if (ImGui::Selectable("Undo   [Ctrl-Z]", false, !ro && CanUndo() ? ok : disa)) Undo();
    if (ImGui::Selectable("Redo   [Ctrl-Y]", false, !ro && CanRedo() ? ok : disa)) Redo();
      
    ImGui::Separator();
      
    if (ImGui::Selectable("Copy   [Ctrl-C]", false, HasSelection() ? ok : disa)) Copy();
    if (ImGui::Selectable("Cut    [Ctrl-X]", false, !ro && HasSelection() ? ok : disa)) Cut();
    if (ImGui::Selectable("Delete [Del]", false, !ro && HasSelection() ? ok : disa)) Delete();
    if (ImGui::Selectable("Paste  [Ctrl-V]", false, !ro && ImGui::GetClipboardText()!=nullptr ? ok : disa)) Paste();
      
    ImGui::Separator();
      
    ImGui::Selectable("More shortcuts...", false, disa);
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("[Ctrl-A]          Select all\n"
                        "[PgUp/PgDn]       Move one page up/down\n"
                        "[Home]            Move to start of line\n"
                        "[End]             Move to end of line\n"
                        "[Ctrl-Home]       Move to start of file\n"
                        "[Ctrl-End]        Move to end of file\n"
                        "[Ctrl-Left/Right] Move left/right one word\n"
                        "[Ins]             Toggle overwrite mode\n"
                        "[Alt-Bksp]        Undo (same as [Ctrl-Z])\n"
                        "[Ctrl-Ins]        Copy (same as [Ctrl-C])\n"
                        "[Shift-Ins]       Paste (same as [Ctrl-V])\n"
                        "[Shift-Del]       Cut (same as [Ctrl-X])\n"
                        "[Shift-Cursor]    Select while moving cursor (up, down, left, right, home, end)\n"
                        "[Mouse-Drag]      Select with mouse\n"
                        );
    
    ImGui::EndPopup();
  }

  // Draw a save button if we are read/write:
  if (ro == false)
  {
    ImGui::SameLine();
    ImGui::TextUnformatted("   "); ImGui::SameLine();
    if (ImGui::Button("Save")) saveFile();
  }
  
  ImGui::Separator();

  // Render the editor in a child window so it can scroll correctly:
  auto cpos = GetCursorPosition();

  ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, GetTotalLines(),
              IsOverwrite() ? "Ovr" : "Ins",
              IsEdited() ? "*" : " ",
              GetLanguageDefinition().mName.c_str());
  
  Render("JeVois-Pro Editor");
}

// ##############################################################################################################
void jevois::GUIeditor::loadFile(std::filesystem::path const & fn)
{
  // Loading will happen in the main loop. Here we just create a new item:
  for (int i = 0; EditorItem const & item : itsItems)
    if (item.filename == fn) { itsNewItem = i; itsWantLoad = true; return; } else ++i;

  // Not already in our list of items, create a new one.  Add an entry for our new file. If it's compilable, then action
  // on save should be to compile, otherwise it should be to reload the module:
  EditorSaveAction action = EditorSaveAction::Reload;
  if (fn.filename() == "CMakeLists.txt")
    action = EditorSaveAction::Compile;
  else if (fn.filename() == "jevoispro-fan.service")
    action = EditorSaveAction::Reboot;
  else
  {
    std::string const ext = fn.extension().string();
    if (ext == ".C" || ext == ".H" || ext == ".cpp" || ext == ".hpp" || ext == ".c" || ext == ".h")
      action = EditorSaveAction::Compile;
  }
  itsItems.emplace_back(EditorItem { fn, "File " + fn.string(), action });
  
  itsNewItem = itsItems.size() - 1;
  itsWantLoad = true;
}

// ##############################################################################################################
void jevois::GUIeditor::loadFileInternal(std::filesystem::path const & fpath, std::string const & failt)
{
  std::filesystem::path fn = fpath; std::string failtxt = failt; bool special_path = false;
  
  if (fpath == "*")
  {
    // If filename is "*", replace by the module's source code name:
    jevois::VideoMapping const & vm = itsHelper->engine()->getCurrentVideoMapping();
    fn = vm.srcpath();
    failtxt = "Could not open Module's source code";
    special_path = true;
  }
  else if (fpath == "#")
  {
    // If filename is "#", replace by the module's CMakeLists.txt:
    jevois::VideoMapping const & vm = itsHelper->engine()->getCurrentVideoMapping();
    fn = vm.cmakepath();
    failtxt = "Could not open Module's CMakeLists.txt";
    special_path = true;
  }
  else if (fpath.is_relative())
  {
    // If path is relative, make it within the module's path (if any):
    auto m = itsHelper->engine()->module();
    if (m) fn = m->absolutePath(fpath);
    special_path = true;
  }

  if (fn != fpath) LINFO("Loading " << fn << " ... [" << fpath << ']'); else LINFO("Loading " << fn << " ...");

  bool got_it = false;
  EditorSaveAction action = EditorSaveAction::Reload;
    
  std::ifstream t(fn);
  if (t.good())
  {
    // Load the whole file and set it as our text:
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    SetText(str);

    // Is this a known file in our pull-down list? Otherwise we need to create a new item:
    for (int i = 0; EditorItem const & item : itsItems)
      if (item.filename == fpath) { itsCurrentItem = i; got_it = true; break; } else ++i;

    // Set the language, read-only, and possibly action according to file extension. C++/C source files are editable if
    // there is a CMakeLists.txt in the same directory (e.g., newly created or cloned module, excludes jevoisbase
    // modules), and then a compilation action will be triggered on save:
    if (fn.filename() == "CMakeLists.txt")
    {
      SetLanguageDefinition(TextEditor::LanguageDefinition::CMake());
      SetReadOnly(false);
      action = EditorSaveAction::Compile;
    }
    else
    {
      std::filesystem::path const ext = fn.extension();
      std::filesystem::path cmak = fn; cmak.remove_filename(); cmak /= "CMakeLists.txt";
      bool has_cmake = std::filesystem::exists(cmak);
      
      if (ext == ".py")
      {
        SetLanguageDefinition(TextEditor::LanguageDefinition::Python());
        SetReadOnly(false);
      }
      else if (ext == ".C" || ext == ".H" || ext == ".cpp" || ext == ".hpp")
      {
        SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
        SetReadOnly(! has_cmake);
        action = EditorSaveAction::Compile;
      }
      else if ( ext == ".c" || ext == ".h")
      {
        SetLanguageDefinition(TextEditor::LanguageDefinition::C());
        SetReadOnly(! has_cmake);
        action = EditorSaveAction::Compile;
      }
      else
      {
        // .cfg, .yaml, etc
        SetLanguageDefinition(TextEditor::LanguageDefinition::JeVoisCfg());
        SetReadOnly(false);
      }
    }
  }
  else
  {
    // Show the fail text:
    SetText(failtxt);
    if (failtxt.empty()) { LINFO("File " << fn << " not found -- CREATING NEW"); SetReadOnly(false); }
    else { LINFO("File " << fn << " not found."); SetReadOnly(true); }
  }

  // If this is a new file, add an item to our pull-down list:
  if (got_it == false && special_path == false)
  {
    itsItems.emplace_back(EditorItem { fn, "File " + fn.string(), action });
    itsCurrentItem = itsItems.size() - 1;
  }
  else if (fpath == "*") itsItems[itsCurrentItem].action = action; // Force compile action if needed on module's src
  
  // Remember the filename, for saveFile():
  itsFilename = fn;
  itsNewItem = itsCurrentItem;
  itsWantLoad = false;
  itsOkToLoad = false;
}

// ##############################################################################################################
void jevois::GUIeditor::saveFile()
{
  LINFO("Saving " << itsFilename << " ...");
  std::ofstream os(itsFilename);
  if (os.is_open() == false) { itsHelper->reportError("Cannot write " + itsFilename.string()); return; }

  std::string const txt = GetText();
  os << txt;

  // Mark as un-edited:
  SetEdited(false);

  // Delete modinfo.html if any, GUIhelper will re-compute it when the info tab is selected:
  std::filesystem::path mi = itsFilename.parent_path() / "modinfo.html";
  if (std::filesystem::exists(mi)) std::filesystem::remove(mi);

  // After a save, execute any required action like reload module, reboot, recompile, etc:
  itsWantAction = true;
}

#endif // JEVOIS_PRO
