# ======================================================================
#  otsdaq-mu2e-tracker main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/otsdaq-mu2e-tracker/ups/setup_for_development
#  buildtool
# ======================================================================
cmake_minimum_required (VERSION 3.19 FATAL_ERROR)
find_package(cetmodules 3.19.02 REQUIRED)

# project(otsdaq_mu2e_tracker VERSION 1.03.02)

project(frontends VERSION 1.01.01)

include(CetCMakeEnv)
cet_cmake_env()

cet_cmake_module_directories(cmake/Modules BINARY)

cet_set_compiler_flags(
        DIAGS VIGILANT
        # WERROR          
        NO_UNDEFINED
        EXTRA_FLAGS -pedantic -Wno-unused-parameter -Wdeprecated-declarations -pthread #-g -O0 -v # Equivalent to debug qualifier, for testing
)

#string(TOUPPER ${CMAKE_BUILD_TYPE} BTYPE_UC )
#if( ${BTYPE_UC} MATCHES "DEBUG" )
#  cet_add_compiler_flags(-fsanitize=address)
#endif()

message("---------- frontends: PRINT CMAKE VARIABLES " )

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
  message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
  
#cet_report_compiler_flags()
find_package(TRACE  3.17.04              REQUIRED EXPORT)
message(-- frontends: Trace_inc_dir=.${Trace_inc_dir}. )

# these are minimum required versions, not the actual product versions
# find_package(canvas_root_io  REQUIRED EXPORT QUIET)
# find_package(canvas          REQUIRED EXPORT QUIET)
find_package(otsdaq_mu2e_tracker 3.01.00 REQUIRED EXPORT)
find_package(mu2e_pcie_utils     3.01.00 REQUIRED EXPORT)

find_package(SQLite3                     REQUIRED EXPORT QUIET)
find_package(messagefacility             REQUIRED EXPORT QUIET)

find_package(Midas                       REQUIRED EXPORT)
find_package(PostgreSQL 13.2             REQUIRED EXPORT)
# find_package(canvas_root_io              REQUIRED EXPORT)

string (REGEX REPLACE "/include" ""  MIDAS_INSTALL_DIR ${MIDAS_INCLUDE_DIR})

message(-- frontends: MIDAS_ROOT=.${MIDAS_ROOT}. )
message(-- frontends: MIDAS_INSTALL_DIR=.${MIDAS_INSTALL_DIR}. )
message(-- frontends: MIDAS_INCLUDE_DIR=.${MIDAS_INCLUDE_DIR}. "    "  MIDAS_LIBRARY_DIR=.${MIDAS_LIBRARY_DIR})
message(-- frontends: current dir : .${CMAKE_SOURCE_DIR}.)
message(-- frontends: midas include dirs : .${MIDAS_INCLUDE_DIRS}.)

# find_package(artdaq_core     REQUIRED EXPORT QUIET)


#------------------------------------------------------------------------------
# XDAQ Extra setup
#------------------------------------------------------------------------------x
# include_directories($ENV{XDAQ_INC} $ENV{XDAQ_INC}/linux)
# link_directories   ($ENV{XDAQ_LIB})

# message(>>> OTSDAQ_MU2E_TRACKER: XDAQ_INC=$ENV{XDAQ_INC})
# 
#string(REGEX MATCHALL "[0-9][0-9]?[0-9]?" ART_VERSION_LIST "$ENV{ART_VERSION}")
#list(GET ART_VERSION_LIST 0 ART_MAJOR_VERSION)
#list(GET ART_VERSION_LIST 1 ART_MINOR_VERSION)
#list(GET ART_VERSION_LIST 2 ART_PATCH_VERSION)

#message("-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION}")
#set(ART_HEX_VERSION 0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})
#add_definitions(-DART_HEX_VERSION=0x${ART_MAJOR_VERSION}${ART_MINOR_VERSION}${ART_PATCH_VERSION})

include(BuildPlugins)
#------------------------------------------------------------------------------
# subdirectories
#------------------------------------------------------------------------------
add_subdirectory(utils)
add_subdirectory(cfo_frontend)
add_subdirectory(dtc_frontend)
add_subdirectory(roc_crv_frontend)
add_subdirectory(roc_trk_frontend)
add_subdirectory(tfm_frontend)
add_subdirectory(test)

# Useful scripts
#add_subdirectory(tools)

# Data

cet_cmake_config()
