# ----------------------------------------------------------------------
# Build this project's executables:

include_directories($ENV{MIDASSYS} $ENV{MIDASSYS}/include)
# include_directories($ENV{MRB_BUILDDIR}/xmlrpc-c/include/xmlrpc-c)
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

cet_make_library(LIBRARY_NAME frontends_tfm 
                 SOURCE       db_runinfo.cc 
                 # dumpvalue.cc 
                 # my_xmlrpc.cc  cmdline_parser.cc string_parser.cc 
                 # getoptx.cc stripcaseeq.cc
                 LIBRARIES PUBLIC 
                 midas
                 ${PostgreSQL_LIBRARIES} 
                 tracelib
                 MF_MessageLogger
                 xmlrpc++ xmlrpc_client xmlrpc 
                 xmlrpc_util xmlrpc_util++ 
                 pthread rt util
)

# Nominally a test exec (no MPI) but we want this to be available to other packages.
cet_make_exec(NAME      client
              SOURCE    client.cc
              LIBRARIES frontends_tfm xmlrpc_client xmlrpc xmlrpc_util 
                        # xmlrpc_client++ xmlrpc++ xmlrpc_util++ 
                        pthread rt util
)                  

cet_make_exec(NAME      tfm_frontend 
              SOURCE    tfm_frontend.cc 
                        tfm_driver.cc
                        $ENV{MIDASSYS}/drivers/class/multi.cxx
                        $ENV{MIDASSYS}/drivers/bus/null.cxx
              LIBRARIES frontends_tfm midas mfe 
                        artdaq_plugin_support::commanderMaker
                        artdaq::Application                       # needed for messagefacility
                        artdaq::DAQrate
                        artdaq::DAQdata
                        artdaq::BuildInfo
                        artdaq_core::artdaq-core_Utilities
                        artdaq_core::artdaq-core_Plugins
)                  

cet_make_exec(NAME        db_runinfo_main
              SOURCE      db_runinfo_main.cc # db_runinfo.cc 
              LIBRARIES   frontends_tfm  
#                         ${PostgreSQL_LIBRARIES} util stdc++
)                  

cet_make_exec(NAME      get_next_run_number
              SOURCE    get_next_run_number.cc 
              LIBRARIES frontends_tfm  ${PostgreSQL_LIBRARIES} util stdc++
)                  
#------------------------------------------------------------------------------
# test executable
#------------------------------------------------------------------------------
cet_make_exec(NAME      register_transitions
              SOURCE    register_transitions.cc 
              LIBRARIES frontends_tfm  ${PostgreSQL_LIBRARIES} util stdc++
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
