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

#ifdef JEVOIS_PRO

#include <jevois/GPU/GPUimage.H>

#include <jevois/Util/Utils.H>
#include <jevois/GPU/GPUtexture.H>
#include <jevois/GPU/GPUtextureDmaBuf.H>
#include <jevois/GPU/GPUprogram.H>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// ##############################################################################################################
// Shaders used here:
namespace jevois
{
  namespace shader
  {
    extern char const * vert;      // Vertex shader
    extern char const * frag_rgba; // To display an RGBA image
    extern char const * frag_rgb;  // To display an RGB image
    extern char const * frag_grey; // To display a greyscale image
    extern char const * frag_yuyv; // To display a YUYV image
    extern char const * frag_oes;  // To display a DMABUF camera image
    extern char const * frag_rgba_twirl; // To display an RGBA image with a twirl effect
    extern char const * frag_rgb_twirl;  // To display an RGB image with a twirl effect
    extern char const * frag_grey_twirl; // To display a greyscale image with a twirl effect
    extern char const * frag_yuyv_twirl; // To display a YUYV image with a twirl effect
    extern char const * frag_oes_twirl;  // To display a DMABUF camera image with a twirl effect
  }
}

// ##############################################################################################################
jevois::GPUimage::GPUimage(bool enable_twirl) : itsTwirl(enable_twirl)
{ }

// ##############################################################################################################
jevois::GPUimage::~GPUimage()
{
  if (itsVertexArray) glDeleteVertexArrays(1, &itsVertexArray);
  if (itsVertexBuffers[0]) glDeleteBuffers(2, itsVertexBuffers);
}

// ##############################################################################################################
void jevois::GPUimage::setInternal(unsigned int width, unsigned int height, unsigned int fmt,
                                   unsigned char const * data)
{
  if (width == 0 || height == 0) LFATAL("Cannot handle zero image width or height");

  // If image format has changed, load the appropriate program:
  if (fmt != itsFormat)
  {
    char const * frag_shader;
    switch (fmt)
    {
    case V4L2_PIX_FMT_YUYV: // YUYV shader gets YUYV (2 pixels) from one RGBA texture value (1 texel)
      if (itsTwirl) frag_shader = jevois::shader::frag_yuyv_twirl;
      else frag_shader = jevois::shader::frag_yuyv;
      itsGLtextureWidth = width / 2; itsGLtextureFmt = GL_RGBA;
      break;

    case V4L2_PIX_FMT_RGB32: // RGBA shader is simple pass-through
      if (itsTwirl) frag_shader = jevois::shader::frag_rgba_twirl;
      else frag_shader = jevois::shader::frag_rgba;
      itsGLtextureWidth = width; itsGLtextureFmt = GL_RGBA;
      break;

    case V4L2_PIX_FMT_GREY: // GRAY shader just converts from greyscale to RGBA
      if (itsTwirl) frag_shader = jevois::shader::frag_grey_twirl;
      else frag_shader = jevois::shader::frag_grey;
      itsGLtextureWidth = width; itsGLtextureFmt = GL_LUMINANCE;
      break;

    case V4L2_PIX_FMT_RGB24: // RGB shader gets R,G,B from 3 successive texels in a 3x wide luminance texture
      if (itsTwirl) frag_shader = jevois::shader::frag_rgb_twirl;
      else frag_shader = jevois::shader::frag_rgb;
      itsGLtextureWidth = width * 3; itsGLtextureFmt = GL_LUMINANCE;
      break;

    case V4L2_PIX_FMT_BGR24:
    case V4L2_PIX_FMT_SRGGB8:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_MJPEG:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_SBGGR16:
    case V4L2_PIX_FMT_SGRBG16:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_YUV444:
    case 0:
    default: LFATAL("Unsupported pixel format " << jevois::fccstr(fmt));
    }
    
    // Load the appropriate program:
    itsProgram.reset(new GPUprogram(jevois::shader::vert, frag_shader));
    
    // Get a handle to s_texture variable in the fragment shader, to update the texture with each new camera frame:
    itsLocation = glGetUniformLocation(itsProgram->id(), "s_texture");
  }
  
  // Generate a texture for incoming image data if needed:
  if (width != itsTextureWidth || height != itsTextureHeight || fmt != itsFormat || !itsTexture)
  {
    // Now create our texture:
    itsTexture.reset(new jevois::GPUtexture(itsGLtextureWidth, height, itsGLtextureFmt, false));
    LDEBUG("Input texture for " << width << 'x' << height << ' ' << jevois::fccstr(fmt) << " ready.");
  }

#ifdef JEVOIS_PLATFORM_PRO
  if (itsTextureDmaBuf) itsTextureDmaBuf.reset(); // invalidate any previously used dmabuf texture
#endif
  
  // Assign pixel data to our texture:
  itsTexture->setPixels(data);

  // Remember our latest size and format:
  itsTextureWidth = width; itsTextureHeight = height; itsFormat = fmt;
}

