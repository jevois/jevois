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
#include <string.h>
#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef JEVOIS_PLATFORM
#include <opencv2/imgcodecs.hpp>
#else
// On older opencv, imwrite is in highgui:
#include <opencv2/highgui/highgui.hpp>
#endif

// Uncomment this to use th eopencv camera driver instead of the JeVois one:
#define USE_RAW_CAMERA

#ifdef USE_RAW_CAMERA
#define NB_BUFFER 4
#include <jevois/Util/Utils.H>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

//! Main daemon that grabs video frames from the camera, sends them to processing, and sends the results out over USB
int main(int argc, char const* argv[])
{
  jevois::logLevel = LOG_DEBUG;

  if (argc != 5) LFATAL("USAGE: jevois_camtest <YUYV|BAYER|RGB565> <width> <height> <fps>");

  jevois::VideoMapping m;
  if (strcmp(argv[1], "BAYER") == 0) m.cfmt = V4L2_PIX_FMT_SRGGB8;
  else if (strcmp(argv[1], "YUYV") == 0) m.cfmt = V4L2_PIX_FMT_YUYV;
  else if (strcmp(argv[1], "RGB565") == 0) m.cfmt = V4L2_PIX_FMT_RGB565;
  else LFATAL("Invalid format, should be BAYER, YUYV or RGB565");

  m.cw = std::atoi(argv[2]);
  m.ch = std::atoi(argv[3]);
  m.cfps = std::atof(argv[4]);

#ifdef USE_RAW_CAMERA
  // Simplest V4L2 capture code, taken from http://staticwave.ca/source/uvccapture/
  int fd; if ((fd = open ("/dev/video0", O_RDWR)) == -1) LFATAL("ERROR opening V4L interface");

  // See what kinds of inputs we have and select the first one that is a camera:
  int camidx = -1; struct v4l2_input inp = { };
  while (true)
  {
    try { XIOCTL(fd, VIDIOC_ENUMINPUT, &inp); } catch (...) { break; }
    if (inp.type == V4L2_INPUT_TYPE_CAMERA)
    {
      if (camidx == -1) camidx = inp.index;
      LINFO("Input " << inp.index << " [" << inp.name << "] is a camera sensor");
    } else LINFO("Input " << inp.index << " [" << inp.name << "] is a NOT camera sensor");
    ++inp.index;
  }
  if (camidx == -1) LFATAL("No valid camera input found");
  
  // Select the camera input, this seems to be required by VFE for the camera to power on:
  XIOCTL(fd, VIDIOC_S_INPUT, &camidx);

  // Check capabilities:
  struct v4l2_capability cap = { };
  XIOCTL(fd, VIDIOC_QUERYCAP, &cap);
  if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) LFATAL("Video capture not supported");
  if (!(cap.capabilities & V4L2_CAP_STREAMING)) LFATAL("Cameradoes not support streaming i/o");

  /* set format in */
  struct v4l2_format fmt = { };
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = m.cw;
  fmt.fmt.pix.height = m.ch;
  fmt.fmt.pix.pixelformat = m.cfmt;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  XIOCTL(fd, VIDIOC_S_FMT, &fmt);

  if ((fmt.fmt.pix.width != m.cw) || (fmt.fmt.pix.height != m.ch)) LFATAL("Format asked unavailable");

  /* request buffers */
  struct v4l2_requestbuffers rb = { };
  rb.count = NB_BUFFER;
  rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rb.memory = V4L2_MEMORY_MMAP;
  XIOCTL(fd, VIDIOC_REQBUFS, &rb);

  /* map the buffers */
  void * mem[NB_BUFFER];
  for (int i = 0; i < NB_BUFFER; i++) {
    struct v4l2_buffer buf = { };
    buf.index = i;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    XIOCTL(fd, VIDIOC_QUERYBUF, &buf);

    mem[i] = mmap(0 /* start anywhere */, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (mem[i] == MAP_FAILED) LFATAL("Unable to map buffer");
  }
  
  /* Queue the buffers. */
  for (int i = 0; i < NB_BUFFER; ++i) {
    struct v4l2_buffer buf = { };
    buf.index = i;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    XIOCTL(fd, VIDIOC_QBUF, &buf);
  }

  // start streaming
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  XIOCTL(fd, VIDIOC_STREAMON, &type);
  LINFO("Grab start...");
  for (int i = 0; i < 100; ++i)
  {
    struct v4l2_buffer buf = { };
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    XIOCTL(fd, VIDIOC_DQBUF, &buf);

    if (i >= 30)
    {
      // FIXME we only support YUYV here for now
      cv::Mat imgbgr;
      cv::Mat imgcv(m.ch, m.cw, CV_8UC2, mem[buf.index]);
      cv::cvtColor(imgcv, imgbgr, cv::COLOR_YUV2BGR_YUYV);
      cv::imwrite(std::string("camtest") + std::to_string(i-30) + ".png", imgbgr);
    }

    XIOCTL(fd, VIDIOC_QBUF, &buf);
  }
  LINFO("All done!");

  // should cleanup, unmap, close, etc
  
#else // USE_RAW_CAMERA
  std::shared_ptr<jevois::Camera> cam(new jevois::Camera("/dev/video0"));
  cam->setFormat(m);
  LINFO("Stream On");
  cam->streamOn();

  // First grab a few trash frames to let the auto exposure, gain, white balance, etc stabilize:
  LINFO("Trashing a few frames...");
  jevois::RawImage img;
  for (int i = 0; i < 30; ++i) { cam->get(img); cam->done(img); }

  // Now grab a few that we convert and save to disk:
  LINFO("Grab start...");
  for (int i = 0; i < 10; ++i)
  {
    cam->get(img);

    cv::Mat imgbgr;
    switch (m.cfmt)
    {
    case V4L2_PIX_FMT_SRGGB8:
    {
      cv::Mat imgcv(img.height, img.width, CV_8UC1, img.buf->data());
      cv::cvtColor(imgcv, imgbgr, cv::COLOR_BayerBG2BGR);
    }
    break;

    case V4L2_PIX_FMT_YUYV:
    {
      cv::Mat imgcv(img.height, img.width, CV_8UC2, img.buf->data());
      cv::cvtColor(imgcv, imgbgr, cv::COLOR_YUV2BGR_YUYV);
    }
    break;

    case V4L2_PIX_FMT_RGB565:
    {
      cv::Mat imgcv(img.height, img.width, CV_8UC2, img.buf->data());
      cv::cvtColor(imgcv, imgbgr, cv::COLOR_BGR5652BGR);
    }
    break;
    }

    cam->done(img);
    cv::imwrite(std::string("camtest") + std::to_string(i) + ".png", imgbgr);
  }
  
  LINFO("All done!");
  cam->streamOff();
  cam.reset();
  
#endif // USE_RAW_CAMERA
  return 0;
}
