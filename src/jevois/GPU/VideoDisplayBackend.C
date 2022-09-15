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

// Some of this code inspired by:
// https://github.com/D3Engineering/410c_camera_support

/* Copyright (c) 2017 D3 Engineering
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. */

#ifdef JEVOIS_PRO

#include <jevois/GPU/VideoDisplayBackend.H>

// ##############################################################################################################
jevois::VideoDisplayBackend::VideoDisplayBackend()
{ }

// ##############################################################################################################
jevois::VideoDisplayBackend::~VideoDisplayBackend()
{
  // It is better to call uninit() explicitly from the same thread that called init() and others, but just in case:
  uninit();
}

// ##############################################################################################################
void jevois::VideoDisplayBackend::init(unsigned short JEVOIS_UNUSED_PARAM(w), unsigned short JEVOIS_UNUSED_PARAM(h),
                                       EGLNativeWindowType win)
{
  if (itsDisplay) { LERROR("Display already initialized -- IGNORED"); return; }

  // Get an EGL display connection:
  itsDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (itsDisplay == EGL_NO_DISPLAY) LFATAL("Could not get an OpenGL display");

  // Initialize the EGL display connection:
  EGLint major, minor;
  GL_CHECK_BOOL(eglInitialize(itsDisplay, &major, &minor););
  LINFO("Initialized EGL v" << major << '.' << minor);

  // Query the EGL API. We need OpenGL-ES:
  EGLenum const api = eglQueryAPI();
  switch(api)
  {
  case EGL_OPENGL_API: LFATAL("EGL API is unsupported EGL_OPENGL_API"); break;
  case EGL_OPENGL_ES_API: LINFO("EGL API is EGL_OPENGL_ES_API"); break;
  case EGL_OPENVG_API: LFATAL("EGL API is unsupported EGL_OPENVG_API"); break;
  case EGL_NONE: LFATAL("EGL API is unsupported EGL_NONE"); break;
  default: LFATAL("EGL API is unknown");
  }

  // Get an appropriate EGL configuration:
  static EGLint const cfg_attr[] =
    {
     EGL_SAMPLES,             EGL_DONT_CARE, // 4
     EGL_ALPHA_SIZE,          8,
     EGL_RED_SIZE,            8,
     EGL_GREEN_SIZE,          8,
     EGL_BLUE_SIZE,           8,
     EGL_BUFFER_SIZE,         32,
     EGL_STENCIL_SIZE,        0,
     EGL_RENDERABLE_TYPE,     EGL_OPENGL_ES3_BIT_KHR,
     EGL_SURFACE_TYPE,        EGL_WINDOW_BIT,
     EGL_DEPTH_SIZE,          16,
     EGL_CONFORMANT,          EGL_OPENGL_ES2_BIT,
     EGL_NONE
    };
  
  EGLint num_config;
  GL_CHECK_BOOL(eglChooseConfig(itsDisplay, cfg_attr, nullptr, 0, &num_config));
  LINFO("OpenGL configs available: " << num_config);
  if (num_config < 1) LFATAL("Could not find a suitable OpenGL config");

  EGLConfig * configs = new EGLConfig[num_config];
  GL_CHECK_BOOL(eglChooseConfig(itsDisplay, cfg_attr, configs, num_config, &num_config));
  bool gotit = false;
  for (int i = 0; i < num_config; ++i)
  {
    EGLint val = 0;
    std::string info;

#define JEVOIS_EGL_INFO(x) GL_CHECK(eglGetConfigAttrib(itsDisplay, configs[i], x, &val)); \
    info += std::string(#x) + '=' + std::to_string(val) + ", ";

    JEVOIS_EGL_INFO(EGL_CONFIG_ID);
    JEVOIS_EGL_INFO(EGL_RED_SIZE);
    JEVOIS_EGL_INFO(EGL_GREEN_SIZE);
    JEVOIS_EGL_INFO(EGL_BLUE_SIZE);
    JEVOIS_EGL_INFO(EGL_ALPHA_SIZE);
    JEVOIS_EGL_INFO(EGL_ALPHA_MASK_SIZE);
    JEVOIS_EGL_INFO(EGL_DEPTH_SIZE);
    JEVOIS_EGL_INFO(EGL_STENCIL_SIZE);
    JEVOIS_EGL_INFO(EGL_SAMPLE_BUFFERS);
    JEVOIS_EGL_INFO(EGL_SAMPLES);
    JEVOIS_EGL_INFO(EGL_CONFIG_CAVEAT);

    JEVOIS_EGL_INFO(EGL_MAX_PBUFFER_WIDTH);
    JEVOIS_EGL_INFO(EGL_MAX_PBUFFER_HEIGHT);
    JEVOIS_EGL_INFO(EGL_MAX_PBUFFER_PIXELS);
    JEVOIS_EGL_INFO(EGL_NATIVE_RENDERABLE);
    JEVOIS_EGL_INFO(EGL_NATIVE_VISUAL_ID);
    JEVOIS_EGL_INFO(EGL_NATIVE_VISUAL_TYPE);
    JEVOIS_EGL_INFO(EGL_SURFACE_TYPE);
    JEVOIS_EGL_INFO(EGL_TRANSPARENT_TYPE);
    JEVOIS_EGL_INFO(EGL_BIND_TO_TEXTURE_RGB);
    JEVOIS_EGL_INFO(EGL_BIND_TO_TEXTURE_RGBA);
    JEVOIS_EGL_INFO(EGL_MAX_SWAP_INTERVAL);
    JEVOIS_EGL_INFO(EGL_MIN_SWAP_INTERVAL);
    JEVOIS_EGL_INFO(EGL_CONFORMANT);
    
    LINFO("EGL config " << i << ": " << info);

#undef JEVOIS_EGL_INFO
    
    GL_CHECK(eglGetConfigAttrib(itsDisplay, configs[i], EGL_RED_SIZE, &val));
    if (val != 8) continue;
    GL_CHECK(eglGetConfigAttrib(itsDisplay, configs[i], EGL_GREEN_SIZE, &val));
    if (val != 8) continue;
    GL_CHECK(eglGetConfigAttrib(itsDisplay, configs[i], EGL_BLUE_SIZE, &val));
    if (val != 8) continue;
    if (gotit == false)
    {
      LINFO("Using config " << i << " with 8-bit R,G,B.");
      itsConfig = configs[i];
      gotit = true;
      //break;
    }
  }
  delete [] configs;

  if (gotit == false) LFATAL("Could not find a suitable OpenGL config");
  
  // Create a native surface:
  static EGLint const win_attr[] =
    {
     //EGL_RENDER_BUFFER,     EGL_BACK_BUFFER,
     EGL_NONE
    };
  GL_CHECK(itsSurface = eglCreateWindowSurface(itsDisplay, itsConfig, win, win_attr));
  LINFO("OpenGL surface created ok.");
  
  // Bind to OpenGL-ES API:
  GL_CHECK_BOOL(eglBindAPI(EGL_OPENGL_ES_API));

  LINFO("OpenGL-ES API bound ok.");

  // Create an EGL rendering context:
  static EGLint const ctx_attr[] =
    {
     EGL_CONTEXT_CLIENT_VERSION, 3,
     EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
     EGL_CONTEXT_MINOR_VERSION_KHR, 2,
     EGL_NONE
    };
  
  GL_CHECK(itsContext = eglCreateContext(itsDisplay, itsConfig, EGL_NO_CONTEXT, ctx_attr));
  if (itsContext == EGL_NO_CONTEXT) LFATAL("Failed to create OpenGL context");
  LINFO("OpenGL context ok");
  
  // Bind the context to the surface:
  GL_CHECK(eglMakeCurrent(itsDisplay, itsSurface, itsSurface, itsContext)); 

  // Show OpenGL-ES version:
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  LINFO(glGetString(GL_VERSION) <<' '<< glGetString(GL_VENDOR) << " (" << glGetString(GL_RENDERER) <<
        ") GL_VER=" << major << '.' << minor);
  LINFO("OpenGL extensions: " << glGetString(GL_EXTENSIONS));
  
  // Synchronize buffer swapping to vsync for tear-free display:
  GL_CHECK_BOOL(eglSwapInterval(itsDisplay, 0));

  // Enable blending, used for RGBA textures that have transparency:
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ##############################################################################################################
void jevois::VideoDisplayBackend::uninit()
{
  if (itsDisplay != EGL_NO_DISPLAY)
  {
    eglBindAPI(EGL_OPENGL_ES_API);
    eglMakeCurrent(itsDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, itsContext);
    eglMakeCurrent(itsDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (itsSurface) { eglDestroySurface(itsDisplay, itsSurface); itsSurface = 0; }
    eglDestroyContext(itsDisplay, itsContext); itsContext = EGL_NO_CONTEXT;
    eglTerminate(itsDisplay); itsDisplay = EGL_NO_DISPLAY;
    itsConfig = 0;
  }
}

// ##############################################################################################################
void jevois::VideoDisplayBackend::newFrame()
{
  // Check if display window has been resized by user:
  unsigned short w, h; getWindowSize(w, h);

  if (w == 0) LFATAL("Need to call init() first");
  
  // Set the viewport and clear the display:
  glViewport(0, 0, w, h);
  glClear(GL_COLOR_BUFFER_BIT);
}

// ##############################################################################################################
void jevois::VideoDisplayBackend::render()
{
  // Display the frame after render is complete at the next vertical sync. This is drawn on our EGL surface:
  GL_CHECK_BOOL(eglSwapBuffers(itsDisplay, itsSurface));
}

// ##############################################################################################################
void jevois::VideoDisplayBackend::getWindowSize(unsigned short & w, unsigned short & h) const
{
  if (itsSurface)
  {
    EGLint ww, hh;
    GL_CHECK(eglQuerySurface(itsDisplay, itsSurface, EGL_WIDTH, &ww));
    GL_CHECK(eglQuerySurface(itsDisplay, itsSurface, EGL_HEIGHT, &hh));
    w = ww; h = hh;
  }
  else
  {
    w = 0; h = 0;
  }
}

// ##############################################################################################################
EGLDisplay jevois::VideoDisplayBackend::getDisplay() const
{ return itsDisplay; }

#endif // JEVOIS_PRO
