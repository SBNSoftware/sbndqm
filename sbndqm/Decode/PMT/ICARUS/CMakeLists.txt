cet_build_plugin( DaqDecoderIcarusPMT art::module
  LIBRARIES
  art::Utilities
  canvas::canvas  
  artdaq_core::artdaq-core_Data  
  artdaq_core::artdaq-core_Utilities
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_ICARUS 
  lardataobj::RawData
  fhiclcpp::fhiclcpp
  art::Framework_Core
  messagefacility::MF_MessageLogger
  messagefacility::headers
  ROOT::Core
  cetlib::cetlib
  cetlib_except::cetlib_except
  sbndqm_Decode_PMT_PMTDecodeData
)

install_headers()
install_fhicl()
install_source()

