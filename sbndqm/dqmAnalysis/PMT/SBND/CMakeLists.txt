simple_plugin( CAENV1730StreamsSBND "module"

  lardataobj::RawData
  larana::OpticalDetector_OpHitFinder
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
  sbndqm::Decode_PMT_PMTDecodeData
  tpcAnalysis_SBN
  
)

simple_plugin( CAENV1730FlashMetricsSBND "module" 
  sbndaq_artdaq_core::sbndaq-artdaq-core_Obj_SBND
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
)

add_subdirectory(fcl)
