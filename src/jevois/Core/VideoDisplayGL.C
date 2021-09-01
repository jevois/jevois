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

#include <jevois/Core/VideoDisplayGL.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective

// ##############################################################################################################
jevois::VideoDisplayGL::VideoDisplayGL(size_t nbufs) :
    jevois::VideoOutput(), itsImageQueue(std::max(size_t(2), nbufs)), itsStreaming(false)
{ }

// ##############################################################################################################
void jevois::VideoDisplayGL::setFormat(jevois::VideoMapping const & m)
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
jevois::VideoDisplayGL::~VideoDisplayGL()
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
void jevois::VideoDisplayGL::get(jevois::RawImage & img)
{
  if (itsStreaming.load() == false) LFATAL("Not streaming");

  // Take this buffer out of our queue and hand it over:
  img = itsImageQueue.pop();
  LDEBUG("Empty image " << img.bufindex << " handed over to application code for filling");
}

// ##############################################################################################################
void jevois::VideoDisplayGL::send(jevois::RawImage const & img)
{
  if (itsStreaming.load() == false) LFATAL("Not streaming");

  // Get current window size, will be 0x0 if not initialized yet:
  unsigned short winw, winh;
  itsBackend.getWindowSize(winw, winh);

  if (winw == 0)
  {
    // Need to init the display:
    itsBackend.init(1920, 1080, true); ///////FIXME
    itsBackend.getWindowSize(winw, winh); // Get the actual window size
  }

  // Start a new frame and get its size. Will init the display if needed:
  itsBackend.newFrame();

  // Poll any events. FIXME: for now just ignore requests to close:
  bool shouldclose = false; itsBackend.pollEvents(shouldclose);

  // In this viewer, we do not use perspective. So just rescale our drawing from pixel coordinates to normalized device
  // coordinates using a scaling matrix:
#ifdef JEVOIS_PLATFORM
  // On platform, we need to translate a bit to avoid aliasing issues, which are problematic with our YUYV shader:
  static glm::mat4 pvm = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f / winw, 2.0f / winh, 1.0f)),
                                        glm::vec3(0.375f, 0.375f, 0.0f));
                                    
#else
  static glm::mat4 pvm = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f / winw, 2.0f / winh, 1.0f));
#endif

  // Draw the image, as big as possible on the screen:
  itsImage.set(img);
  int x = 0, y = 0; unsigned short w = 0, h = 0;
  itsImage.draw(x, y, w, h, true, pvm); // true for no aliasing

  // Done, ask backend to swap buffers:
  itsBackend.render();
  
  // Just push the buffer back into our queue. Note: we do not bother clearing the data or checking that the image is
  // legit, i.e., matches one that was obtained via get():
  itsImageQueue.push(img);
  LDEBUG("Empty image " << img.bufindex << " ready for filling in by application code");
}

// ##############################################################################################################
void jevois::VideoDisplayGL::streamOn()
{ itsStreaming.store(true); }

// ##############################################################################################################
void jevois::VideoDisplayGL::abortStream()
{ itsStreaming.store(false); }

// ##############################################################################################################
void jevois::VideoDisplayGL::streamOff()
{ itsStreaming.store(false); }

#endif //  JEVOIS_PRO
