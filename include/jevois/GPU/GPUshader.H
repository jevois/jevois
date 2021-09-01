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

#include <jevois/GPU/OpenGL.H>

namespace jevois
{
  //! Simple class to load and compile some OpenGL-ES shader code
  class GPUshader
  {
    public:
      //! Constructor
      GPUshader();
      
      //! Destructor
      ~GPUshader();
      
      //! Load a shader from file
      /*! type should be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER */
      void load(char const * filename, GLuint type);
      
      //! Load a shader from inlined code in a C-string
      /*! type should be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER. Name is only used for info and error messages,
          so typically can just be "vertex" or "fragment" or something describing which instantiation of those. */
      void set(char const * name, char const * str, GLuint type);
      
      //! Get the shader's ID
      GLuint id() const;
      
    private:
      GLuint itsId;
  };
} // namespace jevois
