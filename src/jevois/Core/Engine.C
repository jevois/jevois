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

#include <jevois/Core/Engine.H>

#include <jevois/Core/Camera.H>
#include <jevois/Core/MovieInput.H>

#include <jevois/Core/Gadget.H>
#include <jevois/Core/VideoDisplay.H>
#include <jevois/Core/VideoOutputNone.H>
#include <jevois/Core/MovieOutput.H>

#include <jevois/Core/Serial.H>
#include <jevois/Core/StdioInterface.H>

#include <jevois/Core/Module.H>
#include <jevois/Core/DynamicLoader.H>
#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/PythonModule.H>

#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Debug/SysInfo.H>

#include <cmath> // for fabs
#include <fstream>
#include <algorithm>
#include <cstdlib> // for std::system()
#include <cstdio> // for std::remove()

// On the older platform kernel, detect class is not defined:
#ifndef V4L2_CTRL_CLASS_DETECT
#define V4L2_CTRL_CLASS_DETECT          0x00a30000
#endif

namespace
{
  // Assign a short name to every V4L2 control, for use by getcam and setcam commands
  struct shortcontrol { int id; char const * const shortname; };

  // All V4L2 controls
  // From this: grep V4L2_CID v4l2-controls.h | awk '{ print "    { " $2 ", \"\" }," }'
  // then fill-in the short names.
  static shortcontrol camcontrols[] = {
    // In V4L2_CID_BASE class:
    { V4L2_CID_BRIGHTNESS, "brightness" },
    { V4L2_CID_CONTRAST, "contrast" },
    { V4L2_CID_SATURATION, "saturation" },
    { V4L2_CID_HUE, "hue" },
    { V4L2_CID_AUDIO_VOLUME, "audiovol" },
    { V4L2_CID_AUDIO_BALANCE, "audiobal" },
    { V4L2_CID_AUDIO_BASS, "audiobass" },
    { V4L2_CID_AUDIO_TREBLE, "audiotreble" },
    { V4L2_CID_AUDIO_MUTE, "audiomute" },
    { V4L2_CID_AUDIO_LOUDNESS, "audioloudness" },
    { V4L2_CID_BLACK_LEVEL, "blacklevel" },
    { V4L2_CID_AUTO_WHITE_BALANCE, "autowb" },
    { V4L2_CID_DO_WHITE_BALANCE, "dowb" },
    { V4L2_CID_RED_BALANCE, "redbal" },
    { V4L2_CID_BLUE_BALANCE, "bluebal" },
    { V4L2_CID_GAMMA, "gamma" },
    { V4L2_CID_WHITENESS, "whiteness" },
    { V4L2_CID_EXPOSURE, "exposure" },
    { V4L2_CID_AUTOGAIN, "autogain" },
    { V4L2_CID_GAIN, "gain" },
    { V4L2_CID_HFLIP, "hflip" },
    { V4L2_CID_VFLIP, "vflip" },
    { V4L2_CID_POWER_LINE_FREQUENCY, "powerfreq" },
    { V4L2_CID_HUE_AUTO, "autohue" },
    { V4L2_CID_WHITE_BALANCE_TEMPERATURE, "wbtemp" },
    { V4L2_CID_SHARPNESS, "sharpness" },
    { V4L2_CID_BACKLIGHT_COMPENSATION, "backlight" },
    { V4L2_CID_CHROMA_AGC, "chromaagc" },
    { V4L2_CID_COLOR_KILLER, "colorkiller" },
    { V4L2_CID_COLORFX, "colorfx" },
    { V4L2_CID_AUTOBRIGHTNESS, "autobrightness" },
    { V4L2_CID_BAND_STOP_FILTER, "bandfilter" },
    { V4L2_CID_ROTATE, "rotate" },
    { V4L2_CID_BG_COLOR, "bgcolor" },
    { V4L2_CID_CHROMA_GAIN, "chromagain" },
    { V4L2_CID_ILLUMINATORS_1, "illum1" },
    { V4L2_CID_ILLUMINATORS_2, "illum2" },
    { V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, "mincapbuf" },
    { V4L2_CID_MIN_BUFFERS_FOR_OUTPUT, "minoutbuf" },
    { V4L2_CID_ALPHA_COMPONENT, "alphacompo" },
    // This one is not defined in our older platform kernel:
    //{ V4L2_CID_COLORFX_CBCR, "colorfxcbcr" },

    // In V4L2_CID_CAMERA_CLASS_BASE class
    { V4L2_CID_EXPOSURE_AUTO, "autoexp" },
    { V4L2_CID_EXPOSURE_ABSOLUTE, "absexp" },
    { V4L2_CID_EXPOSURE_AUTO_PRIORITY, "exppri" },
    { V4L2_CID_PAN_RELATIVE, "panrel" },
    { V4L2_CID_TILT_RELATIVE, "tiltrel" },
    { V4L2_CID_PAN_RESET, "panreset" },
    { V4L2_CID_TILT_RESET, "tiltreset" },
    { V4L2_CID_PAN_ABSOLUTE, "panabs" },
    { V4L2_CID_TILT_ABSOLUTE, "tiltabs" },
    { V4L2_CID_FOCUS_ABSOLUTE, "focusabs" },
    { V4L2_CID_FOCUS_RELATIVE, "focusrel" },
    { V4L2_CID_FOCUS_AUTO, "focusauto" },
    { V4L2_CID_ZOOM_ABSOLUTE, "zoomabs" },
    { V4L2_CID_ZOOM_RELATIVE, "zoomrel" },
    { V4L2_CID_ZOOM_CONTINUOUS, "zoomcontinuous" },
    { V4L2_CID_PRIVACY, "privacy" },
    { V4L2_CID_IRIS_ABSOLUTE, "irisabs" },
    { V4L2_CID_IRIS_RELATIVE, "irisrel" },

    // definition for this one seems to be in the kernel but missing somehow here:
#ifndef V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE
#define V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE	(V4L2_CID_CAMERA_CLASS_BASE+20)
#endif
    { V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE, "presetwb" },

    // Those are not defined in our older platform kernel:
    //{ V4L2_CID_AUTO_EXPOSURE_BIAS, "expbias" },
    //{ V4L2_CID_WIDE_DYNAMIC_RANGE, "wdr" },
    //{ V4L2_CID_IMAGE_STABILIZATION, "stabilization" },
    //{ V4L2_CID_ISO_SENSITIVITY, "isosens" },
    //{ V4L2_CID_ISO_SENSITIVITY_AUTO, "isosensauto" },
    //{ V4L2_CID_EXPOSURE_METERING, "expmetering" },
    //{ V4L2_CID_SCENE_MODE, "scene" },
    //{ V4L2_CID_3A_LOCK, "3alock" },
    //{ V4L2_CID_AUTO_FOCUS_START, "autofocusstart" },
    //{ V4L2_CID_AUTO_FOCUS_STOP, "autofocusstop" },
    //{ V4L2_CID_AUTO_FOCUS_STATUS, "autofocusstatus" },
    //{ V4L2_CID_AUTO_FOCUS_RANGE, "autofocusrange" },
    //{ V4L2_CID_PAN_SPEED, "panspeed" },
    //{ V4L2_CID_TILT_SPEED, "tiltspeed" },

    // In V4L2_CID_FLASH_CLASS_BASE:
    { V4L2_CID_FLASH_LED_MODE, "flashled" },
    { V4L2_CID_FLASH_STROBE_SOURCE, "flashstrobesrc" },
    { V4L2_CID_FLASH_STROBE, "flashstrobe" },
    { V4L2_CID_FLASH_STROBE_STOP, "flashstrobestop" },
    { V4L2_CID_FLASH_STROBE_STATUS, "flashstrovestat" },
    { V4L2_CID_FLASH_TIMEOUT, "flashtimeout" },
    { V4L2_CID_FLASH_INTENSITY, "flashintens" },
    { V4L2_CID_FLASH_TORCH_INTENSITY, "flashtorch" },
    { V4L2_CID_FLASH_INDICATOR_INTENSITY, "flashindintens" },
    { V4L2_CID_FLASH_FAULT, "flashfault" },
    { V4L2_CID_FLASH_CHARGE, "flashcharge" },
    { V4L2_CID_FLASH_READY, "flashready" },

    // In V4L2_CID_JPEG_CLASS_BASE:
    { V4L2_CID_JPEG_CHROMA_SUBSAMPLING, "jpegchroma" },
    { V4L2_CID_JPEG_RESTART_INTERVAL, "jpegrestartint" },
    { V4L2_CID_JPEG_COMPRESSION_QUALITY, "jpegcompression" },
    { V4L2_CID_JPEG_ACTIVE_MARKER, "jpegmarker" },

    // In V4L2_CID_IMAGE_SOURCE_CLASS_BASE:
    // Those are not defined in our older platform kernel:
    //{ V4L2_CID_VBLANK, "vblank" },
    //{ V4L2_CID_HBLANK, "hblank" },
    //{ V4L2_CID_ANALOGUE_GAIN, "again" },
    //{ V4L2_CID_TEST_PATTERN_RED, "testpatred" },
    //{ V4L2_CID_TEST_PATTERN_GREENR, "testpatgreenr" },
    //{ V4L2_CID_TEST_PATTERN_BLUE, "testpatblue" },
    //{ V4L2_CID_TEST_PATTERN_GREENB, "testpatbreenb" },

    // In V4L2_CID_IMAGE_PROC_CLASS_BASE:
    //{ V4L2_CID_LINK_FREQ, "linkfreq" },
    //{ V4L2_CID_PIXEL_RATE, "pixrate" },
    //{ V4L2_CID_TEST_PATTERN, "testpat" },

    // In V4L2_CID_DETECT_CLASS_BASE:
    //{ V4L2_CID_DETECT_MD_MODE, "detectmode" },
    //{ V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD, "detectthresh" },
    //{ V4L2_CID_DETECT_MD_THRESHOLD_GRID, "detectthreshgrid" },
    //{ V4L2_CID_DETECT_MD_REGION_GRID, "detectregiongrid" },
  };
  
