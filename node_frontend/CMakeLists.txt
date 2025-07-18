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

cet_make_exec(NAME      node_frontend
              SOURCE    node_frontend.cc
                        TEquipmentManager.cc
                        TEquipmentManager_Rpc.cc
                        TEqCrvDtc.cc
                        TEqArtdaq.cc
                        TEqDisk.cc
#
                        TEqTrkDtc.cc
                        TEqTrkDtc_Commands.cc
#                        TEqTrkDtc_Commands_LoadThresholds.cc
#                        TEqTrkDtc_Commands_MeasureThresholds.cc
#                        TEqTrkDtc_Commands_Rates.cc
#                        TEqTrkDtc_Commands_Read.cc
#                        TEqTrkDtc_Commands_SetThresholds.cc
                        TEqTrkDtc_ProcessCommand.cc

                        TMu2eEqBase.cc


              LIBRARIES frontends_utils
                        otsdaq-mu2e-tracker::Ui
                        mu2e-pcie-utils::DTCInterface
                        midas
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
