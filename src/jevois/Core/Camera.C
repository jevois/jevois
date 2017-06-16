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

#include <jevois/Core/Camera.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Core/VideoMapping.H>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace
{
  //! Temporary fix for bug in sunxi-vfe kernel camera driver which returns MBUS format instead or V4L2
  unsigned int v4l2sunxiFix(unsigned int fcc)
  {
    switch (fcc)
    {
    case 0x2008: // Handle bug in our sunxi camera driver
    case V4L2_PIX_FMT_YUYV: return V4L2_PIX_FMT_YUYV;
      
    case V4L2_PIX_FMT_GREY: return V4L2_PIX_FMT_GREY;
      
    case 0x3001: // Handle bug in our sunxi camera driver
    case V4L2_PIX_FMT_SRGGB8: return V4L2_PIX_FMT_SRGGB8;
      
    case 0x1008: // Handle bug in our sunxi camera driver
    case V4L2_PIX_FMT_RGB565: return V4L2_PIX_FMT_RGB565;
      
    case V4L2_PIX_FMT_MJPEG: return V4L2_PIX_FMT_MJPEG;
      
    case V4L2_PIX_FMT_BGR24: return V4L2_PIX_FMT_BGR24;
      
    default: LFATAL("Unsupported pixel format " << jevois::fccstr(fcc));
    }
  }
}

// ##############################################################################################################
jevois::Camera::Camera(std::string const & devname, unsigned int const nbufs) :
    jevois::VideoInput(devname, nbufs), itsFd(-1), itsBuffers(nullptr), itsFormat(), itsStreaming(false),
    itsFps(0.0F), itsDoneIdx(itsInvalidIdx), itsRunning(false)
{
  JEVOIS_TRACE(1);

  JEVOIS_TIMED_LOCK(itsMtx);
  
  // Get our run() thread going and wait until it is cranking, it will flip itsRunning to true as it starts:
  itsRunFuture = std::async(std::launch::async, &jevois::Camera::run, this);
  while (itsRunning.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // Open the device:
  itsFd = open(devname.c_str(), O_RDWR | O_NONBLOCK, 0);
  if (itsFd == -1) PLFATAL("Camera device open failed on " << devname);

  // See what kinds of inputs we have and select the first one that is a camera:
  int camidx = -1;
  struct v4l2_input inp = { };
  while (true)
  {
    try { XIOCTL_QUIET(itsFd, VIDIOC_ENUMINPUT, &inp); } catch (...) { break; }
    if (inp.type == V4L2_INPUT_TYPE_CAMERA)
    {
      if (camidx == -1) camidx = inp.index;
      LDEBUG("Input " << inp.index << " [" << inp.name << "] is a camera sensor");
    } else LDEBUG("Input " << inp.index << " [" << inp.name << "] is a NOT camera sensor");
    ++inp.index;
  }

  if (camidx == -1) LFATAL("No valid camera input found on device " << devname);
  
  // Select the camera input, this seems to be required by VFE for the camera to power on:
  XIOCTL(itsFd, VIDIOC_S_INPUT, &camidx);

  // Find out what camera can do:
  struct v4l2_capability cap = { };
  XIOCTL(itsFd, VIDIOC_QUERYCAP, &cap);
  
  LINFO('[' << itsFd << "] V4L2 camera " << devname << " card " << cap.card << " bus " << cap.bus_info);
  if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) LFATAL(devname << " is not a video capture device");
  if ((cap.capabilities & V4L2_CAP_STREAMING) == 0) LFATAL(devname << " does not support streaming");
  
  // List the supported formats:
  struct v4l2_fmtdesc fmtdesc = { };
  fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  while (true)
  {
    try { XIOCTL_QUIET(itsFd, VIDIOC_ENUM_FMT, &fmtdesc); } catch (...) { break; }
    LDEBUG("Format " << fmtdesc.index << " is [" << fmtdesc.description << "] fcc " << std::showbase <<
           std::hex << fmtdesc.pixelformat << " [" << jevois::fccstr(fmtdesc.pixelformat) << ']');
    ++fmtdesc.index;
  }
}

