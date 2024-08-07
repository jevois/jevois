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

#include <jevois/Core/VideoInput.H>
#include <jevois/Core/VideoBuffers.H>
#include <jevois/Core/CameraDevice.H>

#include <mutex>

namespace jevois
{
  //! JeVois camera driver class - grabs frames from a Video4Linux camera sensor
  /*! On the platform hardware, the Camera class provides access to the camera sensor on the parallel camera bus
      (CSI). On other hardware, it can provide access to any Video4Linux2 camera (e.g., USB webcam), although with some
      limitations, such as:
      - cannot read/write raw camera registers.
      - usually cannot select any frames/s but instead limited to a few fixed values like 15fps, 30fps.
      - usually limited to 30fps max (no 60fps or 120fps modes like on JeVois platform hardware)
      - usually limited number of controls (exposure, gain, etc) available.

      The camera runs a thread that handles capturing frames automatically. Users can access the next grabbed frame
      using get(), which may block until the frame is fully captured. Once they are finished with a frame obtained
      through get(), users should hand it back to the Camera by calling done(). When using the JeVois Engine and a
      Module, get() and done() are automatically called in an exception-safe manner through the use of the InputFrame
      class. Indeed, video grabbing uses a fixed set of image buffers that are allocated by the Linux kernel when a
      video format is configured, and the camera hardware then cycles through the buffers.  On the platform, the camera
      image buffers are allocated in the ION carveout memory area, which is a reserved memory zone configured in the
      Linux kernel, and the USB output buffers are also allocated in that zone. To avoid reserving too much memory that
      will not be used, the ION carveout reservation is quite small by default, but is sufficient for 3 1280x1024 YUYV
      buffers.

      Camera implements a zero-copy, zero-wait access to input video frames, that is:
      - the pixel data of the image you obtain via get() is directly the memory-mapped pixel buffer that the silicon
        hardware on the JeVois chip uses via direct-memory-access (DMA) to stream the pixel data from the camera chip to
        processor memory;
      - as soon as an image is captured by the camera hardware, get() unblocks and returns it (as opposed to having a
        fixed, regular interval at which images may be available). Camera has several image buffers, allowing one to be
        captured while another is being handed over for processing via get(). These buffers are recycled, i.e., once
        done() is called, the underlying buffer is sent back to the camera hardware for future capture.

      Camera will drop frames if they are not processed by user code fast enough. That is, if all buffers have been
      grabbed already and not yet handed over through get() and then returned by done(), then the next camera sensor
      frame will be dropped.

      Most programmers will never use Camera directly, instead using Engine and InputFrame. \ingroup core */
  class Camera : public VideoInput
  {
    public:
      //! Construct and open the device
      /*! \param devname device name, e.g., /dev/video0
          \param s currently installed camera sensor
          \param nbufs number of video grab buffers, or 0 for automatic. */
      Camera(std::string const & devname, jevois::CameraSensor s = jevois::CameraSensor::any,
             unsigned int const nbufs = 0);

      //! Close the device and free all resources
      ~Camera();

      //! Start streaming
      void streamOn() override;

      //! Abort streaming
      /*! This only cancels future get() and done() calls, one should still call streamOff() to turn off streaming. */
      void abortStream() override;
      
      //! Stop streaming
      void streamOff() override;

      //! Get the next captured buffer
      /*! Throws if we are not streaming or blocks until an image is available (has been captured). img should have been
          created by the caller (with no pixel buffer allocated) and will be filled in by what we receive from the
          device here. */
      void get(RawImage & img) override;

      //! Check whether a second input image scaled by the JeVoisPro Platform ISP is available
      /*! Returns false unless we are on JeVois-Pro Platform and the camera format modifier jevois::CropType::CropScale
          is currently in use. */
      bool hasScaledImage() const override;

      //! Get the next captured buffer, for second ISP-scaled image
      /*! On JeVois-Pro Platform only, the camera ISP can output 2 frames: 1) raw from sensor, 2) scaled by ISP. This
          function is to access the ISP scaled frame. Throws if not JeVois-Pro Platform or the camera stream type is not
          jevois::StreamType::RawAndScaled. Throws if we are not streaming or blocks until an image is available (has
          been captured). img should have been created by the caller (with no pixel buffer allocated) and will be filled
          in by what we receive from the device here. */
      void get2(RawImage & img) override;

      //! Indicate that user processing is done with an image previously obtained via get()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled.

          Calling done() on a RawImage invalidates the image and in particular its pixel buffer. Users should make sure
          that no attempt to use the image or the pixel buffer will be made after done() is called. */
      void done(RawImage & img) override;

      //! Indicate that user processing is done with an image previously obtained via get2()
      /*! You should call this as soon after get2() as possible, once you are finished with the RawImage data so that it
          can be recycled.

          Calling done2() on a RawImage invalidates the image and in particular its pixel buffer. Users should make sure
          that no attempt to use the image or the pixel buffer will be made after done2() is called. */
      void done2(RawImage & img) override;

      //! Get information about a control, throw if unsupported by hardware
      /*! Caller should zero-out qc and then set the id field to the desired control id. See VIDIOC_QUERYCTRL for more
          information. */
      void queryControl(struct v4l2_queryctrl & qc) const override;

      //! Get the available menu entry names for a menu-type control, throw if unsupported by hardware
      /*! Caller should zero-out qm and then set the id and index fields to the desired control id and menu item
          index. See VIDIOC_QUERYMENU for more information. */
      void queryMenu(struct v4l2_querymenu & qm) const override;
     
      //! Get a control's current value, throw if unsupported by hardware
      /*! This is just a pass-through to VIDIOC_G_CTRL, users should zero-out ctrl and then set in ctrl.id the desired
          control ID. */
      void getControl(struct v4l2_control & ctrl) const;
      
      //! Set a control, throw if control not supported or the hardware rejects the value
      /*! This is just a pass-through to VIDIOC_S_CTRL */
      void setControl(struct v4l2_control const & ctrl) override;

      //! Set the video format and frame rate
      void setFormat(VideoMapping const & m) override;

      //! Write a value to one of the camera's registers
      /*! This very low-level access is for development of optimal camera settings only and should not be used in normal
          operation, it can crash your system. */
      void writeRegister(unsigned short reg, unsigned short val);

      //! Read a value from one of the camera's registers
      /*! This very low-level access is for development of optimal camera settings only and should not be used in normal
          operation, it can crash your system. */
      unsigned short readRegister(unsigned short reg);

      //! Lock the camera and return its file descriptor
      /*! Used by IMUi2c to access the IMU registers over the I2C bus shared with the camera sensor. Use with caution
          and make sure you catch exceptions so you can guarantee that you will call unlock(), otherwise your camera
          will be stuck and enable to stream. */
      int lock();

      //! Unlock the camera that was previously locked by lock()
      void unlock();
      
    protected:
      //! Sensor flags
      /*! Keep this in sync with jevois-sdk/linux-3.4/drivers/media/video/sunxi-vfe/device/camera.h */
      enum Flags
      {
        JEVOIS_SENSOR_COLOR    = 0x00,
        JEVOIS_SENSOR_MONO     = 0x01,
        JEVOIS_SENSOR_ICM20948 = 0x02
      };
      
      //! Get the sensor flags
      Flags readFlags();
      
    private:
      jevois::CameraSensor itsSensor;
      std::vector<std::shared_ptr<jevois::CameraDevice>> itsDev;
      int itsDevIdx = -1, itsDev2Idx = -1;
      int itsFd = -1, itsFd2 = -1;
      Flags itsFlags;
      
      mutable std::timed_mutex itsMtx;
  };

} // namespace jevois
