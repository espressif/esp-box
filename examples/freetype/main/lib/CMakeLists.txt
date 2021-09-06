# CMakeLists.txt
#
# Copyright (C) 2013-2021 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# Written originally by John Cary <cary@txcorp.com>
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
#
# FreeType explicitly marks the API to be exported and relies on the compiler
# to hide all other symbols. CMake supports a C_VISBILITY_PRESET property
# starting with 2.8.12.
cmake_minimum_required(VERSION 2.8.12)

if (NOT CMAKE_VERSION VERSION_LESS 3.3)
  # Allow symbol visibility settings also on static libraries. CMake < 3.3
  # only sets the property on a shared library build.
  cmake_policy(SET CMP0063 NEW)

  # Support new IN_LIST if() operator.
  cmake_policy(SET CMP0057 NEW)
endif ()

include(CheckIncludeFile)

project(freetype C)

set(VERSION_MAJOR "2")
set(VERSION_MINOR "11")
set(VERSION_PATCH "0")

# Generate LIBRARY_VERSION and LIBRARY_SOVERSION.
set(LIBTOOL_REGEX "version_info='([0-9]+):([0-9]+):([0-9]+)'")
file(STRINGS "${PROJECT_SOURCE_DIR}/freetype/builds/unix/configure.raw"
  VERSION_INFO
  REGEX ${LIBTOOL_REGEX})
string(REGEX REPLACE
  ${LIBTOOL_REGEX} "\\1"
  LIBTOOL_CURRENT "${VERSION_INFO}")
string(REGEX REPLACE
  ${LIBTOOL_REGEX} "\\2"
  LIBTOOL_REVISION "${VERSION_INFO}")
string(REGEX REPLACE
  ${LIBTOOL_REGEX} "\\3"
  LIBTOOL_AGE "${VERSION_INFO}")

# This is what libtool does internally on Unix platforms.
math(EXPR LIBRARY_SOVERSION "${LIBTOOL_CURRENT} - ${LIBTOOL_AGE}")
set(LIBRARY_VERSION "${LIBRARY_SOVERSION}.${LIBTOOL_AGE}.${LIBTOOL_REVISION}")

# Disallow in-source builds
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}/freetype")
  message(FATAL_ERROR
    "In-source builds are not permitted!  Make a separate folder for"
    " building, e.g.,\n"
    "  cmake -E make_directory build\n"
    "  cmake -E chdir build cmake ..\n"
    "Before that, remove the files created by this failed run with\n"
    "  cmake -E remove CMakeCache.txt\n"
    "  cmake -E remove_directory CMakeFiles")
endif ()


# Add local cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/freetype/builds/cmake)


# Create the configuration file
if (UNIX)
  check_include_file("unistd.h" HAVE_UNISTD_H)
  check_include_file("fcntl.h" HAVE_FCNTL_H)

  file(READ "${PROJECT_SOURCE_DIR}/freetype/builds/unix/ftconfig.h.in"
    FTCONFIG_H)
  if (HAVE_UNISTD_H)
    string(REGEX REPLACE
      "#undef +(HAVE_UNISTD_H)" "#define \\1 1"
      FTCONFIG_H "${FTCONFIG_H}")
  endif ()
  if (HAVE_FCNTL_H)
    string(REGEX REPLACE
      "#undef +(HAVE_FCNTL_H)" "#define \\1 1"
      FTCONFIG_H "${FTCONFIG_H}")
  endif ()
else ()
  file(READ "${PROJECT_SOURCE_DIR}/freetype/include/freetype/config/ftconfig.h"
    FTCONFIG_H)
endif ()

set(FTCONFIG_H_NAME "${PROJECT_SOURCE_DIR}/freetype/include/freetype/config/ftconfig.h")
if (EXISTS "${FTCONFIG_H_NAME}")
  file(READ "${FTCONFIG_H_NAME}" ORIGINAL_FTCONFIG_H)
else ()
  set(ORIGINAL_FTCONFIG_H "")
endif ()
if (NOT (ORIGINAL_FTCONFIG_H STREQUAL FTCONFIG_H))
  file(WRITE "${FTCONFIG_H_NAME}" "${FTCONFIG_H}")
endif ()


# Create the options file
file(READ "${PROJECT_SOURCE_DIR}/freetype/include/freetype/config/ftoption.h"
  FTOPTION_H)

