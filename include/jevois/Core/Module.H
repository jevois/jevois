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

#include <memory>
#include <jevois/Image/RawImage.H>
#include <jevois/Core/VideoBuf.H>
#include <jevois/Core/UserInterface.H> // not strictly needed but derived classes will want to use it
#include <jevois/Component/Component.H>
#include <jevois/Types/ObjReco.H>
#include <jevois/Types/Enum.H>
#include <opencv2/core/core.hpp>
#include <ostream>

namespace jevois
{
  class VideoInput;
  class VideoOutput;
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
      /*! Throws if we the camera is not streaming or blocks until an image is available (has been captured). */
      RawImage const & get(bool casync = false) const;

      //! Indicate that user processing is done with the image previously obtained via get()
      /*! You should call this as soon after get() as possible, once you are finished with the RawImage data so that it
          can be recycled and sent back to the camera driver for video capture. */
      void done() const;

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

      //! Destructor, returns the buffers to the driver as needed
      ~InputFrame();
      
    private:
      InputFrame() = delete;
      InputFrame(InputFrame const & other) = delete;
      InputFrame & operator=(InputFrame const & other) = delete;

      friend class Engine;
      InputFrame(std::shared_ptr<VideoInput> const & cam, bool turbo); // Only our friends can construct us

      std::shared_ptr<VideoInput> itsCamera;
      mutable bool itsDidGet;
      mutable bool itsDidDone;
      mutable RawImage itsImage;
      bool const itsTurbo;
  };

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

  //! Virtual base class for a vision processing module
  /*! Module is the base class to implement camera-to-USB frame-by-frame video processing. The Engine instantiates one
      class derived from Module, according to the current VideoMapping selected by the end user (e.g., current image
      resolution, format, and frame rate setected by a webcam viewing program on a host computer). The Module is loaded
      as shared object (.so) file according to the VideoMapping definitions in videomappings.cfg and the current
      VideoMapping selected by the user.

      Module derives from Component and as such can contain:

      - any number of Parameter settings

      - any arbitrarily complex sub-hierarchy of Component objects to implement various functionality. Parameter
        settings from all the sub-components are available to the Module and to users connected over Serial ports of the
        Engine.

      This allows one to implement complex vision processing pipelines efficiently and with substantial code re-use. For
      example, one may first want to implement an EdgeDetector or Saliency component, with Parameter settings for
      various thresholds, features, etc. One can then create any number of top-level objects that derive from Module and
      that may contain one or more EdgeDetector, Saliency, etc components in their hierarchy of sub-components, with the
      implementation in the module simply routing images from one Component to another to create a processing pipeline.

      Classes that derive from Module should implement up to four functions:

      - process(InputFrame && inframe, OutputFrame && outframe) is called once per iteration of the Engine main loop
        when the current VideoMapping specifies both a particular Camera resolution and format, and a USB resolution and
        format. This function should process the received input frame and fill the pixel buffer of the output frame with
        results. Memory has already been allocated for both the input and output images before process() is
        called. Because the output image is actually allocated by the USB Gadget driver (and, ultimately, by the Linux
        kernel), its pixel memory location cannot be moved (hence, do not attempt to copy the output image or replace it
        by another image, etc; just write pixel data into the output image's pixel array). There is no restriction on
        video modes or frame rates, except as suported by the Camera hardware, and as limited by USB bandwidth. For most
        implementations, matching the input and output frame rate is easiest, and means that each invocation of
        process() would access and use both of the provided InputFrame and OutputFrame (one-input-to-one-output
        processing pipeline). But this is not mandatory. For example, a motion flow computation Module for use in a
        flying drone may have 320x240 YUYV 53.0fps inputs and 100x142 GREY 10.6fps output (since output frame rate is 5x
        lower than input in this example, the process() function would here get, fill, and send the OutputFrame only
        once every 5 times it is called; implementation of the process() function should keep track of that, e.g.,
        through a member variable that gets incremented each time process() is called). In addition to filling the pixel
        data of the OutputFrame, process() may also send results over the serial ports (e.g., for use by an Arduino
        connected to the JeVois platform hardware) using sendSerial().

      - process(InputFrame && inframe) is called once per Camera frame when the current VideoMapping specifies a
        particular Camera resolution and format, and NONE as USB output format. This function should process the
        received input frame and would typicaly then send results to serial ports (e.g., for use by an Arduino connected
        to the JeVois platform hardware) using sendSerial(). There is no restriction on video modes or frame rates,
        except as suported by the Camera hardware.

      - parseSerial(std::string const & str, std::shared_ptr<UserInterface> s) allows the Module to support custom user
        commands. Engine will forward to this function any command received over Serial or other UserInterface that it
        does not understand. You should use this for things that go beyond Parameter settings (which is already natively
        supported by Engine) or built-in commands of Engine (see \ref UserCli). For example, one could implement here a
        command called "start" to allow users to start some specific thing.

      - supportedCommands(std::ostream & os) should stream out a human-readable description of any custom commands
        supported by parseSerial(). These will be shown to users when they type "help" over a Serial port.

      \note Every module implementation file should contain a call to #JEVOIS_REGISTER_MODULE(MODULENAME) for the
      module's class. This creates some plain-C entry points that will be used when the module is loaded from a dynamic
      library (.so) file to instantiate the module. See \ref ModuleTutorial for examples.

      \ingroup core */
  class Module : public Component
  {
    public:
      //! Constructor
      /*! the instance is a user-defined string that may be used to differentiate between several instances of the
          same module. */
      Module(std::string const & instance);

      //! Virtual destructor for safe inheritance
      virtual ~Module();

      //! Processing function, version that receives a frame from camera and sends a frame out over USB
      /*! This function is called once for each grabbed video frame from the camera, and it should complete within the
          camera's frame period in order to avoid dropping frames. The InputFrame and OutputFrame objects are simple
          wrappers to ensure that the low-level video buffers will always be returned to the low-level camera and USB
          drivers even if the process function throws at any point during the processing. If any error occurs, it is
          hence ok to throw from within process() at any time, just make sure your locally allocated resources will be
          freed, which is usually best achieved by using shared_ptr and similar wrappers around them. The Engine (which
          calls process() on your module for every frame) will catch any exception an proceed to the next frame. 

          Default implementation in the base class just throws. Derived classes should override it. */
      virtual void process(InputFrame && inframe, OutputFrame && outframe);

      //! Processing function, version that receives a frame from camera and does not use USB
      /*! This function is called once for each grabbed video frame from the camera, and it should complete within the
          camera's frame period in order to avoid dropping frames. The InputFrame object is a simple wrapper to ensure
          that the low-level video buffers will always be returned to the low-level camera driver even if the process
          function throws at any point during the processing. If any error occurs, it is hence ok to throw from within
          process() at any time, just make sure your locally allocated resources will be freed, which is usually best
          achieved by using shared_ptr and similar wrappers around them. The Engine (which calls process() on your
          module for every frame) will catch any exception an proceed to the next frame.

          Default implementation in the base class just throws. Derived classes should override it. */
      virtual void process(InputFrame && inframe);

      //! Send a string over the 'serout' serial port
      /*! The default implementation just sends the string to the serial port specified by the 'serout' Parameter in
          Engine (which could be the hardware serial port, the serial-over-USB port, both, or none; see \ref UserCli for
          information about \c serout). No need to override in most cases. Typically, you would use this function from
          within process() to send out some results of your processing.

          Note that the default 'serout' Parameter setting in Engine is None. This is to allow users to configure
          parameters, get parameter values, possibly read the help message, etc before the flow of serial outputs from
          vision processing starts. Once ready to receive serial outputs, one would typically issue a command 'setpar
          serout Hard' over the JeVois command line to enable serial outputs to the hardware serial port. An Arduino
          would issue that setpar commands when it is ready to work. See ArduinoTutorial for an example. */
      virtual void sendSerial(std::string const & str);

      //! Receive a string from a serial port which contains a user command
      /*! This function may be called in between calls to process() with any received string from any of the serial
          ports. Some commands are parsed upstream already (like "help", set param value, set camera control, etc; see
          the Engine class) and will not be received here. Only the ones not recognized by the Engine will be received
          (i.e., custom commands specific to your module).

          The default implementation just throws std::runtime_error("Unsupported command"), but some modules may want to
          override this function to handle custom commands. If you successfully process the command, just return;
          otherwise, throw, and if your exception derives from std::exception, the Engine will append its what() to the
          error message issued to the user. When you support commands here, you should update the implementation of
          supportedCommands() to provide some description of those commands to the users.

          The \c s parameter is the serial port that received the command. You can send any results back to that port
          using writeString() on it. Note that the Engine will automatically add the 'OK' message upon success, so you
          do not have to send that here. */
      virtual void parseSerial(std::string const & str, std::shared_ptr<UserInterface> s);

      //! Human-readable description of this Module's supported custom commands
      /*! The format here is free. Just use std::endl to demarcate lines, these will be converted to the appropriate
          line endings by the serial ports. Default implementation writes "None" to os. */
      virtual void supportedCommands(std::ostream & os);

      //! Get frame number from the Engine
      /*! The Engine maintains a master frame counter that is incremented on each call to process(), whether or not the
	  call succeeds. The counter is not incremented when a module has not been loaded (e.g., failed to load). The
	  counter is reset to zero each time a new module is loaded. */
      size_t frameNum() const;

      //! Write a value of one of the camera's registers
      /*! This very low-level access is for development of optimal camera settings only and should not be used in normal
          operation, it can crash your system. */
      void writeCamRegister(unsigned short reg, unsigned short val);

      //! Read a value from one of the camera's registers
      /*! This very low-level access is for development of optimal camera settings only and should not be used in normal
          operation, it can crash your system. */
      unsigned short readCamRegister(unsigned short reg);

      //! Write a value to one of the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. */
      void writeIMUregister(unsigned short reg, unsigned short val);

      //! Read a value from one of the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. */
      unsigned short readIMUregister(unsigned short reg);

      //! Write an array of values to the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. */
      void writeIMUregisterArray(unsigned short reg, unsigned char const * vals, size_t num);

      //! Read an array of values from the camera's IMU registers
      /*! This very low-level access is for development of optimal IMU settings only and should not be used in normal
          operation, it can crash your system. Memory for vals should have been allocated by the caller with at least
          num bytes. num must be less than 32. */
      void readIMUregisterArray(unsigned short reg, unsigned char * vals, size_t num);
  };

  namespace module
  {
    static ParameterCategory const ParamCateg("Module Serial Message Options");

    //! Enum for Parameter \relates jevois::StdModule
    JEVOIS_DEFINE_ENUM_CLASS(SerStyle, (Terse) (Normal) (Detail) (Fine) )
    
    //! Parameter \relates jevois::StdModule
    JEVOIS_DECLARE_PARAMETER(serstyle, SerStyle, "Style for standardized serial messages as defined in "
                             "http://jevois.org/doc/UserSerialStyle.html",
                             SerStyle::Terse, SerStyle_Values, ParamCateg);

    //! Parameter \relates jevois::StdModule
    JEVOIS_DECLARE_PARAMETER(serprec, unsigned int, "Number of decimal points in standardized serial messages as "
                             "defined in http://jevois.org/doc/UserSerialStyle.html",
                             0U, jevois::Range<unsigned int>(0U, 10U), ParamCateg);

    //! Enum for Parameter \relates jevois::StdModule
    JEVOIS_DEFINE_ENUM_CLASS(SerStamp, (None) (Frame) (Time) (FrameTime) (FrameDateTime) )

    //! Parameter \relates jevois::StdModule
    JEVOIS_DECLARE_PARAMETER(serstamp, SerStamp, "Prepend standardized serial messages with a frame number, "
			     "time, frame+time, or frame+date+time. See details in "
			     "http://jevois.org/doc/UserSerialStyle.html",
                             SerStamp::None, SerStamp_Values, ParamCateg);

    //! Enum for Parameter \relates jevois::StdModule
    JEVOIS_DEFINE_ENUM_CLASS(SerMark, (None) (Start) (Stop) (Both) )

    //! Parameter \relates jevois::StdModule
    JEVOIS_DECLARE_PARAMETER(sermark, SerMark, "Send serial message to mark the beginning (MARK START) of the "
			     "processing of a video frame from the camera sensor, the end (MARK STOP), or both. "
			     "Useful, among others, if one needs to know when no results were sent over serial "
			     "on a given frame. Combine with parameter serstamp if you need to know the frame number.",
                             SerMark::None, SerMark_Values, ParamCateg);
  }
  
  //! Base class for a module that supports standardized serial messages
  /*! Modules that can output standardized serial messages should derive from StdModule instead of Module. StdModule
      brings in extra parameters to set serial message style and precision, and extra member functions to assemble,
      format, and send standardized serial messages. The process(), sendSerial(), parseSerial(), supportedCommands(),
      etc of StdModule functions are directly inherited from Module. See \ref UserSerialStyle for standardized serial
      messages. \ingroup core */
  class StdModule : public Module,
		    public Parameter<module::serprec, module::serstyle, module::serstamp, module::sermark>
  {
    public:
      //! Constructor
      /*! the instance is a user-defined string that may be used to differentiate between several instances of the
          same module. */
      StdModule(std::string const & instance);

      //! Virtual destructor for safe inheritance
      virtual ~StdModule();

      //! Send standardized 1D message for an X image coordinate
      /*! See \ref UserSerialStyle for more info. Coordinates should be in camera image pixels, this function will
          convert them to standardized coordinates as per \ref coordhelpers. */
      void sendSerialImg1Dx(unsigned int camw, float x, float size = 0.0F, std::string const & id = "",
                            std::string const & extra = "");

      //! Send standardized 1D message for a standardized X coordinate
      /*! See \ref UserSerialStyle for more info. Coordinates should be in camera image pixels, this function will
          convert them to standardized coordinates as per \ref coordhelpers. */
      void sendSerialStd1Dx(float x, float size = 0.0F, std::string const & id = "", std::string const & extra = "");

      //! Send standardized 1D message for an Y image coordinate
      /*! See \ref UserSerialStyle for more info. Coordinates should be in camera image pixels, this function will
          convert them to standardized coordinates as per \ref coordhelpers. */
      void sendSerialImg1Dy(unsigned int camh, float y, float size = 0.0F, std::string const & id = "",
                            std::string const & extra = "");

      //! Send standardized 1D message for a standardized Y coordinate
      /*! See \ref UserSerialStyle for more info. Coordinates should be in camera image pixels, this function will
          convert them to standardized coordinates as per \ref coordhelpers. */
      void sendSerialStd1Dy(float y, float size = 0.0F, std::string const & id = "", std::string const & extra = "");

      //! Send standardized 2D message for image coordinates
      /*! Use this function if you only know location and optionally size. Use the other variants if you have the
          corners. An upright rectangular shape will be assumed here. See \ref UserSerialStyle for more
          info. Coordinates should be in camera image pixels, this function will convert them to standardized
          coordinates as per \ref coordhelpers. */
      void sendSerialImg2D(unsigned int camw, unsigned int camh, float x, float y, float w = 0.0F, float h = 0.0F,
                           std::string const & id = "", std::string const & extra = "");
      
      //! Send standardized 2D message for standardized coordinates
      /*! Use this function if you only know location and optionally size. Use the other variants if you have the
          corners. An upright rectangular shape will be assumed here. See \ref UserSerialStyle for more
          info. Coordinates should be in camera image pixels, this function will convert them to standardized
          coordinates as per \ref coordhelpers. */
      void sendSerialStd2D(float x, float y, float w = 0.0F, float h = 0.0F,
                           std::string const & id = "", std::string const & extra = "");

      //! Send standardized 2D message for polygons in image coordinates
      /*! Use this function if you have a polygon around your object, for example, one of the contours found with
          cv::findContours(), or if you have the 4 corners of a rectangular object. See \ref UserSerialStyle for more
          info. Coordinates should be in camera image pixels, this function will convert them to standardized
          coordinates as per \ref coordhelpers. For \b Terse serial style, the center of gravity of the points will be
          computed and output; for \b Normal, an upright bounding rectangle will be computed and output; for \b
          Detailed, a rotated bounding rectangle will be computed and output; for \b Fine, all the given points will
          be output. Make sure you try to reduce the number of points so the message is not too long; for example see
          OpenCV approxPolyDP() or similar. */
      template <typename T = int>
      void sendSerialContour2D(unsigned int camw, unsigned int camh, std::vector<cv::Point_<T> > points,
                                std::string const & id = "", std::string const & extra = "");

      //! Send standardized 3D message
      /*! Use this function if you only know location and optionally size and an orientation quaternion. Use the other
          variants if you have a bunch of vertices. See \ref UserSerialStyle for more info. Coordinates should be in
          millimeters. */
      void sendSerialStd3D(float x, float y, float z, float w = 0.0F, float h = 0.0F, float d = 0.0F,
                           float q1 = 0.0F, float q2 = 0.0F, float q3 = 0.0f, float q4 = 0.0F,
                           std::string const & id = "", std::string const & extra = "");

      //! Send standardized 3D message
      /*! Use this function if you only know location and optionally size and an orientation quaternion. Use the other
          variants if you have a bunch of vertices. See \ref UserSerialStyle for more info. Coordinates should be in
          millimeters. */
      void sendSerialStd3D(std::vector<cv::Point3f> points, std::string const & id = "",
                           std::string const & extra = "");

      //! Send a standardized object recognition message
      /*! \param res should be a list of scores and category names, in descending order of scores. Note that no message
          is sent if the vector is empty. */
      void sendSerialObjReco(std::vector<ObjReco> const & res);

      //! Send a standardized object detection + recognition message
      /*! \param res should be a list of scores and category names, in descending order of scores. Note that no message
          is sent if the vector is empty. See sendSerialImg2D() for info about the object box. */
      void sendSerialObjDetImg2D(unsigned int camw, unsigned int camh, float x, float y, float w, float h,
				 std::vector<ObjReco> const & res);
      
    protected:
      friend class jevois::Engine;
      
      //! Send a message <b>MARK START</b> to indicate the beginning of processing
      /*! A stamp may be prepended depending on param \p serstamp. Engine calls this automatically so users would
	  normally not use this function. Note that this function may not send anything depending on the current
	  value of parameter \p sermark. */
      void sendSerialMarkStart();

      //! Send a message <b>MARK STOP</b> to indicate the end of processing
      /*! A stamp may be prepended depending on param \p serstamp. Engine calls this automatically so users would
	  normally not use this function. Note that this function may not send anything depending on the current
	  value of parameter \p sermark. */
      void sendSerialMarkStop();
      
      //! Get a string with the frame/date/time stamp in it, depending on serstamp parameter
      std::string getStamp() const;
  };
}

//! Register a module, allowing it to be dynamically loaded from a .so file
/* \def JEVOIS_REGISTER_MODULE(MODULENAME)
   \hideinitializer 

   Every module implementation file should contain a call to JEVOIS_REGISTER_MODULE for the module's class. This creates
   some plain-C entry points that will be used when the module is loaded from a dynamic library (.so) file to
   instantiate the module.  \relates Module */
#define JEVOIS_REGISTER_MODULE(MODULENAME)                              \
  extern "C" std::shared_ptr<jevois::Module> MODULENAME##_create(std::string const & instanceid) \
  { return std::shared_ptr<jevois::Module>(new MODULENAME(instanceid)); } \
  extern "C" int MODULENAME##_version_major() { return JEVOIS_VERSION_MAJOR; } \
  extern "C" int MODULENAME##_version_minor() { return JEVOIS_VERSION_MINOR; } \


