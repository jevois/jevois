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

#ifdef JEVOIS_PRO

#include <jevois/GPU/OpenGL.H>
#include <opencv2/core.hpp>
#include <imgui.h>

namespace jevois
{
  //! Wrapper for an image that can be rendered into ImGui
  /*! The wrapper combines an OpenCV image and an OpenGL texture. */
  class ImGuiImage
  {
    public:
      //! Constructor, image is uninitialized and loaded() returns false
      ImGuiImage();

      //! Load from file
      void load(std::string const & fname);
      
      //! Load from an OpenCV image
      /*! Allocates a texture on the GPU for the image. */
      void load(cv::Mat const & img, bool isbgr = true);

      //! Returns true if an image has been loaded through load()
      bool loaded() const;
      
      //! Free the texture and revert to un-initialized state
      void clear();
      
      //! Destructor
      /*! Deletes the associated GPU texture and OpenCV pixel data. */
      ~ImGuiImage();

      //! Draw into into current ImGui window or a drawlist
      void draw(ImVec2 const & pos, ImVec2 const & size, ImDrawList * dl = nullptr);

    private:
      GLuint itsId = 0;
  };
} // namespace jevois

#endif // JEVOIS_PRO