set(FTOPTION_H_NAME "${PROJECT_BINARY_DIR}/include/freetype/config/ftoption.h")
if (EXISTS "${FTOPTION_H_NAME}")
  file(READ "${FTOPTION_H_NAME}" ORIGINAL_FTOPTION_H)
else ()
  set(ORIGINAL_FTOPTION_H "")
endif ()
if (NOT (ORIGINAL_FTOPTION_H STREQUAL FTOPTION_H))
  file(WRITE "${FTOPTION_H_NAME}" "${FTOPTION_H}")
endif ()

file(GLOB PUBLIC_HEADERS "include/ft2build.h" "include/freetype/*.h")
file(GLOB PUBLIC_CONFIG_HEADERS "include/freetype/config/*.h")
file(GLOB PRIVATE_HEADERS "include/freetype/internal/*.h")

set(BASE_SRCS
  freetype/src/autofit/autofit.c
  freetype/src/base/ftbase.c
  freetype/src/base/ftbbox.c
  freetype/src/base/ftbdf.c
  freetype/src/base/ftbitmap.c
  freetype/src/base/ftcid.c
  freetype/src/base/ftfstype.c
  freetype/src/base/ftgasp.c
  freetype/src/base/ftglyph.c
  freetype/src/base/ftgxval.c
  freetype/src/base/ftinit.c
  freetype/src/base/ftmm.c
  freetype/src/base/ftotval.c
  freetype/src/base/ftpatent.c
  freetype/src/base/ftpfr.c
  freetype/src/base/ftstroke.c
  freetype/src/base/ftsynth.c
  freetype/src/base/fttype1.c
  freetype/src/base/ftwinfnt.c
  freetype/src/bdf/bdf.c
  freetype/src/bzip2/ftbzip2.c
  freetype/src/cache/ftcache.c
  freetype/src/cff/cff.c
  freetype/src/cid/type1cid.c
  freetype/src/gzip/ftgzip.c
  freetype/src/lzw/ftlzw.c
  freetype/src/pcf/pcf.c
  freetype/src/pfr/pfr.c
  freetype/src/psaux/psaux.c
  freetype/src/pshinter/pshinter.c
  freetype/src/psnames/psnames.c
  freetype/src/raster/raster.c
  freetype/src/sdf/sdf.c
  freetype/src/sfnt/sfnt.c
  freetype/src/smooth/smooth.c
  freetype/src/truetype/truetype.c
  freetype/src/type1/type1.c
  freetype/src/type42/type42.c
  freetype/src/winfonts/winfnt.c
)

if (UNIX)
  list(APPEND BASE_SRCS "freetype/builds/unix/ftsystem.c")
elseif (WIN32)
  list(APPEND BASE_SRCS "freetype/builds/windows/ftsystem.c")
else ()
  list(APPEND BASE_SRCS "freetype/src/base/ftsystem.c")
endif ()

if (WIN32)
  enable_language(RC)
  list(APPEND BASE_SRCS freetype/builds/windows/ftdebug.c
                        freetype/src/base/ftver.rc)
elseif (WINCE)
  list(APPEND BASE_SRCS freetype/builds/wince/ftdebug.c)
else ()
  list(APPEND BASE_SRCS freetype/src/base/ftdebug.c)
endif ()

