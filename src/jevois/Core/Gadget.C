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

#include <jevois/Core/Gadget.H>
#include <jevois/Debug/Log.H>
#include <jevois/Core/VideoInput.H>
#include <jevois/Util/Utils.H>
#include <jevois/Core/VideoBuffers.H>
#include <jevois/Core/Engine.H>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h> // for gettimeofday()

namespace
{
  inline void debugCtrlReq(struct usb_ctrlrequest const & ctrl)
  {
    (void)ctrl; // avoid compiler warning about unused param if LDEBUG is turned off
    
    LDEBUG(std::showbase << std::hex << "bRequestType " << ctrl.bRequestType << " bRequest " << ctrl.bRequest
           << " wValue " << ctrl.wValue << " wIndex " << ctrl.wIndex << " wLength " << ctrl.wLength);
  }

  inline void debugStreamingCtrl(std::string const & msg, struct uvc_streaming_control const & ctrl)
  {
    (void)ctrl; // avoid compiler warning about unused param if LDEBUG is turned off
    (void)msg; // avoid compiler warning about unused param if LDEBUG is turned off
    LDEBUG(msg << ": " << std::showbase << std::hex << "bmHint=" << ctrl.bmHint << ", bFormatIndex=" <<
           ctrl.bFormatIndex << ", bFrameIndex=" << ctrl.bFrameIndex << ", dwFrameInterval=" << ctrl.dwFrameInterval <<
           ", wKeyFrameRate=" << ctrl.wKeyFrameRate << ", wPFrameRate=" << ctrl.wPFrameRate <<
           ", wCompQuality=" << ctrl.wCompQuality << ", wCompWindowSize=" << ctrl.wCompWindowSize <<
           ", wDelay=" << ctrl.wDelay << ", dwMaxVideoFrameSize=" << ctrl.dwMaxVideoFrameSize <<
           ", dwMaxPayloadTransferSize=" << ctrl.dwMaxPayloadTransferSize << ", dwClockFrequency=" <<
           ctrl.dwClockFrequency << ", bmFramingInfo=" << ctrl.bmFramingInfo << ", bPreferedVersion=" <<
           ctrl.bPreferedVersion << ", bMinVersion=" << ctrl.bMinVersion << ", bMaxVersion=" << ctrl.bMaxVersion);
  }
  
