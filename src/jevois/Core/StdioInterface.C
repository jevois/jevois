// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
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

#include <jevois/Core/StdioInterface.H>
#include <jevois/Debug/Log.H>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>

// ####################################################################################################
jevois::StdioInterface::StdioInterface(std::string const & instance) :
    jevois::UserInterface(instance), itsRunning(true)
{
  itsThread = std::thread([&]{
      struct timeval tv; fd_set fds; tv.tv_sec = 0; tv.tv_usec = 30000;
      while (itsRunning.load())
      {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        int ret = select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
        if (ret == -1) LERROR("Ignoring error on stdin: " << strerror(errno));
        else if (ret > 0) // some input is available, read an entire line
        {
          std::string str; std::getline(std::cin, str);
          std::lock_guard<std::mutex> _(itsMtx);
          itsString = std::move(str);
        }
      }
    });
}

// ####################################################################################################
jevois::StdioInterface::~StdioInterface()
{
  itsRunning.store(false);
  itsThread.join();
}

// ####################################################################################################
bool jevois::StdioInterface::readSome(std::string & str)
{
  std::lock_guard<std::mutex> _(itsMtx);
  if (itsString.empty() == false) { str = std::move(itsString); itsString = std::string(); return true; }
  return false;
}

// ####################################################################################################
void jevois::StdioInterface::writeString(std::string const & str)
{
  std::lock_guard<std::mutex> _(itsMtx);
  std::cout << str << std::endl;
}

// ####################################################################################################
jevois::UserInterface::Type jevois::StdioInterface::type() const
{ return jevois::UserInterface::Type::Stdio; }

