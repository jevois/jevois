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

#include <iosfwd>
#include <vector>
#include <string>
#include <linux/videodev2.h> // for v4l2_fract

namespace jevois
{
  //! Simple struct to hold video mapping definitions for the processing Engine
  /*! This struct specifies an output video format, resolution, and frame rate (to be send to the end user over USB),
      the corresponding camera capture video format, resolution and frame rate, and the Module to use to process the
      camera frames and generate the corresponding output frames. This class also provides conversion functions between
      frame rate and frame interval periods for both USB and V4L2, which use different units.

      Operation of JeVois is based on a list of available VideoMapping definitions, which is configured in a file called
      JEVOIS:config/videomappings.cfg on the microSD card. The video mappings indicate which output formats are exposed
      to the host computer connected over USB, and which corresponding camera format and vision processing module should
      be used when a given output format is selected by video capture software running on the host computer.

      See \ref UserModes for explanations about how to organize videomappings.cfg

      \ingroup core */
  struct VideoMapping
  {
      unsigned int ofmt; //!< output pixel format, or 0 for no output over USB
      unsigned int ow;   //!< output width
      unsigned int oh;   //!< output height
      float ofps;        //!< output frame rate in frames/sec

      unsigned int cfmt; //!< camera pixel format
      unsigned int cw;   //!< camera width
      unsigned int ch;   //!< camera height
      float cfps;        //!< camera frame rate in frames/sec

      unsigned int uvcformat; //!< USB-UVC format number (1-based)
      unsigned int uvcframe;  //!< USB UVC frame number (1-based)

      std::string vendor; //!< Module creator name, used as a directory to organize the modules

      std::string modulename; //!< Name of the Module that will process this mapping

      bool ispython;      //!< True if the module is written in Python; affects behavior of sopath() only
      
      //! Return the full absolute path and file name of the module's .so or .py file
      std::string sopath() const;

      //! Return the size in bytes of an output image
      unsigned int osize() const;

      //! Return the size in bytes of a camera image
      unsigned int csize() const;
      
      //! Convert from USB/UVC interval to fps
      /*! This function rounds to the nearest 1/100 fps. */
      static float uvcToFps(unsigned int interval);

      //! Convert from fps to USB/UVC interval
      static unsigned int fpsToUvc(float fps);

      //! Convert from V4L2 interval to fps
      /*! This function rounds to the nearest 1/100 fps. */
      static float v4l2ToFps(struct v4l2_fract const & interval);

      //! Convert from fps to V4L2 interval
      static struct v4l2_fract fpsToV4l2(float fps);

      //! Return true if this VideoMapping's output format is a match to the given output parameters
      bool match(unsigned int oformat, unsigned int owidth, unsigned int oheight, float oframespersec) const;

      //! Convenience function to print out FCC WxH @ fps, for the output (UVC) format
      std::string ostr() const;
      
      //! Convenience function to print out FCC WxH @ fps, for the input (camera) format
      std::string cstr() const;

      //! Convenience function to print out the whole mapping in a human-friendly way
      std::string str() const;

      //! Equality operator for specs but not vendor or module name
      /*! Note that two mappings will be declared to match if their fps values are within 0.01fps, to avoid mismatches
          due to floating point representation and rounding. */
      bool hasSameSpecsAs(VideoMapping const & other) const;

      //! Equality operator for specs and also vendor or module name
      /*! Note that two mappings will be declared to match if their fps values are within 0.01fps, to avoid mismatches
          due to floating point representation and rounding. */
      bool isSameAs(VideoMapping const & other) const;
  };

  //! Stream a VideoMapping out, intended for machines
  /*! Note that no std::endl is issued at the end.
      \relates jevois::VideoMapping */
  std::ostream & operator<<(std::ostream & out, VideoMapping const & m);

  //! Stream a VideoMapping in, intended for machines
  /*! Note that the assumption is that the mapping is clean (no extra garbage).
      \relates jevois::VideoMapping */
  std::istream & operator>>(std::istream & in, VideoMapping & m);

  //! Load all the video mappings from the default config file
  /*! \relates VideoMapping */
  std::vector<VideoMapping> loadVideoMappings(size_t & defidx, bool checkso = true);
  
  //! Parse all the mappings in a config file and also indicate which one is the default
  /*! The contents of the file are sorted so that the resulting vector is ordered by increasing 1) format fcc, then 2)
      resolution (from large to small, looking at x first), and 3) framerate (from high to low). 

      In case of duplicate output formats, frame rates will be decreased by 1fps for each additional duplicate. This is
      because we need to present the host computer with distinct video formats so that users can select the one they
      want. For example:
      \verbatim
      YUYV 320 240 60.0 YUYV 320 240 60.0 JeVois SaveVideo
      YUYV 320 240 60.0 YUYV 320 240 60.0 VendorX MyModule
      YUYV 320 240 60.0 YUYV 320 240 60.0 VendorY MyModule
      YUYV 320 240 60.0 YUYV 320 240 60.0 VendorZ MyModule
      \endverbatim

      will be disambiguated into:

      \verbatim
      YUYV 320 240 60.0 YUYV 320 240 60.0 JeVois SaveVideo
      YUYV 320 240 59.0 YUYV 320 240 60.0 VendorX MyModule
      YUYV 320 240 58.0 YUYV 320 240 60.0 VendorY MyModule
      YUYV 320 240 57.0 YUYV 320 240 60.0 VendorZ MyModule
      \endverbatim

      and in \b guvcview or similar program running on a host computer, these 4 mappings will be available since they
      correspond to 4 different framerates. It is recommended that you issue a \b listmapping command in the JeVois
      command-line interface to confirm the final mappings that are used at runtime after any adjustments; see \ref
      UserCli for details.

      defidx is the index of the default format in the resulting vector of mappings. If several default formats are
      specified, the first one prevails. 

      See \ref UserModes for explanations about how to organize \b videomappings.cfg
      \relates VideoMapping */
  std::vector<VideoMapping> videoMappingsFromStream(std::istream & is, size_t & defidx, bool checkso = true);
}

