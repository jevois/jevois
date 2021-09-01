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

#include <jevois/Core/CameraDevice.H>
#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Util/Async.H>
#include <jevois/Core/VideoMapping.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Core/ICM20948_regs.H>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#define FDLDEBUG(msg) LDEBUG('[' << itsDevName << ':' << itsFd << "] " << msg)
#define FDLINFO(msg) LINFO('[' << itsDevName << ':' << itsFd << "] " << msg)
#define FDLERROR(msg) LERROR('[' << itsDevName << ':' << itsFd << "] " << msg)
#define FDLFATAL(msg) LFATAL('[' << itsDevName << ':' << itsFd << "] " << msg)

// V4L2_MODE_VIDEO not defined in our kernel? Needed by capturemode of VIDIOC_S_PARM
#ifndef V4L2_MODE_VIDEO
#define V4L2_MODE_VIDEO 2
#endif

#ifdef JEVOIS_PLATFORM_A33
namespace
{
  //! Temporary fix for bug in sunxi-vfe kernel camera driver which returns MBUS format instead or V4L2
  unsigned int v4l2sunxiFix(unsigned int fcc)
  {
    switch (fcc)
    {
    case 0x2008: // Handle bug in our sunxi camera driver
    case V4L2_PIX_FMT_YUYV: return V4L2_PIX_FMT_YUYV;
      
    case 0x2001: // Handle bug in our sunxi camera driver
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

// Define a few things which are absent from our A33 platform kernel 3.4:
#define V4L2_COLORSPACE_DEFAULT v4l2_colorspace(0)
#endif

// ##############################################################################################################
jevois::CameraDevice::CameraDevice(std::string const & devname, unsigned int const nbufs, bool dummy) :
    itsDevName(devname), itsNbufs(nbufs), itsBuffers(nullptr), itsStreaming(false), itsFormatOk(false),
    itsRunning(false)
{
  JEVOIS_TRACE(1);

  // Open the device:
  itsFd = open(devname.c_str(), O_RDWR | O_NONBLOCK, 0);
  if (itsFd == -1) LFATAL("Camera device open failed on " << devname);
  
  // See what kinds of inputs we have and select the first one that is a camera:
  int camidx = -1;
  struct v4l2_input inp = { };
  while (true)
  {
    try { XIOCTL_QUIET(itsFd, VIDIOC_ENUMINPUT, &inp); } catch (...) { break; }
    if (inp.type == V4L2_INPUT_TYPE_CAMERA)
    {
      if (camidx == -1) camidx = inp.index;
      FDLDEBUG(devname << ": Input " << inp.index << " [" << inp.name << "] is a camera sensor");
    } else FDLDEBUG(devname << ": Input " << inp.index << " [" << inp.name << "] is a NOT camera sensor");
    ++inp.index;
  }

  if (camidx == -1) FDLFATAL("No valid camera input");

  // Select the camera input, this seems to be required by sunxi-VFE on JeVois-A33 for the camera to power on:
  XIOCTL(itsFd, VIDIOC_S_INPUT, &camidx);
  
  // Find out what camera can do:
  struct v4l2_capability cap = { };
  XIOCTL(itsFd, VIDIOC_QUERYCAP, &cap);
  
  FDLINFO("V4L2 camera " << devname << " card " << cap.card << " bus " << cap.bus_info);

  // Amlogic ISP V4L2 does not report a video capture capability, but it has video capture mplane
  if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) itsMplane = true;

  if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0 && itsMplane == false)
    FDLFATAL(devname << " is not a video capture device");

  if ((cap.capabilities & V4L2_CAP_STREAMING) == 0)
    FDLFATAL(devname << " does not support streaming");
  
  // List the supported formats, only once:
  static bool showfmts = true;
  if (dummy == false && showfmts)
  {
    struct v4l2_fmtdesc fmtdesc = { };
    if (itsMplane) fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; else fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (true)
    {
      try { XIOCTL_QUIET(itsFd, VIDIOC_ENUM_FMT, &fmtdesc); } catch (...) { break; }
      FDLINFO("Supported format " << fmtdesc.index << " is [" << fmtdesc.description << "] fcc " << std::showbase <<
              std::hex << fmtdesc.pixelformat << " [" << jevois::fccstr(fmtdesc.pixelformat) << ']');
      ++fmtdesc.index;
    }
    showfmts = false;
  }

  // Get our run() thread going and wait until it is cranking, it will flip itsRunning to true as it starts:
  if (dummy == false)
  {
    itsRunFuture = jevois::async_little(std::bind(&jevois::CameraDevice::run, this));
    while (itsRunning.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

// ##############################################################################################################
jevois::CameraDevice::~CameraDevice()
{
  JEVOIS_TRACE(1);

  // Turn off streaming if it was on:
  try { streamOff(); } catch (...) { jevois::warnAndIgnoreException(); }
 
  // Block until the run() thread completes:
  itsRunning.store(false);
  JEVOIS_WAIT_GET_FUTURE(itsRunFuture);

  while (true)
  {
    std::unique_lock lck(itsMtx, std::chrono::seconds(5));
    if (lck.owns_lock() == false) { FDLERROR("Timeout trying to acquire camera lock"); continue; }
    
    if (itsBuffers) delete itsBuffers;
    if (itsFd != -1) close(itsFd);
    break;
  }
}

// ##############################################################################################################
int jevois::CameraDevice::getFd() const
{ return itsFd; }

// ##############################################################################################################
void jevois::CameraDevice::run()
{
  JEVOIS_TRACE(1);
  
  fd_set rfds; // For new images captured
  fd_set efds; // For errors
  struct timeval tv;

  // Switch to running state:
  itsRunning.store(true);
  LDEBUG("run() thread ready");

  // NOTE: The flow is a little complex here, the goal is to minimize latency between a frame being captured and us
  // dequeueing it from the driver and making it available to get(). To achieve low latency, we thus need to be polling
  // the driver most of the time, and we need to prevent other threads from doing various ioctls while we are polling,
  // as the SUNXI-VFE driver does not like that. Thus, there is high contention on itsMtx which we lock most of the
  // time. For this reason we do a bit of sleeping with itsMtx unlocked at places where we know it will not increase our
  // captured image delivery latency.
  std::vector<size_t> doneidx;
  
  // Wait for events from the kernel driver and process them:
  while (itsRunning.load())
    try
    {
      // Requeue any done buffer. To avoid having to use a double lock on itsOutputMtx (for itsDoneIdx) and itsMtx (for
      // itsBuffers->qbuf()), we just swap itsDoneIdx into a local variable here, and invalidate it, with itsOutputMtx
      // locked, then we will do the qbuf() later, if needed, while itsMtx is locked:
      {
        JEVOIS_TIMED_LOCK(itsOutputMtx);
        if (itsDoneIdx.empty() == false) itsDoneIdx.swap(doneidx);
      }

      std::unique_lock lck(itsMtx, std::chrono::seconds(5));
      if (lck.owns_lock() == false) FDLFATAL("Timeout trying to acquire camera lock");

      // Do the actual qbuf of any done buffer, ignoring any exception:
      if (itsBuffers) { for (size_t idx : doneidx) try { itsBuffers->qbuf(idx); } catch (...) { } }
      doneidx.clear();

      // SUNXI-VFE does not like to be polled when not streaming; if indeed we are not streaming, unlock and then sleep
      // a bit to avoid too much contention on itsMtx:
      if (itsStreaming.load() == false)
      {
        lck.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }
      
      // Check whether user code cannot keep up with the frame rate, and if so requeue all dequeued buffers except maybe
      // the one currently associated with itsOutputImage:
      if (itsBuffers && itsBuffers->nqueued() < 4)
      {
        LERROR("Running out of camera buffers - your process() function is too slow - DROPPING FRAMES");
        size_t keep = 12345678;

        lck.unlock();
        {
          JEVOIS_TIMED_LOCK(itsOutputMtx);
          if (itsOutputImage.valid()) keep = itsOutputImage.bufindex;
        }
        lck.lock();

        itsBuffers->qbufallbutone(keep);
      }

      // Poll the device to wait for any new captured video frame:
      FD_ZERO(&rfds); FD_ZERO(&efds); FD_SET(itsFd, &rfds); FD_SET(itsFd, &efds);
      tv.tv_sec = 0; tv.tv_usec = 5000;

      int ret = select(itsFd + 1, &rfds, nullptr, &efds, &tv);
      if (ret == -1) { FDLERROR("Select error"); if (errno == EINTR) continue; else PLFATAL("Error polling camera"); }
      else if (ret > 0) // NOTE: ret == 0 would mean timeout
      {
        if (FD_ISSET(itsFd, &efds)) FDLFATAL("Camera device error");

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
            JEVOIS_TIMED_LOCK(itsOutputMtx);

            // If user never called get()/done() on an image we already have, drop it and requeue the buffer:
            if (itsOutputImage.valid()) itsDoneIdx.push_back(itsOutputImage.bufindex);

            // Set our new output image:
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
void jevois::CameraDevice::streamOn()
{
  JEVOIS_TRACE(2);

  LDEBUG("Turning on camera stream");

  JEVOIS_TIMED_LOCK(itsMtx);

  if (itsFormatOk == false) FDLFATAL("No valid capture format was set -- ABORT");
  
  if (itsStreaming.load() || itsBuffers) { FDLERROR("Stream is already on -- IGNORED"); return; }

  itsStreaming.store(false); // just in case user forgot to call abortStream()
  /*
  // If number of buffers is zero, adjust it depending on frame size:
  unsigned int nbuf = itsNbufs;
  if (nbuf == 0)
  {
    unsigned int framesize = jevois::v4l2ImageSize(itsFormat.fmt.pix.pixelformat, itsFormat.fmt.pix.width,
                                                   itsFormat.fmt.pix.height);

    // Aim for about 4 mbyte when using small images, and no more than 5 buffers in any case:
    nbuf = (4U * 1024U * 1024U) / framesize;
    if (nbuf > 5) nbuf = 5;
  }

  // Force number of buffers to a sane value:
  if (nbuf < 3) nbuf = 3; else if (nbuf > 63) nbuf = 63;
  */
  unsigned int nbuf = 10;
  
  // Allocate the buffers for our current video format:
  v4l2_buf_type btype = itsMplane ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
  itsBuffers = new jevois::VideoBuffers("camera", itsFd, btype, nbuf);
  FDLINFO(itsBuffers->size() << " buffers of " << itsBuffers->get(0)->length() << " bytes allocated");

  // Enqueue all our buffers:
  itsBuffers->qbufall();
  FDLDEBUG("All buffers queued to camera driver");
  
  // Start streaming at the device level:
  XIOCTL(itsFd, VIDIOC_STREAMON, &btype);
  FDLDEBUG("Device stream on");
  
  itsStreaming.store(true);
  FDLDEBUG("Streaming is on");
}

// ##############################################################################################################
void jevois::CameraDevice::abortStream()
{
  JEVOIS_TRACE(2);

  // Set its Streaming to false here while unlocked, which will introduce some sleeping in our run() thread, thereby
  // helping us acquire our needed double lock:
  itsStreaming.store(false);

  // Unblock any get() that is waiting on itsOutputCondVar, it will then throw now that streaming is off:
  for (int i = 0; i < 20; ++i) itsOutputCondVar.notify_all();
}

// ##############################################################################################################
void jevois::CameraDevice::streamOff()
{
  JEVOIS_TRACE(2);

  // Note: we allow for several streamOff() without complaining, this happens, e.g., when destroying a Camera that is
  // not currently streaming.
  
  FDLDEBUG("Turning off camera stream");

  // Abort stream in case it was not already done, which will introduce some sleeping in our run() thread, thereby
  // helping us acquire our needed double lock:
  abortStream();

  // We need a double lock here so that we can both turn off the stream and nuke our output image and done idx:
  std::unique_lock<std::timed_mutex> lk1(itsMtx, std::defer_lock);
  std::unique_lock<std::timed_mutex> lk2(itsOutputMtx, std::defer_lock);
  LDEBUG("Ready to double-lock...");
  std::lock(lk1, lk2);
  LDEBUG("Double-lock success.");

  // Invalidate our output image:
  itsOutputImage.invalidate();

  // User may have called done() but our run() thread has not yet gotten to requeueing this image, if so requeue it here
  // as it seems to keep the driver happier:
  if (itsBuffers)
    for (size_t idx : itsDoneIdx) try { itsBuffers->qbuf(idx); } catch (...) { jevois::warnAndIgnoreException(); }
  itsDoneIdx.clear();
  
  // Stop streaming at the device level:
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  try { XIOCTL_QUIET(itsFd, VIDIOC_STREAMOFF, &type); } catch (...) { }

  // Nuke all the buffers:
  if (itsBuffers) { delete itsBuffers; itsBuffers = nullptr; }

  // Unblock any get() that is waiting on itsOutputCondVar, it will then throw now that streaming is off:
  lk2.unlock();
  for (int i = 0; i < 20; ++i) itsOutputCondVar.notify_all();

  FDLDEBUG("Camera stream is off");
}

// ##############################################################################################################
void jevois::CameraDevice::get(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);

  if (itsConvertedOutputImage.valid())
  {
    // We need to convert from Bayer/Mono to YUYV:
    std::unique_lock ulck(itsOutputMtx, std::chrono::seconds(5));
    if (ulck.owns_lock() == false) FDLFATAL("Timeout trying to acquire output lock");
    
    if (itsOutputImage.valid() == false)
    {
      if (itsOutputCondVar.wait_for(ulck, std::chrono::milliseconds(2500),
                                    [&]() { return itsOutputImage.valid() || itsStreaming.load() == false; }) == false)
        throw std::runtime_error("Timeout waiting for camera frame or camera not streaming");
    }

    if (itsStreaming.load() == false) throw std::runtime_error("Camera not streaming");

    switch (itsFormat.fmt.pix.pixelformat) // FIXME may need to protect this?
    {
    case V4L2_PIX_FMT_SRGGB8: jevois::rawimage::convertBayerToYUYV(itsOutputImage, itsConvertedOutputImage); break;
    case V4L2_PIX_FMT_GREY: jevois::rawimage::convertGreyToYUYV(itsOutputImage, itsConvertedOutputImage); break;
    default: FDLFATAL("Oops, cannot convert captured image");
    }

    img = itsConvertedOutputImage;
    img.bufindex = itsOutputImage.bufindex;
    itsOutputImage.invalidate();
  }
  else
  {
    // Regular get() with no conversion:
    std::unique_lock ulck(itsOutputMtx, std::chrono::seconds(5));
    if (ulck.owns_lock() == false) FDLFATAL("Timeout trying to acquire output lock");

    if (itsOutputImage.valid() == false)
    {
      if (itsOutputCondVar.wait_for(ulck, std::chrono::milliseconds(2500),
                                    [&]() { return itsOutputImage.valid() || itsStreaming.load() == false; }) == false)
        throw std::runtime_error("Timeout waiting for camera frame or camera not streaming");
    }
    
    if (itsStreaming.load() == false) throw std::runtime_error("Camera not streaming");

    img = itsOutputImage;
    itsOutputImage.invalidate();
  }
  
  LDEBUG("Camera image " << img.bufindex << " handed over to processing");
}

// ##############################################################################################################
void jevois::CameraDevice::done(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);

  if (itsStreaming.load() == false) throw std::runtime_error("Camera done() rejected while not streaming");

  // To avoid blocking for a long time here, we do not try to lock itsMtx and to qbuf() the buffer right now, instead we
  // just make a note that this buffer is available and it will be requeued by our run() thread:
  JEVOIS_TIMED_LOCK(itsOutputMtx);
  itsDoneIdx.push_back(img.bufindex);

  LDEBUG("Image " << img.bufindex << " freed by processing");
}

// ##############################################################################################################
void jevois::CameraDevice::setFormat(unsigned int const fmt, unsigned int const capw, unsigned int const caph,
                                     float const fps, unsigned int const cropw, unsigned int const croph)
{
  JEVOIS_TRACE(2);

  // We may be streaming, eg, if we were running a mapping with no USB out and then the user starts a video grabber. So
  // make sure we stream off first:
  if (itsStreaming.load()) streamOff();
  
  JEVOIS_TIMED_LOCK(itsMtx);

  // Assume format not set in case we exit on exception:
  itsFormatOk = false;
  
  // Set desired format:
  if (itsMplane)
  {
    // Get current format:
    itsFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    XIOCTL(itsFd, VIDIOC_G_FMT, &itsFormat);

    // Set desired format:
    // see https://chromium.googlesource.com/chromiumos/platform/cros-yavta/+/refs/heads/upstream/master/yavta.c
    itsFormat.fmt.pix_mp.width = capw;
    itsFormat.fmt.pix_mp.height = caph;
    itsFormat.fmt.pix_mp.pixelformat = fmt;
    itsFormat.fmt.pix_mp.num_planes = 1; // FIXME force to 1 plane, likely will break NV12 support
    itsFormat.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    itsFormat.fmt.pix_mp.field = V4L2_FIELD_NONE;
    itsFps = fps;
    LDEBUG("Requesting multiplane video format " << itsFormat.fmt.pix.width << 'x' << itsFormat.fmt.pix.height << ' ' <<
           jevois::fccstr(itsFormat.fmt.pix.pixelformat));

    // Amlogic kernel bugfix: still set the regular fields:
    itsFormat.fmt.pix.width = capw;
    itsFormat.fmt.pix.height = caph;
    itsFormat.fmt.pix.pixelformat = fmt;
    itsFormat.fmt.pix.colorspace = V4L2_COLORSPACE_DEFAULT;
    itsFormat.fmt.pix.field = V4L2_FIELD_NONE;
  }
  else
  {
    // Get current format:
    itsFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    XIOCTL(itsFd, VIDIOC_G_FMT, &itsFormat);

    // Set desired format:
    itsFormat.fmt.pix.width = capw;
    itsFormat.fmt.pix.height = caph;
    itsFormat.fmt.pix.pixelformat = fmt;
    itsFormat.fmt.pix.colorspace = V4L2_COLORSPACE_DEFAULT;
    itsFormat.fmt.pix.field = V4L2_FIELD_NONE;
    itsFps = fps;
    LDEBUG("Requesting video format " << itsFormat.fmt.pix.width << 'x' << itsFormat.fmt.pix.height << ' ' <<
           jevois::fccstr(itsFormat.fmt.pix.pixelformat));
  }
  
  // Try to set the format. If it fails, try to see whether we can use BAYER or MONO instead, and we will convert:
  try
  {
    XIOCTL_QUIET(itsFd, VIDIOC_S_FMT, &itsFormat);
  }
  catch (...)
  {
    if (itsMplane)
      FDLFATAL("Could not set camera format to " << capw << 'x' << caph << ' ' << jevois::fccstr(fmt) <<
               ". Maybe the sensor does not support requested pixel type or resolution.");
    else
    {
      try
      {
        // Oops, maybe this sensor only supports raw Bayer:
        itsFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_SRGGB8;
        XIOCTL_QUIET(itsFd, VIDIOC_S_FMT, &itsFormat);
      }
      catch (...)
      {
        try
        {
          // Oops, maybe this sensor only supports monochrome:
          itsFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
          XIOCTL_QUIET(itsFd, VIDIOC_S_FMT, &itsFormat);
        }
        catch (...)
        {
          FDLFATAL("Could not set camera format to " << capw << 'x' << caph << ' ' << jevois::fccstr(fmt) <<
                   ". Maybe the sensor does not support requested pixel type or resolution.");
        }
      }
    }
  }
  
  // Get the format back as the driver may have adjusted some sizes, etc:
  XIOCTL(itsFd, VIDIOC_G_FMT, &itsFormat);
  
  // The sunxi driver on JeVois-A33 returns a different format code, may be the mbus code instead of the v4l2 fcc...
#ifdef JEVOIS_PLATFORM_A33
  itsFormat.fmt.pix.pixelformat = v4l2sunxiFix(itsFormat.fmt.pix.pixelformat);
#endif
  
  FDLINFO("Camera set video format to " << itsFormat.fmt.pix.width << 'x' << itsFormat.fmt.pix.height << ' ' <<
        jevois::fccstr(itsFormat.fmt.pix.pixelformat));
  
  // Because modules may rely on the exact format that they request, throw if the camera modified it:
  if (itsMplane)
  {
    if (itsFormat.fmt.pix_mp.width != capw ||
        itsFormat.fmt.pix_mp.height != caph ||
        itsFormat.fmt.pix_mp.pixelformat != fmt)
      FDLFATAL("Camera did not accept the requested video format as specified");
  }
  else
  {
    if (itsFormat.fmt.pix.width != capw ||
        itsFormat.fmt.pix.height != caph ||
        (itsFormat.fmt.pix.pixelformat != fmt &&
         (fmt != V4L2_PIX_FMT_YUYV ||
          (itsFormat.fmt.pix.pixelformat != V4L2_PIX_FMT_SRGGB8 &&
           itsFormat.fmt.pix.pixelformat != V4L2_PIX_FMT_GREY))))
      FDLFATAL("Camera did not accept the requested video format as specified");
  }
  
  // Reset cropping parameters. NOTE: just open()'ing the device does not reset it, according to the unix toolchain
  // philosophy. Hence, although here we do not provide support for cropping, we still need to ensure that it is
  // properly reset. Note that some cameras do not support this so here we swallow that exception:
  if (fps > 0.0F)
    try
    {
      struct v4l2_cropcap cropcap = { };
      cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // Note: kernel docs say do not use the MPLANE type here.
      XIOCTL_QUIET(itsFd, VIDIOC_CROPCAP, &cropcap);
      
      LDEBUG("Cropcap bounds " << cropcap.bounds.width << 'x' << cropcap.bounds.height <<
             " @ (" << cropcap.bounds.left << ", " << cropcap.bounds.top << ')');
      LDEBUG("Cropcap defrect " << cropcap.defrect.width << 'x' << cropcap.defrect.height <<
             " @ (" << cropcap.defrect.left << ", " << cropcap.defrect.top << ')');
      
      struct v4l2_crop crop = { };
      crop.type = itsFormat.type;
      if (capw == cropw && caph == croph)
        crop.c = cropcap.defrect;
      else
      {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // Note: kernel docs say do not use the MPLANE type here.
        crop.c.top = ((caph - croph) >> 1) & 0xfffc; // force multiple of 4
        crop.c.left = ((capw - cropw) >> 1) & 0xfffc;
        crop.c.width = cropw; crop.c.height = croph;
        
        // From now on, as far as we are concerned, these are the capture width and height:
        itsFormat.fmt.pix.width = cropw;
        itsFormat.fmt.pix.height = croph;
      }
      
      XIOCTL_QUIET(itsFd, VIDIOC_S_CROP, &crop);
      
      LINFO("Set cropping rectangle to " << crop.c.width << 'x' << crop.c.height <<
            " @ (" << crop.c.left << ", " << crop.c.top << ')');
    }
    catch (...) { LERROR("Querying/setting crop rectangle not supported"); }

  // From now on, as far as we are concerned, these are the capture width and height:
  itsFormat.fmt.pix.width = cropw;
  itsFormat.fmt.pix.height = croph;
  
  // Allocate a RawImage for conversion from Bayer or Monochrome to YUYV if needed:
  itsConvertedOutputImage.invalidate();
  if (itsMplane == false && fmt == V4L2_PIX_FMT_YUYV &&
      (itsFormat.fmt.pix.pixelformat == V4L2_PIX_FMT_SRGGB8 || itsFormat.fmt.pix.pixelformat == V4L2_PIX_FMT_GREY))
  {
    // We will grab raw Bayer/Mono and store that into itsOutputImage, finally converting to YUYV in get():
    itsConvertedOutputImage.width = itsFormat.fmt.pix.width;
    itsConvertedOutputImage.height = itsFormat.fmt.pix.height;
    itsConvertedOutputImage.fmt = V4L2_PIX_FMT_YUYV;
    itsConvertedOutputImage.fps = itsFps;
    itsConvertedOutputImage.buf = std::make_shared<jevois::VideoBuf>(-1, itsConvertedOutputImage.bytesize(), 0, -1);
  }
  
  // Set frame rate:
  if (fps > 0.0F)
    try
    {
      struct v4l2_streamparm parms = { };
      parms.type = itsFormat.type;
      parms.parm.capture.timeperframe = jevois::VideoMapping::fpsToV4l2(fps);
      parms.parm.capture.capturemode = V4L2_MODE_VIDEO;
      XIOCTL(itsFd, VIDIOC_S_PARM, &parms);
      
      LDEBUG("Set framerate to " << fps << " fps");
    }
    catch (...) { LERROR("Setting frame rate to " << fps << " fps failed -- IGNORED"); }

  // All good, note that we succeeded:
  itsFormatOk = true;
}
