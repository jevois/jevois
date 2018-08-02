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

#include <jevois/Core/Module.H>
#include <jevois/Core/VideoMapping.H>
#include <boost/python.hpp>

namespace jevois
{
  class Engine;
  
  /*! \defgroup python Support for JeVois modules written in Python
  
      In addition to writing modules in C++, JeVois supports writing modules in Python. JeVois provides two-way
      mappings:

      - C++ functions and classes of JeVois become accessible in Python by importing Python package \p libjevois
      - The JeVois engine can directly invoke class member functions of a Python class implementing a machine vision
        processing module

      \ingroup core */

  //! Wrapper around InputFrame to be used by Python
  /*! This wrapper is to work around the lack of move semantics in our Python support. This class is not intended for
      general use, but only for use by PythonModule. Users of this class must ensure that the original InputFrame will
      outlive any and all InputFramePython instances, since InputFramePython just references to the source InputFrame by
      unprotected raw pointer. Although the C++ object is called InputFramePython, we will expose it to python under
      the name InputFrame (see PythonSupport.C). \ingroup python */
  class InputFramePython
  {
    public:
      //! Default constructor to keep boost::python happy, object is not operational
      InputFramePython() = default;
      
      //! Construct from a regular (move-only) InputFrame that shoube be coming from Engine
      InputFramePython(InputFrame * src);
      
      //! Get the next captured camera image, thin wrapper for default arg value
      RawImage const & get1(bool casync) const;

      //! Get the next captured camera image, thin wrapper for default arg value
      RawImage const & get() const;

      //! Indicate that user processing is done with the image previously obtained via get()
      void done() const;

      //! Shorthand to get the input image as a GRAY cv::Mat and release the raw buffer
      cv::Mat getCvGRAY1(bool casync) const;

      //! Shorthand to get the input image as a GRAY cv::Mat and release the raw buffer
      cv::Mat getCvGRAY() const;

      //! Shorthand to get the input image as a BGR cv::Mat and release the raw buffer
      cv::Mat getCvBGR1(bool casync) const;

      //! Shorthand to get the input image as a BGR cv::Mat and release the raw buffer
      cv::Mat getCvBGR() const;

      //! Shorthand to get the input image as a RGB cv::Mat and release the raw buffer
      cv::Mat getCvRGB1(bool casync) const;

      //! Shorthand to get the input image as a RGB cv::Mat and release the raw buffer
      cv::Mat getCvRGB() const;

      //! Shorthand to get the input image as a RGBA cv::Mat and release the raw buffer
      cv::Mat getCvRGBA1(bool casync) const;

      //! Shorthand to get the input image as a RGBA cv::Mat and release the raw buffer
      cv::Mat getCvRGBA() const;

    private:
      InputFrame * itsInputFrame;
  };
  
  //! Wrapper around OutputFrame to be used by Python
  /*! This wrapper is to work around the lack of move semantics in our Python support. This class is not intended for
      general use, but only for use by PythonModule. Users of this class must ensure that the original OutputFrame will
      outlive any and all OutputFramePython instances, since OutputFramePython just references to the source OutputFrame
      by unprotected raw pointer. Although the C++ object is called OutputFramePython, we will expose it to python
      under the name OutputFrame (see PythonSupport.C). \ingroup python */
  class OutputFramePython
  {
    public:
      //! Default constructor to keep boost::python happy, object is not operational
      OutputFramePython() = default;
      
      //! Construct from a regular (move-only) OutputFrame that shoube be coming from Engine
      OutputFramePython(OutputFrame * src);
      
      //! Get the next captured camera image
      RawImage const & get() const;

      //! Indicate that user processing is done with the image previously obtained via get()
      void send() const;