if (NOT DISABLE_FORCE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif ()


add_library(freetype
  ${PUBLIC_HEADERS}
  ${PUBLIC_CONFIG_HEADERS}
  ${PRIVATE_HEADERS}
  ${BASE_SRCS}
)

set_target_properties(
  freetype PROPERTIES
    C_VISIBILITY_PRESET hidden)

target_compile_definitions(
  freetype PRIVATE FT2_BUILD_LIBRARY)

if (WIN32)
  target_compile_definitions(
    freetype PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
  if (BUILD_SHARED_LIBS)
    target_compile_definitions(
      freetype PRIVATE DLL_EXPORT)
  endif ()
endif ()

if (BUILD_SHARED_LIBS)
  set_target_properties(freetype PROPERTIES
    VERSION ${LIBRARY_VERSION}
    SOVERSION ${LIBRARY_SOVERSION})
endif ()

# Pick up ftconfig.h and ftoption.h generated above, first.
target_include_directories(
  freetype
    PUBLIC
      $<INSTALL_INTERFACE:include/freetype2>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/freetype/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/freetype/include>
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}/include
      ${CMAKE_CURRENT_SOURCE_DIR}/include
      
      # Make <ftconfig.h> available for builds/unix/ftsystem.c.
      ${CMAKE_CURRENT_BINARY_DIR}/include/freetype/config
)

set(PKG_CONFIG_REQUIRED_PRIVATE "")
set(PKG_CONFIG_LIBS_PRIVATE "")

# Installation
include(GNUInstallDirs)

if (NOT SKIP_INSTALL_HEADERS AND NOT SKIP_INSTALL_ALL)
  install(
    # Note the trailing slash in the argument to `DIRECTORY'!
    DIRECTORY ${PROJECT_SOURCE_DIR}/freetype/include/
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/freetype2
      COMPONENT headers
      PATTERN "internal" EXCLUDE
      PATTERN "ftconfig.h" EXCLUDE
      PATTERN "ftoption.h" EXCLUDE)
  install(
    FILES ${PROJECT_BINARY_DIR}/include/freetype/config/ftconfig.h
          ${PROJECT_BINARY_DIR}/include/freetype/config/ftoption.h
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/freetype2/freetype/config
      COMPONENT headers)
endif ()

if (NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
  # Generate the pkg-config file
  file(READ "${PROJECT_SOURCE_DIR}/freetype/builds/unix/freetype2.in" FREETYPE2_PC_IN)

  string(REPLACE ";" ", " PKG_CONFIG_REQUIRED_PRIVATE "${PKG_CONFIG_REQUIRED_PRIVATE}")

  string(REPLACE "%prefix%" ${CMAKE_INSTALL_PREFIX}
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})
  string(REPLACE "%exec_prefix%" "\${prefix}"
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})
  string(REPLACE "%libdir%" "\${prefix}/${CMAKE_INSTALL_LIBDIR}"
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})
  string(REPLACE "%includedir%" "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}"
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})
  string(REPLACE "%ft_version%" "${LIBTOOL_CURRENT}.${LIBTOOL_REVISION}.${LIBTOOL_AGE}"
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})
  string(REPLACE "%REQUIRES_PRIVATE%" "${PKG_CONFIG_REQUIRED_PRIVATE}"
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})
  string(REPLACE "%LIBS_PRIVATE%" "${PKG_CONFIG_LIBS_PRIVATE}"
          FREETYPE2_PC_IN ${FREETYPE2_PC_IN})

  set(FREETYPE2_PC_IN_NAME "${PROJECT_BINARY_DIR}/freetype2.pc")
  if (EXISTS "${FREETYPE2_PC_IN_NAME}")
    file(READ "${FREETYPE2_PC_IN_NAME}" ORIGINAL_FREETYPE2_PC_IN)
  else ()
    set(ORIGINAL_FREETYPE2_PC_IN "")
  endif ()
  if (NOT (ORIGINAL_FREETYPE2_PC_IN STREQUAL FREETYPE2_PC_IN))
    file(WRITE "${FREETYPE2_PC_IN_NAME}" ${FREETYPE2_PC_IN})
  endif ()

  install(
    FILES ${PROJECT_BINARY_DIR}/freetype2.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    COMPONENT pkgconfig)

  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/freetype-config-version.cmake
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    COMPATIBILITY SameMajorVersion)

  install(
    TARGETS freetype
      EXPORT freetype-targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      FRAMEWORK DESTINATION Library/Frameworks
      COMPONENT libraries)
  install(
    EXPORT freetype-targets
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/freetype
      FILE freetype-config.cmake
      COMPONENT headers)
  install(
    FILES ${PROJECT_BINARY_DIR}/freetype-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/freetype
    COMPONENT headers)
endif ()


# Packaging
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The FreeType font rendering library.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/freetype/README")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/freetype/LICENSE.TXT")

set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

if (WIN32)
  set(CPACK_GENERATOR ZIP)
else ()
  set(CPACK_GENERATOR TGZ)
endif ()

set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C/C++ Headers")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION
  "Library used to build programs which use FreeType")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION
  "C/C++ header files for use with FreeType")
set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)
set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
set(CPACK_COMPONENT_HEADERS_GROUP "Development")

include(CPack)
