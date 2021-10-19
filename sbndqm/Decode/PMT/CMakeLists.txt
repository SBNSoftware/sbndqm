simple_plugin( DaqDecoderIcarusPMT "module"
  
  art_Utilities
  canvas
  
  artdaq_core::artdaq-core_Data  
  artdaq_core::artdaq-core_Utilities
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_ICARUS 
  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}

  cetlib cetlib_except

  sbndqm_Decode_PMT_PMTDecodeData

)

install_headers()
install_fhicl()
install_source()

add_subdirectory(PMTDecodeData)

