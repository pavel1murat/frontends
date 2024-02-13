# ----------------------------------------------------------------------
# Build this project's executables:

include_directories($ENV{MRB_SOURCE})
include_directories($ENV{MIDASSYS} $ENV{MIDASSYS}/include)
include_directories($ENV{XMLRPC_C_INCLUDE_DIR})
include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories($ENV{TRACE_INC})
include_directories($ENV{MESSAGEFACILITY_INC})
include_directories($ENV{CETLIB_EXCEPT_INC})

link_directories($ENV{TRACE_LIB})
link_directories($ENV{MESSAGEFACILITY_LIB})
link_directories($ENV{MIDASSYS}/lib)
link_directories($ENV{XMLRPC_C_FQ_DIR}/lib)

cet_set_compiler_flags(
        DIAGS VIGILANT
        # WERROR          
        NO_UNDEFINED
        EXTRA_FLAGS -pedantic -Wno-unused-parameter -Wdeprecated-declarations 
        -Wmissing-field-initializers
        # -v
        -g -O0 # Equivalent to debug qualifier, for testing
        -pthread 
)

# Nominally a test exec (no MPI) but we want this to be available to other packages.
cet_make_exec(NAME      client
              SOURCE    client.cc
              LIBRARIES frontends xmlrpc_client xmlrpc xmlrpc_util 
                        # xmlrpc_client++ xmlrpc++ xmlrpc_util++ 
                        pthread rt util
)                  

cet_make_exec(NAME      tfm_mon_fe
              SOURCE    tfm_mon_fe.cc 
                        tfm_br_driver.cc
                        tfm_dr_driver.cc
                        tfm_disk_driver.cc
                        $ENV{MIDASSYS}/drivers/bus/null.cxx
              LIBRARIES frontends midas mfe 
                        artdaq_plugin_support::commanderMaker
                        artdaq::Application                       # needed for messagefacility
                        artdaq::DAQrate
                        artdaq::DAQdata
                        artdaq::BuildInfo
                        artdaq_core::artdaq-core_Utilities
                        artdaq_core::artdaq-core_Plugins
)                  

cet_make_exec(NAME      tfm_launch_fe 
              SOURCE    tfm_launch_fe.cc 
                        $ENV{MIDASSYS}/drivers/device/nulldev.cxx
                        $ENV{MIDASSYS}/drivers/bus/null.cxx
              LIBRARIES frontends midas mfe 
                        artdaq_plugin_support::commanderMaker
                        artdaq::Application                       # needed for messagefacility
                        artdaq::DAQrate
                        artdaq::DAQdata
                        artdaq::BuildInfo
                        artdaq_core::artdaq-core_Utilities
                        artdaq_core::artdaq-core_Plugins
)                  

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()

# set(BUILD_ROOT_INTERFACE   $ENV{BUILD_ROOT_INTERFACE}  )
# 
# if(${BUILD_ROOT_INTERFACE})
#   message(-- frontends/tfm: BUILD_ROOT_INTERFACE=${BUILD_ROOT_INTERFACE})
#   add_subdirectory(root)
# endif()
