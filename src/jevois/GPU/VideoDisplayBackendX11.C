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

#include <jevois/GPU/VideoDisplayBackendX11.H>

// Restore None, etc as defined by X11/X.h as we need it here, and only here. See OpenGL.H for what we undef:
#ifndef None
#define None 0L
#endif

#ifndef Status
#define Status int
#endif

#ifndef Success
#define Success 0
#endif

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/dpms.h>
// ##############################################################################################################
class jevois::VideoDisplayBackendX11::Impl
{
  public:
    // On host, use an X11 window:
    Display *itsX11display = nullptr;
    Window itsWindow = 0;
};

// ##############################################################################################################
jevois::VideoDisplayBackendX11::VideoDisplayBackendX11() :
    jevois::VideoDisplayBackend(), pimpl(new Impl)
{ }

// ##############################################################################################################
jevois::VideoDisplayBackendX11::~VideoDisplayBackendX11()
{
  // It is better to call uninit() explicitly from the same thread that called init() and others, but just in case:
  uninit();
}

// ##############################################################################################################
void jevois::VideoDisplayBackendX11::init(unsigned short w, unsigned short h, bool fullscreen)
{
  // Open the default display:
  pimpl->itsX11display = XOpenDisplay(NULL);
  if (pimpl->itsX11display == nullptr) LFATAL("Cannot open X11 display");
  
  // Access to the top level window:
  Window win_root = DefaultRootWindow(pimpl->itsX11display);

  // Allow the window manager to control the window that will be created:
  XSetWindowAttributes xattr { };
  xattr.override_redirect = false;
  
  // Announce events when the keyboard is pressed or the window state changes:
  xattr.event_mask = ExposureMask | KeyPressMask;
  
  // Set the background to white, making it clear when X has the window cleared:
  xattr.background_pixel = XWhitePixel(pimpl->itsX11display, XDefaultScreen(pimpl->itsX11display));
  
  // create the native window based on the attributes selected an resolution passed in.
  pimpl->itsWindow = XCreateWindow(pimpl->itsX11display, win_root, 0, 0, w, h, 0,
                                   CopyFromParent, InputOutput, CopyFromParent,
                                   CWOverrideRedirect | CWEventMask | CWBackPixel, &xattr);

  // Set the window properties:
  XSetStandardProperties(pimpl->itsX11display, pimpl->itsWindow, itsName.c_str(), itsName.c_str(), None, NULL, 0, NULL);

  // Disable the display power management to turn keep the display on:
  DPMSDisable(pimpl->itsX11display);

  // Set the window to the background pixel and display it to the front:
  XClearWindow(pimpl->itsX11display, pimpl->itsWindow);
  XMapRaised (pimpl->itsX11display, pimpl->itsWindow);
  
  // Optionally request that the window becomes full screen and borderless. It will take some time for the display to
  // acknowledge the event. An Expose event will occur when this is completed:
  if (fullscreen)
  {
    Atom wm_state = XInternAtom(pimpl->itsX11display, "_NET_WM_STATE", false);
    Atom fs = XInternAtom(pimpl->itsX11display, "_NET_WM_STATE_FULLSCREEN", false);
    
    XEvent fs_event { };
    fs_event.type = ClientMessage;
    fs_event.xclient.window = pimpl->itsWindow;
    fs_event.xclient.message_type = wm_state;
    fs_event.xclient.format = 32;
    fs_event.xclient.data.l[0] = 1;
    fs_event.xclient.data.l[1] = fs;
    fs_event.xclient.data.l[2] = 0;
    
    XSendEvent(pimpl->itsX11display, win_root, false, SubstructureRedirectMask | SubstructureNotifyMask, &fs_event);
  }

  // Initialize OpenGL. Will turn itsDisplayReady to true:
  jevois::VideoDisplayBackend::init(w, h, (EGLNativeWindowType)(pimpl->itsWindow));
}

// ##############################################################################################################
void jevois::VideoDisplayBackendX11::uninit()
{
  if (pimpl->itsX11display)
  {
    // Turn display power management back on:
    DPMSEnable(pimpl->itsX11display);
    
    // Close the application window and release display handle::
    if (pimpl->itsWindow) { XDestroyWindow(pimpl->itsX11display, pimpl->itsWindow); pimpl->itsWindow = 0; }
    XCloseDisplay(pimpl->itsX11display); pimpl->itsX11display = nullptr;
  }

  // ALso close OpenGL using our base class:
  jevois::VideoDisplayBackend::uninit();
}

// ##############################################################################################################
bool jevois::VideoDisplayBackendX11::pollEvents(bool & shouldclose)
{
  XEvent event;
  bool gotsome = false;
  
  // Process events, one at a time, until there are none remaining:
  while (XPending(pimpl->itsX11display))
  {
    gotsome = true;
    
    XNextEvent(pimpl->itsX11display, &event);
    switch (event.type)
    {
    case Expose:
      // Window resized/hidden/shown etc. ignored as we will refresh soon anyway.
      break;

    case KeyPress:
    {
      // Keyboard events occured, get the key sequence, limit to 10 keys total:
      char text[11]; KeySym key_press;
      int keys = XLookupString(&event.xkey, text, 10, &key_press, 0);
      if (keys == 1 && text[0] == 'q') shouldclose = true;
      break;
    }
    
    default: break;
    }
  }

  return gotsome;
}

#endif // JEVOIS_HOST_PRO
