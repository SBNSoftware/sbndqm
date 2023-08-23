simple_plugin( SPECTDCStreams "module"

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
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND 
  sbnobj::SBND_Timing

  artdaq_core::artdaq-core_Utilities
  artdaq_utilities::artdaq-utilities_Plugins

  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis

)

add_subdirectory(fcl)
