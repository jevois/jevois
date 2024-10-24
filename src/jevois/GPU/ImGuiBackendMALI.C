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

#ifdef JEVOIS_PLATFORM_PRO

#include <jevois/GPU/ImGuiBackendMALI.H>
#include <jevois/Debug/Log.H>
#include <jevois/GPU/OpenGL.H>
#include <jevois/Util/Console.H>
#include <jevois/Util/Utils.H>

#include <imgui_impl_opengl3.h>
#include <imgui.h>

#include <linux/input.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cctype>
#include <wchar.h>
#include <wctype.h>

/* Mysteriously missing defines from <linux/input.h>. */
#define EV_MAKE 1
#define EV_BREAK 0
#define EV_REPEAT 2

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

namespace
{
  // ##############################################################################################################
  static std::string clipboardText;
  
  char const * ImGui_ImplMALI_GetClipboardText(ImGuiContext *)
  { return clipboardText.c_str(); }
  
  void ImGui_ImplMALI_SetClipboardText(ImGuiContext *, char const * text)
  { clipboardText = text; }

  void ImGui_ImplMALI_PlatformSetImeData(ImGuiContext*, ImGuiViewport*, ImGuiPlatformImeData* data)
  {
    if (data->WantVisible)
    {
      // This gets called each time we click into a text entry widget
      
      //LERROR("IME (input method editor) not yet supported");

      /* here is the code from the SDL2 backend:
      SDL_Rect r;
      r.x = (int)data->InputPos.x;
      r.y = (int)data->InputPos.y;
      r.w = 1;
      r.h = (int)data->InputLineHeight;
      SDL_SetTextInputRect(&r);
      */
    }
  }

  // Code from https://github.com/bingmann/evdev-keylogger/blob/master/keymap.c
  
  /* *******************************************************************************
   * Copyright (C) 2012 Jason A. Donenfeld <Jason@zx2c4.com>
   * Copyright (C) 2014 Timo Bingmann <tb@panthema.net>
   *
   * This program is free software; you can redistribute it and/or modify it
   * under the terms of the GNU General Public License as published by the Free
   * Software Foundation; either version 2 of the License, or (at your option)
   * any later version.
   *
   * This program is distributed in the hope that it will be useful, but WITHOUT
   * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
   * more details.
   *
   * You should have received a copy of the GNU General Public License along with
   * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
   * Place, Suite 330, Boston, MA 02111-1307 USA
   ******************************************************************************/

  /* c = character key
   * f = function key
   * _ = blank/error
   *
   * Source: KEY_* defines from <linux/input.h>
   */
  static char const char_or_func[] =
    /*   0123456789ABCDEF */
        "_fccccccccccccff"  /* 0 */
        "ccccccccccccfFcc"  /* 1 */
        "ccccccccccFccccc"  /* 2 */
        "ccccccFfFfFfffff"  /* 3 */
        "fffffFFfffffffff"  /* 4 */
        "ffff__cff_______"  /* 5 */
        "fFffFfffffffffff"  /* 6 */
        "_______f_____FFF"; /* 7 */
  
  int is_char_key(unsigned int code)
  {
    if (code >= sizeof(char_or_func)) throw std::range_error("is_char_key: invalid code " + std::to_string(code));
    return char_or_func[code] == 'c';
  }
  /*
  int is_func_key(unsigned int code)
  {
    if (code >= sizeof(char_or_func)) throw std::range_error("is_func_key: invalid code " + std::to_string(code));
    return char_or_func[code] == 'f' || char_or_func[code] == 'F';
  }
  */
  /*
  int is_modfunc_key(unsigned int code)
  {
    if (code >= sizeof(char_or_func)) throw std::range_error("is_modfunc_key: invalid code " + std::to_string(code));
    return char_or_func[code] == 'F';
  }
  */
  /*
  int is_used_key(unsigned int code)
  {
    if (code >= sizeof(char_or_func)) throw std::range_error("is_used_key: invalid code " + std::to_string(code));
    return char_or_func[code] != '_';
  }
  */
  
