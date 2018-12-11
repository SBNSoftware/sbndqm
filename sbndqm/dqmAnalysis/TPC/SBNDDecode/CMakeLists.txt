find_ups_product(artdaq_core)
simple_plugin( DaqDecoder module
  tpcAnalysis_MODE
  sbnddaq-datatypes_Overlays 
  sbnddaq-datatypes_NevisTPC
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

