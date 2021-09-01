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
  class VideoOutput;
  class Engine;
  
  //! Exception-safe wrapper around a raw image to be sent over USB
  /*! This wrapper operates much like std:future in standard C++11. Users can get the next memory-allocated but blank
      image to be sent over USB by calling get(), which may block if all buffers are still being sent over USB by Gadget
      and no blank one is available, or may throw if getting that buffer fails for some reason (e.g., usb disconnect,
      user just changed video mode in their webcam software or closed it). The allocated image size and pixel type is as
      defined by the current VideoMapping, USB section, i.e., it is the USB video mode currently selected by the
      user. To save time, image buffers are not zeroed out, so you should not assume that the image is filled with black
      pixels, it could contain random pixels, or previous output frames.  In addition, a send() function is provided
      which users may use as soon as they are finished with writing the pixel data into the image obtained via get(), to
      allow the USB driver to send that image to the connected host computer. If send() has not been called by the time
      the OutputFrame is destroyed, it will be called automatically, if get() had been called.

      OutputFrame implements a zero-copy, zero-wait access to output video frames, that is:

      1. the pixel data of the image you obtain via get() is directly the memory-mapped pixel buffer that the silicon
         hardware on the JeVois chip will use via direct-memory-access (DMA) to stream the data out over the USB link;
      2. as soon as you call send() that buffer will be queued for sending over USB (as opposed to having a fixed,
         regular interval at which images may be streamed out). Gadget has several image buffers, allowing one to be
         streamed out over USB while another is being handed over for filling by your application via get(). These
         buffers are recycled, i.e., once send() is called, the underlying buffer is streamed over USB and then sent
         back to the Gadget for future access by your code.

      \ingroup core */
  class OutputFrame
  {
    public:
      //! Move constructor
      OutputFrame(OutputFrame && other) = default;
      
      //! Get a pre-allocated image so that we can fill the pixel data and later send out over USB using send()
      /*! May throw if not buffer is available, i.e., all have been queued to send to the host but have not yet been
          sent. Application code must balance exactly one send() for each get(). */
      RawImage const & get() const;

      //! Send an image out over USB to the host computer
      /*! May throw if the format is incorrect or std::overflow_error if we have not yet consumed the previous image. */
      void send() const;

      //! Shorthand to send a cv::Mat after converting / scaling it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat will be rescaled to the same dims as the output frame.

	  The pixel format of the given cv::Mat is guessed as follows:
	  - if img.type() == CV_8UC3, assume BGR pixels
	  - if img.type() == CV_8UC1, assume GRAY pixels
	  - if img.type() == CV_8UC4, assume RGBA pixels

	  C++ module writers should stick to the get()/send() pair as this provides better fine-grained control. Note
          that the cv::Mat image will always be copied or converted to the destination RawImage and will then be sent
          out immediately by calling send(), so users should not call send() after using this function. This function is
          basically equivalent to calling get(), converting the given cv::Mat to the proper output format, and calling
          send(). quality is used only if the output format is MJPEG and should be between 1 and 100.*/
      void sendCv(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a GRAY cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat must have same dims as the output frame. C++ module writers should stick to the
          get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always be
          copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100. */
      void sendCvGRAY(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a BGR cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat must have same dims as the output frame. C++ module writers should stick to the
          get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always be
          copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100.*/
      void sendCvBGR(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a RGB cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat must have same dims as the output frame. C++ module writers should stick to the
          get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always be
          copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100.*/
      void sendCvRGB(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a RGBA cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat must have same dims as the output frame. C++ module writers should stick to the
          get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always be
          copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100.*/
      void sendCvRGBA(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a GRAY cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat will be rescaled to the same dims as the output frame. C++ module writers should stick
          to the get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always
          be copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100. */
      void sendScaledCvGRAY(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a BGR cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat will be rescaled to the same dims as the output frame. C++ module writers should stick
          to the get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always
          be copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100.*/
      void sendScaledCvBGR(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a RGB cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat will be rescaled to the same dims as the output frame. C++ module writers should stick
          to the get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always
          be copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100.*/
      void sendScaledCvRGB(cv::Mat const & img, int quality = 75) const;

      //! Shorthand to send a RGBA cv::Mat after converting it to the current output format
      /*! This is mostly intended for Python module writers, as they will likely use OpenCV for all their image
          processing. The cv::Mat will be rescaled to the same dims as the output frame. C++ module writers should stick
          to the get()/send() pair as this provides better fine-grained control. Note that the cv::Mat image will always
          be copied or converted to the destination RawImage and will then be sent out immediately by calling send(), so
          users should not call send() after using this function. This function is basically equivalent to calling
          get(), converting the given cv::Mat to the proper output format, and calling send(). quality is used only if
          the output format is MJPEG and should be between 1 and 100.*/
      void sendScaledCvRGBA(cv::Mat const & img, int quality = 75) const;
      
      //! Destructor, returns the buffers to the driver as needed
      ~OutputFrame();
      
    private:
      OutputFrame() = delete;
      OutputFrame(OutputFrame const & other) = delete;
      OutputFrame & operator=(OutputFrame const & other) = delete;

      // Only our friends can construct us:
      friend class Engine;
      OutputFrame(std::shared_ptr<VideoOutput> const & gad, RawImage * excimg = nullptr);

      std::shared_ptr<VideoOutput> itsGadget;
      mutable bool itsDidGet;
      mutable bool itsDidSend;
      mutable RawImage itsImage;
      jevois::RawImage * itsImagePtrForException;
  };

} // namespace jevois
