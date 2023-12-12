# -----------------------------------------------------------------------------
# 
#------------------------------------------------------------------------------

include_directories($ENV{MIDASSYS}/include)
link_directories($ENV{MIDASSYS}/lib)

# cet_make_library(LIBRARY_NAME frontends_trk_dcs
#                  SOURCE       db_runinfo.cc 
#                  # dumpvalue.cc 
#                  # my_xmlrpc.cc  cmdline_parser.cc string_parser.cc 
#                  # getoptx.cc stripcaseeq.cc
#                  LIBRARIES PUBLIC 
#                  ${PostgreSQL_LIBRARIES} 
#                  xmlrpc++ xmlrpc_client xmlrpc 
#                  xmlrpc_util xmlrpc_util++ 
#                  pthread rt util
# )

cet_make_exec(NAME      trk_dcs_frontend 
              SOURCE    trk_dcs_frontend.cc 
              LIBRARIES 
              frontends_trk_dcs
              midas mfe 
)                  

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()

set(BUILD_ROOT_INTERFACE   $ENV{BUILD_ROOT_INTERFACE}  )

if(${BUILD_ROOT_INTERFACE})
  message(-- frontends/trk_dcs: BUILD_ROOT_INTERFACE=${BUILD_ROOT_INTERFACE})
  add_subdirectory(root)
endif()
