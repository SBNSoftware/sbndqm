simple_plugin( SBNDTPCDecoder module
  sbndqm_Decode_Mode
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND_NevisTPC
  ${LARDATAOBJ}
  lardataobj_RawData
  artdaq_core::artdaq-core_Utilities
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}
)

install_headers()
install_fhicl()
install_source()

