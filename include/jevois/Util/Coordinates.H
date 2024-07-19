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

#include <jevois/Image/RawImage.H>

//! Main namespace for all JeVois classes and functions
/*! See [Topics](http://jevois.org/doc/html/topics.html) for more details. */
namespace jevois
{
  //! Namespace for helper functions to convert coordinates from camera resolution to standardized
  /*! See \ref coordhelpers for more details. */
  namespace coords
  {
    /*! \defgroup coordhelpers Helper functions to convert coordinates from camera resolution to standardized
        
        Different machine vision algorithms in JeVois may be able to operate with different camera resolutions, such as
        1280x1024, 320x240, or 176x144. When some item of interest is detected in the camera frame, one may often want
        to send the coordinates of that thing to the serial port. This poses a problem if one were to directly send the
        image coordinates of the item out, which is that now the receiver (e.g., an Arduino) needs to know which camera
        image resolution was used, so that it can properly interpret these coordinates. For example, if the visual
        attention (saliency) algorithm is running with 640x480 camera input, then a salient object at the center of the
        camera's field of view would have coordinates 320,240. But if the same saliency algorithm is configured to
        process 320x240 input video (so that it can run at a higher framerate), now an object at the center of the field
        of view would have coordinates 160,120. If one connects an Arduino that controls, for example, a pan/tilt head
        to JeVois, we need a way to communicate coordinates of target objects in the world independently of the video
        resolution used by the camera.
        
        Thus, JeVois defines a standardized coordinate system, as follows:
        
        - center fo the camera's field of view is at x=0, y=0
        - left edge of the camera image is always at x=-1000
        - right edge of the camera image is always at x=1000
        - top edge of the camera image is usually at y=-750
        - bottom edge of the camera image is usually at y=750
        
        Note that the value of 750 here comes from the assumption of a 4:3 aspect ratio for the camera sensor, and is
        actually defined in JEVOIS_CAMERA_ASPECT. All camera sensor resolutions use 4:3 aspect ratio and hence have y
        values between -750 and 750, except for 1280x1024, which has y values between -800 and 800.
        
        When writing a machine vision algorithm that sends over serial the coordinates of things detected in the camera
        frames, be sure to first transform those coordinates from image to standardized space.
        
        For more on how standardized coordinates are used to communicate with embedded controllers, and for 3D
        coordinates, see \ref UserSerialStyle

        \ingroup utils */

    //! Aspect ratio of the JeVois camera
    /*! \ingroup coordhelpers */
#ifdef JEVOIS_PRO
#define JEVOIS_CAMERA_ASPECT (16.0 / 9.0)
#else
#define JEVOIS_CAMERA_ASPECT (4.0 / 3.0)
#endif
    
    //! Transform coordinates in-place from camera to standardized, using a RawImage to establish image size
    /*! The RawImage from the camera is used to specify pixel width and height of the camera image, and this is the
        source coordinate system. The destination coordinate system is the standardized one, with x in [-1000 ... 1000]
        and y in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void imgToStd(float & x, float & y, RawImage const & camimg, float const eps = 0.1F);
    
    //! Transform coordinates in-place from camera to standardized, using given image width and height
    /*! The width and height are used to specify pixel width and height of the camera image, and this is the source
        coordinate system. The destination coordinate system is the standardized one, with x in [-1000 ... 1000] and y
        in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void imgToStd(float & x, float & y, unsigned int const width, unsigned int const height, float const eps = 0.1F);

    //! Transform X coordinate in-place from camera to standardized, using given image width and height
    /*! The width is used to specify pixel width of the camera image, and this is the source
        coordinate system. The destination coordinate system is the standardized one, with x in [-1000 ... 1000].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void imgToStdX(float & x, unsigned int const width, float const eps = 0.1F);
    
    //! Transform Y coordinate in-place from camera to standardized, using given image width and height
    /*! The height is used to specify pixel height of the camera image, and this is the source
        coordinate system. The destination coordinate system is the standardized one, with y in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void imgToStdY(float & y, unsigned int const height, float const eps = 0.1F);

    //! Transform size in-place from camera to standardized, using given image width and height
    /*! Arguments w and h define the size of an object in pixels, which will be converted to standardized units. The
        width and height are used to specify pixel width and height of the camera image, and this is the source
        coordinate system. The destination coordinate system is the standardized one, with x in [-1000 ... 1000] and y
        in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void imgToStdSize(float & w, float & h, unsigned int const width, unsigned int const height,
                      float const eps = 0.1F);

    //! Transform coordinates in-place from standardized to image, using a RawImage to establish image size
    /*! The RawImage would typically be from the camera is used to specify pixel width and height of the camera image,
        and this is the destination coordinate system. The source coordinate system is the standardized one, with x in
        [-1000 ... 1000] and y in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void stdToImg(float & x, float & y, RawImage const & camimg, float const eps = 0.1F);

    //! Transform coordinates in-place from standardized to image, using a RawImage to establish image size
    /*! The width and height would typically be from the camera and are used to specify pixel width and height of the
        camera image, and this is the destination coordinate system. The source coordinate system is the standardized
        one, with x in [-1000 ... 1000] and y in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void stdToImg(float & x, float & y, unsigned int const width, unsigned int const height, float const eps = 0.1F);

    //! Transform size in-place from standardized to image, using a RawImage to establish image size
    /*! Arguments w and h define the size of an object in standardized units, which will be converted to pixels. The
        width and height would typically be from the camera and are used to specify pixel width and height of the
        camera image, and this is the destination coordinate system. The source coordinate system is the standardized
        one, with x in [-1000 ... 1000] and y in [-750 ... 750].

        eps is used for rounding of returned coordinates, which is convenient to avoid sending very long floating point
        values over serial port.

        \ingroup coordhelpers */
    void stdToImgSize(float & x, float & y, unsigned int const width, unsigned int const height,
                      float const eps = 0.1F);
  }
}

