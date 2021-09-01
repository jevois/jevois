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

#include <jevois/Core/VideoBuffers.H>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/Log.H>
#include <sys/ioctl.h>
#include <thread>

#define FDLDEBUG(msg) LDEBUG('[' << itsFd << ':' << itsName << "] " << msg)
#define FDLINFO(msg) LINFO('[' << itsFd << ':' << itsName << "] " << msg)
#define FDLERROR(msg) LERROR('[' << itsFd << ':' << itsName << "] " << msg)
#define FDLFATAL(msg) LFATAL('[' << itsFd << ':' << itsName << "] " << msg)

// ####################################################################################################
jevois::VideoBuffers::VideoBuffers(char const * name, int const fd, v4l2_buf_type const type, size_t const num) :
    itsFd(fd), itsName(name), itsType(type), itsNqueued(0)
{
  // First, request the buffers:
  struct v4l2_requestbuffers req = { };
  req.count = num;
  req.type = type;
  req.memory = V4L2_MEMORY_MMAP;
  XIOCTL(fd, VIDIOC_REQBUFS, &req);
  FDLDEBUG("Reqbufs for " << num << " buffers returned " << req.count << " buffers");
  
  // Allocate VideoBuf and MMAP the buffers:
  for (unsigned int i = 0; i < req.count; ++i)
  {
    struct v4l2_buffer buf = { };
    struct v4l2_plane plane = { };
    buf.type = type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    // Note: guvcview adds: buf.flags=V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC
    if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
    {
      // In multi-plane mode, allocate a struct for the planes:
      buf.length = 1; // FIXME only 1 plane supported
      buf.m.planes = &plane;
      
      try { XIOCTL(fd, VIDIOC_QUERYBUF, &buf); } catch (...) { FDLFATAL("Failed to request buffers"); }
      if (buf.length != 1) LFATAL("Only one V4L2 plane is supported for now, i.e., only packed video formats.");
    }
    else
    {
      // Standard single-plane mode:
      try { XIOCTL(fd, VIDIOC_QUERYBUF, &buf); } catch (...) { FDLFATAL("Failed to request buffers"); }
    }

    int dmafd = -1;

#ifdef JEVOIS_PRO
    // Try to get a dma_buf fd, if kernel driver supports it:
    struct v4l2_exportbuffer ex_buf = { };
    ex_buf.type = type;
    ex_buf.index = i;
    ex_buf.plane = 0; // FIXME only 1 plane supported
    ex_buf.flags = 0;
    ex_buf.fd = -1;
      
    try { XIOCTL(fd, VIDIOC_EXPBUF, &ex_buf); } catch (...) { FDLDEBUG("Could not get dma_buf fd for buffer " << i); }
    dmafd = ex_buf.fd;
#endif
    
    if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
      itsBuffers.push_back(std::make_shared<jevois::VideoBuf>(fd, buf.m.planes[0].length,
                                                              buf.m.planes[0].m.mem_offset, dmafd));
    else
      itsBuffers.push_back(std::make_shared<jevois::VideoBuf>(fd, buf.length, buf.m.offset, dmafd));
    
    FDLDEBUG("Added mmap'd buffer " << i << " of size " << buf.length);
  }
}

// ####################################################################################################
jevois::VideoBuffers::~VideoBuffers()
{
  if (itsNqueued) FDLDEBUG(itsNqueued << " buffers are still queued...");
  
  // Free and unmap all our buffers:
  for (auto & b : itsBuffers)
  {
    if (b.use_count() > 1) FDLDEBUG("Ref count non zero when attempting to free VideoBuf");

    b.reset(); // VideoBuf destructor will do the memory unmapping
  }

  itsBuffers.clear();
  
  // Then free all buffers at the device level:
  struct v4l2_requestbuffers req = { };
  req.count = 0;  // request 0 buffers to free all the previously requested ones
  req.type = itsType;
  req.memory = V4L2_MEMORY_MMAP;
  try { XIOCTL_QUIET(itsFd, VIDIOC_REQBUFS, &req); }
  catch (...) { FDLDEBUG("Error trying to free V4L2 buffers -- IGNORED"); }
}

// ####################################################################################################
size_t jevois::VideoBuffers::size() const
{
  return itsBuffers.size();
}

// ####################################################################################################
size_t jevois::VideoBuffers::nqueued() const
{
  return itsNqueued;
}

// ####################################################################################################
std::shared_ptr<jevois::VideoBuf> jevois::VideoBuffers::get(size_t const index) const
{
  if (index >= itsBuffers.size()) FDLFATAL("Index " << index << " out of range [0.." << itsBuffers.size() << ']');

  return itsBuffers[index];
}

// ####################################################################################################
void jevois::VideoBuffers::qbuf(size_t const index)
{
  if (itsType == V4L2_BUF_TYPE_VIDEO_OUTPUT) FDLFATAL("Cannot enqueue output buffers by index");
  if (itsNqueued == itsBuffers.size())
    throw std::runtime_error('[' + std::to_string(itsFd) + ':' + itsName + "] All buffers have already been queued");

  struct v4l2_buffer buf = { };
  buf.type = itsType;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = index;

  if (itsType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
  {
    struct v4l2_plane plane = { };
    buf.length = 1; // FIXME only one plane supported.
    buf.m.planes = &plane;
  }

  XIOCTL_QUIET(itsFd, VIDIOC_QBUF, &buf);
  
  ++itsNqueued;
}

// ####################################################################################################
void jevois::VideoBuffers::qbuf(struct v4l2_buffer & buf)
{
  if (itsNqueued == itsBuffers.size())
    throw std::runtime_error('[' + std::to_string(itsFd) + ':' + itsName + "] All buffers have already been queued");
  
  XIOCTL_QUIET(itsFd, VIDIOC_QBUF, &buf);

  ++itsNqueued;
}

// ####################################################################################################
void jevois::VideoBuffers::qbufall()
{
  for (unsigned int i = 0; i < itsBuffers.size(); ++i) qbuf(i);
}

// ####################################################################################################
void jevois::VideoBuffers::qbufallbutone(size_t const index)
{
  for (unsigned int i = 0; i < itsBuffers.size(); ++i)
    if (i != index) try { qbuf(i); } catch (...) { }
}

// ####################################################################################################
void jevois::VideoBuffers::dqbuf(struct v4l2_buffer & buf)
{
  if (itsNqueued == 0) FDLFATAL("No buffer is currently queued");
  
  memset(&buf, 0, sizeof(struct v4l2_buffer));
  buf.type = itsType;
  buf.memory = V4L2_MEMORY_MMAP;
    
  if (itsType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
  {
    struct v4l2_plane plane = { };
    buf.length = 1; // FIXME only one plane supported.
    buf.m.planes = &plane;
  
    XIOCTL_QUIET_ONCE(itsFd, VIDIOC_DQBUF, &buf);
  }
  else
  {
    XIOCTL_QUIET_ONCE(itsFd, VIDIOC_DQBUF, &buf);
  }
  
  --itsNqueued;
}

// ####################################################################################################
void jevois::VideoBuffers::dqbufall()
{
  struct v4l2_buffer buf;
  int retry = 100;
  
  // Loop until all are dequeued; we may need to sleep a bit if some buffers are still in use by the hardware:
  while (itsNqueued && retry-- >= 0)
  {
    try { dqbuf(buf); } catch (...) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
  }
}
