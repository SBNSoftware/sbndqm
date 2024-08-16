cet_build_plugin( PMTSunsetAnalyzer art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  lardataobj::RawData
  sbndqm::Decode_PMT_PMTDecodeData

  ROOT::Core
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
)

cet_build_plugin( TPCSunsetAnalyzer art::module
  LIBRARIES
  lardataobj::RawData
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  tpcAnalysis_SBN
  art::Utilities
  fhiclcpp::fhiclcpp
  art::Framework_Core
  ROOT::Core
  art::Framework_Principal
  art::Framework_Services_Registry
  art_root_io::tfile_support
  art_root_io::TFileService_service
  art_root_io::art_root_io
  art::Persistency_Common
  art::Framework_IO_Sources
)

install_headers()
install_fhicl()
install_source()
