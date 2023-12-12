# ----------------------------------------------------------------------
# Build this project's executables:

include_directories($ENV{TRACE_INC})
include_directories($ENV{MIDASSYS}/include)
include_directories(${PostgreSQL_INCLUDE_DIRS})

link_directories($ENV{MIDASSYS}/lib)

cet_make_exec(NAME      feslow 
              SOURCE    feslow.cc
              LIBRARIES mfe midas 

              artdaq_plugin_support::commanderMaker
              artdaq::Application                       # needed for messagefacility
              artdaq::DAQrate
              artdaq::DAQdata
              artdaq::BuildInfo
              artdaq_core::artdaq-core_Utilities
              artdaq_core::artdaq-core_Plugins

              stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()

set(BUILD_ROOT_INTERFACE   $ENV{BUILD_ROOT_INTERFACE}  )

if(${BUILD_ROOT_INTERFACE})
  message(-- frontends/tfm: BUILD_ROOT_INTERFACE=${BUILD_ROOT_INTERFACE})
  add_subdirectory(root)
endif()