  /* Translates character keycodes to continuous array indexes. */
  int to_char_keys_index(unsigned int keycode)
  {
    // keycodes 2-13: US keyboard: 1, 2, ..., 0, -, =
    if (keycode >= KEY_1 && keycode <= KEY_EQUAL) return keycode - KEY_1;
    // keycodes 16-27: q, w, ..., [, ]
    if (keycode >= KEY_Q && keycode <= KEY_RIGHTBRACE) return keycode - KEY_Q + 12;
    // keycodes 30-41: a, s, ..., ', `
    if (keycode >= KEY_A && keycode <= KEY_GRAVE) return keycode - KEY_A + 24;
    // keycodes 43-53: \, z, ..., ., /
    if (keycode >= KEY_BACKSLASH && keycode <= KEY_SLASH) return keycode - KEY_BACKSLASH + 36;
    // key right to the left of 'Z' on US layout
    if (keycode == KEY_102ND) return 47;
    return -1; // not character keycode
  }

  /* Translates function keys keycodes to continuous array indexes. */
  int to_func_keys_index(unsigned int keycode)
  {
    // 1
    if (keycode == KEY_ESC) return 0;
    // 14-15
    if (keycode >= KEY_BACKSPACE && keycode <= KEY_TAB) return keycode - 13;
    // 28-29
    if (keycode >= KEY_ENTER && keycode <= KEY_LEFTCTRL) return keycode - 25;
    // 42
    if (keycode == KEY_LEFTSHIFT) return keycode - 37;
    // 54-83
    if (keycode >= KEY_RIGHTSHIFT && keycode <= KEY_KPDOT) return keycode - 48;
    // 87-88
    if (keycode >= KEY_F11 && keycode <= KEY_F12) return keycode - 51;
    // 96-111
    if (keycode >= KEY_KPENTER && keycode <= KEY_DELETE) return keycode - 58;
    // 119
    if (keycode == KEY_PAUSE) return keycode - 65;
    // 125-127
    if (keycode >= KEY_LEFTMETA && keycode <= KEY_COMPOSE) return keycode - 70;
    
    return -1; // not function key keycode
  }
  
  /* Determines the system keymap via the dump keys program and some disgusting parsing of it. */
  void load_system_keymap(std::array<wchar_t, 49> & char_keys, std::array<wchar_t, 49> & shift_keys,
                          std::array<wchar_t, 49> & altgr_keys)
  {
    /* HACK: This is obscenely ugly, and we should really just do this in C... */
    FILE * dumpkeys = popen("/usr/bin/dumpkeys -n | /bin/grep '^\\([[:space:]]shift[[:space:]]\\)*\\([[:space:]]altgr[[:space:]]\\)*keycode' | /bin/sed 's/U+/0x/g' 2>&1", "r");
    if (!dumpkeys) { LERROR("Cannot run dumpkeys -- NOT UPDATING KEYMAP"); return; }

    // Here we can get strings like:
    // keycode x = y
    // shift keycode x = y
    // altgr keycode x = y
    // shift altgr keycode x = y: never happens on my keyboard

    char_keys.fill(0);
    shift_keys.fill(0);
    altgr_keys.fill(0);

    LINFO("Loading keymap...");
    char buffer[256]; int n = 0;
    while (fgets(buffer, sizeof(buffer), dumpkeys))
    {
      auto tok = jevois::split(buffer);
      size_t const ntok = tok.size();
      
      if (ntok < 4) continue;
      if (tok[ntok - 4] != "keycode") continue;
      unsigned int const keycode = std::stoi(tok[ntok - 3]);
      if (!is_char_key(keycode)) continue;
      if (keycode >= sizeof(char_or_func)) continue;
      bool const shift = (tok[1] == "shift");
      bool const altgr = (tok[1] == "altgr" || tok[2] == "altgr");
      std::string const & val = tok[ntok - 1];
      if (val.empty()) { LERROR("Skipping invalid empty keycode value"); continue; }

      int index = to_char_keys_index(keycode);
      wchar_t wch = (wchar_t)jevois::from_string<unsigned int>(val);
      if (val[0] == '+' && (wch & 0xB00)) wch ^= 0xB00;

      if (tok[0] == "keycode") { char_keys[index] = wch; ++n; }
      else
      {
        if (shift)
        {
          if (wch == L'\0') wch = towupper(char_keys[index]);
          shift_keys[index] = wch;
          ++n;
        }
        else if (altgr) { altgr_keys[index] = wch; ++n; }
      }
    }
    pclose(dumpkeys);
    LINFO("Loaded " << n << " keymap entries.");
  }

