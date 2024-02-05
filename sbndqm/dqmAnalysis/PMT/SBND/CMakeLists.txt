simple_plugin( CAENV1730StreamsSBND "module"

  lardataobj::RawData
  larana::OpticalDetector_OpHitFinder
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
  sbndqm::Decode_PMT_PMTDecodeData
  tpcAnalysis_SBN

)

add_subdirectory(fcl)
