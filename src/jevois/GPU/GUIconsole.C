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


#ifdef JEVOIS_PRO

#include <jevois/GPU/GUIconsole.H>
#include <imgui.h>
#include <imgui_internal.h>

// ##############################################################################################################
jevois::GUIconsole::GUIconsole(std::string const & instance) :
    jevois::UserInterface(instance)
{
  itsInputBuf[0] = '\0';
  /*
  // "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
  Commands.push_back("HELP");
  Commands.push_back("HISTORY");
  Commands.push_back("CLEAR");
  Commands.push_back("CLASSIFY");
  */
}

// ##############################################################################################################
jevois::GUIconsole::~GUIconsole()
{ }

// ##############################################################################################################
jevois::UserInterface::Type jevois::GUIconsole::type() const
{ return jevois::UserInterface::Type::GUI; }

// ##############################################################################################################
void jevois::GUIconsole::clear()
{
  std::lock_guard<std::mutex> _(itsDataMtx);
  itsData.clear();
}

// ##############################################################################################################
bool jevois::GUIconsole::readSome(std::string & str)
{
  std::lock_guard<std::mutex> _(itsDataMtx);
  if (itsLastInput.empty()) return false;
  str = itsLastInput;
  itsLastInput.clear();

  // Also keep a copy of the input for our display:
  itsData.push_back(std::make_pair(true, str));
  
  return true;
}

// ##############################################################################################################
void jevois::GUIconsole::writeString(std::string const & str)
{
  std::lock_guard<std::mutex> _(itsDataMtx);
  itsData.push_back(std::make_pair(false, str));
  while (itsData.size() > 10000) itsData.pop_front();
}

// ##############################################################################################################
static int TextEditCallbackStub(ImGuiInputTextCallbackData * data)
{
  jevois::GUIconsole * console = (jevois::GUIconsole *)data->UserData;
  return console->callback(data);
}

// ##############################################################################################################
void jevois::GUIconsole::draw()
{
  // Reserve enough left-over height for 1 separator + 1 input text
  const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                    ImGuiWindowFlags_HorizontalScrollbar);
  
  // Right click on the log to get a popup menu that can clear it:
  if (ImGui::BeginPopupContextWindow())
  {
    if (ImGui::Selectable("Clear")) clear();
    ImGui::EndPopup();
  }
  
  // Tighten spacing:
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

  // Colorize and draw each data line:
  std::lock_guard<std::mutex> _(itsDataMtx);
  for (auto const & p : itsData)
  {
    ImVec4 color; bool has_color = false;
    auto const & s = p.second;
 
    if (p.first)
    { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
    else
    {
      if (s == "OK") { color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); has_color = true; }
      else if (jevois::stringStartsWith(s, "DBG ")) { color = ImVec4(0.2f, 0.2f, 1.0f, 1.0f); has_color = true; }
      else if (jevois::stringStartsWith(s, "INF ")) { color = ImVec4(0.4f, 0.7f, 0.4f, 1.0f); has_color = true; }
      else if (jevois::stringStartsWith(s, "ERR ")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
      else if (jevois::stringStartsWith(s, "FTL ")) { color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); has_color = true; }
    }
    if (has_color) ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(s.c_str());
    if (has_color) ImGui::PopStyleColor();
  }

  static bool autoScroll = true;
  static bool scrollToBottom = true;

  if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) ImGui::SetScrollHereY(1.0f);
  scrollToBottom = false;
  
  ImGui::PopStyleVar();
  ImGui::EndChild();
  ImGui::Separator();
  
  // Command-line input:
  bool reclaim_focus = false;
  ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;

  // |
  //ImGuiInputTextFlags_CallbackCompletion 
  if (ImGui::InputTextWithHint("Input", "Type JeVois commands here...",
                               itsInputBuf, IM_ARRAYSIZE(itsInputBuf), input_text_flags,
                               &TextEditCallbackStub, this))
  {
    itsLastInput = itsInputBuf;
    itsInputBuf[0] = '\0';
    reclaim_focus = true;

    // On command input, we scroll to bottom even if AutoScroll==false
    scrollToBottom = true;
        
    // Delete from history if we already had this command:
    itsHistoryPos = -1;
    if (itsHistory.empty() == false)
      for (int i = int(itsHistory.size()) - 1; i >= 0; --i)
        if (itsHistory[i] == itsLastInput) { itsHistory.erase(itsHistory.begin() + i); break; }

    // Insert into history:
    itsHistory.push_back(itsLastInput);
    while (itsHistory.size() > 100) itsHistory.erase(itsHistory.begin());
  }
  
  // Auto-focus on window apparition
  ImGui::SetItemDefaultFocus();
  if (reclaim_focus) ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
}

// ##############################################################################################################
int jevois::GUIconsole::callback(ImGuiInputTextCallbackData * data)
{
  switch (data->EventFlag)
  {
    /*
  case ImGuiInputTextFlags_CallbackCompletion:
  {
    // Example of TEXT COMPLETION
    
    // Locate beginning of current word
    const char* word_end = data->Buf + data->CursorPos;
    const char* word_start = word_end;
    while (word_start > data->Buf)
    {
      const char c = word_start[-1];
      if (c == ' ' || c == '\t' || c == ',' || c == ';')
        break;
      word_start--;
    }
    
    // Build a list of candidates
    ImVector<const char*> candidates;
    for (int i = 0; i < Commands.Size; i++)
      if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
        candidates.push_back(Commands[i]);
    
    if (candidates.Size == 0)
    {
      // No match
      AddLog("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
    }
    else if (candidates.Size == 1)
    {
      // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
      data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
      data->InsertChars(data->CursorPos, candidates[0]);
      data->InsertChars(data->CursorPos, " ");
    }
    else
    {
      // Multiple matches. Complete as much as we can..
      // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
      int match_len = (int)(word_end - word_start);
      for (;;)
      {
        int c = 0;
        bool all_candidates_matches = true;
        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
          if (i == 0)
            c = toupper(candidates[i][match_len]);
          else if (c == 0 || c != toupper(candidates[i][match_len]))
            all_candidates_matches = false;
        if (!all_candidates_matches)
          break;
        match_len++;
      }
      
      if (match_len > 0)
      {
        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
      }
      
      // List matches
      AddLog("Possible matches:\n");
      for (int i = 0; i < candidates.Size; i++)
        AddLog("- %s\n", candidates[i]);
    }
    
    break;
  }
    */
  case ImGuiInputTextFlags_CallbackHistory:
  {
    // Example of HISTORY
    const int prev_history_pos = itsHistoryPos;
    if (data->EventKey == ImGuiKey_UpArrow)
    {
      if (itsHistoryPos == -1) itsHistoryPos = int(itsHistory.size()) - 1;
      else if (itsHistoryPos > 0) --itsHistoryPos;
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
      if (itsHistoryPos != -1 && ++itsHistoryPos >= int(itsHistory.size())) itsHistoryPos = -1;
    }
    
    // A better implementation would preserve the data on the current input line along with cursor position.
    if (prev_history_pos != itsHistoryPos)
    {
      std::string const & history_str = (itsHistoryPos >= 0) ? itsHistory[itsHistoryPos] : "";
      data->DeleteChars(0, data->BufTextLen);
      data->InsertChars(0, history_str.c_str());
    }
  }
  }
  return 0;
}

#endif // JEVOIS_PRO
