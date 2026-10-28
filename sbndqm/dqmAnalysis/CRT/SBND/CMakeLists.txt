simple_plugin( BernCRTdqmSBND module
  art::Framework_Services_Registry
  ${ROOT_BASIC_LIB_LIST}
  sbndaq_artdaq_core::sbndaq-artdaq-core_Overlays_Common
  sbndaq_artdaq_core::sbndaq-artdaq-core_Obj_SBND
  artdaq_core::artdaq-core_Data
  artdaq_utilities::artdaq-utilities_Plugins
  sbndaq_online::storage
  sbndaq_online::hiredis
  sbndqm_Decode_Mode
)

add_subdirectory(fcl)