  /* Translates struct input_event *event into the string of size buffer_length
   * pointed to by buffer. Stores state originating from sequence of event
   * structs in struc input_event_state *state. Returns the number of bytes
   * written to buffer. */
  size_t translate_eventw(input_event const *event, input_event_state *state,
                          wchar_t *wbuffer, size_t wbuffer_len, std::array<wchar_t, 49> const & char_keys,
                          std::array<wchar_t, 49> const & shift_keys, std::array<wchar_t, 49> const & altgr_keys,
                          wchar_t const func_keys[58][8])
  {
    wchar_t wch;
    size_t len = 0;

    if (event->type != EV_KEY)
    {
      // not a keyboard event
    }
    else if (event->code >= sizeof(char_or_func))
    {
      len += swprintf(&wbuffer[len], wbuffer_len, L"<E-%x>", event->code);
    }
    else if (event->value == EV_MAKE || event->value == EV_REPEAT)
    {
      if (event->code == KEY_LEFTSHIFT || event->code == KEY_RIGHTSHIFT) state->shift = 1;
      else if (event->code == KEY_RIGHTALT) state->altgr = 1;
      else if (event->code == KEY_LEFTALT) state->alt = 1;
      else if (event->code == KEY_LEFTCTRL || event->code == KEY_RIGHTCTRL) state->ctrl = 1;
      else if (event->code == KEY_LEFTMETA || event->code == KEY_RIGHTMETA) state->meta = 1;

      // If ctrl is pressed, do not translate here. Those keys will be probed directly via io.KeysDown, which is set
      // upstream by our caller:
      if (state->ctrl) return 0;

      // For any key that can translate to a meaningful wchar_t, add the translated value to the buffer:
      if (is_char_key(event->code))
      {
        if (state->altgr)
        {
          wch = altgr_keys[to_char_keys_index(event->code)];
          if (wch == L'\0')
          {
            if (state->shift) wch = shift_keys[to_char_keys_index(event->code)];
            else wch = char_keys[to_char_keys_index(event->code)];
          }
        }
        else if (state->shift)
        {
          wch = shift_keys[to_char_keys_index(event->code)];
          if (wch == L'\0') wch = char_keys[to_char_keys_index(event->code)];
        }
        else wch = char_keys[to_char_keys_index(event->code)];

        if (wch != L'\0') len += swprintf(&wbuffer[len], wbuffer_len, L"%lc", wch);
      }

      else if (event->code == 57) // special handling of space which is listed as a function key
        len += swprintf(&wbuffer[len], wbuffer_len, L"%ls", func_keys[to_func_keys_index(event->code)]);
      /*
      else if (is_modfunc_key(event->code) && event->code == 57)
      {
        if (event->value == EV_MAKE)
          len += swprintf(&wbuffer[len], wbuffer_len, L"%ls", func_keys[to_func_keys_index(event->code)]);
      }
      else if (is_func_key(event->code) && event->code == 57)
      {
        len += swprintf(&wbuffer[len], wbuffer_len, L"%ls", func_keys[to_func_keys_index(event->code)]);
      }
      else
      {
        //len += swprintf(&wbuffer[len], wbuffer_len, L"<E-%x>", event->code);
      }
      */

    }
    else if (event->value == EV_BREAK)
    {
      if (event->code == KEY_LEFTSHIFT || event->code == KEY_RIGHTSHIFT) state->shift = 0;
      else if (event->code == KEY_RIGHTALT) state->altgr = 0;
      else if (event->code == KEY_LEFTALT) state->alt = 0;
      else if (event->code == KEY_LEFTCTRL || event->code == KEY_RIGHTCTRL) state->ctrl = 0;
      else if (event->code == KEY_LEFTMETA || event->code == KEY_RIGHTMETA) state->meta = 0;
    }
    
    return len;
  }

