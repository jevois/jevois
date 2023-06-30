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

#include <jevois/Core/Module.H>
#include <jevois/Image/RawImageOps.H>
#include <jevois/Debug/Timer.H>

//! JeVois sample module
/*! This module is provided as an example of how to create a new standalone module.

    By default, we get the next video frame from the camera as an OpenCV BGR (color) image named 'inimg'.
    We then apply some image processing to it to create an overlay in Pro/GUI mode, an output BGR image named
    'outimg' in Legacy mode, or no image in Headless mode.
    
    - In Legacy mode (JeVois-A33 or JeVois-Pro acts as a webcam connected to a host): process(inframe, outframe) is
      called on every frame. A video frame from the camera sensor is given in 'inframe' and the process() function
      create an output frame that is sent over USB to the host computer (JeVois-A33) or displayed (JeVois-Pro).
    
    - In Pro/GUI mode (JeVois-Pro is connected to an HDMI display): process(inframe, helper) is called on every frame. A
      video frame from the camera is given, as well as a GUI helper that can be used to create overlay drawings.
    
    - In Headless mode (JeVois-A33 or JeVois-Pro only produces text messages over serial port, no video output):
      process(inframe) is called on every frame. A video frame from the camera is given, and the module sends messages
      over serial to report what it sees.
    
    Which mode is activated depends on which VideoMapping was selected by the user. The VideoMapping specifies camera
    format and framerate, and what kind of mode and output format to use.
    
    See http://jevois.org/tutorials for tutorials on getting started with programming JeVois in Python without having
    to install any development software on your host computer.
    
    @author __AUTHOR__
    
    @videomapping __VIDEOMAPPING__
    @email __EMAIL__
    @address fixme
    @copyright Copyright (C) 2023 by __AUTHOR__
    @mainurl __WEBSITE__
    @supporturl 
    @otherurl 
    @license __LICENSE__
    @distribution Unrestricted
    @restrictions None
    @ingroup modules */
class __MODULE__ : public jevois::StdModule
{
  public:
    //! Default base class constructor ok
    using jevois::StdModule::StdModule;

    //! Virtual destructor for safe inheritance
    virtual ~__MODULE__() { }

    // ####################################################################################################
    //! Processing function
    // ####################################################################################################
    virtual void process(jevois::InputFrame && inframe, jevois::OutputFrame && outframe) override
    {
      // Wait for next available camera image:
      jevois::RawImage const inimg = inframe.get(true);

      // We only support YUYV pixels in this example, any resolution:
      inimg.require("input", inimg.width, inimg.height, V4L2_PIX_FMT_YUYV);
      
      // Wait for an image from our gadget driver into which we will put our results:
      jevois::RawImage outimg = outframe.get();

      // Enforce that the input and output formats and image sizes match:
      outimg.require("output", inimg.width, inimg.height, inimg.fmt);
      
      // Just copy the pixel data over:
      memcpy(outimg.pixelsw<void>(), inimg.pixels<void>(), std::min(inimg.buf->length(), outimg.buf->length()));

      // Print a text message:
      jevois::rawimage::writeText(outimg, "Hello JeVois!", 100, 230, jevois::yuyv::White, jevois::rawimage::Font20x38);
      
      // Let camera know we are done processing the input image:
      inframe.done(); // NOTE: optional here, inframe destructor would call it anyway

      // Send the output image with our processing results to the host over USB:
      outframe.send(); // NOTE: optional here, outframe destructor would call it anyway
    }

    // ####################################################################################################
    //! Processing function with no USB output (headless)
    // ####################################################################################################
    virtual void process(jevois::InputFrame && inframe) override
    {
      // Wait for next available camera image:
      jevois::RawImage const inimg = inframe.get(true);
      
      // Do something with the frame...
      
      // ...

      // Let camera know we are done processing the input image:
      inframe.done(); // NOTE: optional here, inframe destructor would call it anyway
      
      // Send some results to serial port:
      sendSerial("This is an example -- fixme");
    }
    
#ifdef JEVOIS_PRO
    // ####################################################################################################
    //! Processing function with zero-copy and GUI on JeVois-Pro
    // ####################################################################################################
    virtual void process(jevois::InputFrame && inframe, jevois::GUIhelper & helper) override
    {
      static jevois::Timer timer("processing", 100, LOG_DEBUG);
      
      // Start the GUI frame: will return display size in winw, winh, and whether mouse/keyboard are idle
      unsigned short winw, winh;
      bool idle = helper.startFrame(winw, winh);

      // Draw the camera frame: will return its location (x,y) and size (iw,ih) on screen
      int x = 0, y = 0; unsigned short iw = 0, ih = 0;
      helper.drawInputFrame("camera", inframe, x, y, iw, ih);

      // Wait for next available camera image to be used for processing (possibly at a lower resolution):
      jevois::RawImage const inimg = inframe.getp();
      helper.itext("JeVois-Pro Sample module");
     
      timer.start();

      // Process inimg and draw some results in the GUI, possibly send some serial messages too...

      // ...
      
      // Show processing fps:
      std::string const & fpscpu = timer.stop();
      helper.iinfo(inframe, fpscpu, winw, winh);
      
      // Render the image and GUI:
      helper.endFrame();

      (void)idle; // avoid compiler warning since we did not use idle in this example
    }

#endif // JEVOIS_PRO
};

// Allow the module to be loaded as a shared object (.so) file:
JEVOIS_REGISTER_MODULE(__MODULE__);
