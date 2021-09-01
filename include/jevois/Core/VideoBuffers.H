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

#pragma once

#include <jevois/Core/VideoBuf.H>
#include <vector>
#include <memory>
#include <linux/videodev2.h>
#include <string>

namespace jevois
{
  //! Collection of buffers for V4L2 video frames (Camera or Gadget) with hooks to the MMAP'd areas
  /*! Both the Camera and the Gadget use a VideoBuffers object to manage their video buffers. VideoBuffer is just a
      vector of VideoBuf objects, with added functions to queue a buffer (send it to the kernel driver, either so that
      it will be filed with a new camera frame, or so that its contents will be streamed out over the USB link), and to
      dequeue a buffer (get the next grabbed camera frame, or the next empty USB buffer to be filled by user
      code). 

      Note that this class provides no threading safety and must be protected externally in case of multithreaded
      access.

      VideoBuffers is mainly for internal use by Camera and Gadget. Machine vision programmers should just use Module
      and the InputFrame and OutputFrame wrappers instead.

      \ingroup core */
  class VideoBuffers
  {
    public:
      //! Construct and allocate MMAP'd video buffers
      /*! Type is the buffer type, typically V4L2_BUF_TYPE_VIDEO_CAPTURE, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE or
          V4L2_BUF_TYPE_VIDEO_OUTPUT.  \note name is only for debug messages, so we can differentiate camera from USB
          buffers. */
      VideoBuffers(char const * name, int const fd, v4l2_buf_type type, size_t const num = 4);

      //! Free the MMAP'd memory area
      ~VideoBuffers();

      //! Get the number of buffers allocated
      size_t size() const;
      
      //! Get the number of buffers queued, this is always in [0 .. size()[
      size_t nqueued() const;
      
      //! Get one buffer, by index [0 .. size()[
      std::shared_ptr<VideoBuf> get(size_t const index) const;

      //! Queue one buffer to V4L2, by index [0 .. size()[
      /*! Beware that this throws if used on an output device as we would not know what value to use for bytesused (how
          much data there is in the buffer; this is necessary to support streaming out MJPG images whose number of bytes
          used is variable). Use qbuf(v4l2_buffer) instead on those, with typically a buffer obtained from dqbuf(), and
          the number of bytes used set in the bytesused field on that buffer when it contains an MJPG image. */
      void qbuf(size_t const index);

      //! Queue one buffer to V4L2, by v4l2_buffer
      /*! The caller is responsible for setting all the fields in the v4l2_buf, including an index that corresponds to a
          valid previously requested and not already queued buffer. Typically, this is used by output devices which
          first get the buffer via dqbuf(), then stuff the pixel data into it, and then send it back to qbuf(). */
      void qbuf(struct v4l2_buffer & buf);

      //! Queue all buffers, typically used when starting streaming on capture devices
      void qbufall();

      //! Queue all buffers that are not already queued except one specified
      void qbufallbutone(size_t const index);
     
      //! Dequeue the next captured/displayed buffer, blocks until one is available
      void dqbuf(struct v4l2_buffer & buf);

      //! Dequeue all buffers, typically used when stopping a stream, not that this may take some time
      void dqbufall();

    private:
      int const itsFd;
      std::string const itsName;
      v4l2_buf_type const itsType;
      std::vector<std::shared_ptr<VideoBuf> > itsBuffers;
      size_t itsNqueued;
  };

} // namespace jevois

