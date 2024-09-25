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

find_package(cetmodules 3.24.01 REQUIRED)
project(sbndqm VERSION 1.03.00 LANGUAGES CXX)

message(STATUS "\n\n  ==========================   ${PROJECT_NAME}   ==========================")

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  ALLOW_DEPRECATIONS
  EXTRA_FLAGS -Wno-pedantic -Wno-dev )

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_package( larsoft REQUIRED )
find_package( lardataobj REQUIRED )
find_package( nutools REQUIRED )
find_package( art REQUIRED )
find_package( art_root_io REQUIRED )
find_package( sbndaq_artdaq_core REQUIRED)
find_package( artdaq_utilities REQUIRED)
find_package( jsoncpp REQUIRED )
find_package( artdaq_core EXPORT REQUIRED)
find_package( sbndaq_online EXPORT REQUIRED )
find_package( sbnobj REQUIRED )
find_package( sbnanaobj REQUIRED )
find_package( sbncode REQUIRED)
find_package( TRACE REQUIRED)

find_package( FFTW3 REQUIRED )
find_package( ROOT REQUIRED )
find_package( Boost COMPONENTS system filesystem REQUIRED )

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

# packaging utility
cet_cmake_config()