  /* Translates event into multi-byte string description. */
  size_t translate_event(input_event const *event, input_event_state *state,
                         char *buffer, size_t buffer_len, std::array<wchar_t, 49> const & char_keys,
                         std::array<wchar_t, 49> const & shift_keys, std::array<wchar_t, 49> const & altgr_keys,
                         wchar_t const func_keys[58][8])
  {
    wchar_t *wbuffer;
    size_t wbuffer_len, len;
    
    wbuffer = (wchar_t*)buffer;
    wbuffer_len = buffer_len / sizeof(wchar_t);
    
    len = translate_eventw(event, state, wbuffer, wbuffer_len, char_keys, shift_keys, altgr_keys, func_keys);
    
    if (!len) *buffer = 0;
    else wcstombs(buffer, wbuffer, buffer_len);
    return len;
  }
  
  // We could maybe use that in scanDevices()...
  // from https://github.com/bingmann/evdev-keylogger/blob/master/logger.c
  /*
#define MAX_PATH 256
#define MAX_EVDEV 16
  
  int find_default_keyboard_list(char event_device[MAX_EVDEV][MAX_PATH])
  {
    FILE *devices;
    char events[128];
    char handlers[128];
    char *event;
    int evnum = 0, i;
    
    devices = fopen("/proc/bus/input/devices", "r");
    if (!devices) {
      perror("fopen");
      return evnum;
    }
    while (fgets(events, sizeof(events), devices))
    {
      if (strstr(events, "H: Handlers=") == events)
        strcpy(handlers, events);
        else if (!strcmp(events, "B: EV=120013\n") && (event = strstr(handlers, "event")))
        {
          for (i = 0, event += sizeof("event") - 1; *event && isdigit(*event); ++event, ++i)
            handlers[i] = *event;
          handlers[i] = '\0';
          
          snprintf(event_device[evnum], sizeof(event_device[evnum]),
                   "/dev/input/event%s", handlers);
          
          fprintf(stderr, "listening to keyboard: %s\n", event_device[evnum]);
          
          if (++evnum == MAX_EVDEV) break;
        }
    }
    fclose(devices);
    return evnum;
  }
  */
  
} // anonymous namespace

// ##############################################################################################################
jevois::ImGuiBackendMALI::ImGuiBackendMALI() :
    jevois::VideoDisplayBackendMALI(), itsLastNewFrameTime(std::chrono::steady_clock::now()),
    itsMouseX(0), itsMouseY(0), itsConsoleFd(-1), itsTTY(-1), itsKBmode(0)
{
  for (size_t i = 0; i < NELEMS(itsFd); ++i) itsFd[i] = -1;
  for (size_t i = 0; i < NELEMS(itsMouseButton); ++i) itsMouseButton[i] = false;

  load_system_keymap(itsCharKeys, itsShiftKeys, itsAltgrKeys);
}

// ##############################################################################################################
jevois::ImGuiBackendMALI::~ImGuiBackendMALI()
{
  if (itsInitialized)
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
  }
  
  // Close console:
  if (itsConsoleFd >= 0) { ioctl(itsConsoleFd, KDSETMODE, KD_TEXT); close(itsConsoleFd); }

  // Unmute keyboard:
  if (itsTTY >= 0)
  {
    try { jevois::unMuteKeyboard(itsTTY, itsKBmode); } catch (...) { jevois::warnAndIgnoreException(); }
    close(itsTTY);
  }
}

