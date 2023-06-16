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

#include <jevois/Core/IMU.H>
#include <jevois/Core/IMUspi.H>
#include <jevois/Core/IMUi2c.H>

#include <jevois/Core/Gadget.H>
#include <jevois/Core/VideoOutputNone.H>
#include <jevois/Core/MovieOutput.H>
#include <jevois/Core/VideoDisplay.H>
#include <jevois/Core/VideoDisplayGL.H>
#include <jevois/Core/VideoDisplayGUI.H>
#include <jevois/GPU/GUIhelper.H>
#include <jevois/GPU/GUIconsole.H>

#include <jevois/Core/Serial.H>
#include <jevois/Core/StdioInterface.H>

#include <jevois/Core/Module.H>
#include <jevois/Core/DynamicLoader.H>
#include <jevois/Core/PythonSupport.H>
#include <jevois/Core/PythonModule.H>

#include <jevois/Debug/Log.H>
#include <jevois/Util/Utils.H>
#include <jevois/Util/Async.H>
#include <jevois/Debug/SysInfo.H>

#include <cmath> // for fabs
#include <fstream>
#include <algorithm>
#include <cstdlib> // for std::system()
#include <cstdio> // for std::remove()
#include <regex>

#ifdef JEVOIS_PRO
#include <imgui_internal.h>
#endif

// On the older JeVois-A33 platform kernel, detect class is not defined:
#ifndef V4L2_CTRL_CLASS_DETECT
#define V4L2_CTRL_CLASS_DETECT          0x00a30000
#endif

