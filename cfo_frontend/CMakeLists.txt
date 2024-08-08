# ----------------------------------------------------------------------
# Build this project's executables:

# find_package(messagefacility REQUIRED)
#------------------------------------------------------------------------------
# link directories 
#------------------------------------------------------------------------------
include_directories(${MIDAS_INSTALL_DIR}/include)
include_directories(${MIDAS_INSTALL_DIR})
#------------------------------------------------------------------------------
# library directories 
#------------------------------------------------------------------------------
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
      
cet_make_exec(NAME      cfo_frontend
              SOURCE    cfo_frontend.cc 
                        cfo_gen_driver.cc 
                        cfo_mon_driver.cc 
                        ${MIDAS_INSTALL_DIR}/drivers/bus/null.cxx
              LIBRARIES frontends 
                        otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
                        mu2e_pcie_utils::CFOInterface
                        mu2e_pcie_utils::DTCInterface
                        mfe midas 
                        stdc++ m util pthread rt
)

# cet_make_exec(NAME      cfo_emu_frontend
#               SOURCE    cfo_emu_frontend.cc 
#                         cfo_mon_driver.cc 
#                         ${MIDAS_INSTALL_DIR}/drivers/bus/null.cxx
#               LIBRARIES frontends 
#                         otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
#                         mu2e_pcie_utils::CFOInterface
#                         mu2e_pcie_utils::DTCInterface
#                         mfe midas 
#                         stdc++ m util pthread rt
# )
                      
cet_make_exec(NAME      cfo_ext_frontend
              SOURCE    cfo_ext_frontend.cc 
                        cfo_mon_driver.cc 
                        ${MIDAS_INSTALL_DIR}/drivers/bus/null.cxx
              LIBRARIES frontends 
                        otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
                        mu2e_pcie_utils::CFOInterface
                        mu2e_pcie_utils::DTCInterface
                        mfe midas 
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
