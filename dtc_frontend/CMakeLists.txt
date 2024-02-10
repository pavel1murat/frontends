# ----------------------------------------------------------------------
# Build this project's executables:

find_package(messagefacility REQUIRED)
#------------------------------------------------------------------------------
# link directories 
#------------------------------------------------------------------------------
include_directories($ENV{TRACE_INC})
include_directories($ENV{MIDASSYS}/include)
include_directories($ENV{MIDASSYS})
include_directories(${PostgreSQL_INCLUDE_DIRS})
#------------------------------------------------------------------------------
# library directories 
#------------------------------------------------------------------------------
link_directories($ENV{TRACE_LIB})
link_directories($ENV{MIDASSYS}/lib)
link_directories($ENV{MESSAGEFACILITY_LIB})
link_directories($ENV{XMLRPC_C_FQ_DIR}/lib)

cet_set_compiler_flags(
        DIAGS VIGILANT
        # WERROR          
        NO_UNDEFINED
        EXTRA_FLAGS -pedantic -Wno-unused-parameter -Wdeprecated-declarations 
        -Wmissing-field-initializers
        # -v
        # -g -O0 # Equivalent to debug qualifier, for testing
        -pthread 
)
cet_make_exec(NAME      dtc_frontend
              SOURCE    dtc_frontend.cc dtc_driver.cc 
                        $ENV{MIDASSYS}/drivers/bus/null.cxx
              LIBRARIES frontends mu2e_pcie_utils::DTCInterface
                        mfe midas 
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