// ##############################################################################################################
void jevois::Camera::setFormat(jevois::VideoMapping const & m)
{
  JEVOIS_TRACE(2);

  JEVOIS_TIMED_LOCK(itsMtx);
  
  // Get current format:
  itsFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  XIOCTL(itsFd, VIDIOC_G_FMT, &itsFormat);
  
  // Set desired format:
  itsFormat.fmt.pix.width = m.cw;
  itsFormat.fmt.pix.height = m.ch;
  itsFormat.fmt.pix.pixelformat = m.cfmt;
  itsFormat.fmt.pix.field = V4L2_FIELD_NONE;
  itsFps = m.cfps;
  
  LDEBUG("Requesting video format " << itsFormat.fmt.pix.width << 'x' << itsFormat.fmt.pix.height << ' ' <<
         jevois::fccstr(itsFormat.fmt.pix.pixelformat));
  
  XIOCTL(itsFd, VIDIOC_S_FMT, &itsFormat);
  
  // Get the format back as the driver may have adjusted some sizes, etc:
  XIOCTL(itsFd, VIDIOC_G_FMT, &itsFormat);
  
  // The driver returns a different format code, may be the mbus code instead of the v4l2 fcc...
  itsFormat.fmt.pix.pixelformat = v4l2sunxiFix(itsFormat.fmt.pix.pixelformat);
  
  LINFO("Camera set video format to " << itsFormat.fmt.pix.width << 'x' << itsFormat.fmt.pix.height << ' ' <<
        jevois::fccstr(itsFormat.fmt.pix.pixelformat));
  
  // Because modules may rely on the exact format that they request, throw if the camera modified it:
  if (itsFormat.fmt.pix.width != m.cw || itsFormat.fmt.pix.height != m.ch || itsFormat.fmt.pix.pixelformat != m.cfmt)
    LFATAL("Camera did not accept the requested video format as specified");
  
  // Reset cropping parameters. NOTE: just open()'ing the device does not reset it, according to the unix toolchain
  // philosophy. Hence, although here we do not provide support for cropping, we still need to ensure that it is
  // properly reset. Note that some cameras do not support this so here we swallow that exception:
  try
  {
    struct v4l2_cropcap cropcap = { };
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    XIOCTL_QUIET(itsFd, VIDIOC_CROPCAP, &cropcap);
    
    struct v4l2_crop crop = { };
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; crop.c = cropcap.defrect;
    XIOCTL_QUIET(itsFd, VIDIOC_S_CROP, &crop);
    
    LDEBUG("Set cropping rectangle to " << cropcap.defrect.width << 'x' << cropcap.defrect.height << " @ ("
           << cropcap.defrect.left << ", " << cropcap.defrect.top << ')');
  }
  catch (...) { LDEBUG("Querying/setting crop rectangle not supported"); }
  
  // Set frame rate:
  try
  {
    struct v4l2_streamparm parms = { };
    parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parms.parm.capture.timeperframe = jevois::VideoMapping::fpsToV4l2(m.cfps);
    parms.parm.capture.capturemode = 2; // V4L2_MODE_VIDEO not defined in our headers? its value is 2.
    XIOCTL(itsFd, VIDIOC_S_PARM, &parms);
    
    LDEBUG("Set framerate to " << m.cfps << " fps");
  }
  catch (...) { LERROR("Setting frame rate to " << m.cfps << " fps failed -- IGNORED"); }
}

// ##############################################################################################################
jevois::Camera::~Camera()
{
  JEVOIS_TRACE(1);

  // Turn off streaming if it was on:
  try { streamOff(); } catch (...) { jevois::warnAndIgnoreException(); }
 
  // Block until the run() thread completes:
  itsRunning.store(false);
  if (itsRunFuture.valid()) try { itsRunFuture.get(); } catch (...) { jevois::warnAndIgnoreException(); }

  if (itsBuffers) delete itsBuffers;
  
  if (close(itsFd) == -1) PLERROR("Error closing V4L2 camera");
}