  unsigned int uvcToV4Lcontrol(unsigned int entity, unsigned int cs)
  {
    switch (entity)
    {
    case 1: // Our camera unit
      // --   #define UVC_CT_SCANNING_MODE_CONTROL                    0x01
      // OK   #define UVC_CT_AE_MODE_CONTROL                          0x02
      // OK   #define UVC_CT_AE_PRIORITY_CONTROL                      0x03
      // OK   #define UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL           0x04
      // --   #define UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL           0x05
      // --   #define UVC_CT_FOCUS_ABSOLUTE_CONTROL                   0x06
      // --   #define UVC_CT_FOCUS_RELATIVE_CONTROL                   0x07
      // --   #define UVC_CT_FOCUS_AUTO_CONTROL                       0x08
      // --   #define UVC_CT_IRIS_ABSOLUTE_CONTROL                    0x09
      // --   #define UVC_CT_IRIS_RELATIVE_CONTROL                    0x0a
      // --   #define UVC_CT_ZOOM_ABSOLUTE_CONTROL                    0x0b
      // --   #define UVC_CT_ZOOM_RELATIVE_CONTROL                    0x0c
      // --   #define UVC_CT_PANTILT_ABSOLUTE_CONTROL                 0x0d
      // --   #define UVC_CT_PANTILT_RELATIVE_CONTROL                 0x0e
      // --   #define UVC_CT_ROLL_ABSOLUTE_CONTROL                    0x0f
      // --   #define UVC_CT_ROLL_RELATIVE_CONTROL                    0x10
      // --   #define UVC_CT_PRIVACY_CONTROL                          0x11
      // note: UVC 1.5 has a few more...
      //
      // Windows 10 insists on doing a GET_DEF on this 10-byte control even though we never said we support it:
      // --   #define UVC_CT_REGION_OF_INTEREST_CONTROL               0x14
      // This is now handled by sending a default blank reply to all unsupported controls.
      switch (cs)
      {
      case UVC_CT_AE_MODE_CONTROL: return V4L2_CID_EXPOSURE_AUTO;
      case UVC_CT_AE_PRIORITY_CONTROL: return V4L2_CID_EXPOSURE_AUTO_PRIORITY;
      case UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL: return V4L2_CID_EXPOSURE_ABSOLUTE;
      }
      break;
      
    case 2: // Our processing unit
      // From uvcvideo.h and the UVC specs, here are the available processing unit controls:
      // A.9.5. Processing Unit Control Selectors

      // Note one trick here: UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL contains both the V4L2_CID_RED_BALANCE and
      // V4L2_CID_BLUE_BALANCE; we handle that in the ioctl processing section, here we just return V4L2_CID_RED_BALANCE
      
      // OK   #define UVC_PU_BACKLIGHT_COMPENSATION_CONTROL           0x01
      // OK   #define UVC_PU_BRIGHTNESS_CONTROL                       0x02
      // OK   #define UVC_PU_CONTRAST_CONTROL                         0x03
      // OK   #define UVC_PU_GAIN_CONTROL                             0x04
      // OK   #define UVC_PU_POWER_LINE_FREQUENCY_CONTROL             0x05
      // OK   #define UVC_PU_HUE_CONTROL                              0x06
      // OK   #define UVC_PU_SATURATION_CONTROL                       0x07
      // OK   #define UVC_PU_SHARPNESS_CONTROL                        0x08
      // --   #define UVC_PU_GAMMA_CONTROL                            0x09
      // --   #define UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL        0x0a
      // --   #define UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL   0x0b
      // OK   #define UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL          0x0c
      // OK   #define UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL     0x0d
      // TODO #define UVC_PU_DIGITAL_MULTIPLIER_CONTROL               0x0e
      // TODO #define UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL         0x0f
      // TODO #define UVC_PU_HUE_AUTO_CONTROL                         0x10
      // TODO #define UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL            0x11
      // TODO #define UVC_PU_ANALOG_LOCK_STATUS_CONTROL               0x12
      switch (cs)
      {
      case UVC_PU_BACKLIGHT_COMPENSATION_CONTROL: return V4L2_CID_BACKLIGHT_COMPENSATION;
      case UVC_PU_BRIGHTNESS_CONTROL: return V4L2_CID_BRIGHTNESS;
      case UVC_PU_CONTRAST_CONTROL: return V4L2_CID_CONTRAST;
      case UVC_PU_GAIN_CONTROL: return V4L2_CID_GAIN;
      case UVC_PU_POWER_LINE_FREQUENCY_CONTROL: return V4L2_CID_POWER_LINE_FREQUENCY;
      case UVC_PU_HUE_CONTROL: return V4L2_CID_HUE;
      case UVC_PU_SATURATION_CONTROL: return V4L2_CID_SATURATION;
      case UVC_PU_SHARPNESS_CONTROL: return V4L2_CID_SHARPNESS;
      case UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL: return V4L2_CID_RED_BALANCE;
      case UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: return V4L2_CID_AUTO_WHITE_BALANCE;
      //case UVC_PU_GAMMA_CONTROL: return V4L2_CID_GAMMA;
      }
      break;
    }

    LFATAL("Request to access unsupported control " << cs << " on entity " << entity);
  }
    
} // namespace