// ##############################################################################################################
void jevois::GPUimage::set(jevois::RawImage const & img)
{
  setInternal(img.width, img.height, img.fmt, static_cast<unsigned char const *>(img.buf->data()));
}

// ##############################################################################################################
void jevois::GPUimage::set(cv::Mat const & img, bool rgb)
{
  unsigned int fmt;
  
  switch(img.type())
  {
  case CV_8UC4: if (rgb) fmt = V4L2_PIX_FMT_RGB32; else fmt = V4L2_PIX_FMT_BGR32; break;
  case CV_8UC3: if (rgb) fmt = V4L2_PIX_FMT_RGB24; else fmt = V4L2_PIX_FMT_BGR24; break;
  case CV_8UC2: fmt = V4L2_PIX_FMT_YUYV; break;
  case CV_8UC1: fmt = V4L2_PIX_FMT_GREY; break;
  default: LFATAL("Unsupported OpenCV image format: " << img.type());
  }
  
  setInternal(img.cols, img.rows, fmt, img.data);
}

#ifdef JEVOIS_PLATFORM_PRO
// ##############################################################################################################
void jevois::GPUimage::set(jevois::InputFrame const & frame, EGLDisplay display)
{
  jevois::RawImage const img = frame.get();

  // EGLimageKHR which we use with DMAbuf requires width to be a multiple of 32; otherwise revert to normal texture:
  if (img.width % 32) { set(img); return; }

  // DMAbuf only supports some formats, otherwise revert to normal texture. Keep in sync with GPUtextureDmaBuf:
  switch (img.fmt)
  {
  case V4L2_PIX_FMT_YUYV:
  case V4L2_PIX_FMT_RGB32:
  case V4L2_PIX_FMT_RGB565:
  case V4L2_PIX_FMT_BGR24:
  case V4L2_PIX_FMT_RGB24:
  case V4L2_PIX_FMT_UYVY:
  {
    int const dmafd = frame.getDmaFd();
    setWithDmaBuf(img, dmafd, display);
    break;
  }
  default:
    set(img);
  }
}

// ##############################################################################################################
void jevois::GPUimage::set2(jevois::InputFrame const & frame, EGLDisplay display)
{
  jevois::RawImage const img = frame.get2();

  // EGLimageKHR which we use with DMAbuf requires width to be a multiple of 32; otherwise revert to normal texture:
  if (img.width % 32) { set(img); return; }
  
  // DMAbuf only supports some formats, otherwise revert to normal texture. Keep in sync with GPUtextureDmaBuf:
  switch (img.fmt)
  {
  case V4L2_PIX_FMT_YUYV:
  case V4L2_PIX_FMT_RGB32:
  case V4L2_PIX_FMT_RGB565:
  case V4L2_PIX_FMT_BGR24:
  case V4L2_PIX_FMT_RGB24:
  case V4L2_PIX_FMT_UYVY:
  {
    int const dmafd = frame.getDmaFd2();
    setWithDmaBuf(img, dmafd, display);
    break;
  }
  default:
    set(img);
  }
}

// ##############################################################################################################
void jevois::GPUimage::setWithDmaBuf(jevois::RawImage const & img, int dmafd, EGLDisplay display)
{
  if (img.width == 0 || img.height == 0) LFATAL("Cannot handle zero image width or height");

  // Generate a dmabuf texture for incoming image data if needed:
  if (img.width != itsTextureWidth || img.height != itsTextureHeight || img.fmt != itsFormat ||
      itsTexture || !itsTextureDmaBuf)
  {
    itsTextureDmaBuf.reset(new jevois::GPUtextureDmaBuf(display, img.width, img.height, img.fmt, dmafd));
    itsTexture.reset(); // invalidate any previously used regular texture
    LDEBUG("Input DMABUF texture for " << img.width <<'x'<< img.height << ' ' << jevois::fccstr(img.fmt) << " ready.");

    // Remember our latest size:
    itsTextureWidth = img.width; itsTextureHeight = img.height;

    // Load the appropriate program:
    if (itsTwirl) itsProgram.reset(new GPUprogram(jevois::shader::vert, jevois::shader::frag_oes_twirl));
    else itsProgram.reset(new GPUprogram(jevois::shader::vert, jevois::shader::frag_oes));
    
	// Get a handle to s_texture variable in the fragment shader, to update the texture with each new camera frame:
    itsLocation = glGetUniformLocation(itsProgram->id(), "s_texture");

    // Remember our latest format:
    itsFormat = img.fmt;
  }

  // All done. No need to assign pixel data, it will be DMA'd over.
}