// ##############################################################################################################
void jevois::Camera::run()
{
  JEVOIS_TRACE(1);
  
  fd_set rfds; // For new images captured
  fd_set efds; // For errors
  struct timeval tv;

  // Switch to running state:
  itsRunning.store(true);

  // We may have to wait until the device is opened:
  while (itsFd == -1) std::this_thread::sleep_for(std::chrono::milliseconds(1));

  LDEBUG("run() thread ready");

  // NOTE: The flow is a little complex here, the goal is to minimize latency between a frame being captured and us
  // dequeueing it from the driver and making it available to get(). To achieve low latency, we thus need to be polling
  // the driver most of the time, and we need to prevent other threads from doing various ioctls while we are polling,
  // as the SUNXI-VFE driver does not like that. Thus, there is high contention on itsMtx which we lock most of the
  // time. For this reason we do a bit of sleeping with itsMtx unlocked at places where we know it will not increase our
  // captured image delivery latency.
  
  // Wait for event from the gadget kernel driver and process them:
  while (itsRunning.load())
    try
    {
      // Requeue any done buffer. To avoid having to use a double lock on itsOutputMtx (for itsDoneIdx) and itsMtx (for
      // itsBuffers->qbuf()), we just copy itsDoneIdx into a local variable here, and invalidate it, with itsOutputMtx
      // locked, then we will do the qbuf() later, if needed, while itsMtx is locked:
      size_t doneidx;
      {
        std::lock_guard<std::mutex> _(itsOutputMtx);
        doneidx = itsDoneIdx;
        itsDoneIdx = itsInvalidIdx;
      }

      std::unique_lock<std::timed_mutex> lck(itsMtx);

      // Do the actual qbuf of any done buffer:
      if (doneidx != itsInvalidIdx) itsBuffers->qbuf(doneidx);
      
      // SUNXI-VFE does not like to be polled when not streaming; if indeed we are not streaming, unlock and then sleep
      // a bit to avoid too much contention on itsMtx:
      if (itsStreaming.load() == false)
      {
        lck.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }
      
      // Poll the device to wait for any new captured video frame:
      FD_ZERO(&rfds); FD_ZERO(&efds); FD_SET(itsFd, &rfds); FD_SET(itsFd, &efds);
      tv.tv_sec = 0; tv.tv_usec = 5000;

      int ret = select(itsFd + 1, &rfds, nullptr, &efds, &tv);
      if (ret == -1) { PLERROR("Select error"); if (errno == EINTR) continue; else break; }
      else if (ret > 0) // NOTE: ret == 0 would mean timeout
      {
        if (FD_ISSET(itsFd, &efds)) LFATAL("Camera device error");

        if (FD_ISSET(itsFd, &rfds))
        {
          // A new frame has been captured. Dequeue a buffer from the camera driver:
          struct v4l2_buffer buf;
          itsBuffers->dqbuf(buf);

          // Create a RawImage from that buffer:
          jevois::RawImage img;
          img.width = itsFormat.fmt.pix.width;
          img.height = itsFormat.fmt.pix.height;
          img.fmt = itsFormat.fmt.pix.pixelformat;
          img.fps = itsFps;
          img.buf = itsBuffers->get(buf.index);
          img.bufindex = buf.index;

          // Unlock itsMtx:
          lck.unlock();

          // We want to never block waiting for people to consume our grabbed frames here, hence we just overwrite our
          // output image here, it just always contains the latest grabbed image:
          {
            std::lock_guard<std::mutex> _(itsOutputMtx);
            itsOutputImage = img;
          }
          LDEBUG("Captured image " << img.bufindex << " ready for processing");

          // Let anyone trying to get() our image know it's here:
          itsOutputCondVar.notify_all();

          // This is also a good time to sleep a bit since it will take a while for the next frame to arrive, this
          // should allow people who had been trying to get a lock on itsMtx to get it now:
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
      }
    } catch (...) { jevois::warnAndIgnoreException(); }
  
  // Switch out of running state in case we did interrupt the loop here by a break statement:
  itsRunning.store(false);
}

// ##############################################################################################################
void jevois::Camera::streamOn()
{
  JEVOIS_TRACE(2);
  
  LDEBUG("Turning on camera stream");

  JEVOIS_TIMED_LOCK(itsMtx);

  if (itsStreaming.load() || itsBuffers) { LERROR("Stream is already on -- IGNORED"); return; }

  itsStreaming.store(false); // just in case user forgot to call abortStream()

  // If number of buffers is zero, adjust it depending on frame size:
  unsigned int nbuf = itsNbufs;
  if (nbuf == 0)
  {
    unsigned int framesize = jevois::v4l2ImageSize(itsFormat.fmt.pix.pixelformat, itsFormat.fmt.pix.width,
                                                   itsFormat.fmt.pix.height);

    // Aim for about 4 mbyte when using small images:
    nbuf = (4U * 1024U * 1024U) / framesize;
  }

  // Force number of buffers to a sane value:
  if (nbuf < 3) nbuf = 3; else if (nbuf > 63) nbuf = 63;
  
  // Allocate the buffers for our current video format:
  itsBuffers = new jevois::VideoBuffers("camera", itsFd, V4L2_BUF_TYPE_VIDEO_CAPTURE, nbuf);
  LINFO(itsBuffers->size() << " buffers of " << itsBuffers->get(0)->length() << " bytes allocated");

  // Enqueue all our buffers:
  itsBuffers->qbufall();
  LDEBUG("All buffers queued to camera driver");
  
  // Start streaming at the device level:
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  XIOCTL(itsFd, VIDIOC_STREAMON, &type);
  LDEBUG("Device stream on");
  
  itsStreaming.store(true);
  LDEBUG("Streaming is on");
}

// ##############################################################################################################
void jevois::Camera::abortStream()
{
  JEVOIS_TRACE(2);

  // Set its Streaming to false here while unlocked, which will introduce some sleeping in our run() thread, thereby
  // helping us acquire our needed double lock:
  itsStreaming.store(false);

  // Unblock any get() that is waiting on itsOutputCondVar, it will then throw now that streaming is off:
  itsOutputCondVar.notify_all();
}

// ##############################################################################################################
void jevois::Camera::streamOff()
{
  JEVOIS_TRACE(2);

  // Note: we allow for several streamOff() without complaining, this happens, e.g., when destroying a Camera that is
  // not currently streaming.
  
  LDEBUG("Turning off camera stream");

  // Abort stream in case it was not already done, which will introduce some sleeping in our run() thread, thereby
  // helping us acquire our needed double lock:
  abortStream();

  // We need a double lock here so that we can both turn off the stream and nuke our output image and done idx:
  std::unique_lock<std::timed_mutex> lk1(itsMtx, std::defer_lock);
  std::unique_lock<std::mutex> lk2(itsOutputMtx, std::defer_lock);
  std::lock(lk1, lk2);

  // Invalidate our output image:
  itsOutputImage.invalidate();

  // User may have called done() but our run() thread has not yet gotten to requeueing this image, if so requeue it here
  // as it seems to keep the driver happier:
  if (itsDoneIdx != itsInvalidIdx && itsBuffers) itsBuffers->qbuf(itsDoneIdx);
  itsDoneIdx = itsInvalidIdx;
  
  // Stop streaming at the device level:
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  try { XIOCTL_QUIET(itsFd, VIDIOC_STREAMOFF, &type); } catch (...) { }

  // Nuke all the buffers:
  if (itsBuffers) { delete itsBuffers; itsBuffers = nullptr; }

  // Unblock any get() that is waiting on itsOutputCondVar, it will then throw now that streaming is off:
  lk2.unlock();
  itsOutputCondVar.notify_all();

  LDEBUG("Camera stream is off");
}

// ##############################################################################################################
void jevois::Camera::get(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);

  {
    std::unique_lock<std::mutex> ulck(itsOutputMtx);
    itsOutputCondVar.wait(ulck, [&]() { return itsOutputImage.valid() || itsStreaming.load() == false; });
    if (itsStreaming.load() == false) { LDEBUG("Not streaming"); throw std::runtime_error("Camera not streaming"); }
    img = itsOutputImage;
    itsOutputImage.invalidate();
  }
  
  LDEBUG("Camera image " << img.bufindex << " handed over to processing");
}