// ##############################################################################################################
jevois::Gadget::Gadget(std::string const & devname, jevois::VideoInput * camera, jevois::Engine * engine,
                       size_t const nbufs) :
    itsFd(-1), itsNbufs(nbufs), itsBuffers(nullptr), itsCamera(camera), itsEngine(engine), itsRunning(false),
    itsFormat(), itsFps(0.0F), itsStreaming(false), itsErrorCode(0), itsControl(0), itsEntity(0)
{
  JEVOIS_TRACE(1);
  
  if (itsCamera == nullptr) LFATAL("Gadget requires a valid camera to work");

  jevois::VideoMapping const & m = itsEngine->getDefaultVideoMapping();
  fillStreamingControl(&itsProbe, m);
  fillStreamingControl(&itsCommit, m);

  // Get our run() thread going and wait until it is cranking, it will flip itsRunning to true as it starts:
  itsRunFuture = std::async(std::launch::async, &jevois::Gadget::run, this);
  while (itsRunning.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // Open the device:
  itsFd = open(devname.c_str(), O_RDWR | O_NONBLOCK);
  if (itsFd == -1) PLFATAL("Gadget device open failed for " << devname);

  // Get ready to handle UVC events:
  struct v4l2_event_subscription sub = { };
    
  sub.type = UVC_EVENT_SETUP;
  XIOCTL(itsFd, VIDIOC_SUBSCRIBE_EVENT, &sub);
  
  sub.type = UVC_EVENT_DATA;
  XIOCTL(itsFd, VIDIOC_SUBSCRIBE_EVENT, &sub);
  
  sub.type = UVC_EVENT_STREAMON;
  XIOCTL(itsFd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    
  sub.type = UVC_EVENT_STREAMOFF;
  XIOCTL(itsFd, VIDIOC_SUBSCRIBE_EVENT, &sub);
    
  // Find out what the driver can do:
  struct v4l2_capability cap = { };
  XIOCTL(itsFd, VIDIOC_QUERYCAP, &cap);
  
  LINFO('[' << itsFd << "] UVC gadget " << devname << " card " << cap.card << " bus " << cap.bus_info);
  if ((cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) == 0) LFATAL(devname << " is not a video output device");
  if ((cap.capabilities & V4L2_CAP_STREAMING) == 0) LFATAL(devname << " does not support streaming");
}

// ##############################################################################################################
jevois::Gadget::~Gadget()
{
  JEVOIS_TRACE(1);
  
  streamOff();

  // Tell run() thread to finish up:
  itsRunning.store(false);

  // Will block until the run() thread completes:
  if (itsRunFuture.valid()) try { itsRunFuture.get(); } catch (...) { jevois::warnAndIgnoreException(); }

  if (close(itsFd) == -1) PLERROR("Error closing UVC gadget -- IGNORED");
}

// ##############################################################################################################
void jevois::Gadget::setFormat(jevois::VideoMapping const & m)
{
  JEVOIS_TRACE(2);

  JEVOIS_TIMED_LOCK(itsMtx);

  // Set the format:
  memset(&itsFormat, 0, sizeof(struct v4l2_format));
  
  itsFormat.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  itsFormat.fmt.pix.width = m.ow;
  itsFormat.fmt.pix.height = m.oh;
  itsFormat.fmt.pix.pixelformat = m.ofmt;
  itsFormat.fmt.pix.field = V4L2_FIELD_NONE;
  itsFormat.fmt.pix.sizeimage = m.osize();
  itsFps = m.ofps;
  
  // First try to set our own format, will throw if phony:
  XIOCTL(itsFd, VIDIOC_S_FMT, &itsFormat);

  // Note that the format does not include fps, this is done with VIDIOC_S_PARM:
  try
  {
    // The gadget driver may not support this ioctl...
    struct v4l2_streamparm sparm = { };
    sparm.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    sparm.parm.output.outputmode = 2; // V4L2_MODE_VIDEO not defined in our headers? its value is 2.
    sparm.parm.output.timeperframe = jevois::VideoMapping::fpsToV4l2(m.ofps);
    XIOCTL_QUIET(itsFd, VIDIOC_S_PARM, &sparm);
  } catch (...) { }
}

// ##############################################################################################################
void jevois::Gadget::run()
{
  JEVOIS_TRACE(1);
  
  fd_set wfds; // For UVC video streaming
  fd_set efds; // For UVC events
  struct timeval tv;
  
  // Switch to running state:
  itsRunning.store(true);

  // We may have to wait until the device is opened:
  while (itsFd == -1) std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Wait for event from the gadget kernel driver and process them:
  while (itsRunning.load())
  {
    // Wait until we either receive an event or we are ready to send the next buffer over:
    FD_ZERO(&wfds); FD_ZERO(&efds); FD_SET(itsFd, &wfds); FD_SET(itsFd, &efds);
    tv.tv_sec = 0; tv.tv_usec = 10000;
    
    int ret = select(itsFd + 1, nullptr, &wfds, &efds, &tv);
    
    if (ret == -1) { PLERROR("Select error"); if (errno == EINTR) continue; else break; }
    else if (ret > 0) // We have some events, handle them right away:
    {
      // Note: we may have more than one event, so here we try processEvents() several times to be sure:
      if (FD_ISSET(itsFd, &efds))
      {
        // First event, we will report error if any:
        try { processEvents(); } catch (...) { jevois::warnAndIgnoreException(); }

        // Let's try to dequeue one more, in most cases it should throw:
        while (true) try { processEvents(); } catch (...) { break; }
      }
        
      if (FD_ISSET(itsFd, &wfds)) try { processVideo(); } catch (...) { jevois::warnAndIgnoreException(); }
    }

    // We timed out

    // Sometimes we miss events in the main loop, likely because more events come while we are unlocked in the USB UDC
    // driver and processing here. So let's try to dequeue one more, in most cases it should throw:
    while (true) try { processEvents(); } catch (...) { break; }

    // While the driver is not busy in select(), queue at most one buffer that is ready to send off:
    try
    {
      JEVOIS_TIMED_LOCK(itsMtx);
      if (itsDoneImgs.size())
      {
        LDEBUG("Queuing image " << itsDoneImgs.front() << " for sending over USB");
        
        // We need to prepare a legit v4l2_buffer, including bytesused:
        struct v4l2_buffer buf = { };
        
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = itsDoneImgs.front();
        buf.length = itsBuffers->get(buf.index)->length();

        if (itsFormat.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
          buf.bytesused = itsBuffers->get(buf.index)->bytesUsed();
        else
          buf.bytesused = buf.length;

        buf.field = V4L2_FIELD_NONE;
        buf.flags = 0;
        gettimeofday(&buf.timestamp, nullptr);
        
        // Queue it up so it can be sent to the host:
        itsBuffers->qbuf(buf);
        
        // This one is done:
        itsDoneImgs.pop_front();
      }
    } catch (...) { jevois::warnAndIgnoreException(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
  }

  // Switch out of running state in case we did interrupt the loop here by a break statement:
  itsRunning.store(false);
}

// ##############################################################################################################
void jevois::Gadget::processEvents()
{
  JEVOIS_TRACE(3);
  
  // Get the event from the driver:
  struct v4l2_event v4l2ev = { };
  XIOCTL_QUIET(itsFd, VIDIOC_DQEVENT, &v4l2ev);
  struct uvc_event * uvcev = reinterpret_cast<struct uvc_event *>(&v4l2ev.u.data);
  
  // Prepare our response, if any will be sent:
  struct uvc_request_data resp = { };
  resp.length = -EL2HLT;

  // Act according to the event type:
  try
  {
    switch (v4l2ev.type)
    {
    case UVC_EVENT_CONNECT: return;
    case UVC_EVENT_DISCONNECT: LDEBUG("EVENT DISCONNECT"); itsEngine->streamOff(); return;
    case UVC_EVENT_SETUP: LDEBUG("EVENT SETUP"); processEventSetup(uvcev->req, resp); return;
    case UVC_EVENT_DATA: LDEBUG("EVENT DATA"); processEventData(uvcev->data); return;
    case UVC_EVENT_STREAMON: LDEBUG("EVENT STREAMON"); itsEngine->streamOn(); return;
    case UVC_EVENT_STREAMOFF: LDEBUG("EVENT STREAMOFF"); itsEngine->streamOff(); return;
    }
  } catch (...) { }
}

// ##############################################################################################################
void jevois::Gadget::processVideo()
{
  JEVOIS_TRACE(3);
  
  jevois::RawImage img;
  JEVOIS_TIMED_LOCK(itsMtx);

  // If we are not streaming anymore, abort:
  if (itsStreaming.load() == false) LFATAL("Aborted while not streaming");
  
  // Dequeue a buffer from the gadget driver, this is an image that has been sent to the host and hence the buffer is
  // now available to be filled up with image data and later queued again to the gadget driver:
  struct v4l2_buffer buf;
  itsBuffers->dqbuf(buf);

  // Create a RawImage from that buffer:
  img.width = itsFormat.fmt.pix.width;
  img.height = itsFormat.fmt.pix.height;
  img.fmt = itsFormat.fmt.pix.pixelformat;
  img.fps = itsFps;
  img.buf = itsBuffers->get(buf.index);
  img.bufindex = buf.index;

  // Push the RawImage to outside consumers:
  itsImageQueue.push_back(img);
  LDEBUG("Empty image " << img.bufindex << " ready for filling in by application code");
}

// ##############################################################################################################
void jevois::Gadget::processEventSetup(struct usb_ctrlrequest const & ctrl, struct uvc_request_data & resp)
{
  JEVOIS_TRACE(3);
  
  itsControl = 0; itsEntity = 0;
  
  debugCtrlReq(ctrl);

  switch (ctrl.bRequestType & USB_TYPE_MASK)
  {
  case USB_TYPE_STANDARD:  processEventStandard(ctrl, resp); break;
  case USB_TYPE_CLASS:     processEventClass(ctrl, resp); break;
  default: LERROR("Unsupported setup event type " << std::showbase << std::hex <<
                  (ctrl.bRequestType & USB_TYPE_MASK) << " -- IGNORED");
  }
  if (ctrl.bRequestType != 0x21) XIOCTL(itsFd, UVCIOC_SEND_RESPONSE, &resp);
}

// ##############################################################################################################
void jevois::Gadget::processEventStandard(struct usb_ctrlrequest const & ctrl,
                                          struct uvc_request_data & JEVOIS_UNUSED_PARAM(resp))
{
  JEVOIS_TRACE(3);
  
  LDEBUG("UVC standard setup event ignored:");
  debugCtrlReq(ctrl);
}

// ##############################################################################################################
void jevois::Gadget::processEventClass(struct usb_ctrlrequest const & ctrl, struct uvc_request_data & resp)
{
  JEVOIS_TRACE(3);
  
  if ((ctrl.bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE) return;

  switch (ctrl.wIndex & 0xff)
  {
  case UVC_INTF_CONTROL:
    processEventControl(ctrl.bRequest, ctrl.wValue >> 8, ctrl.wIndex >> 8, ctrl.wLength, resp);
    break;

  case UVC_INTF_STREAMING:
    processEventStreaming(ctrl.bRequest, ctrl.wValue >> 8, resp);
    break;

  default:
    LERROR("Unsupported setup event class " << std::showbase << std::hex << (ctrl.wIndex & 0xff) << " -- IGNORED");
  }
}

// ##############################################################################################################
void jevois::Gadget::processEventControl(uint8_t req, uint8_t cs, uint8_t entity_id, uint8_t len,
                                         struct uvc_request_data & resp)
{
  JEVOIS_TRACE(3);
  
  // Local function we run on successful processing of an event: we just reset our internal error code
#define success() { itsErrorCode = 0; }

  // Local function we run on failed processing of an event: stall the request, set our internal error code
#define failure(code) { resp.length = -EL2HLT; itsErrorCode = code; }

  // Shortcurt to successfully send a 1-byte response:
#define byteresponse(val) { resp.data[0] = val; resp.length = 1; itsErrorCode = 0; }

  // Shortcurt to successfully send a 2-byte response:
#define wordresponse(val) { resp.data[0] = val & 0xff; resp.data[1] = (val >> 8) & 0xff; \
    resp.length = 2; itsErrorCode = 0; }

  // Shortcurt to successfully send a 4-byte response:
#define intresponse(val) { resp.data[0] = val & 0xff; resp.data[1] = (val >> 8) & 0xff; \
    resp.data[2] = (val >> 16) & 0xff; resp.data[3] = (val >> 24) & 0xff; \
    resp.length = 4; itsErrorCode = 0; }

  // Shortcurt to successfully send a blank N-byte response:
#define arrayblankresponse(len) { memset(resp.data, 0, len); resp.length = len; itsErrorCode = 0; }

  // If anything throws here, we will return failure:
  try
  {
    // First handle any request that is directed to entity 0:
    if (entity_id == 0)
    {
      switch (cs)
      {
      case UVC_VC_REQUEST_ERROR_CODE_CONTROL: byteresponse(itsErrorCode); return; // Send error code last prepared
      default: failure(0x06); return;
      }
    }
    
    // Process according to the entity that this event is directed to and the control that is requested:
    if (req == UVC_SET_CUR)
    {
      // We need to wait for the data phase, so for now just remember the control and return success:
      itsEntity = entity_id; itsControl = cs;
      resp.data[0] = 0x0; resp.length = len; success();
      LDEBUG("SET_CUR ent " << itsEntity <<" ctrl "<< itsControl <<" len "<< len);
    }
    else if (req == UVC_GET_INFO)
    {
      // FIXME: controls could also be disabled, autoupdate, or asynchronous:
      byteresponse(UVC_CONTROL_CAP_GET | UVC_CONTROL_CAP_SET);
    }
    else if (req == UVC_GET_CUR)
    {
      // Fetch the current value from the camera. Note: both Windows and Android insist on querying some controls, like
      // IRIS and GAMMA, which we did not declare as supported in the kernel driver. We need to handle those requests
      // and to send some phony data:
      struct v4l2_control ctrl = { };
      try { ctrl.id = uvcToV4Lcontrol(entity_id, cs); itsCamera->getControl(ctrl); } catch (...) { ctrl.id = 0; }

      // We need a special handling of white balance here:
      if (ctrl.id == V4L2_CID_RED_BALANCE)
      {
        unsigned int redval = (ctrl.value & 0xffff) << 16; // red is at offset 2 in PU_WHITE_BALANCE_COMPONENT_CONTROL
        
        // Also get the blue balance value:
        ctrl.id = V4L2_CID_BLUE_BALANCE;
        itsCamera->getControl(ctrl);
        
        // Combine both red and blue values:
        ctrl.value = (ctrl.value & 0xffff) | redval;
      }
      // We also need to remap auto exposure values:
      else if (ctrl.id == V4L2_CID_EXPOSURE_AUTO)
      {
        if (ctrl.value == V4L2_EXPOSURE_MANUAL) ctrl.value = 0x01; // manual mode, set UVC bit D0
        else if (ctrl.value == V4L2_EXPOSURE_AUTO) ctrl.value = 0x02; // auto mode, set UVC bit D1
        else ctrl.value = 0x03;
        // Note, there are 2 more bits under CT_AE_MODE_CONTROL
      }
      // Handle the unknown controls:
      else if (ctrl.id == 0) ctrl.value = 0;
      
      switch (len)
      {
      case 1: byteresponse(ctrl.value); break;
      case 2: wordresponse(ctrl.value); break;
      case 4: intresponse(ctrl.value); break;
      default: LERROR("Unsupported control with length " << len << " -- SENDING BLANK RESPONSE");
	arrayblankresponse(len); break;
      }
    }
    else
    {
      // It's a GET_DEF/RES/MIN/MAX let's first get the data from the camera: Note: both Windows and Android insist on
      // querying some controls, like IRIS and GAMMA, which we did not declare as supported in the kernel driver. We
      // need to handle those requests and to send some phony data:
      struct v4l2_queryctrl qc = { };
      try { qc.id = uvcToV4Lcontrol(entity_id, cs); itsCamera->queryControl(qc); } catch (...) { qc.id = 0; }      

      // We need a special handling of white balance here:
      if (qc.id == V4L2_CID_RED_BALANCE)
      {
        // Also get the blue balance values:
        struct v4l2_queryctrl qc2 = { };
        qc2.id = V4L2_CID_BLUE_BALANCE;
        itsCamera->queryControl(qc2);
        
        // Combine red and blue values into qc:
        qc.default_value = (qc.default_value << 16) | qc2.default_value;
        qc.step = (qc.step << 16) | qc2.step;
        qc.minimum = (qc.minimum << 16) | qc2.minimum;
        qc.maximum = (qc.maximum << 16) | qc2.maximum;
      }
      // We also need to remap auto exposure values:
      else if (qc.id == V4L2_CID_EXPOSURE_AUTO)
      {
        // Tricky: in the 'step' field, we are supposed to provide a bitmap of the modes that are supported, see UVC
        // specs. D0=manual, D1=auto, D2=shutter priority, D3=aperture priority. Min and max are ignored for this
        // control, default is handled.
        qc.minimum = 0; qc.step = 3; qc.maximum = 3; qc.default_value = 1;
      }
      // Also handle the unknown controls here:
      else if (qc.id == 0)
      { qc.minimum = 0; qc.step = 1; qc.maximum = 1; qc.default_value = 0; }
      
      int val = 0;
      switch (req)
      {
      case UVC_GET_DEF: val = qc.default_value; break;
      case UVC_GET_RES: val = qc.step; break;
      case UVC_GET_MIN: val = qc.minimum; break;
      case UVC_GET_MAX: val = qc.maximum; break;
      default: failure(0x07); return;
      }
      
      switch (len)
      {
      case 1: byteresponse(val); break;
      case 2: wordresponse(val); break;
      case 4: intresponse(val); break;
      default: LERROR("Unsupported control with length " << len << " -- SENDING BLANK RESPONSE");
	arrayblankresponse(len); break;
      }
    }
  }
  catch (...)
  {
    LERROR("FAILED entity " << entity_id << " cs " << cs << " len " << len);
    failure(0x06);
  }
}

// ##############################################################################################################
void jevois::Gadget::fillStreamingControl(struct uvc_streaming_control * ctrl, jevois::VideoMapping const & m)
{
  JEVOIS_TRACE(3);
  
  memset(ctrl, 0, sizeof(struct uvc_streaming_control));
  
  ctrl->bFormatIndex = m.uvcformat;
  ctrl->bFrameIndex = m.uvcframe;
  ctrl->dwFrameInterval = jevois::VideoMapping::fpsToUvc(m.ofps);
  ctrl->dwMaxVideoFrameSize = m.osize();
  ctrl->dwMaxPayloadTransferSize = 3072;
  ctrl->bmFramingInfo = 3;
  ctrl->bPreferedVersion = 1;
  ctrl->bMaxVersion = 1;
}

// ##############################################################################################################
void jevois::Gadget::processEventStreaming(uint8_t req, uint8_t cs, struct uvc_request_data & resp)
{
  JEVOIS_TRACE(3);
  
  int const datalen = 26; // uvc 1.0 as reported by our kernel driver
  if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL) return;
  
  struct uvc_streaming_control * ctrl = reinterpret_cast<struct uvc_streaming_control *>(&resp.data);
  struct uvc_streaming_control * target = (cs == UVC_VS_PROBE_CONTROL) ? &itsProbe : &itsCommit;
  resp.length = datalen;

  switch (req)
  {
  case UVC_SET_CUR: itsControl = cs; resp.length = datalen; break; // will finish up in data stage
    
  case UVC_GET_CUR:
  case UVC_GET_MIN: // we have nothing to negotiate
  case UVC_GET_MAX: // we have nothing to negotiate
    memcpy(ctrl, target, datalen);
    break;
    
  case UVC_GET_DEF:
  {
    // If requested format index, frame index, or interval is bogus (including zero), initialize to our default mapping,
    // otherwise pass down the selected mapping:
    size_t idx = itsEngine->getDefaultVideoMappingIdx();
    try { idx = itsEngine->getVideoMappingIdx(ctrl->bFormatIndex, ctrl->bFrameIndex, ctrl->dwFrameInterval); }
    catch (...) { }
    fillStreamingControl(target, itsEngine->getVideoMapping(idx));
    memcpy(ctrl, target, datalen);
  }
  break;

  case UVC_GET_RES: memset(ctrl, 0, datalen); break;

  case UVC_GET_LEN: resp.data[0] = 0x00; resp.data[1] = datalen; resp.length = 2; break;

  case UVC_GET_INFO: resp.data[0] = 0x03; resp.length = 1; break;
  }
}

// ##############################################################################################################
void jevois::Gadget::processEventData(struct uvc_request_data & data)
{
  JEVOIS_TRACE(3);
  
  struct uvc_streaming_control * target;

  // If entity is 1 or 2, this is to set a control:
  if (itsEntity == 2 || itsEntity == 1) { processEventControlData(data); return; }
  
  switch (itsControl)
  {
  case UVC_VS_PROBE_CONTROL:  target = &itsProbe; break;
  case UVC_VS_COMMIT_CONTROL: target = &itsCommit; break;
  default:                    processEventControlData(data); return;
  }

  // Find the selected format and frame info and fill-in the control data:
  struct uvc_streaming_control * ctrl = reinterpret_cast<struct uvc_streaming_control *>(&data.data);

  size_t idx = itsEngine->getVideoMappingIdx(ctrl->bFormatIndex, ctrl->bFrameIndex, ctrl->dwFrameInterval);

  fillStreamingControl(target, itsEngine->getVideoMapping(idx));

  LDEBUG("Host requested " << ctrl->bFormatIndex << '/' << ctrl->bFrameIndex << '/' << ctrl->dwFrameInterval <<
         ", " << ((itsControl == UVC_VS_COMMIT_CONTROL) ? "setting " : "returning ") <<
         itsEngine->getVideoMapping(idx).str());
  
  // Set the format if we are doing a commit control:
  if (itsControl == UVC_VS_COMMIT_CONTROL) itsEngine->setFormat(idx);
}

// ##############################################################################################################
void jevois::Gadget::processEventControlData(struct uvc_request_data & data)
{
  JEVOIS_TRACE(3);
  
  struct v4l2_control ctrl;

  // Get the control ID for V4L or throw if unsupported:
  ctrl.id = uvcToV4Lcontrol(itsEntity, itsControl);
    
  // Copy the data we received into the control's value:
  switch (data.length)
  {
  case 1: ctrl.value = static_cast<int>(data.data[0]); break;
  case 2: ctrl.value = static_cast<int>(__s16(data.data[0] | (static_cast<short>(data.data[1]) << 8))); break;
  case 4: ctrl.value = data.data[0] | (data.data[1] << 8) | (data.data[2] << 16) | (data.data[3] << 24); break;
  default: LFATAL("Ooops data len is " << data.length);
  }

  // Tell the camera to set the control. We do not have enough time here to do it as otherwise our USB transaction will
  // time out while we transfer a bunch of bytes to the camera over the 400kHz serial control link, so we just push it
  // to a queue and our run() thread will do the work. First, handle special cases:
  switch (ctrl.id)
  {
  case V4L2_CID_RED_BALANCE:
  {
    // We need to set both the red and the blue:
    int blue = ctrl.value & 0xffff;
    ctrl.value >>= 16; itsCamera->setControl(ctrl);
    ctrl.id = V4L2_CID_BLUE_BALANCE; ctrl.value = blue; itsCamera->setControl(ctrl);
  }
  break;

  case V4L2_CID_EXPOSURE_AUTO:
    if (ctrl.value & 0x01) ctrl.value = V4L2_EXPOSURE_MANUAL; // UVC bit D0 set for manual mode
    else if (ctrl.value & 0x02) ctrl.value = V4L2_EXPOSURE_AUTO; // auto mode
    // Note, there are 2 more bits under CT_AE_MODE_CONTROL
    itsCamera->setControl(ctrl);
    break;

  default: itsCamera->setControl(ctrl);
  }
}

// ##############################################################################################################
void jevois::Gadget::streamOn()
{
  JEVOIS_TRACE(2);
  
  LDEBUG("Turning on UVC stream");
  
  JEVOIS_TIMED_LOCK(itsMtx);

  if (itsStreaming.load() || itsBuffers) { LERROR("Stream is already on -- IGNORED"); return; }

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
  if (nbuf < 3) nbuf = 3; else if (nbuf > 16) nbuf = 16;

  // Allocate our buffers for the currently selected resolution, format, etc:
  itsBuffers = new jevois::VideoBuffers("gadget", itsFd, V4L2_BUF_TYPE_VIDEO_OUTPUT, nbuf);
  LINFO(itsBuffers->size() << " buffers of " << itsBuffers->get(0)->length() << " bytes allocated");
  
  // Fill itsImageQueue with blank frames that can be given off to application code:
  for (size_t i = 0; i < nbuf; ++i)
  {
    jevois::RawImage img;
    img.width = itsFormat.fmt.pix.width;
    img.height = itsFormat.fmt.pix.height;
    img.fmt = itsFormat.fmt.pix.pixelformat;
    img.fps = itsFps;
    img.buf = itsBuffers->get(i);
    img.bufindex = i;

    // Push the RawImage to outside consumers:
    itsImageQueue.push_back(img);
    LDEBUG("Empty image " << img.bufindex << " ready for filling in by application code");
  }

  // Start streaming over the USB link:
  int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  XIOCTL(itsFd, VIDIOC_STREAMON, &type);
  LDEBUG("Device stream on");

  itsStreaming.store(true);
  LDEBUG("Stream is on");
}

// ##############################################################################################################
void jevois::Gadget::abortStream()
{
  JEVOIS_TRACE(2);
  
  itsStreaming.store(false);
}

// ##############################################################################################################
void jevois::Gadget::streamOff()
{
  JEVOIS_TRACE(2);
  
  // Note: we allow for several streamOff() without complaining, this happens, e.g., when destroying a Gadget that is
  // not currently streaming.

  LDEBUG("Turning off gadget stream");

  // Abort stream in case it was not already done, which will introduce some sleeping in our run() thread, thereby
  // helping us acquire our needed double lock:
  abortStream();

  JEVOIS_TIMED_LOCK(itsMtx);

  // Stop streaming over the USB link:
  int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
  try { XIOCTL_QUIET(itsFd, VIDIOC_STREAMOFF, &type); } catch (...) { }
  
  // Nuke all our buffers:
  if (itsBuffers) { delete itsBuffers; itsBuffers = nullptr; }
  itsImageQueue.clear();
  itsDoneImgs.clear();

  LDEBUG("Gadget stream is off");
}

// ##############################################################################################################
void jevois::Gadget::get(jevois::RawImage & img)
{
  JEVOIS_TRACE(4);
  int retry = 2000;
  
  while (--retry >= 0)
  {
    if (itsStreaming.load() == false)
    { LDEBUG("Not streaming"); throw std::runtime_error("Gadget get() rejected while not streaming"); }

    if (itsMtx.try_lock_for(std::chrono::milliseconds(100)))
    {
      if (itsStreaming.load() == false)
      {
        LDEBUG("Not streaming");
        itsMtx.unlock();
        throw std::runtime_error("Gadget get() rejected while not streaming");
      }

      if (itsImageQueue.size())
      {
        img = itsImageQueue.front();
        itsImageQueue.pop_front();
        itsMtx.unlock();
        LDEBUG("Empty image " << img.bufindex << " handed over to application code for filling");
        return;
      }

      // No image in the queue, unlock and wait for one:
      itsMtx.unlock();
      LDEBUG("Waiting for blank UVC image...");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    else
    {
      LDEBUG("Waiting for lock");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
  LFATAL("Giving up waiting for blank UVC image");
}

// ##############################################################################################################
void jevois::Gadget::send(jevois::RawImage const & img)
{
  JEVOIS_TRACE(4);
  int retry = 2000;

  while (--retry >= 0)
  {
    if (itsStreaming.load() == false)
    { LDEBUG("Not streaming"); throw std::runtime_error("Gadget send() rejected while not streaming"); }

    if (itsMtx.try_lock_for(std::chrono::milliseconds(100)))
    {
      if (itsStreaming.load() == false)
      {
        LDEBUG("Not streaming");
        itsMtx.unlock();
        throw std::runtime_error("Gadget send() rejected while not streaming");
      }
      
      // Check that the format matches, this may not be the case if we changed format while the buffer was out for
      // processing. IF so, we just drop this image since it cannot be sent to the host anymore:
      if (img.width != itsFormat.fmt.pix.width ||
          img.height != itsFormat.fmt.pix.height ||
          img.fmt != itsFormat.fmt.pix.pixelformat)
      {
        LDEBUG("Dropping image to send out as format just changed");
        itsMtx.unlock();
        return;
      }
      
      // We cannot just qbuf() here as our run() thread is likely in select() and the driver will bomb the qbuf as
      // resource unavailable. So we just enqueue the buffer index and the run() thread will handle the qbuf later:
      itsDoneImgs.push_back(img.bufindex);
      itsMtx.unlock();
      LDEBUG("Filled image " << img.bufindex << " received from application code");
      return;
    }
    else
    {
      LDEBUG("Waiting for lock");
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
  LFATAL("Giving up waiting for lock");
}
 
