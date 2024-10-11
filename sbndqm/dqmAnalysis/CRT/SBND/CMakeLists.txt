simple_plugin( BernCRTdqmSBND module
  art::Framework_Services_Registry
  ${ROOT_BASIC_LIB_LIST}
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common
  artdaq_core::artdaq-core_Data
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online_storage
  sbndaq_online_hiredis
  sbndqm_Decode_Mode
)

