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

message(STATUS "JeVois version ${JEVOIS_SOVERSION}")

# Platform choice. Silence this message when compiling natively on JeVois-Pro to not confuse people:
option(JEVOIS_PLATFORM "Cross-compile for hardware platform" OFF)
if (NOT EXISTS /sys/class/npu/info)
  message(STATUS "JEVOIS_PLATFORM: ${JEVOIS_PLATFORM}")
endif()

# Vendor name for modules, etc:
if (NOT DEFINED JEVOIS_VENDOR)
  if (DEFINED ENV{JEVOIS_VENDOR})
    set(JEVOIS_VENDOR $ENV{JEVOIS_VENDOR})
  else (DEFINED ENV{JEVOIS_VENDOR})
    set(JEVOIS_VENDOR "Unknown" CACHE STRING "Module vendor name")
  endif (DEFINED ENV{JEVOIS_VENDOR})
endif (NOT DEFINED JEVOIS_VENDOR)
message(STATUS "JEVOIS_VENDOR: ${JEVOIS_VENDOR}")

# Mount point for the microSD exported by JeVois:
set(JEVOIS_USER "$ENV{USER}" CACHE STRING "JeVois user name (before any sudo)")
set(JEVOIS_USBSERIAL_DEV "/dev/ttyACM0" CACHE STRING "JeVois serial-over-USB device")
string(TOUPPER "${JEVOIS}" JEVOIS_PART)
set(JEVOIS_MICROSD_MOUNTPOINT "/media/${JEVOIS_USER}/${JEVOIS_PART}" CACHE STRING "Mountpoint for JeVois microSD card")

if (NOT JEVOIS_PRO)
  message(STATUS "JeVois microSD card mount point: ${JEVOIS_MICROSD_MOUNTPOINT}")
  message(STATUS "JeVois serial-over-USB device: ${JEVOIS_USBSERIAL_DEV}")
endif()