namespace
{
  // Assign a short name to every V4L2 control, for use by getcam and setcam commands
  struct shortcontrol { unsigned int id; char const * const shortname; };

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
#define V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE    (V4L2_CID_CAMERA_CLASS_BASE+20)
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
namespace jevois { namespace engine { static std::atomic<size_t> frameNumber(0); } }

size_t jevois::frameNum()
{ return jevois::engine::frameNumber.load(); }

// ####################################################################################################
jevois::Engine::Engine(std::string const & instance) :
    jevois::Manager(instance), itsMappings(), itsRunning(false), itsStreaming(false), itsStopMainLoop(false),
    itsShellMode(false), itsTurbo(false), itsManualStreamon(false), itsVideoErrors(false),
    itsNumSerialSent(0), itsRequestedFormat(-2)
{
  JEVOIS_TRACE(1);

#ifdef JEVOIS_PLATFORM_A33
  // Start mass storage thread:
  itsCheckingMassStorage.store(false); itsMassStorageMode.store(false);
  itsCheckMassStorageFut = jevois::async_little(&jevois::Engine::checkMassStorage, this);
  while (itsCheckingMassStorage.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif

  jevois::engine::frameNumber.store(0);
}

// ####################################################################################################
jevois::Engine::Engine(int argc, char const* argv[], std::string const & instance) :
    jevois::Manager(argc, argv, instance), itsMappings(), itsRunning(false), itsStreaming(false),
    itsStopMainLoop(false), itsShellMode(false), itsTurbo(false), itsManualStreamon(false), itsVideoErrors(false),
    itsNumSerialSent(0), itsRequestedFormat(-2)
{
  JEVOIS_TRACE(1);
  
#ifdef JEVOIS_PLATFORM_A33
  // Start mass storage thread:
  itsCheckingMassStorage.store(false); itsMassStorageMode.store(false);
  itsCheckMassStorageFut = jevois::async_little(&jevois::Engine::checkMassStorage, this);
  while (itsCheckingMassStorage.load() == false) std::this_thread::sleep_for(std::chrono::milliseconds(5));
#endif

  jevois::engine::frameNumber.store(0);
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
#ifdef JEVOIS_PRO
  std::ofstream ofs("/sys/devices/system/cpu/cpu2/cpufreq/scaling_governor");
#else
  std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
#endif
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

#ifdef JEVOIS_PRO
// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::cpumodel const & JEVOIS_UNUSED_PARAM(param),
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
#endif

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::cpumax const & JEVOIS_UNUSED_PARAM(param),
                                   unsigned int const & newval)
{
#ifdef JEVOIS_PRO
  std::ofstream ofs("/sys/devices/system/cpu/cpu2/cpufreq/scaling_max_freq");
#else
  std::ofstream ofs("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");
#endif
  
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

#ifdef JEVOIS_PRO
// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::gui const & JEVOIS_UNUSED_PARAM(param),
                                   bool const & newval)
{
  JEVOIS_TIMED_LOCK(itsMtx);
  if (newval)
  {
    if (!itsGUIhelper)
    {
      itsGUIhelper = addComponent<jevois::GUIhelper>("gui", conslock::get());
      auto s = addComponent<jevois::GUIconsole>("guiconsole");
      itsSerials.push_back(s);
      LINFO("GUI enabled.");
    }
  }
  else if (itsGUIhelper)
  {
    for (auto itr = itsSerials.begin(); itr != itsSerials.end(); ++itr)
      if ((*itr)->instanceName() == "guiconsole") { itsSerials.erase(itr); break; }
    removeComponent("guiconsole", false);
    removeComponent(itsGUIhelper);
    itsGUIhelper.reset();
    LINFO("GUI disabled.");
  }
}

// ####################################################################################################
void jevois::Engine::onParamChange(jevois::engine::cpumaxl const & JEVOIS_UNUSED_PARAM(param),
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
#endif

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
void jevois::Engine::reloadVideoMappings()
{
  // Check iw we want to use GUI mode:
  bool usegui = false;
#ifdef JEVOIS_PRO
  usegui = gui::get();
#endif

  itsMappings = jevois::loadVideoMappings(camerasens::get(), itsDefaultMappingIdx, true, usegui);
  LINFO("Loaded " << itsMappings.size() << " vision processing modes.");
}

// ####################################################################################################
void jevois::Engine::postInit()
{
  // First make sure the manager gets to run this:
  jevois::Manager::postInit();

  // Prevent any setFormat() that may be requested, e.g., by the Inventor, as soon as it detects our gadget, until after
  // we have completed running the initscript:
  JEVOIS_TIMED_LOCK(itsMtx);
  
  // Freeze the serial port device names, their params, and camera and gadget too:
  serialdev::freeze();
  usbserialdev::freeze();
  for (auto & s : itsSerials) s->freezeAllParams();
  cameradev::freeze();
  imudev::freeze();
  cameranbuf::freeze();
  camturbo::freeze();
  gadgetdev::freeze();
  gadgetnbuf::freeze();
  itsTurbo = camturbo::get();
  multicam::freeze();
  quietcmd::freeze();
  python::freeze();
  
  // On JeVois-Pro platform, we may get the camera sensor automatically from the device tree. Users should still load
  // the correct overlay in /boot/env.txt to match the installed sensor:
  jevois::CameraSensor camsens = camerasens::get();
#ifdef JEVOIS_PLATFORM_PRO
  if (camsens == jevois::CameraSensor::any)
  {
    std::string str = jevois::getFileString("/proc/device-tree/sensor/sensor-name"); // that name has trailing garbage
    size_t idx = 0; while (idx < str.length() && std::isalnum(str[idx])) ++idx;
    str = str.substr(0, idx);
    camerasens::strset(str);
    camsens = camerasens::get();
    LINFO("Camera sensor selected from device tree: " << camsens);
  }
#endif
  camerasens::freeze();
  
  // Check iw we want to use GUI mode:
  bool usegui = false;
#ifdef JEVOIS_PRO
  gui::freeze();
  usegui = gui::get();
  conslock::freeze();
  watchdog::freeze();
#endif
  
  // Grab the log messages, itsSerials is not going to change anymore now that the serial params are frozen:
  jevois::logSetEngine(this);

  // Load our video mappings:
  reloadVideoMappings();

  // Get python going, we need to do this here to avoid segfaults on platform when instantiating our first python
  // module. This likely has to do with the fact that the python core is not very thread-safe, and setFormatInternal()
  // in Engine, which instantiates python modules, will indeed be invoked from a different thread (the one that receives
  // USB UVC events). Have a look at Python Thread State, Python Gobal Interpreter Lock, etc if interested:
  if (python::get())
  {
    LINFO("Initalizing Python...");
    jevois::python::setEngine(this);
  }
  
  // Instantiate a camera: If device names starts with "/dev/v", assume a hardware camera, otherwise a movie file:
  std::string const camdev = cameradev::get();
  if (jevois::stringStartsWith(camdev, "/dev/v"))
  {
    LINFO("Starting camera device " << camdev);
    
#ifdef JEVOIS_PLATFORM_A33
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
    itsCamera.reset(new jevois::Camera(camdev, camsens, cameranbuf::get()));
    
#ifndef JEVOIS_PLATFORM
    // No need to confuse people with a non-working camreg and imureg params:
    camreg::set(false); camreg::freeze();
    imureg::set(false); imureg::freeze();
#endif

    try
    {
      // On JeVois-A33 platform, hook up an I2C-based IMU to the camera sensor, if supported:
#ifdef JEVOIS_PLATFORM_A33
      if (jevois::sensorHasIMU(camsens))
        itsIMU.reset(new jevois::IMUi2c(std::dynamic_pointer_cast<jevois::Camera>(itsCamera)));
#endif
    
      // On JeVois-Pro platform, instantiate an SPI-based IMU, if supported:
#ifdef JEVOIS_PLATFORM_PRO
      if (jevois::sensorHasIMU(camsens))
        itsIMU.reset(new jevois::IMUspi(imudev::get()));
#endif
    } catch (...) { LERROR("Sensor should have an IMU but we failed to initialize it."); }
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
    itsGadget.reset(new jevois::Gadget(gd, itsCamera.get(), this, gadgetnbuf::get(), multicam::get()));
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
    // Local video display, for use on a host desktop or on JeVois-Pro HDMI output:
#ifdef JEVOIS_PRO
    // On JevoisPro, use OpenGL or ImGui display:
    if (usegui)
    {
      LINFO("Using OpenGL + ImGui display for video output");
      itsGadget.reset(new jevois::VideoDisplayGUI(itsGUIhelper, gadgetnbuf::get()));
    }
    else
    {
      LINFO("Using OpenGL display for video output");
      itsGadget.reset(new jevois::VideoDisplayGL(gadgetnbuf::get()));
    }
#else
    // On JeVois-A33, use an OpenCV display:
    LINFO("Using OpenCV display for video output");
    itsGadget.reset(new jevois::VideoDisplay("JeVois", gadgetnbuf::get()));
    (void)usegui; // keep compiler happy
#endif
    itsManualStreamon = true;
  }
  
  // We are ready to run:
  itsRunning.store(true);

  // Set initial format:
  try { setFormatInternal(midx); } catch (...) { jevois::warnAndIgnoreException(); }

  // Run init script:
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
  
#ifdef JEVOIS_PLATFORM_A33
  // Tell checkMassStorage() thread to finish up:
  itsCheckingMassStorage.store(false);
#endif
  
  // Nuke our module as soon as we can, hopefully soon now that we turned off streaming and running:
  {
    JEVOIS_TIMED_LOCK(itsMtx);
    if (itsModule) removeComponent(itsModule);
    itsModule.reset();

    // Gone, nuke the loader now:
    itsLoader.reset();
  }
  
  // Because we passed the camera as a raw pointer to the gadget, nuke the gadget first and then the camera:
  itsGadget.reset();
  itsCamera.reset();

#ifdef JEVOIS_PLATFORM_A33
  // Will block until the checkMassStorage() thread completes:
  if (itsCheckMassStorageFut.valid())
    try { itsCheckMassStorageFut.get(); } catch (...) { jevois::warnAndIgnoreException(); }
#endif
  
  // Things should be quiet now, unhook from the logger (this call is not strictly thread safe):
  jevois::logSetEngine(nullptr);
}

// ####################################################################################################
#ifdef JEVOIS_PLATFORM_A33
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
  if (itsCamera) itsCamera->streamOn();
  if (itsGadget) itsGadget->streamOn();
  itsStreaming.store(true);
}

// ####################################################################################################
void jevois::Engine::streamOff()
{
  JEVOIS_TRACE(2);

  // First, tell both the camera and gadget to abort streaming, this will make get()/done()/send() throw:
  if (itsGadget) itsGadget->abortStream();
  if (itsCamera) itsCamera->abortStream();

  // Stop the main loop, which will flip itsStreaming to false and will make it easier for us to lock itsMtx:
  LDEBUG("Stopping main loop...");
  itsStopMainLoop.store(true);
  while (itsStopMainLoop.load() && itsRunning.load()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  LDEBUG("Main loop stopped.");
  
  // Lock up and stream off:
  JEVOIS_TIMED_LOCK(itsMtx);
  if (itsGadget) itsGadget->streamOff();
  if (itsCamera) itsCamera->streamOff();
}

// ####################################################################################################
void jevois::Engine::requestSetFormat(int idx)
{
  JEVOIS_TRACE(2);
  itsRequestedFormat.store(idx);
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
void jevois::Engine::setFormatInternal(jevois::VideoMapping const & m, bool reload)
{
  // itsMtx should be locked by caller, idx should be valid:
  JEVOIS_TRACE(2);

  LINFO(m.str());
  itsModuleConstructionError = "Unknown error while starting module " + m.modulename + " ...";

#ifdef JEVOIS_PLATFORM_A33
  if (itsMassStorageMode.load())
    LFATAL("Cannot setup video streaming while in mass-storage mode. Eject the USB drive on your host computer first.");
#endif

  // Nuke the processing module, if any, so we can also safely nuke the loader. We always nuke the module instance so we
  // won't have any issues with latent state even if we re-use the same module but possibly with different input
  // image resolution, etc:
  if (itsModule)
  {
    LDEBUG("Removing current module " << itsModule->className() << ": " << itsModule->descriptor());
    try { removeComponent(itsModule); itsModule.reset(); LDEBUG("Current module removed."); }
    catch (...) { jevois::warnAndIgnoreException(); }
  }
  
  // Set the format at the camera and gadget levels, unless we are just reloading:
  if (reload == false)
  {
    LDEBUG("Setting camera format: " << m.cstrall());
    try { itsCamera->setFormat(m); }
    catch (...)
    {
      jevois::warnAndIgnoreException();
      itsModuleConstructionError = "Camera did not accept format:\n\n" + m.cstrall() +
        "\n\nCheck videomappings.cfg and camera sensor specifications.";
      return;
    }
    
    LDEBUG("Setting gadget format: " << m.ostr());
    try { itsGadget->setFormat(m); }
    catch (...)
    {
      jevois::warnAndIgnoreException();
      itsModuleConstructionError = "Gadget did not accept format:\n\n" + m.ostr() +
        "\n\nCheck videomappings.cfg for any unsupported output formats.";
      return;
    }
  }
  
  // Keep track of our current mapping:
  itsCurrentMapping = m;
  
  // Reset our master frame counter on each module load:
  jevois::engine::frameNumber.store(0);
  
  // Instantiate the module. If the constructor throws, code is bogus, for example some syntax error in a python module
  // that is detected at load time. We get the exception's error message for later display into video frames in the main
  // loop, and mark itsModuleConstructionError:
  try
  {
    // For python modules, we do not need a loader, we just instantiate our special python wrapper module instead:
    std::string const sopath = m.sopath();
    if (m.ispython)
    {
      if (python::get() == false) LFATAL("Python disabled, delete BOOT:nopython and restart to enable python");
      
      // Instantiate the python wrapper:
      itsLoader.reset();
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

    // And finally run any config script, sending any errors to USB (likely JeVois Inventor) and GUI:
    std::shared_ptr<jevois::UserInterface> ser;
    for (auto & s : itsSerials)
      if (s->type() == jevois::UserInterface::Type::USB || s->type() == jevois::UserInterface::Type::GUI)
      { ser = s; break; }

    runScriptFromFile(itsModule->absolutePath(JEVOIS_MODULE_SCRIPT_FILENAME), ser, false);
    
    LINFO("Module [" << m.modulename << "] loaded, initialized, and ready.");
    itsModuleConstructionError.clear();
  }
  catch (...)
  {
    // Note: we do not nuke the module here, as the Inventor may need its path to fix some config files.
    itsModuleConstructionError = jevois::warnAndIgnoreException();
    LERROR("Module [" << m.modulename << "] startup error and not operational.");
  }
}

// ####################################################################################################
int jevois::Engine::mainLoop()
{
  JEVOIS_TRACE(2);

#ifdef JEVOIS_PRO
  // Start watchdog:
  itsWatchdog.reset(new jevois::Watchdog(watchdog::get()));
#endif
  
  std::string pfx; // optional command prefix
  int ret = 0; // our return value
  
  // Announce that we are ready to the hardware serial port, if any. Do not use sendSerial() here so we always issue
  // this message irrespectively of the user serial preferences:
  for (auto & s : itsSerials)
    if (s->type() == jevois::UserInterface::Type::Hard)
      try { s->writeString("INF READY JEVOIS " JEVOIS_VERSION_STRING); }
      catch (...) { jevois::warnAndIgnoreException(); }
  
  while (itsRunning.load())
  {
    bool dosleep = true;

#ifdef JEVOIS_PRO
    // Reset the watchdog:
    itsWatchdog->reset();
#endif
    
    // If we got a format change request through requestSetFormat(), honor it now while we are unlocked:
    // -2 means no change requested; -1 means reload requested (we do not need to change camera or gadget)
    int rf = itsRequestedFormat.load();
    if (rf != -2)
    {
      // This format change request is now marked as handled:
      itsRequestedFormat.store(-2);
      
      try
      {
        // Stop camera and gadget unless we are just reloading:
        if (rf != -1 && itsStreaming.load())
        {
          // Keep this code in sync with streamOff():
          if (itsGadget) itsGadget->abortStream();
          if (itsCamera) itsCamera->abortStream();
          JEVOIS_TIMED_LOCK(itsMtx);
          if (itsGadget) itsGadget->streamOff();
          if (itsCamera) itsCamera->streamOff();
          itsStreaming.store(false);
        }
        
        // Set new format or reload current module:
        if (rf == -1)
        {
          // Reload the current format, eg, after editing code:
          JEVOIS_TIMED_LOCK(itsMtx);
          setFormatInternal(itsCurrentMapping, true);
        }
        else setFormat(rf);
        
#ifdef JEVOIS_PRO
        // Reset the GUI to clear various texture caches and such:
        if (itsGUIhelper) itsGUIhelper->reset( (rf != -1) );
#endif

        // Restart camera and gadget if we stopped them:
        if (rf != -1 && itsCurrentMapping.ofmt != 0)
        {
          // Keep this code in sync with streamOn();
          JEVOIS_TIMED_LOCK(itsMtx);
          if (itsCamera) itsCamera->streamOn();
          if (itsGadget) itsGadget->streamOn();
          itsStreaming.store(true);
        }
        
        // On JeVois Pro running the GUI we need to get the camera and gadget streaming at all times for the GUI to
        // refresh, so restart them. When not using the GUI, users will have to issue a "streamon" to get going:
#ifdef JEVOIS_PRO
        if (itsGUIhelper && itsStreaming.load() == false)
        {
          // Keep this code in sync with streamOn();
          JEVOIS_TIMED_LOCK(itsMtx);
          if (itsCamera) itsCamera->streamOn();
          if (itsGadget) itsGadget->streamOn();
          itsStreaming.store(true);
        }
#endif
      }
      catch (...)
      {
        reportErrorInternal();

        // Stream off:
        try
        {
          if (itsGadget) itsGadget->abortStream();
          if (itsCamera) itsCamera->abortStream();
          JEVOIS_TIMED_LOCK(itsMtx);
          if (itsGadget) itsGadget->streamOff();
          if (itsCamera) itsCamera->streamOff();
          itsStreaming.store(false);
        }
        catch (...) { }
      }
    }
    
    if (itsStreaming.load())
    {
      // Lock up while we use the module:
      JEVOIS_TIMED_LOCK(itsMtx);

      if (itsModuleConstructionError.empty() == false)
      {
        // If we have a module construction error, report it now to GUI/USB/console:
        reportErrorInternal(itsModuleConstructionError);

        // Also get one camera frame to avoid accumulation of stale buffers:
        //try { (void)jevois::InputFrame(itsCamera, itsTurbo).get(); }
        //catch (...) { jevois::warnAndIgnoreException(); }
      }
      else if (itsModule)
      {
        // For standard modules, indicate frame start mark if user wants it:
        jevois::StdModule * stdmod = dynamic_cast<jevois::StdModule *>(itsModule.get());
        if (stdmod) stdmod->sendSerialMarkStart();
    
        // We have a module ready for action. Call its process function and handle any exceptions:
        try
        {
          switch (itsCurrentMapping.ofmt)
          {
          case 0:
          {
            // Process with no USB outputs:
            itsModule->process(jevois::InputFrame(itsCamera, itsTurbo));

#ifdef JEVOIS_PRO
            // We always need startFrame()/endFrame() when using the GUI:
            if (itsGUIhelper) itsGUIhelper->headlessDisplay();
#endif
            break;
          }
          
#ifdef JEVOIS_PRO
          case JEVOISPRO_FMT_GUI:
          {
            // Process with GUI display on JeVois-Pro:
            itsModule->process(jevois::InputFrame(itsCamera, itsTurbo), *itsGUIhelper);
            break;
          }
#endif
          default:
          {
            // Process with USB outputs:
            itsModule->process(jevois::InputFrame(itsCamera, itsTurbo),
                               jevois::OutputFrame(itsGadget, itsVideoErrors.load() ? &itsVideoErrorImage : nullptr));
          }
          }
          
          // If process() did not throw, no need to sleep:
          dosleep = false;
        }
        catch (...) { reportErrorInternal(); }

        // For standard modules, indicate frame start stop if user wants it:
        if (stdmod) stdmod->sendSerialMarkStop();

        // Increment our master frame counter
        ++ jevois::engine::frameNumber;
        itsNumSerialSent.store(0);
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
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
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

          // If the command starts with our hidden command prefix, set the prefix, otherwise clear it:
          if (jevois::stringStartsWith(str, JEVOIS_JVINV_PREFIX))
          {
            pfx = JEVOIS_JVINV_PREFIX;
            str = str.substr(pfx.length());
          }
          else pfx.clear();
      
          // Try to execute this command. If the command is for us (e.g., set a parameter) and is correct,
          // parseCommand() will return true; if it is for us but buggy, it will throw. If it is not recognized by us,
          // it will return false and we should try sending it to the Module:
          try { parsed = parseCommand(str, s, pfx); success = parsed; }
          catch (std::exception const & e)
          { s->writeString(pfx, std::string("ERR ") + e.what()); parsed = true; }
          catch (...)
          { s->writeString(pfx, "ERR Unknown error"); parsed = true; }

          if (parsed == false)
          {
            if (itsModule)
            {
              // Note: prefixing is currently not supported for modules, it is for the Engine only
              try { itsModule->parseSerial(str, s); success = true; }
              catch (std::exception const & me) { s->writeString(pfx, std::string("ERR ") + me.what()); }
              catch (...) { s->writeString(pfx, "ERR Command [" + str + "] not recognized by Engine or Module"); }
            }
            else s->writeString(pfx, "ERR Unsupported command [" + str + "] and no module");
          }
          
          // If success, let user know:
          if (success && quietcmd::get() == false && itsShellMode == false) s->writeString(pfx, "OK");
        }
      }
      catch (...) { jevois::warnAndIgnoreException(); }
    }
  }
  return ret;
}

// ####################################################################################################
void jevois::Engine::sendSerial(std::string const & str, bool islog)
{
  // If not a log message, we may want to limit the number of serout messages that a module sends on each frame:
  size_t slim = serlimit::get();
  if (islog == false && slim)
  {
    if (itsNumSerialSent.load() >= slim) return; // limit reached, message dropped
    ++itsNumSerialSent; // increment number of messages sent. It is reset in the main loop on each new frame.
  }

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

#ifdef JEVOIS_PRO
  // If we did not send to All (which includes the GUI), check whether the GUI wants it too:
  if (itsGUIhelper && ((islog && itsGUIhelper->serlogEnabled()) || (!islog && itsGUIhelper->seroutEnabled())))
    for (auto & s : itsSerials)
      if (s->type() == jevois::UserInterface::Type::GUI)
        try { s->writeString(str); } catch (...) { jevois::warnAndIgnoreException(); }
#endif
}

// ####################################################################################################
void jevois::Engine::reportError(std::string const & err)
{
#ifdef JEVOIS_PRO
  if (itsGUIhelper) itsGUIhelper->reportError(err);
#endif
  LERROR(err);
}

// ####################################################################################################
void jevois::Engine::clearErrors()
{
#ifdef JEVOIS_PRO
  // If using a GUI, clear errors in the GUI:
  if (itsGUIhelper) itsGUIhelper->clearErrors();
#endif
  // Otherwise, no need to clear anything, other errors are not persistently displayed.
}

// ####################################################################################################
void jevois::Engine::reportErrorInternal(std::string const & err)
{
#ifdef JEVOIS_PRO
  // If using a GUI, report error to GUI:
  if (itsGUIhelper && itsCurrentMapping.ofmt == JEVOISPRO_FMT_GUI)
  {
    if (itsGUIhelper->frameStarted() == false) { unsigned short w, h; itsGUIhelper->startFrame(w, h); }
    if (err.empty()) itsGUIhelper->reportError(jevois::warnAndIgnoreException());
    else itsGUIhelper->reportError(err);
    itsGUIhelper->endFrame();
  }
  else
#endif
  // Report exceptions to video if desired: We have to be extra careful here because the exception might have
  // been called by the input frame (camera not streaming) or the output frame (gadget not streaming), in
  // addition to exceptions thrown by the module:
  if (itsCurrentMapping.ofmt != 0 && itsCurrentMapping.ofmt != JEVOISPRO_FMT_GUI && itsVideoErrors.load())
  {
    try
    {
      // If the module threw before get() or after send() on the output frame, get a buffer from the gadget:
      if (itsVideoErrorImage.valid() == false) itsGadget->get(itsVideoErrorImage); // could throw if streamoff
      
      // Draw the error message into our video frame:
      if (err.empty()) jevois::drawErrorImage(jevois::warnAndIgnoreException(), itsVideoErrorImage);
      else jevois::drawErrorImage(err, itsVideoErrorImage);
    }
    catch (...) { jevois::warnAndIgnoreException(); }
    
    try
    {
      // Send the error image over USB:
      if (itsVideoErrorImage.valid()) itsGadget->send(itsVideoErrorImage); // could throw if gadget stream off
    }
    catch (...) { jevois::warnAndIgnoreException(); }
    
    // Invalidate the error image so it is clean for the next frame:
    itsVideoErrorImage.invalidate();
  }
  else
  {
    // Report module exception to serlog, and ignore:
    if (err.empty()) jevois::warnAndIgnoreException();
    else LERROR(err);
  }
}

// ####################################################################################################
std::shared_ptr<jevois::Module> jevois::Engine::module() const
{ return itsModule; }

// ####################################################################################################
std::shared_ptr<jevois::IMU> jevois::Engine::imu() const
{ return itsIMU; }

// ####################################################################################################
std::shared_ptr<jevois::Camera> jevois::Engine::camera() const
{ return std::dynamic_pointer_cast<jevois::Camera>(itsCamera); }

// ####################################################################################################
jevois::VideoMapping const & jevois::Engine::getCurrentVideoMapping() const
{ return itsCurrentMapping; }

// ####################################################################################################
size_t jevois::Engine::numVideoMappings() const
{ return itsMappings.size(); }

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
void jevois::Engine::foreachVideoMapping(std::function<void(jevois::VideoMapping const & m)> && func)
{
  for (jevois::VideoMapping const & m : itsMappings)
    try { func(m); } catch (...) { jevois::warnAndIgnoreException(); }
}

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
void jevois::Engine::foreachCamCtrl(std::function<void(struct v4l2_queryctrl & qc, std::set<int> & doneids)> && func)
{
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
      try { func(qc, doneids); } catch (...) { failed = true; }
      
      // The camera kernel driver is supposed to pass down the next valid control if the requested one is not
      // found, but some drivers do not honor that, so let's move on to the next control manually if needed:
      qc.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
      if (qc.id == old_id) { ++qc.id; if (qc.id > 100 + (cls | 0x900 | V4L2_CTRL_FLAG_NEXT_CTRL)) break; }
      else if (failed) break;
    }
  }
}

// ####################################################################################################
std::string jevois::Engine::camctrlname(unsigned int id, char const * longname) const
{
  for (size_t i = 0; i < sizeof camcontrols / sizeof camcontrols[0]; ++i)
    if (camcontrols[i].id == id) return camcontrols[i].shortname;
  
  // Darn, this control is not in our list, probably something exotic. Compute a name from the control's long name:
  return abbreviate(longname);
}

// ####################################################################################################
unsigned int jevois::Engine::camctrlid(std::string const & shortname)
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

// ####################################################################################################
std::string jevois::Engine::camCtrlInfo(struct v4l2_queryctrl & qc, std::set<int> & doneids)
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
  ss << camctrlname(qc.id, reinterpret_cast<char const *>(qc.name));

  if (qc.flags & V4L2_CTRL_FLAG_DISABLED) ss << " D ";

  switch (qc.type)
  {
  case V4L2_CTRL_TYPE_INTEGER:
    ss << " I " << qc.minimum << ' ' << qc.maximum << ' ' << qc.step
       << ' ' << qc.default_value << ' ' << ctrl.value;
    break;
    
    //case V4L2_CTRL_TYPE_INTEGER64:
    //ss << " J " << ctrl.value64;
    //break;
    
    //case V4L2_CTRL_TYPE_STRING:
    //ss << " S " << qc.minimum << ' ' << qc.maximum << ' ' << qc.step << ' ' << ctrl.string;
    //break;
    
  case V4L2_CTRL_TYPE_BOOLEAN:
    ss << " B " << qc.default_value << ' ' << ctrl.value;
    break;

    // This one is not supported by the older kernel on platform:
    //case V4L2_CTRL_TYPE_INTEGER_MENU:
    //ss << " N " << qc.minimum << ' ' << qc.maximum << ' ' << qc.default_value << ' ' << ctrl.value;
    //break;
    
  case V4L2_CTRL_TYPE_BUTTON:
    ss << " U";
    break;
    
  case V4L2_CTRL_TYPE_BITMASK:
    ss << " K " << qc.maximum << ' ' << qc.default_value << ' ' << ctrl.value;
    break;
    
  case V4L2_CTRL_TYPE_MENU:
  {
    struct v4l2_querymenu querymenu = { };
    querymenu.id = qc.id;
    ss << " M " << qc.default_value << ' ' << ctrl.value;
    for (querymenu.index = qc.minimum; querymenu.index <= (unsigned int)qc.maximum; ++querymenu.index)
    {
      try { itsCamera->queryMenu(querymenu); } catch (...) { strcpy((char *)(querymenu.name), "fixme"); }
      ss << ' ' << querymenu.index << ':' << querymenu.name << ' ';
    }
  }
  break;
  
  default:
    ss << 'X';
  }

  return ss.str();
}

#ifdef JEVOIS_PLATFORM_A33
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
#endif

// ####################################################################################################
void jevois::Engine::reboot()
{
  if (std::system("sync")) LERROR("Disk sync failed -- IGNORED");
  if (std::system("sync")) LERROR("Disk sync failed -- IGNORED");
#ifdef JEVOIS_PLATFORM_A33
  itsCheckingMassStorage.store(false);
#endif
  itsRunning.store(false);

#ifdef JEVOIS_PLATFORM_A33
  // Hard reset to avoid possible hanging during module unload, etc:
  if ( ! std::ofstream("/proc/sys/kernel/sysrq").put('1')) LERROR("Cannot trigger hard reset -- please unplug me!");
  if ( ! std::ofstream("/proc/sysrq-trigger").put('s')) LERROR("Cannot trigger hard reset -- please unplug me!");
  if ( ! std::ofstream("/proc/sysrq-trigger").put('b')) LERROR("Cannot trigger hard reset -- please unplug me!");
#endif

  this->quit();
  //std::terminate();
}

// ####################################################################################################
void jevois::Engine::quit()
{
  // must be locked, camera and gadget must exist:
  itsGadget->abortStream();
  itsCamera->abortStream();
  itsStreaming.store(false);
  itsGadget->streamOff();
  itsCamera->streamOff();
  itsRunning.store(false);

  //std::terminate();
}

// ####################################################################################################
void jevois::Engine::cmdInfo(std::shared_ptr<UserInterface> s, bool showAll, std::string const & pfx)
{
  s->writeString(pfx, "help - print this help message");
  s->writeString(pfx, "help2 - print compact help message about current vision module only");
  s->writeString(pfx, "info - show system information including CPU speed, load and temperature");
  s->writeString(pfx, "setpar <name> <value> - set a parameter value");
  s->writeString(pfx, "getpar <name> - get a parameter value(s)");
  s->writeString(pfx, "runscript <filename> - run script commands in specified file");
  s->writeString(pfx, "setcam <ctrl> <val> - set camera control <ctrl> to value <val>");
  s->writeString(pfx, "getcam <ctrl> - get value of camera control <ctrl>");

  if (showAll || camreg::get())
  {
    s->writeString(pfx, "setcamreg <reg> <val> - set raw camera register <reg> to value <val>");
    s->writeString(pfx, "getcamreg <reg> - get value of raw camera register <reg>");
    s->writeString(pfx, "setimureg <reg> <val> - set raw IMU register <reg> to value <val>");
    s->writeString(pfx, "getimureg <reg> - get value of raw IMU register <reg>");
    s->writeString(pfx, "setimuregs <reg> <num> <val1> ... <valn> - set array of raw IMU register values");
    s->writeString(pfx, "getimuregs <reg> <num> - get array of raw IMU register values");
    s->writeString(pfx, "setdmpreg <reg> <val> - set raw DMP register <reg> to value <val>");
    s->writeString(pfx, "getdmpreg <reg> - get value of raw DMP register <reg>");
    s->writeString(pfx, "setdmpregs <reg> <num> <val1> ... <valn> - set array of raw DMP register values");
    s->writeString(pfx, "getdmpregs <reg> <num> - get array of raw DMP register values");
  }

  s->writeString(pfx, "listmappings - list all available video mappings");
  s->writeString(pfx, "setmapping <num> - select video mapping <num>, only possible while not streaming");
  s->writeString(pfx, "setmapping2 <CAMmode> <CAMwidth> <CAMheight> <CAMfps> <Vendor> <Module> - set no-USB-out "
                 "video mapping defined on the fly, while not streaming");
  s->writeString(pfx, "reload - reload and reset the current module");

  if (showAll || itsCurrentMapping.ofmt == 0 || itsManualStreamon)
  {
    s->writeString(pfx, "streamon - start camera video streaming");
    s->writeString(pfx, "streamoff - stop camera video streaming");
  }

  s->writeString(pfx, "ping - returns 'ALIVE'");
  s->writeString(pfx, "serlog <string> - forward string to the serial port(s) specified by the serlog parameter");
  s->writeString(pfx, "serout <string> - forward string to the serial port(s) specified by the serout parameter");

  if (showAll)
  {
    // Hide machine-oriented commands by default
    s->writeString(pfx, "caminfo - returns machine-readable info about camera parameters");
    s->writeString(pfx, "cmdinfo [all] - returns machine-readable info about Engine commands");
    s->writeString(pfx, "modcmdinfo - returns machine-readable info about Module commands");
    s->writeString(pfx, "paraminfo [hot|mod|modhot] - returns machine-readable info about parameters");
    s->writeString(pfx, "serinfo - returns machine-readable info about serial settings (serout serlog serstyle serprec serstamp)");
    s->writeString(pfx, "fileget <filepath> - get a file from JeVois to the host. Use with caution!");
    s->writeString(pfx, "fileput <filepath> - put a file from the host to JeVois. Use with caution!");
  }
  
#ifdef JEVOIS_PLATFORM_A33
  s->writeString(pfx, "usbsd - export the JEVOIS partition of the microSD card as a virtual USB drive");
#endif
  s->writeString(pfx, "sync - commit any pending data write to microSD");
  s->writeString(pfx, "date [date and time] - get or set the system date and time");

  s->writeString(pfx, "!<string> - execute <string> as a Linux shell command. Use with caution!");
  s->writeString(pfx, "shell <string> - execute <string> as a Linux shell command. Use with caution!");
  s->writeString(pfx, "shellstart - execute all subsequent commands as Linux shell commands. Use with caution!");
  s->writeString(pfx, "shellstop - stop executing all subsequent commands as Linux shell commands.");

#ifdef JEVOIS_PRO
  s->writeString(pfx, "dnnget <key> - download and install a DNN from JeVois Model Converter");
#endif
  
#ifdef JEVOIS_PLATFORM
  s->writeString(pfx, "restart - restart the JeVois smart camera");
#endif

#ifndef JEVOIS_PLATFORM_A33
  s->writeString(pfx, "quit - quit this program");
#endif
}

// ####################################################################################################
void jevois::Engine::modCmdInfo(std::shared_ptr<UserInterface> s, std::string const & pfx)
{
  if (itsModule)
  {    
    std::stringstream css; itsModule->supportedCommands(css);
    for (std::string line; std::getline(css, line); /* */) s->writeString(pfx, line);
  }
}

// ####################################################################################################
bool jevois::Engine::parseCommand(std::string const & str, std::shared_ptr<UserInterface> s, std::string const & pfx)
{
  // itsMtx should be locked by caller

  std::string errmsg;

  // If we are in shell mode, pass any command to the shell except for 'shellstop':
  if (itsShellMode)
  {
    if (str == "shellstop") { itsShellMode = false; return true; }
    
    std::string ret = jevois::system(str, true);
    std::vector<std::string> rvec = jevois::split(ret, "\n");
    for (std::string const & r : rvec) s->writeString(pfx, r);
    return true;
  }
  
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

    // If the string starts with "!", this is like the "shell" command, but parsed differently:
    std::string cmd, rem;
    if (str[0] == '!')
    {
      cmd = "shell"; rem = str.substr(1);
    }
    else
    {
      // Get the first word, i.e., the command:
      size_t const idx = str.find(' ');
      if (idx == str.npos) cmd = str;
      else { cmd = str.substr(0, idx); if (idx < str.length()) rem = str.substr(idx+1); }
    }
  
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "help")
    {
      // Show all commands, first ours, as supported below:
      s->writeString(pfx, "GENERAL COMMANDS:");
      s->writeString(pfx, "");
      cmdInfo(s, false, pfx);
      s->writeString(pfx, "");

      // Then the module's custom commands, if any:
      if (itsModule)
      {
        s->writeString(pfx, "MODULE-SPECIFIC COMMANDS:");
        s->writeString(pfx, "");
        modCmdInfo(s, pfx);
        s->writeString(pfx, "");
      }
      
      // Get the help message for our parameters and write it out line by line so the serial fixes the line endings:
      std::stringstream pss; constructHelpMessage(pss);
      for (std::string line; std::getline(pss, line); /* */) s->writeString(pfx, line);

      // Show all camera controls
      s->writeString(pfx, "AVAILABLE CAMERA CONTROLS:");
      s->writeString(pfx, "");

      foreachCamCtrl([this,&pfx,&s](struct v4l2_queryctrl & qc, std::set<int> & doneids)
                     {
                       try
                       {
                         std::string hlp = camCtrlHelp(qc, doneids);
                         if (hlp.empty() == false) s->writeString(pfx, hlp);
                       } catch (...) { } // silently ignore errors, e.g., some write-only controls
                     });
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "caminfo")
    {
      // Machine-readable list of camera parameters:
      foreachCamCtrl([this,&pfx,&s](struct v4l2_queryctrl & qc, std::set<int> & doneids)
                     {
                       try
                       {
                         std::string hlp = camCtrlInfo(qc, doneids);
                         if (hlp.empty() == false) s->writeString(pfx, hlp);
                       } catch (...) { } // silently ignore errors, e.g., some write-only controls
                     });
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "cmdinfo")
    {
      bool showAll = (rem == "all") ? true : false;
      cmdInfo(s, showAll, pfx);
      return true;
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "modcmdinfo")
    {
      modCmdInfo(s, pfx);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "paraminfo")
    {
      std::map<std::string, std::string> categs;
      bool skipFrozen = (rem == "hot" || rem == "modhot") ? true : false;
      
      if (rem == "mod" || rem == "modhot")
      {
        // Report only on our module's parameter, if any:
        if (itsModule) itsModule->paramInfo(s, categs, skipFrozen, instanceName(), pfx);
      }   
      else
      {
        // Report on all parameters:
        paramInfo(s, categs, skipFrozen, "", pfx);
      }
      
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "serinfo")
    {
      std::string info = getParamStringUnique("serout") + ' ' + getParamStringUnique("serlog");
      if (auto mod = dynamic_cast<jevois::StdModule *>(itsModule.get()))
        info += ' ' + mod->getParamStringUnique("serstyle") + ' ' + mod->getParamStringUnique("serprec") +
          ' ' + mod->getParamStringUnique("serstamp");
      else info += " - - -";
      
      s->writeString(pfx, info);

      return true;
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "help2")
    {
      if (itsModule)
      {
        // Start with the module's commands:
        std::stringstream css; itsModule->supportedCommands(css);
        s->writeString(pfx, "MODULE-SPECIFIC COMMANDS:");
        s->writeString(pfx, "");
        for (std::string line; std::getline(css, line); /* */) s->writeString(pfx, line);
        s->writeString(pfx, "");

        // Now the parameters for that module (and its subs) only:
        s->writeString(pfx, "MODULE PARAMETERS:");
        s->writeString(pfx, "");
        
        // Keep this in sync with Manager::constructHelpMessage():
        std::unordered_map<std::string, // category:description
                           std::unordered_map<std::string, // --name (type) default=[def]
                                              std::vector<std::pair<std::string, // component name
                                                                    std::string  // current param value
                                                                    > > > > helplist;
        itsModule->populateHelpMessage("", helplist);
        
        if (helplist.empty())
          s->writeString(pfx, "None.");
        else
        {
          for (auto const & c : helplist)
          {
            // Print out the category name and description
            s->writeString(pfx, c.first);
            
            // Print out the parameter details
            for (auto const & n : c.second)
            {
              std::vector<std::string> tok = jevois::split(n.first, "[\\r\\n]+");
              bool first = true;
              for (auto const & t : tok)
              {
                // Add current value info to the first thing we write (which is name, default, etc)
                if (first)
                {
                  auto const & v = n.second;
                  if (v.size() == 1) // only one component using this param
                  {
                    if (v[0].second.empty())
                      s->writeString(pfx, t); // only one comp, and using default val
                    else
                      s->writeString(pfx, t + " current=[" + v[0].second + ']'); // using non-default val
                  }
                  else if (v.size() > 1) // several components using this param with possibly different values
                  {
                    std::string sss = t + " current=";
                    for (auto const & pp : v)
                      if (pp.second.empty() == false) sss += '[' + pp.first + ':' + pp.second + "] ";
                    s->writeString(pfx, sss);
                  }
                  else s->writeString(pfx, t); // no non-default value(s) to report
                  
                  first = false;
                }
                
                else // just write out the other lines (param description)
                  s->writeString(pfx, t);
              }
            }
            s->writeString(pfx, "");
          }
        }
      }
      else
        s->writeString(pfx, "No module loaded.");
      
      return true;
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "info")
    {
      s->writeString(pfx, "INFO: JeVois " JEVOIS_VERSION_STRING);
      s->writeString(pfx, "INFO: " + jevois::getSysInfoVersion());
      s->writeString(pfx, "INFO: " + jevois::getSysInfoCPU());
      s->writeString(pfx, "INFO: " + jevois::getSysInfoMem());
      if (itsModule) s->writeString(pfx, "INFO: " + itsCurrentMapping.str());
      else s->writeString(pfx, "INFO: " + jevois::VideoMapping().str());
      return true;
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setpar")
    {
      size_t const remidx = rem.find(' ');
      if (remidx != rem.npos)
      {
        std::string const desc = rem.substr(0, remidx);
        if (remidx < rem.length())
        {
          std::string const val = rem.substr(remidx+1);
          setParamString(desc, val);
          return true;
        }
      }
      errmsg = "Need to provide a parameter name and a parameter value in setpar";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getpar")
    {
      auto vec = getParamString(rem);
      for (auto const & p : vec) s->writeString(pfx, p.first + ' ' + p.second);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setcam")
    {
      std::istringstream ss(rem); std::string ctrl; int val; ss >> ctrl >> val;
      struct v4l2_control c = { }; c.id = camctrlid(ctrl); c.value = val;

      // For ispsensorpreset, need first to set it to non-zero before we set it to zero, otherwise ignored...
      if (val == 0 && ctrl == "ispsensorpreset")
      {
        c.value = 1; itsCamera->setControl(c);
        c.value = 0; itsCamera->setControl(c);
      }
      else itsCamera->setControl(c);

      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getcam")
    {
      struct v4l2_control c = { }; c.id = camctrlid(rem);
      itsCamera->getControl(c);
      s->writeString(pfx, rem + ' ' + std::to_string(c.value));
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setcamreg")
    {
      if (camreg::get())
      {
        auto cam = std::dynamic_pointer_cast<jevois::Camera>(itsCamera);
        if (cam)
        {
          // Read register and value as strings, then std::stoi to int, supports 0x (and 0 for octal, caution)
          std::istringstream ss(rem); std::string reg, val; ss >> reg >> val;
          cam->writeRegister(std::stoi(reg, nullptr, 0), std::stoi(val, nullptr, 0));
          return true;
        }
        else errmsg = "Not using a camera for video input";
      }
      else errmsg = "Access to camera registers is disabled, enable with: setpar camreg true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getcamreg")
    {
      if (camreg::get())
      {
        auto cam = std::dynamic_pointer_cast<jevois::Camera>(itsCamera);
        if (cam)
        {
          unsigned int val = cam->readRegister(std::stoi(rem, nullptr, 0));
          std::ostringstream os; os << std::hex << val;
          s->writeString(pfx, os.str());
          return true;
        }
        else errmsg = "Not using a camera for video input";
      }
      else errmsg = "Access to camera registers is disabled, enable with: setpar camreg true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setimureg")
    {
      if (imureg::get())
      {
        if (itsIMU)
        {
          // Read register and value as strings, then std::stoi to int, supports 0x (and 0 for octal, caution)
          std::istringstream ss(rem); std::string reg, val; ss >> reg >> val;
          itsIMU->writeRegister(std::stoi(reg, nullptr, 0), std::stoi(val, nullptr, 0));
          return true;
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getimureg")
    {
      if (imureg::get())
      {
        if (itsIMU)
        {
          unsigned int val = itsIMU->readRegister(std::stoi(rem, nullptr, 0));
          std::ostringstream os; os << std::hex << val;
          s->writeString(pfx, os.str());
          return true;
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setimuregs")
    {
      if (imureg::get())
      {
        if (itsIMU)
        {
          // Read register and value as strings, then std::stoi to int, supports 0x (and 0 for octal, caution)
          std::vector<std::string> v = jevois::split(rem);
          if (v.size() < 3) errmsg = "Malformed arguments, need at least 3"; 
          else
          {
            unsigned short reg = std::stoi(v[0], nullptr, 0);
            size_t num = std::stoi(v[1], nullptr, 0);
            if (num > 32) errmsg = "Maximum transfer size is 32 bytes";
            else if (num != v.size() - 2) errmsg = "Incorrect number of data bytes, should pass " + v[1] + " values.";
            else
            {
              unsigned char data[32];
              for (size_t i = 2; i < v.size(); ++i) data[i-2] = std::stoi(v[i], nullptr, 0) & 0xff;
              
              itsIMU->writeRegisterArray(reg, data, num);
              return true;
            }
          }
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getimuregs")
    {
      if (imureg::get())
      {
        if (itsIMU)
        {
          std::istringstream ss(rem); std::string reg, num; ss >> reg >> num;
          int n = std::stoi(num, nullptr, 0);
          
          if (n > 32) errmsg = "Maximum transfer size is 32 bytes";
          else
          {
            unsigned char data[32];
            itsIMU->readRegisterArray(std::stoi(reg, nullptr, 0), data, n);
            
            std::ostringstream os; os << std::hex;
            for (int i = 0; i < n; ++i) os << (unsigned int)(data[i]) << ' ';
            s->writeString(pfx, os.str());
            return true;
          }
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setdmpreg")
    {
      if (imureg::get())
      {
        if (itsIMU)
        {
          // Read register and value as strings, then std::stoi to int, supports 0x (and 0 for octal, caution)
          std::istringstream ss(rem); std::string reg, val; ss >> reg >> val;
          itsIMU->writeDMPregister(std::stoi(reg, nullptr, 0), std::stoi(val, nullptr, 0));
          return true;
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getdmpreg")
    {
      if (camreg::get())
      {
        if (itsIMU)
        {
          unsigned int val = itsIMU->readDMPregister(std::stoi(rem, nullptr, 0));
          std::ostringstream os; os << std::hex << val;
          s->writeString(pfx, os.str());
          return true;
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setdmpregs")
    {
      if (camreg::get())
      {
        if (itsIMU)
        {
          // Read register and value as strings, then std::stoi to int, supports 0x (and 0 for octal, caution)
          std::vector<std::string> v = jevois::split(rem);
          if (v.size() < 3) errmsg = "Malformed arguments, need at least 3"; 
          else
          {
            unsigned short reg = std::stoi(v[0], nullptr, 0);
            size_t num = std::stoi(v[1], nullptr, 0);
            if (num > 32) errmsg = "Maximum transfer size is 32 bytes";
            else if (num != v.size() - 2) errmsg = "Incorrect number of data bytes, should pass " + v[1] + " values.";
            else
            {
              unsigned char data[32];
              for (size_t i = 2; i < v.size(); ++i) data[i-2] = std::stoi(v[i], nullptr, 0) & 0xff;
              
              itsIMU->writeDMPregisterArray(reg, data, num);
              return true;
            }
          }
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "getdmpregs")
    {
      if (imureg::get())
      {
        if (itsIMU)
        {
          std::istringstream ss(rem); std::string reg, num; ss >> reg >> num;
          int n = std::stoi(num, nullptr, 0);
          
          if (n > 32) errmsg = "Maximum transfer size is 32 bytes";
          else
          {
            unsigned char data[32];
            itsIMU->readDMPregisterArray(std::stoi(reg, nullptr, 0), data, n);
            
            std::ostringstream os; os << std::hex;
            for (int i = 0; i < n; ++i) os << (unsigned int)(data[i]) << ' ';
            s->writeString(pfx, os.str());
            return true;
          }
        }
        else errmsg = "No IMU driver loaded";
      }
      else errmsg = "Access to IMU registers is disabled, enable with: setpar imureg true";
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "listmappings")
    {
      s->writeString(pfx, "AVAILABLE VIDEO MAPPINGS:");
      s->writeString(pfx, "");
      for (size_t idx = 0; idx < itsMappings.size(); ++idx)
      {
        std::string idxstr = std::to_string(idx);
        if (idxstr.length() < 5) idxstr = std::string(5 - idxstr.length(), ' ') + idxstr; // pad to 5-char long
        s->writeString(pfx, idxstr + " - " + itsMappings[idx].str());
      }
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setmapping")
    {
      size_t const idx = std::stoi(rem);

      if (itsStreaming.load() && itsCurrentMapping.ofmt)
        errmsg = "Cannot set mapping while streaming: Stop your webcam program on the host computer first.";
      else if (idx >= itsMappings.size())
        errmsg = "Requested mapping index " + std::to_string(idx) + " out of range [0 .. " +
          std::to_string(itsMappings.size()-1) + ']';
      else
      {
        try
        {
          setFormatInternal(idx);
          return true;
        }
        catch (std::exception const & e) { errmsg = "Error parsing or setting mapping [" + rem + "]: " + e.what(); }
        catch (...) { errmsg = "Error parsing or setting mapping [" + rem + ']'; }
      }
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "setmapping2")
    {
      if (itsStreaming.load() && itsCurrentMapping.ofmt)
        errmsg = "Cannot set mapping while streaming: Stop your webcam program on the host computer first.";
      else
      {
        try
        {
          jevois::VideoMapping m; std::istringstream full("NONE 0 0 0.0 " + rem); full >> m;
          setFormatInternal(m);
          return true;
        }
        catch (std::exception const & e) { errmsg = "Error parsing or setting mapping [" + rem + "]: " + e.what(); }
        catch (...) { errmsg = "Error parsing or setting mapping [" + rem + ']'; }
      }
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "reload")
    {
      setFormatInternal(itsCurrentMapping, true);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (itsCurrentMapping.ofmt == 0 || itsCurrentMapping.ofmt == JEVOISPRO_FMT_GUI || itsManualStreamon)
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
      s->writeString(pfx, "ALIVE");
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
#ifdef JEVOIS_PLATFORM_A33
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
      if (std::system("sync")) errmsg = "Disk sync failed";
      else return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "date")
    {
      std::string dat = jevois::system("/bin/date " + rem);
      s->writeString(pfx, "date now " + dat.substr(0, dat.size()-1)); // skip trailing newline
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "runscript")
    {
      std::string const fname = itsModule ? itsModule->absolutePath(rem).string() : rem;
      
      try { runScriptFromFile(fname, s, true); return true; }
      catch (...) { errmsg = "Script " + fname + " execution failed"; }
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "shell")
    {
      std::string ret = jevois::system(rem, true);
      std::vector<std::string> rvec = jevois::split(ret, "\n");
      for (std::string const & r : rvec) s->writeString(pfx, r);
      return true;
    }

    // ----------------------------------------------------------------------------------------------------
    if (cmd == "shellstart")
    {
      itsShellMode = true;
      return true;
      // note: shellstop is handled above
    }

#ifdef JEVOIS_PRO
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "dnnget")
    {
      if (rem.length() != 4 || std::regex_match(rem, std::regex("^[a-zA-Z0-9]+$")) == false)
        errmsg = "Key must be a 4-character alphanumeric string, as emailed to you by the model converter.";
      else
      {
        // Download the zip using curl:
        s->writeString(pfx, "Downloading custom DNN model " + rem + " ...");
        std::string const zip = rem + ".zip";
        std::string ret = jevois::system("/usr/bin/curl " JEVOIS_CUSTOM_DNN_URL "/" + zip + " -o "
                                         JEVOIS_CUSTOM_DNN_PATH "/" + zip, true);
        std::vector<std::string> rvec = jevois::split(ret, "\n");
        for (std::string const & r : rvec) s->writeString(pfx, r);

        // Check that the file exists:
        std::ifstream ifs(JEVOIS_CUSTOM_DNN_PATH "/" + zip);
        if (ifs.is_open() == false)
          errmsg = "Failed to download. Check network connectivity and available disk space.";
        else
        {
          // Unzip it:
          s->writeString(pfx, "Unpacking custom DNN model " + rem + " ...");
          ret = jevois::system("/usr/bin/unzip -o " JEVOIS_CUSTOM_DNN_PATH "/" + zip +
                               " -d " JEVOIS_CUSTOM_DNN_PATH, true);
          rvec = jevois::split(ret, "\n"); for (std::string const & r : rvec) s->writeString(pfx, r);

          ret = jevois::system("/bin/rm " JEVOIS_CUSTOM_DNN_PATH "/" + zip, true);
          rvec = jevois::split(ret, "\n"); for (std::string const & r : rvec) s->writeString(pfx, r);
          
          s->writeString(pfx, "Reload your model zoo for changes to take effect.");
          
          return true;
        }
      }
    }
#endif
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "fileget")
    {
      std::shared_ptr<jevois::Serial> ser = std::dynamic_pointer_cast<jevois::Serial>(s);
      if (!ser)
        errmsg = "File transfer only supported over USB or Hard serial ports";
      else
      {
        std::string const abspath = itsModule ? itsModule->absolutePath(rem).string() : rem;
        ser->fileGet(abspath);
        return true;
      }
    }
    
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "fileput")
    {
      std::shared_ptr<jevois::Serial> ser = std::dynamic_pointer_cast<jevois::Serial>(s);
      if (!ser)
        errmsg = "File transfer only supported over USB or Hard serial ports";
      else
      {
        std::string const abspath = itsModule ? itsModule->absolutePath(rem).string() : rem;
        ser->filePut(abspath);
        if (std::system("sync")) { } // quietly ignore any errors on sync
        return true;
      }
    }
    
#ifdef JEVOIS_PLATFORM
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "restart")
    {
      s->writeString(pfx, "Restart command received - bye-bye!");
      
      if (itsStreaming.load())
        s->writeString(pfx, "ERR Video streaming is on - you should quit your video viewer before rebooting");

      if (std::system("sync")) s->writeString(pfx, "ERR Disk sync failed -- IGNORED");

#ifdef JEVOIS_PLATFORM_A33
      // Turn off the SD storage if it is there:
      std::ofstream(JEVOIS_USBSD_SYS).put('\n'); // ignore errors

      if (std::system("sync")) s->writeString(pfx, "ERR Disk sync failed -- IGNORED");
#endif
      
      // Hard reboot:
      this->reboot();
      return true;
    }
    // ----------------------------------------------------------------------------------------------------
#endif

#ifndef JEVOIS_PLATFORM_A33
    // ----------------------------------------------------------------------------------------------------
    if (cmd == "quit")
    {
      s->writeString(pfx, "Quit command received - bye-bye!");
      this->quit();
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

  // We need to identify a serial to send any errors to, if none was given to us. Let's use the GUI console, or the
  // serial in serlog, or, if none is specified there, the first available serial:
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

#ifdef JEVOIS_PRO
    if (itsGUIhelper)
      for (auto & s : itsSerials) if (s->type() == jevois::UserInterface::Type::GUI) { ser = s; break; }
#endif
    
    if (!ser) ser = itsSerials.front();
  }
  
  // Ok, run the script, plowing through any errors:
  size_t linenum = 0;
  for (std::string line; std::getline(ifs, line); /* */)
  {
    ++linenum;

    // Strip any extra whitespace at end, which could be a CR if the file was edited in Windows:
    line = jevois::strip(line);
    
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
  }
}

// ####################################################################################################
#ifdef JEVOIS_PRO
// ####################################################################################################
void jevois::Engine::drawCameraGUI()
{
  ImGui::Columns(2, "camctrl");

  foreachCamCtrl([this](struct v4l2_queryctrl & qc, std::set<int> & doneids)
                 {
                   try { camCtrlGUI(qc, doneids); } catch (...) { }
                 });

  ImGui::Columns(1);
}

// ####################################################################################################
void jevois::Engine::camCtrlGUI(struct v4l2_queryctrl & qc, std::set<int> & doneids)
{
  // See if we have this control:
  itsCamera->queryControl(qc);
  qc.id &= ~V4L2_CTRL_FLAG_NEXT_CTRL;

  // If we have already done this control, just return:
  if (doneids.find(qc.id) != doneids.end()) return; else doneids.insert(qc.id);
  
  // Control exists, let's also get its current value:
  struct v4l2_control ctrl = { }; ctrl.id = qc.id;
  itsCamera->getControl(ctrl);

  // Instantiate widgets depending on control type:
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(reinterpret_cast<char const *>(qc.name));
  ImGui::NextColumn();

  // Grey out the item if it is disabled:
  if (qc.flags & V4L2_CTRL_FLAG_DISABLED)
  {
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
  }

  // We need a unique ID for each ImGui widget, and we will use no visible widget name:
  static char wname[16]; snprintf(wname, 16, "##c%d", ctrl.id);
  bool reset = false; // will set to true if we want a reset button
  
  switch (qc.type)
  {
  case V4L2_CTRL_TYPE_INTEGER:
  case V4L2_CTRL_TYPE_INTEGER_MENU:
  {
    // Do a slider if range is reasonable, otherwise typein:
    long range = long(qc.maximum) - long(qc.minimum);
    if (range > 1 && range < 5000)
    {
      if (ImGui::SliderInt(wname, &ctrl.value, qc.minimum, qc.maximum)) itsCamera->setControl(ctrl);
      reset = true;
    }
    else
    {
      if (ImGui::InputInt(wname, &ctrl.value, qc.step, qc.step * 2)) itsCamera->setControl(ctrl);
      reset = true;
    }
  }
  break;
    
  //case V4L2_CTRL_TYPE_INTEGER64:
  //{
  //  double val = ctrl.value64;
  //  if (ImGui::InputDouble(wname, &val)) { ctrl.value64 = long(val + 0.4999); itsCamera->setControl(ctrl); }
  //}
  //break;
  
  //case V4L2_CTRL_TYPE_STRING:
  //  if (ImGui::InputText(wname, ctrl.string, sizeof(ctrl.string))) itsCamera->setControl(ctrl);
  //  break;
    
  case V4L2_CTRL_TYPE_BOOLEAN:
  {
    bool checked = (ctrl.value != 0);
    if (ImGui::Checkbox(wname, &checked)) { ctrl.value = checked ? 1 : 0; itsCamera->setControl(ctrl); }
  }
  break;

      
  case V4L2_CTRL_TYPE_BUTTON:
    static char bname[16]; snprintf(bname, 16, "Go##%d", ctrl.id);
    if (ImGui::Button(bname)) { ctrl.value = 1; itsCamera->setControl(ctrl); }
    break;
    
  case V4L2_CTRL_TYPE_BITMASK:
    ///ss << " K " << qc.maximum << ' ' << qc.default_value << ' ' << ctrl.value;
    break;
    
  case V4L2_CTRL_TYPE_MENU:
  {
    struct v4l2_querymenu querymenu = { };
    querymenu.id = qc.id;
    char * items[qc.maximum - qc.minimum + 1];
    
    for (querymenu.index = qc.minimum; querymenu.index <= (unsigned int)qc.maximum; ++querymenu.index)
    {
      try { itsCamera->queryMenu(querymenu); } catch (...) { strncpy((char *)querymenu.name, "fixme", 32); }
      items[querymenu.index] = new char[32];
      strncpy(items[querymenu.index], (char const *)querymenu.name, 32);
    }
    
    int idx = ctrl.value - qc.minimum;
    if (ImGui::Combo(wname, &idx, items, qc.maximum - qc.minimum + 1))
    { ctrl.value = qc.minimum + idx; itsCamera->setControl(ctrl); }
    
    for (int i = qc.minimum; i <= qc.maximum; ++i) delete [] items[i];
  }
  break;
  
  default: break;
  }

  // Add a reset button if desired:
  if (reset)
  {
    static char rname[16]; snprintf(rname, 16, "Reset##%d", ctrl.id);
    ImGui::SameLine();
    if (ImGui::Button(rname)) { ctrl.value = qc.default_value; itsCamera->setControl(ctrl); }
  }
  
  // Restore any grey out:
  if (qc.flags & V4L2_CTRL_FLAG_DISABLED)
  {
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
  }

  // Ready for next row:
  ImGui::NextColumn();
}

// ####################################################################################################
#endif // JEVOIS_PRO

// ####################################################################################################
void jevois::Engine::registerPythonComponent(jevois::Component * comp, void * pyinst)
{
  LDEBUG(comp->instanceName() << " -> " << std::hex << pyinst);
  std::lock_guard<std::mutex> _(itsPyRegMtx);
  auto itr = itsPythonRegistry.find(pyinst);
  if (itr != itsPythonRegistry.end()) LFATAL("Trying to register twice -- ABORT");
  itsPythonRegistry.insert(std::make_pair(pyinst, comp));
}

// ####################################################################################################
void jevois::Engine::unRegisterPythonComponent(Component * comp)
{
  LDEBUG(comp->instanceName());
  std::lock_guard<std::mutex> _(itsPyRegMtx);
  auto itr = itsPythonRegistry.begin(), stop = itsPythonRegistry.end();
  while (itr != stop) if (itr->second == comp) itr = itsPythonRegistry.erase(itr); else ++itr;
}
  
// ####################################################################################################
jevois::Component * jevois::Engine::getPythonComponent(void * pyinst) const
{
  LDEBUG(std::hex << pyinst);
  std::lock_guard<std::mutex> _(itsPyRegMtx);
  auto itr = itsPythonRegistry.find(pyinst);
  if (itr == itsPythonRegistry.end()) LFATAL("Python instance not registered -- ABORT");
  return itr->second;
}
