

cet_build_plugin( CAENV1730StreamsSBND art::module
  LIBRARIES
  lardataobj::RawData
  larana::OpticalDetector_OpHitFinder
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  sbndqm::Decode_PMT_PMTDecodeData
  tpcAnalysis_SBN

)

add_subdirectory(fcl)
