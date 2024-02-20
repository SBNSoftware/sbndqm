art_make_library(LIBRARY_NAME TriggerParser
        SUBDIRS detail
        LIBRARIES
                messagefacility::MF_MessageLogger
                fhiclcpp::fhiclcpp
                cetlib_except::cetlib_except
                cetlib::cetlib
                ROOT::RIO
                ROOT::Core
                art::Framework_Services_Registry
)

simple_plugin( DaqDecoderIcarusTrigger "module"
  
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
  TriggerParser
)

install_headers()
install_fhicl()
install_source()
