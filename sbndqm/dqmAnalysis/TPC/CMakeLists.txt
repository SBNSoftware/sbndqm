find_ups_product( lardataobj v1_17_00 )

# filter modules for running online monitoring
add_subdirectory(OnlineFilters)
# subdirectory for SBND decoder
add_subdirectory(SBND_Decode)
# install fcl
add_subdirectory(fcl)

link_directories($ENV{FFTW_LIBRARY})

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
    Purity.cc
	LIBRARIES
		tpcAnalysis_MODE
		${LARDATAOBJ} 
		lardataobj_RawData
		lardata_Utilities
        	${ART_FRAMEWORK_IO_SOURCES}
       		${ART_FRAMEWORK_PRINCIPAL}
       		canvas
	        ${ART_FRAMEWORK_SERVICES_REGISTRY}
		${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
		art_Framework_Services_Optional_TFileService_service
		art_Framework_Services_FileServiceInterfaces
		${ART_UTILITIES}
		${FHICLCPP}
		${ART_FRAMEWORK_CORE}
		${ART_FRAMEWORK_PRINCIPAL}
		${ROOT_BASIC_LIB_LIST} 
		fftw3
                           larcore_Geometry_Geometry_service
                           larcorealg_Geometry
                           lardataobj_Simulation
                           lardata_Utilities
                           lardataobj_RawData
                           lardata_DetectorInfoServices_DetectorClocksServiceStandard_service
                           nutools_RandomUtils_NuRandomService_service
                           ${ART_FRAMEWORK_CORE}
                           ${ART_FRAMEWORK_PRINCIPAL}
                           ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
                           ${ART_FRAMEWORK_SERVICES_OPTIONAL_RANDOMNUMBERGENERATOR_SERVICE}
                           canvas
                           ${MF_MESSAGELOGGER}
                           ${MF_UTILITIES}
                           ${FHICLCPP}
                           cetlib cetlib_except
                           ${CLHEP}
                           ${ROOT_BASIC_LIB_LIST}
		)

simple_plugin( OfflineAnalysis module
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
  ${ART_FRAMEWORK_SERVICES_OPTIONAL_TFILESERVICE_SERVICE}
)


install_headers()
install_fhicl()
install_source()



