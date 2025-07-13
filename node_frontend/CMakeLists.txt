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
                        TEquipmentNode.cc
                        TEquipmentNode_Artdaq.cc
                        TEquipmentNode_Disk.cc
                        TEquipmentNode_Dtc.cc
                        TEquipmentNode_Rpc.cc
                        TEquipmentNode_Rpc_ControlRoc_Read.cc
                        TEquipmentNode_Rpc_ControlRoc_Rates.cc
                        TEquipmentNode_Rpc_ControlRoc_ReadDDR.cc
                        TEquipmentNode_Rpc_ControlRoc_SetThresholds.cc
                        TEquipmentNode_Rpc_LoadThresholds.cc
                        TEquipmentNode_Rpc_ControlRoc_MeasureThresholds.cc
                        TEquipmentNode_Rpc_SetCalDac.cc
                        TEquipmentNode_Rpc_DumpSettings.cc
              LIBRARIES frontends_utils
                        otsdaq-mu2e-tracker::Ui
                        otsdaq-mu2e-crv::Ui
                        mu2e-pcie-utils::DTCInterface
                        midas
                        stdc++ m util pthread rt
)

# install_fhicl(SUBDIRS fcl)
# install_headers()
# install_source()
