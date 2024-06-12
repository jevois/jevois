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

#ifdef JEVOIS_PRO

#include <jevois/GPU/GUIserial.H>

// ##############################################################################################################
jevois::GUIserial::~GUIserial()
{ }

// ##############################################################################################################
bool jevois::GUIserial::readSome(std::string & str)
{
  bool ret = jevois::Serial::readSome(str);

  if (ret)
  {
    // We received a complete string, keep a copy:
    std::lock_guard<std::mutex> _(itsDataMtx);
    itsData.push_back(std::make_pair(true, str));
  }
  
  return ret;
}

// ##############################################################################################################
void jevois::GUIserial::writeString(std::string const & str)
{
  {
    // Keep a copy:
    std::lock_guard<std::mutex> _(itsDataMtx);
    itsData.push_back(std::make_pair(false, str));
    while (itsData.size() > 1000) itsData.pop_front();
  }

  // Send it off over serial:
  jevois::Serial::writeString(str);
}

// ##############################################################################################################
void jevois::GUIserial::draw()
{
  // Keep this in sync with GUIconsole::draw() for aesthetic consistency

  static ImVec4 const col_from { 1.0f, 0.8f, 0.6f, 1.0f };
  static ImVec4 const col_to { 0.6f, 0.6f, 0.4f, 1.0f };
  static ImVec4 const col_to_ok { 0.2f, 1.0f, 0.2f, 1.0f };
  static ImVec4 const col_to_dbg { 0.2f, 0.2f, 1.0f, 1.0f };
  static ImVec4 const col_to_inf { 0.4f, 0.7f, 0.4f, 1.0f };
  static ImVec4 const col_to_err { 1.0f, 0.4f, 0.4f, 1.0f };
  static ImVec4 const col_to_ftl { 1.0f, 0.0f, 0.0f, 1.0f };
  
  ImGui::Text("Legend: "); ImGui::SameLine();
  ImGui::TextColored(col_from, "From Serial Device; "); ImGui::SameLine();
  ImGui::TextColored(col_to, "To Serial Device, with accents for "); ImGui::SameLine();
  ImGui::TextColored(col_to_ok, "OK, "); ImGui::SameLine();
  ImGui::TextColored(col_to_dbg, "DBG, "); ImGui::SameLine();
  ImGui::TextColored(col_to_inf, "INF, "); ImGui::SameLine();
  ImGui::TextColored(col_to_err, "ERR, "); ImGui::SameLine();
  ImGui::TextColored(col_to_ftl, "FTL");
  ImGui::Separator();
  
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  
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
    ImVec4 color;
    auto const & s = p.second;
 
    if (p.first)
      // Data we have read from serial device:
      color = col_from;
    else
    {
      // Data we have written to serial device:
      if (s == "OK") color = col_to_ok;
      else if (jevois::stringStartsWith(s, "DBG ")) color = col_to_dbg;
      else if (jevois::stringStartsWith(s, "INF ")) color = col_to_inf;
      else if (jevois::stringStartsWith(s, "ERR ")) color = col_to_err;
      else if (jevois::stringStartsWith(s, "FTL ")) color = col_to_ftl;
      else color = col_to;
    }
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(s.c_str());
    ImGui::PopStyleColor();
  }

  static bool autoScroll = true;
  static bool scrollToBottom = true;

  if (scrollToBottom || (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) ImGui::SetScrollHereY(1.0f);
  scrollToBottom = false;
  
  ImGui::PopStyleVar();
  ImGui::EndChild();
}

// ##############################################################################################################
void jevois::GUIserial::clear()
{
  std::lock_guard<std::mutex> _(itsDataMtx);
  itsData.clear();
}

#endif // JEVOIS_PRO
