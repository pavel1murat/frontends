#------------------------------------------------------------------------------
# Build this project's executables
#------------------------------------------------------------------------------
find_package(messagefacility REQUIRED)

include_directories($ENV{TRACE_INC})
include_directories(${MIDAS_INSTALL_DIR}/include)
include_directories(${MIDAS_INSTALL_DIR})
include_directories(${PostgreSQL_INCLUDE_DIRS})

link_directories(${MIDAS_INSTALL_DIR}/lib)

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

cet_make_exec(NAME      roc_crv_frontend
              SOURCE    roc_crv_frontend.cc roc_crv_driver.cc 
                        #$ENV{MIDASSYS}/drivers/class/multi.cxx
                        cd_multi.cxx
                        ${MIDAS_INSTALL_DIR}/drivers/bus/null.cxx
              LIBRARIES mu2e_pcie_utils::DTCInterface
                        mfe midas 
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
