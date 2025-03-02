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

#include <jevois/Core/VideoMapping.H>
#include <jevois/Core/Serial.H>
#include <jevois/Component/Manager.H>
#include <jevois/Types/Enum.H>
#include <jevois/Image/RawImage.H>
#include <jevois/Core/CameraCalibration.H>
#include <jevois/Debug/Watchdog.H>

#include <memory>
#include <mutex>
#include <vector>
#include <list>
#include <atomic>

// #################### Platform mode config:
#ifdef JEVOIS_PLATFORM

#if defined(JEVOIS_A33)
// ########## JeVois-A33 platform:

// On the JeVois-A33 platform, we use a gadget driver by default to send output frames over USB, one hardware serial
// driver, and one serial-over-USB driver:

//! On platform hardware, device for the camera sensor
#define JEVOIS_CAMERA_DEFAULT "/dev/video0"

//! On platform hardware, device for the USB gadget driver (which sends video frames over USB to a host computer)
#define JEVOIS_GADGET_DEFAULT "/dev/video1"

//! On platform hardware, device for the 4-pin hardware serial port
#define JEVOIS_SERIAL_DEFAULT "/dev/ttyS0"

//! On platform hardware, device for serial-over-USB port
#define JEVOIS_USBSERIAL_DEFAULT "/dev/ttyGS0"

//! Default camera sensor
#define JEVOIS_CAMERASENS_DEFAULT CameraSensor::ov9650

//! Default IMU spi device
#define JEVOIS_IMUSPI_DEFAULT ""

#elif defined(JEVOIS_PRO)
// ########## JeVois-Pro platform:

// On the JeVois-Pro platform, we have no gadget for now (which will trigger displaying output frames to a window), one
// hardware serial driver, and not yet one serial-over-USB driver:

//! On platform hardware, device for the camera sensor
#define JEVOIS_CAMERA_DEFAULT "/dev/video0"

//! On platform hardware, device for the USB gadget driver (which sends video frames over USB to a host computer)
#define JEVOIS_GADGET_DEFAULT ""

//! On platform hardware, device for the 4-pin hardware serial port
#define JEVOIS_SERIAL_DEFAULT "/dev/ttyS4"

//! On platform hardware, device for serial-over-USB port
#define JEVOIS_USBSERIAL_DEFAULT ""
//#define JEVOIS_USBSERIAL_DEFAULT "/dev/ttyGS0"

//! Default camera sensor
#define JEVOIS_CAMERASENS_DEFAULT CameraSensor::any

//! Default IMU spi device
#define JEVOIS_IMUSPI_DEFAULT "/dev/spidev32766.0"

#else
#error "Neither JEVOIS_A33 nor JEVOIS_PRO defined -- ABORT"
#endif

#else // JEVOIS_PLATFORM
// #################### Host mode config:

// On the host, we have no gadget (which will trigger displaying output frames to a window) and we use the terminal in
// which jevois-daemon was started for serial commands:

//! On generic computer hardware, device for the camera sensor
#define JEVOIS_CAMERA_DEFAULT "/dev/video0"

//! On generic computer hardware, device for the USB gadget driver should always be empty
#define JEVOIS_GADGET_DEFAULT ""

//! On generic computer hardware, device for serial port should always be stdio to use an StdioInterface
#define JEVOIS_SERIAL_DEFAULT "stdio"

//! On generic computer hardware, device for the serial-over-USB port should always be empty
#define JEVOIS_USBSERIAL_DEFAULT ""

//! Default IMU spi device
#define JEVOIS_IMUSPI_DEFAULT ""

#ifdef JEVOIS_PRO
//! Default camera sensor
#define JEVOIS_CAMERASENS_DEFAULT CameraSensor::imx290
#else
//! Default camera sensor
#define JEVOIS_CAMERASENS_DEFAULT CameraSensor::ov9650
#endif

#endif // JEVOIS_PLATFORM

namespace cv { namespace parallel { class ParallelForAPI; } }

