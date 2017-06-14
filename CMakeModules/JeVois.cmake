######################################################################################################################
#
# JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2016 by Laurent Itti, the University of Southern
# California (USC), and iLab at USC. See http://iLab.usc.edu and http://jevois.org for information about this project.
#
# This file is part of the JeVois Smart Embedded Machine Vision Toolkit.  This program is free software; you can
# redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software
# Foundation, version 2.  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.  You should have received a copy of the GNU General Public License along with this program;
# if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, CA 90089-2520 - USA.
# Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
######################################################################################################################

# CMake helper functions for JeVois and additional libraries and modules:

message(STATUS "JeVois version ${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}")

# Platform choice:
option(JEVOIS_PLATFORM "Cross-compile for hardware platform" OFF)
message(STATUS "JEVOIS_PLATFORM: ${JEVOIS_PLATFORM}")

# Vendor name for modules, etc:
if (NOT DEFINED JEVOIS_VENDOR)
  if (DEFINED ENV{JEVOIS_VENDOR})
    set(JEVOIS_VENDOR $ENV{JEVOIS_VENDOR})
  else (DEFINED ENV{JEVOIS_VENDOR})
    set(JEVOIS_VENDOR "Unknown" CACHE STRING "Module vendor name")
  endif (DEFINED ENV{JEVOIS_VENDOR})
endif (NOT DEFINED JEVOIS_VENDOR)
message(STATUS "JEVOIS_VENDOR: ${JEVOIS_VENDOR}")

