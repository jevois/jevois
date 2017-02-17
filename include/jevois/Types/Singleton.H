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
   copyright GNU Public License (GPL v3)
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

namespace jevois
{
  //! A generic singleton class to enforce a single instance of an object
  /*! The Singleton class should be inherited by any class which needs:
        -# To have only one instance
        -# To be globally accessible

      Note that there are currently no guarantees on the order of destruction of Singletons, so care should be taken to
      never reference another Singleton in the destructor of a Singleton.

      This particular implementation is from:
      http://stackoverflow.com/questions/1008019/c-singleton-design-pattern

      The following is an example use of the Singleton:

      @code
      class MyClass : public Singleton<MyClass>
      {
      public:
        void doIt() {...}

      private:
        MyClass();
      };

      int main()
      {
        ...
        MyClass::instance().doIt();
        ...
      }
      @endcode

      @tparam T The class that is being made the singleton.

      This class was first designed by Rand Voorhies in the Neuromorphic Robotics Toolkit.
      \ingroup types */
  template <class T>
  class Singleton
  {
    public:
      //! Get the global, unique instance of the class T
      /*! If the global, unique instance of class T has not yet been created, then this method will create it in a
          thread-safe manner. */
      static T & instance();

    protected:
      //! Only classes that inherit from Singleton can construct it via its protected default constructor
      Singleton();

    private:
      // Prevent copy-construction and copy, only allow default construction
      Singleton(Singleton const &) = delete;
      void operator=(Singleton const &) = delete;
  };  
}

// Include implementation details:
#include <jevois/Types/details/SingletonImpl.H>
