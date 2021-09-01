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

#ifdef JEVOIS_PLATFORM_PRO

#include <jevois/GPU/VideoDisplayBackendMALI.H>
#include <chrono>
#include <string>
#include <array>

#include <wchar.h>
#include <wctype.h>

//using namespace std::literals;
using namespace std::string_literals;
//using namespace std::literals::string_literals;

//! Helper struct to key track of key states
struct input_event_state
{
    int altgr:1;
    int alt:1;
    int shift:1;
    int ctrl:1;
    int meta:1;
};

namespace jevois
{
  //! BackendMALI for ImGui on JeVois-Pro
  /*! This abstract base class provides an interface used by VideoDisplayGUI. Derived implementations are available for
      SDL2 and Mali framebuffer + evdev.

      JeVois Pro hardware reports the following:

      EGL v1.4
      EGL API is EGL_OPENGL_ES_API
      OpenGL ES 3.2 git.c8adbf9.122c9daed32dbba4b3056f41a2f23c58 ARM (Mali-G52) GL_VER=3.2
      
      OpenGL extensions: GL_ARM_rgba8 GL_ARM_mali_shader_binary GL_OES_depth24 GL_OES_depth_texture
      GL_OES_depth_texture_cube_map GL_OES_packed_depth_stencil GL_OES_rgb8_rgba8 GL_EXT_read_format_bgra
      GL_OES_compressed_paletted_texture GL_OES_compressed_ETC1_RGB8_texture GL_OES_standard_derivatives
      GL_OES_EGL_image GL_OES_EGL_image_external GL_OES_EGL_image_external_essl3 GL_OES_EGL_sync GL_OES_texture_npot
      GL_OES_vertex_half_float GL_OES_required_internalformat GL_OES_vertex_array_object GL_OES_mapbuffer
      GL_EXT_texture_format_BGRA8888 GL_EXT_texture_rg GL_EXT_texture_type_2_10_10_10_REV GL_OES_fbo_render_mipmap
      GL_OES_element_index_uint GL_EXT_shadow_samplers GL_OES_texture_compression_astc
      GL_KHR_texture_compression_astc_ldr GL_KHR_texture_compression_astc_hdr GL_KHR_texture_compression_astc_sliced_3d
      GL_EXT_texture_compression_astc_decode_mode GL_EXT_texture_compression_astc_decode_mode_rgb9e5 GL_KHR_debug
      GL_EXT_occlusion_query_boolean GL_EXT_disjoint_timer_query GL_EXT_blend_minmax GL_EXT_discard_framebuffer
      GL_OES_get_program_binary GL_OES_texture_3D GL_EXT_texture_storage GL_EXT_multisampled_render_to_texture
      GL_OES_surfaceless_context GL_OES_texture_stencil8 GL_EXT_shader_pixel_local_storage
      GL_ARM_shader_framebuffer_fetch GL_ARM_shader_framebuffer_fetch_depth_stencil GL_ARM_mali_program_binary
      GL_EXT_sRGB GL_EXT_sRGB_write_control GL_EXT_texture_sRGB_decode GL_EXT_texture_sRGB_R8 GL_EXT_texture_sRGB_RG8
      GL_KHR_blend_equation_advanced GL_KHR_blend_equation_advanced_coherent GL_OES_texture_storage_multisample_2d_array
      GL_OES_shader_image_atomic GL_EXT_robustness GL_EXT_draw_buffers_indexed GL_OES_draw_buffers_indexed
      GL_EXT_texture_border_clamp GL_OES_texture_border_clamp GL_EXT_texture_cube_map_array
      GL_OES_texture_cube_map_array GL_OES_sample_variables GL_OES_sample_shading
      GL_OES_shader_multisample_interpolation GL_EXT_shader_io_blocks GL_OES_shader_io_blocks GL_EXT_tessellation_shader
      GL_OES_tessellation_shader GL_EXT_primitive_bounding_box GL_OES_primitive_bounding_box GL_EXT_geometry_shader
      GL_OES_geometry_shader GL_ANDROID_extension_pack_es31a GL_EXT_gpu_shader5 GL_OES_gpu_shader5 GL_EXT_texture_buffer
      GL_OES_texture_buffer GL_EXT_copy_image GL_OES_copy_image GL_EXT_shader_non_constant_global_initializers
      GL_EXT_color_buffer_half_float GL_EXT_color_buffer_float GL_EXT_YUV_target GL_OVR_multiview GL_OVR_multiview2
      GL_OVR_multiview_multisampled_render_to_texture GL_KHR_robustness GL_KHR_robust_buffer_access_behavior
      GL_EXT_draw_elements_base_vertex GL_OES_draw_elements_base_vertex GL_EXT_buffer_storage
      GL_EXT_texture_filter_anisotropic

      To get correct UTF-8 keyboard translation, you need env var LC_CTYPE to be set, e.g., en_US.UTF-8

      \ingroup core */
  class ImGuiBackendMALI : public VideoDisplayBackendMALI
  {
    public:
      //! Constructor 
      ImGuiBackendMALI();

