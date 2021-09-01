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

#include <jevois/Util/Console.H>
#include <stdexcept>
#include <string>

#include <linux/input.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cctype>

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

// ##############################################################################################################
bool jevois::isInputDevice(int fd)
{
#define BITS_PER_LONG           (sizeof(unsigned long) * 8)
#define NBITS(x)                ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)                  ((x)%BITS_PER_LONG)
#define LONG(x)                 ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)
  
  unsigned long bitmask_ev[NBITS(EV_MAX)];
  unsigned long bitmask_key[NBITS(KEY_MAX)];
  unsigned long bitmask_abs[NBITS(ABS_MAX)];
  unsigned long bitmask_rel[NBITS(REL_MAX)];
  
  if (ioctl(fd, EVIOCGBIT(0, sizeof(bitmask_ev)), &bitmask_ev) == -1) return false;
  if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bitmask_key)), &bitmask_key) == -1) return false;
  if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(bitmask_abs)), &bitmask_abs) == -1) return false;
  if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(bitmask_rel)), &bitmask_rel) == -1) return false;
  
  bool is_keyboard = (bitmask_key[0] & 0xFFFFFFFE);
  bool is_abs = test_bit(EV_ABS, bitmask_ev) && test_bit(ABS_X, bitmask_abs) && test_bit(ABS_Y, bitmask_abs);
  bool is_rel = test_bit(EV_REL, bitmask_ev) && test_bit(REL_X, bitmask_rel) && test_bit(REL_Y, bitmask_rel);
  bool is_mouse = (is_abs || is_rel) && test_bit(BTN_MOUSE, bitmask_key);
  bool is_touch = is_abs && (test_bit(BTN_TOOL_FINGER, bitmask_key) || test_bit(BTN_TOUCH, bitmask_key));
  
  return is_keyboard || is_mouse || is_touch;
#undef BITS_PER_LONG
#undef NBITS
#undef OFF
#undef LONG
#undef test_bit
}

// ##############################################################################################################
#ifndef KDSKBMUTE
#define KDSKBMUTE 0x4B51
#endif
#ifndef KDSKBMODE
#define KDSKBMODE 0x4B45
#endif
#ifndef K_OFF
#define K_OFF 0x04
#endif

static char const * const consoles[] =
{ "/proc/self/fd/0", "/dev/tty", "/dev/tty0", "/dev/tty1", "/dev/tty2", "/dev/tty3", "/dev/tty4",
  "/dev/tty5", "/dev/tty6", "/dev/vc/0", "/dev/console" };

#define IS_CONSOLE(fd) isatty(fd) && ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84))

int jevois::getConsoleFd()
{
  char arg;
  
  // Try a few consoles to see which one we have read access to:
  for (size_t i = 0; i < NELEMS(consoles); ++i)
  {
    int fd = open(consoles[i], O_RDONLY);
    if (fd >= 0) { if (IS_CONSOLE(fd)) return fd; else close(fd); }
  }
  
  // Also try stdin, stdout, stderr:
  for (int fd = 0; fd < 3; ++fd) if (IS_CONSOLE(fd)) return fd;
  
  throw std::runtime_error("Could not get console fd");
}

// ##############################################################################################################
void jevois::muteKeyboard(int tty, int & kb_mode)
{
  kb_mode = 0; char arg;
  if (!IS_CONSOLE(tty)) throw std::runtime_error("Tried to mute an invalid tty");
  
  ioctl(tty, KDGKBMODE, &kb_mode); // Not fatal if fails
  if (ioctl(tty, KDSKBMUTE, 1) && ioctl(tty, KDSKBMODE, K_OFF)) throw std::runtime_error("Failed muting keyboard");
}
  
// ##############################################################################################################
// Restore the keyboard mode for given tty:
void jevois::unMuteKeyboard(int tty, int kb_mode)
{
  if (ioctl(tty, KDSKBMUTE, 0) && ioctl(tty, KDSKBMODE, kb_mode))
    throw std::runtime_error("Failed restoring keyboard mode");
}
  
// ##############################################################################################################
int jevois::getActiveTTY()
{
  char ttyname[256];
  char arg;
    
  int fd = open("/sys/class/tty/tty0/active", O_RDONLY);
  if (fd < 0) throw std::runtime_error("Could not determine which tty is active -- IGNORED");
  ssize_t len = read(fd, ttyname, 255);
  close(fd);
  if (len <= 0) throw std::runtime_error("Could not read which tty is active");
  
  if (ttyname[len-1] == '\n') ttyname[len-1] = '\0';
  else ttyname[len] = '\0';
  
  std::string const ttypath = std::string("/dev/") + ttyname;
  fd = open(ttypath.c_str(), O_RDWR | O_NOCTTY);
  if (fd < 0) throw std::runtime_error("Could not open tty: " + ttypath);
  if (!IS_CONSOLE(fd)) { close(fd); throw std::runtime_error("Invalid tty obtained: " + ttypath); }
  
  return fd;  
}

#endif // JEVOIS_PRO
