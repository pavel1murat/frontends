#------------------------------------------------------------------------------
# Build this project's executables
#------------------------------------------------------------------------------
find_package(messagefacility REQUIRED)

include_directories($ENV{TRACE_INC})
include_directories($ENV{MIDASSYS}/include)
include_directories($ENV{MIDASSYS})
include_directories(${PostgreSQL_INCLUDE_DIRS})

link_directories($ENV{MIDASSYS}/lib)

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

cet_make_exec(NAME      roc_frontend
              SOURCE    roc_frontend.cc roc_driver.cc 
                        $ENV{MIDASSYS}/drivers/class/multi.cxx
                        $ENV{MIDASSYS}/drivers/bus/null.cxx
              LIBRARIES mu2e_pcie_utils::DTCInterface
                        mfe midas 
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
