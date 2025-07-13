# ----------------------------------------------------------------------
# Build this project's executables:

include_directories($ENV{MRB_SOURCE})
include_directories(${MIDAS_INSTALL_DIR} ${MIDAS_INSTALL_DIR}/include)
include_directories($ENV{XMLRPC_C_INCLUDE_DIR})
include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories($ENV{TRACE_INC})
include_directories($ENV{MESSAGEFACILITY_INC})
include_directories($ENV{CETLIB_EXCEPT_INC})

link_directories($ENV{TRACE_LIB})
link_directories($ENV{MESSAGEFACILITY_LIB})
link_directories(${MIDAS_INSTALL_DIR}/lib)
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
              LIBRARIES frontends_utils xmlrpc_client xmlrpc xmlrpc_util 
                        # xmlrpc_client++ xmlrpc++ xmlrpc_util++ 
                        pthread rt util
)                  

install(PROGRAMS tfm_fe.py DESTINATION bin)

# install(FILES
#         m_frontend.py
#         m_client.py
#         DESTINATION python/frontends/tfm_frontend )

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()

# set(BUILD_ROOT_INTERFACE   $ENV{BUILD_ROOT_INTERFACE}  )
# 
# if(${BUILD_ROOT_INTERFACE})
#   message(-- frontends/tfm: BUILD_ROOT_INTERFACE=${BUILD_ROOT_INTERFACE})
#   add_subdirectory(root)
# endif()
