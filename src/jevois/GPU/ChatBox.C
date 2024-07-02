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

#include <jevois/GPU/ChatBox.H>
#include <jevois/Util/Utils.H>
#include <jevois/Component/Component.H> // only for jevois::frameNum()
#include <imgui.h>
#include <imgui_internal.h>

// Keep this code in tight sync with jevois::GUIconsole

// ##############################################################################################################
jevois::ChatBox::ChatBox(std::string title) : itsTitle(title)
{
  itsInputBuf[0] = '\0';
}

// ##############################################################################################################
jevois::ChatBox::~ChatBox()
{ }

// ##############################################################################################################
void jevois::ChatBox::clear()
{
  itsData.clear();
}

// ##############################################################################################################
std::string jevois::ChatBox::get()
{
  std::string str;

  if (itsLastInput.empty() == false)
  {
    str = itsLastInput;
    
    // Also keep a copy of the input for our display:
    itsData.push_back(std::make_pair(true, itsLastInput));
    itsLastInput.clear();
  }

  return str;
}

// ##############################################################################################################
void jevois::ChatBox::writeString(std::string const & str)
{
  if (str.empty()) return;
  
  // If our last data was from the bot, start a new entry for the user:
  if (itsData.back().first) itsData.emplace_back(std::make_pair(false, std::string()));
  
  // Concatenate to last line or create a new one?
  size_t idx = str.find('\n');
  if (idx == str.npos)
    itsData.back().second += str;
  else
  {
    auto tok = jevois::split(str, "\\n");
    for (auto const & t : tok) itsData.emplace_back(std::make_pair(false, t));
  }
  
  while (itsData.size() > 10000) itsData.pop_front();
}

// ##############################################################################################################
void jevois::ChatBox::freeze(bool doit)
{
  itsFrozen = doit;
}

// ##############################################################################################################
static int TextEditCallbackStub(ImGuiInputTextCallbackData * data)
{
  jevois::ChatBox * console = (jevois::ChatBox *)data->UserData;
  return console->callback(data);
}

// ##############################################################################################################
void jevois::ChatBox::draw()
{
  ImGui::SetNextWindowPos(ImVec2(100, 200), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);

  // Keep this in sync with GUIserial::draw() for aesthetic consistency
  if (ImGui::Begin(itsTitle.c_str()))
  {
    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing() + 5;
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushTextWrapPos(ImGui::GetWindowSize().x - ImGui::GetFontSize() * 2.0f);

    // Right click on the log to get a popup menu that can clear it:
    if (ImGui::BeginPopupContextWindow())
    {
      if (ImGui::Selectable("Clear")) clear();
      ImGui::EndPopup();
    }
    
    // Tighten spacing:
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    
    // Colorize and draw each data line:
    for (auto const & p : itsData)
    {
      ImVec4 color; bool has_color = false;
      auto const & s = p.second;

      // Colors are RGBA
      if (p.first) { color = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); has_color = true; }
      /*
        else
        {
        if (s == "OK") { color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); has_color = true; }
        else if (jevois::stringStartsWith(s, "DBG ")) { color = ImVec4(0.2f, 0.2f, 1.0f, 1.0f); has_color = true; }
        else if (jevois::stringStartsWith(s, "INF ")) { color = ImVec4(0.4f, 0.7f, 0.4f, 1.0f); has_color = true; }
        else if (jevois::stringStartsWith(s, "ERR ")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
        else if (jevois::stringStartsWith(s, "FTL ")) { color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); has_color = true; }
        }
      */
      if (has_color) ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::TextUnformatted(s.c_str());
      if (has_color) ImGui::PopStyleColor();
    }
    
    // If user input is frozen, display some animation showing we are working, until response starts:
    if (itsFrozen && itsData.empty() == false && itsData.back().first)
    {
      int ndots = itsWaitState < 10 ? itsWaitState : 20 - itsWaitState;
      std::string msg = std::string(10-ndots, ' ') + std::string(ndots, '.') +
        " Working " + std::string(ndots, '.');
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
      ImGui::TextUnformatted(msg.c_str());
      ImGui::PopStyleColor();
      
      if (jevois::frameNum() % 10 == 0) { ++itsWaitState; if (itsWaitState > 20) itsWaitState = 0; }
    }
      
    static bool autoScroll = true;
    static bool scrollToBottom = true;
    
    if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) ImGui::SetScrollHereY(1.0f);
    scrollToBottom = false;
    
    ImGui::PopStyleVar();
    ImGui::PopTextWrapPos();
    ImGui::EndChild();
    ImGui::Separator();

    // Command-line input:
    std::string hint = "Type queries here...";
    int flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;
    if (itsFrozen)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      hint = "Working... Please wait...";
      flags |= ImGuiInputTextFlags_ReadOnly;
    }

    bool reclaim_focus = false;
    if (ImGui::InputTextWithHint("Input", hint.c_str(), itsInputBuf, IM_ARRAYSIZE(itsInputBuf),
                                 flags, &TextEditCallbackStub, this))
    {
      itsLastInput = itsInputBuf;
      itsInputBuf[0] = '\0';
      reclaim_focus = true;
      itsWaitState = 10; // reset the wait animation
      
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

    // Restore any grey out:
    if (itsFrozen)
    {
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
        
    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus) ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
  }
  ImGui::End();
}

// ##############################################################################################################
int jevois::ChatBox::callback(ImGuiInputTextCallbackData * data)
{
  if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
  {
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
    
    if (prev_history_pos != itsHistoryPos)
    {
      std::string const & history_str = (itsHistoryPos >= 0) ? itsHistory[itsHistoryPos] : "";
      data->DeleteChars(0, data->BufTextLen);
      data->InsertChars(0, history_str.c_str());
    }
  }
  return 0;
}
      
#endif // JEVOIS_PRO
