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
)

