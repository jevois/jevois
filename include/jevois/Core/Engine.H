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
#include <jevois/Component/Manager.H>
#include <jevois/Types/Enum.H>
#include <jevois/Image/RawImage.H>

#include <memory>
#include <mutex>
#include <vector>
#include <list>
#include <atomic>

#ifdef JEVOIS_PLATFORM
// On the platform (JeVois hardware), we use a gadget driver by default to send output frames over USB, one hardware
// serial driver, and one serial-over-USB driver:

//! On platform hardware, device for the camera sensor
#define JEVOIS_CAMERA_DEFAULT "/dev/video0"

//! On platform hardware, device for the USB gadget driver (which sends video frames over USB to a host computer)
#define JEVOIS_GADGET_DEFAULT "/dev/video1"

//! On platform hardware, device for the 4-pin hardware serial port
#define JEVOIS_SERIAL_DEFAULT "/dev/ttyS0"

//! On platform hardware, device for serial-over-USB port
#define JEVOIS_USBSERIAL_DEFAULT "/dev/ttyGS0"

#else
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

#endif

namespace jevois
{
  class VideoInput;
  class VideoOutput;
  class Module;
  class DynamicLoader;
  class UserInterface;
  
  namespace engine
  {
    static ParameterCategory const ParamCateg("Engine Options");

    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER(cameradev, std::string, "Camera device name (if starting with /dev/v...), or movie "
                             "file name (e.g., movie.mpg) or image sequence (e.g., im%02d.jpg, to read frames "
                             "im00.jpg, im01.jpg, etc).",
                             JEVOIS_CAMERA_DEFAULT, ParamCateg);

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
    JEVOIS_DECLARE_PARAMETER(videomapping, int, "Index of Video Mapping to use, or -1 to use the default mapping",
                             -1, ParamCateg);

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
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(cpumode, CPUmode, "CPU frequency modulation mode",
                                           CPUmode::Performance, CPUmode_Values, ParamCateg);

    // keep this in sync with sunxi-cpufreq.c
    //! Parameter \relates jevois::Engine
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(cpumax, unsigned int, "CPU maximum frequency in MHz",
                                           1344, { 120, 240, 312, 408, 480, 504, 600, 648, 720, 816, 912, 1008,
                                               1044, 1056, 1080, 1104, 1116, 1152, 1200, 1224, 1248, 1296, 1344 },
                                           ParamCateg);

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
                 public Parameter<engine::cameradev, engine::cameranbuf, engine::gadgetdev, engine::gadgetnbuf,
                                  engine::videomapping, engine::serialdev, engine::usbserialdev, engine::camreg,
                                  engine::camturbo, engine::serlog, engine::videoerrors, engine::serout,
                                  engine::cpumode, engine::cpumax, engine::multicam, engine::quietcmd,
				  engine::python>
  {
    public:
      //! Constructor
      Engine(std::string const & instance);

      //! Constructor with command-line parsing
      Engine(int argc, char const* argv[], std::string const & instance);

      //! Destructor
      ~Engine();

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
      void mainLoop();

      //! Send a string to all serial ports
      /*! \note When islog is true, this is assumes to be a log message, and it will be sent to the port(s) specified by
          parameter serlog. Otherwise, the message will be sent to the ports specified by parameter serout. */
      void sendSerial(std::string const & str, bool islog = false);

    protected:
      //! Run a script from file
      /*! The filename should be absolute. The file should have any of the commands supported by Engine, one per
          line. Filename should be relative to the current module's path. */
      void runScriptFromFile(std::string const & filename, std::shared_ptr<UserInterface> ser,
                             bool throw_no_file);
      
      //! Parameter callback
      void onParamChange(engine::cameradev const & param, std::string const & newval);
      
      //! Parameter callback
      void onParamChange(engine::gadgetdev const & param, std::string const & newval);
      
      //! Parameter callback
      void onParamChange(engine::serialdev const & param, std::string const & newval);
      
      //! Parameter callback
      void onParamChange(engine::usbserialdev const & param, std::string const & newval);

      //! Parameter callback
      void onParamChange(engine::cpumode const & param, engine::CPUmode const & newval);

      //! Parameter callback
      void onParamChange(engine::cpumax const & param, unsigned int const & newval);

      //! Parameter callback
      void onParamChange(engine::videoerrors const & param, bool const & newval);

      size_t itsDefaultMappingIdx; //!< Index of default mapping
      std::vector<VideoMapping> const itsMappings; //!< All our mappings from videomappings.cfg
      VideoMapping itsCurrentMapping; //!< Current video mapping, may not match any in itsMappings if setmapping2 used

      std::shared_ptr<VideoInput> itsCamera; //!< Our camera
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
          command was not recognized and should be tried by Module. */
      bool parseCommand(std::string const & str, std::shared_ptr<UserInterface> s);
      
    private:
      std::list<std::shared_ptr<UserInterface> > itsSerials;
      
      void setFormatInternal(size_t idx); // itsMtx should be locked by caller
      void setFormatInternal(jevois::VideoMapping const & m); // itsMtx should be locked by caller
      
      // Return help string for a camera control or throw
      std::string camCtrlHelp(struct v4l2_queryctrl & qc, std::set<int> & doneids);
      
      // Get short name from V4L2 ID, long name is a backup in case we don't find the control in our list
      std::string camctrlname(int id, char const * longname) const;
      
      // Get V4L2 ID from short name
      int camctrlid(std::string const & shortname);
      
      bool itsTurbo;
      bool itsManualStreamon; // allow manual streamon when outputing video to None or file
      std::atomic<bool> itsVideoErrors; // fast cached value for engine::videoerrors
      jevois::RawImage itsVideoErrorImage;
      std::string itsModuleConstructionError; // Non-empty error message if module constructor threw
      
#ifdef JEVOIS_PLATFORM
      // Things related to mass storage gadget to export our /jevois partition as a virtual USB flash drive:
      void checkMassStorage(); // thread to check mass storage gadget status
      std::future<void> itsCheckMassStorageFut;
      std::atomic<bool> itsCheckingMassStorage;
      std::atomic<bool> itsMassStorageMode;
      void startMassStorageMode();
      void stopMassStorageMode();
      void reboot();
#endif
  };

} // namespace jevois
  
