find_ups_product(icarus_artdaq_base)
find_ups_product(artdaq_core)
simple_plugin( DaqDecoderIcarus module
  tpcAnalysis_MODE
  icarus-artdaq-core_Overlays 
  
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

