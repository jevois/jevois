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

cmake_minimum_required(VERSION 3.1)

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

## Patch up a few things in order to compile natively on arm as opposed to cross-compiling. Note that here we use the
# host settings for most things, except:
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
  set(JEVOIS_ARCH_FLAGS ${JEVOIS_PLATFORM_ARCH_FLAGS})
  set(JEVOIS_CFLAGS ${JEVOIS_PLATFORM_CFLAGS})
endif ()

## Set project name, detects compiler (which has been set by our helper module). Then set some complation flags:
project(__PROJECT__ CXX C ASM)
jevois_project_set_flags()
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

## Setup our module:
add_library(__MODULE__ SHARED __MODULE__.C)
set_target_properties(__MODULE__ PROPERTIES PREFIX "") # no lib prefix in the .so name

# could add modinfo logic here..............

## Add any link libraries. Here, we link against libjevoispro and libjevoisprobase which will pull opencv, etc
target_link_libraries(__MODULE__ jevoispro jevoisprobase)

# Custom install directive:
install(TARGETS __MODULE__ LIBRARY DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/")


########################################################################################################################
# Debian packaging:

# To list the files created in a package, run: dpkg -c <package.deb>
set(CPACK_PACKAGE_DESCRIPTION "JeVois Module __MODULE__")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "__SYNOPSIS__")
set(CPACK_PACKAGE_CONTACT "__AUTHOR__ <__EMAIL__>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_DEBIAN_PACKAGE_SECTION "universe")
set(CPACK_PACKAGE_VENDOR "__VENDOR__")
set(CPACK_PACKAGE_VERSION_MAJOR "${MOD_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MOD_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MOD_VERSION_PATCH}")
set(JEVOIS_PACKAGE_RELEASE "1") # packager revision number

if (JEVOIS_PLATFORM)
  set(JEVOIS_DEPEND "${JEVOIS}-platform (>=${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}.${JEVOIS_VERSION_PATCH})")
else (JEVOIS_PLATFORM)
  set(JEVOIS_DEPEND "${JEVOIS}-host (>=${JEVOIS_VERSION_MAJOR}.${JEVOIS_VERSION_MINOR}.${JEVOIS_VERSION_PATCH})")
endif (JEVOIS_PLATFORM)

# Add any other dependencies here for the package, separated by commas:
set(CPACK_DEBIAN_PACKAGE_DEPENDS "${JEVOIS_DEPEND}")

# Add the videomapping on install of the deb:
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/postinst" "jevois-add-videomapping __VIDEOMAPPING__")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/prerm" " ")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst;${CMAKE_CURRENT_SOURCE_DIR}/prerm;")

# Use helper from JeVois.cmake for all other settings:
jevois_setup_cpack(__PROJECT__)
