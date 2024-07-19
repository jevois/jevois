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

// for demo
#include <math.h>
#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS // Access to math operators
#include <imgui_internal.h>
#include <algorithm> // for tweak demo

#include <jevois/GPU/GUIhelper.H>
#include <jevois/GPU/GUIconsole.H>
#include <jevois/GPU/GPUimage.H>
#include <jevois/Core/Module.H>
#include <jevois/GPU/GUIeditor.H>
#include <jevois/GPU/GUIserial.H>
#include <jevois/Debug/SysInfo.H>
#include <jevois/Util/Utils.H>
#include <jevois/DNN/Utils.H>
#include <jevois/Debug/PythonException.H>
#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtx/euler_angles.hpp>
#include <fstream>
#include <set>

// ##############################################################################################################
jevois::GUIhelper::GUIhelper(std::string const & instance, bool conslock) :
    jevois::Component(instance), itsConsLock(conslock), itsBackend()
{
  // We defer OpenGL init to startFrame() so that all OpenGL code is in the same thread.

  // If auto-detecting screen size, get the framebuffer size here so we can use it later:
  if (winsize::get().width == 0)
  {
    // Query the framebuffer if no window size was given:
    int w = 1920, h = 1080;  // defaults in case we fail to read from framebuffer
    try
    {
      std::string const ws = jevois::getFileString("/sys/class/graphics/fb0/virtual_size");
      std::vector<std::string> const tok = jevois::split(ws, "\\s*[,;x]\\s*");
      if (tok.size() == 2) { w = std::stoi(tok[0]); h = std::stoi(tok[1]) / 2; } // Reported height is double...
      LINFO("Detected framebuffer size: " << w << 'x' << h);
    }
    catch (...) { } // silently ignore any errors

    winsize::set(cv::Size(w, h));
  }

  itsWindowTitle = "JeVois-Pro v" + std::string(JEVOIS_VERSION_STRING);

  // Create some config files for the config editor:
  std::vector<EditorItem> fixedcfg
    {
     { JEVOIS_ROOT_PATH "/config/videomappings.cfg", "JeVois videomappings.cfg", EditorSaveAction::RefreshMappings },
     { JEVOIS_ROOT_PATH "/config/initscript.cfg", "JeVois initscript.cfg", EditorSaveAction::Reboot },
     { "params.cfg", "Module's params.cfg", EditorSaveAction::Reload },
     { "script.cfg", "Module's script.cfg", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/models.yml",
       "JeVois models.yml DNN Zoo Root", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/opencv.yml",
       "JeVois opencv.yml DNN Zoo for OpenCV models", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/npu.yml",
       "JeVois npu.yml DNN Zoo for A311D NPU models", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/spu.yml",
       "JeVois spu.yml DNN Zoo for Hailo SPU models", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/tpu.yml",
       "JeVois tpu.yml DNN Zoo for Coral TPU models", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/vpu.yml",
       "JeVois vpu.yml DNN Zoo for Myriad-X VPU models", EditorSaveAction::Reload },
     { JEVOIS_ROOT_PATH "/share/dnn/ort.yml",
       "JeVois ort.yml DNN Zoo for ONNX-Runtime CPU models", EditorSaveAction::Reload },
    };

  // Create the config editor:
  itsCfgEditor.reset(new jevois::GUIeditor(this, "cfg", std::move(fixedcfg), JEVOIS_CUSTOM_DNN_PATH, "Custom DNN ",
                                           { ".yaml", ".yml" }));
  
  // Create some config files for the code editor:
  std::vector<EditorItem> fixedcode
    {
     { "*", "Module's source code", EditorSaveAction::Reload },
     // Gets added at runtime: { "#", "Module's CMakeLists.txt", EditorSaveAction::Compile },
    };

  // Create the code editor:
  itsCodeEditor.reset(new jevois::GUIeditor(this, "code", std::move(fixedcode), JEVOIS_PYDNN_PATH, "PyDNN ",
                                            { ".py", ".C", ".H", ".cpp", ".hpp", ".c", ".h", ".txt" }));
}

// ##############################################################################################################
jevois::GUIhelper::~GUIhelper()
{
  // In case a DNN get was in progress, wait for it while letting user know:
  JEVOIS_WAIT_GET_FUTURE(itsDnnGetFut);
}

// ##############################################################################################################
void jevois::GUIhelper::resetstate(bool modulechanged)
{
  itsEndFrameCalled = true;
  itsImages.clear();
  itsImages2.clear();
  itsLastDrawnImage = nullptr;
  itsLastDrawnTextLine = -1;
  itsIdle = true;
  itsIcon.clear();
  itsModName.clear();
  itsModDesc.clear();
  itsModAuth.clear();
  itsModLang.clear();
  itsModDoc.clear();

  if (modulechanged)
  {
    itsCfgEditor->refresh();
    itsCodeEditor->refresh();
  }

  {
    // Clear old error messages:
    std::lock_guard<std::mutex> _(itsErrorMtx);
    itsErrors.clear();
  }
  
  // Get the actual window/screen size:
  unsigned short w, h;
  itsBackend.getWindowSize(w, h);

  if (w == 0) LFATAL("Need to call startFrame() at least once first");
  
  float const fov_y = 45.0f;

  proj = glm::perspective(glm::radians(fov_y), float(w) / float(h), 1.0f, h * 2.0f);
  const_cast<float &>(pixel_perfect_z) = -float(h) / (2.0 * tan(fov_y * M_PI / 360.0));
#ifdef JEVOIS_PLATFORM
  // On platform, we need to translate a bit to avoid aliasing issues, which are problematic with our YUYV shader:
  proj = glm::translate(proj, glm::vec3(0.375f, 0.375f, 0.0f));
#endif
  view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, pixel_perfect_z));

  // Set the style (on first call):
  style::set(style::get());

  // Refresh whether we have a USB serial gadget loaded:
  itsUSBserial = ! engine()->getParamStringUnique("engine:usbserialdev").empty();

  // If the engine's current videomapping has a CMakeLists.txt, copy it to itsNewMapping to allow recompilation:
  jevois::VideoMapping const & m = engine()->getCurrentVideoMapping();
  if (std::filesystem::exists(m.cmakepath())) itsNewMapping = m;
}

// ##############################################################################################################
bool jevois::GUIhelper::startFrame(unsigned short & w, unsigned short & h)
{
  if (itsEndFrameCalled == false) LFATAL("You need to call endFrame() at the end of your process() function");
  
  // Get current window size, will be 0x0 if not initialized yet:
  itsBackend.getWindowSize(w, h);

  if (w == 0)
  {
    // Need to init the display:
    cv::Size const siz = winsize::get();
    bool const fs = fullscreen::get();
    LINFO("OpenGL init " << siz.width << 'x' << siz.height << (fs ? " fullscreen" : ""));
    itsBackend.init(siz.width, siz.height, fs, scale::get(), itsConsLock);
    rounding::set(int(rounding::get() * scale::get() + 0.499F));

    // Get the actual window size and update our param:
    unsigned short winw, winh; itsBackend.getWindowSize(winw, winh); winsize::set(cv::Size(winw, winh));
    winsize::freeze(true);
    fullscreen::freeze(true);

    // Reset the GUI:
    resetstate();
  }

  // Poll events:
  bool shouldclose = false; auto const now = std::chrono::steady_clock::now();
  if (itsBackend.pollEvents(shouldclose)) itsLastEventTime = now;

  if (shouldclose && allowquit::get())
  {
    LINFO("Closing down on user request...");
    engine()->quit();
  }

  // Start the frame on the backend:
  itsBackend.newFrame();

  // Check if we have been idle:
  if (itsIdleBlocked)
    itsIdle = false;
  else
  {
    float const hs = hidesecs::get();
    if (hs)
    {
      std::chrono::duration<float> elapsed = now - itsLastEventTime;
      itsIdle = (elapsed.count() >= hs);
    }
    else itsIdle = false;
  }
  
  itsEndFrameCalled = false;

  return itsIdle;
}

// ##############################################################################################################
bool jevois::GUIhelper::idle() const
{ return itsIdle; }

// ##############################################################################################################
void jevois::GUIhelper::onParamChange(jevois::gui::scale const & param, float const & newval)
{
  float oldval = param.get();
  if (newval == oldval) return;
  
  ImGui::GetStyle().ScaleAllSizes(newval / oldval);
  ImGui::GetIO().FontGlobalScale = newval;
  ImGui::GetStyle().MouseCursorScale = 2.0f; // do not scale the cursor, otherwise it disappears...

  // Also scale the window corner rounding, make it no more than 24:
  if (oldval) rounding::set(std::min(24, int(rounding::get() * newval / oldval + 0.499F)));
}

// ##############################################################################################################
void jevois::GUIhelper::onParamChange(jevois::gui::style const &, jevois::gui::GuiStyle const & newval)
{
  switch (newval)
  {
  case jevois::gui::GuiStyle::Dark:
    ImGui::StyleColorsDark();
    itsCfgEditor->SetPalette(TextEditor::GetDarkPalette());
    itsCodeEditor->SetPalette(TextEditor::GetDarkPalette());
    break;
    
  case jevois::gui::GuiStyle::Light:
    ImGui::StyleColorsLight();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.92f); // a bit transparent
    itsCfgEditor->SetPalette(TextEditor::GetLightPalette());
    itsCodeEditor->SetPalette(TextEditor::GetLightPalette());
    break;
    
  case jevois::gui::GuiStyle::Classic:
    ImGui::StyleColorsClassic();
    itsCfgEditor->SetPalette(TextEditor::GetRetroBluePalette());
    itsCodeEditor->SetPalette(TextEditor::GetRetroBluePalette());
    break;
  }
}

// ##############################################################################################################
void jevois::GUIhelper::onParamChange(jevois::gui::rounding const &, int const & newval)
{
  auto & s = ImGui::GetStyle();
  s.WindowRounding = newval;
  s.ChildRounding = newval;
  s.FrameRounding = newval;
  s.PopupRounding = newval;
  s.ScrollbarRounding = newval;
  s.GrabRounding = newval;
}

// ##############################################################################################################
bool jevois::GUIhelper::frameStarted() const
{ return (itsEndFrameCalled == false); }

