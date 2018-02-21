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

#include <mutex>
#include <future>
#include <deque>
#include <atomic>
#include <linux/usb/video.h> // for uvc_streaming_control
#include <linux/videodev2.h>
#include <jevois/Core/VideoOutput.H>
#include <jevois/Core/VideoMapping.H>
#include <jevois/Image/RawImage.H>

// for UVC gadget specific definitions; yes, this is only in the kernel tree, kernel maintainers should expose those
// definitions in the standard headers instead:
//#include "../../../../lichee/linux-3.4/drivers/usb/gadget/uvc.h"
// We now include this file in our tree in so our code can compile without requiring the kernel source tree:
#include "uvc.h"

namespace jevois
{
  class VideoInput;
  class Engine;
  class VideoBuffers;
  
  //! JeVois gadget driver - exposes a uvcvideo interface to host computer connected over USB
  /*! Gadget is a user-space interface to the Linux kernel's gadget driver implemented by JeVois. A USB gadget driver is
      a USB driver that runs on a device, as opposed to a USB driver that runs on a host computer. Gadget drivers
      implement USB functions such as USB printers, USB modems, or USB cameras. In JeVois, the Gadget driver makes the
      JeVois hardware platform appear as a UVC (USB Video Class) camera. Because UVC has been standardized, no special
      driver is needed on the host computer to start using the UVC device, as virtually all modern operating systems
      already provide a built-in driver for UVC-compliant cameras.

      The internal operation of Gadget is quite complex, especially due to its tight interaction with the kernel-side
      UVC gadget driver. Yet, for users, Gadget is just another VideoOutput device: One can set its format, obtain blank
      image buffers from it using get(), paint results into those buffers, and send them to the host computer when
      complete using send().

      Gadget implements a zero-copy, zero-wait access to output video frames, that is:
      - the pixel data of the image you obtain via get() is directly the memory-mapped pixel buffer that the silicon
        hardware on the JeVois chip will use via direct-memory-access (DMA) to stream the data out over the USB link;
      - as soon as you call send() that buffer will be queued for sending over USB (as opposed to having a fixed,
        regular interval at which images may be streamed out). Gadget has several image buffers, allowing one to be
        streamed out over USB while another is being handed over for filling by your application via get(). These
        buffers are recycled, i.e., once send() is called, the underlying buffer is streamed over USB and then sent back
        to the Gadget for future access by your code.

      Most programmers will never use Gadget directly, instead using Engine and OutputFrame. \ingroup core */
  class Gadget : public VideoOutput
  {
    public:
      //! Construct and open the device
      /*! A vaid non-null camera is required for this gadget to work. To avoid testing for a non-null camera on each
          operation of the gadget, we only test once at construction and then assume the camera will remain operational
          for the lifetime of the gadget. Use 0 for nbufs to set it automatically. */
      Gadget(std::string const & devname, VideoInput * camera, Engine * engine, size_t const nbufs = 0,
	     bool multicam = false);
      
      //! Close the device and free all resources
      virtual ~Gadget();

      //! Set the video format and frame rate
      void setFormat(jevois::VideoMapping const & m) override;

      //! Get a pre-allocated image so that we can fill the pixel data and later send out over USB using send()
      /*! May throw if not buffer is available, i.e., all have been queued to send to the host but have not yet been
          sent. Application code must balance exactly one send() for each get(). */
      void get(RawImage & img) override;
      
      //! Send an image out over USB to the host computer
      /*! May throw if the format is incorrect or std::overflow_error if we have not yet consumed the previous image. */
      void send(RawImage const & img) override;

      //! Start streaming
      void streamOn() override;

      //! Abort streaming
      /*! This only cancels future get() and done() calls, one should still call streamOff() to turn off streaming. */
      void abortStream() override;

      //! Stop streaming
      void streamOff() override;

    private:
      volatile int itsFd;
      bool const itsMulticam;
      size_t itsNbufs;
      VideoBuffers * itsBuffers;
      VideoInput * itsCamera;
      Engine * itsEngine;
      
      void run(); // Function to service requests, runs in a separate thread
      std::future<void> itsRunFuture;
      std::atomic<bool> itsRunning;

      void processEvents();
      void processEventSetup(struct usb_ctrlrequest const & ctrl, struct uvc_request_data & resp);
      void processEventStandard(struct usb_ctrlrequest const & ctrl, struct uvc_request_data & resp);
      void processEventClass(struct usb_ctrlrequest const & ctrl, struct uvc_request_data & resp);
      void processEventData(struct uvc_request_data & data);
      void processEventControlData(struct uvc_request_data & data);
      void processEventControl(uint8_t req, uint8_t cs, uint8_t entity_id, uint8_t len, struct uvc_request_data & resp);
      void processEventStreaming(uint8_t req, uint8_t cs, struct uvc_request_data & resp);
      void processVideo();

      struct v4l2_format itsFormat;
      float itsFps;
      void fillStreamingControl(struct uvc_streaming_control * ctrl, VideoMapping const & m);
      std::atomic<bool> itsStreaming;
      int itsErrorCode;
      int itsControl;
      int itsEntity;
      struct uvc_streaming_control itsProbe;
      struct uvc_streaming_control itsCommit;

      std::deque<RawImage> itsImageQueue;
      std::deque<size_t> itsDoneImgs;

      mutable std::timed_mutex itsMtx;
  };

} // namespace jevois
  
