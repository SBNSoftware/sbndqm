simple_plugin( CAENV1730Streams "module"

  art_Utilities
  canvas

  art_Framework_Services_Registry

  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${ART_FRAMEWORK_CORE}

  ${ROOT_BASIC_LIB_LIST}

  art_root_io_tfile_support
  art_root_io_TFileService_service

  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common 

  artdaq_core::artdaq-core_Utilities
  artdaq_utilities::artdaq-utilities_Plugins

  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis

  sbndqm_Decode_PMT_PMTDecodeData

  larana_OpticalDetector_OpHitFinder

)

add_subdirectory(fcl)
