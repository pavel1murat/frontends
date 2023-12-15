# ----------------------------------------------------------------------
# Build this project's executables:

include_directories($ENV{TRACE_INC})
include_directories($ENV{MIDASSYS}/include)
include_directories($ENV{MIDASSYS}/drivers)
include_directories(${PostgreSQL_INCLUDE_DIRS})

link_directories($ENV{MIDASSYS}/lib)

cet_set_compiler_flags(
        DIAGS VIGILANT
        # WERROR          
        NO_UNDEFINED
        EXTRA_FLAGS -pedantic -Wno-unused-parameter -Wdeprecated-declarations 
        -v
        # -g -O0 # Equivalent to debug qualifier, for testing
        -pthread 
)
cet_make_exec(NAME      scfe_example
              SOURCE    scfe_example.cc drv_example.cc 
                        $ENV{MIDASSYS}/drivers/class/multi.cxx
                        $ENV{MIDASSYS}/drivers/class/hv.cxx
                        #                    $ENV{MIDASSYS}/drivers/device/nulldev.cxx
                        $ENV{MIDASSYS}/drivers/bus/null.cxx
              LIBRARIES mfe midas 
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
