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
#------------------------------------------------------------------------------
# build common library
#------------------------------------------------------------------------------
cet_make_library(LIBRARY_NAME frontends::utils
                 SOURCE
                 db_runinfo.cc
                 mu2e_sc.cc
                 get_xmlrpc_url.cc utils.cc
                 get_host_name.cc
                 MidasInterface.cc
                 OdbInterface.cc
                 LIBRARIES PUBLIC 
                 midas
                 tracelib
                 MF_MessageLogger
                 xmlrpc++ xmlrpc_client xmlrpc 
                 xmlrpc_util xmlrpc_util++ 
                 #                 ${PostgreSQL_LIBRARIES}
                 pq
                 pthread rt util
)
include(ArtDictionary)

art_dictionary(
  DICTIONARY_LIBRARIES
  frontends::utils
  midas
)

get_target_property(DictCompileOptions utils_dict COMPILE_OPTIONS)

if(DictCompileOptions)
	string(REPLACE "-fsanitize=thread"  "" DictCompileOptions "${DictCompileOptions}")
	string(REPLACE "-fsanitize=address" "" DictCompileOptions "${DictCompileOptions}")
	string(REPLACE "-ltsan"             "" DictCompileOptions "${DictCompileOptions}")
	string(REPLACE "-lasan"             "" DictCompileOptions "${DictCompileOptions}")

	set_target_properties(utils_dict PROPERTIES COMPILE_OPTIONS "${DictCompileOptions}")
endif()
#------------------------------------------------------------------------------
# test executable
#------------------------------------------------------------------------------
cet_make_exec(NAME      register_transitions
              SOURCE    register_transitions.cc 
              LIBRARIES frontends::utils
              #              ${PostgreSQL_LIBRARIES}
              pq
              util stdc++
)                  

cet_make_exec(NAME        db_runinfo_main
              SOURCE      db_runinfo_main.cc
              LIBRARIES   frontends::utils
              #                         ${PostgreSQL_LIBRARIES} util stdc++
              pq util stdc++
)                  

cet_make_exec(NAME      get_next_run_number
              SOURCE    get_next_run_number.cc 
              LIBRARIES frontends::utils
#              ${PostgreSQL_LIBRARIES}
              pq
              util stdc++
)                  

install(FILES runinfodb.py      DESTINATION python/frontends/utils)
install(FILES       MidasInterface.hh
                    OdbInterface.hh
                    utils.hh
        DESTINATION include/frontends/utils)
  
# install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
