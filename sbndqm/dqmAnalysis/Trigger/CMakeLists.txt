add_subdirectory(detail)
add_subdirectory(fcl)

simple_plugin( TriggerMonitor "module"
		art_Utilities
  		canvas
  		art_Framework_Services_Registry
  		${LARDATAOBJ}
  		lardataobj_RawData
  		lardataobj_Simulation
  		${ART_UTILITIES}
 		${FHICLCPP}
  		${MF_MESSAGELOGGER}
  		${MF_UTILITIES}
  		${ART_FRAMEWORK_CORE}
  		${ROOT_BASIC_LIB_LIST}
  		art_root_io_tfile_support
  		art_root_io_TFileService_service 
  		artdaq_core::artdaq-core_Data
  		artdaq_core::artdaq-core_Utilities
  		artdaq_utilities::artdaq-utilities_Plugins
  		sbndaq_online_storage
  		sbndaq_online_redis_connection
  		sbndaq_online_hiredis
  		sbndqm_Decode_PMT_PMTDecodeData
		sbndqm_dqmAnalysis_Trigger_detail
	)

install_headers()
install_source()
install_fhicl()
