add_subdirectory(DaqDecodePMT)

art_dictionary(DICTIONARY_LIBRARIES pmtAnalysis_SBN)

cet_make_library( 

  LIBRARY_NAME pmtAnalysis_SBN
  
  SOURCE  
    PMTWaveform.cc
    PMTDigitizerInfo.cc
  
  LIBRARIES
    lardataobj_RawData
    ${ART_UTILITIES}
    ${ART_FRAMEWORK_PRINCIPAL}
    ${ART_FRAMEWORK_SERVICES_REGISTRY}
    ${ART_FRAMEWORK_SERVICES_BASIC}
    ${ART_FRAMEWORK_CORE}
    canvas
    ${MF_MESSAGELOGGER}
    ${MF_UTILITIES}
    ${FHICLCPP}
    cetlib cetlib_except
    ${CLHEP}
    ${ROOT_BASIC_LIB_LIST}
)

simple_plugin( CAENV1730Stream "module"
  art_Framework_Services_Registry
  ${ROOT_BASIC_LIB_LIST}
  sbndaq-artdaq-core_Overlays_Common 
	artdaq-core_Utilities
  artdaq-utilities_Plugins
  sbndqm_Decode_Mode
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
)

simple_plugin( CAENV1730WaveformAnalysis "module"
  art_Framework_Services_Registry
  ${ROOT_BASIC_LIB_LIST}
  art_root_io_tfile_support
  art_root_io_TFileService_service
  sbndaq-artdaq-core_Overlays_Common 
  artdaq-core_Utilities
  artdaq-utilities_Plugins
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
)
