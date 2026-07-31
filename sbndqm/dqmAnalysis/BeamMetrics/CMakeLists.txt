simple_plugin( CAENV1730FlashMetricsSBND "module"
  sbndaq_artdaq_core::sbndaq-artdaq-core_Obj_SBND
  sbndaq_online::storage
  sbndaq_online::redis_connection
  sbndaq_online::hiredis
  artdaq_utilities::artdaq-utilities_Plugins
)

install_fhicl()
