simple_plugin( CAENV1730FlashMetricsSBND "module"
  sbndaq_artdaq_core::sbndaq-artdaq-core_Obj_SBND
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_SBND
  sbndaq_online_storage
  sbndaq_online_redis_connection
  sbndaq_online_hiredis
  sbndqm_dqmAnalysis_Utils
)

install_fhicl()