# Settings for native host compilation or hardware platform compilation:
if (JEVOIS_PLATFORM)

  # On platform, install to jvpkg, staging area, or live microSD?
  option(JEVOIS_MODULES_TO_STAGING "Install modules to ${JEVOIS_PLATFORM_INSTALL_PREFIX} as opposed to jvpkg" OFF)
  option(JEVOIS_MODULES_TO_MICROSD "Install modules to ${JEVOIS_MICROSD_MOUNTPOINT} as opposed to jvpkg" OFF)
  option(JEVOIS_MODULES_TO_LIVE
    "Install modules to live JeVois camera at ${JEVOIS_MICROSD_MOUNTPOINT} as opposed to jvpkg" OFF)

  if (NOT JEVOIS_PRO)
    message(STATUS "JEVOIS_MODULES_TO_STAGING: ${JEVOIS_MODULES_TO_STAGING}")
    message(STATUS "JEVOIS_MODULES_TO_MICROSD: ${JEVOIS_MODULES_TO_MICROSD}")
    message(STATUS "JEVOIS_MODULES_TO_LIVE: ${JEVOIS_MODULES_TO_LIVE}")
  endif()
  
  # Flags for native compilation on platform from the JeVois-Pro GUI (as opposed to cross-compilation):
  if (JEVOIS_NATIVE)
    
    set(CMAKE_C_COMPILER ${JEVOIS_HOST_C_COMPILER})
    set(CMAKE_CXX_COMPILER ${JEVOIS_HOST_CXX_COMPILER})
    set(CMAKE_FC_COMPILER ${JEVOIS_HOST_FORTRAN_COMPILER})
    set(JEVOIS_INSTALL_PREFIX ${JEVOIS_HOST_INSTALL_PREFIX})
    set(JEVOIS_OPENCV_LIBS ${JEVOIS_PLATFORM_NATIVE_OPENCV_LIBS})
    set(JEVOIS_PYTHON_LIBS ${JEVOIS_PLATFORM_PYTHON_LIBS})
    set(JEVOIS_OPENGL_LIBS ${JEVOIS_PLATFORM_OPENGL_LIBS})
    set(JEVOIS_MODULES_ROOT ${JEVOIS_HOST_MODULES_ROOT})
    set(JEVOIS_ARCH_FLAGS ${JEVOIS_PLATFORM_ARCH_FLAGS})
    set(JEVOIS_CFLAGS ${JEVOIS_PLATFORM_NATIVE_CFLAGS})
    set(JEVOIS_CXX_STD ${JEVOIS_PLATFORM_CXX_STD})
    set(JEVOIS_PYTHON_MAJOR ${JEVOIS_PLATFORM_PYTHON_MAJOR})
    set(JEVOIS_PYTHON_MINOR ${JEVOIS_PLATFORM_PYTHON_MINOR})
    set(JEVOIS_PYTHON_M "${JEVOIS_PLATFORM_PYTHON_M}")
    set(JEVOIS_ARCH "arm64")

  else (JEVOIS_NATIVE)
    
    set(CMAKE_C_COMPILER ${JEVOIS_PLATFORM_C_COMPILER})
    set(CMAKE_CXX_COMPILER ${JEVOIS_PLATFORM_CXX_COMPILER})
    set(CMAKE_FC_COMPILER ${JEVOIS_PLATFORM_FORTRAN_COMPILER})
    set(JEVOIS_INSTALL_PREFIX ${JEVOIS_PLATFORM_INSTALL_PREFIX})
    set(JEVOIS_OPENCV_LIBS ${JEVOIS_PLATFORM_OPENCV_LIBS})
    set(JEVOIS_PYTHON_LIBS ${JEVOIS_PLATFORM_PYTHON_LIBS})
    set(JEVOIS_OPENGL_LIBS ${JEVOIS_PLATFORM_OPENGL_LIBS})
    set(JEVOIS_MODULES_ROOT ${JEVOIS_PLATFORM_MODULES_ROOT})
    set(JEVOIS_ARCH_FLAGS ${JEVOIS_PLATFORM_ARCH_FLAGS})
    set(JEVOIS_CFLAGS ${JEVOIS_PLATFORM_CFLAGS})
    set(JEVOIS_CXX_STD ${JEVOIS_PLATFORM_CXX_STD})
    set(JEVOIS_PYTHON_MAJOR ${JEVOIS_PLATFORM_PYTHON_MAJOR})
    set(JEVOIS_PYTHON_MINOR ${JEVOIS_PLATFORM_PYTHON_MINOR})
    set(JEVOIS_PYTHON_M "${JEVOIS_PLATFORM_PYTHON_M}")
    if (JEVOIS_PRO)
      
      option(JEVOISPRO_PLATFORM_DEB "Build .deb file for platform (aarch64) rather than host (amd64)." OFF)
      message(STATUS "JEVOISPRO_PLATFORM_DEB: ${JEVOISPRO_PLATFORM_DEB}")

      # Setup the target system type and root filesystem so we can find libs:
      message(STATUS "Cross-compiling for aarch64 with sysroot at ${JEVOIS_BUILD_BASE}")
      set(CMAKE_SYSTEM_NAME Linux)
      set(CMAKE_SYSTEM_PROCESSOR aarch64)
      set(GNU_MACHINE "aarch64-linux-gnu")
      set(CMAKE_SYSROOT ${JEVOIS_BUILD_BASE})
      set(CMAKE_FIND_ROOT_PATH ${JEVOIS_BUILD_BASE})
      set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
      set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
      set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

      # Install modules to /jevoispro in the deb file when using platform deb. Using a relative path here will ensure
      # that files get compiled and installed to /var/lib/jevoispro-build-pdeb/jevoispro but cpack will change that to
      # /jevoispro when packing the deb file:
      if (JEVOISPRO_PLATFORM_DEB)
        set(JEVOIS_INSTALL_PREFIX ${JEVOIS_PLATFORM_INSTALL_PREFIX_PDEB})
        set(JEVOIS_MODULES_ROOT "../${JEVOIS}")
      endif()
      
      # JEVOIS_ARCH is used to include/install the correct contributed libs (e.g., libtensorflowlite.so,
      # libonnxruntime.so, etc):
      set(JEVOIS_ARCH "arm64")
    else (JEVOIS_PRO)
      set(JEVOIS_ARCH "armhf")
    endif(JEVOIS_PRO)
    
  endif (JEVOIS_NATIVE)
  