  // Convert a long name to a short name:
  std::string abbreviate(std::string const & longname)
  {
    std::string name(longname);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    name.erase(std::remove_if(name.begin(), name.end(), [](int c) { return !std::isalnum(c); }), name.end());
    return name;
  }
} // anonymous namespace


// ####################################################################################################
jevois::Engine::Engine(std::string const & instance) :
    jevois::Manager(instance), itsMappings(jevois::loadVideoMappings(itsDefaultMappingIdx)),
    itsRunning(false), itsStreaming(false), itsStopMainLoop(false), itsTurbo(false),
    itsManualStreamon(false), itsVideoErrors(false)
{
  JEVOIS_TRACE(1);

  LINFO("Loaded " << itsMappings.size() << " vision processing modes.");

#ifdef JEVOIS_PLATFORM
  // Start mass storage thread:
  itsCheckingMassStorage.store(false); itsMassStorageMode.store(false);
  itsCheckMassStorageFut = std::async(std::launch::async, &jevois::Engine::checkMassStorage, this);
  while (itsCheckingMassStorage.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
}

// ####################################################################################################
jevois::Engine::Engine(int argc, char const* argv[], std::string const & instance) :
    jevois::Manager(argc, argv, instance), itsMappings(jevois::loadVideoMappings(itsDefaultMappingIdx)),
    itsRunning(false), itsStreaming(false), itsStopMainLoop(false), itsTurbo(false),
    itsManualStreamon(false), itsVideoErrors(false)
{
  JEVOIS_TRACE(1);

  LINFO("Loaded " << itsMappings.size() << " vision processing modes.");

#ifdef JEVOIS_PLATFORM
  // Start mass storage thread:
  itsCheckingMassStorage.store(false); itsMassStorageMode.store(false);
  itsCheckMassStorageFut = std::async(std::launch::async, &jevois::Engine::checkMassStorage, this);
  while (itsCheckingMassStorage.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif
}

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::serialdev const & JEVOIS_UNUSED_PARAM(param),
                                   std::string const & newval)
{
  JEVOIS_TIMED_LOCK(itsMtx);

  // If we have a serial already, nuke it:
  for (std::list<std::shared_ptr<UserInterface> >::iterator itr = itsSerials.begin(); itr != itsSerials.end(); ++itr)
    if ((*itr)->instanceName() == "serial") itr = itsSerials.erase(itr);
  removeComponent("serial", false);
  
  // Open the usb hardware (4-pin connector) serial port, if any:
  if (newval.empty() == false)
    try
    {
      std::shared_ptr<jevois::UserInterface> s;
      if (newval == "stdio")
        s = addComponent<jevois::StdioInterface>("serial");
      else
      {
        s = addComponent<jevois::Serial>("serial", jevois::UserInterface::Type::Hard);
        s->setParamVal("devname", newval);
      }
      
      itsSerials.push_back(s);
      LINFO("Using [" << newval << "] hardware (4-pin connector) serial port");
    }
    catch (...) { jevois::warnAndIgnoreException(); LERROR("Could not start hardware (4-pin connector) serial port"); }
  else LINFO("No hardware (4-pin connector) serial port used");
}

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::usbserialdev const & JEVOIS_UNUSED_PARAM(param),
                                   std::string const & newval)
{
  JEVOIS_TIMED_LOCK(itsMtx);

  // If we have a usbserial already, nuke it:
  for (std::list<std::shared_ptr<UserInterface> >::iterator itr = itsSerials.begin(); itr != itsSerials.end(); ++itr)
    if ((*itr)->instanceName() == "usbserial") itr = itsSerials.erase(itr);
  removeComponent("usbserial", false);

  // Open the USB serial port, if any:
  if (newval.empty() == false)
    try
    {
      std::shared_ptr<jevois::UserInterface> s =
        addComponent<jevois::Serial>("usbserial", jevois::UserInterface::Type::USB);
      s->setParamVal("devname", newval);
      itsSerials.push_back(s);
      LINFO("Using [" << newval << "] USB serial port");
    }
    catch (...) { jevois::warnAndIgnoreException(); LERROR("Could not start USB serial port"); }
  else LINFO("No USB serial port used");
}

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::cpumode const & JEVOIS_UNUSED_PARAM(param),
                                   jevois::engine::CPUmode const & newval)
{
  std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
  if (ofs.is_open() == false)
  {
#ifdef JEVOIS_PLATFORM
    LERROR("Cannot set cpu frequency governor mode -- IGNORED");
#endif
    return;
  }

  switch (newval)
  {
  case engine::CPUmode::PowerSave: ofs << "powersave" << std::endl; break;
  case engine::CPUmode::Conservative: ofs << "conservative" << std::endl; break;
  case engine::CPUmode::OnDemand: ofs << "ondemand" << std::endl; break;
  case engine::CPUmode::Interactive: ofs << "interactive" << std::endl; break;
  case engine::CPUmode::Performance: ofs << "performance" << std::endl; break;
  }
}

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::cpumax const & JEVOIS_UNUSED_PARAM(param),
                                   unsigned int const & newval)
{
  std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
  if (ofs.is_open() == false)
  {
#ifdef JEVOIS_PLATFORM
    LERROR("Cannot set cpu max frequency -- IGNORED");
#endif
    return;
  }

  ofs << newval * 1000U << std::endl;
}

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::videoerrors const & JEVOIS_UNUSED_PARAM(param),
                                   bool const & newval)
{
  itsVideoErrors.store(newval);
}

