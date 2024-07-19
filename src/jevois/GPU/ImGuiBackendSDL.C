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

#ifdef JEVOIS_HOST_PRO

#include <jevois/GPU/ImGuiBackendSDL.H>
#include <jevois/Debug/Log.H>

#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>
#include <imgui.h>
#include <jevois/GPU/OpenGL.H>

// ##############################################################################################################
jevois::ImGuiBackendSDL::ImGuiBackendSDL() :
    jevois::ImGuiBackend(), itsSDLctx(0), itsSDLwin(nullptr)
{ }

// ##############################################################################################################
jevois::ImGuiBackendSDL::~ImGuiBackendSDL()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  if (itsSDLwin) SDL_DestroyWindow(itsSDLwin);
  if (itsSDLctx) SDL_GL_DeleteContext(itsSDLctx);
  SDL_Quit();
}

// ##############################################################################################################
void jevois::ImGuiBackendSDL::init(unsigned short, unsigned short, bool)
{ }

// ##############################################################################################################
void jevois::ImGuiBackendSDL::init(unsigned short w, unsigned short h, bool fullscreen, float scale, bool)
{
  if (itsSDLwin) { LERROR("Already initialized -- IGNORED"); return; }
  
  // Setup SDL:
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER))
    LFATAL("SDL initialization error: " <<  SDL_GetError());
   
  // Decide GL+GLSL versions
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  
  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  //SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  //SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  SDL_WindowFlags window_flags;
  if (fullscreen) window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALLOW_HIGHDPI);
  else window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  itsSDLwin = SDL_CreateWindow("JeVois-Pro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, window_flags);
  itsSDLctx = SDL_GL_CreateContext(itsSDLwin);
  SDL_GL_MakeCurrent(itsSDLwin, itsSDLctx);
  SDL_GL_SetSwapInterval(1); // Enable vsync
  
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO & io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  
  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  io.FontGlobalScale = scale;
  ImGui::GetStyle().ScaleAllSizes(scale);
  
  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(itsSDLwin, itsSDLctx);
  ImGui_ImplOpenGL3_Init("#version 300 es");
  
  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  //IM_ASSERT(font != NULL);

  // Show OpenGL-ES version:
  LINFO(glGetString(GL_VERSION) <<' '<< glGetString(GL_VENDOR) << " (" << glGetString(GL_RENDERER) <<')');
  //LINFO("OpenGL extensions: " << glGetString(GL_EXTENSIONS));
}

// ##############################################################################################################
bool jevois::ImGuiBackendSDL::pollEvents(bool & shouldclose)
{
  SDL_Event event;
  bool ret = false;
  while (SDL_PollEvent(&event))
  {
    ret = true;
    ImGui_ImplSDL2_ProcessEvent(&event);
    if (event.type == SDL_QUIT) shouldclose = true;
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
        event.window.windowID == SDL_GetWindowID(itsSDLwin)) shouldclose = true;

    // handle resizing here: https://retifrav.github.io/blog/2019/05/26/sdl-imgui/

  }
  return ret;
}

// ##############################################################################################################
void jevois::ImGuiBackendSDL::newFrame()
{
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(itsSDLwin);
  ImGui::NewFrame();
}

// ##############################################################################################################
void jevois::ImGuiBackendSDL::render()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(itsSDLwin);
}

// ##############################################################################################################
void jevois::ImGuiBackendSDL::getWindowSize(unsigned short & w, unsigned short & h) const
{
  if (itsSDLwin)
  {
    int ww = 0, hh = 0;
    SDL_GetWindowSize(itsSDLwin, &ww, &hh);
    w = ww; h = hh;
  }
  else
  {
    w = 0; h = 0;
  }
}

#endif // JEVOIS_HOST_PRO
