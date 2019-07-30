find_ups_product(icarus_artdaq_base)
find_ups_product(artdaq_core)
find_ups_product(sbndaq_artdaq_core)

simple_plugin( DaqDecoderIcarusPMT module
  sbndaq-artdaq-core_Overlays_ICARUS 
  
  ${LARDATAOBJ}
  lardataobj_RawData
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

