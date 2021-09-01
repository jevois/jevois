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

#include <jevois/Image/RawImage.H>
#include <opencv2/core/core.hpp>
#include <memory>

namespace jevois
{
  class VideoInput;
  class Engine;

  //! Exception-safe wrapper around a raw camera input frame
  /*! This wrapper operates much like std:future in standard C++11. Users can get the next image captured by the camera
      by calling get(), which may block if the capture is not complete yet, or may throw if the capture fails for some
      reason (e.g., the camera is not streaming). The image size and pixel type are as defined by the current
      VideoMapping, camera section. In addition, a done() function is provided which users may use as soon as they are
      finished with the pixel data in the image obtained via get(), to allow the camera driver to setup the underlying
      memory buffer again for capture. If done() has not been called by the time the InputFrame is destroyed, it will be
      called automatically, if get() had been called. It may in some cases improve your frame rate to call done()
      manually as early as possible instead of letting the InputFrame destructor do it.

      InputFrame implements a zero-copy, zero-wait access to input video frames, that is:

      1. the pixel data of the image you obtain via get() is directly the memory-mapped pixel buffer that the silicon
         hardware on the JeVois chip uses via direct-memory-access (DMA) to stream the pixel data from the camera chip
         to processor memory;
      2. as soon as an image is captured by the camera hardware, get() unblocks and returns it (as opposed to having a
         fixed, regular interval at which images may be available). Camera has several image buffers, allowing one to be
         captured while another is being handed over for processing via get(). These buffers are recycled, i.e., once
         done() is called, the underlying buffer is sent back to the camera hardware for future capture.

      \ingroup core */
  class InputFrame
  {
    public:
      //! Move constructor
      InputFrame(InputFrame && other) = default;
      
      //! Get the next captured camera image
      /*! Throws if we the camera is not streaming or blocks until an image is available (has been captured). It is ok
          to call get() several times, but the same image will always be returned. To obtain successive frames from the
          camera, you must be fed successive InputFrame wrappers by the JeVois Engine. */
      RawImage const & get(bool casync = false) const;

      //! Check whether a second input image scaled by the JeVoisPro Platform ISP is available
      /*! Returns false unless we are on JeVois-Pro Platform and the camera format modifier jevois::CropType::CropScale
          is currently in use. */
      bool hasScaledImage() const;
      
      //! Get the next captured camera image, ISP-scaled second frame
      /*! On JeVois-Pro Platform only, the camera ISP can output 2 frames: 1) raw from sensor, 2) scaled by ISP. This
          function is to access the ISP scaled frame. Throws if not JeVois-Pro Platform or the camera stream type is not
          jevois::StreamType::RawAndScaled. Throws if we the camera is not streaming or blocks until an image is
          available (has been captured). It is ok to call get2() several times, but the same image will always be
          returned. To obtain successive frames from the camera, you must be fed successive InputFrame wrappers by the
          JeVois Engine. */
      RawImage const & get2(bool casync = false) const;

      //! Get the next captured camera image that is intended for processing
      /*! Same as get() if hasScaledImage() is false, or as get2() if hasScaledImage() is true. */
      RawImage const & getp(bool casync = false) const;

      //! Get the DMA-BUF file descriptor of the camera frame
      /*! This file descriptor can be used to share the pixel buffer across interfaces that support DMA-BUF. The JeVois
          core uses this to create a zero-copy pipeline from the camera sensor to the hardware-accelerated OpenGL
          display on JeVois-Pro. Returns -1 if DMA-BUF is not supported (on host, and on JeVois-A33 platform). If get()
          has not previously been called, it will be called, which blocks until the next camera frame is available. It
          is ok to call getDmaFd() several times but always the same fd is returned (see get()). */
      int getDmaFd(bool casync = false) const;

      //! Get the DMA-BUF file descriptor of the ISP-scaled second camera frame
      /*! On JeVois-Pro Platform only, the camera ISP can output 2 frames: 1) raw from sensor, 2) scaled by ISP. This
          function is to access the ISP scaled frame. Throws if not JeVois-Pro Platform or the camera stream type is not
          jevois::StreamType::RawAndScaled. This file descriptor can be used to share the pixel buffer across interfaces
          that support DMA-BUF. The JeVois core uses this to create a zero-copy pipeline from the camera sensor to the
          hardware-accelerated OpenGL display on JeVois-Pro. Returns -1 if DMA-BUF is not supported (on host, and on
          JeVois-A33 platform). If get() has not previously been called, it will be called, which blocks until the next
          camera frame is available. It is ok to call getDmaFd() several times but always the same fd is returned (see
          get2()). */
      int getDmaFd2(bool casync = false) const;
      
      //! Indicate that user processing is done with the image previously obtained via get()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled and sent back to the camera driver for video capture. */
      void done() const;

      //! Indicate that user processing is done with the ISP-scaled image previously obtained via get2()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled and sent back to the camera driver for video capture. */
      void done2() const;

