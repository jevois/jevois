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
# Contact information: Laurent Itti - 3641 Watt Way, HNB-07A - Los Angeles, BA 90089-2520 - USA.
# Tel: +1 213 740 3527 - itti@pollux.usc.edu - http://iLab.usc.edu - http://jevois.org
######################################################################################################################

## CMake build rules for JeVois Base library and executables

## You may provide the installed JeVois config root as:
#   cmake -DJEVOIS_CONFIG=/jevois/config ..

cmake_minimum_required(VERSION 3.6)

## Set vendor names, our modules will be placed in a directory by that name under /jevois/modules:
set(JEVOIS_VENDOR "__VENDOR__")
set(JEVOIS_HARDWARE "PRO")
set(JEVOIS_CONFIG "/jevoispro/config" CACHE STRING "Path to JeVois config to use")

## Set a version for your module, used to create the .deb package for platform:
set(MOD_VERSION_MAJOR 1)
set(MOD_VERSION_MINOR 0)
set(MOD_VERSION_PATCH 0)

## Include helper functions, config, etc from the JeVois install:
set(CMAKE_MODULE_PATH ${JEVOIS_CONFIG})
include(jevois_config)
include(JeVois)

## Set project name, detects compiler (which has been set by our helper module). Then set some complation flags:
project(__PROJECT__ CXX C ASM)
jevois_project_set_flags()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(${JEVOIS_HOST_INCLUDE})

## Compile with -O1 optimizations by default. We use -O4 in the core JeVois software, which compiles much slower. Once
## your module works well, you can switch this to -O4 and recompile:
add_definitions("-O1")

## Setup our module:
add_library(__MODULE__ SHARED __MODULE__.C)
set_target_properties(__MODULE__ PROPERTIES PREFIX "") # no lib prefix in the .so name

## Add a dependency and command to build modinfo.html:
add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/modinfo.html"
  COMMAND jevoispro-modinfo __MODULE__.C
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/__MODULE__.C
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(modinfo DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/modinfo.html" "${CMAKE_CURRENT_SOURCE_DIR}/__MODULE__.C")
add_dependencies(__MODULE__ modinfo)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/modinfo.html" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}" COMPONENT bin)

## Add any link libraries. Here, we link against libjevoispro and libjevoisprobase (and dependents):
target_link_libraries(__MODULE__ jevoispro jevoisprobase)

## Add includes pulled from jevois and jevoisbase:
include_directories(/jevoispro/include)
file(GLOB JVINCLUDES "/jevoispro/include/*" "/jevoispro/include/*/include")
FOREACH(child ${JVINCLUDES})
  IF (IS_DIRECTORY "${child}")
    include_directories("${child}")
  ENDIF()
ENDFOREACH()

## A couple more special cases of includes:
include_directories(/usr/include/eigen3)

## Link against all opencv libraries (get full list from jevoisbase/CMakeLists.txt):
target_link_libraries(__MODULE__ ${JEVOIS_OPENCV_LIBS} opencv_alphamat opencv_aruco opencv_bgsegm opencv_bioinspired
  opencv_calib3d opencv_ccalib opencv_core opencv_datasets opencv_dnn_objdetect opencv_dnn opencv_dnn_superres
  opencv_dpm opencv_face opencv_features2d opencv_flann opencv_fuzzy opencv_gapi opencv_hfs opencv_imgcodecs
  opencv_img_hash opencv_imgproc opencv_intensity_transform opencv_line_descriptor opencv_mcc opencv_ml opencv_objdetect
  opencv_optflow opencv_phase_unwrapping opencv_photo opencv_plot opencv_quality opencv_rapid opencv_reg opencv_saliency
  opencv_shape opencv_stereo opencv_stitching opencv_structured_light opencv_superres opencv_surface_matching
  opencv_text opencv_tracking opencv_videoio opencv_video opencv_videostab opencv_wechat_qrcode opencv_xfeatures2d
  opencv_ximgproc opencv_xobjdetect opencv_xphoto)

# Custom install directive: first install the .so and other files in the current dir
install(TARGETS __MODULE__ LIBRARY DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}" COMPONENT bin)

# Compiling and then running C++ modules on a live camera poses some shared library caching issues. 1) overwriting the
# module's .so file while it is running could cause corruption; 2) if we dlopen() and load a module, then dlclose() it,
# then update the .so file on disk, then dlopen() and load it again, the old version is still running (note that this is
# not the case with simple C code, but since our modules are C++ with also static data, etc that may be the
# reason). Using a new version number for the updated .so file solves the problem. The number has to be totally new
# (never loaded before during that run of jevoispro-daemon, trying to just alternate between .so and .so.1 still does
# not work). Thus, we here use the following approach:
#
# - on make install, we produce MyMod.so as usual. MyMod.so is always the latest compiled version.
# - we also check for any MyMod.so.x
# - we then copy MyMod.so to MyMod.so.(x+1)
# - jevois::VideoMapping::sopath() now returns MyMod.so.x with the highest x found in the module's directory
# - so, when compiling MyMod, MyMod.so.x will be currently running and MyMod.so.(x+1) will be loaded when we
#   restart the module.
# - in jevois::Engine::setFormatInternal(), as we load MyMod.so.x, we also instruct jevois::VideoMapping::sopath()
#   to delete all MyMod.so.y for any y<x
# Here, compute the next .so.x version and install to that file too:
jevois_compute_next_so_version(__MODULE__ NEWSOVER)
install(PROGRAMS __MODULE__.so DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/"
  RENAME "__MODULE__.so.${NEWSOVER}" COMPONENT localonly)

# Install (almost) everything in this directory, so it will get into our deb package:
INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/"
  COMPONENT bin
  PATTERN "*~" EXCLUDE
  PATTERN "*.so.*" EXCLUDE
  PATTERN "build" EXCLUDE
  PATTERN "postinst" EXCLUDE
  PATTERN "prerm" EXCLUDE)

########################################################################################################################
# Debian packaging:

# To list the files created in a package, run: dpkg -c <package.deb>
set(CPACK_PACKAGE_DESCRIPTION "JeVois Module __MODULE__")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "__MODULE__: __SYNOPSIS__") # cpack error if empty...
set(CPACK_PACKAGE_CONTACT "__AUTHOR__ <__EMAIL__>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "universe")
set(CPACK_PACKAGE_VENDOR "__VENDOR__")
set(CPACK_PACKAGE_VERSION_MAJOR "${MOD_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MOD_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MOD_VERSION_PATCH}")
set(JEVOIS_PACKAGE_RELEASE "1") # packager revision number
set(JEVOIS_DEPEND "${JEVOIS}-platform (>=${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}.${JEVOIS_VERSION_PATCH})")

# Pack only the 'bin' install component, which will skip module.so.x:
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL bin)
set(CPACK_DEB_COMPONENTS_ALL bin)

# Add any other dependencies below after JEVOIS_DEPEND for the package, separated by commas:
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${JEVOIS_DEPEND}")

# Add the videomapping on install of the deb, and ldconfig:
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/postinst" "jevoispro-add-videomapping __VIDEOMAPPING__\nldconfig")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/prerm" " ")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst;${CMAKE_CURRENT_SOURCE_DIR}/prerm;")

# Use helper from JeVois.cmake for all other settings:
if (JEVOIS_PLATFORM)
  set(JEVOISPRO_PLATFORM_DEB ON)
endif()
jevois_setup_cpack(__PROJECT__)
set(CPACK_PACKAGE_NAME "__PROJECT__-platform")
