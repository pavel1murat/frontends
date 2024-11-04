include(BuildPlugins)

include(artdaq::commandableGenerator)

install_headers()
install_source()

set(MIDAS_LIBS "")
if( DEFINED $ENV{MIDASSYS})
    message(MIDASSYS defined: $ENV{MIDASSYS})
    set(MIDAS_LIBS midas)
endif()

add_compile_definitions(USE_MIDAS)

include_directories(${MIDAS_INSTALL_DIR} ${MIDAS_INSTALL_DIR}/include)
link_directories(${MIDAS_INSTALL_DIR}/lib)
#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
# function(get_all_targets var)
#     set(targets)
#     get_all_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
#     set(${var} ${targets} PARENT_SCOPE)
# endfunction()
# 
# macro(get_all_targets_recursive targets dir)
#     get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
#     foreach(subdir ${subdirectories})
#         get_all_targets_recursive(${targets} ${subdir})
#     endforeach()
# 
#     get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
#     list(APPEND ${targets} ${current_targets})
# endmacro()
# 
# get_all_targets(all_targets)
# message("All targets: ${all_targets}")

cet_build_plugin(TrackerBRDR artdaq::commandableGenerator 
                 LIBRARIES REG
                 otsdaq_mu2e_tracker::otsdaq-mu2e-tracker_Ui
                 artdaq_core_mu2e::artdaq-core-mu2e_Overlays 
                 canvas::canvas 
                 mu2e_pcie_utils::DTCInterface
                 frontends::utils
                 ${XMLRPC_LIBRARIES}
#                 ${MIDAS_LIBRARIES}
                  midas
)

target_include_directories(brdr_TrackerBRDR_generator PRIVATE ${XMLRPC_INCLUDE_DIRS})