else (JEVOIS_PLATFORM)
  
  set(JEVOIS_ARCH "amd64")

  set(CMAKE_C_COMPILER ${JEVOIS_HOST_C_COMPILER})
  set(CMAKE_CXX_COMPILER ${JEVOIS_HOST_CXX_COMPILER})
  set(CMAKE_FC_COMPILER ${JEVOIS_HOST_FORTRAN_COMPILER})
  set(JEVOIS_INSTALL_PREFIX ${JEVOIS_HOST_INSTALL_PREFIX})
  set(JEVOIS_OPENCV_LIBS ${JEVOIS_HOST_OPENCV_LIBS})
  set(JEVOIS_PYTHON_LIBS ${JEVOIS_HOST_PYTHON_LIBS})
  set(JEVOIS_OPENGL_LIBS ${JEVOIS_HOST_OPENGL_LIBS})
  set(JEVOIS_MODULES_ROOT ${JEVOIS_HOST_MODULES_ROOT})
  set(JEVOIS_ARCH_FLAGS ${JEVOIS_HOST_ARCH_FLAGS})
  set(JEVOIS_CFLAGS ${JEVOIS_HOST_CFLAGS})
  set(JEVOIS_CXX_STD ${JEVOIS_HOST_CXX_STD})
  set(JEVOIS_PYTHON_MAJOR ${JEVOIS_HOST_PYTHON_MAJOR})
  set(JEVOIS_PYTHON_MINOR ${JEVOIS_HOST_PYTHON_MINOR})
  set(JEVOIS_PYTHON_M "${JEVOIS_HOST_PYTHON_M}")
    
endif (JEVOIS_PLATFORM)