// ##############################################################################################################
void jevois::ImGuiBackendMALI::init(unsigned short w, unsigned short h, bool fullscreen, float scale, bool conslock)
{
  // Lock the console if desired:
  if (conslock)
  {
    // Open console and tell OS to stop drawing on it:
    try
    {
      itsConsoleFd = jevois::getConsoleFd();
      ioctl(itsConsoleFd, KDSETMODE, KD_GRAPHICS);
    }
    catch (...) { jevois::warnAndIgnoreException(); }
    
    // https://unix.stackexchange.com/questions/173712/
    // best-practice-for-hiding-virtual-console-while-rendering-video-to-framebuffer
    
    // Mute keyboard:
    itsTTY = STDIN_FILENO;
    try { jevois::muteKeyboard(itsTTY, itsKBmode); }
    catch (...)
    {
      // stdin is not a tty, probably we were launched remotely, so we try to disable the active tty:
      try { itsTTY = jevois::getActiveTTY(); jevois::muteKeyboard(itsTTY, itsKBmode); }
      catch (...) { jevois::warnAndIgnoreException(); }
    }
  }
  
  // Init MALI and OpenGL:
  jevois::VideoDisplayBackendMALI::init(w, h, fullscreen);
  
  // Init our internals:
  itsWidth = w; itsHeight = h;
  itsMouseX = w / 2; itsMouseY = h / 2;

  // Setup Dear ImGui context:
  IMGUI_CHECKVERSION();

  ImGui::CreateContext();
  ImGuiIO & io = ImGui::GetIO();
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;  // We can honor GetMouseCursor() values (optional)
  //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;   // We can honor io.WantSetMousePos requests (optional)
  io.BackendPlatformName = "imgui_impl_jevois";

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
  io.KeyMap[ImGuiKey_Tab] = KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = KEY_PAGEUP;
  io.KeyMap[ImGuiKey_PageDown] = KEY_PAGEDOWN;
  io.KeyMap[ImGuiKey_Home] = KEY_HOME;
  io.KeyMap[ImGuiKey_End] = KEY_END;
  io.KeyMap[ImGuiKey_Insert] = KEY_INSERT;
  io.KeyMap[ImGuiKey_Delete] = KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = KEY_SPACE;
  io.KeyMap[ImGuiKey_Enter] = KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = KEY_ESC;
  io.KeyMap[ImGuiKey_KeypadEnter] = KEY_KPENTER;
  io.KeyMap[ImGuiKey_A] = KEY_A;
  io.KeyMap[ImGuiKey_C] = KEY_C;
  io.KeyMap[ImGuiKey_V] = KEY_V;
  io.KeyMap[ImGuiKey_X] = KEY_X;
  io.KeyMap[ImGuiKey_Y] = KEY_Y;
  io.KeyMap[ImGuiKey_Z] = KEY_Z;
  io.KeyMap[ImGuiKey_S] = KEY_S;
  
  ImGuiPlatformIO & platform_io = ImGui::GetPlatformIO();
  platform_io.Platform_SetClipboardTextFn = ImGui_ImplMALI_SetClipboardText;
  platform_io.Platform_GetClipboardTextFn = ImGui_ImplMALI_GetClipboardText;
  platform_io.Platform_ClipboardUserData = nullptr;
  platform_io.Platform_SetImeDataFn = ImGui_ImplMALI_PlatformSetImeData;

  // Set platform dependent data in viewport
  static int windowID = 0; // Only one window when using MALI fbdev
  ImGuiViewport* main_viewport = ImGui::GetMainViewport();
  main_viewport->PlatformHandle = (void*)(intptr_t)windowID;
  main_viewport->PlatformHandleRaw = nullptr;
  
  // Tell imgui to draw a mouse cursor (false at init time):
  io.MouseDrawCursor = false;
  
  // Setup Dear ImGui style:
  ImGui::StyleColorsDark();
  io.FontGlobalScale = scale;
  ImGui::GetStyle().ScaleAllSizes(scale);

  // Setup renderer backend:
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

  // Do an initial scan for input event devices:
  //scanDevices();

  // Weird ID conflict with column separators in GUIhelper::drawParameters() reported by ImGui, disable the warning:
  io.ConfigDebugHighlightIdConflicts = false;

  itsInitialized = true; // will be used to close ImGui if needed
}

