cet_build_plugin( PMTAnalyzer art::module
  LIBRARIES
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  lardataobj::RawData
  sbndqm::Decode_PMT_PMTDecodeData
)

cet_build_plugin( TPCAnalyzer art::module
  LIBRARIES
  lardataobj::RawData
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  tpcAnalysis_SBN
)

install_headers()
install_fhicl()
install_source()