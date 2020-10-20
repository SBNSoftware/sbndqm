simple_plugin( ExampleTimeStream "module"
  art_Framework_Services_Registry
#  art_Framework_Services_Optional
  ${ROOT_BASIC_LIB_LIST}
  artdaq-utilities_Plugins
  sbndaq_online_storage
)

simple_plugin( ExampleDatabaseStorage "module"
  art_Framework_Services_Registry
	#  art_Framework_Services_Optional
  ${ROOT_BASIC_LIB_LIST}
  sbndaq_online_storage
  sbndaq_online_hiredis	
  sbndaq_online_redis_connection
)

simple_plugin( ExampleAlarm "module"
  art_Framework_Services_Registry
	#  art_Framework_Services_Optional
  ${ROOT_BASIC_LIB_LIST}
  sbndaq_online_storage
  sbndaq_online_hiredis	
  sbndaq_online_redis_connection
)