// ##############################################################################################################
bool jevois::ImGuiBackendMALI::pollEvents(bool & shouldclose)
{
  ImGuiIO & io = ImGui::GetIO();
  bool gotsome = false;
  
  // Rescan our devices once in a while; could use udev instead
  static int count = 0;
  if (++count == 100) { scanDevices(); count = 0; }

  // Mouse button detection: we want to make sure we do not miss click-then-release within a video frame, while at ths
  // same time detecting releases from drag:
  bool mouse_pressed[5] = { false, false, false, false, false };
  bool mouse_released[5] = { false, false, false, false, false };

  // Check all our input devices in one select() call:
  static fd_set rfds; // ready to read
  static fd_set efds; // has error
  static struct timeval tv; // timeout

  FD_ZERO(&rfds); FD_ZERO(&efds);
  tv.tv_sec = 0; tv.tv_usec = 5;
  int maxfd = -1;

  for (size_t i = 0; i < NELEMS(itsFd); ++i)
    if (itsFd[i] != -1)
    {
      FD_SET(itsFd[i], &rfds);
      FD_SET(itsFd[i], &efds);
      maxfd = std::max(maxfd, itsFd[i]);
    }
  if (maxfd <= 0) return gotsome; // Nothing to check
  
  int ret = select(maxfd + 1, &rfds, nullptr, &efds, &tv);
  if (ret == -1)
  {
    LERROR("Select error");
    if (errno == EINTR) return gotsome; // interrupted; will try again next time...
  }
  else if (ret > 0)
  {
    for (size_t i = 0; i < NELEMS(itsFd); ++i)
    {
      // Only consider the fds that we monitor:
      if (itsFd[i] == -1) continue;
      
      // If error flag is set, terminate this device:
      if (FD_ISSET(itsFd[i], &efds)) { removeDevice(i); continue; }

      // If ready to read, let's read:
      if (FD_ISSET(itsFd[i], &rfds))
      {
        static struct input_event events[32];
        ssize_t len = read(itsFd[i], events, sizeof(events));

        if (len == -1)
        {
          // Drop this device (unplugged?) if true error:
          if (errno != EAGAIN) removeDevice(i);
          continue;
        }
        else if (len > 0)
        {
          len /= sizeof(events[0]);
          gotsome = true; // we received some events, let caller know
          
          for (ssize_t j = 0; j < len; ++j)
          {
            unsigned int const code = events[j].code;
            int const val = events[j].value;
            
            switch (events[j].type)
            {
              // --------------------------------------------------
            case EV_KEY:
              // A keyboard or mouse button event: First check for mouse button. To not miss click-then-release within
              // one video frame, we here just keep track of whether a button press or release event occurred:
              if (code >= BTN_MOUSE && code < BTN_MOUSE + NELEMS(itsMouseButton))
              {
                if (val) mouse_pressed[code - BTN_MOUSE] = true;
                else mouse_released[code - BTN_MOUSE] = true;
                continue;
              }

              // Now check keyboard keys:
              if (code == KEY_ESC)
              {
                shouldclose = true;
                
                // Terminate on ESC
                /*
                if (itsConsoleFd >= 0) jevois::unMuteKeyboard(itsTTY, itsKBmode);
                if (itsTTY >= 0) ioctl(itsConsoleFd, KDSETMODE, KD_TEXT);
                std::terminate();
                */
              }
                
              // Any other key: send to ImGui
              io.KeysDown[code] = (val != 0);
              if (translate_event(&events[j], &itsInputState, itsInputBuffer, sizeof(itsInputBuffer), itsCharKeys,
                                  itsShiftKeys, itsAltgrKeys, itsFuncKeys) > 0)
                io.AddInputCharactersUTF8(itsInputBuffer);

              //io.KeyAltgr = (itsInputState.altgr != 0);  // not supported by ImGui?
              io.KeyAlt = (itsInputState.alt != 0);
              io.KeyShift = (itsInputState.shift != 0);
              io.KeyCtrl = (itsInputState.ctrl != 0);
              io.KeySuper = (itsInputState.meta != 0);
              
              break;

              // --------------------------------------------------
            case EV_ABS:
              // An absolute position event:
              switch (code)
              {
              case ABS_X: itsMouseX = val; break;
              case ABS_Y: itsMouseY = val; break;
              }
              break;
              
              // --------------------------------------------------
            case EV_REL:
              // Relative mouse movement:
              switch (code)
              {
              case REL_X:
                itsMouseX += val;
                if (itsMouseX < 0) itsMouseX = 0; else if (itsMouseX >= int(itsWidth)) itsMouseX = itsWidth - 1;
                break;
                
              case REL_Y:
                itsMouseY += val;
                if (itsMouseY < 0) itsMouseY = 0; else if (itsMouseY >= int(itsHeight)) itsMouseY = itsHeight - 1;
                break;
                
              case REL_WHEEL:
                io.MouseWheel += val;
                break;
                
              case REL_HWHEEL:
                io.MouseWheelH += val;
                break;
              }
              break;
            }
          }
        }
      }
    }
  }

  // Tell imgui about mouse position and buttons now:
  for (size_t i = 0; i < NELEMS(itsMouseButton); ++i)
  {
    // If a button was pressed, say so, unless a button was released and not also pressed:
    if (mouse_pressed[i]) io.MouseDown[i] = true;
    else if (mouse_released[i]) io.MouseDown[i] = false;
  }
  
  io.MousePos = ImVec2(float(itsMouseX), float(itsMouseY));

  return gotsome;
}