# Settings for native host compilation or hardware platform compilation:
if (JEVOIS_PLATFORM)

  # On platform, install to jvpkg, staging area, or live microSD?
  option(JEVOIS_MODULES_TO_STAGING "Install modules to ${JEVOIS_PLATFORM_INSTALL_PREFIX} as opposed to jvpkg" OFF)
  message(STATUS "JEVOIS_MODULES_TO_STAGING: ${JEVOIS_MODULES_TO_STAGING}")
  option(JEVOIS_MODULES_TO_MICROSD "Install modules to /media/$ENV{USER}/JEVOIS as opposed to jvpkg" OFF)
  message(STATUS "JEVOIS_MODULES_TO_MICROSD: ${JEVOIS_MODULES_TO_MICROSD}")

  set(CMAKE_C_COMPILER ${JEVOIS_PLATFORM_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${JEVOIS_PLATFORM_CXX_COMPILER})
  set(JEVOIS_INSTALL_PREFIX ${JEVOIS_PLATFORM_INSTALL_PREFIX})
  set(JEVOIS_OPENCV_LIBS ${JEVOIS_PLATFORM_OPENCV_LIBS})
  set(JEVOIS_PYTHON_LIBS ${JEVOIS_PLATFORM_PYTHON_LIBS})
  set(JEVOIS_MODULES_ROOT ${JEVOIS_PLATFORM_MODULES_ROOT})
  set(JEVOIS_ARCH_FLAGS ${JEVOIS_PLATFORM_ARCH_FLAGS})
  set(JEVOIS_CFLAGS ${JEVOIS_PLATFORM_CFLAGS})

else (JEVOIS_PLATFORM)

  set(JEVOIS_INSTALL_PREFIX ${JEVOIS_HOST_INSTALL_PREFIX})
  set(JEVOIS_OPENCV_LIBS ${JEVOIS_HOST_OPENCV_LIBS})
  set(JEVOIS_PYTHON_LIBS ${JEVOIS_HOST_PYTHON_LIBS})
  set(JEVOIS_MODULES_ROOT ${JEVOIS_HOST_MODULES_ROOT})
  set(JEVOIS_ARCH_FLAGS ${JEVOIS_HOST_ARCH_FLAGS})
  set(JEVOIS_CFLAGS ${JEVOIS_HOST_CFLAGS})
  
endif (JEVOIS_PLATFORM)

message(STATUS "Install prefix for executables: ${JEVOIS_INSTALL_PREFIX}")
message(STATUS "Host path to jevois modules root: ${JEVOIS_MODULES_ROOT}")

####################################################################################################
# from http://stackoverflow.com/questions/7787823/cmake-how-to-get-the-name-of-all-subdirectories-of-a-directory
macro(subdirlist result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if (IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

####################################################################################################
# Helper to start a project:
macro(jevois_project_set_flags)
  # Pass the compiler flags to cmake (doing this before project() gives problems); same with the install prefix:
  set(CMAKE_C_FLAGS "-std=gnu99 ${JEVOIS_CFLAGS} -I${JEVOIS_INSTALL_PREFIX}/include -include jevois/Config/Config.H")
  set(CMAKE_CXX_FLAGS "-std=c++17 ${JEVOIS_CFLAGS} -I${JEVOIS_INSTALL_PREFIX}/include -include jevois/Config/Config.H")
  set(CMAKE_INSTALL_PREFIX ${JEVOIS_INSTALL_PREFIX})
  link_directories(${JEVOIS_INSTALL_PREFIX}/lib) # to find libjevois

  # Check for optional JEVOIS_SDK_ROOT environment variable, or use /usr/share/jevois-sdk by default:
  if (DEFINED ENV{JEVOIS_SDK_ROOT})
    set(JEVOIS_SDK_ROOT $ENV{JEVOIS_SDK_ROOT})
  else (DEFINED ENV{JEVOIS_SDK_ROOT})
    set(JEVOIS_SDK_ROOT "/usr/share/jevois-sdk")
  endif (DEFINED ENV{JEVOIS_SDK_ROOT})
  message(STATUS "JeVois SDK root: ${JEVOIS_SDK_ROOT}")

  # add a dependency and command to create the jvpkg package:
  add_custom_target(jvpkg
    COMMAND jevois-jvpkg ../${JEVOIS_VENDOR}_${CMAKE_PROJECT_NAME}.jvpkg
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jvpkg )
  
  # Check for JEVOIS_ROOT environment variable:
  if (DEFINED ENV{JEVOIS_ROOT})
    set(JEVOIS_ROOT $ENV{JEVOIS_ROOT})
  else (DEFINED ENV{JEVOIS_ROOT})
    set(JEVOIS_ROOT "/jevois")
    message(WARNING "You should set JEVOIS_ROOT environment variable to the root of where JeVois modules and "
      "config files are installed. Using default: ${JEVOIS_ROOT}")
  endif (DEFINED ENV{JEVOIS_ROOT})

endmacro()

####################################################################################################
# Setup some modules, base scenario:
macro(jevois_setup_modules basedir deps)
  subdirlist(JV_MODULEDIRS "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}")
  foreach (JV_MODULE ${JV_MODULEDIRS})
    # Find all C++ source files for this module:
    file(GLOB_RECURSE MODFILES ${basedir}/${JV_MODULE}/*.[Cc] ${basedir}/${JV_MODULE}/*.cpp)

    # Find all python source files for this module:
    file(GLOB_RECURSE PYFILES ${basedir}/${JV_MODULE}/*.[Pp][Yy])
    
    if (MODFILES)
      # Get this C++ module compiled:
      message(STATUS "Adding compilation directives for C++ module ${JV_MODULE} base ${basedir}")
      add_library(${JV_MODULE} SHARED ${MODFILES})
      set_target_properties(${JV_MODULE} PROPERTIES PREFIX "") # no lib prefix in the .so name
      target_link_libraries(${JV_MODULE} jevois)
      if (${deps})
	add_dependencies(${JV_MODULE} ${deps})
      endif (${deps})

      # add a dependency and command to build modinfo.yaml:
      add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.yaml" "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.html"
	COMMAND jevois-modinfo ${JV_MODULE}.C
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.C
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE})

      add_custom_target(modinfo_${JV_MODULE}
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.yaml" ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.C)
      add_dependencies(${JV_MODULE} modinfo_${JV_MODULE})

    endif (MODFILES)

    if (PYFILES)
      # Get this python module ready
      message(STATUS "Adding setup directives for Python module ${JV_MODULE} base ${basedir}")

      # add a dependency and command to build modinfo.yaml:
      add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.yaml" "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.html"
	COMMAND jevois-modinfo ${JV_MODULE}.py
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.py
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE})

      add_custom_target(modinfo_${JV_MODULE} ALL
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.yaml" ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.py)

    endif (PYFILES)
    
    # On platform, we install to jvpkg directory, staging, or live microsd; on host we always install to
    # JEVOIS_MODULES_ROOT which usually is /jevois:
    if (JEVOIS_PLATFORM)
      if (JEVOIS_MODULES_TO_MICROSD) # if both are specified, microsd takes preference over staging
	set(DESTDIR "/media/$ENV{USER}/JEVOIS/modules/${JEVOIS_VENDOR}")
      else (JEVOIS_MODULES_TO_MICROSD)
	if (JEVOIS_MODULES_TO_STAGING)
	  set(DESTDIR "${JEVOIS_PLATFORM_INSTALL_PREFIX}/modules/${JEVOIS_VENDOR}")
	else (JEVOIS_MODULES_TO_STAGING)
	  set(DESTDIR "${CMAKE_CURRENT_SOURCE_DIR}/jvpkg/modules/${JEVOIS_VENDOR}")
	endif (JEVOIS_MODULES_TO_STAGING)
      endif (JEVOIS_MODULES_TO_MICROSD)
    else (JEVOIS_PLATFORM)
      set(DESTDIR "${JEVOIS_MODULES_ROOT}/modules/${JEVOIS_VENDOR}")
    endif (JEVOIS_PLATFORM)
      
    # Install everything that is in that directory except for the source file:
    install(DIRECTORY ${basedir}/${JV_MODULE} DESTINATION "${DESTDIR}"
      PATTERN "*.[hHcC]" EXCLUDE
      PATTERN "*.hpp" EXCLUDE
      PATTERN "*.cpp" EXCLUDE
      PATTERN "modinfo.*" EXCLUDE
      PATTERN "*~" EXCLUDE
      PATTERN "*ubyte" EXCLUDE # tiny-dnn training data
      PATTERN "*.bin" EXCLUDE # tiny-dnn training data
      PATTERN "__pycache__" EXCLUDE) # compiled python

    # Install the compiled module .so itself:
    if (MODFILES)
      install(TARGETS ${JV_MODULE} LIBRARY DESTINATION "${DESTDIR}/${JV_MODULE}")
    endif (MODFILES)
    
  endforeach()
endmacro()

####################################################################################################
# Setup a library
macro(jevois_setup_library basedir libname libversion)
  file(GLOB_RECURSE JV_LIB_SRC_FILES ${basedir}/*.[Cc] ${basedir}/*.cpp)
  add_library(${libname} SHARED ${JV_LIB_SRC_FILES})

  # On host, make sure we prefer /usr/local/lib, where latest opencv is:
  if (JEVOIS_PLATFORM)
    target_link_libraries(${libname} jevois)
  else (JEVOIS_PLATFORM)
    target_link_libraries(${libname} -L/usr/local/lib jevois)
  endif (JEVOIS_PLATFORM)

  # Add version information, this will create symlinks as needed:
  if (NOT JEVOIS_PLATFORM)
    set_target_properties(${libname} PROPERTIES VERSION "${libversion}" SOVERSION ${libversion})
  endif (NOT JEVOIS_PLATFORM)

  link_libraries(${libname})

  # On platform, we install to jvpkg directory, staging, or live microsd; on host we always install to
  # JEVOIS_MODULES_ROOT which usually is /jevois:
  if (JEVOIS_PLATFORM)
    if (JEVOIS_MODULES_TO_MICROSD) # if both are specified, microsd takes preference over staging
	install(TARGETS ${libname} LIBRARY
	  DESTINATION "/media/$ENV{USER}/JEVOIS/lib/${JEVOIS_VENDOR}"
	  COMPONENT libs)
    else (JEVOIS_MODULES_TO_MICROSD)
      if (JEVOIS_MODULES_TO_STAGING)
	install(TARGETS ${libname} LIBRARY
	  DESTINATION "${JEVOIS_PLATFORM_INSTALL_PREFIX}/lib/${JEVOIS_VENDOR}"
	  COMPONENT libs)
      else (JEVOIS_MODULES_TO_STAGING)
	install(TARGETS ${libname} LIBRARY
	  DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/jvpkg/lib/${JEVOIS_VENDOR}"
	  COMPONENT libs)
      endif (JEVOIS_MODULES_TO_STAGING)
    endif (JEVOIS_MODULES_TO_MICROSD)
  else (JEVOIS_PLATFORM)
    install(TARGETS ${libname} LIBRARY DESTINATION lib COMPONENT libs)
  endif (JEVOIS_PLATFORM)
    
endmacro()

####################################################################################################
# Setup packaging for debian using cpack
macro(jevois_setup_cpack packagename)
  # Create packages (Debian, RPM): in hbuild/ or pbuild/, just type 'sudo cpack' to create the package.
  # To list the files created in a package, run: dpkg -c <package.deb>
  if (JEVOIS_PLATFORM)
    set(CPACK_PACKAGE_NAME "${packagename}-platform")
    set(JEVOIS_CPACK_ARCH "all")
    set(CPACK_SET_DESTDIR ON) # needed to avoid having cpack use default install dir
  else (JEVOIS_PLATFORM)
    set(CPACK_PACKAGE_NAME "${packagename}-host")
  execute_process(COMMAND dpkg --print-architecture
    OUTPUT_VARIABLE JEVOIS_CPACK_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
   endif (JEVOIS_PLATFORM)

  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/scripts/postinst;${CMAKE_SOURCE_DIR}/scripts/prerm;")
  set(CPACK_GENERATOR "DEB;")  # could be DEB;RPM;

  # Use a file name that includes debian version so we can publish the same package for different ubunt versions:
  # Format is: <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
  # We cannot just do: set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")  because some packages here have arch 'all'
  execute_process(COMMAND lsb_release -rs
    OUTPUT_VARIABLE UBU_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-ubuntu${UBU_RELEASE}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}_${JEVOIS_CPACK_ARCH}")

  SET(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}/*")

  include(CPack)

endmacro()