      //! Shorthand to get the input image as a GRAY cv::Mat and release the raw buffer
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. C++ module writers should stick to the get()/done() pair as this provides better fine-grained
          control. Note that the raw image from the camera will always be copied or converted to cv::Mat and will then
          be released by calling done(), so users should not call done() after using this function. This function is
          basically equivalent to calling get(), converting to cv::Mat, and calling done(). */
      cv::Mat getCvGRAY(bool casync = false) const;

      //! Shorthand to get the input image as a BGR cv::Mat and release the raw buffer
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. C++ module writers should stick to the get()/done() pair as this provides better fine-grained
          control. Note that the raw image from the camera will always be copied or converted to cv::Mat and will then
          be released by calling done(), so users should not call done() after using this function. This function is
          basically equivalent to calling get(), converting to cv::Mat, and calling done(). */
      cv::Mat getCvBGR(bool casync = false) const;

      //! Shorthand to get the input image as a RGB cv::Mat and release the raw buffer
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. C++ module writers should stick to the get()/done() pair as this provides better fine-grained
          control. Note that the raw image from the camera will always be copied or converted to cv::Mat and will then
          be released by calling done(), so users should not call done() after using this function. This function is
          basically equivalent to calling get(), converting to cv::Mat, and calling done(). */
      cv::Mat getCvRGB(bool casync = false) const;

      //! Shorthand to get the input image as a RGBA cv::Mat and release the raw buffer
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. C++ module writers should stick to the get()/done() pair as this provides better fine-grained
          control. Note that the raw image from the camera will always be copied or converted to cv::Mat and will then
          be released by calling done(), so users should not call done() after using this function. This function is
          basically equivalent to calling get(), converting to cv::Mat, and calling done(). */
      cv::Mat getCvRGBA(bool casync = false) const;

      //! Shorthand to get the input image for processing as a GRAY cv::Mat and release the raw buffer
      /*! Returns the frame intended for processing, i.e., either the single camera frame when using single-stream
          capture, or the second frame when using dual stream capture. This is mostly intended for Python module
          writers, as they will likely use OpenCV for all their image processing. C++ module writers should stick to the
          get()/done() pair as this provides better fine-grained control. Note that the raw image from the camera will
          always be copied or converted to cv::Mat and will then be released by calling done(), so users should not call
          done() after using this function. This function is basically equivalent to calling get(), converting to
          cv::Mat, and calling done(). */
      cv::Mat getCvGRAYp(bool casync = false) const;

      //! Shorthand to get the input image for processing as a BGR cv::Mat and release the raw buffer
      /*! Returns the frame intended for processing, i.e., either the single camera frame when using single-stream
          capture, or the second frame when using dual stream capture. This is mostly intended for Python module
          writers, as they will likely use OpenCV for all their image processing. C++ module writers should stick to the
          get()/done() pair as this provides better fine-grained control. Note that the raw image from the camera will
          always be copied or converted to cv::Mat and will then be released by calling done(), so users should not call
          done() after using this function. This function is basically equivalent to calling get(), converting to
          cv::Mat, and calling done(). */
      cv::Mat getCvBGRp(bool casync = false) const;

      //! Shorthand to get the input image for processing as a RGB cv::Mat and release the raw buffer
      /*! Returns the frame intended for processing, i.e., either the single camera frame when using single-stream
          capture, or the second frame when using dual stream capture. This is mostly intended for Python module
          writers, as they will likely use OpenCV for all their image processing. C++ module writers should stick to the
          get()/done() pair as this provides better fine-grained control. Note that the raw image from the camera will
          always be copied or converted to cv::Mat and will then be released by calling done(), so users should not call
          done() after using this function. This function is basically equivalent to calling get(), converting to
          cv::Mat, and calling done(). */
      cv::Mat getCvRGBp(bool casync = false) const;

      //! Shorthand to get the input image for processing as a RGBA cv::Mat and release the raw buffer
      /*! Returns the frame intended for processing, i.e., either the single camera frame when using single-stream
          capture, or the second frame when using dual stream capture. This is mostly intended for Python module
          writers, as they will likely use OpenCV for all their image processing. C++ module writers should stick to the
          get()/done() pair as this provides better fine-grained control. Note that the raw image from the camera will
          always be copied or converted to cv::Mat and will then be released by calling done(), so users should not call
          done() after using this function. This function is basically equivalent to calling get(), converting to
          cv::Mat, and calling done(). */
      cv::Mat getCvRGBAp(bool casync = false) const;

      //! Destructor, returns the buffers to the driver as needed
      ~InputFrame();
      
    private:
      InputFrame() = delete;
      InputFrame(InputFrame const & other) = delete;
      InputFrame & operator=(InputFrame const & other) = delete;

      friend class Engine;
      InputFrame(std::shared_ptr<VideoInput> const & cam, bool turbo); // Only our friends can construct us

      std::shared_ptr<VideoInput> itsCamera;
      mutable bool itsDidGet = false, itsDidGet2 = false;
      mutable bool itsDidDone = false, itsDidDone2 = false;
      mutable RawImage itsImage, itsImage2;
      mutable int itsDmaFd = -1, itsDmaFd2 = -1;
      bool const itsTurbo;
  };

} // namespace jevois
