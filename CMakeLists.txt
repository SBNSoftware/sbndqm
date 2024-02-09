# ======================================================================
#  sbndqm main build file
#
#  cd .../path/to/build/directory
#  source .../path/to/sbndqm/ups/setup_for_development <-d|-p>
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================


# use cmake 2.8 or later
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

find_package(cetmodules 3.20.00 REQUIRED)
project(sbndqm VERSION 1.03.00 LANGUAGES CXX)

message(STATUS "\n\n  ==========================   ${PROJECT_NAME}   ==========================")

# cetbuildtools contains our cmake modules
#find_package(cetbuildtools REQUIRED)

#list(APPEND CMAKE_MODULE_PATH $ENV{CANVAS_ROOT_IO_DIR}/Modules $ENV{ART_DIR}/Modules )

include(CetCMakeEnv)
cet_cmake_env()


cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  ALLOW_DEPRECATIONS
  EXTRA_FLAGS -std=c++17 )

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( larsoft REQUIRED )
find_ups_product( lardataobj REQUIRED )
find_ups_product( nutools REQUIRED )
find_ups_product( art REQUIRED )
find_ups_product( art_root_io REQUIRED )
#find_ups_product( cetbuildtools )
find_ups_product( sbndaq_artdaq_core EXPORT REQUIRED)
find_ups_product( artdaq_utilities EXPORT REQUIRED)
find_ups_product( jsoncpp REQUIRED )
find_ups_product( artdaq_core EXPORT REQUIRED)
find_ups_product( fftw REQUIRED )
find_ups_product( sbndaq_online REQUIRED )
find_ups_product( sbnobj REQUIRED )
find_ups_product( sbnanaobj REQUIRED )
find_ups_product( sbncode REQUIRED)
find_ups_product( TRACE REQUIRED)

find_package( ROOT REQUIRED )
find_package( Boost COMPONENTS system filesystem REQUIRED )

#find_ups_boost()
#find_ups_root()

# macros for dictionary and simple_plugin
include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)

# ADD SOURCE CODE SUBDIRECTORIES HERE
add_subdirectory(sbndqm)

add_subdirectory(fcl)

# tests
add_subdirectory(test)

# tools - utility scripts
add_subdirectory(tools)

# ups - table and config files
#add_subdirectory(ups)

# packaging utility
cet_cmake_config()
#include(UseCPack)