#else // JEVOIS_PLATFORM_PRO

// ##############################################################################################################
void jevois::GPUimage::set(jevois::InputFrame const & frame, EGLDisplay JEVOIS_UNUSED_PARAM(display))
{
  // DMABUF acceleration not supported:
  jevois::RawImage const img = frame.get();
  set(img);
}

// ##############################################################################################################
void jevois::GPUimage::set2(jevois::InputFrame const & frame, EGLDisplay JEVOIS_UNUSED_PARAM(display))
{
  // DMABUF acceleration not supported:
  jevois::RawImage const img = frame.get2();
  set(img);
}

#endif // JEVOIS_PLATFORM_PRO

// ##############################################################################################################
void jevois::GPUimage::draw(int & x, int & y, unsigned short & w, unsigned short & h, bool noalias,
                            glm::mat4 const & pvm)
{
  if (itsTextureWidth == 0) throw std::runtime_error("You must call set() before draw()");

  // Enable blending, used for RGBA textures that have transparency:
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Get the current viewport size:
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);

  if (w == 0 || h == 0)
  {
    // Letterbox the image into the window to occupy as much space as possible without changing aspect ratio:
    unsigned int winw = viewport[2], winh = viewport[3];
    itsDrawWidth = itsTextureWidth; itsDrawHeight = itsTextureHeight;
    jevois::applyLetterBox(itsDrawWidth, itsDrawHeight, winw, winh, noalias);
    x = (winw - itsDrawWidth) / 2; y = (winh - itsDrawHeight) / 2; w = itsDrawWidth; h = itsDrawHeight;
  }

  // Flip the ordinate. Our OpenGL texture rendering uses 0,0 at the bottom-left corner, with increasing y going up on
  // the screen. But machine vision (and our users) assumes 0,0 at top left of the screen with increasing y going down:
  int const yy = viewport[3] - y - h;
  
  // Allocate/update vertex arrays if needed (including on first frame):
  if (itsDrawX != x || itsDrawY != y || itsDrawWidth != w || itsDrawHeight != h)
  {
    // Compute vertex coordinates: We here assume that the projection matrix is such that at the pixel perfect plane,
    // our vertex x,y coordinates match their pixel counterparts, except that image center is at 0,0. The view matrix is
    // responsible for translating our z from 0 here to the pixel pixel perfect plane:
    float const tx = x - 0.5 * viewport[2];
    float const ty = yy - 0.5 * viewport[3];
    float const bx = tx + w;
    float const by = ty + h;

    // Create a new vertex array. A group of vertices and indices stored in GPU memory.  The frame being drawn is static
    // for the program duration, upload all the information to the GPU at the beginning then reference the location each
    // frame without copying the data each frame.
    GLfloat const vertices[] = { tx, by, 0.0f,         0.0f, 0.0f,
                                 tx, ty, 0.0f,         0.0f, 1.0f,
                                 bx, ty, 0.0f,         1.0f, 1.0f,
                                 bx, by, 0.0f,         1.0f, 0.0f  };
    static GLushort const indices[] = { 0, 1, 2, 0, 2, 3 };

    if (itsVertexArray) { glDeleteVertexArrays(1, &itsVertexArray); glDeleteBuffers(2, itsVertexBuffers); }
	glGenVertexArrays(1, &itsVertexArray);
    
	// Select the vertex array that was just created and bind the new vertex buffers to the array:
	glBindVertexArray(itsVertexArray);
    
    // Generate vertex buffers in GPU memory. First buffer is for vertex data, second for index data:
    if (itsVertexBuffers[0]) glDeleteBuffers(2, itsVertexBuffers);
	glGenBuffers(2, itsVertexBuffers);
    
	// Bind the first vertex buffer with the vertices to the vertex array:
	glBindBuffer(GL_ARRAY_BUFFER, itsVertexBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
	// Bind the second vertex buffer with the indices to the vertex array:
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, itsVertexBuffers[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
	// Enable a_position (location 0) and a_tex_coord (location 1) in the vertex shader:
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
    
	// Copy the vertices to the GPU to be used in a_position:
	GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0));
    
	// Copy the texture co-ordinates to the GPU to be used in a_tex_coord:
	GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void const *)(3 * sizeof(GLfloat))));
    
    // Select the default vertex array, allowing the application's array to be unbound:
	glBindVertexArray(0);

    // Remember our latest draw location and size:
    itsDrawX = x; itsDrawY = y; itsDrawWidth = w; itsDrawHeight = h;
  }
  
  if (!itsProgram) throw std::runtime_error("You must call set() before draw()");
  
  // Tell OpenGL to use our program:
  glUseProgram(itsProgram->id());
    
  // Select the vertex array which includes the vertices and indices describing the window rectangle:
  glBindVertexArray(itsVertexArray);
  
  // Bind our texture, regular or DMABUF:
  GL_CHECK(glActiveTexture(GL_TEXTURE0));
  if (itsTexture)
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, itsTexture->Id));
#ifdef JEVOIS_PLATFORM_PRO
  else if (itsTextureDmaBuf)
    GL_CHECK(glBindTexture(GL_TEXTURE_EXTERNAL_OES, itsTextureDmaBuf->Id));
