# ----------------------------------------------------------------------
# Build this project's executables:
#------------------------------------------------------------------------------
include_directories($ENV{SPACK_ENV})
include_directories($ENV{TRACE_INC})
include_directories(${MIDAS_INSTALL_DIR})
include_directories(${MIDAS_INSTALL_DIR}/include)
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

cet_make_exec(NAME trk_cfg_frontend
  SOURCE
  trk_cfg_frontend.cc
  TEqTracker.cc
  TEqTracker_ExecuteDtcCommand.cc
  TEqTracker_PulserOn.cc
  TEqTracker_PulserOff.cc
  TEqTracker_PanelPrintStatus.cc
  TEqTracker_ResetOutput.cc
  TEqTracker_ResetStationLV.cc
  LIBRARIES
  frontends_utils
  midas
  stdc++ m util pthread rt
)

install(PROGRAMS
        mu2e_config_fe.py
        DESTINATION bin)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()

