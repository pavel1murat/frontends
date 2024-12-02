# ----------------------------------------------------------------------
# Build this project's executables:

find_package(messagefacility REQUIRED)
#------------------------------------------------------------------------------
# link directories 
#------------------------------------------------------------------------------
# include_directories($ENV{MRB_SOURCE})
include_directories($ENV{TRACE_INC})
include_directories(${MIDAS_INSTALL_DIR})
include_directories(${MIDAS_INSTALL_DIR}/include)
include_directories(${PostgreSQL_INCLUDE_DIRS})
#------------------------------------------------------------------------------
# library directories 
#------------------------------------------------------------------------------
link_directories($ENV{TRACE_LIB})
link_directories(${MIDAS_INSTALL_DIR}/lib)
link_directories($ENV{MESSAGEFACILITY_LIB})
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
# cet_make_exec(NAME      dtc_frontend
#               SOURCE    dtc_frontend.cc
#                         dtc_driver.cc
#                         emucfo_driver.cc
#                         ${MIDAS_INSTALL_DIR}/drivers/bus/null.cxx
#               LIBRARIES frontends_utils
#                         otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
#                         mu2e_pcie_utils::DTCInterface
#                         mfe midas
#                         stdc++ m util pthread rt
# )

cet_make_exec(NAME      trk_dtc_frontend
              SOURCE    trk_dtc_frontend.cc
                        trk_dtc_driver.cc
#                        emucfo_driver.cc
                        #                        ../utils/my_mfe.cxx
                        ${MIDAS_INSTALL_DIR}/drivers/bus/null.cxx
              LIBRARIES frontends_utils
                        otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
                        mu2e_pcie_utils::DTCInterface
                        mfe
                        midas
                        stdc++ m util pthread rt
)

cet_make_exec(NAME      trk_dtc_ctl_frontend
              SOURCE    trk_dtc_ctl_frontend.cc
              LIBRARIES frontends_utils
                        otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
                        mu2e_pcie_utils::DTCInterface
                        mfe
                        midas
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
