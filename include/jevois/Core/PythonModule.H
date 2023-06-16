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
#include <jevois/Core/PythonWrapper.H>
#include <jevois/GPU/GUIhelper.H>

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
      
      //! Construct from a regular (move-only) InputFrame that should be be coming from Engine
      InputFramePython(InputFrame * src);
      
      //! Get the next captured camera image, thin wrapper for default arg value
      RawImage const & get1(bool casync) const;

      //! Get the next captured camera image, thin wrapper for default arg value
      RawImage const & get() const;

      //! Check whether a second input image scaled by the JeVoisPro Platform ISP is available
      bool hasScaledImage() const;

      //! Indicate that user processing is done with the image previously obtained via get()
      void done() const;

      //! Indicate that user processing is done with the ISP-scaled image previously obtained via get2()
      void done2() const;

      //! Get the next captured camera image, ISP-scaled second frame
      RawImage const & get21(bool casync) const;

      //! Get the next captured camera image, ISP-scaled second frame
      RawImage const & get2() const;

      //! Get the next captured camera image that is intended for processing
      RawImage const & getp1(bool casync) const;

      //! Get the next captured camera image that is intended for processing
      RawImage const & getp() const;

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

      //! Shorthand to get the input image for processing as a GRAY cv::Mat and release the raw buffer
      cv::Mat getCvGRAYp() const;

      //! Shorthand to get the input image for processing as a BGR cv::Mat and release the raw buffer
      cv::Mat getCvBGRp() const;

      //! Shorthand to get the input image for processing as a RGB cv::Mat and release the raw buffer
      cv::Mat getCvRGBp() const;

      //! Shorthand to get the input image for processing as a RGBA cv::Mat and release the raw buffer
      cv::Mat getCvRGBAp() const;
      
    private:
      friend class GUIhelperPython;
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
      
      //! Construct from a regular (move-only) OutputFrame that should be be coming from Engine
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

