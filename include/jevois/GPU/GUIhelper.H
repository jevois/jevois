// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

// To minimize the amount of ifdef junk in user code, define type alias OptGUIhelper here as either GUIhelper or void
namespace jevois
{
#ifdef JEVOIS_PRO
  class GUIhelper;
  using OptGUIhelper = GUIhelper;
#else
  using OptGUIhelper = void;
#endif
}

// This is only available on JeVoisPro
#ifdef JEVOIS_PRO

#include <jevois/Component/Component.H>
#include <jevois/Core/Engine.H>
#include <jevois/Core/InputFrame.H>
#include <jevois/GPU/ImGuiBackendSDL.H>
#include <jevois/GPU/ImGuiBackendMALI.H>
#include <jevois/GPU/ImGuiImage.H>
#include <jevois/Types/Enum.H>
#include <chrono>
#include <imgui.h>
#include <glm/glm.hpp>
#include <opencv2/core/core.hpp>

namespace jevois
{
  class GPUimage;
  class GUIeditor;
  
  namespace gui
  {
    static jevois::ParameterCategory const ParamCateg("Graphical Interface Options");

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(fullscreen, bool, "Use a fullscreen display when true, only has effect on host. "
                             "Platform always is fullscreen as the MALI OpenGL driver do not support windowing.",
                             false, ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(winsize, cv::Size, "Initial window size to use on host. On platform, size is determined "
                             "by hardware and the value of this parameter will be overwritten on init. The "
                             "parameter can still be used to query display size.",
#ifdef JEVOIS_PLATFORM
                             // Use 0 x 0 default on platform which will trigger a query for framebuffer size in
                             // VideoDisplayBackEndMali:
                             cv::Size(0, 0),
#else
                             // On host, provide a default size since by default we run in a window:
                             cv::Size(1920, 1080),
#endif
                             ParamCateg);
    
    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(hidesecs, float, "Number of seconds of inactivity from keyboard/mouse/joystick/etc after "
                             "which we hide the GUI, or 0.0 to never hide it. Note: The GUI starts hidden until the "
                             "first input event from a keyboard, mouse, etc.",
                             30.0F, ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(scale, float, "Scale factor applied to the GUI",
                                           2.0f, { 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.0f,
                                                   2.1f, 2.2f, 2.3f, 2.4f, 2.5f, 2.6f, 2.7f, 2.8f, 2.9f, 3.0f },
                                           ParamCateg);

    //! Enum for Parameter \relates jevois::GUIhelper
    JEVOIS_DEFINE_ENUM_CLASS(GuiStyle, (Light) (Dark) (Classic) );

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(style, GuiStyle, "Color style for the JeVois GUI",
                                           GuiStyle::Light, GuiStyle_Values, ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(rounding, int, "Window and widget corner rounding for the JeVois GUI. "
                                           "Note than changing GUI scale will update this parameter as well.",
                                           4, jevois::Range<int>(0, 24), ParamCateg);
    
    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(overlaycolor, ImColor, "Default color to use for overlay text",
                             ImColor(255, 255, 255, 255), ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(linethick, float, "Line thickness for overlay drawings",
                             5.0F, jevois::Range<float>(0.1F, 20.0F), ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(fillalpha, float, "Alpha multiplier for overlay fills",
                             0.25F, jevois::Range<float>(0.0F, 1.0F), ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER(allowquit, bool, "Quit application on ESC key (platform) or window close (host)",
                             false, ParamCateg);

    //! Parameter \relates jevois::GUIhelper
    JEVOIS_DECLARE_PARAMETER_WITH_CALLBACK(twirl, float, "Apply twirl effect to displayed video and images, useful "
                                           "mainly for demo mode",
                                           0.0F, jevois::Range<float>(-15.0F, 15.0F), ParamCateg);
  } // namespace gui

  /*! \defgroup gui JeVois-Pro Graphical Interface

      Classes and utilities to provide a graphical interface using Dear ImGui. Supported on JeVois-Pro only.
        
      \ingroup core */
 
  //! Helper class to assist modules in creating graphical and GUI elements
  /*! This class only works on JeVois-Pro. \ingroup gui */
  class GUIhelper : public Component,
                    public Parameter<gui::fullscreen, gui::winsize, gui::hidesecs, gui::scale,
                                     gui::style, gui::rounding, gui::overlaycolor, gui::linethick,
                                     gui::fillalpha, gui::allowquit, gui::twirl>
  {
    public:
      //! Constructor
      GUIhelper(std::string const & instance, bool conslock = false);

      //! Destructor
      virtual ~GUIhelper();

      //! Start a new rendering frame
      /*! This should be called on every new video frame to be rendered. Will initialize OpenGL and open the window or
          screen if needed. Sets the current window size into w and h, returns true if idle (no keyboard/mouse/etc
          events received in more than hidesecs seconds). */
      bool startFrame(unsigned short & w, unsigned short & h);

      //! Check for idle in case startFrame() was called elsewhere
      bool idle() const;
      
      //! Helper to indicate that startFrame() was called, and thus endFrame() should be called
      /*! Mostly useful during exception handling inside Engine. */
      bool frameStarted() const;
      
      //! Draw a RawImage, copying pixel data to an OpenGL texture
      /*! Name should be a unique name, and should typically remain constant across successive video frames. This name
          is used as an index to OpenGL textures, shaders, and programs created to render the images. Note that OpenGL
          will slow down and eventually run out of resources if you create too many textures, thus Module writers should
          try to minimize the number of different names that are used. Note that we will add a suffix to the name,
          corresponding to the bufindex of the RawImage. If you pass w=0 or h=0 then the image will be rescaled to fill
          the display as much as possible without changing the aspect ratio, and the actually used x,y,w,h will be
          returned. Otherwise, x,y,w,h are not modified. If noalias is specified, the scaling factor will be rounded
          down to the nearest integer to prevent aliasing in the display. This may reduce the displayed image size. For
          example, with a 1920x1080 window, a 640x480 image would be letterboxed to 1440x1080 when noalias is false. But
          that is a scaling factor of 2.25 which may create rendering aliasing. When noalias is true, the letterboxed
          image size will be 1280x960 (scale factor of 2.0). If isoverlay is true, then the image is assumed to be a
          semi-transparent overlay to be drawn on top of a previously drawn image, and we preserve the drawing
          parameters of that previous image. */
      void drawImage(char const * name, RawImage const & img, int & x, int & y, unsigned short & w, unsigned short & h,
                     bool noalias = false, bool isoverlay = false);

      //! Draw an OpenCV image, copying pixel data to an OpenGL texture
      /*!  Name should be a unique name, and should typically remain constant across successive video frames. This name
          is used as an index to OpenGL textures, shaders, and programs created to render the images. Note that OpenGL
          will slow down and eventually run out of resources if you create too many textures, thus Module writers should
          try to minimize the number of different names that are used. If image has three or four 8-bit channels,
          interpret as RGB[A] if rgb is true, otherwise BGR[A]. If two 8-bit channels, interpret as YUYV. If one,
          interpret as GRAY. If you pass w=0 or h=0 then the image will be rescaled to fill the display as much as
          possible without changing the aspect ratio, and the actually used x,y,w,h will be returned. Further, if
          noalias is true, that rescaling will ensure an integer scaling factor. Otherwise, x,y,w,h are not modified. If
          isoverlay is true, then the image is assumed to be a semi-transparent overlay to be drawn on top of a
          previously drawn image, and we preserve the drawing parameters of that previous image. */
      void drawImage(char const * name, cv::Mat const & img, bool rgb, int & x, int & y, unsigned short & w,
                     unsigned short & h, bool noalias = false, bool isoverlay = false);

      //! Draw the input video frame from the camera using zero-copy
      /*! If you pass w=0 or h=0 then the image will be rescaled to fill the display as much as possible without
          changing the aspect ratio, and the actually used x,y,w,h will be returned. Further, if noalias is true, that
          rescaling will ensure an integer scaling factor. Otherwise, x,y,w,h are not modified. */
      void drawInputFrame(char const * name, InputFrame const & frame, int & x, int & y,
                          unsigned short & w, unsigned short & h, bool noalias = false, bool casync = false);

      //! Draw the second (scaled) input video frame from the camera using zero-copy
      /*! If you pass w=0 or h=0 then the image will be rescaled to fill the display as much as possible without
          changing the aspect ratio, and the actually used x,y,w,h will be returned. Further, if noalias is true, that
          rescaling will ensure an integer scaling factor. Otherwise, x,y,w,h are not modified. Throws unless we are
          JeVois-Pro Platform and the camera is set to CropScale mode. */
      void drawInputFrame2(char const * name, InputFrame const & frame, int & x, int & y,
                           unsigned short & w, unsigned short & h, bool noalias = false, bool casync = false);

      //! Convert coordinates of a point from within a rendered image to on-screen
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 i2d(ImVec2 p, char const * name = nullptr);

      //! Convert coordinates of a point from within a rendered image to on-screen
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 i2d(float x, float y, char const * name = nullptr);

      //! Convert a 2D size from within a rendered image to on-screen
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 i2ds(ImVec2 p, char const * name = nullptr);

      //! Convert a 2D size from within a rendered image to on-screen
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 i2ds(float x, float y, char const * name = nullptr);

      //! Draw line over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawLine(float x1, float y1, float x2, float y2, ImU32 col = IM_COL32(128,255,128,255));
      
      //! Draw rectangular box over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawRect(float x1, float y1, float x2, float y2, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);

      //! Draw polygon over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawPoly(std::vector<cv::Point> const & pts, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw polygon over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawPoly(std::vector<cv::Point2f> const & pts, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw circle over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawCircle(float x, float y, float r, ImU32 col = IM_COL32(128,255,128,255), bool filled = true);
      
      //! Draw text over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawText(float x, float y, char const * txt, ImU32 col = IM_COL32(128,255,128,255));
      
      //! Draw text over an image
      /*! Coordinates used should be image coordinates, as this function internally calls i2d(). */
      void drawText(float x, float y, std::string const & txt, ImU32 col = IM_COL32(128,255,128,255));
      
      //! Get coordinates of the start of a given line of text to be drawn as overlay on top of an image
      /*! Use this to draw overlay text on top of a previously drawn image, it will scale by font size for you. If line
          is -1, we will increment over the last time this function was called (use with caution). Line number is reset
          to 0 when drawImage() or drawInputFrame(), etc are called. */
      ImVec2 iline(int line = -1, char const * name = nullptr);

      //! Draw some overlay text on top of an image
      /* If line is -1, we will increment over the last time this function was called (use with caution). Line number is
         reset to 0 when drawImage() or drawInputFrame(), etc are called. If col is not given, use overlaycolor. */
      void itext(char const * txt, ImU32 const & col = IM_COL32_BLACK_TRANS, int line = -1);
      
      //! Draw some overlay text on top of an image
      /* If line is -1, we will increment over the last time this function was called (use with caution). Line number is
         reset to 0 when drawImage() or drawInputFrame(), etc are called. If col is not given, use overlaycolor. */
      void itext(std::string const & txt, ImU32 const & col = IM_COL32_BLACK_TRANS, int line = -1);

      //! Display processing and video info at bottom of screen
      /*! This helper is to be used by modules to provide a consistent info display at the bottom of the screen.
          fpscpu should be the string returned by Timer::stop(). If winw and winh are not given, will query from display
          backend, which will cost a few CPU cycles; so pass it on if you already have it, e.g, from running
          startFrame() previously. */
      void iinfo(jevois::InputFrame const & inframe, std::string const & fpscpu,
                 unsigned short winw = 0, unsigned short winh = 0);
      
      //! Release an image
      /*! Call this if you plan on re-using the same image name later for an image of different size or
          format. Otherwise, once an image has been used once, its OpenGL texture (including its dims) will remain the
          same and only the pixel values will be updated when the image is drawn again. Silently ignores if the image is
          not found, i.e., has not been drawn before. */
      void releaseImage(char const * name);
      
      //! Release an image, second video stream
      /*! Call this if you plan on re-using the same image name later for an image of different size or
          format. Otherwise, once an image has been used once, its OpenGL texture (including its dims) will remain the
          same and only the pixel values will be updated when the image is drawn again. Silently ignores if the image is
          not found, i.e., has not been drawn before. */
      void releaseImage2(char const * name);
      
      //! Finish current frame and render it
      /*! This should be called once for every call to startFrame(). */
      void endFrame();

      //! Reset to default state, typically called on Module or video format change
      /*! Free images, reset matrices, etc. */
      void reset(bool modulechanged = true);

      //! Report an error in an overlay window
      void reportError(std::string const & err);

      //! Report current exception in a modal dialog, then ignore it
      void reportAndIgnoreException(std::string const & prefix = "");

      //! Report current exception in a modal dialog, then re-throw it
      void reportAndRethrowException(std::string const & prefix = "");

      //! Clear all errors currently displayed in the JeVois-Pro GUI
      /*! In the JevoisPro GUI, errors reported via reportError() remain displayed for a few seconds, but sometimes we
          want to clear them right away, e.g., after DNN pipeline threw, if the user selects another one, we want the
          previous error to disappear immediately since it is not applicable anymore. */
      void clearErrors();
      
      //! Z distance from camera to image plane to achieve pixel-perfect rendering
      float const pixel_perfect_z = 0.0F;
      
      //! Our projection matrix
      glm::mat4 proj;
      
      //! Our view matrix
      /*! On the first call to startFrame(), the whole OpenGL engine is initialized and the view matrix is set so that
          the pixel perfect image plane is vertical and centered on the origin, while the camera is translated in
          negative Z to achieve pixel perfect rendering. */
      glm::mat4 view;

      //! Display a (?) label and show tooltip when it is hovered
      /*! If 2 or more messages are provided, they will be separated by a separator. */
      void helpMarker(char const * msg, char const * msg2 = nullptr, char const * msg3 = nullptr);

      //! Helper to draw a toggle button
      bool toggleButton(char const * name, bool * val);

      //! Helper to draw a modal with 2 choices
      /*! Returns 1 if the first button was clicked, 2 if the second was, or some other value if no button was clicked
          (caller should just wait and try again at the next frame if not 1 or 2, passing again that returned value). By
          default, the first button is in focus. If default_val is not nullptr, add a "don't ask again" checkbox. If
          *default_val is 1 or 2, just return that without even showing the modal. */
      int modal(std::string const & title, char const * text, int * default_val = nullptr,
                char const * b1txt = "Ok", char const * b2txt = "Cancel");

      //! Show a message that we are running headless
      void headlessDisplay();

      //! Tell whether user enabled serlog messages to GUI console
      bool serlogEnabled() const;

      //! Tell whether user enabled serout messages to GUI console
      bool seroutEnabled() const;
      
      //! Convert coordinates of a point from on-screen to within a rendered image
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 d2i(ImVec2 p, char const * name = nullptr);

      //! Convert coordinates of a point from on-screen to within a rendered image
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 d2i(float x, float y, char const * name = nullptr);

      //! Convert a 2D size from on-screen to within a rendered image
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 d2is(ImVec2 p, char const * name = nullptr);

      //! Convert a 2D size from on-screen to within a rendered image
      /*! If name is nullptr, the last image used by drawImage() or drawInputFrame() is used. In such case, if
          drawInputFrame() was called on a frame that also had a second, scaled image, assume that we have displayed the
          full-resolution input frame but sent the scaled image to processing, and that hence we also need to scale from
          second to first frame. */
      ImVec2 d2is(float x, float y, char const * name = nullptr);

      //! Compile a newly created module
      /*! This is mainly for internal use. Called by the code editor when saving C++ code to let us know to start the
          compilation process again. itsNewMapping should contain the new module's data. */
      void startCompilation();

      //! Like ImGui::Textunformatted but in a highlight color (typically, red)
      void highlightText(std::string const & str);

      //! Display some text in a big banner, used by demo mode
      /*! Any non-empty banner remains on screen until demoBanner() called again with empty title (default args). */
      void demoBanner(std::string const & title = "", std::string const & msg = "");
      
    protected:
      std::map<std::string /* name */, GPUimage> itsImages, itsImages2;
      GPUimage * itsLastDrawnImage = nullptr;
      int itsLastDrawnTextLine = -1;
      bool itsUsingScaledImage = false;
      float itsScaledImageFacX = 1.0F, itsScaledImageFacY = 1.0F;
      
      std::chrono::time_point<std::chrono::steady_clock> itsLastEventTime;
      bool itsIdle = true; //!< Idle state, updated by startFrame(), used by endFrame() to decide whether to draw GUI
      bool itsIdleBlocked = false; //!< While creating/compiling new modules, prevent idle
      bool itsEndFrameCalled = true; // try to avoid violent assert() crash from ImGui if user forgets to endFrame()

      mutable std::mutex itsErrorMtx;
      struct ErrorData
      {
          std::string err;
          std::chrono::time_point<std::chrono::steady_clock> firsttime;
          std::chrono::time_point<std::chrono::steady_clock> lasttime;
      };
      std::list<ErrorData> itsErrors;
      
      std::set<std::string> itsOpenModals;

      void drawPolyInternal(ImVec2 const * pts, size_t npts, ImU32 col, bool filled);
      ImU32 applyFillAlpha(ImU32 col) const;
      
      bool itsConsLock;
#ifdef JEVOIS_PLATFORM_PRO
      ImGuiBackendMALI itsBackend;
#else
      ImGuiBackendSDL itsBackend;
#endif
      std::string itsWindowTitle;
      ImGuiImage itsIcon;
      ImGuiImage itsHeadless;
      std::string itsModName;
      std::string itsModDesc;
      std::string itsModAuth;
      std::string itsModLang;
      std::vector<std::string> itsModDoc;
      gui::GuiStyle itsCurrentStyle = gui::GuiStyle::Dark;
      bool itsShowStyleEditor = false;
      bool itsShowAppMetrics = false;
      bool itsShowImGuiDemo = false;
      bool itsSerLogEnabled = true;
      bool itsSerOutEnabled = false;
      
      void drawJeVoisGUI();
      void drawInfo();
      void drawParameters();
      void drawConsole();
      void drawCamCtrls();
      void drawTweaks();
      void drawSystem();
      void drawMenuBar();
      void drawModuleSelect();
      void drawErrorPopup();
      void drawNewModuleForm();
      void compileModule();
      
      void newModEntry(char const * wname, std::string & str, char const * desc, char const * hint, char const * hlp);

      // Set a parameter by string, catch any exception and report it in popup modal
      void setparstr(std::string const & descriptor, std::string const & val);

      // Set a parameter by value, catch any exception and report it in popup modal
      template <class T>
      void setparval(std::string const & descriptor, T const & val);

      void onParamChange(gui::scale const & param, float const & newval) override;
      void onParamChange(gui::rounding const & param, int const & newval) override;
      void onParamChange(gui::style const & param, gui::GuiStyle const & newval) override;
      void onParamChange(gui::twirl const & param, float const & newval) override;

      // Config files:
      std::shared_ptr<GUIeditor> itsCfgEditor;
      
      // Code editing:
      std::shared_ptr<GUIeditor> itsCodeEditor;

      // dnnget helpers
      std::future<std::string> itsDnnGetFut;

      // Flag to refresh video mappings when we save the file or add a new one by creating a new module:
      bool itsRefreshVideoMappings = false;
      int itsVideoMappingListType = 0;

      // Flag to indicate whether we have a USB serial gadget
      bool itsUSBserial = false;

      // Compilation progress
      enum CompilationState { Idle, Start, Cmake, Make, Install, CPack, Success, Error };
      CompilationState itsCompileState = CompilationState::Idle;
      VideoMapping itsNewMapping = { };
      std::future<std::string> itsCompileFut;
      bool compileCommand(std::string const & cmd, std::string & msg); // true on success, false in progress, throws
      std::array<std::string, 4> itsCompileMessages; // messages for the compilation steps
      void runNewModule(); // install and load up the module defined in itsNewMapping

      // Banner stuff:
      std::string itsBannerTitle, itsBannerMsg;
      float itsGlobalAlpha = 1.0F;
  };

} // namespace jevois

// Include inlined implementation details that are of no interest to the end user
#include <jevois/GPU/details/GUIhelperImpl.H>

#endif // JEVOIS_PRO