namespace jevois
{
  class VideoInput;
  class VideoOutput;
  class Module;
  class DynamicLoader;
  class UserInterface;
  class GUIhelper;
  class GUIconsole;
  class Camera;
  class IMU;

  //! Parameters of the Engine class
  namespace engine
  {
    static ParameterCategory const ParamCateg("Engine Options");

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(cameradev, std::string, "Camera device name (if starting with /dev/v...), or movie "
                             "file name (e.g., movie.mpg) or image sequence (e.g., im%02d.jpg, to read frames "
                             "im00.jpg, im01.jpg, etc).",
                             JEVOIS_CAMERA_DEFAULT, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(camerasens, CameraSensor, "Camera sensor. Users would usually not set this parameter "
                             "manually, it is set through boot-time configuration.",
                             JEVOIS_CAMERASENS_DEFAULT, CameraSensor_Values, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(cameralens, CameraLens, "Camera lens. Users should usually set this parameter "
                             "using the global JeVois params.cfg config file.",
                             CameraLens::standard, CameraLens_Values, ParamCateg);

    JEVOIS_DECLARE_PARAMETER(imudev, std::string, "IMU SPI device name, typically starting with /dev/spidev..., "
                             "or empty if device does not have an IMU with SPI interface.",
                             JEVOIS_IMUSPI_DEFAULT, ParamCateg);
    
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(cameranbuf, unsigned int, "Number of video input (camera) buffers, or 0 for automatic.",
                             0, ParamCateg);
    
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(gadgetdev, std::string, "Gadget device name. This is used on platform hardware only. "
                             "On host hardware, a display window will be used unless gadgetdev is None (useful "
                             "for benchmarking) or is a file stem for a movie file that does not start with /dev/ "
                             "(and which should contain a printf-style directive for a single int argument, "
                             "the movie number).",
                             JEVOIS_GADGET_DEFAULT, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(gadgetnbuf, unsigned int, "Number of video output (USB video) buffers, or 0 for auto",
                             0, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(videomapping, int, "Index of Video Mapping to use, or -1 to use the default mapping. "
                             "Note that this parameter is only available when parsing command-line arguments. "
                             "At runtime, the setmapping command should be used instead.",
                             -1, ParamCateg);

#ifdef JEVOIS_PRO
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(serialmonitors, bool, "If true, serial port monitors will be enabled "
                             "in the GUI, which can be used to peek at serial communications not "
                             "directed to the console. Can be turned off at start time (e.g., in the "
                             "global JeVois params.cfg) as there is some small CPU cost to it.",
                             true, ParamCateg);
#endif
    
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(serialdev, std::string, "Hardware (4-pin connector) serial device name, "
                                           "or 'stdio' to use the console, or empty for no hardware serial port",
                                           JEVOIS_SERIAL_DEFAULT, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(usbserialdev, std::string, "Over-the-USB serial device name, or empty",
                                           JEVOIS_USBSERIAL_DEFAULT, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(camreg, bool, "Enable raw access to camera registers through setcamreg and getcamreg",
                             false, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(imureg, bool, "Enable raw access to IMU registers through setimureg and getimureg",
                             false, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(camturbo, bool, "Enable camera turbo mode by relaxing the need for DMA-coherent video "
                             "buffer memory. This can accelerate severalfolds access to the captured image data, but "
                             "it may also yield stripe artifacts with some modules, such as PassThrough. The stripes "
                             "are pieces of incorrect data in the cache. You should experiment with each particular "
                             "module. Turbo mode is not recommended for any production-grade application.",
                             false, ParamCateg);

    //! Enum for Parameter \relates jevois::Engine
    JEVOIS_DEFINE_ENUM_CLASS(SerPort, (None) (All) (Hard) (USB) );
    
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(serlog, SerPort, "Show log and debug messages on selected serial port(s)",
                             SerPort::None, SerPort_Values, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(videoerrors, bool, "Show any machine vision module errors (exceptions) "
                                           "in the video stream. Only takes effect if streaming video to USB.",
                                           true, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(serout, SerPort, "Send module serial messages to selected serial port(s)",
                             SerPort::None, SerPort_Values, ParamCateg);
   
    //! Enum for Parameter \relates jevois::Engine
    JEVOIS_DEFINE_ENUM_CLASS(CPUmode, (PowerSave) (Conservative) (OnDemand) (Interactive) (Performance) );

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(cpumode, CPUmode, "CPU frequency modulation mode"
#ifdef JEVOIS_PRO
                                           " for A73 big cores"
#endif
                                           , CPUmode::Performance, CPUmode_Values, ParamCateg);

#ifdef JEVOIS_PRO
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(cpumodel, CPUmode, "CPU frequency modulation mode for A53 little cores",
                                           CPUmode::Performance, CPUmode_Values, ParamCateg);
#endif

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(cpumax, unsigned int, "CPU maximum frequency in MHz"
#ifdef JEVOIS_PRO
                                           ". To enable overclock frequencies above 2208 MHz, you need to first edit "
                                           "/boot/env.txt and change max_freq_a73, then reboot. Use with caution!"
#endif
#ifdef JEVOIS_A33
                                           // keep this in sync with sunxi-cpufreq.c
                                           , 1344, { 120, 240, 312, 408, 480, 504, 600, 648, 720, 816, 912, 1008,
                                                   1044, 1056, 1080, 1104, 1116, 1152, 1200, 1224, 1248, 1296, 1344 },
#else
                                           // keep this in sync with device tree
                                           // A73 cores
                                           , 2208, { 500, 667, 1000, 1200, 1398, 1512, 1608, 1704, 1800, 1908, 2016,
                                                   2100, 2208, 2304, 2400 },
                                           // A53 cores
                                           //1800, { 500, 667, 1000, 1200, 1398, 1512, 1608, 1704, 1800, 1908, 2016,
                                           //        2100, 2208 },
#endif
                                           ParamCateg);
#ifdef JEVOIS_PRO
     //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(cpumaxl, unsigned int, "CPU maximum frequency in MHz, for A53 little cores. "
                                           "To enable overclock frequencies above 1800 MHz, you need to first edit "
                                           "/boot/env.txt and change max_freq_a53, then reboot. Use with caution!",
                                           // keep this in sync with device tree
                                           1800, { 500, 667, 1000, 1200, 1398, 1512, 1608, 1704, 1800, 1908, 2016,
                                                   2100, 2208 },
                                           ParamCateg);
#endif
    
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(multicam, bool, "Allow up to 3 JeVois cameras on one USB bus. Enabling this "
			     "reduces the amount of USB bandwidth used by each JeVois camera, from 3kb "
			     "per USB isochronous microframe to 1kb. All 3 JeVois cameras must have this "
			     "option enabled, and the JeVois linux kernel module should also have "
			     "been loaded with multicam on.",
			     false, ParamCateg);
    
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(quietcmd, bool, "When true, do not issue a message 'OK' after every correct command "
			     "received at the command-line interface. Recommended for advanced users only.",
			     false, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(python, bool, "When true, enable support for modules written in Python. Otherwise, "
			     "attempting to load a python module will throw an exception. Disabling python saves "
			     "a lot of memory and may be useful when using C++ modules that run large deep neural "
			     "networks.",
			     true, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(serlimit, size_t, "Maximum number of serial messages that can be sent by a module "
			     "using sendSerial(), for each video frame, or 0 for no limit. Any message sent by "
			     "the module beyond the first serlimit ones will be dropped. This is useful to avoid "
			     "overloading the serial link, for example in case one is running a ArUco detector and "
			     "a large number of ArUco tags are present in the field of view of JeVois.",
			     0, ParamCateg);

#ifdef JEVOIS_PRO
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(gui, bool, "Use a graphical user interface instead of plain display "
                                           "when true",
                                           true, ParamCateg);
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(conslock, bool, "Lock the console and capture the keyboard and mouse to avoid "
                             "interference, only effective on JeVois-Pro Platform, otherwise ignored. Set conslock "
                             "to false if you are experiencing hard crashes and want to run jevoispro-daemon in gdb.",
                             true, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(watchdog, double, "Timeout in seconds after which we kill this process if the main loop "
                             "is stuck somehow, or 0.0 for no watchdog",
                             10.0, ParamCateg);

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(demomode, float, "Show a demonstration of some available JeVois-Pro "
                                           "machine vision modules, cycling to the next modules after a number "
                                           "of seconds specified by this parameter (or 0.0 for no demo mode).",
                                           0.0F, ParamCateg);
#endif
  }

  //! JeVois processing engine - gets images from camera sensor, processes them, and sends results over USB
  /*! The Engine is orchestrating the execution of vision processing software. It is a Manager, i.e., it is the root of
      a hierarchy of Component objects and it handles access to their Parameter settings and their construction, init(),
      unInit(), and destruction. The component hierarchy consists of Engine at the root, then one Module which is
      selected by the user at runtime, e.g., by selecting a given video format on video camera software running on a
      host computer connected to the JeVois hardware. The Module may then contain an arbitrarily complex hierarchy of
      Component objects with Parameter settings in them. Module derives from Component and thus may also have its own
      Parameter settings.

      Engine contains the following basic elements:

      - A VideoInput, instantiated as either a Camera for live video streaming or a MovieInput for processing of
        pre-recorded video files or sequences of images (useful during algorithm development, to test and optimize on
        reproducible inputs);

      - A VideoOutput, instantiated either as a USB Gadget driver when running on the JeVois hardware platform, or as a
        VideoDisplay when running on a computer that has a graphics display, or as a MovieOutput to save output video
        frames to disk, or as a VideoOutputNone if desired for benchmarking of vision algorithms while discounting any
        work related to transmitting output frames.

      - A DynamicLoader which handles loading the chosen vision processing Module at runtime depending on user
        selections;

      - Any number of UserInterface objects, instantiated as either a hardware Serial port (for the 4-pin JST 1.0mm
        connector on the platform hardware), a serial-over-USB Serial port (visible on the host computer to which the
        JeVois hardware is connected by USB), or an StdioInterface (used to accept commands and print results directly
        in the terminal where the JeVois Engine was started, particularly useful when running on a generic computer as
        opposed to the platform hardware). When running on platform hardware, usually two UserInterface objects are
        created (one hardware Serial, one serial-over-USB Serial), while, when running on a generic computer, usually
        only one UserInterface is created (of type StdioInterface to accept commands directly in the terminal in which
        the jevois-daemon was started);

      - The list of VideoMapping definitions imported from your videomappings.cfg file. These definitions specify which
        video output modes are available over USB and their corresponding Camera settings and which Module to use, as
        well as which modes are available that do not have any sreaming video output over USB (e.g., when connecting the
        hardware platform to an Arduino only).

     The main loop of Engine runs until the user decides to quit, and basically goes through the following steps:

      - Create an InputFrame object which is an exception-safe wrapper around the next available Camera frame. The frame
        may not have been captured yet. The InputFrame can be understood as a mechanism to gain access to that frame in
        the future, when it has become available (i.e., has been captured by the camera). This is very similar to the
        std::future framework of C++11.

      - When the current VideoMapping specifies that we will be streaming video frames out over USB, also create an
        OutputFrame object which is an exception-safe wrapper around the next available Gadget frame. This is also just
        a mechanism for gaining access to the next blank video buffer that is available from the USB driver and that we
        should fill with interesting pixel data before sending it over USB to a host computer.

      - Call the currently-loaded Module's process() function, either as process(InputFrame, OutputFrame) when the
        current VideoMapping specifies that some video output is to be sent over USB, or as process(InputFrame) when the
        current VideoMapping specifies no video output. Any exception thrown by the Module's process() function will be
        caught, reported, and ignored. The process() function would typically request the next available camera image
        through the InputFrame wrapper (this request may block until the frame has been captured by the camera sensor
        hardware), process that image, request the next available output image through the OutputFrame wrapper (when
        VideoMapping specifies that there is USB video output), and paint some results into that output image, which
        will then be sent to the host coputer over USB, for display by some webcam program or for further processing by
        some custom vision software running on that computer. In addition, the currently loaded Module may issue
        messages over the UserInterface ports (e.g., indicating the location at which an object was found, to let an
        Arduino know about it).

      - Read any new commands issued by users over the UserInterface ports and execute the appropriate commands.

      - Handle user requests to change VideoMapping, when they select a different video mode in their webcam software
        running on the host computer connected to the JeVois hardware. Such requests may trigger unloading of the
        current Module and loading a new one, and changing camera pixel format, image size, etc. These changes are
        guaranteed to occur when the Module's process() function is not running, i.e., Module programmers do not have to
        worry about possible changes in image dimensions or pixel formats during execution of their process() function.

      - Pass any user requests received over USB or UserInterface to adjust camera parameters to the actual Camera
        hardware driver (e.g., when users change contrast in their webcam program, that request is sent to the Engine
        over USB, and the Engine then forwards it to the Camera hardware driver).

     \ingroup core */
  class Engine : public Manager,
                 public Parameter<engine::cameradev, engine::camerasens, engine::cameralens, engine::cameranbuf,
                                  engine::gadgetdev, engine::gadgetnbuf, engine::imudev, engine::videomapping,
                                  engine::serialdev, engine::usbserialdev, engine::camreg, engine::imureg,
                                  engine::camturbo, engine::serlog, engine::videoerrors, engine::serout,
                                  engine::cpumode, engine::cpumax, engine::multicam, engine::quietcmd,
                                  engine::python, engine::serlimit
#ifdef JEVOIS_PRO
                                  , engine::serialmonitors, engine::gui, engine::conslock, engine::cpumaxl,
                                  engine::cpumodel, engine::watchdog, engine::demomode
#endif
                                  >
  {
    public:
      //! Constructor
      Engine(std::string const & instance);

      //! Constructor with command-line parsing
      Engine(int argc, char const* argv[], std::string const & instance);

      //! Destructor
      ~Engine();

      //! Re-load video mappings from videomappings.cfg
      /*! Mappings are automatically loaded on startup so this should only be used if the file has been modified and the
          mappings need to be refreshed. Note that this will not refresh the available resolutions for USB output, which
          requires a full reboot to re-initialize the kernel Gadget module. Also beware of possible state inconsistency
          (e.g., if external code is holding a reference previously returned by findVideoMapping(). So, use with
          caution. Basically, only GUIhelper should use this. */
      void reloadVideoMappings();
      
      //! Find the VideoMapping that has the given output specs, or throw if not found
      VideoMapping const & findVideoMapping(unsigned int oformat, unsigned int owidth, unsigned int oheight,
                                            float oframespersec) const;

      //! Get the current video mapping
      /*! Note that the current mapping may not have an entry in our list of mappings obtained from videomappings.cfg,
          if the current one was set on the fly by the setmapping2 CLI command. */
      VideoMapping const & getCurrentVideoMapping() const;

      //! Return the number of video mappings
      size_t numVideoMappings() const;

      //! Allow access to our video mappings which are parsed from file at construction
      VideoMapping const & getVideoMapping(size_t idx) const;

      //! Get the video mapping index for a given UVC iformat, iframe and interval
      size_t getVideoMappingIdx(unsigned int iformat, unsigned int iframe, unsigned int interval) const;

      //! Allow access to the default video mapping
      VideoMapping const & getDefaultVideoMapping() const;

      //! Allow access to the default video mapping index
      size_t getDefaultVideoMappingIdx() const;

      //! Run a function on every video mapping
      /*! The first mapping your function will be called on is for mapping with index 0, and so on until index
          numVideoMappings()-1. If your function throws, we report the exception and then ignore it, then we move on to
          the next mapping. */
      void foreachVideoMapping(std::function<void(VideoMapping const & m)> && func);

      //! Use this to request a format change from within process()
      /*! This should only be used on JeVois-Pro in GUI mode. The engine is locked up hence and setFormat() cannot be
          called from within a Module's process function, to avoid possible disasters of changing format while we
          process. Modules or the GUI can use requestSetFormat() to request a format change in between two calls to
          process(). Note special values: -1 to just reload the current format (e.g., after editing code), -2 does
          nothing. */
      void requestSetFormat(int idx);

      //! Terminate the program
      void quit();

      //! Request a reboot
      /*! On JeVois-A33 Platform, trigger a hard reset. On JeVois-Pro Platform or JeVois-Host, just terminate the
          program. */
      void reboot();
      
      //! Callback for when the user selects a new output video format
      /*! Here, we stop streaming, nuke any current processing module, set the camera format, set the gadget output
          format, load the new processing module, and start streaming again. The given VideoMapping will typically be
          obtained using findVideoMapping() from output specs received over the USB link. */
      void setFormat(size_t idx);

      //! Start streaming on video from camera, processing, and USB
      void streamOn();

      //! Stop streaming on video from camera, processing, and USB
      void streamOff();
      
      //! Main loop: grab, process, send over USB. Should be called by main application thread
      int mainLoop();

      //! Send a string to all serial ports
      /*! \note When islog is true, this is assumes to be a log message, and it will be sent to the port(s) specified by
          parameter serlog. Otherwise, the message will be sent to the ports specified by parameter serout. Note how the
          number of messages that can be sent for each video frame may be limited by parameter \p serlimit; only up to
          \p serlimit messages will be sent for a given video frame. This is useful to avoid overloading the serial
          link, for example in cases one is running a ArUco detector and a large number of ArUco tags are present in the
          field of view of JeVois. */
      void sendSerial(std::string const & str, bool islog = false);
      
      //! Retrieve a pointer to the hardware serial interface
      /*! \note  Returns nullptr if none is available */
      Serial* getHardwareSerial();

      //! Get a pointer to our current module (may be null)
      std::shared_ptr<Module> module() const;

      //! Get a pointer to our IMU (may be null)
      std::shared_ptr<IMU> imu() const;

      //! Get a pointer to our Camera (may be null, especially if not using a camera but, eg, movie input)
      std::shared_ptr<Camera> camera() const;
     
#ifdef JEVOIS_PRO
      //! Draw all camera controls into our GUI
      void drawCameraGUI();
#endif

      //! Helper to load an OpenCV camera matrix and distortion coeffs for the current running module
      /*! If do_throw is false, just report an error and provide identity defaults if calibration mot found. This would
          typically be called in preInit() or postInit() of any module that will need the calibration parameters. Note:
          for thread safety, engine should be locked; calling this inside Module::process() is safe. */
      CameraCalibration loadCameraCalibration(std::string const & stem = "calibration", bool do_throw = false);

      //! Helper to save an OpenCV camera matrix and distortion coeffs for the current running module
      /*! Note: for thread safety, engine should be locked; calling this inside Module::process() is safe. */
      void saveCameraCalibration(CameraCalibration const & calib, std::string const & stem = "calibration");
      
      //! Register a component as linked to some python code, used by dynamic params created in python
      /*! Use with extreme caution to guarantee thread safety and object lifetime since we just use raw pointers here */
      void registerPythonComponent(Component * comp, void * pyinst);

      //! Unregister a component as linked to some python code, used by dynamic params created in python
      /*! Use with extreme caution to guarantee thread safety and object lifetime since we just use raw pointers here */
      void unRegisterPythonComponent(Component * comp);

      //! Get the component registered with a given python instance
      /*! Use with extreme caution to guarantee thread safety and object lifetime since we just use raw pointers here */
      Component * getPythonComponent(void * pyinst) const;
      
      // Report an error to console and JeVois-Pro GUI
      /*! Try to minimize the use of this function, and normally use LERROR() or LFATAL() instead. Currently the only
          use is in jevois::dnn::Pipeline, to report parameters set in the zoo file but not used by the pipeline, as
          issuing an LFATAL() for that may be too strict, but issuing an LERROR() may go un-noticed since the pipeline
          is still running just fine. */
      void reportError(std::string const & err);

      // Report an info message to console and JeVois-Pro GUI
      /*! Try to minimize the use of this function, and normally use LINFO() instead. This is mainly to report info
          messages that may go unnoticed in the JeVois-Pro GUI if one is not looking at the console tab, e.g., notices
          that a file was loaded or saved. */
      void reportInfo(std::string const & info);

      //! Clear all errors currently displayed in the JeVois-Pro GUI
      /*! In the JevoisPro GUI, errors reported via reportError() remain displayed for a few seconds, but sometimes we
          want to clear them right away, e.g., after DNN pipeline threw, if the user selects another one, we want the
          previous error to disappear immediately since it is not applicable anymore. When the JeVois-Pro GUI is not
          used, this has no effect. */
      void clearErrors();

#ifdef JEVOIS_PRO
      //! When in demo mode, switch to next demo
      void nextDemo();

      //! When in demo mode, abort demo mode
      void abortDemo();

      //! Set OpenCV parallel threading framework
      /*! The default OpenCV threading uses all cores, which may sometimes give slow results if an image operation is
          split across big and little cores, as we will end up waiting for the little cores to finish. However, using
          all cores may be beneficial for deep nets. Hence, current polity is to use JeVois Big threadpool unless one
          wants to use the default OpenCV threadpool by calling this function, which NetworkOpenCV does. Valid values
          here are "jevois" (to only use big A73 cores and the JeVois ThreadPool), "tbb", "openmp", "pthreads" */
      void setOpenCVthreading(char const * name = "jevois");
#endif
      
    protected:
      //! Run a script from file
      /*! The filename should be absolute. The file should have any of the commands supported by Engine, one per
          line. Filename should be relative to the current module's path. */
      void runScriptFromFile(std::string const & filename, std::shared_ptr<UserInterface> ser,
                             bool throw_no_file);
      
      //! Parameter callback
      void onParamChange(engine::serialdev const & param, std::string const & newval) override;
      
      //! Parameter callback
      void onParamChange(engine::usbserialdev const & param, std::string const & newval) override;

      //! Parameter callback
      void onParamChange(engine::cpumode const & param, engine::CPUmode const & newval) override;

      //! Parameter callback
      void onParamChange(engine::cpumax const & param, unsigned int const & newval) override;

      //! Parameter callback
      void onParamChange(engine::videoerrors const & param, bool const & newval) override;

#ifdef JEVOIS_PRO
      //! Parameter callback
      void onParamChange(engine::gui const & param, bool const & newval) override;

      //! Parameter callback
      void onParamChange(engine::cpumaxl const & param, unsigned int const & newval) override;

      //! Parameter callback
      void onParamChange(engine::cpumodel const & param, engine::CPUmode const & newval) override;

      //! Parameter callback
      void onParamChange(engine::demomode const & param, float const & newval) override;
#endif
      
      size_t itsDefaultMappingIdx; //!< Index of default mapping
      std::vector<VideoMapping> itsMappings; //!< All our mappings from videomappings.cfg
      VideoMapping itsCurrentMapping; //!< Current mapping, may not match any in itsMappings if setmapping2 used

      std::shared_ptr<VideoInput> itsCamera; //!< Our camera
      std::shared_ptr<IMU> itsIMU; //! Our IMU
      std::shared_ptr<VideoOutput> itsGadget; //!< Our gadget

      std::unique_ptr<DynamicLoader> itsLoader; //!< Our module loader
      std::shared_ptr<Module> itsModule; //!< Our current module
      
      std::atomic<bool> itsRunning; //!< True when we are running
      std::atomic<bool> itsStreaming; //!< True when we are streaming video
      std::atomic<bool> itsStopMainLoop; //!< Flag used to stop the main loop

      mutable std::timed_mutex itsMtx; //!< Mutex to protect our internals

      void preInit() override; //!< Override of Manager::preInit()
      void postInit() override; //!< Override of Manager::postInit()

      //! Parse a user command received over serial port
      /*! Throw upon receiving an incorrect command (eg, bad parameter value), return true if success, return false if
          command was not recognized and should be tried by Module. pfx is an optional prefix which will be added to all
          produced messages or errors. */
      bool parseCommand(std::string const & str, std::shared_ptr<UserInterface> s, std::string const & pfx = "");
      
    private:
      std::list<std::shared_ptr<UserInterface> > itsSerials;
      
      void setFormatInternal(size_t idx); // itsMtx should be locked by caller
      void setFormatInternal(jevois::VideoMapping const & m, bool reload = false); // itsMtx should be locked by caller

      // Loop over all available camera controls and run a function on each:
      void foreachCamCtrl(std::function<void(struct v4l2_queryctrl & qc, std::set<int> & doneids)> && func);
      
      // Return help string for a camera control or throw
      std::string camCtrlHelp(struct v4l2_queryctrl & qc, std::set<int> & doneids);

      // Return machine-oriented string for a camera control or throw
      std::string camCtrlInfo(struct v4l2_queryctrl & qc, std::set<int> & doneids);

      // Send info about built-in engine commands
      void cmdInfo(std::shared_ptr<UserInterface> s, bool showAll, std::string const & pfx = "");
      
      // Send info about module commands
      void modCmdInfo(std::shared_ptr<UserInterface> s, std::string const & pfx = "");

      // Get short name from V4L2 ID, long name is a backup in case we don't find the control in our list
      std::string camctrlname(unsigned int id, char const * longname) const;
      
      // Get V4L2 ID from short name
      unsigned int camctrlid(std::string const & shortname);

      // Report an error to console, video frame, or GUI
      /*! Call this from within catch. Note, in GUI mode, this calls endFrame() so it should not be used except for
          exceptions that will not be ignored. */
      void reportErrorInternal(std::string const & err = "");

      bool itsShellMode; // When true, pass any CLI command to the Linux shell
      bool itsTurbo;
      bool itsManualStreamon; // allow manual streamon when outputing video to None or file
      std::atomic<bool> itsVideoErrors; // fast cached value for engine::videoerrors
      jevois::RawImage itsVideoErrorImage;
      std::string itsModuleConstructionError; // Non-empty error message if module constructor threw
      
#ifdef JEVOIS_PLATFORM_A33
      // Things related to mass storage gadget to export our /jevois partition as a virtual USB flash drive:
      void checkMassStorage(); // thread to check mass storage gadget status
      std::future<void> itsCheckMassStorageFut;
      std::atomic<bool> itsCheckingMassStorage;
      std::atomic<bool> itsMassStorageMode;
      void startMassStorageMode();
      void stopMassStorageMode();
#endif

      std::atomic<size_t> itsNumSerialSent; // Number of serial messages sent this frame; see serlimit
      std::atomic<int> itsRequestedFormat; // Set by requestSetFormat(), could be -1 to reload, otherwise -2
      
#ifdef JEVOIS_PRO
      std::shared_ptr<GUIhelper> itsGUIhelper;
      
      // Draw ImGui widgets for all camera controls
      void camCtrlGUI(struct v4l2_queryctrl & qc, std::set<int> & doneids);

      std::shared_ptr<jevois::Watchdog> itsWatchdog;

      bool itsDemoReset = true; // Restart the demo
      void runDemoStep(); // run one step of the demo mode
      struct DemoData
      {
          int mapping_idx;
          std::string title;
          std::string msg;
          std::vector<std::pair<std::string /* param name */, std::string /* param val */>> params;
      };
      std::vector<DemoData> itsDemoData;
      bool itsNextDemoRequested = false;
#endif
      
      // Python code registry, used to assign dynamic parameters created in python code to the correct owning component.
      // This looks thread-unsafe but should be ok as long as objects inherit first from Component and then from
      // PythonWrapper.
      std::map<void *, Component *> itsPythonRegistry;
      mutable std::mutex itsPyRegMtx;

#ifdef JEVOIS_PRO
      // Custom threading for OpenCV on JeVois-Pro
      std::shared_ptr<cv::parallel::ParallelForAPI> itsOpenCVparallelAPI;
#endif
  };
} // namespace jevois
  
