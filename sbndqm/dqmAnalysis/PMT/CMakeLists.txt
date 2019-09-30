add_subdirectory(DaqDecodePMT)
simple_plugin( CAENV1730Stream "module"
  art_Framework_Services_Registry
#  art_root_io_TFileService_service
  ${ROOT_BASIC_LIB_LIST}
  sbndaq-artdaq-core_Overlays_Common 
  tpcAnalysis_MODE
	artdaq-core_Utilities
  artdaq-utilities_Plugins
  sbndaq-redis-plugin_redis_metric
  sbndqm_redis_config
)

simple_plugin( CAENV1730WaveformAnalysis "module"
  art_Framework_Services_Registry
  ${ROOT_BASIC_LIB_LIST}
  art_root_io_tfile_support
  art_root_io_TFileService_service
  sbndaq-artdaq-core_Overlays_Common 
  tpcAnalysis_MODE
  artdaq-core_Utilities
  artdaq-utilities_Plugins
  sbndaq-redis-plugin_redis_metric
  sbndqm_redis_config
)
