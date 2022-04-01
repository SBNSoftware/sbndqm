find_ups_product(artdaq_core)
find_ups_product(sbndaq_artdaq_core)

simple_plugin( ICARUSTPCDecoder module
  sbndqm_Decode_Mode
  sbndaq-artdaq-core_Overlays_ICARUS 
  ${LARDATAOBJ}
  lardataobj_RawData
	artdaq-core_Utilities
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