      //! Virtual destructor for safe inheritance, free resources
      virtual ~ImGuiBackendMALI();

      //! Initialize the underlying engine that will process events, create windows, etc
      /*! The init starts with the given initial window/framebuffer size. If conslock is true, we lock the console and
          the framebuffer; set to false when running in gdb for debugging, using gdb --args jevoispro-daemon
          --conslock=false */
      virtual void init(unsigned short w, unsigned short h, bool fullscreen = false, float scale = 1.0f,
                        bool conslock = true);

      //! Poll events such as mouse movements, buttons, keyboard, joystick, and pass to ImGui
      /*! Returns true if some events were received, which can be used to turn on mouse pointer and GUI. */
      virtual bool pollEvents(bool & shouldclose) override;

      //! Start a new frame and clear the window/framebuffer
      virtual void newFrame() override;

      //! Render the ImGui graphics
      virtual void render() override;

    protected:
      std::chrono::time_point<std::chrono::steady_clock> itsLastNewFrameTime;
      int itsFd[32];
      void scanDevices();
      void addDevice(size_t num, int fd);
      void removeDevice(size_t num);
      int itsMouseX, itsMouseY;
      bool itsMouseButton[5];
      int itsConsoleFd;
      int itsTTY;
      int itsKBmode;
      int itsWidth = 0, itsHeight = 0;

      std::array<wchar_t, 49> itsCharKeys { L"1234567890-=qwertyuiop[]asdfghjkl;'`\\zxcvbnm,./<" };
      std::array<wchar_t, 49> itsShiftKeys { L"!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~|ZXCVBNM<>?>" };
      std::array<wchar_t, 49> itsAltgrKeys = { 0 };
      static constexpr wchar_t itsFuncKeys[58][8] =
      {
       L"<Esc>", L"<BckSp>", L"<Tab>", L"<Enter>", L"<LCtrl>", L"<LShft>",
       L"<RShft>", L"<KP*>", L"<LAlt>", L" ", L"<CpsLk>", L"<F1>", L"<F2>",
       L"<F3>", L"<F4>", L"<F5>", L"<F6>", L"<F7>", L"<F8>", L"<F9>", L"<F10>",
       L"<NumLk>", L"<ScrLk>", L"<KP7>", L"<KP8>", L"<KP9>", L"<KP->", L"<KP4>",
       L"<KP5>", L"<KP6>", L"<KP+>", L"<KP1>", L"<KP2>", L"<KP3>", L"<KP0>",
       L"<KP.>", /*"<",*/ L"<F11>", L"<F12>", L"<KPEnt>", L"<RCtrl>", L"<KP/>",
       L"<PrtSc>", L"<AltGr>", L"<Break>" /*linefeed?*/, L"<Home>", L"<Up>",
       L"<PgUp>", L"<Left>", L"<Right>", L"<End>", L"<Down>", L"<PgDn>",
       L"<Ins>", L"<Del>", L"<Pause>", L"<LMeta>", L"<RMeta>", L"<Menu>"
      };
      
      input_event_state itsInputState = { };
      char itsInputBuffer[256] = { 0 };
  };
  
} // namespace jevois

#endif // JEVOIS_PLATFORM_PRO