      //! Shorthand to send a cv::Mat after scaling/converting it to the current output format
      /* The pixel format of the given cv::Mat is guessed as follows:

	  - if img.type() == CV_8UC3, assume BGR pixels
	  - if img.type() == CV_8UC1, assume GRAY pixels
	  - if img.type() == CV_8UC4, assume RGBA pixels

	  If this is not what you want (e.g., you have CV_8UC3 but RGB pixels instead of BGR, then  use the 
	  other, more specialized sendScaledCv...() functions. */
      void sendCv1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a cv::Mat after scaling/converting it to the current output format
      /* The pixel format of the given cv::Mat is guessed as follows:

	  - if img.type() == CV_8UC3, assume BGR pixels
	  - if img.type() == CV_8UC1, assume GRAY pixels
	  - if img.type() == CV_8UC4, assume RGBA pixels

	  If this is not what you want (e.g., you have CV_8UC3 but RGB pixels instead of BGR, then  use the 
	  other, more specialized sendScaledCv...() functions. */
      void sendCv(cv::Mat const & img) const;

      //! Shorthand to send a GRAY cv::Mat after converting it to the current output format
      void sendCvGRAY1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a GRAY cv::Mat after converting it to the current output format
      void sendCvGRAY(cv::Mat const & img) const;

      //! Shorthand to send a BGR cv::Mat after converting it to the current output format
      void sendCvBGR1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a BGR cv::Mat after converting it to the current output format
      void sendCvBGR(cv::Mat const & img) const;

      //! Shorthand to send a RGB cv::Mat after converting it to the current output format
      void sendCvRGB1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a RGB cv::Mat after converting it to the current output format
      void sendCvRGB(cv::Mat const & img) const;

      //! Shorthand to send a RGBA cv::Mat after converting it to the current output format
      void sendCvRGBA1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a RGBA cv::Mat after converting it to the current output format
      void sendCvRGBA(cv::Mat const & img) const;

      //! Shorthand to send a GRAY cv::Mat after scaling/converting it to the current output format
      void sendScaledCvGRAY1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a GRAY cv::Mat after scaling/converting it to the current output format
      void sendScaledCvGRAY(cv::Mat const & img) const;

      //! Shorthand to send a BGR cv::Mat after scaling/converting it to the current output format
      void sendScaledCvBGR1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a BGR cv::Mat after scaling/converting it to the current output format
      void sendScaledCvBGR(cv::Mat const & img) const;

      //! Shorthand to send a RGB cv::Mat after scaling/converting it to the current output format
      void sendScaledCvRGB1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a RGB cv::Mat after scaling/converting it to the current output format
      void sendScaledCvRGB(cv::Mat const & img) const;

      //! Shorthand to send a RGBA cv::Mat after scaling/converting it to the current output format
      void sendScaledCvRGBA1(cv::Mat const & img, int quality) const;

      //! Shorthand to send a RGBA cv::Mat after scaling/converting it to the current output format
      void sendScaledCvRGBA(cv::Mat const & img) const;
      
    private:
      OutputFrame * itsOutputFrame;
  };
  
  //! Wrapper module to allow users to develop new modules written in Python
  /*! This wrapper module calls a process function written in Python on every frame. Note how sendSerial() is added
      dynamically after the python class is defined, as a new member function of the class. \ingroup python */
  class PythonModule : public Module
  {
    public:
      //! Constructor needs the full path to a Python source code file
      /*! Note that, contrary to C++ modules, construction will not throw. This is so that the module is always valid
	  and initialized, and its module path can be set by Engine, which is necessary to allow saving the source code
	  from JeVois Inventor. Instead, any construction error is stored internally in this class and will be re-thrown
	  at any access to process(), parfseSerial(), etc. */
      PythonModule(VideoMapping const & m);

      //! Virtual destructor for safe inheritance
      virtual ~PythonModule();

      //! Processing function, version that receives a frame from camera and sends a frame out over USB
      virtual void process(InputFrame && inframe, OutputFrame && outframe) override;

      //! Processing function, version that receives a frame from camera and does not use USB
      virtual void process(InputFrame && inframe) override;

      //! Receive a string from a serial port which contains a user command
      virtual void parseSerial(std::string const & str, std::shared_ptr<UserInterface> s) override;

      //! Human-readable description of this Module's supported custom commands
      virtual void supportedCommands(std::ostream & os) override;

      //! Optionally call uninit() python module function, if implemented
      void postUninit() override;
      
    private:
      boost::python::object itsMainModule, itsMainNamespace;
      boost::python::object itsInstance;
      std::string itsConstructionError;
  };

} // namespace jevois

