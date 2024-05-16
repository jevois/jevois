######################################################################################################################
#
# JeVois Smart Embedded Machine Vision Toolkit - Copyright (C) 2020 by Laurent Itti, the University of Southern
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

# Option to select JeVois-A33 or JeVois-Pro hardware device. When A33 is selected (default), JEVOIS_A33 is defined and
# files are installed in /jevois, /var/lib/jevois-microsd, etc. When Pro is selected, JEVOIS_PRO is defined and
# files are installed in /jevoispro, /var/lib/jevoispro-microsd, etc.
#
# For CMake, variable JEVOIS is set to "jevois" or "jevoispro" and can be used to prefix library names, directories, etc

set(JEVOIS_HARDWARE A33 CACHE STRING "JeVois hardware platform type (A33 or PRO)")
set_property(CACHE JEVOIS_HARDWARE PROPERTY STRINGS A33 PRO)
message(STATUS "JEVOIS_HARDWARE: ${JEVOIS_HARDWARE}")

####################################################################################################
# Helper to transform a list of libraries into versioned linker flags, used when a package contains
# somelib.so.4.9.0 but is missing the somelib.so link. Included here because JeVoisPro.cmake needs it.
# First arg is raw name of a list, second is version value, third is name of destination variable.
# Keep it sync with the definition in JeVois.cmake
macro(jevois_versioned_libs libs ver retvar)
  list(TRANSFORM ${libs} APPEND ${ver})
  list(TRANSFORM ${libs} PREPEND "-l:lib")
  string(REPLACE ";" " " ${retvar} "${${libs}}")
endmacro()

########################################################################################################################
if (JEVOIS_HARDWARE STREQUAL "A33")
  set(JEVOIS_A33 ON)
  set(JEVOIS_PRO OFF)
  set(JEVOIS "jevois")
  include(JeVoisA33)
########################################################################################################################
elseif (JEVOIS_HARDWARE STREQUAL "PRO")
  set(JEVOIS_A33 OFF)
  set(JEVOIS_PRO ON)
  set(JEVOIS "jevoispro")
  include(JeVoisPro)
########################################################################################################################
else()
  message(FATAL_ERROR "Invalid value ${JEVOIS_HARDWARE} for JEVOIS_HARDWARE: should be A33 or PRO")
endif()