// ####################################################################################################
void jevois::Engine::preInit()
{
  // Set any initial parameters from global config file:
  std::string const paramcfg = std::string(JEVOIS_CONFIG_PATH) + '/' + JEVOIS_MODULE_PARAMS_FILENAME;
  std::ifstream ifs(paramcfg); if (ifs.is_open()) setParamsFromStream(ifs, paramcfg);
  
  // Run the Manager version. This parses the command line:
  jevois::Manager::preInit();
}

// ####################################################################################################
void jevois::Engine::postInit()
{
  // First make sure the manager gets to run this:
  jevois::Manager::postInit();

  // Freeze the serial port device names, their params, and camera and gadget too:
  serialdev::freeze();
  usbserialdev::freeze();
  for (auto & s : itsSerials) s->freezeAllParams();
  cameradev::freeze();
  cameranbuf::freeze();
  camturbo::freeze();
  gadgetdev::freeze();
  gadgetnbuf::freeze();
  itsTurbo = camturbo::get();

  // Grab the log messages, itsSerials is not going to change anymore now that the serial params are frozen:
  jevois::logSetEngine(this);

  // Get python going, we need to do this here to avoid segfaults on platform when instantiating our first python
  // module. This likely has to do with the fact that the python core is not very thread-safe, and setFormatInternal()
  // in Engine, which instantiates python modules, will indeed be invoked from a different thread (the one that receives
  // USB UVC events). Have a look at Python Thread State, Python Gobal Interpreter Lock, etc if interested:
  LINFO("Initalizing Python...");
  jevois::pythonModuleSetEngine(this);
  
  // Instantiate a camera: If device names starts with "/dev/v", assume a hardware camera, otherwise a movie file:
  std::string const camdev = cameradev::get();
  if (jevois::stringStartsWith(camdev, "/dev/v"))
  {
    LINFO("Starting camera device " << camdev);
    
#ifdef JEVOIS_PLATFORM
    // Set turbo mode or not:
    std::ofstream ofs("/sys/module/vfe_v4l2/parameters/turbo");
    if (ofs.is_open())
    {
      if (itsTurbo) ofs << "1" << std::endl; else ofs << "0" << std::endl;
      ofs.close();
    }
    else LERROR("Could not access VFE turbo parameter -- IGNORED");
#endif
    
    // Now instantiate the camera:
    itsCamera.reset(new jevois::Camera(camdev, cameranbuf::get()));

#ifndef JEVOIS_PLATFORM
    // No need to confuse people with a non-working camreg param:
    camreg::set(false);
    camreg::freeze();
#endif
  }
  else
  {
    LINFO("Using movie input " << camdev << " -- issue a 'streamon' to start processing.");
    itsCamera.reset(new jevois::MovieInput(camdev, cameranbuf::get()));

    // No need to confuse people with a non-working camreg param:
    camreg::set(false);
    camreg::freeze();
  }
  
  // Instantiate a USB gadget: Note: it will want to access the mappings. If the user-selected video mapping has no usb
  // out, do not instantiate a gadget:
  int midx = videomapping::get();

  // The videomapping parameter is now disabled, users should use the 'setmapping' command once running:
  videomapping::freeze();
  
  if (midx >= int(itsMappings.size()))
  { LERROR("Mapping index " << midx << " out of range -- USING DEFAULT"); midx = -1; }

  if (midx < 0) midx = itsDefaultMappingIdx;

  // Always instantiate a gadget even if not used right now, may be used later:
  std::string const gd = gadgetdev::get();
  if (gd == "None")
  {
    LINFO("Using no USB video output.");
    // No USB output and no display, useful for benchmarking only:
    itsGadget.reset(new jevois::VideoOutputNone());
    itsManualStreamon = true;
  }
  else if (jevois::stringStartsWith(gd, "/dev/"))
  {
    LINFO("Loading USB video driver " << gd);
    // USB gadget driver:
    itsGadget.reset(new jevois::Gadget(gd, itsCamera.get(), this, gadgetnbuf::get()));
  }
  else if (gd.empty() == false)
  {
    LINFO("Saving output video to file " << gd);
    // Non-empty filename, save to file:
    itsGadget.reset(new jevois::MovieOutput(gd));
    itsManualStreamon = true;
  }
  else
  {
    LINFO("Using display for video output");
    // Local video display, for use on a host desktop:
    itsGadget.reset(new jevois::VideoDisplay("jevois", gadgetnbuf::get()));
    itsManualStreamon = true;
  }
  
  // We are ready to run:
  itsRunning.store(true);

  // Set initial format:
  setFormat(midx);
  
  // Run init script:
  JEVOIS_TIMED_LOCK(itsMtx);
  runScriptFromFile(JEVOIS_ENGINE_INIT_SCRIPT, nullptr, false);
}

// ####################################################################################################
jevois::Engine::~Engine()
{
  JEVOIS_TRACE(1);

  // Turn off stream if it is on:
  streamOff();  

  // Tell our run() thread to finish up:
  itsRunning.store(false);
  
#ifdef JEVOIS_PLATFORM
  // Tell checkMassStorage() thread to finish up:
  itsCheckingMassStorage.store(false);
#endif
  
  // Nuke our module as soon as we can, hopefully soon now that we turned off streaming and running:
  {
    JEVOIS_TIMED_LOCK(itsMtx);
    removeComponent(itsModule);
    itsModule.reset();

    // Gone, nuke the loader now:
    itsLoader.reset();
  }
  
  // Because we passed the camera as a raw pointer to the gadget, nuke the gadget first and then the camera:
  itsGadget.reset();
  itsCamera.reset();

#ifdef JEVOIS_PLATFORM
  // Will block until the checkMassStorage() thread completes:
  if (itsCheckMassStorageFut.valid())
    try { itsCheckMassStorageFut.get(); } catch (...) { jevois::warnAndIgnoreException(); }
#endif
  
  // Things should be quiet now, unhook from the logger (this call is not strictly thread safe):
  jevois::logSetEngine(nullptr);
}

