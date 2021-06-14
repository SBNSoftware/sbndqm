find_ups_product(artdaq_core)
find_ups_product(sbndaq_artdaq_core)

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

simple_plugin( DaqDecoderIcarusPMT module
  
  art_Utilities
  canvas
  
  artdaq-core_Utilities
  sbndaq-artdaq-core_Overlays_COMMON
  sbndaq-artdaq-core_Overlays_ICARUS 

  ${LARDATAOBJ}
  lardataobj_RawData
  ${ART_UTILITIES}
  ${FHICLCPP}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${ART_FRAMEWORK_CORE}
  ${ROOT_BASIC_LIB_LIST}

  cetlib cetlib_except

  pmtAnalysis_SBN

)

install_headers()
install_fhicl()
install_source()

