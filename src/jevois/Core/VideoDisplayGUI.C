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

#include <jevois/Core/VideoDisplayGUI.H>
#include <jevois/GPU/GUIhelper.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Image/RawImageOps.H>

// ##############################################################################################################
jevois::VideoDisplayGUI::VideoDisplayGUI(std::shared_ptr<GUIhelper> helper, size_t nbufs) :
    jevois::VideoOutput(), itsImageQueue(std::max(size_t(2), nbufs)), itsHelper(helper), itsStreaming(false)
{
  // We defer OpenGL init to send() so that all OpenGL code is in the same thread.
}

// ##############################################################################################################
void jevois::VideoDisplayGUI::setFormat(jevois::VideoMapping const & m)
{
  // Nuke any old buffers:
  itsStreaming.store(false);
  itsBuffers.clear();
  itsImageQueue.clear();
  size_t const nbufs = itsImageQueue.size();
  
  // Allocate the buffers and make them all immediately available as RawImage:
  unsigned int imsize = m.osize();

  for (size_t i = 0; i < nbufs; ++i)
  {
    itsBuffers.push_back(std::make_shared<jevois::VideoBuf>(-1, imsize, 0, -1));

    jevois::RawImage img;
    img.width = m.ow;
    img.height = m.oh;
    img.fmt = m.ofmt;
    img.fps = m.ofps;
    img.buf = itsBuffers[i];
    img.bufindex = i;

    // Push the RawImage to outside consumers:
    itsImageQueue.push(img);
  }
  
  LDEBUG("Allocated " << nbufs << " buffers");
}

// ##############################################################################################################
jevois::VideoDisplayGUI::~VideoDisplayGUI()
{
  // Free all our buffers:
  for (auto & b : itsBuffers)
  {
    if (b.use_count() > 1) LERROR("Ref count non zero when attempting to free VideoBuf");
    b.reset(); // VideoBuf destructor will do the memory freeing
  }
  
  itsBuffers.clear();
}

// ##############################################################################################################
void jevois::VideoDisplayGUI::get(jevois::RawImage & img)
{
  if (itsStreaming.load() == false) LFATAL("Not streaming");
  
  // Take this buffer out of our queue and hand it over:
  img = itsImageQueue.pop();
  LDEBUG("Empty image " << img.bufindex << " handed over to application code for filling");
}

// ##############################################################################################################
void jevois::VideoDisplayGUI::send(jevois::RawImage const & img)
{
  if (itsStreaming.load() == false) LFATAL("Not streaming");

  // Start the frame: Internally, this initializes or resizes the display as needed, polls and handles events (inputs,
  // window resize, etc.), and clears the frame. Variables winw and winh are set by startFrame(0 to the current window
  // size, and true is returned if no keyboard/mouse action in a while (can be used to hide any GUI elements):
  unsigned short winw, winh;
  itsHelper->startFrame(winw, winh);
  
  // Draw the image:
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsHelper->drawImage("output", img, x, y, w, h, true); // true for no aliasing

  // Draw and render the GUI, swap buffers:
  itsHelper->endFrame();
  
  // Just push the buffer back into our queue. Note: we do not bother clearing the data or checking that the image is
  // legit, i.e., matches one that was obtained via get():
  itsImageQueue.push(img);
  LDEBUG("Empty image " << img.bufindex << " ready for filling in by application code");
}

// ##############################################################################################################
void jevois::VideoDisplayGUI::streamOn()
{ itsStreaming.store(true); }

// ##############################################################################################################
void jevois::VideoDisplayGUI::abortStream()
{ itsStreaming.store(false); }

// ##############################################################################################################
void jevois::VideoDisplayGUI::streamOff()
{ itsStreaming.store(false); }

#endif //  JEVOIS_PRO