message(STATUS "Install prefix for executable programs: ${JEVOIS_INSTALL_PREFIX}")
message(STATUS "Install prefix for JeVois modules: ${JEVOIS_MODULES_ROOT}")

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
  set(CMAKE_C_FLAGS "-std=gnu99 ${JEVOIS_CFLAGS} -I${JEVOIS_INSTALL_PREFIX}/include \
      -include jevois/Config/Config-${JEVOIS}.H")
  set(CMAKE_CXX_FLAGS "-std=${JEVOIS_CXX_STD} ${JEVOIS_CFLAGS} -I${JEVOIS_INSTALL_PREFIX}/include \
      -include jevois/Config/Config-${JEVOIS}.H")
  set(CMAKE_INSTALL_PREFIX ${JEVOIS_INSTALL_PREFIX})
  link_directories(${JEVOIS_INSTALL_PREFIX}/lib) # to find libjevois
  message(STATUS "JeVois C++ standard used: ${JEVOIS_CXX_STD}")
  
  # add a dependency and command to create the jvpkg package:
  add_custom_target(jvpkg
    COMMAND jevois-jvpkg ../${JEVOIS_VENDOR}_${CMAKE_PROJECT_NAME}.jvpkg
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/jvpkg )
  
  # If installing to live microSD inside JeVois, mount it before make install:
  if (JEVOIS_MODULES_TO_LIVE)
    install(CODE "EXECUTE_PROCESS(COMMAND /usr/bin/jevois-usbsd start ${JEVOIS_MICROSD_MOUNTPOINT} ${JEVOIS_USBSERIAL_DEV})")
  endif (JEVOIS_MODULES_TO_LIVE)
    
  # If installing to microSD, add a check that it is here before make install:
  if (JEVOIS_MODULES_TO_MICROSD)
    install(CODE "EXECUTE_PROCESS(COMMAND /bin/ls \"${JEVOIS_MICROSD_MOUNTPOINT}/\" )")
  endif (JEVOIS_MODULES_TO_MICROSD)

  # Set variable JEVOIS_INSTALL_ROOT which may be used by the CMakeLists.txt of modules:
  # On platform, we install to jvpkg directory, staging, or live microsd; on host we always install to
  # /jevois[pro]:
  if (JEVOIS_PLATFORM)
    if (JEVOIS_MODULES_TO_MICROSD OR JEVOIS_MODULES_TO_LIVE) # if both specified, microsd/live precedes staging
      set(JEVOIS_INSTALL_ROOT "${JEVOIS_MICROSD_MOUNTPOINT}")
    else ()
      if (JEVOIS_MODULES_TO_STAGING)
        if (JEVOISPRO_PLATFORM_DEB)
          set(JEVOIS_INSTALL_ROOT ${JEVOIS_PLATFORM_MODULES_ROOT_PDEB})
        else ()
          set(JEVOIS_INSTALL_ROOT "${JEVOIS_PLATFORM_MODULES_ROOT}")
        endif ()
      else ()
	    set(JEVOIS_INSTALL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/jvpkg")
      endif ()
    endif ()
  else (JEVOIS_PLATFORM)
    set(JEVOIS_INSTALL_ROOT "${JEVOIS_MODULES_ROOT}")
  endif (JEVOIS_PLATFORM)

  message(STATUS "JeVois install root: ${JEVOIS_INSTALL_ROOT}")

  # Find includes and libs installed by JeVois (imgui, function2, tensorflow, etc):
  if (JEVOISPRO_PLATFORM_DEB)
    set(JEVOIS_INST_PATH "${JEVOIS_INSTALL_PREFIX}/${JEVOIS_MODULES_ROOT}")
  else ()
    set(JEVOIS_INST_PATH "${JEVOIS_MODULES_ROOT}")
  endif ()

  include_directories("${JEVOIS_INST_PATH}/include")
  subdirlist(SUBINCLUDES "${JEVOIS_INST_PATH}/include")
  list(TRANSFORM SUBINCLUDES PREPEND "${JEVOIS_INST_PATH}/include/")
  include_directories(${SUBINCLUDES})

  # Also tensorflow donwloaded tools:
  file(GLOB SUBINCLUDES "${JEVOIS_INST_PATH}/include/tensorflow/lite/tools/make/downloads/*/include")
  include_directories(${SUBINCLUDES})

  # Find libs installed by JeVois:
  link_directories("${JEVOIS_INST_PATH}/lib")
  #subdirlist(SUBLIBS "${JEVOIS_INST_PATH}/lib")
  #list(TRANSFORM SUBLIBS PREPEND "${JEVOIS_INST_PATH}/lib/")
  #link_directories("${SUBLIBS}")
endmacro()

####################################################################################################
# Helper to complete a project - should be called last in CMakeLists.txt:
macro(jevois_project_finalize)
  # If installing to live microSD inside JeVois, un-mount it after make install:
  if (JEVOIS_MODULES_TO_LIVE)
    install(CODE "EXECUTE_PROCESS(COMMAND /usr/bin/jevois-usbsd stop ${JEVOIS_MICROSD_MOUNTPOINT} ${JEVOIS_USBSERIAL_DEV})")
  endif (JEVOIS_MODULES_TO_LIVE)
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
      target_link_libraries(${JV_MODULE} ${JEVOIS})
      if (${deps})
	    add_dependencies(${JV_MODULE} ${deps})
      endif (${deps})
      
      # add a dependency and command to build modinfo.html:
      add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.html"
	    COMMAND ${JEVOIS}-modinfo ${JV_MODULE}.C
	    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.C
	    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE})
      
      add_custom_target(modinfo_${JV_MODULE}
	    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.C")
      add_dependencies(${JV_MODULE} modinfo_${JV_MODULE})
      
    endif (MODFILES)
    
    if (PYFILES)
      # Get this python module ready
      message(STATUS "Adding setup directives for Python module ${JV_MODULE} base ${basedir}")
      
      # Add a dependency and command to build modinfo.html:
      add_custom_command(OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.html"
	    COMMAND ${JEVOIS}-modinfo ${JV_MODULE}.py
	    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.py
	    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE})
      
      add_custom_target(modinfo_${JV_MODULE} ALL
	    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/modinfo.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/${basedir}/${JV_MODULE}/${JV_MODULE}.py")

    endif (PYFILES)
    
    set(DESTDIR "${JEVOIS_MODULES_ROOT}/modules/${JEVOIS_VENDOR}")
    
    # Install everything that is in that directory except for the source file:
    install(DIRECTORY ${basedir}/${JV_MODULE} DESTINATION "${DESTDIR}"
      PATTERN "*~" EXCLUDE
      PATTERN "__pycache__" EXCLUDE) # compiled python
    
    # Install the compiled module .so itself:
    if (MODFILES)
      install(TARGETS ${JV_MODULE} LIBRARY DESTINATION "${DESTDIR}/${JV_MODULE}")
    endif (MODFILES)
    
  endforeach()
endmacro()

####################################################################################################
# Setup a library - keep in sync with second version below
macro(jevois_setup_library basedir libname libversion)
  file(GLOB_RECURSE JV_LIB_SRC_FILES ${basedir}/*.[Cc] ${basedir}/*.cpp)
  add_library(${libname} SHARED ${JV_LIB_SRC_FILES})

  # Register jevois core library as a link dependency:
  target_link_libraries(${libname} ${JEVOIS})

  # Add version information, this will create symlinks as needed:
  # NOTE: now disabled since we install to /jevois[pro] which is vfat and does not support symlinks
  #if (NOT JEVOIS_PLATFORM)
  #  set_target_properties(${libname} PROPERTIES VERSION "${libversion}" SOVERSION ${libversion})
  #endif (NOT JEVOIS_PLATFORM)
  
  link_libraries(${libname})

  install(TARGETS ${libname} LIBRARY
    DESTINATION "${JEVOIS_MODULES_ROOT}/lib"
    COMPONENT libs)
    
endmacro()

####################################################################################################
# Setup a library - keep in sync with second version above
macro(jevois_setup_library2 srcfile libname libversion)
  add_library(${libname} SHARED ${srcfile})

  # Register jevois core library as a link dependency:
  target_link_libraries(${libname} ${JEVOIS})

  # Add version information, this will create symlinks as needed:
  # NOTE: now disabled since we install to /jevois[pro] which is vfat and does not support symlinks
  #if (NOT JEVOIS_PLATFORM)
  #  set_target_properties(${libname} PROPERTIES VERSION "${libversion}" SOVERSION ${libversion})
  #endif (NOT JEVOIS_PLATFORM)
  
  link_libraries(${libname})

  install(TARGETS ${libname} LIBRARY
    DESTINATION "${JEVOIS_MODULES_ROOT}/lib"
    COMPONENT libs)
    
endmacro()

####################################################################################################
# Setup packaging for debian using cpack
macro(jevois_setup_cpack packagename)
  # Create packages (Debian, RPM): in hbuild/ or pbuild/, just type 'sudo cpack' to create the package.
  # To list the files created in a package, run: dpkg -c <package.deb>
  if (JEVOIS_PLATFORM)
    set(CPACK_PACKAGE_NAME "${packagename}-platform")

    if (JEVOISPRO_PLATFORM_DEB)
      set(JEVOIS_CPACK_ARCH "arm64") # pack for aarch64 to install on platform
      set(CPACK_SET_DESTDIR OFF) # cpack installs using default install dir
    else ()
      set(JEVOIS_CPACK_ARCH "amd64") # pack for amd64 to install on host
      set(CPACK_SET_DESTDIR ON) # needed to avoid having cpack use default install dir
    endif ()

  else (JEVOIS_PLATFORM)
    set(CPACK_PACKAGE_NAME "${packagename}-host")
    execute_process(COMMAND dpkg --print-architecture
      OUTPUT_VARIABLE JEVOIS_CPACK_ARCH
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif (JEVOIS_PLATFORM)

  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${JEVOIS_CPACK_ARCH})
  set(CPACK_GENERATOR "DEB;")  # could be DEB;RPM;
  
  # Use a file name that includes debian version so we can publish the same package for different ubuntu versions:
  # Format is: <PackageName>_<VersionNumber>-<DebianRevisionNumber>_<DebianArchitecture>.deb
  # We cannot just do: set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")  because some packages here have arch 'all'
  execute_process(COMMAND lsb_release -rs
    OUTPUT_VARIABLE UBU_RELEASE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  
  set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${JEVOIS_PACKAGE_RELEASE}ubuntu${UBU_RELEASE}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_DEBIAN_PACKAGE_VERSION}_${JEVOIS_CPACK_ARCH}")
  
  SET(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}/*")
  
  include(CPack)
  
endmacro()

####################################################################################################
# Check that jevois-sdk version matches the version of dependent projects (jevois, jevoisbase, etc)
macro(jevois_check_sdk_version ver)
  if (JEVOIS_PRO)
    set(verfile "${JEVOIS_BUILD_BASE}/../${JEVOIS}-sdk-version.txt")
  else()
    set(verfile "/var/lib/${JEVOIS}-build/${JEVOIS}-sdk-version.txt")
  endif()
  if (NOT EXISTS "${verfile}")
    message(FATAL_ERROR "Cannot find ${verfile}. Install the ${JEVOIS} SDK first with: \
            sudo apt install ${JEVOIS}-sdk-dev -- ABORT")
  endif()
  
  file (STRINGS "${verfile}" SDK_VERSION)
  if (NOT (${SDK_VERSION} VERSION_EQUAL ${ver}))
    message("You are tring to compile jevois software ${ver} against mismatched ${JEVOIS} sdk ${SDK_VERSION}")
    message("This will likely fail. If using jevois sdk from deb and jevois/jevoisbase/etc from github, then try:")
    message("    git checkout ${SDK_VERSION}")
    message("inside your directory from github to roll it to the same version as ${JEVOIS} sdk.")
    message(FATAL_ERROR "Compilation aborted due to jevois sdk version mismatch.")
  endif()
endmacro()

####################################################################################################
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
#
# In this macro, we just list all MyMod.so.x files and return the largest x found + 1
macro(jevois_compute_next_so_version module version_returned)
  set(v 0)
  FILE(GLOB module_libs
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${module}.so.*") 

  foreach (modlib ${module_libs})
    get_filename_component(soext "${modlib}" LAST_EXT)
    string(SUBSTRING "${soext}" 1 -1 soext) # strip leading . from soext
    if (soext GREATER v)
      set(v "${soext}")
    endif ()
  endforeach ()

  MATH(EXPR ${version_returned} "${v}+1")
endmacro()

####################################################################################################
# Helper to transform a list of libraries into versioned linker flags, used when a package contains
# somelib.so.4.9.0 but is missing the somelib.so link.
# First arg is raw name of a list, second is version value, third is name of destination variable.
# Keep it sync with the definition in JeVoisHardware.cmake
macro(jevois_versioned_libs libs ver retvar)
  list(TRANSFORM ${libs} APPEND ${ver})
  list(TRANSFORM ${libs} PREPEND "-l:lib")
  string(REPLACE ";" " " ${retvar} "${${libs}}")
endmacro()