// ##############################################################################################################
void jevois::ImGuiBackendMALI::newFrame()
{
  // OpenGL new frame:
  ImGui_ImplOpenGL3_NewFrame(); // initializes ImGui objects on first frame
  jevois::VideoDisplayBackendMALI::newFrame(); // Clear OpenGL display
  
  // Backend new frame:
  ImGuiIO & io = ImGui::GetIO();
  if (io.Fonts->IsBuilt() == false) LERROR("Font atlas not built -- IGNORED");

  // Update imgui displaysize and scale:
  io.DisplaySize = ImVec2(float(itsWidth), float(itsHeight));
  if (itsWidth > 0 && itsHeight > 0) io.DisplayFramebufferScale = ImVec2(1.0F, 1.0F);

  // Update io.DeltaTime with our rendering frame period:
  auto const now = std::chrono::steady_clock::now();
  std::chrono::duration<double> const dur = now - itsLastNewFrameTime;
  io.DeltaTime = dur.count();
  itsLastNewFrameTime = now;
  
  // Imgui new frame:
  ImGui::NewFrame();
}

// ##############################################################################################################
void jevois::ImGuiBackendMALI::render()
{
  ImGui::Render(); // Draw ImGui stuff
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Render to OpenGL
  jevois::VideoDisplayBackendMALI::render(); // OpenGL swapbuffers
}

// ##############################################################################################################
void jevois::ImGuiBackendMALI::addDevice(size_t num, int fd)
{
  if (num >= NELEMS(itsFd)) LFATAL("Invalid device number " << num);
  if (itsFd[num] != -1) { LERROR("Invalid device number " << num << ": already open -- IGNORED"); return; }
  itsFd[num] = fd; LINFO("Registered new input device " << fd << ": /dev/input/event" << num);
}

// ##############################################################################################################
void jevois::ImGuiBackendMALI::removeDevice(size_t num)
{
  if (num >= NELEMS(itsFd)) LFATAL("Invalid device number " << num);
  if (itsFd[num] == -1) { LERROR("Invalid device number " << num << ": not open -- IGNORED"); return; }
  close(itsFd[num]);
  LINFO("Un-registered input device " << itsFd[num]);
  itsFd[num] = -1;
}

// ##############################################################################################################
void jevois::ImGuiBackendMALI::scanDevices()
{
  // FIXME: do not even try to open event devices 0 and 1 as those flow down our framerate...  
  for (size_t i = 2; i < NELEMS(itsFd); ++i)
  {
    if (itsFd[i] == -1)
    {
      // We do not have this device currently under our watch; see if a new keyboard/mouse was plugged in:
      std::string const devname = "/dev/input/event" + std::to_string(i);

      int fd = open(devname.c_str(), O_RDONLY | O_NONBLOCK);
      if (fd > 0) { if (jevois::isInputDevice(fd)) addDevice(i, fd); else close(fd); }
    }
    else if (jevois::isInputDevice(itsFd[i]) == false)
      removeDevice(i); // We had this device, but it likely got unplugged:
  }
}

#endif // JEVOIS_PLATFORM_PRO
