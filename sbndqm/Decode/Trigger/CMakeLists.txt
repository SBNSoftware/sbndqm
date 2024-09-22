art_make_library(SUBDIRS detail)

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
  sbndqm_Decode_Trigger
)

install_headers()
install_fhicl()
install_source()
