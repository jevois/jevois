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

#include <jevois/Image/ColorConversion.h>

#define CLAMP(value) if (value < 0) value = 0; else if (value > 255) value = 255;

// ####################################################################################################
void convertYUYVtoRGB24(unsigned int w, unsigned int h, unsigned char const * srcptr, unsigned char * dstptr)
{
  // FIXME This code does not work if w is odd
  const int K1 = (int)(1.402f * (1 << 16));
  const int K2 = (int)(0.714f * (1 << 16));
  const int K3 = (int)(0.334f * (1 << 16));
  const int K4 = (int)(1.772f * (1 << 16));
  const unsigned int pitch = w * 2; // 2 bytes per one YU-YV pixel
  unsigned int x, y;
  unsigned char Y1, Y2;
  int uf, vf, R, G, B;
  
  for (y = 0; y < h; ++y)
  {
    for (x = 0; x < pitch; x += 4)  // Y1 U Y2 V
    {
      Y1 = *srcptr++; uf = *srcptr++ - 128; Y2 = *srcptr++; vf = *srcptr++ - 128;
 
      R = Y1 + (K1 * vf >> 16);
      G = Y1 - (K2 * vf >> 16) - (K3 * uf >> 16);
      B = Y1 + (K4 * uf >> 16);
      CLAMP(R); CLAMP(G); CLAMP(B);
      *dstptr++ = (unsigned char)(R); *dstptr++ = (unsigned char)(G); *dstptr++ = (unsigned char)(B);
 
      R = Y2 + (K1 * vf >> 16);
      G = Y2 - (K2 * vf >> 16) - (K3 * uf >> 16);
      B = Y2 + (K4 * uf >> 16);
      CLAMP(R); CLAMP(G); CLAMP(B);
      *dstptr++ = (unsigned char)(R); *dstptr++ = (unsigned char)(G); *dstptr++ = (unsigned char)(B);
    }
  }
}

// ####################################################################################################
inline void convertYUYVtoRGBYLinternal(int R, int G, int B, int * dstrg, int * dstby, int * dstlum,
                                       int thresh, int lshift, int lumlshift)
{
  CLAMP(R); CLAMP(G); CLAMP(B);
  
  int L = R + G + B;
  *dstlum = (L / 3) << lumlshift;
  
  if (L < thresh)
  {
    *dstrg = 0;
    *dstby = 0;
  }
  else
  {
    int red = (2 * R - G - B);
    int green = (2 * G - R - B);
    int blue = (2 * B - R - G);
    int rg = R - G; if (rg < 0) rg = -rg;
    int yellow = -2 * blue - 4 * rg;
    
    if (red < 0) red = 0;
    if (green < 0) green = 0;
    if (blue < 0) blue = 0;
    if (yellow < 0) yellow = 0;
    
    *dstrg = (3 * (red - green) << lshift) / L;
    *dstby = (3 * (blue - yellow) << lshift) / L;
  }
}

// ####################################################################################################
void convertYUYVtoRGBYL(unsigned int w, unsigned int h, unsigned char const * srcptr, int * dstrg,
                        int * dstby, int * dstlum, int thresh, int inputbits)
{
  // FIXME This code does not work if w is odd
  const int K1 = (int)(1.402f * (1 << 16));
  const int K2 = (int)(0.714f * (1 << 16));
  const int K3 = (int)(0.334f * (1 << 16));
  const int K4 = (int)(1.772f * (1 << 16));
  const unsigned int pitch = w * 2; // 2 bytes per one YU-YV pixel
  const int lshift = inputbits - 3; // FIXME assumes inputbits > 3
  const int lumlshift = inputbits - 8; // FIXME assumes inputbits > 8; why two different shifts?

  unsigned int x, y;
  unsigned char Y1, Y2;
  int uf, vf, R, G, B;
  
  for (y = 0; y < h; ++y)
  {
    for (x = 0; x < pitch; x += 4)  // Y1 U Y2 V
    {
      Y1 = *srcptr++; uf = *srcptr++ - 128; Y2 = *srcptr++; vf = *srcptr++ - 128;
 
      R = Y1 + (K1 * vf >> 16);
      G = Y1 - (K2 * vf >> 16) - (K3 * uf >> 16);
      B = Y1 + (K4 * uf >> 16);

      convertYUYVtoRGBYLinternal(R, G, B, dstrg, dstby, dstlum, thresh, lshift, lumlshift);

      ++dstrg; ++dstby; ++dstlum;
 
      R = Y2 + (K1 * vf >> 16);
      G = Y2 - (K2 * vf >> 16) - (K3 * uf >> 16);
      B = Y2 + (K4 * uf >> 16);

      convertYUYVtoRGBYLinternal(R, G, B, dstrg, dstby, dstlum, thresh, lshift, lumlshift);

      ++dstrg; ++dstby; ++dstlum;
    }
  }
}