#endif
  else throw std::runtime_error("You must call set() before draw()");
  
  // Indicate that GL_TEXTURE0 is s_texture from previous lookup:
  glUniform1i(itsLocation, 0);
  
  // Let the fragment shader know the true (unscaled by bpp) image width and height. Also set the PVM matrix. Ignore any
  // errors as some shaders may not have these variables:
  glUniform2f(glGetUniformLocation(itsProgram->id(), "tdim"), GLfloat(itsTextureWidth), GLfloat(itsTextureHeight));
  glUniformMatrix4fv(glGetUniformLocation(itsProgram->id(), "pvm"), 1, GL_FALSE, glm::value_ptr(pvm));

  if (itsTwirl) glUniform1f(glGetUniformLocation(itsProgram->id(), "twirlamount"), itsTwirlAmount);

  // Draw the two triangles from 6 indices to form a rectangle from the data in the vertex array.
  // The fourth parameter, indices value here is passed as null since the values are already
  // available in the GPU memory through the vertex array
  // GL_TRIANGLES - draw each set of three vertices as an individual triangle.
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
  
  // Select the default vertex array, allowing the application array to be unbound:
  glBindVertexArray(0);
}

// ##############################################################################################################
ImVec2 jevois::GPUimage::i2d(ImVec2 const & p)
{
  if (itsDrawWidth == 0) throw std::runtime_error("Need to call set() then draw() first");
  return ImVec2(itsDrawX + p.x * itsDrawWidth / itsTextureWidth, itsDrawY + p.y * itsDrawHeight / itsTextureHeight);
}

// ##############################################################################################################
ImVec2 jevois::GPUimage::i2ds(ImVec2 const & p)
{
  if (itsDrawWidth == 0) throw std::runtime_error("Need to call set() then draw() first");
  return ImVec2(p.x * itsDrawWidth / itsTextureWidth, p.y * itsDrawHeight / itsTextureHeight);
}

// ##############################################################################################################
ImVec2 jevois::GPUimage::d2i(ImVec2 const & p)
{
  if (itsDrawWidth == 0) throw std::runtime_error("Need to call set() then draw() first");
  return ImVec2((p.x - itsDrawX) * itsTextureWidth / itsDrawWidth, (p.y - itsDrawY) * itsTextureHeight / itsDrawHeight);
}

// ##############################################################################################################
ImVec2 jevois::GPUimage::d2is(ImVec2 const & p)
{
  if (itsDrawWidth == 0) throw std::runtime_error("Need to call set() then draw() first");
  return ImVec2(p.x * itsTextureWidth / itsDrawWidth, p.y * itsTextureHeight / itsDrawHeight);
}

// ##############################################################################################################
void jevois::GPUimage::setTwirl(float t)
{
  if (itsTwirl == false) LERROR("Need to construct GPU image with twirl enabled to use twirl -- IGNORED");
  itsTwirlAmount = t;
}

#endif // JEVOIS_PRO