// ##############################################################################################################
void jevois::Camera::done(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);

  if (itsStreaming.load() == false)
  { LDEBUG("Not streaming"); throw std::runtime_error("Camera done() rejected while not streaming"); }

  // To avoid blocking for a long time here, we do not try to lock itsMtx and to qbuf() the buffer right now, instead we
  // just make a note that this buffer is available and it will be requeued by our run() thread:
  {
    std::lock_guard<std::mutex> _(itsOutputMtx);
    if (itsDoneIdx != itsInvalidIdx) LFATAL("Previous done() image not yet recycled by driver");
    itsDoneIdx = img.bufindex;
  }

  LDEBUG("Image " << img.bufindex << " freed by processing");
}

// ##############################################################################################################
void jevois::Camera::queryControl(struct v4l2_queryctrl & qc) const
{
  XIOCTL_QUIET(itsFd, VIDIOC_QUERYCTRL, &qc);
}

// ##############################################################################################################
void jevois::Camera::queryMenu(struct v4l2_querymenu & qm) const
{
  XIOCTL_QUIET(itsFd, VIDIOC_QUERYMENU, &qm);
}

// ##############################################################################################################
void jevois::Camera::getControl(struct v4l2_control & ctrl) const
{
#ifdef JEVOIS_PLATFORM
  XIOCTL(itsFd, 0xc00c561b /* should be VIDIOC_G_CTRL, bug in kernel headers? */, &ctrl);
#else
  XIOCTL(itsFd, VIDIOC_G_CTRL, &ctrl);
#endif
}

// ##############################################################################################################
void jevois::Camera::setControl(struct v4l2_control const & ctrl)
{
#ifdef JEVOIS_PLATFORM
  XIOCTL(itsFd, 0xc00c561c /* should be VIDIOC_S_CTRL, bug in kernel headers? */, &ctrl);
#else
  XIOCTL(itsFd, VIDIOC_S_CTRL, &ctrl);
#endif  
}

// ##############################################################################################################
void jevois::Camera::writeRegister(unsigned char reg, unsigned char val)
{
  unsigned char data[2] = { reg, val };

  LINFO("Writing 0x" << std::hex << val << " to 0x" << reg);
  XIOCTL(itsFd, _IOW('V', 192, short), data);
}

// ##############################################################################################################
unsigned char jevois::Camera::readRegister(unsigned char reg)
{
  unsigned char data[2] = { reg, 0 };

  XIOCTL(itsFd, _IOWR('V', 193, short), data);
  LINFO("Register 0x" << std::hex << reg << " has value 0x" << data[1]);
  return data[1];
}
