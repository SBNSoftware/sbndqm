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

cet_build_plugin( DaqDecoderIcarusTrigger art::module
  LIBRARIES
  lardataobj::RawData
  art::Utilities
  canvas::canvas  
  art::Framework_Core
  messagefacility::MF_MessageLogger
  messagefacility::headers
  ROOT::Core
  cetlib::cetlib
  cetlib_except::cetlib_except
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common 
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_ICARUS
  artdaq_core::artdaq-core_Utilities
  artdaq_utilities::artdaq-utilities_Plugins
  TriggerParser
)

install_headers()
install_fhicl()
install_source()
