#
# include_directories($ENV{ROOTSYS}/include)
# link_directories($ENV{ROOTSYS}/lib)
#

message("--- frontends/crv: PROJECT_NAME: ${PROJECT_NAME}")

set(subpkg "crv"                  )
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
  DtcInterfaceCrv.hh
  LINKDEF Linkdef.h)

set_property(SOURCE ${dict}.cxx PROPERTY generated)

add_library(${lib} SHARED
  DtcInterfaceCrv.cc
  DtcInterfaceCrv_print.cc
  ${dict}.cxx
)
add_library(frontends::crv ALIAS ${lib})

target_link_libraries(${lib} otsdaq-mu2e-tracker::Ui ROOT::Core)

cet_add_compiler_flags(-g -O0)

install(FILES       DtcInterfaceCrv.hh
        DESTINATION include/frontends/crv)

install(TARGETS ${lib})
# install_headers()
# install_source()
# install(DIRECTORY .  DESTINATION include/${PROJECT_NAME})