// ##############################################################################################################
void jevois::GUIhelper::drawImage(char const * name, jevois::RawImage const & img, int & x, int & y,
                                  unsigned short & w, unsigned short & h, bool noalias, bool isoverlay)
{
  // We will use one image per v4l2 buffer:
  std::string const imgname = name + std::to_string(img.bufindex);

  // Get the image by name (will create a default-constructed one the first time a new name is used):
  auto & im = itsImages[imgname];

  // Set the new pixel data:
  im.set(img);

  // Draw it:
  im.draw(x, y, w, h, noalias, proj * view);

  // Remember which image was drawn last, used by i2d():
  if (isoverlay == false)
  {
    itsLastDrawnImage = & im;
    itsUsingScaledImage = false;
    itsLastDrawnTextLine = -1;
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawImage(char const * name, cv::Mat const & img, bool rgb, int & x, int & y,
                                  unsigned short & w, unsigned short & h, bool noalias, bool isoverlay)
{
  // Get the image by name (will create a default-constructed one the first time a new name is used):
  auto & im = itsImages[name];

  // Set the new pixel data:
  im.set(img, rgb);

  // Draw it:
  im.draw(x, y, w, h, noalias, proj * view);

  // Remember which image was drawn last, used by i2d():
  if (isoverlay == false)
  {
    itsLastDrawnImage = & im;
    itsUsingScaledImage = false;
    itsLastDrawnTextLine = -1;
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawInputFrame(char const * name, jevois::InputFrame const & frame, int & x, int & y,
                                       unsigned short & w, unsigned short & h, bool noalias, bool casync)
{
  // We will use one image per v4l2 buffer:
  jevois::RawImage const img = frame.get(casync);
  std::string const imgname = name + std::to_string(img.bufindex);
  
  // Get the image by name (will create a default-constructed one the first time a new name is used):
  auto & im = itsImages[imgname];

  // Set the new pixel data using DMABUF acceleration. This will boil down to a no-op unless image size or format has
  // changed (including the first time we set it):
  im.set(frame, itsBackend.getDisplay());

  // Draw it:
  im.draw(x, y, w, h, noalias, proj * view);

  // Remember which image was drawn last, used by i2d():
  itsLastDrawnImage = & im;
  itsUsingScaledImage = frame.hasScaledImage();
  itsLastDrawnTextLine = -1;
  if (itsUsingScaledImage)
  {
    jevois::RawImage img2 = frame.get2(casync);
    itsScaledImageFacX = float(img.width) / float(img2.width);
    itsScaledImageFacY = float(img.height) / float(img2.height);
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawInputFrame2(char const * name, jevois::InputFrame const & frame, int & x, int & y,
                                       unsigned short & w, unsigned short & h, bool noalias, bool casync)
{
  // We will use one image per v4l2 buffer:
  jevois::RawImage const img = frame.get2(casync);
  std::string const imgname = name + std::to_string(img.bufindex);
  
  // Get the image by name (will create a default-constructed one the first time a new name is used):
  auto & im = itsImages2[imgname];

  // Set the new pixel data using DMABUF acceleration. This will boil down to a no-op unless image size or format has
  // changed (including the first time we set it):
  im.set2(frame, itsBackend.getDisplay());

  // Draw it:
  im.draw(x, y, w, h, noalias, proj * view);

  // Remember which image was drawn last, used by i2d():
  itsLastDrawnImage = & im;
  itsUsingScaledImage = false;
  itsLastDrawnTextLine = -1;
}

// ##############################################################################################################
void jevois::GUIhelper::onParamChange(gui::twirl const &, float const & newval)
{
  // Compute alpha so that more twirl is also more faded (lower alpha). Here, we want to mask out the whole display,
  // including all GUI drawings except the banner. Hence we will just draw a full-screen semi-transparent rectangle in
  // endFrame(), using our computed alpha value:
  itsGlobalAlpha = std::abs(1.0F - newval / 15.0F);  
  
  // Note: we only twirl the display images (itsImages), not the processing images (itsImages2):
  for (auto & ip : itsImages) ip.second.twirl(newval);
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::i2d(ImVec2 p, char const * name)
{
  // Find the image:
  GPUimage * img;
  
  if (name == nullptr)
  {
    if (itsLastDrawnImage == nullptr) throw std::range_error("You need to call drawImage() or drawInputFrame() first");
    img = itsLastDrawnImage;
  }
  else
  {
    std::string nstr = name;
    auto itr = itsImages.find(nstr);
    if (itr == itsImages.end())
    {
      // Add a zero in case we are dealing with a camera frame (drawImage() and drawInputFrame() add suffixes, but all
      // image buffers are the same size):
      itr = itsImages.find(nstr + '0');
      if (itr == itsImages.end()) throw std::range_error("No previously drawn image with name [" + nstr + "] found");
    }
    img = & itr->second;
  }

  // Adjust size if using scaled image:
  if (itsUsingScaledImage) { p.x *= itsScaledImageFacX; p.y *= itsScaledImageFacY; }

  // Delegate:
  return img->i2d(p);
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::i2d(float x, float y, char const * name)
{ return i2d(ImVec2(x, y), name); }

// ##############################################################################################################
ImVec2 jevois::GUIhelper::i2ds(ImVec2 p, char const * name)
{
  // Find the image:
  GPUimage * img;
  
  if (name == nullptr)
  {
    if (itsLastDrawnImage == nullptr) throw std::range_error("You need to call drawImage() or drawInputFrame() first");
    img = itsLastDrawnImage;
  }
  else
  {
    std::string nstr = name;
    auto itr = itsImages.find(nstr);
    if (itr == itsImages.end())
    {
      // Add a zero in case we are dealing with a camera frame (drawImage() and drawInputFrame() add suffixes, but all
      // image buffers are the same size):
      itr = itsImages.find(nstr + '0');
      if (itr == itsImages.end()) throw std::range_error("No previously drawn image with name [" + nstr + "] found");
    }
    img = & itr->second;
  }

  // Adjust size if using scaled image:
  if (itsUsingScaledImage) { p.x *= itsScaledImageFacX; p.y *= itsScaledImageFacY; }

  // Delegate:
  return img->i2ds(p);
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::i2ds(float x, float y, char const * name)
{ return i2ds(ImVec2(x, y), name); }

// ##############################################################################################################
void jevois::GUIhelper::drawLine(float x1, float y1, float x2, float y2, ImU32 col)
{
  ImGui::GetBackgroundDrawList()->AddLine(i2d(x1, y1), i2d(x2, y2), col, linethick::get());
}

// ##############################################################################################################
void jevois::GUIhelper::drawRect(float x1, float y1, float x2, float y2, ImU32 col, bool filled)
{
  auto dlb = ImGui::GetBackgroundDrawList();
  ImVec2 const tl = i2d(x1, y1);
  ImVec2 const br = i2d(x2, y2);

  if (filled) dlb->AddRectFilled(tl, br, applyFillAlpha(col));

  dlb->AddRect(tl, br, col, 0.0F, ImDrawCornerFlags_All, linethick::get());
}

// ##############################################################################################################
void jevois::GUIhelper::drawPolyInternal(ImVec2 const * pts, size_t npts, ImU32 col, bool filled)
{
  auto dlb = ImGui::GetBackgroundDrawList();
  float const thick = linethick::get();

  if (filled) dlb->AddConvexPolyFilled(pts, npts, applyFillAlpha(col));

  if (npts > 1)
  {
    for (size_t i = 0; i < npts - 1; ++i)
    {
      ImVec2 const & p1 = pts[i];
      ImVec2 const & p2 = pts[i + 1];

      // Draw line if it is not collapsed to a single point:
      if (p1.x != p2.x || p1.y != p2.y) dlb->AddLine(p1, p2, col, thick);
    }
    dlb->AddLine(pts[npts - 1], pts[0], col, thick); // close the polygon
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawPoly(std::vector<cv::Point> const & pts, ImU32 col, bool filled)
{
  size_t const npts = pts.size();
  if (npts < 3) return;
  
  ImVec2 iv[npts+1];
  for (int i = 0; auto const & p : pts) iv[i++] = i2d(p.x, p.y);
  
  drawPolyInternal(iv, npts, col, filled);
}

// ##############################################################################################################
void jevois::GUIhelper::drawPoly(std::vector<cv::Point2f> const & pts, ImU32 col, bool filled)
{
  size_t const npts = pts.size();
  if (npts < 3) return;
  
  ImVec2 iv[npts];
  for (int i = 0; auto const & p : pts) iv[i++] = i2d(p.x, p.y);

  drawPolyInternal(iv, npts, col, filled);
}

// ##############################################################################################################
void jevois::GUIhelper::drawPoly(cv::Mat const & pts, ImU32 col, bool filled)
{
  float const * ptr = pts.ptr<float>(0);
  
  switch (pts.type())
  {
  case CV_32F:
  {
    if (pts.rows == 1 || pts.cols == 1)
    {
      // x,y,x,y,x,y... on one row or column of length 2N for N vertices:
      int n = std::max(pts.rows, pts.cols);
      if (n % 1) LFATAL("Incorrect input " << jevois::dnn::shapestr(pts) << ": odd number of coordinates");
      if (std::min(pts.rows, pts.cols) < 3) return;
      n /= 2; ImVec2 p[n]; for (int i = 0; i < n; ++i) { p[i] = i2d(ptr[0], ptr[1]); ptr += 2; }
      drawPolyInternal(p, n, col, filled);
    }
    else if (pts.rows == 2)
    {
      // first row: x,x,x,x,x... second row: y,y,y,y,y... with N columns:
      if (pts.cols < 3) return;
      ImVec2 p[pts.cols]; for (int i = 0; i < pts.cols; ++i) { p[i] = i2d(ptr[0], ptr[pts.cols]); ++ptr; }
      drawPolyInternal(p, pts.cols, col, filled);
    }
    else if (pts.cols == 2)
    {
      // x,y; x,y; x,y; ... for N rows of size 2 each:
      if (pts.rows < 3) return;
      ImVec2 p[pts.rows]; for (int i = 0; i < pts.rows; ++i) { p[i] = i2d(ptr[0], ptr[1]); ptr += 2; }
      drawPolyInternal(p, pts.rows, col, filled);
    }
    else LFATAL("Incorrect input " << jevois::dnn::shapestr(pts) << ": 32F must be 1x2N, 2xN, 2Nx1, or Nx2");
  }
  break;

  case CV_32FC2:
  {
    if (pts.rows == 1)
    {
      // (x,y),(x,y),... for N columns of pairs:
      if (pts.cols < 3) return;
      ImVec2 p[pts.cols]; for (int i = 0; i < pts.cols; ++i) { p[i] = i2d(ptr[0], ptr[1]); ptr += 2; }
      drawPolyInternal(p, pts.cols, col, filled);
    }
    else if (pts.cols == 1)
    {
      // (x,y); (x,y); (x,y); ... for N rows of pairs
      if (pts.rows < 3) return;
      ImVec2 p[pts.rows]; for (int i = 0; i < pts.rows; ++i) { p[i] = i2d(ptr[0], ptr[1]); ptr += 2; }
      drawPolyInternal(p, pts.rows, col, filled);
    }
    else LFATAL("Incorrect input " << jevois::dnn::shapestr(pts) << ": 32FC2 must be Nx1 or 1xN");
  }
  break;
  
  default: LFATAL("Incorrect input " << jevois::dnn::shapestr(pts) << ": must be 32F or 32FC2");
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawCircle(float x, float y, float r, ImU32 col, bool filled)
{
  auto dlb = ImGui::GetBackgroundDrawList();

  ImVec2 const center = i2d(x, y);
  float const rad = i2ds(r, 0).x;

  if (filled) dlb->AddCircleFilled(center, rad, applyFillAlpha(col), 0);

  dlb->AddCircle(center, rad, col, 0, linethick::get());
}

// ##############################################################################################################
void jevois::GUIhelper::drawText(float x, float y, char const * txt, ImU32 col)
{
  ImGui::GetBackgroundDrawList()->AddText(i2d(x, y), col, txt);
}

// ##############################################################################################################
void jevois::GUIhelper::drawText(float x, float y, std::string const & txt, ImU32 col)
{
  drawText(x, y, txt.c_str(), col);
}

// ##############################################################################################################
ImU32 jevois::GUIhelper::applyFillAlpha(ImU32 col) const
{
  unsigned char alpha = (col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT;
  alpha = (unsigned char)(fillalpha::get() * alpha);
  return (col & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::iline(int line, char const * name)
{
  if (line == -1) line = ++itsLastDrawnTextLine; else itsLastDrawnTextLine = line;
  ImVec2 p = i2d(0, 0, name);
  p.x += 5.0F;
  p.y += 5.0F + (ImGui::GetFontSize() + 5.0F) * line;
  return p;
}

// ##############################################################################################################
void jevois::GUIhelper::itext(char const * txt, ImU32 const & col, int line)
{
  ImU32 const c = (col == IM_COL32_BLACK_TRANS) ? ImU32(overlaycolor::get()) : col;
  ImGui::GetBackgroundDrawList()->AddText(iline(line), c, txt);
}

// ##############################################################################################################
void jevois::GUIhelper::itext(std::string const & txt, ImU32 const & col, int line)
{
  itext(txt.c_str(), col, line);
}

// ##############################################################################################################
void jevois::GUIhelper::iinfo(jevois::InputFrame const & inframe, std::string const & fpscpu,
                              unsigned short winw, unsigned short winh)
{
  unsigned short ww, wh;
  if (winw == 0 || winh == 0) itsBackend.getWindowSize(ww, wh); else { ww = winw; wh = winh; }

  jevois::RawImage const & inimg = inframe.get();
  std::string cam2str;
  if (inframe.hasScaledImage())
  {
    jevois::RawImage const & inimg2 = inframe.get2();
    cam2str += jevois::sformat(" + %s:%dx%d", jevois::fccstr(inimg2.fmt).c_str(), inimg2.width, inimg2.height);
  }
  std::string const msg = jevois::sformat("%s, Camera: %s:%dx%d%s, Display: RGBA:%dx%d", fpscpu.c_str(),
                                          jevois::fccstr(inimg.fmt).c_str(), inimg.width, inimg.height,
                                          cam2str.c_str(), ww, wh);
  
  ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, wh-10-ImGui::GetFontSize()), overlaycolor::get(), msg.c_str());
}

// ##############################################################################################################
void jevois::GUIhelper::releaseImage(char const * name)
{
  auto itr = itsImages.find(name);
  if (itr != itsImages.end()) itsImages.erase(itr);
}

// ##############################################################################################################
void jevois::GUIhelper::releaseImage2(char const * name)
{
  auto itr = itsImages2.find(name);
  if (itr != itsImages2.end()) itsImages2.erase(itr);
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::d2i(ImVec2 p, char const * name)
{
  // Find the image:
  GPUimage * img;
  float fx = 1.0F, fy = 1.0F;

  if (name == nullptr)
  {
    if (itsLastDrawnImage == nullptr) throw std::range_error("You need to call drawImage() or drawInputFrame() first");
    img = itsLastDrawnImage;
    if (itsUsingScaledImage) { fx = 1.0F / itsScaledImageFacX; fy = 1.0F / itsScaledImageFacY; }
  }
  else
  {
    std::string nstr = name;
    auto itr = itsImages.find(nstr);
    if (itr == itsImages.end())
    {
      // Add a zero in case we are dealing with a camera frame (drawImage() and drawInputFrame() add suffixes, but all
      // image buffers are the same size):
      itr = itsImages.find(nstr + '0');
      if (itr == itsImages.end()) throw std::range_error("No previously drawn image with name [" + nstr + "] found");
    }
    img = & itr->second;
  }

  // Delegate:
  ImVec2 ret = img->d2i(p);
  ret.x *= fx; ret.y *= fy;
  return ret;
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::d2i(float x, float y, char const * name)
{ return d2i(ImVec2(x, y), name); }

// ##############################################################################################################
ImVec2 jevois::GUIhelper::d2is(ImVec2 p, char const * name)
{
  // Find the image:
  GPUimage * img;
  float fx = 1.0F, fy = 1.0F;
  
  if (name == nullptr)
  {
    if (itsLastDrawnImage == nullptr) throw std::range_error("You need to call drawImage() or drawInputFrame() first");
    img = itsLastDrawnImage;
    if (itsUsingScaledImage) { fx = 1.0F / itsScaledImageFacX; fy = 1.0F / itsScaledImageFacY; }
  }
  else
  {
    std::string nstr = name;
    auto itr = itsImages.find(nstr);
    if (itr == itsImages.end())
    {
      // Add a zero in case we are dealing with a camera frame (drawImage() and drawInputFrame() add suffixes, but all
      // image buffers are the same size):
      itr = itsImages.find(nstr + '0');
      if (itr == itsImages.end()) throw std::range_error("No previously drawn image with name [" + nstr + "] found");
    }
    img = & itr->second;
  }

  // Delegate:
  ImVec2 ret = img->d2is(p);
  ret.x *= fx; ret.y *= fy;
  return ret;
}

// ##############################################################################################################
ImVec2 jevois::GUIhelper::d2is(float x, float y, char const * name)
{ return d2is(ImVec2(x, y), name); }

// ##############################################################################################################
void jevois::GUIhelper::endFrame()
{
  // Decide whether to show mouse cursor based on idle state:
  ImGui::GetIO().MouseDrawCursor = ! itsIdle;

  // Draw our JeVois GUI on top of everything:
  if (itsIdle == false) drawJeVoisGUI();

  // If compiling, draw compilation window:
  if (itsCompileState != CompilationState::Idle) compileModule();
  
  // Do we want to fade out the whole display?
  if (itsGlobalAlpha != 1.0F)
  {
    if (itsGlobalAlpha < 0.0F || itsGlobalAlpha > 1.0F)
    {
      LERROR("Invalid global alpha " << itsGlobalAlpha << " -- RESET TO 1.0");
      itsGlobalAlpha = 1.0F;
    }
    else
    {
      auto dlb = ImGui::GetBackgroundDrawList();
      cv::Size const ws = winsize::get();
      dlb->AddRectFilled(ImVec2(0, 0), ImVec2(ws.width, ws.height), ImColor(0.0F, 0.0F, 0.0F, 1.0F - itsGlobalAlpha));
    }
  }
  
  // Do we have a demo banner to show?
  if (itsBannerTitle.empty() == false)
  {
    ImGui::SetNextWindowPos(ImVec2(800, 500));
    ImGui::SetNextWindowSize(ImVec2(1000, 400));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0xf0e0ffe0); // ABGR
    ImGui::Begin("JeVois-Pro Demo Mode", nullptr, ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(1.5F);
    ImGui::TextUnformatted(" ");
    ImGui::TextUnformatted(itsBannerTitle.c_str());
    ImGui::TextUnformatted(" ");
    ImGui::SetWindowFontScale(1.0F);

    int wrap = ImGui::GetWindowSize().x - ImGui::GetFontSize() * 2.0f;
    ImGui::PushTextWrapPos(wrap);
    ImGui::TextUnformatted(itsBannerMsg.c_str());
    ImGui::PopTextWrapPos();

    ImGui::SetCursorPosY(ImGui::GetWindowSize().y - ImGui::CalcTextSize("X").y * 2.3F);
    ImGui::Separator();
    
    if (ImGui::Button("Skip to Next Demo")) engine()->nextDemo();
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine(600);
    if (ImGui::Button("Abort Demo Mode")) engine()->abortDemo();
    ImGui::End();
    ImGui::PopStyleColor();
  }
  
  // Render everything and swap buffers:
  itsBackend.render();

  itsEndFrameCalled = true;
}

// ##############################################################################################################
void jevois::GUIhelper::drawJeVoisGUI()
{
  // Set window size applied only on first use ever, otherwise from imgui.ini:
  ImGui::SetNextWindowPos(ImVec2(920, 358), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(941, 639), ImGuiCond_FirstUseEver);
  
  if (ImGui::Begin(itsWindowTitle.c_str(), nullptr /* no closing */))//, ImGuiWindowFlags_MenuBar))
  {
    //drawMenuBar();
    drawModuleSelect();
    
    //ImGui::Text("Camera + Process + Display: %.2f fps", ImGui::GetIO().Framerate);
    ImGui::Separator();
    
    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
    {
      
      if (ImGui::BeginTabItem("Info"))
      {
        drawInfo();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("Parameters"))
      {
        drawParameters();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("Console"))
      {
        drawConsole();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("Camera"))
      {
        drawCamCtrls();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("Config"))
      {
        itsCfgEditor->draw();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("Code"))
      {
        itsCodeEditor->draw();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("System"))
      {
        drawSystem();
        ImGui::EndTabItem();
      }
      
      if (ImGui::BeginTabItem("Tweaks"))
      {
        drawTweaks();
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
    ImGui::End();
  }
  else ImGui::End(); // do not draw anything if window is collapsed

  // Show style editor window, imgui demo, etc if user requested it:
  if (itsShowStyleEditor)
  {
    ImGui::Begin("GUI Style Editor", &itsShowStyleEditor);
    ImGui::ShowStyleEditor();
    ImGui::End();
  }
  if (itsShowAppMetrics) ImGui::ShowMetricsWindow(&itsShowAppMetrics);
  if (itsShowImGuiDemo) ImGui::ShowDemoWindow(&itsShowImGuiDemo);

  // Show Hardware serial monitor if desired:
  if (itsShowHardSerialWin)
  {
    // Set window size applied only on first use ever, otherwise from imgui.ini:
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

    // Open or display window until user closes it:
    ImGui::Begin("Hardware 4-pin Serial Monitor", &itsShowHardSerialWin);
    try { engine()->getComponent<jevois::GUIserial>("serial")->draw(); }
    catch (...)
    {
      ImGui::TextUnformatted("No Hardware serial port found!");
      ImGui::Separator();
      ImGui::TextUnformatted("Check engine:serialdev parameter, and");
      ImGui::TextUnformatted("that engine:serialmonitors is true.");
    }
    ImGui::End();
  }

  // Show USB serial monitor if desired:
  if (itsShowUsbSerialWin)
  {
    // Set window size applied only on first use ever, otherwise from imgui.ini:
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);

    // Open or display window until user closes it:
    ImGui::Begin("USB Serial Monitor", &itsShowUsbSerialWin);
    try { engine()->getComponent<jevois::GUIserial>("usbserial")->draw(); }
    catch (...)
    {
      ImGui::TextUnformatted("No USB serial port found!");
      ImGui::Separator();
      ImGui::TextUnformatted("Check engine:usbserialdev parameter, and");
      ImGui::TextUnformatted("that engine:serialmonitors is true.");
    }
    ImGui::End();
  }

  // Draw an error popup, if any exception was received through reportError():
  drawErrorPopup();
}

// ##############################################################################################################
void jevois::GUIhelper::drawMenuBar()
{
  if (ImGui::BeginMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      if (ImGui::MenuItem("Quit")) engine()->quit();
      
      //ShowExampleMenuFile();
      ImGui::EndMenu();
    }
    /*
    if (ImGui::BeginMenu("Machine Vision Module"))
    {
      auto e = engine();

      // Start with the JeVois-Pro mappings:
      size_t idx = 0;
      e->foreachVideoMapping([&idx,&e](jevois::VideoMapping const & m) {
                               if (m.ofmt == JEVOISPRO_FMT_GUI && ImGui::MenuItem(m.str().c_str()))
                                 e->requestSetFormat(idx++); else ++idx;
                             });
      ImGui::Separator();

      // Then the compatibility JeVois modules:
      idx = 0;
      e->foreachVideoMapping([&idx,&e](jevois::VideoMapping const & m) {
                               if (m.ofmt != 0 && m.ofmt != JEVOISPRO_FMT_GUI && ImGui::MenuItem(m.str().c_str()))
                                 e->requestSetFormat(idx++); else ++idx;
                             });
      
      ImGui::Separator();
      // Finally the no-USB modules: FIXME - those currently kill the gui since no startFrame() is emitted
      idx = 0;
      e->foreachVideoMapping([&idx,&e](jevois::VideoMapping const & m) {
                               if (m.ofmt == 0 && ImGui::MenuItem(m.str().c_str()))
                                 e->requestSetFormat(idx++); else ++idx;
                             });
      
      ImGui::EndMenu();
    }
    */
    if (ImGui::BeginMenu("Tools"))
    {
      ImGui::MenuItem("ImGui Style Editor", NULL, &itsShowStyleEditor);
      ImGui::MenuItem("ImGui Metrics/Debugger", NULL, &itsShowAppMetrics);
      ImGui::MenuItem("ImGui Demo/Doc", NULL, &itsShowImGuiDemo);
 
      ImGui::EndMenu();
    }
    
    ImGui::EndMenuBar();
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawModuleSelect()
{
  static std::map<std::string, size_t> mods;
  auto e = engine();
  static std::string currstr;

  // Refresh the list of modules if needed:
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Module:");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(6 * ImGui::GetFontSize() + 5);
  if (ImGui::Combo("##typemachinevisionmodule", &itsVideoMappingListType, "Pro/GUI\0Legacy\0Headless\0\0")
      || currstr.empty() || itsRefreshVideoMappings)
  {
    // Recompute mods for the selected type:
    itsRefreshVideoMappings = false;
    mods.clear();
    
    switch (itsVideoMappingListType)
    {
    case 0:
    {
      // JeVois-Pro GUI mappings:
      size_t idx = 0;
      e->foreachVideoMapping([&idx](jevois::VideoMapping const & m) {
                               if (m.ofmt == JEVOISPRO_FMT_GUI) mods[m.menustr()] = idx;
                               ++idx;
                             });
    }
    break;

    case 1:
    {
      // Legacy JeVois mappings:
      size_t idx = 0;
      e->foreachVideoMapping([&idx](jevois::VideoMapping const & m) {
                               if (m.ofmt != 0 && m.ofmt != JEVOISPRO_FMT_GUI) mods[m.menustr()] = idx;
                               ++idx;
                             });
    }
    break;

    case 2:
    {
      // Headless modules
      size_t idx = 0;
      e->foreachVideoMapping([&idx](jevois::VideoMapping const & m) {
                               if (m.ofmt == 0) mods[m.menustr()] = idx;
                               ++idx;
                             });
    }
    break;

    default: LFATAL("Internal error, itsVideoMappingListType=" << itsVideoMappingListType);
    }

    // Refresh our display string:
    currstr = e->getCurrentVideoMapping().menustr().c_str();
  }

  // Draw the module selector and allow selection:
  ImGui::SameLine();

  if (ImGui::BeginCombo("##machinevisionmodule", currstr.c_str()))
  {
    for (auto const & m : mods)
    {
      bool is_selected = false;
      if (ImGui::Selectable(m.first.c_str(), is_selected))
      {
        e->requestSetFormat(m.second);
        currstr.clear();
      }
        
    }
    ImGui::EndCombo();
  }  
}

// ##############################################################################################################
void jevois::GUIhelper::drawInfo()
{
  std::shared_ptr<jevois::Module> m = engine()->module();
  if (m)
  {
    // Get the icon and display it:
    if (itsIcon.loaded() == false)
      try { itsIcon.load(m->absolutePath("icon.png")); }
      catch (...)
      {
        // Load a default C++ or Python icon:
        LERROR("This module has no icon -- USING DEFAULT");
        try
        {
          if (engine()->getCurrentVideoMapping().ispython) itsIcon.load(JEVOIS_SHARE_PATH "/icons/py.png");
          else itsIcon.load(JEVOIS_SHARE_PATH "/icons/cpp.png");
        }
        catch (...)
        {
          // Defaults icons not found, just use a blank:
          cv::Mat blank(32, 32, CV_8UC4, 0);
          itsIcon.load(blank);
        }
      }
    
    if (itsIcon.loaded())
    {
      int const siz = ImGui::CalcTextSize("      ").x;
      itsIcon.draw(ImGui::GetCursorScreenPos(), ImVec2(siz, siz));
    }

    // Get the html doc if we have not yet parsed it:
    if (itsModName.empty())
    {
      std::filesystem::path fname = m->absolutePath("modinfo.html");
      std::ifstream ifs(fname);
      if (ifs.is_open() == false)
      {
        // No modinfo file, maybe we can create one if we have the source:
        LINFO("Recomputing module's modinfo.html ...");
        jevois::VideoMapping const & vm = engine()->getCurrentVideoMapping();
        try
        {
          std::string const cmdout =
            jevois::system("cd " + m->absolutePath().string() + " && "
                           "JEVOIS_SRC_ROOT=none jevoispro-modinfo " + vm.modulename, true);
          
          if (! std::filesystem::exists(fname))
            throw std::runtime_error("Failed to create " + vm.modinfopath() + ": " + cmdout);
        }
        catch (...) { itsModName = vm.modulename; itsModAuth = "Cannot read file: " + fname.string(); }
        // If we did not throw, itsModName is still empty, but now modinfo.html exists. Will load it next time.
      }
      else
      {
        int state = 0;
        for (std::string s; std::getline(ifs, s); )
          switch (state)
          {
          case 0: // Looking for module display name
          {
            std::string const str = jevois::extractString(s, "<td class=modinfoname>", "</td>");
            if (str.empty() == false) { itsModName = str; ++state; }
            break;
          }
          
          case 1: // Looking for short synopsis
          {
            std::string const str = jevois::extractString(s, "<td class=modinfosynopsis>", "</td>");
            if (str.empty() == false) { itsModDesc = str; ++state; }
            break;
          }
          
          case 2: // Looking for author info
          {
            std::string const str = jevois::extractString(s, "<table class=modinfoauth width=100%>", "</table>");
            if (str.empty() == false)
            {
              itsModAuth = jevois::join(jevois::split(str, "<[^<]*>"), " ").substr(2); // remove HTML tags
              ++state;
            }
            break;
          }
          
          case 3: // Looking for language info
          {
            std::string const str = jevois::extractString(s, "<table class=moduledata>", "</table>");
            if (str.empty() == false)
            {
              itsModLang = jevois::join(jevois::split(str, "<[^<]*>"), " "); // remove HTML tags
              auto tok = jevois::split(itsModLang, "&nbsp;");
              if (tok.size() >= 3)
              {
                if (jevois::stringStartsWith(tok[2], "C++")) itsModLang = "Language: C++";
                else itsModLang = "Language: Python"; 
              }
              else itsModLang = "Language: Unknown";
              ++state;
            }
            break;
          }
          
          case 4: // Looking for main doc start
          {
            std::string const str = jevois::extractString(s, "<td class=modinfodesc>", "");
            if (str.empty() == false)
            {
              std::string str2 = jevois::extractString(str, "<div class=\"textblock\">", "");
              str2 = jevois::join(jevois::split(str2, "<[^<]*>"), " ");
              size_t idx = str2.find_first_not_of(" "); if (idx != str2.npos) str2 = str2.substr(idx);
              itsModDoc.emplace_back(str2);
              ++state;
            }
            break;
          }
          
          case 5: // Extracting main doc until its end marker is encountered
          {
            if (s == "</div></td></tr>") ++state;
            else
            {
              std::string ss = jevois::join(jevois::split(s, "<[^<]*>"), " ");
              std::string prefix;
              
              if (s.find("/h1>") != s.npos || s.find("/h2>") != s.npos || s.find("/h3>") != s.npos) prefix = "* ";
              if (s.find("<li>") != s.npos) prefix = "- ";
              ss = prefix + ss;

              // Finally fix multiple spaces:
              // https://stackoverflow.com/questions/8362094/replace-multiple-spaces-with-one-space-in-a-string
              std::string::iterator new_end =
                std::unique(ss.begin(), ss.end(), [](char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); });
              ss.erase(new_end, ss.end());
              size_t idx = ss.find_first_not_of(" "); if (idx != ss.npos) ss = ss.substr(idx);
              itsModDoc.push_back(ss);
            }
          }
          
          default: break;
          }
        ifs.close();
      }
    }

    // Display the doc:
    ImGui::TextUnformatted(("        " + itsModName).c_str());
    ImGui::TextUnformatted(("        " + itsModDesc).c_str());
    ImGui::TextUnformatted(("        " + itsModAuth + " " + itsModLang).c_str());
    ImGui::TextUnformatted("   ");

    int wrap = ImGui::GetCursorPos().x + ImGui::GetWindowSize().x - ImGui::GetFontSize() * 2.0f;
    if (wrap < 200) wrap = 200;
    ImGui::PushTextWrapPos(wrap);

    bool show = true;
    for (std::string const & s : itsModDoc)
    {
      // Create collapsible header and get its collapsed status:
      if (jevois::stringStartsWith(s, "* "))
        show = ImGui::CollapsingHeader(s.c_str() + 2, ImGuiTreeNodeFlags_DefaultOpen);
      else if (show)
      {
        // If header not collapsed, show data:
        if (jevois::stringStartsWith(s, "- ")) { ImGui::Bullet(); ImGui::TextUnformatted(s.c_str() + 2); }
        else ImGui::TextUnformatted(s.c_str());
      }
    }
    ImGui::PopTextWrapPos();
  }
  else
    ImGui::TextUnformatted("No JeVois Module currently loaded.");
}

// ##############################################################################################################
void jevois::GUIhelper::setparstr(std::string const & descriptor, std::string const & val)
{
  try { engine()->setParamStringUnique(descriptor, val); }
  catch (...) { reportError(jevois::warnAndIgnoreException()); }
  // drawGUI() will call drawErrorPopup() which will show the popup
}

// ##############################################################################################################
void jevois::GUIhelper::drawParameters()
{
  static bool show_frozen = true; static bool show_system = false;

  toggleButton("Show Frozen Parameters", &show_frozen);
  ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Show System Parameters").x - 30.0f);
  toggleButton("Show System Parameters", &show_system);

  jevois::Engine * e = engine();
  jevois::Component * c; if (show_system) c = e; else c = e->module().get();

  // Stop here if we want to show module params but we have no module:
  if (c == nullptr) { ImGui::TextUnformatted("No module loaded."); return; }

  // Record any ambiguous parameter names, so we can prefix them by component name:
  std::set<std::string> pnames, ambig;

  // Get all the parameter summaries:
  std::map<std::string /* categ */, std::vector<jevois::ParameterSummary>> psm;
  c->foreachParam([this, &psm, &pnames, &ambig](std::string const &, jevois::ParameterBase * p) {
                    jevois::ParameterSummary psum = p->summary();
                    if (pnames.insert(psum.name).second == false) ambig.insert(psum.name);
                    psm[psum.category].push_back(std::move(psum)); } );
  
  // Stop here if no params to display:
  if (psm.empty()) { ImGui::Text("This module has no parameters."); return; }
  
  // Create a collapsing header for each parameter category:
  int widgetnum = 0; // we need a unique ID for each widget
  float maxlen = 0.0f;
  for (auto const & pp : psm)
  {
    // Do not even show a header if all params under it are frozen and we do not want to show frozen params:
    if (show_frozen == false)
    {
      bool all_frozen = true;
      for (auto const & ps : pp.second) if (ps.frozen == false) { all_frozen = false; break; }
      if (all_frozen) continue;
    }

    // Show a header for this category:
    if (ImGui::CollapsingHeader(pp.first.c_str()))
    {
      ImGui::Columns(3, "parameters");
      
      // Create a widget for each param:
      for (auto const & ps : pp.second)
      {
        // Skip if frozen and we do not want to show frozen params:
        if (ps.frozen && show_frozen == false) continue;
        
        // We need a unique ID for each ImGui widget, and we will use no visible widget name:
        static char wname[16]; snprintf(wname, 16, "##p%d", widgetnum);
        bool rst = true; // will set to false if we do not want a reset button
        
        // Start with parameter name and a tooltip with its descriptor:
        ImGui::AlignTextToFramePadding();
        std::string nam = ps.name;
        if (ambig.contains(nam))
        {
          // Here we are only going to disambiguate by the owning component name:
          auto tok = jevois::split(ps.descriptor, ":");
          if (tok.size() >= 2) nam = tok[tok.size()-2] + ':' + nam;
        }
        
        ImGui::TextUnformatted(nam.c_str());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", ps.descriptor.c_str());
        maxlen = std::max(maxlen, ImGui::CalcTextSize(nam.c_str()).x);
        
        // Add a tooltip:
        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        std::string const & vtype = ps.valuetype;
        if (jevois::stringStartsWith(ps.validvalues, "None:"))
          helpMarker(ps.description.c_str(), ("Parameter type: " + vtype).c_str());
        else
          helpMarker(ps.description.c_str(), ("Parameter type: " + vtype).c_str(),
                     ("Allowed values: " + ps.validvalues).c_str());
        
        // Now a widget for the parameter:
        ImGui::NextColumn();
        bool const is_uchar = (vtype == "unsigned char");
        bool const is_int = (vtype == "short" || vtype == "int" || vtype == "long int" || vtype == "long long int");
        bool const is_uint = (is_uchar || vtype == "unsigned short" || vtype == "unsigned int" || 
                              vtype == "unsigned long int" || vtype == "unsigned long long int" || vtype == "size_t");
        bool const is_real = (vtype == "float" || vtype == "double" || vtype == "long double");
        
        // Grey out the item if it is disabled:
        int textflags = ImGuiInputTextFlags_EnterReturnsTrue;
        if (ps.frozen)
        {
          ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
          ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
          textflags |= ImGuiInputTextFlags_ReadOnly;
        }
        
        // ----------------------------------------------------------------------
        if (jevois::stringStartsWith(ps.validvalues, "List:["))
        {
          std::string vals = ps.validvalues.substr(6, ps.validvalues.size() - 7);
          auto vv = jevois::split(vals, "\\|");
          
          if (vv.empty() == false)
          {
            // Find the index of the current value:
            int index = 0; for (auto const & v : vv) if (v == ps.value) break; else ++index;

            // Draw a combobox widget:
            if (combo(wname, vv, index)) setparstr(ps.descriptor, vv[index]);
          }
        }
        // ----------------------------------------------------------------------
        else if (is_uchar || jevois::stringStartsWith(ps.validvalues, "Range:["))
        {
          if (is_uchar)
          {
            // For unsigned char, use a slider:
            unsigned long val; jevois::paramStringToVal(ps.value, val);
            std::string rng = ps.validvalues.substr(7, ps.validvalues.size() - 8);
            
            // We may or not have a range spec, if not, use 0..255:
            if (rng.empty())
            {
              long mi = 0, ma = 255;
              if (ImGui::SliderScalar(wname, ImGuiDataType_U64, &val, &mi, &ma))
                setparstr(ps.descriptor, std::to_string(val));
            }
            else
            {
              jevois::Range<unsigned long> r; jevois::paramStringToVal(rng, r);
              if (ImGui::SliderScalar(wname, ImGuiDataType_U64, &val, &r.min(), &r.max()))
                setparstr(ps.descriptor, std::to_string(val));
            }
          }
          else if (is_uint)
          {
            // For uint that has a range specified, use a slider:
            std::string rng = ps.validvalues.substr(7, ps.validvalues.size() - 8);
            jevois::Range<unsigned long> r; jevois::paramStringToVal(rng, r);
            unsigned long val; jevois::paramStringToVal(ps.value, val);
            if (ImGui::SliderScalar(wname, ImGuiDataType_U64, &val, &r.min(), &r.max()))
              setparstr(ps.descriptor, std::to_string(val));
          }
          else if (is_int)
          {
            // For int that has a range specified, use a slider:
            std::string rng = ps.validvalues.substr(7, ps.validvalues.size() - 8);
            jevois::Range<long> r; jevois::paramStringToVal(rng, r);
            long val; jevois::paramStringToVal(ps.value, val);
            if (ImGui::SliderScalar(wname, ImGuiDataType_S64, &val, &r.min(), &r.max()))
              setparstr(ps.descriptor, std::to_string(val));
          }
          else if (is_real)
          {
            // For real with a range specified, use a slider:
            std::string rng = ps.validvalues.substr(7, ps.validvalues.size() - 8);
            jevois::Range<double> r; jevois::paramStringToVal(rng, r);
            double val; jevois::paramStringToVal(ps.value, val);
            if (ImGui::SliderScalar(wname, ImGuiDataType_Double, &val, &r.min(), &r.max()))
              setparstr(ps.descriptor, std::to_string(val));
          }
          else
          {
            // For more complex types, just allow free typing, parameter will do the checking:
            char buf[256]; strncpy(buf, ps.value.c_str(), sizeof(buf)-1);
            if (ImGui::InputText(wname, buf, sizeof(buf), textflags))
              setparstr(ps.descriptor, buf);
          }
        }
        // ----------------------------------------------------------------------
        else if (vtype == "jevois::Range<unsigned char>")
        {
          // For a range parameter, use a double drag:
          jevois::Range<unsigned char> val; jevois::paramStringToVal(ps.value, val);
          int mi = val.min(), ma = val.max();
          if (ImGui::DragIntRange2(wname, &mi, &ma, 0, 0, 255, "Min: %d", "Max: %d"))
            setparval(ps.descriptor, jevois::Range<unsigned char>(mi, ma));
        }
        // ----------------------------------------------------------------------
        else if (vtype == "bool")
        {
          bool val; jevois::paramStringToVal(ps.value, val);
          if (ImGui::Checkbox(wname, &val)) setparval(ps.descriptor, val);
        }
        // ----------------------------------------------------------------------
        else if (vtype == "ImColor")
        {
          ImColor val; jevois::paramStringToVal(ps.value, val);
          if (ImGui::ColorEdit4(wname, (float *)&val, ImGuiColorEditFlags_AlphaPreview)) setparval(ps.descriptor, val);
        }
        // ----------------------------------------------------------------------
        else
        {
          // User will type in some value, parameter will check it:
          char buf[256]; strncpy(buf, ps.value.c_str(), sizeof(buf)-1);
          if (ImGui::InputText(wname, buf, sizeof(buf), textflags))
            setparstr(ps.descriptor, buf);
        }
        
        // Possibly add a reset button:
        if (rst)
        {
          static char rname[18]; snprintf(rname, 18, "Reset##%d", widgetnum);
          ImGui::SameLine();
          if (ImGui::Button(rname)) setparstr(ps.descriptor, ps.defaultvalue);
        }
        
        // Restore any grey out:
        if (ps.frozen)
        {
          ImGui::PopItemFlag();
          ImGui::PopStyleVar();
        }
        
        // Ready for next row:
        ImGui::NextColumn(); ++widgetnum;
      }

      // Back to single column before the next param categ:
      ImGui::Columns(1);
    }
  }

  if (maxlen)
  {
    ImGui::Columns(3, "parameters");
    ImGui::SetColumnWidth(0, maxlen + ImGui::CalcTextSize("XX").x);
    ImGui::SetColumnWidth(1, ImGui::CalcTextSize(" (?) ").x);
    ImGui::SetColumnWidth(2, 2000);
    ImGui::Columns(1);
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawConsole()
{
  jevois::Engine * e = engine();

  // Start with toggle buttons for serlog and serout:
  bool slusb = false, slhard = false, schanged = false;
  auto sl = e->getParamValUnique<jevois::engine::SerPort>("engine:serlog");
  switch (sl)
  {
  case jevois::engine::SerPort::None: slusb = false; slhard = false; break;
  case jevois::engine::SerPort::All: slusb = true; slhard = true; break;
  case jevois::engine::SerPort::Hard: slusb = false; slhard = true; break;
  case jevois::engine::SerPort::USB: slusb = true; slhard = false; break;
  }
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Log messages:"); ImGui::SameLine();
  if (itsUSBserial == false) // grey out USB button if no driver
  {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    if (toggleButton("USB##serlogu", &slusb)) schanged = true;
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
#ifdef JEVOIS_PLATFORM
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    { ImGui::BeginTooltip(); ImGui::Text("Disabled - enable USB serial in the System tab"); ImGui::EndTooltip(); }
#else
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    { ImGui::BeginTooltip(); ImGui::Text("Disabled - not available on host"); ImGui::EndTooltip(); }
#endif
  }
  else if (toggleButton("USB##serlogu", &slusb)) schanged = true;
  
  ImGui::SameLine(); if (toggleButton("Hard##serlogh", &slhard)) schanged = true;
  ImGui::SameLine(); toggleButton("Cons##serlogc", &itsSerLogEnabled);
  ImGui::SameLine(0, 50);

  bool sousb = false, sohard = false;
  auto so = e->getParamValUnique<jevois::engine::SerPort>("engine:serout");
  switch (so)
  {
  case jevois::engine::SerPort::None: sousb = false; sohard = false; break;
  case jevois::engine::SerPort::All: sousb = true; sohard = true; break;
  case jevois::engine::SerPort::Hard: sousb = false; sohard = true; break;
  case jevois::engine::SerPort::USB: sousb = true; sohard = false; break;
  }
  ImGui::Text("Module output:"); ImGui::SameLine();
  if (itsUSBserial == false) // grey out USB button if no driver
  {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    if (toggleButton("USB##seroutu", &sousb)) schanged = true;
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
#ifdef JEVOIS_PLATFORM
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    { ImGui::BeginTooltip(); ImGui::Text("Disabled - enable USB serial in the System tab"); ImGui::EndTooltip(); }
#else
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    { ImGui::BeginTooltip(); ImGui::Text("Disabled - not available on host"); ImGui::EndTooltip(); }
#endif
  }
  else if (toggleButton("USB##seroutu", &sousb)) schanged = true;
  ImGui::SameLine(); if (toggleButton("Hard##serouth", &sohard)) schanged = true;
  ImGui::SameLine(); toggleButton("Cons##seroutc", &itsSerOutEnabled);

  if (schanged)
  {
    if (slusb)
    {
      if (slhard) e->setParamValUnique("engine:serlog", jevois::engine::SerPort::All);
      else e->setParamValUnique("engine:serlog", jevois::engine::SerPort::USB);
    }
    else
    {
      if (slhard) e->setParamValUnique("engine:serlog", jevois::engine::SerPort::Hard);
      else e->setParamValUnique("engine:serlog", jevois::engine::SerPort::None);
    }

    if (sousb)
    {
      if (sohard) e->setParamValUnique("engine:serout", jevois::engine::SerPort::All);
      else e->setParamValUnique("engine:serout", jevois::engine::SerPort::USB);
    }
    else
    {
      if (sohard) e->setParamValUnique("engine:serout", jevois::engine::SerPort::Hard);
      else e->setParamValUnique("engine:serout", jevois::engine::SerPort::None);
    }
  }

  // Now a combo for serstyle:
  auto m = dynamic_cast<jevois::StdModule *>(e->module().get());
  if (m)
  {
    auto sp = m->getParamValUnique<jevois::module::SerStyle>("serstyle");
    int idx = 0; for (auto const & v : jevois::module::SerStyle_Values) if (v == sp) break; else ++idx;

    ImGui::SameLine();
    if (combo("serstyle", jevois::module::SerStyle_Strings, idx))
      try { e->setParamValUnique("serstyle", jevois::module::SerStyle_Values[idx]); }
      catch (...) { jevois::warnAndIgnoreException(); }
  }
  ImGui::Separator();

  // Now a console:
  auto c = e->getComponent<jevois::GUIconsole>("guiconsole");
  if (c) c->draw();
}

// ##############################################################################################################
bool jevois::GUIhelper::serlogEnabled() const
{ return itsSerLogEnabled; }

// ##############################################################################################################
bool jevois::GUIhelper::seroutEnabled() const
{ return itsSerOutEnabled; }

// ##############################################################################################################
void jevois::GUIhelper::drawCamCtrls()
{
  engine()->drawCameraGUI();
}

// ##############################################################################################################
int jevois::GUIhelper::modal(std::string const & title, char const * text, int * default_val,
                             char const * b1txt, char const * b2txt)
{
  // Handle optional default_val pointer:
  int ret = 0; int * retptr = default_val ? default_val : &ret;
  
  // Do we want to just return the default value?
  if (*retptr == 1 || *retptr == 2) return *retptr;
  
  // Open the modal if needed, and remember it:
  if (itsOpenModals.find(title) == itsOpenModals.end())
  {
    ImGui::OpenPopup(title.c_str());
    itsOpenModals.insert(title);
  }

  // Display the modal and get any button clicks:
  bool dont_ask_me_next_time = (*retptr == 3);
  
  if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(text); ImGui::TextUnformatted(" ");
    ImGui::Separator();
    if (default_val)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
      ImGui::PushStyleColor(ImGuiCol_FrameBg, 0xf0ffe0e0); // ABGR - in light theme, can't see the box
      ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
      ImGui::PopStyleColor();
      ImGui::PopStyleVar();
    }
    float const b1w = std::max(120.0F, ImGui::CalcTextSize(b1txt).x + 20.0F);
    if (ImGui::Button(b1txt, ImVec2(b1w, 0))) ret = 1;
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    float const b2w = std::max(120.0F, ImGui::CalcTextSize(b2txt).x + 20.0F);
    if (ImGui::Button(b2txt, ImVec2(b2w, 0))) ret = 2; 
    ImGui::EndPopup();
  }
  
  // Close the modal if button clicked:
  if (ret == 1 || ret == 2)
  {
    ImGui::CloseCurrentPopup();
    itsOpenModals.erase(title);
    if (dont_ask_me_next_time) *retptr = ret; // remember the choice as new default choice
  }
  else *retptr = dont_ask_me_next_time ? 3 : 0; // propagate checkbox status
  
  return ret;
}

// ##############################################################################################################
bool jevois::GUIhelper::combo(std::string const & name, std::vector<std::string> const & items, int & selected_index)
{
  return ImGui::Combo(name.c_str(), &selected_index,
                      [](void * vec, int idx, const char ** out_text)
                      {
                        auto & ve = *static_cast<std::vector<std::string>*>(vec);
                        if (idx < 0 || idx >= static_cast<int>(ve.size())) return false;
                        *out_text = ve.at(idx).c_str();
                        return true;
                      },
                      const_cast<void *>(static_cast<void const *>(&items)), items.size());
}

// ##############################################################################################################
void jevois::GUIhelper::newModEntry(char const * wname, std::string & str, char const * desc,
                                    char const * hint, char const * hlp)
{
  static char buf[256];
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(desc);
  ImGui::NextColumn();
  helpMarker(desc, hlp);
  ImGui::NextColumn();
  strncpy(buf, str.c_str(), sizeof(buf)-1);
  if (ImGui::InputTextWithHint(wname, hint, buf, sizeof(buf))) str = buf;
  ImGui::NextColumn();
}

// ##############################################################################################################
void jevois::GUIhelper::drawSystem()
{
  float const fontw = ImGui::GetFontSize();
  
  static int refresh = 1;
  static std::string cpu, mem, ver;
  static size_t npu, tpu, vpu, spu; static int fan;
  if (--refresh == 0)
  {
    refresh = 60;
    cpu = jevois::getSysInfoCPU();
    mem = jevois::getSysInfoMem();
    ver = jevois::getSysInfoVersion();
    npu = jevois::getNumInstalledNPUs();
    tpu = jevois::getNumInstalledTPUs();
    vpu = jevois::getNumInstalledVPUs();
    spu = jevois::getNumInstalledSPUs();
    fan = jevois::getFanSpeed();
  }
  ImGui::Text("JeVois-Pro v%s -- %s", JEVOIS_VERSION_STRING, ver.c_str());
  ImGui::Text(cpu.c_str());
  ImGui::Text(mem.c_str());
  ImGui::Text("NPU: %d, TPU: %d, VPU: %d, SPU: %d. Fan: %d%%", npu, tpu, vpu, spu, fan);
  ImGui::Separator();
  
  // #################### Create new module:
  drawNewModuleForm();
  ImGui::Separator();

  // #################### Open serial monitors:
  if (ImGui::Button("Open Hardware serial monitor...")) itsShowHardSerialWin = true;
  ImGui::SameLine();
  if (ImGui::Button("Open USB serial monitor...")) itsShowUsbSerialWin = true;
  ImGui::Separator();

  // #################### ping:
  static std::string pingstr;
  static int showping = 0;
  if (ImGui::Button("Ping jevois.usc.edu"))
  {
    std::string ret = jevois::system("/usr/bin/ping -c 1 -w 2 jevois.usc.edu");
    std::vector<std::string> rvec = jevois::split(ret, "\n");
    if (rvec.size() < 2) reportError("Unable to ping jevois.usc.edu");
    else { pingstr = rvec[1]; showping = 60; }
  }
  if (showping)
  {
    ImGui::SameLine();
    ImGui::Text(pingstr.c_str());
    --showping;
  }

  ImGui::Separator();
  
  // #################### dnnget:
  static std::string zip;
  static std::string donestr;
  static int state = 0;

  ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
  if (state) flags |= ImGuiInputTextFlags_ReadOnly;
  ImGui::SetNextItemWidth(fontw * 6.0F);
  char buf[5] = { };
  if (ImGui::InputText("Load Custom DNN", buf, sizeof(buf), flags)) state = 1;
  ImGui::SameLine();
  
  switch (state)
  {
  case 0:
    ImGui::Text(" ");
    break;

  case 1:
  {
    ImGui::Text("-- Downloading...");
    zip = std::string(buf) + ".zip";
    itsDnnGetFut =
      jevois::async_little([]()
                           {
                             return jevois::system("/usr/bin/curl " JEVOIS_CUSTOM_DNN_URL "/" + zip + " -o "
                                                   JEVOIS_CUSTOM_DNN_PATH "/" + zip, true);
                           });
    state = 2;
  }
  break;
    
  case 2:
  {
    if (itsDnnGetFut.valid() == false) { reportError("Unknown error while loading custom DNN"); state = 0; break; }
    ImGui::Text("-- Downloading...");
    if (itsDnnGetFut.wait_for(std::chrono::microseconds(100)) == std::future_status::ready)
    {
      itsDnnGetFut.get();
      
      // Check that the file exists:
      std::ifstream ifs(JEVOIS_CUSTOM_DNN_PATH "/" + zip);
      if (ifs.is_open() == false)
      {
        reportError("Failed to download. Check network connectivity and available disk space.");
        state = 0;
        break;
      }
      itsDnnGetFut =
        jevois::async_little([]()
                             {
                               return jevois::system("/usr/bin/unzip -o " JEVOIS_CUSTOM_DNN_PATH "/" + zip +
                                                     " -d " JEVOIS_CUSTOM_DNN_PATH, true);
                             });
      state = 3;
    }
  }
  break;
  
  case 3:
  {
    if (itsDnnGetFut.valid() == false) { reportError("Unknown error while unpacking custom DNN"); state = 0; break; }
    ImGui::Text("-- Installing...");
    if (itsDnnGetFut.wait_for(std::chrono::microseconds(100)) == std::future_status::ready)
    {
      std::string ret = itsDnnGetFut.get();
      std::vector<std::string> rvec = jevois::split(ret, "\n");
      if (rvec.size() > 2 && jevois::stringStartsWith(rvec[1], "  End-of-central-directory signature not found"))
        donestr = "-- Invalid file, check DNN download key.";
      else
        donestr = "-- Done. Reload model zoo to take effect.";
      jevois::system("/bin/rm " JEVOIS_CUSTOM_DNN_PATH "/" + zip, true);
      state = 4;
    }
  }
  break;
  
  case 200:
    ImGui::Text(" ");
    state = 0;
    break;
    
  default:
    ImGui::Text(donestr.c_str());
    ++state;
  }
  ImGui::Separator();

#ifdef JEVOIS_PLATFORM
  // #################### boot mode:
  static int bootmode = 0;
  ImGui::AlignTextToFramePadding();
  ImGui::Text("On boot, start:");
  ImGui::SameLine();
  if (ImGui::Combo("##onboot", &bootmode, "(no change)\0JeVois-Pro\0Ubuntu Console\0Ubuntu Graphical\0\0"))
  {
    switch (bootmode)
    {
    case 0: break;
    case 2: jevois::system("systemctl --no-reload set-default multi-user.target"); break;
    case 3: jevois::system("systemctl --no-reload set-default graphical.target"); break;
    default: jevois::system("systemctl --no-reload set-default jevoispro.target"); break;
    }
    jevois::system("sync");
  }
  ImGui::Separator();

  // #################### gadget serial:
  static bool gserial = false;
  try { gserial = (0 != std::stoi(jevois::getFileString(JEVOISPRO_GSERIAL_FILE))); } catch (...) { }
  if (ImGui::Checkbox("Enable serial outputs/logs over mini-USB (on next reboot)", &gserial))
  {
    std::ofstream ofs(JEVOISPRO_GSERIAL_FILE);
    ofs << (gserial ? 1 : 0) << std::endl;
    ofs.close();
    jevois::system("sync");
  }
  ImGui::Separator();
      
  // #################### Edit fan settings:
  static bool show_fan_modal = false;
  if (ImGui::Button("Edit fan settings"))
  {
    itsCfgEditor->loadFile("/lib/systemd/system/jevoispro-fan.service");
    show_fan_modal = true;
  }

  if (show_fan_modal)
  {
    static int doit_default = 0;
    int ret = modal("Ready to edit", "File will now be loaded in the Config tab of the main window and "
                    "ready to edit.\nPlease switch to the Config tab in the main window.\n\n"
                    "When you save it, we will reboot the camera.",
                    &doit_default, "Ok", "Thanks");
    switch (ret)
    {
    case 1: show_fan_modal = false; break;
    case 2: show_fan_modal = false; break;
    default: break;  // Need to wait
    }
  }

  ImGui::Separator();

#endif

}

// ##############################################################################################################
namespace
{
  unsigned int get_v4l2_fmt(int idx)
  {
    switch (idx)
    {
    case 0: return V4L2_PIX_FMT_YUYV;
    case 1: return V4L2_PIX_FMT_RGB24;
    case 2: return V4L2_PIX_FMT_RGB32;
    case 3: return V4L2_PIX_FMT_GREY;
    case 4: return V4L2_PIX_FMT_SBGGR16;
    default: return 0;
    }
  }

  unsigned int get_v4l2_idx(int fcc)
  {
    switch (fcc)
    {
    case V4L2_PIX_FMT_YUYV: return 0;
    case V4L2_PIX_FMT_RGB24: return 1;
    case V4L2_PIX_FMT_RGB32: return 2;
    case V4L2_PIX_FMT_GREY: return 3;
    case V4L2_PIX_FMT_SBGGR16: return 4;
    default: return 0;
    }
  }

  // Copy a file and replace special fields:
  void cookedCopy(std::filesystem::path const & src, std::filesystem::path const & dst,
                  std::string const & name, std::string const & vendor, std::string const & synopsis,
                  std::string const & author, std::string const & email, std::string const & website,
                  std::string const & license, std::string const & videomapping)
  {
    std::ifstream f(src);
    if (f.is_open() == false) LFATAL("Cannot read " << src);
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::string proj = jevois::tolower(name);
    
    jevois::replaceStringAll(str, "__MODULE__", name);
    jevois::replaceStringAll(str, "__PROJECT__", proj);
    jevois::replaceStringAll(str, "__VENDOR__", vendor);
    jevois::replaceStringAll(str, "__SYNOPSIS__", synopsis);
    jevois::replaceStringAll(str, "__AUTHOR__", author);
    jevois::replaceStringAll(str, "__EMAIL__", email);
    jevois::replaceStringAll(str, "__WEBSITE__", website);
    jevois::replaceStringAll(str, "__LICENSE__", license);
    jevois::replaceStringAll(str, "__VIDEOMAPPING__", videomapping);
          
    std::ofstream ofs(dst);
    if (ofs.is_open() == false) LFATAL("Cannot write " << dst << " -- check that you are running as root.");
    ofs << str << std::endl;
    LINFO("Translated copy " << src << " => " << dst);             
  }

  // Copy an existing CMakeLists.txt and replace some fields:
  void cookedCmakeClone(std::filesystem::path const & src, std::filesystem::path const & dst,
                        std::string const & oldname, std::string const & newname,
                        std::string const & oldvendor, std::string const & newvendor, std::string const & synopsis,
                        std::string const & author, std::string const & email, std::string const & website,
                        std::string const & license, std::string const & videomapping)
  {
    std::ifstream f(src);
    if (f.is_open() == false) LFATAL("Cannot read " << src);
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::string oldproj = jevois::tolower(oldname);
    std::string newproj = jevois::tolower(newname);

    jevois::replaceStringAll(str, oldname, newname);
    jevois::replaceStringAll(str, oldproj, newproj);
    jevois::replaceStringAll(str, oldvendor, newvendor);
    jevois::replaceStringAll(str, "__SYNOPSIS__", synopsis);
    jevois::replaceStringAll(str, "__AUTHOR__", author);
    jevois::replaceStringAll(str, "__EMAIL__", email);
    jevois::replaceStringAll(str, "__WEBSITE__", website);
    jevois::replaceStringAll(str, "__LICENSE__", license);
    jevois::replaceStringAll(str, "__VIDEOMAPPING__", videomapping);
         
    std::ofstream ofs(dst);
    if (ofs.is_open() == false) LFATAL("Cannot write " << dst << " -- check that you are running as root.");
    ofs << str << std::endl;
    LINFO("Translated copy " << src << " => " << dst);             
  }

  // Try to replace class name and namespace name. This is very hacky...
  void cookedCodeClone(std::filesystem::path const & src, std::filesystem::path const & dst,
                       std::string const & oldname, std::string const & newname)
  {
    std::ifstream f(src);
    if (f.is_open() == false) LFATAL("Cannot read " << src);
    std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    std::string oldnspc = jevois::tolower(oldname);
    std::string newnspc = jevois::tolower(newname);

    jevois::replaceStringAll(str, "class " + oldname, "class " + newname);
    jevois::replaceStringAll(str, oldname + '(', newname + '('); // constructor and destructor
    jevois::replaceStringAll(str, '(' + oldname, '(' + newname); // JEVOIS_REGISTER_MODULE
    jevois::replaceStringAll(str, oldname + "::", newname + "::");

    jevois::replaceStringAll(str, "namespace " + oldnspc, "namespace " + newnspc);
    jevois::replaceStringAll(str, oldnspc + "::", newnspc + "::");

    // Revert a few replacements if jevois was involved. E.g., when cloning the DNN module that uses the
    // jevois::dnn namespace...
    jevois::replaceStringAll(str, "jevois::" + newnspc, "jevois::" + oldnspc);
    jevois::replaceStringAll(str, "jevois::" + newname, "jevois::" + oldname);
    
    std::ofstream ofs(dst);
    if (ofs.is_open() == false) LFATAL("Cannot write " << dst << " -- check that you are running as root.");
    ofs << str << std::endl;
    LINFO("Translated copy " << src << " => " << dst);             
  }
}

// ##############################################################################################################
void jevois::GUIhelper::drawNewModuleForm()
{
  float const fontw = ImGui::GetFontSize();
  auto e = engine();
  static int templ = 0; // Which code template to use (pro, legacy, headless, clone). Resets for each new module.

  ImGui::PushStyleColor(ImGuiCol_PopupBg, 0xf0e0ffe0);

  if (ImGui::Button("Create new machine vision module..."))
  {
    ImGui::OpenPopup("Create new machine vision module");
    itsIdleBlocked = true; // prevent GUI from disappearing if we are slow to fill the form...
    templ = 0; // default template pro/GUI each time
  }
  
  ImVec2 const center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowContentSize(ImVec2(940, 750));

  if (ImGui::BeginPopupModal("Create new machine vision module", NULL, ImGuiWindowFlags_AlwaysAutoResize))
  {
    try
    {
      static std::string name, vendor, synopsis, author, email, website, license, srcvendor, srcname;
      static int language = 0;
      static int ofmt = 0; static int ow = 320, oh = 240; static float ofps = 30.0F;
      static int cmode = 0;
      static int wdrmode = 0;
      static int cfmt = 0; static int cw = 1920, ch = 1080; static float cfps = 30.0F;
      static int c2fmt = 1; static int c2w = 512, c2h = 288;
      static jevois::VideoMapping m;
      
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Fill out the details below, or clone from");
      ImGui::SameLine();

      static std::map<std::string, size_t> mods;
      static std::string currstr = "...";

      size_t idx = 0;
      e->foreachVideoMapping([&idx](jevois::VideoMapping const & m) { mods[m.menustr2()] = idx++; });

      // Draw the module clone selector and allow selection:
      if (ImGui::BeginCombo("##clonemodule", currstr.c_str()))
      {
        for (auto const & mod : mods)
        {
          bool is_selected = false;
          if (ImGui::Selectable(mod.first.c_str(), is_selected))
          {
            m = e->getVideoMapping(mod.second);
            currstr = mod.first;

            // Prefill some data:
            vendor = "Testing";
            name = "My" + m.modulename; int i = 2;
            while (std::filesystem::exists(JEVOIS_MODULE_PATH "/" + vendor + '/' + name))
            { name = "My" + m.modulename + std::to_string(i++); }
            
            language = m.ispython ? 0 : 1;
            templ = 3; // clone
            ofmt = m.ofmt; ow = m.ow; oh = m.oh; ofps = m.ofps;
            switch (m.crop)
            {
            case jevois::CropType::CropScale: cmode = 0; break;
            case jevois::CropType::Crop: cmode = 1; break;
            case jevois::CropType::Scale: cmode = 2; break;
            }
            switch (m.wdr)
            {
            case jevois::WDRtype::Linear: wdrmode = 0; break;
            case jevois::WDRtype::DOL: wdrmode = 1; break;
            }
            cfmt = get_v4l2_idx(m.cfmt); cw = m.cw; ch = m.ch; cfps = m.cfps;
            c2fmt = get_v4l2_idx(m.c2fmt); c2w = m.c2w; c2h = m.c2h;
            srcvendor = m.vendor; srcname = m.modulename;
          }
        }
        ImGui::EndCombo();
      }

      // Draw the form items:
      ImGui::Separator();
      
      ImGui::Columns(3, "new module");
      
      newModEntry("##NewModname", name, "Module Name", "MyModule",
                  "Required, even when cloning. Must start with an uppercase letter. "
                  "Will be a folder name under /jevoispro/modules/VendorName");
      
      newModEntry("##NewModvendor", vendor, "Vendor Name", "MyVendor",
                  "Required, even when cloning. Must start with an uppercase letter. "
                  "Will be a folder name under /jevoispro/modules/");
      
      // If cloning, disable all edits except for vendor and module names:
      if (templ == 3)
      {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      }
      
      newModEntry("##NewModsynopsis", synopsis, "Synopsis", "Detect Object of type X",
                  "Optional. Brief description of what the module does.");
      
      newModEntry("##NewModauthor", author, "Author Name", "John Smith", "Optional");
      
      newModEntry("##NewModemail", email, "Author Email", "you@yourcompany.com", "Optional");
      
      newModEntry("##NewModwebsite", website, "Author Website", "http://yourcompany.com", "Optional");
      
      newModEntry("##NewModlicense", license, "License", "GPL v3", "Optional");

      // Language:
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Module Language");
      ImGui::NextColumn();
      helpMarker("Module Language", "Machine language to use for your module.");
      ImGui::NextColumn();
      ImGui::Combo("##NewModlanguage", &language, "Python\0C++\0\0");
      ImGui::NextColumn();
      
      // Template:
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Module Template");
      ImGui::NextColumn();
      helpMarker("Module Template", "Type of placeholder code that will be provided to get your module started.");
      ImGui::NextColumn();
      if (templ < 3) ImGui::Combo("##NewModtemplate", &templ, "Pro/GUI\0Legacy\0Headless\0\0");
      else ImGui::Combo("##NewModtemplate", &templ, "Pro/GUI\0Legacy\0Headless\0Clone\0\0");
      ImGui::NextColumn();
      
      // Output video mapping:
      int oflags = ImGuiInputTextFlags_None;
      if (templ != 1)
      {
        oflags |= ImGuiInputTextFlags_ReadOnly;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      }
      
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Module Output");
      ImGui::NextColumn();
      helpMarker("Module Output", "Output video format for legacy module.");
      ImGui::NextColumn();
      ImGui::SetNextItemWidth(fontw * 5.0F);
      ImGui::Combo("##NewModofmt", &ofmt, "YUYV\0RGB\0RGBA\0GREY\0BAYER\0\0");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputInt("##NewModow", &ow, 0, 0, ImGuiInputTextFlags_CharsDecimal | oflags);
      ImGui::SameLine();
      ImGui::Text("x");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputInt("##NewModoh", &oh, 0, 0, ImGuiInputTextFlags_CharsDecimal | oflags);
      ImGui::SameLine();
      ImGui::Text("@");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputFloat("##NewModofps", &ofps, 0.0F, 0.0F, "%.1f", ImGuiInputTextFlags_CharsDecimal | oflags);
      ImGui::SameLine();
      ImGui::Text("fps");
      ImGui::NextColumn();
      
      if (templ != 1)
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }
      
      // Camera mode:
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Camera Mode");
      ImGui::NextColumn();
      helpMarker("Camera Mode", "Camera sensor configuration for your module.");
      ImGui::NextColumn();
      ImGui::SetNextItemWidth(fontw * 18.0F);
      ImGui::Combo("##NewModcmode", &cmode, "Dual-resolution (Crop+Scale)\0Single-resolution Crop\0"
                   "Single-resolution Scale\0\0");
      ImGui::NextColumn();
      
      // Camera WDR mode:
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Camera WDR");
      ImGui::NextColumn();
      helpMarker("Camera WDR", "Camera sensor wide-dynamic-range (WDR) setting for your module. Linear is for no WDR, "
                 "DOL is for digital overlap (merging short and long exposure frames).");
      ImGui::NextColumn();
      ImGui::Combo("##NewModwdrmode", &wdrmode, "Linear\0\0"); // FIXME
      //ImGui::Combo("##NewModwdrmode", &wdrmode, "Linear\0DOL\0\0");
      ImGui::NextColumn();
      
      // Camera video mapping:
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Camera Format");
      ImGui::NextColumn();
      helpMarker("Camera Format", "Camera video format to use for input to module and for GUI display.");
      ImGui::NextColumn();
      ImGui::SetNextItemWidth(fontw * 5.0F);
      ImGui::Combo("##NewModcfmt", &cfmt, "YUYV\0RGB\0RGBA\0GREY\0BAYER\0\0");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputInt("##NewModcw", &cw, 0, 0, ImGuiInputTextFlags_CharsDecimal);
      ImGui::SameLine();
      ImGui::Text("x");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputInt("##NewModch", &ch, 0, 0, ImGuiInputTextFlags_CharsDecimal);
      ImGui::SameLine();
      ImGui::Text("@");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputFloat("##NewModcfps", &cfps, 0.0F, 0.0F, "%.1f", ImGuiInputTextFlags_CharsDecimal);
      ImGui::SameLine();
      ImGui::Text("fps");
      ImGui::NextColumn();
      
      // Camera second frame video mapping:
      int c2flags = ImGuiInputTextFlags_None;
      if (cmode != 0)
      {
        oflags |= ImGuiInputTextFlags_ReadOnly;
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
      }
      
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Camera Format 2");
      ImGui::NextColumn();
      helpMarker("Camera Format 2", "Camera video format for the second stream (for processing).");
      ImGui::NextColumn();
      ImGui::SetNextItemWidth(fontw * 5.0F);
      ImGui::Combo("##NewModc2fmt", &c2fmt, "YUYV\0RGB\0RGBA\0GREY\0BAYER\0\0");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputInt("##NewModc2w", &c2w, 0, 0, ImGuiInputTextFlags_CharsDecimal | c2flags);
      ImGui::SameLine();
      ImGui::Text("x");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(fontw * 4.0F);
      ImGui::InputInt("##NewModc2h", &c2h, 0, 0, ImGuiInputTextFlags_CharsDecimal | c2flags);
      ImGui::NextColumn();
      
      if (cmode != 0)
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }

      if (templ == 3)
      {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
      }
      
      // Adjust columns:
      ImGui::SetColumnWidth(0, fontw * 10.0F);
      ImGui::SetColumnWidth(1, ImGui::CalcTextSize("(?)").x + 30.0F);
      //ImGui::SetColumnWidth(2, 800.0F);
      
      ImGui::Columns(1);
      
      ImGui::Separator();
      ImGui::Separator();
      
      ImVec2 const button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
      if (ImGui::Button("Cancel", button_size))
      {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        ImGui::PopStyleColor();
        itsIdleBlocked = false;
        currstr = "...";
        return;
      }

      ImGui::SameLine(0, 540);

      // ====================================================================================================
      if (ImGui::Button("Create", button_size))
      {
        // Validate inputs:
        if (name.empty()) LFATAL("New Module cannot have an empty name.");
        if (name[0]<'A' || name[0]>'Z') LFATAL("New Module name must start with an uppercase letter.");
        if (vendor.empty()) LFATAL("New Module cannot have empty vendor name.");
        LINFO("New Module data valid...");
        
        // First create a directory for vendor, if not already present:
        std::filesystem::path const newvdir = JEVOIS_MODULE_PATH "/" + vendor;
        if (std::filesystem::exists(newvdir) == false && std::filesystem::create_directory(newvdir) == false)
          PLFATAL("Error creating dir " << newvdir << " -- check that you are running as root");
        std::filesystem::path const newmdir = newvdir / name;
        std::string vmstr; // machine string version of our videomapping
        
        // ----------------------------------------------------------------------------------------------------
        // If cloning, clone and cook:
        if (templ == 3)
        {
          // Remember original module's source path:
          std::filesystem::path srcpath = m.srcpath();
          std::filesystem::path cmakepath = m.cmakepath();

          // Replace vendor and module names in our mapping, keep all other data from source module:
          m.vendor = vendor;
          m.modulename = name;
 
          // Copy all files, so we get any icons, aux source files, calibration data, etc:
          std::string const copycmd = jevois::sformat("/bin/cp -ar %s/%s/%s %s", JEVOIS_MODULE_PATH, srcvendor.c_str(),
                                                      srcname.c_str(), newmdir.c_str());
          jevois::system(copycmd);
          LINFO("Cloned module using: " << copycmd);
          
          // Delete source and .so files, we will replace those with cooking:
          unlink((m.path() + '/' + srcname + ".C").c_str());
          unlink((m.path() + '/' + srcname + ".py").c_str());
          unlink((m.path() + '/' + srcname + ".so").c_str());
          std::filesystem::remove_all(m.path() + "/__pycache__");
          std::filesystem::remove_all(m.path() + "/build");

          // Cook and copy the module source:
          std::ostringstream oss; oss << m; vmstr = oss.str();
          cookedCodeClone(srcpath, m.srcpath(), srcname, name);

          // If C++, also copy and cook CMakeLists.txt, if present, otherwise make one from template:
          if (language == 1)
          {
            if (std::filesystem::exists(cmakepath))
              cookedCmakeClone(cmakepath, m.cmakepath(), srcname, name, srcvendor, vendor, synopsis, author,
                               email, website, license, vmstr);
            else
              cookedCopy(JEVOIS_SHARE_PATH "/templates/CMakeLists.txt", m.cmakepath(),
                         name, vendor, synopsis, author, email, website, license, vmstr);
          }
        }
        // ----------------------------------------------------------------------------------------------------
        // If not cloning, copy and cook from template:
        else
        {
          // Create a new directory and copy the template into it:
          if (std::filesystem::exists(newmdir))
            LFATAL("Directory [" << newmdir << "] already exists -- Choose another name");
          if (std::filesystem::create_directory(newmdir) == false)
            PLFATAL("Error creating directory [" << newmdir << "] for new module. Maybe not running as root?");
          LINFO("Created new Module directory: " << newmdir);
        
          // Create a new video mapping to add to videomappimgs.cfg:
          m = { };
          switch (templ)
          {
          case 0: m.ofmt = JEVOISPRO_FMT_GUI; break;
          case 1: m.ofmt = get_v4l2_fmt(ofmt); m.ow = ow; m.oh = oh; m.ofps = ofps; break;
          case 2: m.ofmt = 0; break;
          default: break;
          }
          
          m.cfmt = get_v4l2_fmt(cfmt);
          m.cw = cw; m.ch = ch; m.cfps = cfps;
          
          m.vendor = vendor;
          m.modulename = name;
          
          switch (wdrmode)
          {
          case 0: m.wdr = jevois::WDRtype::Linear; break;
          case 1: m.wdr = jevois::WDRtype::DOL; break;
          default: break;
          }
          
          switch (cmode)
          {
          case 0: m.crop = jevois::CropType::CropScale;
            m.c2fmt = get_v4l2_fmt(c2fmt);
            m.c2w = c2w; m.c2h = c2h;
            break;
          case 1: m.crop = jevois::CropType::Crop; break;
          case 2: m.crop = jevois::CropType::Scale; break;
          default: break;
          }
          
          m.ispython = (language == 0);
          std::ostringstream oss; oss << m; vmstr = oss.str();

          // Copy the desired code template and cook it:
          std::string code; std::string tname;
          switch (language)
          {
          case 0:
            tname = "PyModule.py";
            break;

          case 1:
          {
            tname = "Module.C";

            // Also copy and cook the CMakeLists.txt
            std::filesystem::path srccmak = JEVOIS_SHARE_PATH "/templates/CMakeLists.txt";
            std::filesystem::path dstcmak = m.cmakepath();
            cookedCopy(srccmak, dstcmak, name, vendor, synopsis, author, email, website, license, vmstr);
          }
          break;

          default: LFATAL("Invalid language " << language);
          }

          std::filesystem::path srcmod = JEVOIS_SHARE_PATH "/templates/" + tname;
          std::filesystem::path dstmod = m.srcpath();
          cookedCopy(srcmod, dstmod, name, vendor, synopsis, author, email, website, license, vmstr);
        }
        
        // Add the video mapping:
        jevois::system(JEVOIS "-add-videomapping " + vmstr);
        LINFO("Added videomapping: " << vmstr);

        // Remember this new mapping in case we need it to compile and load the module later:
        itsNewMapping = m;

        // For python modules, load right now. For C++ modules, we will not find the mapping because the .so is missing,
        // we need to compile the module first:
        if (language == 0) runNewModule();
        else itsCompileState = CompilationState::Start;

        // Clear a few things before the next module:
        name.clear(); vendor.clear(); synopsis.clear(); currstr = "...";
        
        ImGui::CloseCurrentPopup();
        itsIdleBlocked = false;
      }
    }
    catch (...) { reportAndIgnoreException(); }

    // Make sure we always end the popup, even if we had an exception:
    ImGui::EndPopup();
  }
      
  ImGui::PopStyleColor();
}

// ##############################################################################################################
void jevois::GUIhelper::runNewModule()
{
  engine()->reloadVideoMappings();
  size_t idx = 0; size_t foundidx = 12345678;
  engine()->foreachVideoMapping([&](VideoMapping const & mm)
                                { if (itsNewMapping.isSameAs(mm)) foundidx = idx; ++idx; });

  if (foundidx != 12345678) engine()->requestSetFormat(foundidx);
  else LFATAL("Internal error, could not find the module we just created -- CHECK LOGS");
    
  itsRefreshVideoMappings = true; // Force a refresh of our list of video mappings
  //itsRefreshCfgList = true; // Force a refresh of videomappings.cfg in the config editor
    
  // Switch to the mapping list that contains our new module:
  if (itsNewMapping.ofmt == JEVOISPRO_FMT_GUI) itsVideoMappingListType = 0;
  else if (itsNewMapping.ofmt != 0 && itsNewMapping.ofmt != JEVOISPRO_FMT_GUI) itsVideoMappingListType = 1;
  else if (itsNewMapping.ofmt == 0) itsVideoMappingListType = 2;
  else LERROR("Internal error: cannot determine video mapping list type -- IGNORED"); 
}

// ##############################################################################################################
// imgui opengl demo in 1024 bytes (before we renamed a couple things for clearer docs):
using V=ImVec2;using F=float;int h;struct TinyImGUIdemo{V p;F z,w;bool operator<(TinyImGUIdemo&o){return z<o.z;}};TinyImGUIdemo G[999];
#define L(i,x,y,z)for(F i=x;i<y;i+=z)
#define Q(y)sin((y+t)*.03)*(1-sin(t*3)*cos(y/99+t))*9
#define H(p,w)L(k,0,5,1)d->AddCircleFilled(p+V(1,-1)*w*k/8,w*(1-k/5),k<4?0xff000000+k*0x554400:-1);
#define J(b)L(i,0,h,1){TinyImGUIdemo&o=G[int(i)];if(b*o.z>0)H(o.p,(o.z*.3+4)*o.w)}
#define O(b)i=t+1.6+j/99;if(b*sin(i)>0)H(c+v*j+u*(cos(i)*40+Q(j)),30)
void drawTinyImGUIdemo(ImDrawList*d,V a,V b,V,ImVec4,F t){F i=sin(t)-.7;V u(cos(i),sin(i)),v(-sin(i),cos(i)),c=(a+b)/2;F l=300;
F w=0;L(z,4,20,1){w+=z;L(yy,-l,l,z*2){F y=yy+fmod(t*z*10,z*2);L(i,-1,2,2)d->AddCircle(c+v*y+u*i*(w+sin((y+t)/25)*w/30),z,0xff000000+0x110e00*int(z*z*z/384),12,z/2);}}
h=0;L(y,-l,l,15)L(b,0,16,1){i=t+b*.2+y/99;G[h++]={c+v*y+u*(cos(i)*60+Q(y)),sinf(i),(b<1||b>14)?2.f:1.5f};}std::sort(G,G+h);
 F j=(-2+fmod(t*3,5))*99;J(-1)O(-1)a=c+v*-l;L(y,-l,l,15){b=c+v*y+u*((15-rand())&31)*.1;L(k,0,9,1)d->AddLine(a-v,b,k<8?0x11222200*k:-1,(9-k)*4);a=b;}O(1)J(1)}

// ##############################################################################################################
void jevois::GUIhelper::drawTweaks()
{
  if (ImGui::Button("Open Style Editor")) itsShowStyleEditor = true;
  ImGui::SameLine();
  if (ImGui::Button("Open App Metrics")) itsShowAppMetrics = true;
  ImGui::SameLine();
  if (ImGui::Button("Open ImGui Demo")) itsShowImGuiDemo = true;
  ImGui::Separator();

  
  float camz = pixel_perfect_z;
  float yaw = 0.0f;
  float pitch = 0.0f;
  float roll = 0.0f;
  static float fudgex = 0.375f;
  static float fudgey = 0.375f;
  static float fudgez = 0.0f;
  unsigned short winw, winh; itsBackend.getWindowSize(winw, winh);

  ImGui::SliderFloat("OpenGL Camera z", &camz, -2.0f * winh, -1.0f);
  ImGui::SliderAngle("OpenGL yaw", &yaw, -179.0f, 180.0f);
  ImGui::SliderAngle("OpenGL pitch", &pitch, -179.0f, 180.0f);
  ImGui::SliderAngle("OpenGL roll", &roll, -179.0f, 180.0f);

  ImGui::SliderFloat("fudge x", &fudgex, -1.0f, 1.0f);
  ImGui::SliderFloat("fudge y", &fudgey, -1.0f, 1.0f);
  ImGui::SliderFloat("fudge z", &fudgez, -1.0f, 1.0f);
  
  // mess with the projection matrix:
  proj = glm::perspective(glm::radians(45.0f), float(winw) / float(winh), 1.0f, winh * 2.0f);
  proj = glm::translate(proj, glm::vec3(fudgex, fudgey, fudgez));
  
  // update our view matrix
  view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, camz));
  view *= glm::yawPitchRoll(yaw, pitch, roll);

  // Note: the ortho projection of ImGui is like: glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f);
  /*
  glm::mat4 gogo = glm::ortho(0.0f, 1920.0f, 1080.0f, 0.0f);
  gogo = glm::translate(gogo, glm::vec3(0.125f, 0.125f, 0.0f));
  
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      printf("gogo[%d][%d] = %f\n", i, j, gogo[i][j]);
  */

  //static bool demo = false;
  //if (ImGui::Checkbox("OpenGL Demo", &demo))
  //{
    // from https://github.com/ocornut/imgui/issues/3606
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Tiny OpenGL demo", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImVec2 size(320.0f, 180.0f);
    ImGui::InvisibleButton("canvas", size);
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    ImVec4 mouse_data;
    mouse_data.x = (io.MousePos.x - p0.x) / size.x;
    mouse_data.y = (io.MousePos.y - p0.y) / size.y;
    mouse_data.z = io.MouseDownDuration[0];
    mouse_data.w = io.MouseDownDuration[1];

    drawTinyImGUIdemo(draw_list, p0, p1, size, mouse_data, (float)ImGui::GetTime());
    draw_list->PopClipRect();
    ImGui::End();
    //}
  
}

// ##############################################################################################################
void jevois::GUIhelper::reportError(std::string const & err)
{
  auto now = std::chrono::steady_clock::now();

  std::lock_guard<std::mutex> _(itsErrorMtx);

  // Did we already report this error? If so, just update the last received time:
  for (auto & e : itsErrors) if (e.err == err) { e.lasttime = now; return; }

  // Too many errors already?
  if (itsErrors.size() > 10) return;
  else if (itsErrors.size() == 10)
  {
    ErrorData d { "Too many errors -- TRUNCATING", now, now };
    itsErrors.emplace(itsErrors.end(), std::move(d));
    return;
  }
  
  // It's a new error, push a new entry into our list:
  ErrorData d { err, now, now };
  itsErrors.emplace(itsErrors.end(), std::move(d));

  // drawErrorPopup() will clear old entries
}

// ##############################################################################################################
void jevois::GUIhelper::clearErrors()
{
  std::lock_guard<std::mutex> _(itsErrorMtx);
  itsErrors.clear();
}

// ##############################################################################################################
void jevois::GUIhelper::reportAndIgnoreException(std::string const & prefix)
{
  if (prefix.empty())
  {
    try { throw; }
    catch (std::exception const & e)
    { reportError(e.what()); }
    catch (boost::python::error_already_set & e)
    { reportError("Python error:\n"+jevois::getPythonExceptionString(e)); }
    catch (...)
    { reportError("Unknown error"); }
  }
  else
  {
    try { throw; }
    catch (std::exception const & e)
    { reportError(prefix + ": " + e.what()); }
    catch (boost::python::error_already_set & e)
    { reportError(prefix + ": Python error:\n"+jevois::getPythonExceptionString(e)); }
    catch (...)
    { reportError(prefix + ": Unknown error"); }
  }
}

// ##############################################################################################################
void jevois::GUIhelper::reportAndRethrowException(std::string const & prefix)
{
  reportAndIgnoreException(prefix);
  throw;
}

// ##############################################################################################################
void jevois::GUIhelper::drawErrorPopup()
{
  std::lock_guard<std::mutex> _(itsErrorMtx);
  if (itsErrors.empty()) return;

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;

  ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0xc0e0e0ff);

  static bool show = true;
  
  if (ImGui::Begin("Error detected!", &show, window_flags))
  {
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::Text("Error detected!");

    auto itr = itsErrors.begin();
    while (itr != itsErrors.end())
    {
      // Clear the error after a while, unless mouse cursor is on it:
      std::chrono::duration<float> d = std::chrono::steady_clock::now() - itr->lasttime;
      std::chrono::duration<float> d2 = std::chrono::steady_clock::now() - itr->firsttime;
      if (d.count() >= 1.0f && d2.count() >= 10.0f && ImGui::IsWindowHovered() == false)
        itr = itsErrors.erase(itr);
      else
      {
        // Show this error:
        ImGui::Separator();
        ImGui::TextUnformatted(itr->err.c_str());
        ++itr;
      }
    }
    ImGui::PopTextWrapPos();

    ImGui::End();
  }
  ImGui::PopStyleColor();
}

// ##############################################################################################################
void jevois::GUIhelper::helpMarker(char const * msg, char const * msg2, char const * msg3)
{
  //ImGui::TextDisabled("(?)");
  ImGui::Text("(?)");
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(msg);
    if (msg2) { ImGui::Separator(); ImGui::TextUnformatted(msg2); }
    if (msg3) { ImGui::Separator(); ImGui::TextUnformatted(msg3); }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

// ##############################################################################################################
bool jevois::GUIhelper::toggleButton(char const * name, bool * val)
{
  bool changed = false;
  if (*val)
  {
    ImGui::PushID(name);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(4.0f/7.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(4.0f/7.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4.0f/7.0f, 0.5f, 0.5f));
    if (ImGui::Button(name)) { *val = false; changed = true; }
    ImGui::PopStyleColor(3);
    ImGui::PopID();
  }
  else if (ImGui::Button(name)) { *val = true; changed = true; }

  return changed;
}

// ##############################################################################################################
void jevois::GUIhelper::headlessDisplay()
{
  unsigned short winw, winh;
  startFrame(winw, winh);

  if (itsHeadless.loaded() == false)
    try { itsHeadless.load(JEVOIS_SHARE_PATH "/icons/headless.png"); }
    catch (...)
    {
      jevois::warnAndIgnoreException();
      cv::Mat blank(winh, winw, CV_8UC4, 0);
      itsHeadless.load(blank);
    }
  
  if (itsHeadless.loaded()) itsHeadless.draw(ImVec2(0, 0), ImVec2(winw, winh), ImGui::GetBackgroundDrawList());

  endFrame();
}

// ##############################################################################################################
void jevois::GUIhelper::highlightText(std::string const & str)
{
  ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
  ImGui::TextUnformatted(str.c_str());
  ImGui::PopStyleColor();  
}

// ##############################################################################################################
void jevois::GUIhelper::demoBanner(std::string const & title, std::string const & msg)
{
  // Here we just update our internals, we will do the drawing in the main loop:
  itsBannerTitle = title;
  itsBannerMsg = msg;
}

// ##############################################################################################################
void jevois::GUIhelper::startCompilation()
{
  if (itsCompileState == CompilationState::Cmake || itsCompileState == CompilationState::Make ||
      itsCompileState == CompilationState::Install || itsCompileState == CompilationState::CPack)
    reportError("Still compiling... Try again later...");
  else if (std::filesystem::exists(itsNewMapping.srcpath()) && std::filesystem::exists(itsNewMapping.cmakepath()))
    itsCompileState = CompilationState::Start;
  else
  {
    // Maybe the user opened another file than the currently running module, which can occur if a module gives a hard
    // crash and cannot be loaded anymore. See if we can compile it using the code editor's current file path:
    std::filesystem::path const & fpath = itsCodeEditor->getLoadedFilePath();
    std::filesystem::path const fn = fpath.filename();

    if (fn == "CMakeLists.txt" || fn.extension() == ".C")
    {
      itsNewMapping = { };
      std::filesystem::path p = fpath.parent_path();
      itsNewMapping.modulename = p.filename();
      itsNewMapping.vendor = p.parent_path().filename();
      itsNewMapping.ispython = false;
      
      // Beware that the rest of the mapping is uninitialized but that should be enough to recompile...
      if (std::filesystem::exists(itsNewMapping.srcpath()) && std::filesystem::exists(itsNewMapping.cmakepath()))
        itsCompileState = CompilationState::Start;
      else reportError("Cannot find " + itsNewMapping.srcpath() + " or " + itsNewMapping.cmakepath() + " -- IGNORED");
    }
    else reportError("Cannot compile " + fpath.string() + " -- IGNORED");
  }
}

// ##############################################################################################################
void jevois::GUIhelper::compileModule()
{
  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0xf0e0ffe0);
  // Set window size applied only on first use ever, otherwise from imgui.ini:
  ImGui::SetNextWindowSize(ImVec2(800, 680), ImGuiCond_FirstUseEver);

  // Open the window, allow closing when we are in error:
  int constexpr flags = ImGuiWindowFlags_HorizontalScrollbar;
  bool keep_window_open = true;
  if (itsCompileState == CompilationState::Error) ImGui::Begin("C++ Module Compilation", &keep_window_open, flags);
  else ImGui::Begin("C++ Module Compilation", nullptr, flags);

  ImGui::TextUnformatted(("Compiling " + itsNewMapping.srcpath()).c_str());
  ImGui::TextUnformatted(("Using " +  itsNewMapping.cmakepath()).c_str());
  ImGui::Separator();
  std::string const modpath = itsNewMapping.path();
  std::string const buildpath = modpath + "/build";

  try
  {
    switch (itsCompileState)
    {
      // ----------------------------------------------------------------------------------------------------
    case CompilationState::Start:
      LINFO("Start compiling " << itsNewMapping.srcpath());
      itsIdleBlocked = true;
      for (std::string & s : itsCompileMessages) s.clear();
      std::filesystem::remove_all(buildpath);
      itsCompileState = CompilationState::Cmake;

      // Write a small script to allow users to recompile by hand in case of a bad crashing module:
      try
      {
        std::filesystem::path sp(modpath + "/rebuild.sh");
        
        std::ofstream ofs(sp);
        if (ofs.is_open() == false)
          reportError("Cannot write " + sp.string() + " -- check that you are running as root.");
        else
        {
          // Keep this in sync with the commands run below:
          ofs << "#!/bin/sh" << std::endl << "set -e" << std::endl;
          ofs << "cmake -S " << modpath << " -B " << buildpath << " -DJEVOIS_HARDWARE=PRO"
#ifdef JEVOIS_PLATFORM
              << " -DJEVOIS_PLATFORM=ON -DJEVOIS_NATIVE=ON"
#endif
              << std::endl;
          ofs << "JEVOIS_SRC_ROOT=none cmake --build " << buildpath << std::endl;
          ofs << "cmake --install " << buildpath << std::endl;
          ofs << "cd " << buildpath <<
            " && cpack && mkdir -p /jevoispro/debs && /bin/mv *.deb /jevoispro/debs/" << std::endl;
          ofs.close();
          
          // Set the file as executable:
          using std::filesystem::perms;
          std::filesystem::permissions(sp, perms::owner_all | perms::group_read | perms::group_exec |
                                       perms::others_read | perms::others_exec);
        }
      } catch (...) { }
      break;
      
      // ----------------------------------------------------------------------------------------------------
    case CompilationState::Cmake:
      if (compileCommand("cmake -S " + modpath + " -B " + buildpath +
                         " -DJEVOIS_HARDWARE=PRO"
#ifdef JEVOIS_PLATFORM
                         " -DJEVOIS_PLATFORM=ON -DJEVOIS_NATIVE=ON"
#endif
                         , itsCompileMessages[0]))
        itsCompileState = CompilationState::Make;
      break;

      // ----------------------------------------------------------------------------------------------------
    case CompilationState::Make:
      if (compileCommand("JEVOIS_SRC_ROOT=none cmake --build " + buildpath, itsCompileMessages[1]))
        itsCompileState = CompilationState::Install;
      break;
    
      // ----------------------------------------------------------------------------------------------------
    case CompilationState::Install:
      if (compileCommand("cmake --install " + buildpath, itsCompileMessages[2]))
        itsCompileState = CompilationState::CPack;
      break;

    // ----------------------------------------------------------------------------------------------------
    case CompilationState::CPack:
      if (compileCommand("cd " + buildpath + " && cpack && mkdir -p /jevoispro/debs && /bin/mv *.deb /jevoispro/debs/",
                         itsCompileMessages[3]))
        itsCompileState = CompilationState::Success;
      break;
    
      // ----------------------------------------------------------------------------------------------------
    case CompilationState::Success:
    {
      ImGui::TextUnformatted("Compilation success!");
      ImGui::TextUnformatted("See below for details...");
      ImGui::Separator();
      static int run_default = 0;
      int ret = modal("Success! Run the module now?", "Compilation success!\n\nA deb package of your module is in "
                      "/jevoispro/debs/\nif you want to share it.\n\nLoad and run the new module now?",
                      &run_default, "Yes", "No");
      switch (ret)
      {
      case 1:
        runNewModule();
        itsIdleBlocked = false;
        itsCompileState = CompilationState::Idle;
        break;
        
      case 2:
        // make sure we have the mapping for the new module...
        itsIdleBlocked = false;
        itsCompileState = CompilationState::Idle;
        engine()->reloadVideoMappings();
        itsRefreshVideoMappings = true;
        break;

      default: break;  // Need to wait
      }
    }
    break;
      
    case CompilationState::Error:
    {
      itsIdleBlocked = false;
      highlightText("Compilation failed!");
      ImGui::TextUnformatted("You may need to edit the two files above.");
      ImGui::TextUnformatted("See below for details...");
      ImGui::Separator();
      static bool show_modal = false;
      
      ImGui::AlignTextToFramePadding();
      if (ImGui::Button("Edit CMakeLists.txt"))
      { itsCodeEditor->loadFile(itsNewMapping.cmakepath()); show_modal = true; }

      ImGui::SameLine(); ImGui::Text("    "); ImGui::SameLine();
      if (ImGui::Button(("Edit " + itsNewMapping.modulename + ".C").c_str()))
      { itsCodeEditor->loadFile(itsNewMapping.srcpath()); show_modal = true; }

      ImGui::SameLine(); ImGui::Text("    "); ImGui::SameLine();
      if (ImGui::Button("Compile again")) itsCompileState = CompilationState::Start;

      ImGui::Separator();

      if (show_modal)
      {
        static int doit_default = 0;
        int ret = modal("Ready to edit", "File will now be loaded in the Code tab of the main window and "
                        "ready to edit. Please switch to the Code tab in the main window.\n\n"
                        "When you save it, we will try to compile it again.",
                        &doit_default, "Close Compilation Window", "Keep Open");
        switch (ret)
        {
        case 1: show_modal = false; itsCompileState = CompilationState::Idle; break;
        case 2: show_modal = false; break; // user want to keep this window open
        default: break;  // Need to wait
        }
      }
    }
    break;
      
      // ----------------------------------------------------------------------------------------------------
    default:
      LERROR("Internal error: invalid compilation state " << int(itsCompileState) << " -- RESET TO IDLE");
      itsCompileState = CompilationState::Idle;
      itsIdleBlocked = false;
    }
  } catch (...) { reportAndIgnoreException(); }

  // Display the messages:
  for (std::string & s : itsCompileMessages)
    if (s.empty() == false)
    {
      ImGui::TextUnformatted(s.c_str());
      ImGui::Separator();
    }
  
  ImGui::End();
  ImGui::PopStyleColor();

  // Switch to idle if we were in error and user closed the window:
  if (keep_window_open == false) { itsIdleBlocked = false; itsCompileState = CompilationState::Idle; }
}


// ##############################################################################################################
bool jevois::GUIhelper::compileCommand(std::string const & cmd, std::string & msg)
{
  if (itsCompileFut.valid() == false)
  {
    itsCompileFut = jevois::async([&](std::string cmd) { return jevois::system(cmd, true); }, cmd);
    msg = "Running: " + cmd + "\nPlease wait ...\n";
    LINFO("Running: " + cmd);
  }
  else if (itsCompileFut.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
    try
    {
      msg += itsCompileFut.get() + "\nSuccess!";
      LINFO("Success running: " << cmd);
      return true;
    }
    catch (...)
    {
      LERROR("Failed running: " << cmd);
      itsCompileState = CompilationState::Error;
      msg += jevois::warnAndIgnoreException();
    }

  return false;
}

#endif // JEVOIS_PRO
