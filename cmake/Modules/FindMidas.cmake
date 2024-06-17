# Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.
# All rights reserved.
#
# For the licensing terms see $ROOTSYS/LICENSE.
# For the list of contributors see $ROOTSYS/README/CREDITS.

#.rst:
# FindMidas
# -------
#
# Find the Midas library header and define variables.
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines :prop_tgt:`IMPORTED` target ``MIDAS::MIDAS``,
# if Midas has been found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ::
#
#   MIDAS_FOUND          - True if Midas is found.
#   MIDAS_INCLUDE_DIRS   - Where to find midas/midasMath.h
#
# ::
#
#   MIDAS_VERSION        - The version of Midas found (x.y.z)
#   MIDAS_VERSION_MAJOR  - The major version of Midas
#   MIDAS_VERSION_MINOR  - The minor version of Midas
#   MIDAS_VERSION_PATCH  - The patch version of Midas
#

if(NOT MIDAS_INCLUDE_DIR)
  find_path(MIDAS_INCLUDE_DIR NAME midas.h            PATH_SUFFIXES include)
endif()

if(NOT MIDAS_LIBRARY)
  find_library(MIDAS_LIBRARY NAMES midas)
endif()

mark_as_advanced(MIDAS_INCLUDE_DIR MIDAS_INCLUDE_DIRS)

if (MIDAS_INCLUDE_DIR)
  file(STRINGS "${MIDAS_INCLUDE_DIR}/midas.h" MIDAS_H REGEX "^#define MIDAS_VERSION_[A-Z]+[ ]+[0-9]+.*$")
  set(MIDAS_VERSION "0.4")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Midas FOUND_VAR MIDAS_FOUND
  REQUIRED_VARS MIDAS_INCLUDE_DIR MIDAS_LIBRARY VERSION_VAR MIDAS_VERSION)

if(MIDAS_FOUND)
  set(MIDAS_INCLUDE_DIRS ${MIDAS_INCLUDE_DIR})

  set(MIDAS_ROOT         ${MIDAS_ROOT})

  if(NOT MIDAS_LIBRARIES)
    set(MIDAS_LIBRARIES ${MIDAS_LIBRARY})
  endif()

  if(NOT TARGET MIDAS::MIDAS)
    add_library(MIDAS::MIDAS UNKNOWN IMPORTED)

    set_target_properties(MIDAS::MIDAS
      PROPERTIES
        IMPORTED_LOCATION "${MIDAS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MIDAS_INCLUDE_DIRS}"
    )
  endif()
endif()
