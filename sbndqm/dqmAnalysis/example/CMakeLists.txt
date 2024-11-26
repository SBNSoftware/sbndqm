cet_build_plugin( ExampleTimeStream art::module
  LIBRARIES
  art::Framework_Services_Registry
  ROOT::Core
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online::storage
)

cet_build_plugin( ExampleDatabaseStorage art::module
  LIBRARIES
  art::Framework_Services_Registry
  ROOT::Core
  sbndaq_online::storage
  sbndaq_online::hiredis	
  sbndaq_online::redis_connection
)

cet_build_plugin( ExampleAlarm art::module
  LIBRARIES
  art::Framework_Services_Registry
  ROOT::Core
  sbndaq_online::storage
  sbndaq_online::hiredis	
  sbndaq_online::redis_connection
)

install_source()
install_headers()
install_fhicl()
