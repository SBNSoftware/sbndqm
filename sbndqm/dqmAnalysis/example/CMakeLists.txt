simple_plugin( ExampleTimeStream "module"
  art_Framework_Services_Registry
#  art_Framework_Services_Optional
  ${ROOT_BASIC_LIB_LIST}
  artdaq-utilities_Plugins
  sbndqm_redis_config
)

simple_plugin( ExampleDatabaseStorage "module"
  art_Framework_Services_Registry
#  art_Framework_Services_Optional
  ${ROOT_BASIC_LIB_LIST}
  sbndqm_redis_config
  sbndaq_online_storage
)

