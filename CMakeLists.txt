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
cmake_minimum_required (VERSION 3.14)

project(sbndqm)


find_package(cetbuildtools REQUIRED)
list(APPEND CMAKE_MODULE_PATH $ENV{ART_DIR}/Modules)

include(CetCMakeEnv)
cet_cmake_env()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
#  EXTRA_FLAGS -pedantic
  EXTRA_CXX_FLAGS -std=c++17 )

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_ups_product( larsoft v1_00_00 )
find_ups_product( nutools v1_00_00 )
find_ups_product( art v3_02_05 )
find_ups_product( art_root_io v1_00_05 )
find_ups_product( cetbuildtools v7_13_02 )
find_ups_product( sbndaq_artdaq_core v0_03_00 )
find_ups_product( artdaq_utilities v1_05_02 )
find_ups_product( jsoncpp v1_00_00 )
find_ups_product( artdaq_core v3_05_07 )
find_ups_product( fftw )
#find_ups_product( sbndaq_decode v0_03_00 )
find_ups_product( sbndaq_online v0_03_00 )
find_ups_product( sbnobj )
find_ups_product( sbncode )
# include_directories($ENV{JSONCPP_LIB})

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
add_subdirectory(ups)

# packaging utility
include(UseCPack)
