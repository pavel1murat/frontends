#
# include_directories($ENV{ROOTSYS}/include)
# link_directories($ENV{ROOTSYS}/lib)
#

message("--- frontends/cal: PROJECT_NAME: ${PROJECT_NAME}")

set(subpkg "cal"                  )
set(dict   "${PROJECT_NAME}_${subpkg}_dict")
set(lib    "${PROJECT_NAME}_${subpkg}"     )

include_directories(
  $ENV{SPACK_VIEW}/include
  $ENV{ROOTSYS}/include/root
  ${CMAKE_CURRENT_SOURCE_DIR}        # /${subpkg}
  $ENV{SPACK_ENV}/${PROJECT_NAME} 
)

# link_directories($ENV{ROOTSYS}/lib)

ROOT_GENERATE_DICTIONARY(${dict}
  DtcInterfaceCal.hh
  LINKDEF Linkdef.h)

set_property(SOURCE ${dict}.cxx PROPERTY generated)

add_library(${lib} SHARED
  DtcInterfaceCal.cc
  DtcInterfaceCal_print.cc
  ${dict}.cxx
)
add_library(frontends::cal ALIAS ${lib})

target_link_libraries(${lib} otsdaq-mu2e-tracker::Ui ROOT::Core)

cet_add_compiler_flags(-g -O0)

install(FILES       DtcInterfaceCal.hh
        DESTINATION include/frontends/${subpkg})

install(TARGETS ${lib})
# install_headers()
# install_source()
# install(DIRECTORY .  DESTINATION include/${PROJECT_NAME})
