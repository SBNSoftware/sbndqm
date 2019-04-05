find_ups_product( lardataobj)

include_directories($ENV{FFTW_LIBRARY})

find_library(FFTW fftw3 HINTS $ENV{FFTW_LIBRARY})

# filter modules for running online monitoring
add_subdirectory(OnlineFilters)

# check if we have sbnddaq-datatypes locally
# TODO: Better way to do this?????
if( DEFINED ENV{SBNDDAQ_DATATYPES_VERSION} )
# subdirectory for SBND decoder
  add_subdirectory(SBNDDecode)
endif()

if (DEFINED ENV{ICARUS_ARTDAQ_BASE_VERSION} )
    add_subdirectory(ICARUSDecode)
endif()

# install fcl
add_subdirectory(fcl)

art_dictionary(DICTIONARY_LIBRARIES tpcAnalysis_SBN)

cet_make_library( LIBRARY_NAME tpcAnalysis_MODE
	SOURCE
		Mode.cc
)

cet_make_library( LIBRARY_NAME tpcAnalysis_SBN
	SOURCE  Analysis.cc
		FFT.cc
		Noise.cc
		PeakFinder.cc
		ChannelData.cc
	LIBRARIES
		tpcAnalysis_MODE
		${LARDATAOBJ} 
		lardataobj_RawData
		lardata_Utilities
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
       		canvas
		art_Framework_Services_FileServiceInterfaces
		${ART_UTILITIES}
		${FHICLCPP}
		${ROOT_BASIC_LIB_LIST} 
		${FFTW}
                           larcore_Geometry_Geometry_service
                           larcorealg_Geometry
                           lardataobj_Simulation
                           lardata_Utilities
                           lardataobj_RawData
                           lardata_DetectorInfoServices_DetectorClocksServiceStandard_service
                           nutools_RandomUtils_NuRandomService_service
                           ${ART_FRAMEWORK_CORE}
                           ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR_SERVICE}
                           canvas
                           ${MF_MESSAGELOGGER}
                           ${MF_UTILITIES}
                           ${FHICLCPP}
                           cetlib cetlib_except
                           ${CLHEP}
                           ${ROOT_BASIC_LIB_LIST}
			${Boost_SYSTEM_LIBRARY}
)

simple_plugin( OfflineAnalysis module
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
)

simple_plugin( OnlineAnalysis module
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL}
                        ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
  artdaq-utilities_Plugins
  sbndqm_redis_config
)

install_headers()
install_fhicl()
install_source()