#ifdef JEVOIS_PRO
  //! Wrapper around GUIhelper to be used by Python
  /*! This class is not intended for general use, but only for use by PythonModule. Users of this class must ensure that
      the original GUIhelper will outlive any and all GUIhelperPython instances, since GUIhelperPython just references
      to the source GUIhelper by unprotected raw pointer. Although the C++ object is called GUIhelperPython, we will
      expose it to python under the name GUIhelper (see PythonSupport.C). \ingroup python */
  class GUIhelperPython
  {
    public:
      //! Construct from a regular GUIhelper that should be be coming from Engine
      GUIhelperPython(GUIhelper * src);

      //! Start a new rendering frame
      boost::python::tuple startFrame();

      //! Helper to indicate that startFrame() was called, and thus endFrame() should be called
      bool frameStarted() const;
      
      //! Draw a RawImage, copying pixel data to an OpenGL texture
      boost::python::tuple drawImage(char const * name, RawImage const & img,
                                     bool noalias = false, bool isoverlay = false);

      //! Draw an OpenCV image, copying pixel data to an OpenGL texture
      boost::python::tuple drawImage1(char const * name, cv::Mat const & img, bool rgb,
                                      bool noalias = false, bool isoverlay = false);

      //! Draw an OpenCV image, copying pixel data to an OpenGL texture
      boost::python::tuple drawImage2(char const * name, RawImage const & img, int x, int y, int w, int h,
                                      bool noalias = false, bool isoverlay = false);

      //! Draw an OpenCV image, copying pixel data to an OpenGL texture
      boost::python::tuple drawImage3(char const * name, cv::Mat const & img, bool rgb, int x, int y, int w, int h,
                                      bool noalias = false, bool isoverlay = false);

      //! Draw the input video frame from the camera using zero-copy
      boost::python::tuple drawInputFrame(char const * name, InputFramePython const & frame,
                                          bool noalias = false, bool casync = false);

      //! Draw the second (scaled) input video frame from the camera using zero-copy
      boost::python::tuple drawInputFrame2(char const * name, InputFramePython const & frame,
                                           bool noalias = false, bool casync = false);

      //! Convert coordinates of a point from within a rendered image to on-screen
      ImVec2 i2d(ImVec2 p, char const * name = nullptr);

      //! Convert coordinates of a point from within a rendered image to on-screen
      ImVec2 i2d1(float x, float y, char const * name = nullptr);

      //! Convert a 2D size from within a rendered image to on-screen
      ImVec2 i2ds(ImVec2 p, char const * name = nullptr);

      //! Convert a 2D size from within a rendered image to on-screen
      ImVec2 i2ds1(float x, float y, char const * name = nullptr);

      //! Draw line over an image
      void drawLine(float x1, float y1, float x2, float y2, ImU32 col = IM_COL32(128,255,128,255));
      
      //! Draw rectangular box over an image
      void drawRect(float x1, float y1, float x2, float y2, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);

      //! Draw polygon over an image
      void drawPoly(std::vector<cv::Point> const & pts, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw polygon over an image
      void drawPoly1(std::vector<cv::Point2f> const & pts, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw polygon over an image
      void drawPoly2(cv::Mat const & pts, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw circle over an image
      void drawCircle(float x, float y, float r, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw text over an image
      void drawText(float x, float y, char const * txt, ImU32 col = IM_COL32(128,255,128,255));
      
      //! Get coordinates of the start of a given line of text to be drawn as overlay on top of an image
      ImVec2 iline(int line = -1, char const * name = nullptr);

      //! Draw some overlay text on top of an image
      void itext(char const * txt, ImU32 const & col = IM_COL32_BLACK_TRANS, int line = -1);
      
      //! Display processing and video info at bottom of screen
      void iinfo(jevois::InputFramePython const & inframe, std::string const & fpscpu,
                 unsigned short winw = 0, unsigned short winh = 0);
      
      //! Release an image
      void releaseImage(char const * name);
      
      //! Release an image, second video stream
      void releaseImage2(char const * name);
      
      //! Finish current frame and render it
      void endFrame();

      //! Convert coordinates of a point from on-screen to within a rendered image
      ImVec2 d2i(ImVec2 p, char const * name = nullptr);

      //! Convert coordinates of a point from on-screen to within a rendered image
      ImVec2 d2i1(float x, float y, char const * name = nullptr);

      //! Convert a 2D size from on-screen to within a rendered image
      ImVec2 d2is(ImVec2 p, char const * name = nullptr);

      //! Convert a 2D size from on-screen to within a rendered image
      ImVec2 d2is1(float x, float y, char const * name = nullptr);

      //! Report an error in an overlay window
      void reportError(std::string const & err);

      //! Report current exception in a modal dialog, then ignore it
      void reportAndIgnoreException(std::string const & prefix = "");

      //! Report current exception in a modal dialog, then re-throw it
      void reportAndRethrowException(std::string const & prefix = "");

      //! ImGui helper: get mouse position
      ImVec2 getMousePos();
      
      //! ImGui helper: check if mouse button clicked
      bool isMouseClicked(int button_num);

      //! ImGui helper: check if mouse button double-clicked
      bool isMouseDoubleClicked(int button_num);

      //! ImGui helper: check if mouse button dragged
      bool isMouseDragging(int button_num);

      //! ImGui helper: check if mouse button pressed
      bool isMouseDown(int button_num);

      //! ImGui helper: check if mouse button released
      bool isMouseReleased(int button_num);
      
    private:
      GUIhelper * itsGUIhelper;
  };
#endif
  
  //! Wrapper module to allow users to develop new modules written in Python
  /*! This wrapper module calls a process function written in Python on every frame. Note how sendSerial() is added
      dynamically after the python class is defined, as a new member function of the class. \ingroup python */
  class PythonModule : public Module, public PythonWrapper
  {
    public:
      //! Constructor needs the full path to a Python source code file
      /*! Note that, contrary to C++ modules, construction will not throw. This is so that the module is always valid
	  and initialized, and its module path can be set by Engine, which is necessary to allow saving the source code
	  from JeVois Inventor. Instead, any construction error is stored internally in this class and will be re-thrown
	  at any access to process(), parfseSerial(), etc. */
      PythonModule(VideoMapping const & m);

      //! Load python code and optionally call init() python module function, if implemented
      void preInit() override;
 
      //! Virtual destructor for safe inheritance
      virtual ~PythonModule();

      //! Processing function, version that receives a frame from camera and sends a frame out over USB
      virtual void process(InputFrame && inframe, OutputFrame && outframe) override;

      //! Processing function, version that receives a frame from camera and does not use USB
      virtual void process(InputFrame && inframe) override;

#ifdef JEVOIS_PRO
      //! Processing function, version that receives a frame from camera, no USB, but GUI output on JeVois-Pro
      virtual void process(InputFrame && inframe, GUIhelper & helper);
#endif
      
      //! Receive a string from a serial port which contains a user command
      virtual void parseSerial(std::string const & str, std::shared_ptr<UserInterface> s) override;

      //! Human-readable description of this Module's supported custom commands
      virtual void supportedCommands(std::ostream & os) override;

      //! Optionally call uninit() python module function, if implemented
      void postUninit() override;

    private:
      std::string itsPyPath;
  };

  namespace dnn
  {
    class PreProcessor;
    
    //! Pre-Processor interface exposed to the python side
    /*! This wrapper is passed down to the process() function of a python post-processor, it provides a python-friendly
        interface to b2i(), blobsizes(), etc \ingroup python */
    class PreProcessorForPython
    {
      public:
        //! Construct from an existing PreProcessor
        /*! Caller must ensure that pp outlives us. */
        PreProcessorForPython(PreProcessor * pp);

        //! Access the last processed image size
        /*! Returned as a tuple (width, height). */
        boost::python::tuple imagesize() const;
        
        //! Access the last computed blobs (or empty if process() has not yet been called)
        boost::python::list blobs() const;

        //! Access the width and height of a given blob, accounting for NCHW or NHWC
        /*! Returned as a tuple (width, height). */
        boost::python::tuple blobsize(size_t num) const;
        
        //! Convert coordinates from blob back to original image
        /*! Given coords x,y should be in [0..w-1]x[0..h-1] where w,h are the blob's width and height. This is useful to
            convert detected boxes back into original input coordinates. Returned as a tuple (x, y). */
        boost::python::tuple b2i(float x, float y, size_t blobnum);

        //! Get unscaled crop rectangle in image coordinates
        /*! This is useful to display an image overlay on top of the input image.
            Returned as a tuple (x, y, w, h). */
        boost::python::tuple getUnscaledCropRect(size_t blobnum);

        //! Convert coordinates from image to blob
        /*! Given coords x,y should be in [0..w-1]x[0..h-1] where w,h are the image's width and height. This is useful
            to convert mouse coordinates (after they have been converted from screen to image coords) to locations
            within an input blob. */
        boost::python::tuple i2b(float  x, float  y, size_t blobnum);

      private:
        PreProcessor * itsPP;
    };
    
    class PostProcessorDetectYOLO;
    
    //! YOLO post-processor exposed to python
    /*! A bit hacky since PostProcessorDetectYOLO is a Component that holds parameters. This class will add a
        PostProcessorDetectYOLO subcomponent to the current module, and then will forward yolo post-processing requests
        to it. */
    class PostProcessorDetectYOLOforPython
    {
      public:
        //! Constructor constructs itsYOLO and adds it to current module
        PostProcessorDetectYOLOforPython();

        //! Destructor removes itsYOLO from current module
        ~PostProcessorDetectYOLOforPython();

        //! Freeze/unfreeze parameters that users should not change while running
        void freeze(bool doit);

        //! Generic raw YOLO processing
        /*! The returned tuple has 3 elements: list of int for classIds, list of float for confidences, and list of
            4-element tuples of floats for boxes (x, y, w, h). */
        boost::python::tuple yolo(boost::python::list outs, int nclass, float boxThreshold,
                                  float confThreshold, int bw, int bh, int fudge, int maxbox);

      private:
        std::shared_ptr<PostProcessorDetectYOLO> itsYOLO;
    };
    
  } // namespace dnn
} // namespace jevois
