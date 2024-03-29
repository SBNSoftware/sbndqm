include_directories(${FFTW_INCLUDE_DIR})
#cet_find_library(FFTW_LIBRARY NAMES fftw3 fftw3-3 PATHS ENV FFTW_LIBRARY )

# filter modules for running online monitoring
add_subdirectory(OnlineFilters)

# install fcl
add_subdirectory(fcl)

art_dictionary(DICTIONARY_LIBRARIES tpcAnalysis_SBN)

cet_make_library( LIBRARY_NAME tpcAnalysis_SBN
	SOURCE  Analysis.cc
		FFT.cc
		Noise.cc
		PeakFinder.cc
		ChannelData.cc
	LIBRARIES
	        sbndqm_Decode_Mode
		${LARDATAOBJ} 
		lardataobj_RawData
		lardata_Utilities
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
			art_root_io::tfile_support
			art_root_io::TFileService_service
			art_root_io::art_root_io
                        art_Persistency_Common
			art_Persistency_Provenance
        	${ART_FRAMEWORK_IO_SOURCES}
       		canvas
		art_Framework_Services_FileServiceInterfaces
		${ART_UTILITIES}
		${FHICLCPP}
		${ROOT_BASIC_LIB_LIST} 
		FFTW_LIBRARY
                           larcore_Geometry_Geometry_service
                           larcorealg_Geometry
                           lardataobj_Simulation
                           lardata_Utilities
                           lardataobj_RawData
                           lardata_DetectorInfoServices_DetectorClocksServiceStandard_service
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

cet_build_plugin( OfflineAnalysis art::module
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
			art_root_io::tfile_support
			art_root_io::TFileService_service
			art_root_io::art_root_io
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
		${ROOT_BASIC_LIB_LIST} 
)

cet_build_plugin( TPCWaveformCreator art::module
  sbndaq_online_hiredis
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
			art_root_io::tfile_support
			art_root_io::TFileService_service
			art_root_io::art_root_io
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
)

cet_build_plugin( OnlineAnalysis art::module
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
			art_root_io::tfile_support
			art_root_io::TFileService_service
			art_root_io::art_root_io
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
  artdaq_utilities::artdaq-utilities_Plugins
)

cet_build_plugin( TPCPurityDQMSender art::module
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  sbnobj_Common_Analysis
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}#
			art_root_io::tfile_support#
			art_root_io::TFileService_service
			art_root_io::art_root_io
                        art_Persistency_Common
        	${ART_FRAMEWORK_IO_SOURCES}
  artdaq_utilities::artdaq-utilities_Plugins
)
cet_build_plugin( TPCWaveformAndFftRedis art::module
  tpcAnalysis_SBN
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
                        ${ART_FRAMEWORK_CORE}
                        ${ART_FRAMEWORK_PRINCIPAL}
                        ${ART_FRAMEWORK_SERVICES_REGISTRY}
                        ${ART_FRAMEWORK_SERVICES_BASIC}
                        art_root_io::tfile_support
                        art_root_io::TFileService_service
			art_root_io::art_root_io
                        art_Persistency_Common
                ${ART_FRAMEWORK_IO_SOURCES}
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online_storage
  sbndaq_online_hiredis
  sbndaq_online_redis_connection
)

install_headers()
install_fhicl()
install_source()



