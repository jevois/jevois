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

#include <jevois/GPU/GPUshader.H>

// ####################################################################################################
jevois::GPUshader::GPUshader() :
    itsId(0)
{ }

// ####################################################################################################
jevois::GPUshader::~GPUshader()
{
  if (itsId) glDeleteShader(itsId);
}

// ####################################################################################################
GLuint jevois::GPUshader::id() const
{ return itsId; }

// ####################################################################################################
void jevois::GPUshader::load(char const * filename, GLuint type)
{
  if (itsId) { glDeleteShader(itsId); itsId = 0; }

  // Read the whole file into memory (much faster than using streambuf):
  FILE * f = fopen(filename, "rb"); if (f == nullptr) PLFATAL("Failed to read file " << filename);
  fseek(f, 0, SEEK_END);
  int sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  char * src = new GLchar[sz+1];
  if (fread(src, 1, sz, f) != 1) { fclose(f); LFATAL("Failed to read " << filename); }
  src[sz] = 0; // null terminate it
  fclose(f);

  // Set and compile the shader:
  try { this->set(filename, src, type); } catch (...) { jevois::warnAndIgnoreException(); }

  delete [] src;
}

// ####################################################################################################
void jevois::GPUshader::set(char const * name, char const * str, GLuint type)
{
  if (itsId) { glDeleteShader(itsId); itsId = 0; }

  // Create and compile the shader:
  GL_CHECK(itsId = glCreateShader(type));
  GL_CHECK(glShaderSource(itsId, 1, (const GLchar**)&str, 0));
  GL_CHECK(glCompileShader(itsId));

  // Compilation check:
  GLint compiled; glGetShaderiv(itsId, GL_COMPILE_STATUS, &compiled);
  if (compiled == 0)
  {
    GLint loglen = 4096; char log[loglen];
    glGetShaderInfoLog(itsId, loglen, &loglen, &log[0]); log[loglen] = '\0';
    glDeleteShader(itsId);
    LFATAL("Failed to compile shader [" << name << "], Log: " << &log[0]);
  }
  
  LDEBUG("Compiled shader [" << name << ']');
}