// ####################################################################################################
#ifdef JEVOIS_PLATFORM
void jevois::Engine::checkMassStorage()
{
  itsCheckingMassStorage.store(true);

  while (itsCheckingMassStorage.load())
  {
    // Check from the mass storage gadget (with JeVois extension) whether the virtual USB drive is mounted by the
    // host. If currently in mass storage mode and the host just ejected the virtual flash drive, resume normal
    // operation. If not in mass-storage mode and the host mounted it, enter mass-storage mode (may happen if
    // /boot/usbsdauto was selected):
    std::ifstream ifs("/sys/devices/platform/sunxi_usb_udc/gadget/lun0/mass_storage_in_use");
    if (ifs.is_open())
    {
        int inuse; ifs >> inuse;
        if (itsMassStorageMode.load())
        {
          if (inuse == 0) stopMassStorageMode();
        }
        else
        {
          if (inuse) { JEVOIS_TIMED_LOCK(itsMtx); startMassStorageMode(); }
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
#endif

// ####################################################################################################
void jevois::Engine::streamOn()
{
  JEVOIS_TRACE(2);

  JEVOIS_TIMED_LOCK(itsMtx);
  itsCamera->streamOn();
  itsGadget->streamOn();
  itsStreaming.store(true);
}

// ####################################################################################################
void jevois::Engine::streamOff()
{
  JEVOIS_TRACE(2);

  // First, tell both ethe camera and gadget to abort streaming, this will make get()/done()/send() to throw:
  itsGadget->abortStream();
  itsCamera->abortStream();

  // Stop the main loop, which will flit itsStreaming to false and will make it easier for us to lock itsMtx:
  LDEBUG("Stopping main loop...");
  itsStopMainLoop.store(true);
  while (itsStopMainLoop.load() && itsRunning.load()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  LDEBUG("Main loop stopped.");
  
  // Lock up and stream off:
  JEVOIS_TIMED_LOCK(itsMtx);
  itsGadget->streamOff();
  itsCamera->streamOff();
}

// ####################################################################################################
void jevois::Engine::setFormat(size_t idx)
{
  JEVOIS_TRACE(2);

  LDEBUG("Set format number " << idx << " start...");
  
  if (idx >= itsMappings.size())
    LFATAL("Requested mapping index " << idx << " out of range [0 .. " << itsMappings.size()-1 << ']');

  JEVOIS_TIMED_LOCK(itsMtx);
  setFormatInternal(idx);
  LDEBUG("Set format number " << idx << " done");
}

// ####################################################################################################
void jevois::Engine::setFormatInternal(size_t idx)
{
  // itsMtx should be locked by caller, idx should be valid:
  JEVOIS_TRACE(2);

  jevois::VideoMapping const & m = itsMappings[idx];
  setFormatInternal(m);
}

// ####################################################################################################
void jevois::Engine::setFormatInternal(jevois::VideoMapping const & m)
{
  // itsMtx should be locked by caller, idx should be valid:
  JEVOIS_TRACE(2);

  LINFO(m.str());

#ifdef JEVOIS_PLATFORM
  if (itsMassStorageMode.load())
    LFATAL("Cannot setup video streaming while in mass-storage mode. Eject the USB drive on your host computer first.");
#endif
  
  // Now that the module is nuked, we won't have any get()/done()/send() requests on the camera or gadget, thus it is
  // safe to change the formats on both:
  itsCamera->setFormat(m);
  if (m.ofmt) itsGadget->setFormat(m);

  // Keep track of our current mapping:
  itsCurrentMapping = m;
  
  // Nuke the processing module, if any, so we can also safely nuke the loader. We always nuke the module instance so we
  // won't have any issues with latent state even if we re-use the same module but possibly with different input
  // image resolution, etc:
  if (itsModule) { removeComponent(itsModule); itsModule.reset(); }

  // For python modules, we do not need a loader, we just instantiate our special python wrapper module instead:
  std::string const sopath = m.sopath();
  if (m.ispython)
  {
    // Instantiate the python wrapper:
    itsModule.reset(new jevois::PythonModule(m));
  }
  else
  {
    // C++ compiled module. We can re-use the same loader and avoid closing the .so if we will use the same module:
    if (itsLoader.get() == nullptr || itsLoader->sopath() != sopath)
    {
      // Nuke our previous loader and free its resources if needed, then start a new loader:
      LINFO("Instantiating dynamic loader for " << sopath);
      itsLoader.reset(new jevois::DynamicLoader(sopath, true));
    }
    
    // Check version match:
    auto version_major = itsLoader->load<int()>(m.modulename + "_version_major");
    auto version_minor = itsLoader->load<int()>(m.modulename + "_version_minor");
    if (version_major() != JEVOIS_VERSION_MAJOR || version_minor() != JEVOIS_VERSION_MINOR)
      LERROR("Module " << m.modulename << " in file " << sopath << " was build for JeVois v" << version_major() << '.'
             << version_minor() << ", but running framework is v" << JEVOIS_VERSION_STRING << " -- TRYING ANYWAY");
  
    // Instantiate the new module:
    auto create = itsLoader->load<std::shared_ptr<jevois::Module>(std::string const &)>(m.modulename + "_create");
    itsModule = create(m.modulename); // Here we just use the class name as instance name
  }
  
  // Add the module as a component to us. Keep this code in sync with Manager::addComponent():
  {
    // Lock up so we guarantee the instance name does not get robbed as we add the sub:
    boost::unique_lock<boost::shared_mutex> ulck(itsSubMtx);
    
    // Then add it as a sub-component to us:
    itsSubComponents.push_back(itsModule);
    itsModule->itsParent = this;
    itsModule->setPath(sopath.substr(0, sopath.rfind('/')));
  }
  
  // Bring it to our runstate and load any extra params. NOTE: Keep this in sync with Component::init():
  if (itsInitialized) itsModule->runPreInit();
  
  std::string const paramcfg = itsModule->absolutePath(JEVOIS_MODULE_PARAMS_FILENAME);
  std::ifstream ifs(paramcfg); if (ifs.is_open()) itsModule->setParamsFromStream(ifs, paramcfg);
  
  if (itsInitialized) { itsModule->setInitialized(); itsModule->runPostInit(); }

  // And finally run any config script:
  runScriptFromFile(itsModule->absolutePath(JEVOIS_MODULE_SCRIPT_FILENAME), nullptr, false);
  
  LINFO("Module [" << m.modulename << "] loaded, initialized, and ready.");
}

// ####################################################################################################
void jevois::Engine::mainLoop()
{
  JEVOIS_TRACE(2);

  // Announce that we are ready to the hardware serial port, if any. Do not use sendSerial() here so we always issue
  // this message irrespectively of the user serial preferences:
  for (auto & s : itsSerials)
    if (s->instanceName() == "serial")
      try { s->writeString("INF READY JEVOIS " JEVOIS_VERSION_STRING); }
      catch (...) { jevois::warnAndIgnoreException(); }
  
  while (itsRunning.load())
  {
    bool dosleep = true;

    if (itsStreaming.load())
    {
      JEVOIS_TIMED_LOCK(itsMtx);

      if (itsModule)
      {
        try
        {
          if (itsCurrentMapping.ofmt) // Process with USB outputs:
            itsModule->process(jevois::InputFrame(itsCamera, itsTurbo),
                               jevois::OutputFrame(itsGadget, itsVideoErrors.load() ? &itsVideoErrorImage : nullptr));
          else  // Process with no USB outputs:
            itsModule->process(jevois::InputFrame(itsCamera, itsTurbo));
          dosleep = false;
        }
        catch (...)
        {
          // Report exceptions to video if desired:
          if (itsVideoErrors.load())
          {
            // If the module threw before get() or after send() on the output frame, get a buffer from the gadget:
            if (itsVideoErrorImage.valid() == false) itsGadget->get(itsVideoErrorImage);
            
            // Report module exception to serlog and video, and ignore:
            jevois::warnAndIgnoreException(&itsVideoErrorImage);
            
            // Send the error image over USB:
            itsGadget->send(itsVideoErrorImage);
            
            // Invalidate the error image so it is clean for the next frame:
            itsVideoErrorImage.invalidate();
          }
          else
          {
            // Report module exception to serlog, and ignore:
            jevois::warnAndIgnoreException();
          }
        }
      }
    }
  
    if (itsStopMainLoop.load())
    {
      itsStreaming.store(false);
      LDEBUG("-- Main loop stopped --");
      itsStopMainLoop.store(false);
    }

    if (dosleep)
    {
      LDEBUG("No processing module loaded or not streaming... Sleeping...");
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Serial input handling. Note that readSome() and writeString() on the serial could throw. The code below is
    // organized to catch all other exceptions, except for those, which are caught here at the first try level:
    for (auto & s : itsSerials)
    {
      try
      {
        std::string str; bool parsed = false; bool success = false;
        
        if (s->readSome(str))
        {
          JEVOIS_TIMED_LOCK(itsMtx);

          // Try to execute this command. If the command is for us (e.g., set a parameter) and is correct,
          // parseCommand() will return true; if it is for us but buggy, it will throw. If it is not recognized by us,
          // it will return false and we should try sending it to the Module:
          try { parsed = parseCommand(str, s); success = parsed; }
          catch (std::exception const & e) { s->writeString(std::string("ERR ") + e.what()); parsed = true; }
          catch (...) { s->writeString("ERR Unknown error"); parsed = true; }

          if (parsed == false)
          {
            if (itsModule)
            {
              try { itsModule->parseSerial(str, s); success = true; }
              catch (std::exception const & me) { s->writeString(std::string("ERR ") + me.what()); }
              catch (...) { s->writeString("ERR Command [" + str + "] not recognized by Engine or Module"); }
            }
            else s->writeString("ERR Unsupported command [" + str + "] and no module");
          }
          
          // If success, let user know:
          if (success) s->writeString("OK");
        }
      }
      catch (...) { jevois::warnAndIgnoreException(); }
    }
  }
}

// ####################################################################################################
void jevois::Engine::sendSerial(std::string const & str, bool islog)
{
  // Decide where to send this message based on the value of islog:
  jevois::engine::SerPort p = islog ? serlog::get() : serout::get();

  switch (p)
  {
  case jevois::engine::SerPort::None:
    break; // Nothing to send

  case jevois::engine::SerPort::All:
    for (auto & s : itsSerials)
      try { s->writeString(str); } catch (...) { jevois::warnAndIgnoreException(); }
    break;

  case jevois::engine::SerPort::Hard:
    for (auto & s : itsSerials)
      if (s->type() == jevois::UserInterface::Type::Hard)
        try { s->writeString(str); } catch (...) { jevois::warnAndIgnoreException(); }
    break;

  case jevois::engine::SerPort::USB:
    for (auto & s : itsSerials)
      if (s->type() == jevois::UserInterface::Type::USB)
        try { s->writeString(str); } catch (...) { jevois::warnAndIgnoreException(); }
    break;
  }
}

// ####################################################################################################
jevois::VideoMapping const & jevois::Engine::getCurrentVideoMapping() const
{
  return itsCurrentMapping;
}

// ####################################################################################################
size_t jevois::Engine::numVideoMappings() const
{
  return itsMappings.size();
}

// ####################################################################################################
jevois::VideoMapping const & jevois::Engine::getVideoMapping(size_t idx) const
{
  if (idx >= itsMappings.size())
    LFATAL("Index " << idx << " out of range [0 .. " << itsMappings.size()-1 << ']');

  return itsMappings[idx];
}

// ####################################################################################################
size_t jevois::Engine::getVideoMappingIdx(unsigned int iformat, unsigned int iframe, unsigned int interval) const
{
  // If the iformat or iframe is zero, that's probably a probe for the default mode, so return it:
  if (iformat == 0 || iframe == 0) return itsDefaultMappingIdx;
  
  // If interval is zero, probably a driver trying to probe for our default interval, so return the first available one;
  // otherwise try to find the desired interval and return the corresponding mapping:
  if (interval)
  {
    float const fps = jevois::VideoMapping::uvcToFps(interval);
    size_t idx = 0;
  
    for (jevois::VideoMapping const & m : itsMappings)
      if (m.uvcformat == iformat && m.uvcframe == iframe && std::fabs(m.ofps - fps) < 0.1F) return idx;
      else ++idx;

    LFATAL("No video mapping for iformat=" << iformat <<", iframe=" << iframe << ", interval=" << interval);
  }
  else
  {
    size_t idx = 0;
  
    for (jevois::VideoMapping const & m : itsMappings)
      if (m.uvcformat == iformat && m.uvcframe == iframe) return idx;
      else ++idx;

    LFATAL("No video mapping for iformat=" << iformat <<", iframe=" << iframe << ", interval=" << interval);
  }
}

// ####################################################################################################
jevois::VideoMapping const & jevois::Engine::getDefaultVideoMapping() const
{ return itsMappings[itsDefaultMappingIdx]; }

// ####################################################################################################
size_t jevois::Engine::getDefaultVideoMappingIdx() const
{ return itsDefaultMappingIdx; }

// ####################################################################################################
jevois::VideoMapping const &
jevois::Engine::findVideoMapping(unsigned int oformat, unsigned int owidth, unsigned int oheight,
                                 float oframespersec) const
{
  for (jevois::VideoMapping const & m : itsMappings)
    if (m.match(oformat, owidth, oheight, oframespersec)) return m;

  LFATAL("Could not find mapping for output format " << jevois::fccstr(oformat) << ' ' <<
         owidth << 'x' << oheight << " @ " << oframespersec << " fps");
}

// ####################################################################################################
std::string jevois::Engine::camctrlname(int id, char const * longname) const
{
  for (size_t i = 0; i < sizeof camcontrols / sizeof camcontrols[0]; ++i)
    if (camcontrols[i].id == id) return camcontrols[i].shortname;
  
  // Darn, this control is not in our list, probably something exotic. Compute a name from the control's long name:
  return abbreviate(longname);
}

// ####################################################################################################
int jevois::Engine::camctrlid(std::string const & shortname)
{
  for (size_t i = 0; i < sizeof camcontrols / sizeof camcontrols[0]; ++i)
    if (shortname.compare(camcontrols[i].shortname) == 0) return camcontrols[i].id;

  // Not in our list, all right, let's find it then in the camera:
  struct v4l2_queryctrl qc = { };
  for (int cls = V4L2_CTRL_CLASS_USER; cls <= V4L2_CTRL_CLASS_DETECT; cls += 0x10000)
  {
    // Enumerate all controls in this class. Looks like there is some spillover between V4L2 classes in the V4L2
    // enumeration process, we end up with duplicate controls if we try to enumerate all the classes. Hence the
    // doneids set to keep track of the ones already reported:
    qc.id = cls | 0x900;
    while (true)
    {
      qc.id |= V4L2_CTRL_FLAG_NEXT_CTRL; unsigned int old_id = qc.id; bool failed = false;
      try
      {
        itsCamera->queryControl(qc);
        if (abbreviate(reinterpret_cast<char const *>(qc.name)) == shortname) return qc.id;
      }
      catch (...) { failed = true; }

      // With V4L2_CTRL_FLAG_NEXT_CTRL, the camera kernel driver is supposed to pass down the next valid control if
      // the requested one is not found, but some drivers do not honor that, so let's move on to the next control
      // manually if needed:
      qc.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
      if (qc.id == old_id) { ++qc.id; if (qc.id > 100 + (cls | 0x900 | V4L2_CTRL_FLAG_NEXT_CTRL)) break; }
      else if (failed) break;
    }
  }

  LFATAL("Could not find control [" << shortname << "] in the camera");
}

// ####################################################################################################
std::string jevois::Engine::camCtrlHelp(struct v4l2_queryctrl & qc, std::set<int> & doneids)
{
  // See if we have this control:
  itsCamera->queryControl(qc);
  qc.id &= ~V4L2_CTRL_FLAG_NEXT_CTRL;

  // If we have already done this control, just return an empty string:
  if (doneids.find(qc.id) != doneids.end()) return std::string(); else doneids.insert(qc.id);
  
  // Control exists, let's also get its current value:
  struct v4l2_control ctrl = { }; ctrl.id = qc.id;
  itsCamera->getControl(ctrl);

  // Print out some description depending on control type:
  std::ostringstream ss;
  ss << "- " << camctrlname(qc.id, reinterpret_cast<char const *>(qc.name));

  switch (qc.type)
  {
  case V4L2_CTRL_TYPE_INTEGER:
    ss << " [int] min=" << qc.minimum << " max=" << qc.maximum << " step=" << qc.step
       << " def=" << qc.default_value << " curr=" << ctrl.value;
    break;
    
    //case V4L2_CTRL_TYPE_INTEGER64:
    //ss << " [int64] value=" << ctrl.value64;
    //break;
    
    //case V4L2_CTRL_TYPE_STRING:
    //ss << " [str] min=" << qc.minimum << " max=" << qc.maximum << " step=" << qc.step
    //   << " curr=" << ctrl.string;
    //break;
    
  case V4L2_CTRL_TYPE_BOOLEAN:
    ss << " [bool] default=" << qc.default_value << " curr=" << ctrl.value;
    break;

    // This one is not supported by the older kernel on platform:
    //case V4L2_CTRL_TYPE_INTEGER_MENU:
    //ss << " [intmenu] min=" << qc.minimum << " max=" << qc.maximum 
    //   << " def=" << qc.default_value << " curr=" << ctrl.value;
    //break;
    
  case V4L2_CTRL_TYPE_BUTTON:
    ss << " [button]";
    break;
    
  case V4L2_CTRL_TYPE_BITMASK:
    ss << " [bitmask] max=" << qc.maximum << " def=" << qc.default_value << " curr=" << ctrl.value;
    break;
    
  case V4L2_CTRL_TYPE_MENU:
  {
    struct v4l2_querymenu querymenu = { };
    querymenu.id = qc.id;
    ss << " [menu] values ";
    for (querymenu.index = qc.minimum; querymenu.index <= (unsigned int)qc.maximum; ++querymenu.index)
    {
      try { itsCamera->queryMenu(querymenu); } catch (...) { strcpy((char *)(querymenu.name), "fixme"); }
      ss << querymenu.index << ':' << querymenu.name << ' ';
    }
    ss << "curr=" << ctrl.value;
  }
  break;
  
  default:
    ss << "[unknown type]";
  }

  if (qc.flags & V4L2_CTRL_FLAG_DISABLED) ss << " [DISABLED]";

  return ss.str();
}

#ifdef JEVOIS_PLATFORM
// ####################################################################################################
void jevois::Engine::startMassStorageMode()
{
  // itsMtx must be locked by caller

  if (itsMassStorageMode.load()) { LERROR("Already in mass-storage mode -- IGNORED"); return; }

  // Nuke any module and loader so we have nothing loaded that uses /jevois:
  if (itsModule) { removeComponent(itsModule); itsModule.reset(); }
  if (itsLoader) itsLoader.reset();

  // Unmount /jevois:
  if (std::system("sync")) LERROR("Disk sync failed -- IGNORED");
  if (std::system("mount -o remount,ro /jevois")) LERROR("Failed to remount /jevois read-only -- IGNORED");

  // Now set the backing partition in mass-storage gadget:
  std::ofstream ofs(JEVOIS_USBSD_SYS);
  if (ofs.is_open() == false) LFATAL("Cannot setup mass-storage backing file to " << JEVOIS_USBSD_SYS);
  ofs << JEVOIS_USBSD_FILE << std::endl;

  LINFO("Exported JEVOIS partition of microSD to host computer as virtual flash drive.");
  itsMassStorageMode.store(true);
}

// ####################################################################################################
void jevois::Engine::stopMassStorageMode()
{
  //itsMassStorageMode.store(false);
  LINFO("JeVois virtual USB drive ejected by host -- REBOOTING");
  reboot();
}

// ####################################################################################################
void jevois::Engine::reboot()
{
  if (std::system("sync")) LERROR("Disk sync failed -- IGNORED");
  itsCheckingMassStorage.store(false);
  itsRunning.store(false);

  // Hard reset to avoid possible hanging during module unload, etc:
  if ( ! std::ofstream("/proc/sys/kernel/sysrq").put('1')) LERROR("Cannot trigger hard reset -- please unplug me!");
  if ( ! std::ofstream("/proc/sysrq-trigger").put('s')) LERROR("Cannot trigger hard reset -- please unplug me!");
  if ( ! std::ofstream("/proc/sysrq-trigger").put('b')) LERROR("Cannot trigger hard reset -- please unplug me!");
  std::terminate();
}
#endif

// ####################################################################################################
bool jevois::Engine::parseCommand(std::string const & str, std::shared_ptr<UserInterface> s)
{
  // itsMtx should be locked by caller

  std::string errmsg;
  
  // Note: ModemManager on Ubuntu sends this on startup, kill ModemManager to avoid:
  // 41 54 5e 53 51 50 4f 52 54 3f 0d 41 54 0d 41 54 0d 41 54 0d 7e 00 78 f0 7e 7e 00 78 f0 7e
  //
  // AT^SQPORT?
  // AT
  // AT
  // AT
  // ~
  //
  // then later on it insists on trying to mess with us, issuing things like AT, AT+CGMI, AT+GMI, AT+CGMM, AT+GMM,
  // AT%IPSYS?, ATE0, ATV1, etc etc
  
  switch (str.length())
  {
  case 0:
    LDEBUG("Ignoring empty string"); return true;
    break;
    
  case 1:
    if (str[0] == '~') { LDEBUG("Ignoring modem config command [~]"); return true; }

    // If the string starts with "#", then just print it out on the serlog port(s). We use this to allow debug messages
    // from the arduino to be printed out to the user:
    if (str[0] == '#') { sendSerial(str, true); return true; }
    break;

  default: // length is 2 or more:

    // Ignore any command that starts with a '~':
    if (str[0] == '~') { LDEBUG("Ignoring modem config command [" << str << ']'); return true; }

    // Ignore any command that starts with "AT":
    if (str[0] == 'A' && str[1] == 'T') { LDEBUG("Ignoring AT command [" << str <<']'); return true; }

    // If the string starts with "#", then just print it out on the serlog port(s). We use this to allow debug messages
    // in the arduino to be printed out to the user:
    if (str[0] == '#') { sendSerial(str, true); return true; }
    
    // Get the first word, i.e., the command:
    size_t const idx = str.find(' '); std::string cmd, rem;
    if (idx == str.npos) cmd = str; else { cmd = str.substr(0, idx); if (idx < str.length()) rem = str.substr(idx+1); }
    
    if (cmd == "help")
    {
      // Show all commands, first ours, as supported below:
      s->writeString("GENERAL COMMANDS:");
      s->writeString("");
      s->writeString("help - print this help message");
      s->writeString("info - show system information including CPU speed, load and temperature");
      s->writeString("setpar <name> <value> - set a parameter value");
      s->writeString("getpar <name> - get a parameter value(s)");
      s->writeString("runscript <filename> - run script commands in specified file");
      s->writeString("setcam <ctrl> <val> - set camera control <ctrl> to value <val>");
      s->writeString("getcam <ctrl> - get value of camera control <ctrl>");
      if (camreg::get())
      {
        s->writeString("setcamreg <reg> <val> - set raw camera register <reg> to value <val>");
        s->writeString("getcamreg <reg> - get value of raw camera register <reg>");
      }
      s->writeString("listmappings - list all available video mappings");
      s->writeString("setmapping <num> - select video mapping <num>, only possible while not streaming");
      s->writeString("setmapping2 <CAMmode> <CAMwidth> <CAMheight> <CAMfps> <Vendor> <Module> - set no-USB-out "
                     "video mapping defined on the fly, while not streaming");
      if (itsCurrentMapping.ofmt == 0 || itsManualStreamon)
      {
        s->writeString("streamon - start camera video streaming");
        s->writeString("streamoff - stop camera video streaming");
      }
      s->writeString("ping - returns 'ALIVE'");
      s->writeString("serlog <string> - forward string to the serial port(s) specified by the serlog parameter");
      s->writeString("serout <string> - forward string to the serial port(s) specified by the serout parameter");

#ifdef JEVOIS_PLATFORM
      s->writeString("usbsd - export the JEVOIS partition of the microSD card as a virtual USB drive");
#endif
      s->writeString("sync - commit any pending data write to microSD");
      s->writeString("date [date and time] - get or set the system date and time");

#ifdef JEVOIS_PLATFORM
      s->writeString("restart - restart the JeVois smart camera");
#else
      s->writeString("quit - quit this program");
#endif
      s->writeString("");

      // Then the module's custom commands, if any:
      if (itsModule)
      {
        std::stringstream css; itsModule->supportedCommands(css);
        s->writeString("MODULE-SPECIFIC COMMANDS:");
        s->writeString("");
        for (std::string line; std::getline(css, line); /* */) s->writeString(line);
        s->writeString("");
      }
      
      // Get the help message for our parameters and write it out line by line so the serial fixes the line endings:
      std::stringstream pss; constructHelpMessage(pss);
      for (std::string line; std::getline(pss, line); /* */) s->writeString(line);

      // Show all camera controls
      s->writeString("AVAILABLE CAMERA CONTROLS:");
      s->writeString("");

      struct v4l2_queryctrl qc = { }; std::set<int> doneids;
      for (int cls = V4L2_CTRL_CLASS_USER; cls <= V4L2_CTRL_CLASS_DETECT; cls += 0x10000)
      {
        // Enumerate all controls in this class. Looks like there is some spillover between V4L2 classes in the V4L2
        // enumeration process, we end up with duplicate controls if we try to enumerate all the classes. Hence the
        // doneids set to keep track of the ones already reported:
        qc.id = cls | 0x900; unsigned int old_id;
        while (true)
        {
          qc.id |= V4L2_CTRL_FLAG_NEXT_CTRL; old_id = qc.id; bool failed = false;
          try { std::string hlp = camCtrlHelp(qc, doneids); if (hlp.empty() == false) s->writeString(hlp); }
          catch (...) { failed = true; }

          // The camera kernel driver is supposed to pass down the next valid control if the requested one is not
          // found, but some drivers do not honor that, so let's move on to the next control manually if needed:
          qc.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
          if (qc.id == old_id) { ++qc.id; if (qc.id > 100 + (cls | 0x900 | V4L2_CTRL_FLAG_NEXT_CTRL)) break; }
          else if (failed) break;
        }
      }

      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "info")
    {
      s->writeString("INFO: JeVois " JEVOIS_VERSION_STRING);
      s->writeString("INFO: " + jevois::getSysInfoVersion());
      s->writeString("INFO: " + jevois::getSysInfoCPU());
      s->writeString("INFO: " + jevois::getSysInfoMem());
      if (itsModule) s->writeString("INFO: " + itsCurrentMapping.str());
      else s->writeString("INFO: " + jevois::VideoMapping().str());
      return true;
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setpar")
    {
      std::istringstream ss(rem); std::string desc, val; ss >> desc; ss >> val;
      setParamString(desc, val);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getpar")
    {
      auto vec = getParamString(rem);
      for (auto const & p : vec) s->writeString(p.first + ' ' + p.second);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setcam")
    {
      std::istringstream ss(rem); std::string ctrl; int val; ss >> ctrl >> val;
      struct v4l2_control c = { }; c.id = camctrlid(ctrl); c.value = val;
      itsCamera->setControl(c);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getcam")
    {
      struct v4l2_control c = { }; c.id = camctrlid(rem);
      itsCamera->getControl(c);
      s->writeString(rem + ' ' + std::to_string(c.value));
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setcamreg")
    {
      if (camreg::get())
      {
        // Read register and value as strings, then std::stoi to convert to int, supports 0x (and 0 for octal, caution)
        std::istringstream ss(rem); std::string reg, val; ss >> reg >> val;
        itsCamera->writeRegister(std::stoi(reg, nullptr, 0), std::stoi(val, nullptr, 0));
        return true;
      }
      errmsg = "Access to camera registers is disabled, enable with --camreg=true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getcamreg")
    {
      if (camreg::get())
      {
        unsigned int val = itsCamera->readRegister(std::stoi(rem, nullptr, 0));
        std::ostringstream os; os << std::hex << val;
        s->writeString(os.str());
        return true;
      }
      errmsg = "Access to camera registers is disabled, enable with --camreg=true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "listmappings")
    {
      s->writeString("AVAILABLE VIDEO MAPPINGS:");
      s->writeString("");
      for (size_t idx = 0; idx < itsMappings.size(); ++idx)
      {
        std::string idxstr = std::to_string(idx);
        if (idxstr.length() < 5) idxstr = std::string(5 - idxstr.length(), ' ') + idxstr; // pad to 5-char long
        s->writeString(idxstr + " - " + itsMappings[idx].str());
      }
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setmapping")
    {
      size_t const idx = std::stoi(rem);
      bool was_streaming = itsStreaming.load();

      if (was_streaming)
      {
        errmsg = "Cannot set mapping while streaming: ";
        if (itsCurrentMapping.ofmt) errmsg += "Stop your webcam program on the host computer first.";
        else errmsg += "Issue a 'streamoff' command first.";
      }
      else if (idx >= itsMappings.size())
        errmsg = "Requested mapping index " + std::to_string(idx) + " out of range [0 .. " +
          std::to_string(itsMappings.size()-1) + ']';
      else
      {
        setFormatInternal(idx);
        return true;
      }
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setmapping2")
    {
      bool was_streaming = itsStreaming.load();

      if (was_streaming)
      {
        errmsg = "Cannot set mapping while streaming: ";
        if (itsCurrentMapping.ofmt) errmsg += "Stop your webcam program on the host computer first.";
        else errmsg += "Issue a 'streamoff' command first.";
      }
      else
      {
        jevois::VideoMapping m;
        try
        {
          std::istringstream full("NONE 0 0 0.0 " + rem); full >> m;
          setFormatInternal(m);
          return true;
        }
        catch (std::exception const & e) { errmsg = "Error parsing or setting mapping [" + rem + "]: " + e.what(); }
        catch (...) { errmsg = "Error parsing or setting mapping [" + rem + ']'; }
      }
    }

    // ----------------------------------------------------------------------------------------------------
    if (itsCurrentMapping.ofmt == 0 || itsManualStreamon)
    {
      if (cmd == "streamon")
      {
        // keep this in sync with streamOn(), modulo the fact that here we are already locked:
        itsCamera->streamOn();
        itsGadget->streamOn();
        itsStreaming.store(true);
        return true;
      }
      
      if (cmd == "streamoff")
      {
        // keep this in sync with streamOff(), modulo the fact that here we are already locked:
        itsGadget->abortStream();
        itsCamera->abortStream();

        itsStreaming.store(false);
  
        itsGadget->streamOff();
        itsCamera->streamOff();
        return true;
      }
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "ping")
    {
      s->writeString("ALIVE");
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "serlog")
    {
      sendSerial(rem, true);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "serout")
    {
      sendSerial(rem, false);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
#ifdef JEVOIS_PLATFORM
    if (cmd == "usbsd")
    {
      if (itsStreaming.load())
      {
        errmsg = "Cannot export microSD over USB while streaming: ";
        if (itsCurrentMapping.ofmt) errmsg += "Stop your webcam program on the host computer first.";
        else errmsg += "Issue a 'streamoff' command first.";
      }
      else
      {
        startMassStorageMode();
        return true;
      }
    }
#endif    

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "sync")
    {
      if (std::system("sync")) LERROR("Disk sync failed -- IGNORED");
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "date")
    {
      std::string dat = jevois::system("/bin/date " + rem);
      LINFO("date now " << dat.substr(0, dat.size()-1)); // skip trailing newline
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "runscript")
    {
      std::string fname;
      if (itsModule) fname = itsModule->absolutePath(rem); else fname = rem;
      
      try { runScriptFromFile(fname, s, true); return true; }
      catch (...) { errmsg = "Script execution failed."; }
    }
 
#ifdef JEVOIS_PLATFORM
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "restart")
    {
      s->writeString("Restart command received - bye-bye!");

      if (itsStreaming.load()) LERROR("Video streaming is on - you should quit your video viewer before rebooting");
      
      // Turn off the SD storage if it is there:
      std::ofstream(JEVOIS_USBSD_SYS).put('\n'); // ignore errors
      if (std::system("sync")) LERROR("Disk sync failed -- IGNORED");

      // Hard reboot:
      this->reboot();
      return true;
    }
    // ----------------------------------------------------------------------------------------------------
#else
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "quit")
    {
      s->writeString("Quit command received - bye-bye!");
      itsRunning.store(false);
      return true;
    }
    // ----------------------------------------------------------------------------------------------------
#endif
  }
  
  // If we make it here, we did not parse the command. If we have an error message, that means we had started parsing
  // the command but it was buggy, so let's throw. Otherwise, we just return false to indicate that we did not parse
  // this command and maybe it is for the Module:
  if (errmsg.size()) throw std::runtime_error("Command error [" + str + "]: " + errmsg);
  return false;
}

// ####################################################################################################
void jevois::Engine::runScriptFromFile(std::string const & filename, std::shared_ptr<jevois::UserInterface> ser,
                                       bool throw_no_file)
{
  // itsMtx should be locked by caller
  
  // Try to find the file:
  std::ifstream ifs(filename);
  if (!ifs) { if (throw_no_file) LFATAL("Could not open file " << filename); else return; }

  // We need to identify a serial to send any errors to, if none was given to us. Let's use the one in serlog, or, if
  // none is specified there, the first available serial:
  if (!ser)
  {
    if (itsSerials.empty()) LFATAL("Need at least one active serial to run script");
    switch (serlog::get())
    {
    case jevois::engine::SerPort::Hard:
      for (auto & s : itsSerials) if (s->type() == jevois::UserInterface::Type::Hard) { ser = s; break; }
      break;

    case jevois::engine::SerPort::USB:
      for (auto & s : itsSerials) if (s->type() == jevois::UserInterface::Type::USB) { ser = s; break; }
      break;
      
    default: break;
    }
    if (!ser) ser = itsSerials.front();
  }
  
  // Ok, run the script, plowing through any errors:
  size_t linenum = 1;
  for (std::string line; std::getline(ifs, line); /* */)
  {
    // Skip comments and empty lines:
    if (line.length() == 0 || line[0] == '#') continue;

    // Go and parse that line:
    try
    {
      bool parsed = false;
      try { parsed = parseCommand(line, ser); }
      catch (std::exception const & e)
      { ser->writeString("ERR " + filename + ':' + std::to_string(linenum) + ": " + e.what()); }
      catch (...)
      { ser->writeString("ERR " + filename + ':' + std::to_string(linenum) + ": Bogus command ["+line+"] ignored"); }

      if (parsed == false)
      {
        if (itsModule)
        {
          try { itsModule->parseSerial(line, ser); }
          catch (std::exception const & me)
          { ser->writeString("ERR " + filename + ':' + std::to_string(linenum) + ": " + me.what()); }
          catch (...)
          { ser->writeString("ERR " + filename + ':' + std::to_string(linenum)+": Bogus command ["+line+"] ignored"); }
        }
        else ser->writeString("ERR Unsupported command [" + line + "] and no module");
      }
    }
    catch (...) { jevois::warnAndIgnoreException(); }
    
    ++linenum;
  }
}
