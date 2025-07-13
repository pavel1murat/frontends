# ======================================================================
#  otsdaq-mu2e-tracker main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/otsdaq-mu2e-tracker/ups/setup_for_development
#  buildtool
# ======================================================================
cmake_minimum_required (VERSION 3.19 FATAL_ERROR)
find_package(cetmodules 3.19.02 REQUIRED)

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

message("---------- frontends: PRINT CMAKE VARIABLES " )

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
  message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
  
cet_report_compiler_flags()
find_package(TRACE  3.17.04              REQUIRED EXPORT)
message(-- frontends: Trace_inc_dir=.${Trace_inc_dir}. )

# these are minimum required versions, not the actual product versions
find_package(otsdaq-mu2e-tracker 3.01.00 REQUIRED EXPORT)
find_package(otsdaq-mu2e-crv 3.01.00 REQUIRED EXPORT)
find_package(mu2e-pcie-utils     3.01.00 REQUIRED EXPORT)

find_package(SQLite3                     REQUIRED EXPORT QUIET)
find_package(messagefacility             REQUIRED EXPORT QUIET)

find_package(Midas                       REQUIRED EXPORT)
find_package(PostgreSQL 13.2             REQUIRED EXPORT)

string (REGEX REPLACE "/include" ""  MIDAS_INSTALL_DIR ${MIDAS_INCLUDE_DIR})

message(-- frontends: MIDAS_ROOT=.${MIDAS_ROOT}. )
message(-- frontends: MIDAS_INSTALL_DIR=.${MIDAS_INSTALL_DIR}. )
message(-- frontends: MIDAS_INCLUDE_DIR=.${MIDAS_INCLUDE_DIR}. "    "  MIDAS_LIBRARY_DIR=.${MIDAS_LIBRARY_DIR})
message(-- frontends: current dir : .${CMAKE_SOURCE_DIR}.)
message(-- frontends: midas include dirs : .${MIDAS_INCLUDE_DIRS}.)

include(BuildPlugins)
#------------------------------------------------------------------------------
# subdirectories
#------------------------------------------------------------------------------
add_subdirectory(utils)
add_subdirectory(cfg_frontend)
add_subdirectory(cfo_frontend)
add_subdirectory(node_frontend)
add_subdirectory(tfm_frontend)
add_subdirectory(test)
add_subdirectory(brdr)
add_subdirectory(mepcis)
# Data

cet_cmake_config()
