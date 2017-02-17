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

// This file has been modified / imported from the Neuromorphic Robotics Toolkit (NRT). Original copyright is:

/* author Randolph Voorhies
    // ////////////////////////////////////////////////////////////////////////
    //              The iLab Neuromorphic Robotics Toolkit (NRT)             //
    // Copyright 2010-2012 by the University of Southern California (USC)    //
    //                          and the iLab at USC.                         //
    //                                                                       //
    //                iLab - University of Southern California               //
    //                Hedco Neurociences Building, Room HNB-10               //
    //                    Los Angeles, Ca 90089-2520 - USA                   //
    //                                                                       //
    //      See http://ilab.usc.edu for information about this project.      //
    // ////////////////////////////////////////////////////////////////////////
    // This file is part of The iLab Neuromorphic Robotics Toolkit.          //
    //                                                                       //
    // The iLab Neuromorphic Robotics Toolkit is free software: you can      //
    // redistribute it and/or modify it under the terms of the GNU General   //
    // Public License as published by the Free Software Foundation, either   //
    // version 3 of the License, or (at your option) any later version.      //
    //                                                                       //
    // The iLab Neuromorphic Robotics Toolkit is distributed in the hope     //
    // that it will be useful, but WITHOUT ANY WARRANTY; without even the    //
    // implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       //
    // PURPOSE.  See the GNU General Public License for more details.        //
    //                                                                       //
    // You should have received a copy of the GNU General Public License     //
    // along with The iLab Neuromorphic Robotics Toolkit.  If not, see       //
    // <http://www.gnu.org/licenses/>.                                       //
    // ////////////////////////////////////////////////////////////////////////
*/

#pragma once

#include <functional>
#include <string>
#include <exception>
#include <dlfcn.h>
#include <jevois/Debug/Log.H>

namespace jevois
{
  //! Class to open shared object (.so) files and load functions contained in them
  /*! This class allows one to open a shared library (.so file) and to invoke functions or classes that are in that
      library.

      Engine uses DynamicLoader to load the appropriate vision processing Module each time the user selects a new
      VideoMapping (i.e., changes image resolution in a USB webcam program running on a host computer, or invokes the
      \c setmapping command over Serial port).

      This class is modified from DynamicLoader in the Neuromorphic Robotics Toolkit (NRT). \ingroup core */
  class DynamicLoader
  {
    public:
      //! Open the shared object located at filename, throws if fail
      /*! If closeOnDestroy is true, the .so file will be closed when the DynamicLoader object is destroyed. If you want
          to maintain access to the functions and classes in a .so file after the loader has run out of scope, set
          closeOnDestroy to false (and you will then have no way of ever unloading that .so file until termination of
          your program). */
      DynamicLoader(std::string const & filename, bool closeOnDestroy);

      //! Retrieve the path to the .so file of this loader, useful to avoid closing and re-loading the same .so file
      std::string const & sopath() const;
      
      //! Destructor, may close the object on destroy (see constructor arguments)
      ~DynamicLoader();
      
      //! Close the shared object file
      /*! Beware that closing the object file will remove all links to any objects/functions/etc that have been created
          from the shared object.  Use this function only if you want nothing more to do with anything created from this
          object file. */
      void close();

      //! Load the symbol named functionName
      /*! This method returns an std::function that points to the requested name in the shared object opened by this
          DynamicLoader. For example, to load the function <code>int foo(double x, bool y)</code> from the file
          "mySharedObject.so" you should write:

          @code
          jevois::DynamicLoader loader("./mySharedObject.so");
          try 
          {
            std::function<int(double,bool)> foo = loader.load<int(double,bool)>("foo");
            int result = foo(3.0, true);
          }
          catch (std::exception const & e)
          {
           LFATAL(e.what());
          }
          @endcode

          You should not try to use anything returned by load() after the DynamicLoader has been close()'d (or
          destroyed, see constructor arguments).

          Throws if the requested symbol could not be found in the opened shared object. */
      template <class Signature>
      std::function<Signature> load(std::string const & functionName);

    private:
      bool itsCloseOnDestroy;
      void * itsLibraryHandle;
      std::string itsFilename;
  };
}


// Implementation details:
#include <jevois/Core/details/DynamicLoaderImpl.H>
