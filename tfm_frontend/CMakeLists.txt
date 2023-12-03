# ----------------------------------------------------------------------
# Build this project's executables:

include_directories($ENV{MIDASSYS}/include)
include_directories($ENV{MRB_BUILDDIR}/xmlrpc-c/include/xmlrpc-c)
include_directories($ENV{XMLRPC_C_INCLUDE_DIR})

# include_directories($ENV{MRB_TOP}/xmlrpc-c/lib/util/include)

link_directories($ENV{MIDASSYS}/lib)
link_directories($ENV{XMLRPC_C_FQ_DIR}/lib)

# Nominally a test exec (no MPI) but we want this to be available to other packages.
cet_make_exec(NAME my_xmlrpc
                   SOURCE 
                   my_xmlrpc_main.cc my_xmlrpc.cc cmdline_parser.cc string_parser.cc 
                   getoptx.cc dumpvalue.cc stripcaseeq.cc
                   LIBRARIES 
                   #                   midas mfe 
                   xmlrpc++ xmlrpc_client xmlrpc xmlrpc_util xmlrpc_util++ 
                   pthread rt util stdc++
)                  

cet_make_exec(NAME tfm_frontend 
                   SOURCE 
                   tfm_frontend.cc my_xmlrpc.cc cmdline_parser.cc string_parser.cc 
                   getoptx.cc dumpvalue.cc stripcaseeq.cc
                   LIBRARIES 
                   midas 
                   # mfe 
                   xmlrpc++ xmlrpc_client xmlrpc xmlrpc_util xmlrpc_util++ 
                   pthread rt util stdc++

  artdaq_plugin_support::commanderMaker
  artdaq::Application # needed for messagefacility
  artdaq::DAQrate
  artdaq::DAQdata
  artdaq::BuildInfo
  artdaq_core::artdaq-core_Utilities
  artdaq_core::artdaq-core_Plugins

)                  

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
